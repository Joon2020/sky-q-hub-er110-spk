/*
<:copyright-BRCM:2010:DUAL/GPL:standard

   Copyright (c) 2010 Broadcom Corporation
   All Rights Reserved

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation (the "GPL").

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.


A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

:>
*/


//**************************************************************************
// File Name  : bcmeapi_runner.c
//
// Description: This is Linux network driver for Broadcom Ethernet controller
//
//**************************************************************************
#define VERSION     "0.1"
#define VER_STR     "v" VERSION " " __DATE__ " " __TIME__

#define _BCMENET_LOCAL_
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/in.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/init.h>
#include <asm/io.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/skbuff.h>
#include <linux/kthread.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/kmod.h>
#include <linux/rtnetlink.h>
#include <net/arp.h>
#include <board.h>
#include <spidevices.h>
#include <bcmnetlink.h>
#include <bcm_map_part.h>
#include <bcm_intr.h>
#include "linux/bcm_assert_locks.h"
#include <linux/stddef.h>
#include <asm/atomic.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/nbuff.h>
#include <linux/kernel_stat.h>
#ifdef CONFIG_BCM968500
#include "bl_lilac_brd.h"
#include "bl_lilac_eth_common.h"
#endif
#ifdef CONFIG_BCM96838
#include "phys_common_drv.h"
#endif
#include "boardparms.h"
#include "bcmenet_common.h"
#include "bcmenet.h"
#include "bcmenet_runner.h"
#include "bcmmii.h"
#include "ethsw.h"
#include "ethsw_phy.h"
#include "bcmsw.h"
#include <rdpa_api.h>
#include "ethswdefs.h"

/* Extern data */
extern enet_global_var_t global;
static bdmf_object_handle rdpa_cpu_obj;
rdpa_system_init_cfg_t init_cfg;
int wan_port_id;
extern extsw_info_t extSwInfo;
extern int vport_cnt;  /* number of vports: bitcount of Enetinfo.sw.port_map */

void bcmeapi_create_vport(struct net_device *dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
    pDevCtrl->default_txq = 0; /* BCM_GMP_MW_UNCLASSIFIED_TRAFFIC_RC; TBD */ 
}

static int cpu_meter_idx[2] = { BDMF_INDEX_UNASSIGNED, BDMF_INDEX_UNASSIGNED };
static void cpu_meter_idx_init(void)
{
    /* XXX: Scan and find available meters */
    cpu_meter_idx[rdpa_dir_ds] = 1;
    cpu_meter_idx[rdpa_dir_us] = 1;
}

static int reason2queue_cfg_skip(rdpa_cpu_reason_index_t *reason_cfg_idx, rdpa_cpu_reason_cfg_t *reason_cfg)
{
    /* Any third party already configured the reason? */
    if (reason_cfg && reason_cfg->queue != NETDEV_CPU_RX_QUEUE_ID)
        return 1;
    /* One of the reasons which we defenitely don't handle? */
    if ((reason_cfg_idx->reason == rdpa_cpu_rx_reason_oam)||
        (reason_cfg_idx->reason == rdpa_cpu_rx_reason_omci)||
        (reason_cfg_idx->reason >= rdpa_cpu_rx_reason_direct_queue_0 &&
        reason_cfg_idx->reason <= rdpa_cpu_rx_reason_direct_queue_7)
#if defined(ENET_EPON_CONFIG)
        || (reason_cfg_idx->reason == rdpa_cpu_rx_reason_etype_udef_2) ||
        (reason_cfg_idx->reason == rdpa_cpu_rx_reason_etype_udef_3)
#endif
       )
    {
        return 1;
    }
    return 0;
}

static int reason2meter_cfg_skip(rdpa_cpu_reason_index_t *reason_cfg_idx, rdpa_cpu_reason_cfg_t *reason_cfg)
{
    if (reason2queue_cfg_skip(reason_cfg_idx, reason_cfg))
        return 1; /* Reason is not trapped to NETDEV_CPU_RX_QUEUE_ID */

    switch (reason_cfg_idx->reason)
    {
        /* Traffic we do want to skip from being metered */
    case rdpa_cpu_rx_reason_local_ip:
    case rdpa_cpu_rx_reason_unknown_sa:
    case rdpa_cpu_rx_reason_unknown_da:
    case rdpa_cpu_rx_reason_bcast:
    case rdpa_cpu_rx_reason_ip_frag:
    case rdpa_cpu_rx_reason_non_tcp_udp:
    case rdpa_cpu_rx_reason_ip_flow_miss:
        return 0;
    default:
        break;
    }

    return 1;
}

static void _rdpa_cpu_set_reasons_to_meter(int cpu_meter_idx_ds, int cpu_meter_idx_us)
{
    rdpa_cpu_reason_index_t reason_cfg_idx = {BDMF_INDEX_UNASSIGNED, BDMF_INDEX_UNASSIGNED};
    rdpa_cpu_reason_cfg_t reason_cfg = {};
    int rc;

    while (!rdpa_cpu_reason_cfg_get_next(rdpa_cpu_obj, &reason_cfg_idx))
    {
        rdpa_cpu_reason_cfg_get(rdpa_cpu_obj, &reason_cfg_idx, &reason_cfg);
        if (reason2meter_cfg_skip(&reason_cfg_idx, &reason_cfg))
            continue;
        reason_cfg.meter = reason_cfg_idx.dir == rdpa_dir_ds ? cpu_meter_idx_ds : cpu_meter_idx_us;
        rc = rdpa_cpu_reason_cfg_set(rdpa_cpu_obj, &reason_cfg_idx, &reason_cfg);
        if (rc < 0)
            printk(KERN_ERR CARDNAME ": Error (%d) configuring CPU reason to meter\n", rc);
    }
}

/* This is the fibunachi series of rates. */
#define NUM_OF_SUGGESTED_RATES 9
static int recommended_rates[NUM_OF_SUGGESTED_RATES] = { 1000, 2000, 3000, 5000, 8000, 13000, 21000, 34000, 40000 };
static inline void _rdpa_cfg_cpu_meter(int increase_rate)
{
    int i, prev_rate_idx;
    rdpa_cpu_meter_cfg_t meter_cfg;
    rdpa_dir_index_t dir_idx;
    static int rate_idx = 1; /* For the first time, need 1 in order to be configured, in order to start from rate 0 */

    prev_rate_idx = rate_idx;
    if (increase_rate)
    {
        if (rate_idx == NUM_OF_SUGGESTED_RATES)
            return; /* skip configuration */
        rate_idx++;
    }
    else
    {
        if (!rate_idx)
            return; /* skip configuration */
        rate_idx--;
    }

    if (rate_idx == NUM_OF_SUGGESTED_RATES)
    {
        /* We reached the maximal meter and want to increase - remove the rate limiter */
        _rdpa_cpu_set_reasons_to_meter(BDMF_INDEX_UNASSIGNED, BDMF_INDEX_UNASSIGNED);
        return;
    }
    else if (prev_rate_idx == NUM_OF_SUGGESTED_RATES)
    {
        /* The only case when prev_rate_idx can be NUM_OF_SUGGESTED_RATES is when we didn't have a meter, and got a call
         * to decrease the rate. In this case, we want to put the meter back. */
        _rdpa_cpu_set_reasons_to_meter(cpu_meter_idx[rdpa_dir_ds], cpu_meter_idx[rdpa_dir_us]);
        return;
    }

    for (i = rdpa_dir_ds; i <= rdpa_dir_us; i++)
    {
        dir_idx.index = cpu_meter_idx[i];
        dir_idx.dir = i;
        meter_cfg.sir = recommended_rates[rate_idx];
        rdpa_cpu_meter_cfg_set(rdpa_cpu_obj, &dir_idx , &meter_cfg);
    } 
}

int bcmeapi_module_init(void)
{
    struct net_device *management_eth_dev;
    unsigned long optical_wan_type = 0;
    bdmf_object_handle system;
    
    if (rdpa_cpu_get(rdpa_cpu_host, &rdpa_cpu_obj))
        return -ESRCH;
    if (rdpa_system_get(&system))
        return -ESRCH; 
    if (rdpa_system_init_cfg_get(system, &init_cfg))
        return -EINVAL;

    /* Unload kernel's initial driver for eth0. */
    management_eth_dev = dev_get_by_name(&init_net, "eth0");
	if (management_eth_dev)
	{
		management_eth_dev->netdev_ops->ndo_stop(management_eth_dev);
		dev_put(management_eth_dev);
		unregister_netdev(management_eth_dev);
		free_netdev(management_eth_dev); /* this will wait until refcnt is zero */
	}
    
    BpGetOpticalWan(&optical_wan_type);
    switch (optical_wan_type)
    {
        case BP_OPTICAL_WAN_GPON:
           wan_port_id = GPON_PORT_ID;
           break;

        case BP_OPTICAL_WAN_EPON:
           wan_port_id = EPON_PORT_ID;
           break;

        default:
           wan_port_id = init_cfg.gbe_wan_emac; /* gbe wan port will always be emac number */
           break;
    }
    
    cpu_meter_idx_init();
    _rdpa_cfg_cpu_meter(0);

    bdmf_put(system);
    return 0;
}
void bcmeapi_module_init2(void)
{
}
void bcmeapi_enet_module_cleanup(void)
{
    bdmf_put(rdpa_cpu_obj);
}

static void bcmeapi_enet_isr(long queue_id)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(vnet_dev[0]);

    rdpa_cpu_int_disable(rdpa_cpu_host, NETDEV_CPU_RX_QUEUE_ID);
    rdpa_cpu_int_clear(rdpa_cpu_host, NETDEV_CPU_RX_QUEUE_ID);
    BCMENET_WAKEUP_RXWORKER(pDevCtrl);
}

void bcmeapi_buf_alloc(BcmEnet_devctrl *pDevCtrl) 
{ 
}

int bcmeapi_queue_select(EnetXmitParams *pParam)
{
	return 0;
}

void bcmeapi_napi_post(struct BcmEnet_devctrl *pDevCtrl)
{
    rdpa_cpu_int_enable(rdpa_cpu_host, NETDEV_CPU_RX_QUEUE_ID);
    /* If the queue got full while the network driver was handling previous
     * packets, then new packets will not cause interrupt (they will be
     * simply dropped by Runner without interrupt). In this case, no one
     * will wake up the network driver again, and traffic will stop. So, the
     * solution is to schedule another NAPI round that will flush the queue. */
    /*just check if ring is full*/

	if (rdpa_cpu_queue_not_empty(rdpa_cpu_host,NETDEV_CPU_RX_QUEUE_ID))
    {
        rdpa_cpu_int_disable(rdpa_cpu_host, NETDEV_CPU_RX_QUEUE_ID);
        rdpa_cpu_int_clear(rdpa_cpu_host, NETDEV_CPU_RX_QUEUE_ID);
        BCMENET_WAKEUP_RXWORKER(pDevCtrl);
    }

}

void bcmeapi_get_tx_queue(EnetXmitParams *pParam)
{
}

void bcmeapi_add_dev_queue(struct net_device *dev)
{
#if defined(CONFIG_BCM968500) || defined(CONFIG_BCM96838)
    int lan_port;
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);
#if defined(ENET_EPON_CONFIG)			    
    if (pDevCtrl->sw_port_id  == EPON_PORT_ID)
    {
        return ;
    }
#endif
#endif
    if (pDevCtrl->sw_port_id != GPON_PORT_ID)
    {

        lan_port = rdpa_emac0 + pDevCtrl->sw_port_id;
        pDevCtrl->phy_addr = BpGetPhyAddr(0,lan_port);
#ifdef   CONFIG_BCM968500
        bl_lilac_phy_auto_enable(pDevCtrl->phy_addr);
#elif defined (CONFIG_BCM96838)
        PhyAutoEnable(lan_port);
#if defined(CONFIG_BCM_ETH_PWRSAVE)
		ethsw_setup_hw_apd(1);
#endif
#endif
    }

}

static int _rdpa_cfg_cpu_rx_queue(int queue_id, uint32_t queue_size,
    rdpa_cpu_rxq_rx_isr_cb_t rx_isr)
{
    rdpa_cpu_rxq_cfg_t rxq_cfg;
    int rc;

    /* Read current configuration, set new drop threshold and ISR and write
     * back. */
    bdmf_lock();
    rc = rdpa_cpu_rxq_cfg_get(rdpa_cpu_obj, queue_id, &rxq_cfg);
    if (rc < 0)
        goto unlock_exit;
    rxq_cfg.size = queue_size;
    rxq_cfg.isr_priv = queue_id;
    rxq_cfg.rx_isr = rx_isr;
    rxq_cfg.sysb_type = bdmf_sysb_fkb;

    rc = rdpa_cpu_rxq_cfg_set(rdpa_cpu_obj, queue_id, &rxq_cfg);

unlock_exit:
    bdmf_unlock();
    return rc;
}

void bcmeapi_del_dev_intr(BcmEnet_devctrl *pDevCtrl)
{
    rdpa_cpu_int_disable(rdpa_cpu_host, NETDEV_CPU_RX_QUEUE_ID);
    rdpa_cpu_int_clear(rdpa_cpu_host, NETDEV_CPU_RX_QUEUE_ID);
    _rdpa_cfg_cpu_rx_queue(NETDEV_CPU_RX_QUEUE_ID, 0, NULL);
}

#define SKB_POOL_SIZE 1024
int bcmeapi_init_queue(BcmEnet_devctrl *pDevCtrl)
{
    unsigned char *pSkbuff;
    int i;

    pDevCtrl->default_txq = 0; /* BCM_GMP_MW_UNCLASSIFIED_TRAFFIC_RC; TBD */ 

    if (!pDevCtrl->skbs_p)
    { /* CAUTION!!! DONOT reallocate SKB pool */
        /*
         * Dynamic allocation of skb logic assumes that all the skb-buffers
         * in 'freeSkbList' belong to the same contiguous address range. So if you do any change
         * to the allocation method below, make sure to rework the dynamic allocation of skb
         * logic. look for kmem_cache_create, kmem_cache_alloc and kmem_cache_free functions 
         * in this file 
         */
        if( (pDevCtrl->skbs_p = kmalloc((SKB_POOL_SIZE * BCM_SKB_ALIGNED_SIZE) + 0x10,
                        GFP_ATOMIC)) == NULL )
            return -ENOMEM;

        memset(pDevCtrl->skbs_p, 0, (SKB_POOL_SIZE * BCM_SKB_ALIGNED_SIZE) + 0x10);

        pDevCtrl->freeSkbList = NULL;

        /* Chain socket skbs */
        for(i = 0, pSkbuff = (unsigned char *)
                (((unsigned long) pDevCtrl->skbs_p + 0x0f) & ~0x0f);
                i < SKB_POOL_SIZE; i++, pSkbuff += BCM_SKB_ALIGNED_SIZE)
        {
            ((struct sk_buff *) pSkbuff)->next_free = pDevCtrl->freeSkbList;
            pDevCtrl->freeSkbList = (struct sk_buff *) pSkbuff;
        }
        pDevCtrl->end_skbs_p = pDevCtrl->skbs_p + (SKB_POOL_SIZE * BCM_SKB_ALIGNED_SIZE) + 0x10;
    }
    return 0;
}

void bcmeapi_get_chip_idrev(unsigned int *chipid, unsigned int *chiprev)
{
#ifdef CONFIG_BCM968500
	*chipid = bl_lilac_soc_id();
	*chiprev = 0;
#endif
}

static void _rdpa_cfg_cpu_reason_to_queue_init(void)
{
    int rc;
    rdpa_cpu_reason_index_t reason_cfg_idx = {BDMF_INDEX_UNASSIGNED, BDMF_INDEX_UNASSIGNED};
    rdpa_cpu_reason_cfg_t reason_cfg = {};

    while (!rdpa_cpu_reason_cfg_get_next(rdpa_cpu_obj, &reason_cfg_idx))
    {
        if (reason2queue_cfg_skip(&reason_cfg_idx, NULL))
            continue;
        reason_cfg.queue = NETDEV_CPU_RX_QUEUE_ID;
        reason_cfg.meter = BDMF_INDEX_UNASSIGNED;
        rc = rdpa_cpu_reason_cfg_set(rdpa_cpu_obj, &reason_cfg_idx, &reason_cfg);
        if (rc < 0)
            printk(KERN_ERR CARDNAME ": Error (%d) configuraing CPU reason to queue \n", rc );
    }
}

int bcmeapi_open_dev(BcmEnet_devctrl *pDevCtrl, struct net_device *dev)
{
    int rc;

    rc = _rdpa_cfg_cpu_rx_queue(NETDEV_CPU_RX_QUEUE_ID,
        NETDEV_CPU_RX_QUEUE_SIZE, bcmeapi_enet_isr);
    if (rc < 0)
    {
        printk(KERN_ERR CARDNAME ": Cannot configure CPU Rx queue (%d)\n", rc);
        return -EINVAL;
    }

    rdpa_cpu_int_clear(rdpa_cpu_host, NETDEV_CPU_RX_QUEUE_ID);
    rdpa_cpu_int_enable(rdpa_cpu_host, NETDEV_CPU_RX_QUEUE_ID);

    _rdpa_cfg_cpu_reason_to_queue_init();
    _rdpa_cpu_set_reasons_to_meter(cpu_meter_idx[rdpa_dir_ds], cpu_meter_idx[rdpa_dir_us]);

    rc = rdpa_cpu_rxq_flush_set(rdpa_cpu_obj, NETDEV_CPU_RX_QUEUE_ID, TRUE);
    if (rc < 0)
        printk(KERN_ERR CARDNAME ": Failed to flush cpu queue %d, rc %d\n", NETDEV_CPU_RX_QUEUE_ID, rc);

    return 0;
}

void bcmeapi_reset_mib_cnt(uint32_t sw_port)
{
}

int bcmeapi_ioctl_kernel_poll(struct net_device *dev, struct ethswctl_data *e)
{
    int int_count = 0; /* For Runner - no ephy interrupts */
    static int mdk_init_done = 0;

    /* MDK will calls this function for the first time after it completes initialization */
    if (!mdk_init_done) 
	{
        mdk_init_done = 1;
		/* Disable HW switching by default for Runner based platforms */
        ethsw_set_hw_switching(HW_SWITCHING_DISABLED);
    }
    if (copy_to_user((void*)(&e->status), (void*)&int_count, sizeof(e->status)))
    {
        return -EFAULT;
    }
    return 0;
}
/* Ideally this function should be common for both Runner and DMA based chips
 * Link polling through Ethernet driver is not commonly used for DMA platforms
 * and performed by SWMDK. This logic should be clubbed with link_poll function 
 * later. */ 

void bcmeapi_update_link_status(void)
{
    struct net_device *dev = vnet_dev[0];
    BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(dev);
    unsigned long port_map = priv->EnetInfo[0].sw.port_map; /* Used only for optimization */

#if defined(CONFIG_BCM_EXT_SWITCH)
    port_map &= (~(1<<extSwInfo.connected_to_internalPort)); /* Remove internal port connected to external switch */
#endif
    if (port_map) 
    {
        int vport, linkMask, physical_port;
        PHY_STAT phys;

        for (vport = vport_cnt; port_map && vport >= 1; vport--) /* Starting backward - internal switch port are towards end */
        {
            if (LOGICAL_PORT_TO_UNIT_NUMBER(VPORT_TO_LOGICAL_PORT(vport)) == 0) /* Internal switch i.e. Runner port */
            {
                physical_port = LOGICAL_PORT_TO_PHYSICAL_PORT(VPORT_TO_LOGICAL_PORT(vport)); /* Get physical port number on Runner */
                port_map &= (~(1<<physical_port)); /* Remove the port from port_map */
#if defined(CONFIG_BCMGPON)
                /* Skip GPON interface */
                if(physical_port == GPON_PORT_ID) {
                    continue;
                }
#endif
                /* Get the status of Phy connected to physical port */
                phys = ethsw_phy_stat(physical_port);
                linkMask = phys.lnk << (PHYSICAL_PORT_TO_LOGICAL_PORT(physical_port,0));
                if ( (priv->linkState & (1<<(PHYSICAL_PORT_TO_LOGICAL_PORT(physical_port,0)))) != linkMask ) { /* Did link state change */
                    printk("\n\n update_link_status : vport = %d port = %d priv->linkState = 0x%08x phy.lnk = %d\n\n",vport,physical_port,priv->linkState,phys.lnk);
                    ethsw_set_mac(vport - 1, phys);
                    link_change_handler(PHYSICAL_PORT_TO_LOGICAL_PORT(physical_port,0), phys.lnk, phys.spd1000?1000 : (phys.spd100?100 : 10), phys.fdx);
                }
            }
        }
    }
}

/* we assume that if RX stime per test period > speficied value, that we decrease the rate. Taking in account that
   period is about 1 sec (which is 1000 jiffies), we don't allow value > 1000. Initially we set it to default of 800,
   and allow to user to reconfigure it. */
static unsigned long rx_stime_ps_thresh = 800;

void bcmeapi_enet_poll_timer(void)
{
    static cputime_t last_rx_stime = 0;
    unsigned long delta_stime = 0;
    static int first_time = 1; 
    BcmEnet_devctrl *pDevCtrl = netdev_priv(vnet_dev[0]);

    if (first_time)
    {
        last_rx_stime = pDevCtrl->rx_thread->stime;
        first_time = 0;
        return;
    }

    delta_stime = cputime_to_jiffies(pDevCtrl->rx_thread->stime - last_rx_stime);
    last_rx_stime = pDevCtrl->rx_thread->stime;

    if (delta_stime < rx_stime_ps_thresh)
        _rdpa_cfg_cpu_meter(1);
    else 
        _rdpa_cfg_cpu_meter(0);
}


static int proc_get_rx_stime_thresh(char *page, char **start, off_t off, int cnt, int *eof, void *data)
{
    int r;

    *eof = 1;
    r = sprintf(page, "%lu\n", rx_stime_ps_thresh);
    return r < cnt ? r : 0;
}

static int proc_set_rx_stime_thresh(struct file *f, const char *buf, unsigned long cnt, void *data)
{
    char input[32] = {};

    if (copy_from_user(input, buf, cnt) != 0)
        return -EFAULT;
    rx_stime_ps_thresh = strtoul(input, NULL, 10);
    if (rx_stime_ps_thresh > 1000)
    {
        printk("Bad value for RX stime threshold\n");
        return -EFAULT; 
    }
    return cnt;
}

void bcmeapi_add_proc_files(struct net_device *dev, BcmEnet_devctrl *pDevCtrl)
{
    struct proc_dir_entry *p;

    p = create_proc_entry("rx_stime_ps_thresh", 0644, NULL);

    if (p == NULL)
        return;

    p->data = dev;
    p->read_proc = proc_get_rx_stime_thresh;
    p->write_proc = proc_set_rx_stime_thresh;
}

void bcmeapi_free_queue(BcmEnet_devctrl *pDevCtrl)
{
    remove_proc_entry("rx_stime_ps_thresh", NULL);
}

