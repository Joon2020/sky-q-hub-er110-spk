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
// File Name  : bcmenet.c
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
#include "linux/if_bridge.h"
#include <net/arp.h>
#include <board.h>
#include <spidevices.h>
#include <bcmnetlink.h>
#include <bcm_intr.h>
#include "linux/bcm_assert_locks.h"
#include <linux/bcm_realtime.h>
#include <linux/stddef.h>
#include <asm/atomic.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>
#include <linux/nbuff.h>

#include <net/net_namespace.h>
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,2,0)
#include <linux/module.h>
#endif
#include <linux/version.h>


#if defined(_CONFIG_BCM_BPM)
#include <linux/gbpm.h>
#include "bpm.h"
#endif

#include "bcmmii.h"
#include "bcmenet.h"
#include "ethsw.h"
#include "ethsw_phy.h"
#include "bcmsw.h"
#include "pktCmf_public.h"

#include "bcmsw_api.h"
#include "bcmswaccess.h"
#include "bcmSpiRes.h"
#include "bcmswshared.h"
#include "ethswdefs.h"

#if defined(_CONFIG_BCM_INGQOS)
#include <linux/iqos.h>
#include "ingqos.h"
#endif

#if defined(CONFIG_BCM_GMAC)
#include <bcmgmac.h>
#endif

#if defined(CONFIG_BCM968500) || defined(CONFIG_BCM96838)
#include "bcmenet_runner_inline.h"
#endif

#if defined(SUPPORT_ETHTOOL)
#include "bcmenet_ethtool.h"
#endif

/* vnet_dev[0] is bcmsw device not attached to any physical port */
#define port_id_from_dev(dev) ((dev->base_addr == vnet_dev[0]->base_addr) ? MAX_TOTAL_SWITCH_PORTS : \
                              ((BcmEnet_devctrl *)netdev_priv(dev))->sw_port_id)
extern int kerSysGetMacAddress(unsigned char *pucaMacAddr, unsigned long ulId);
static int bcm63xx_enet_open(struct net_device * dev);
static int bcm63xx_enet_close(struct net_device * dev);
static void bcm63xx_enet_timeout(struct net_device * dev);
static void bcm63xx_enet_poll_timer(unsigned long arg);
static int bcm63xx_enet_xmit(pNBuff_t pNBuff, struct net_device * dev);
static inline int bcm63xx_enet_xmit2(struct net_device *dev, EnetXmitParams *pParam);
static struct net_device_stats * bcm63xx_enet_query(struct net_device * dev);
static int bcm63xx_enet_change_mtu(struct net_device *dev, int new_mtu);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 1)
static int bcm63xx_enet_rx_thread(void *arg);
#else
static int bcm63xx_enet_poll_napi(struct napi_struct *napi, int budget);
#endif
static uint32 bcm63xx_rx(void *ptr, uint32 budget);
static int bcm_set_soft_switching(int swPort, int type);
static int bcm_set_mac_addr(struct net_device *dev, void *p);
static int bcm63xx_init_dev(BcmEnet_devctrl *pDevCtrl);
static int bcm63xx_uninit_dev(BcmEnet_devctrl *pDevCtrl);
static void __exit bcmenet_module_cleanup(void);
static int __init bcmenet_module_init(void);
static int bcm63xx_enet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd);
int __init bcm63xx_enet_probe(void);
static int bcm_mvtags_len(char *ethHdr);

/* Sanity checks for user configured DMA parameters */
#if (CONFIG_BCM_DEF_NR_RX_DMA_CHANNELS > ENET_RX_CHANNELS_MAX)
#error "ERROR - Defined RX DMA Channels greater than MAX"
#endif
#if (CONFIG_BCM_DEF_NR_TX_DMA_CHANNELS > ENET_TX_CHANNELS_MAX)
#error "ERROR - Defined TX DMA Channels greater than MAX"
#endif

#if (ENET_RX_CHANNELS_MAX > 4)
#error "Overlaying channel and pDevCtrl into context param needs rework; check CONTEXT_CHAN_MASK "
#endif

struct kmem_cache *enetSkbCache;

static DECLARE_COMPLETION(poll_done);
static atomic_t poll_lock = ATOMIC_INIT(1);
static int poll_pid = -1;
struct net_device* vnet_dev[MAX_NUM_OF_VPORTS+1] = {[0 ... (MAX_NUM_OF_VPORTS)] = NULL};
int vport_to_logicalport[MAX_NUM_OF_VPORTS] = {0, 1, 2, 3, 4, 5, 6, 7};
int logicalport_to_vport[MAX_TOTAL_SWITCH_PORTS] = {[0 ... (MAX_TOTAL_SWITCH_PORTS-1)] = -1};
int vport_cnt;  /* number of vports: bitcount of Enetinfo.sw.port_map */

static unsigned int consolidated_portmap;

#if defined(ENET_EPON_CONFIG)
static const char* eponif_name = "epon0";
static const char* oamif_name = "oam";
struct net_device* eponifid_to_dev[MAX_EPON_IFS] = {[0 ... (MAX_EPON_IFS-1)] = NULL};
struct net_device* oam_dev;
void* oam_tx_func = NULL;
void* epon_data_tx_func = NULL;
static int create_epon_vport(const char *name);
static int delete_all_epon_vports(void);
EXPORT_SYMBOL(oam_dev);
EXPORT_SYMBOL(oam_tx_func);
EXPORT_SYMBOL(epon_data_tx_func);
#endif
#if defined(ENET_GPON_CONFIG)
#define UNASSIGED_IFIDX_VALUE (-1)
#define MAX_GPON_IFS_PER_GEM  (5)
static int gem_to_gponifid[MAX_GEM_IDS][MAX_GPON_IFS_PER_GEM];
static int freeid_map[MAX_GPON_IFS] = {0};
static int default_gemid[MAX_GPON_IFS] = {0};
struct net_device* gponifid_to_dev[MAX_GPON_IFS] = {[0 ... (MAX_GPON_IFS-1)] = NULL};
static int create_gpon_vport(char *name);
static int delete_gpon_vport(char *ifname);
static int delete_all_gpon_vports(void);
static int set_get_gem_map(int op, char *ifname, int ifnum, uint8 *pgem_map_arr);
static void dumpGemIdxMap(uint8 *pgem_map_arr);
static void initGemIdxMap(uint8 *pgem_map_arr);
static int set_mcast_gem_id(uint8 *pgem_map_arr);
#endif

#if defined(CONFIG_BCM96818)
MirrorCfg gemMirrorCfg[2];
#endif

static const struct net_device_ops bcm96xx_netdev_ops = {
  .ndo_open   = bcm63xx_enet_open,
  .ndo_stop   = bcm63xx_enet_close,
  .ndo_start_xmit   = (HardStartXmitFuncP)bcm63xx_enet_xmit,
  .ndo_set_mac_address  = bcm_set_mac_addr,
  .ndo_do_ioctl   = bcm63xx_enet_ioctl,
  .ndo_tx_timeout   = bcm63xx_enet_timeout,
  .ndo_get_stats      = bcm63xx_enet_query,
  .ndo_change_mtu     = bcm63xx_enet_change_mtu
};


#ifdef BCM_ENET_DEBUG_BUILD
/* Debug Variables */
/* Number of pkts received on each channel */
static int ch_pkts[ENET_RX_CHANNELS_MAX] = {0};
/* Number of times there are no rx pkts on each channel */
static int ch_no_pkts[ENET_RX_CHANNELS_MAX] = {0};
static int ch_no_bds[ENET_RX_CHANNELS_MAX] = {0};
/* Number of elements in ch_serviced debug array */
#define NUM_ELEMS 4000
/* -1 indicates beginning of an rx(). The bit31 indicates whether a pkt
   is received on that channel or not */
static unsigned int ch_serviced[NUM_ELEMS] = {0};
static int dbg_index;
#define NEXT_INDEX(index) ((++index) % NUM_ELEMS)
#define ISR_START 0xFF
#define WRR_RELOAD 0xEE
#endif
#define LLID_SINGLE 0

extsw_info_t extSwInfo = {
  .switch_id = 0,
  .brcm_tag_type = 0,
  .present = 0,
  .connected_to_internalPort = -1,
};

int bcmenet_in_init_dev = 0;

enet_global_var_t global = {
  .extPhyMask = 0,
  .dump_enable = 0,
  .net_device_stats_from_hw = {0},
  .pVnetDev0_g = NULL
};

struct semaphore bcm_ethlock_switch_config;
struct semaphore bcm_link_handler_config;

spinlock_t bcm_ethlock_phy_access;
spinlock_t bcm_extsw_access;
atomic_t phy_read_ref_cnt = ATOMIC_INIT(0);
atomic_t phy_write_ref_cnt = ATOMIC_INIT(0);

#ifdef DYING_GASP_API

/* OAMPDU Ethernet dying gasp message */
unsigned char dg_ethOam_frame[64] = {
    1, 0x80, 0xc2, 0, 0, 2, 
    0, 0,    0,    0, 0, 0, /* Fill Src MAC at the time of sending, from dev */
    0x88, 0x9, 
    3, /* Subtype */
    5, /* code for DG frame */
    'B', 'R', 'O', 'A', 'D', 'C', 'O', 'M', 
    ' ', 'B', 'C', 'G', 

};

/* Socket buffer and buffer pointer for msg */
struct sk_buff *dg_skbp;

/* Flag indicates we're in Dying Gasp and powering down - don't clear once set */
int dg_in_context=0; 

#endif

/* Delete all the virtual eth ports */
static void delete_vport(void)
{
    int port;

    synchronize_net();

    for (port = 1; port <= vport_cnt; port++)
    {
        if (vnet_dev[port] != NULL)
        {
#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
            if(memcmp(vnet_dev[0]->dev_addr, vnet_dev[port]->dev_addr, ETH_ALEN)) {
                kerSysReleaseMacAddress(vnet_dev[port]->dev_addr);
            }
#endif
            unregister_netdev(vnet_dev[port]);
            free_netdev(vnet_dev[port]);
            vnet_dev[port] = NULL;
        }
    }
}

/* Indirection just to print the caller line number in case of error */
#define bcmenet_set_dev_mac_addr(a,b) _bcmenet_set_dev_mac_addr(a,b,__LINE__)

static int _bcmenet_set_dev_mac_addr(struct net_device *dev, struct sockaddr *pSockAddr, int callerLine)
{
    int retVal;
    pSockAddr->sa_family = dev->type; /* set the sa_family same as dev type to avoid error */
    if (netif_running(dev)) {
        //dev_close(dev); /* ifconfig ethX down - MAC address change is not allowed on running device */
        // clear_bit === ifconfig ethX down ; close & open are unnecessary and costly
        clear_bit(__LINK_STATE_START, &dev->state);  
        if (rtnl_is_locked()) {
            retVal = dev_set_mac_address(dev, pSockAddr);
        }
        else {
            rtnl_lock();
            retVal = dev_set_mac_address(dev, pSockAddr);
            rtnl_unlock();
        }
        //dev_open(dev); /* ifconfig ethX up */
        set_bit(__LINK_STATE_START, &dev->state);
        if (retVal) {
            printk("%d:ERROR<%d> - setting MAC address for dev %s\n",callerLine,retVal,dev->name);
        }
    }
    else {

        if (rtnl_is_locked()) {
            retVal = dev_set_mac_address(dev, pSockAddr);
        }
        else {
            rtnl_lock();
            retVal = dev_set_mac_address(dev, pSockAddr);
            rtnl_unlock();
        }
        if (retVal) {
            printk("%d:ERROR<%d> - setting MAC address for dev %s\n",callerLine,retVal,dev->name);
        }
    }
    return retVal;
}

/* Create virtual eth ports: one for each physical switch port except
   for the GPON port */
static int create_vport(void)
{
    struct net_device *dev;
    struct sockaddr sockaddr;
    int status, i, j;
    int *map_p, map_int, map_ext, map_ext_done=0;
    PHY_STAT phys;
    BcmEnet_devctrl *pDevCtrl = NULL;
    BcmEnet_devctrl *pVnetDev0 = (BcmEnet_devctrl *) netdev_priv(vnet_dev[0]);
    int phy_id, phy_conn;
    char *phy_devName;
    int unit = 0, physical_port = 0;
    unsigned long port_flags;

    /* separate out the portmap for internal and external switch */
    map_int = consolidated_portmap & SW_PORTMAP_M;
    map_ext = consolidated_portmap & SW_PORTMAP_EXT_M;

    if (vport_cnt > MAX_NUM_OF_VPORTS)
        return -1;

    phys.lnk = 0;
    phys.fdx = 0;
    phys.spd1000 = 0;
    phys.spd100 = 0;

    /* Start with external switch map - this is just so vport mapping is backward compatible w/ older releases (pre 4.14) */
    map_p = &map_ext;
   
    for (i = 1, j = 0 ; i < vport_cnt + 1; i++, j++, (*map_p) /= 2)
    {
        if (map_ext_done == 0 && map_ext == 0) { /* Done with all External switch ports */
            map_p = &map_int; /* switch to internal ports */
            map_ext_done = 1;
            j = 0;
        }
        /* Skip the switch ports which are not in the port_map */
        while (((*map_p) % 2) == 0)
        {
            (*map_p) /= 2;
            j ++;
        }

        /* Initialize the vport <--> phyport mapping tables */
        vport_to_logicalport[i] = j;
        logicalport_to_vport[j] = i;

        BCM_ASSERT(j < MAX_TOTAL_SWITCH_PORTS);

#if defined(CONFIG_BCM96818) || defined(CONFIG_BCM968500) || defined(CONFIG_BCM96838)
        /* Skip creating eth interface for GPON port */
        if (j == GPON_PORT_ID) {
            (*map_p) /= 2;
            j ++;
            continue;
        }
#endif

        physical_port = LOGICAL_PORT_TO_PHYSICAL_PORT(j);     
        unit = LOGICAL_PORT_TO_UNIT_NUMBER(j);
        phy_id = pVnetDev0->EnetInfo[unit].sw.phy_id[physical_port];
        phy_conn = pVnetDev0->EnetInfo[unit].sw.phyconn[physical_port];
        phy_devName = pVnetDev0->EnetInfo[unit].sw.phy_devName[physical_port];
        port_flags = pVnetDev0->EnetInfo[unit].sw.port_flags[physical_port];

        dev = alloc_etherdev(sizeof(BcmEnet_devctrl));
        if (dev == NULL) {
            printk("%s: dev alloc failed \n", __FUNCTION__);
            delete_vport();
            return -ENOMEM;
        }
        
        pDevCtrl = netdev_priv(dev);
        memset(netdev_priv(dev), 0, sizeof(BcmEnet_devctrl));

        /* Set the pDevCtrl->dev to dev */
        pDevCtrl->dev = dev;

        if (phy_devName != PHY_DEVNAME_NOT_DEFINED)
        {
            dev_alloc_name(dev, phy_devName);
        }
        else
        {
            dev_alloc_name(dev, dev->name);
        }

  /* SKY VDSL eth port need to remapped because in schamatic
           it's has connected differently.*/
#if defined(CONFIG_SKY_VDSL) && !defined(CONFIG_SKY_ETHAN)
        if(j == 0) {/*eth0-->eth1 */
            sprintf(dev->name, "eth1");
        }else if(j == 1) { /*eth1-->eth2 */
            sprintf(dev->name, "eth2");
        }
        else if(j == 2) { /*eth2-->eth3 */
            sprintf(dev->name, "eth3");
        }
         else if(j == 3) {/*eth3-->eth0 */
            sprintf(dev->name, "eth0");
        }
#endif
        SET_MODULE_OWNER(dev);

        dev->netdev_ops             = vnet_dev[0]->netdev_ops;
#if defined(SUPPORT_ETHTOOL)
        dev->ethtool_ops            = vnet_dev[0]->ethtool_ops;
#endif
        dev->priv_flags             |= vnet_dev[0]->priv_flags;
        dev->base_addr              = j;

        dev->features               = vnet_dev[0]->features;

        bcmeapi_create_vport(dev);
        /* Switch port id of this interface */
        pDevCtrl->sw_port_id        = j; /* This is logical port number */

        netdev_path_set_hw_port(dev, j, BLOG_ENETPHY);
        status = register_netdev(dev);
        if (status != 0)
        {
            unregister_netdev(dev);
            free_netdev(dev);
            return status;
        }
        vnet_dev[i] = dev; 

        /* The vport_id specifies the unique id of virtual eth interface */
        pDevCtrl->vport_id = i;

        /* Set the default tx queue to 0 */
        pDevCtrl->default_txq = 0;
        pDevCtrl->use_default_txq = 0;
        memmove(sockaddr.sa_data, vnet_dev[0]->dev_addr, ETH_ALEN);
        BCM_ENET_DEBUG("phy_id = %d", phy_id);
        if (IsWanPort(phy_id)) {
            pVnetDev0->wanPort |= 1 << j;
            BCM_ENET_DEBUG("Getting MAC for WAN port %d", j);
#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
            status = kerSysGetMacAddress(dev->dev_addr, dev->ifindex);
            if (status == 0) {
                memmove(sockaddr.sa_data, dev->dev_addr, ETH_ALEN);
            }
#endif
          dev->priv_flags |= IFF_WANDEV;
#if !defined(CONFIG_BCM968500) && !defined (CONFIG_BCM96838)
          dev->priv_flags &= ~IFF_HW_SWITCH;
#endif
        }
        bcmenet_set_dev_mac_addr(dev, &sockaddr);
#if defined(CONFIG_BCM_GMAC)
        /* GMAC port is only on the internal switch  */
        if ( gmac_is_gmac_supported() && !IsExternalSwitchPort(j) && gmac_is_gmac_port(physical_port))
        {
            if (IsWanPort(phy_id)) {
                gmac_set_wan_port( 1 );
            }
            pVnetDev0->gmacPort |= 1 << j;
            BCM_ENET_DEBUG("Setting gmac port %d phy id %d to gmacPort %d", physical_port, j, pVnetDev0->gmacPort);
        }
#endif

        /* Note: The parameter i should be the vport_id-1. The ethsw_set_mac
           maps it to physical port id */
        if(pVnetDev0->unit == 0)
        {
            ethsw_set_mac(i-1, phys);
        }

        if(IsExternalSwitchPort(j))
        {
          dev->priv_flags |= IFF_EXT_SWITCH;
        }
        if (IsPortSoftSwitching(port_flags))
        {
            bcm_set_soft_switching(j, TYPE_ENABLE);
        }
        printk("%s: MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
            dev->name,
            dev->dev_addr[0],
            dev->dev_addr[1],
            dev->dev_addr[2],
            dev->dev_addr[3],
            dev->dev_addr[4],
            dev->dev_addr[5]);
    }

    return 0;
}

#undef OFFSETOF
#define OFFSETOF(STYPE, MEMBER)     ((size_t) &((STYPE *)0)->MEMBER)

static inline int bcm_mvtags_len(char *ethHdr)
{
    unsigned int end_offset = 0;
    struct vlan_ethhdr *vhd;
    BcmEnet_hdr* bhd;

    bhd = (BcmEnet_hdr*)ethHdr;
    if (bhd->brcm_type == BRCM_TYPE)
    {
        end_offset += BRCM_TAG_LEN;
        bhd = (BcmEnet_hdr *)((int)bhd + BRCM_TAG_LEN);
    }

    if (bhd->brcm_type == BRCM_TYPE2)
    {
        end_offset += BRCM_TAG_TYPE2_LEN;
    }

#ifdef VLAN_TAG_FFF_STRIP
    vhd = (struct vlan_ethhdr*)(ethHdr + end_offset);
    if((vhd->h_vlan_proto == VLAN_TYPE) &&
            (vhd->h_vlan_TCI & VLAN_VID_MASK) == 0xFFF)
    {
        end_offset += VLAN_HLEN;
    }
#endif

    return end_offset;
}

/*
 *  This is a modified version of eth_type_trans(), for taking care of
 *  Broadcom Tag with Ethernet type BRCM_TYPE [0x8874].
 */

unsigned short bcm_type_trans(struct sk_buff *skb, struct net_device *dev)
{
    struct ethhdr *eth;
    unsigned char *rawp;
    unsigned int end_offset = 0, from_offset = 0;
    uint16 *to, *end, *from;
    unsigned int hdrlen = sizeof(struct ethhdr);

    skb_reset_mac_header(skb);

    end_offset = bcm_mvtags_len(skb->data);
    if (end_offset)
    {
        from_offset = OFFSETOF(struct ethhdr, h_proto);
    
        to = (uint16*)(skb->data + from_offset + end_offset) - 1;
        end = (uint16*)(skb->data + end_offset) - 1;
        from = (uint16*)(skb->data + from_offset) - 1;

        while ( to != end )
            *to-- = *from--;
    }

     skb_set_mac_header(skb, end_offset);

    hdrlen += end_offset;

    skb_pull(skb, hdrlen);
    eth = (struct ethhdr *)skb_mac_header(skb);

    if(*eth->h_dest&1)
    {
        if(memcmp(eth->h_dest,dev->broadcast, ETH_ALEN)==0)
            skb->pkt_type=PACKET_BROADCAST;
        else
            skb->pkt_type=PACKET_MULTICAST;
    }

    /*
     *  This ALLMULTI check should be redundant by 1.4
     *  so don't forget to remove it.
     *
     *  Seems, you forgot to remove it. All silly devices
     *  seems to set IFF_PROMISC.
     */

    else if(1 /*dev->flags&IFF_PROMISC*/)
    {
        if(memcmp(eth->h_dest,dev->dev_addr, ETH_ALEN))
            skb->pkt_type=PACKET_OTHERHOST;
    }

    if (ntohs(eth->h_proto) >= 1536)
        return eth->h_proto;

    rawp = skb->data;

    /*
     *  This is a magic hack to spot IPX packets. Older Novell breaks
     *  the protocol design and runs IPX over 802.3 without an 802.2 LLC
     *  layer. We look for FFFF which isn't a used 802.2 SSAP/DSAP. This
     *  won't work for fault tolerant netware but does for the rest.
     */
    if (*(unsigned short *)rawp == 0xFFFF)
        return htons(ETH_P_802_3);

    /*
     *  Real 802.2 LLC
     */
    return htons(ETH_P_802_2);
}

/* --------------------------------------------------------------------------
    Name: bcm63xx_enet_open
 Purpose: Open and Initialize the EMAC on the chip
-------------------------------------------------------------------------- */
static int bcm63xx_enet_open(struct net_device * dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);

    if (dev != vnet_dev[0])
    {
        if ((vnet_dev[0]->flags & IFF_UP) == 0)
            return -ENETDOWN;

        bcmeapi_add_dev_queue(dev);
        netif_start_queue(dev);
        return 0;
    }

    ASSERT(pDevCtrl != NULL);
    TRACE(("%s: bcm63xx_enet_open\n", dev->name));

    bcmeapi_open_dev(pDevCtrl, dev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 1)
    /* enet does not use NAPI in 3.4 */
#else
    /* napi_enable must be called before the interrupts are enabled
       if an interrupt comes in before napi_enable is called the napi
       handler will not run and the interrupt will not be re-enabled */
    napi_enable(&pDevCtrl->napi);
#endif

    netif_start_queue(dev);

    return 0;
}

/* --------------------------------------------------------------------------
    Name: bcm63xx_enet_close
    Purpose: Stop communicating with the outside world
    Note: Caused by 'ifconfig ethX down'
-------------------------------------------------------------------------- */
static int bcm63xx_enet_close(struct net_device * dev)
{
    BcmEnet_devctrl *pDevCtrl = netdev_priv(dev);

    if (dev != vnet_dev[0])
    {
        netif_stop_queue(dev);
        return 0;
    }

    ASSERT(pDevCtrl != NULL);
    TRACE(("%s: bcm63xx_enet_close\n", dev->name));

    bcmeapi_del_dev_intr(pDevCtrl);

    netif_stop_queue(dev);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 1)
    /* enet does not use NAPI in 3.4 */
#else
    napi_disable(&pDevCtrl->napi);
#endif

    return 0;
}

/* --------------------------------------------------------------------------
    Name: bcm63xx_enet_timeout
 Purpose:
-------------------------------------------------------------------------- */
static void bcm63xx_enet_timeout(struct net_device * dev)
{
    ASSERT(dev != NULL);
    TRACE(("%s: bcm63xx_enet_timeout\n", dev->name));

    dev->trans_start = jiffies;
    netif_wake_queue(dev);
}

/* --------------------------------------------------------------------------
    Name: bcm63xx_enet_query
 Purpose: Return the current statistics. This may be called with the card
          open or closed.
-------------------------------------------------------------------------- */
static struct net_device_stats *
bcm63xx_enet_query(struct net_device * dev)
{
#ifdef REPORT_HARDWARE_STATS
    int port, log_port, extswitch;

    if (dev->base_addr == vnet_dev[0]->base_addr) /* bcmsw device */
    {
        return &(((BcmEnet_devctrl *)netdev_priv(dev))->stats);
    }


    log_port = port_id_from_dev(dev);

#if defined(ENET_EPON_CONFIG)
    if(log_port == EPON_PORT_ID)
    {
        return &(((BcmEnet_devctrl *)netdev_priv(dev))->stats);
    }
#endif 

    port = LOGICAL_PORT_TO_PHYSICAL_PORT(log_port);
    extswitch = IsExternalSwitchPort(log_port);

    if ((port < 0) || (port >= MAX_SWITCH_PORTS)) {
        printk("Invalid port <phy - %d : log - %d>, so stats will not be correct \n", port, log_port);
    }else {
        struct net_device_stats *stats = &global.net_device_stats_from_hw;
        BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
        uint32 rxDropped = 0, txDropped = 0;

        bcmsw_get_hw_stats(port, extswitch, stats);
        /* Add the dropped packets in software */
        stats->rx_dropped += pDevCtrl->stats.rx_dropped;
        stats->tx_dropped += pDevCtrl->stats.tx_dropped;

        bcmeapi_EthGetStats(log_port, &rxDropped, &txDropped); 

        stats->rx_dropped += rxDropped;
        stats->tx_dropped += txDropped;
    }
    return &global.net_device_stats_from_hw;
#else
    return &(((BcmEnet_devctrl *)netdev_priv(dev))->stats);
#endif
}

static int bcm63xx_enet_change_mtu(struct net_device *dev, int new_mtu)
{
	int max_mtu = ENET_MAX_MTU_PAYLOAD_SIZE;
    if (new_mtu < ETH_ZLEN || new_mtu > max_mtu)
        return -EINVAL;
    dev->mtu = new_mtu;
    #if defined(CONFIG_BCM968500)  || defined (CONFIG_BCM96838)   
   {
       struct ethswctl_data e;
       e.mtu = new_mtu;
       bcmeapi_ioctl_ethsw_mtu_set(&e);
    }
    #endif
    
    return 0;
}

#if defined(CONFIG_BCM96818)

int bcmenet_set_spdled(int port, int speed)
{
    int led100;
    int led1000;

    if ( port > 1 )
        return 0;

    if( 0 == port )
    {
        led100  = kLedEth0Spd100;
        led1000 = kLedEth0Spd1000;
    }
    else
    {
        led100  = kLedEth1Spd100;
        led1000 = kLedEth1Spd1000;
    }

    if ( speed == 1000 )
    {
        kerSysLedCtrl(led100, kLedStateOff);
        kerSysLedCtrl(led1000, kLedStateOn);
    }
    else if (speed == 100 || speed == 200)
    {
        kerSysLedCtrl(led100, kLedStateOn);
        kerSysLedCtrl(led1000, kLedStateOff);
    }
    else /* 10 or off */
    {
        kerSysLedCtrl(led100, kLedStateOff);
        kerSysLedCtrl(led1000, kLedStateOff);
    }

    return 0;
}
#endif

/*
 * handle_link_status_change
 */
void link_change_handler(int port, int linkstatus, int speed, int duplex)
{
    IOCTL_MIB_INFO *mib;
    int mask, vport;
    struct net_device *dev = vnet_dev[0];
    struct net_device *pNetDev;
    BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(dev);
    int linkMask;
    int sw_port;
#ifdef SKY	
	char msgData [16];
#endif /* SKY */
#if defined(CONFIG_BCM_ETH_PWRSAVE)
    int phyId = 0; // 0 is a valid phy_id but not an external phy_id. So we are OK initializing it to 0.

    if (!IsExternalSwitchPort(port))
    {
        phyId = priv->EnetInfo[0].sw.phy_id[LOGICAL_PORT_TO_PHYSICAL_PORT(port)];
    }

    ethsw_phy_pll_up(0);
#endif
    if (port >= MAX_TOTAL_SWITCH_PORTS)
    {
        return;
    }

    down(&bcm_link_handler_config);

    sw_port  = port;
    vport    = LOGICAL_PORT_TO_VPORT(port);
    mask     = 1 << port;
    /* Boundary condition check */
    /* This boundary check condition will fail for the internal switch port that is connected to
       the external switch as the vport would be -1. This is normal and expected */
    if ( vport >= 0 || vport < (sizeof(vnet_dev)/sizeof(vnet_dev[0])))
        pNetDev  = vnet_dev[vport];
    else
        pNetDev = NULL;
    linkMask = linkstatus << port;
    vport   -= 1; /* -1 for ethsw_set_mac */

    if ( NULL == pNetDev )
    {
        up(&bcm_link_handler_config);
        return;
    }

    if ((priv->linkState & mask) != linkMask) {
        BCM_ENET_LINK_DEBUG("port=%x; vport=%x", port, vport);

        mib = &((BcmEnet_devctrl *)netdev_priv(pNetDev))->MibInfo;
        if (linkstatus) {
#if defined(CONFIG_BCM_ETH_PWRSAVE)
            /* Link is up, so de-isolate the Phy  */
            if (IsExtPhyId(phyId)) {
                ethsw_isolate_phy(phyId, 0);
            }
#endif

            /* Just set a flag for EEE because a 1 second delay is required */
            priv->eee_enable_request_flag[0] |= (1<<sw_port);

            if (netif_carrier_ok(pNetDev) == 0)
                netif_carrier_on(pNetDev);
            if (speed == 1000)
            {
                mib->ulIfSpeed = SPEED_1000MBIT;
            }
            else if (speed == 100)
            {
                mib->ulIfSpeed = SPEED_100MBIT;
            }
            else if (speed == 200)
            {
                mib->ulIfSpeed = SPEED_200MBIT;
            }
            else
            {
                mib->ulIfSpeed = SPEED_10MBIT;
            }

            bcmeapi_EthSetPhyRate(port, 1, mib->ulIfSpeed, pNetDev->priv_flags & IFF_WANDEV);

#if defined (CONFIG_BCM_JUMBO_FRAME)
            if (speed == 1000) /* When jumbo frame support is enabled - the jumbo MTU is applicable only for 1000M interfaces */
                dev_set_mtu(pNetDev, ENET_MAX_MTU_PAYLOAD_SIZE);
            else
                dev_set_mtu(pNetDev, (ENET_NON_JUMBO_MAX_MTU_PAYLOAD_SIZE));
#endif
            mib->ulIfLastChange  = (jiffies * 100) / HZ;
            mib->ulIfDuplex = (unsigned long)duplex;
            priv->linkState |= mask;
            bcmeapi_set_mac_speed(port, speed);
            printk((KERN_CRIT "%s (%s switch port: %d) (Logical Port: %d) Link UP %d mbps %s duplex\n"),
                    pNetDev->name, (LOGICAL_PORT_TO_UNIT_NUMBER(port)?"Ext":"Int"),LOGICAL_PORT_TO_PHYSICAL_PORT(port), port, speed, duplex?"full":"half");
#ifdef SKY				
            memset (msgData,0,sizeof(msgData));
            strcpy (msgData,pNetDev->name);
            strcat (msgData,",UP");
#endif /* SKY */					
        } else {
#if defined(CONFIG_BCM_ETH_PWRSAVE)
            /* Link is down, so isolate the Phy. To prevent switch rx lockup 
            because of packets entering switch with DLL/Clock disabled */
            if (IsExtPhyId(phyId)) {
                ethsw_isolate_phy(phyId, 1);
            }
#endif

            /* Clear any pending request to enable eee and disable it */
            priv->eee_enable_request_flag[0] &= ~(1<<sw_port);
            priv->eee_enable_request_flag[1] &= ~(1<<sw_port);
            BCM_ENET_DEBUG("%s: port %d  disabling EEE\n", __FUNCTION__, sw_port);
#if defined(CONFIG_BCM_GMAC)
            if (IsGmacPort( sw_port ) )
            {
                volatile GmacEEE_t *gmacEEEp = GMAC_EEE;
                gmacEEEp->eeeCtrl.linkUp = 0;
                gmacEEEp->eeeCtrl.enable = 0;
            }
#endif
            ethsw_eee_port_enable(sw_port, 0, 0);

            if (IsExternalSwitchPort(sw_port))
            {
                extsw_fast_age_port(LOGICAL_PORT_TO_PHYSICAL_PORT(sw_port), 0);
            }
            else
            {
                fast_age_port(LOGICAL_PORT_TO_PHYSICAL_PORT(sw_port), 0);
            }

            if (netif_carrier_ok(pNetDev) != 0)
                netif_carrier_off(pNetDev);
            mib->ulIfLastChange  = 0;
            mib->ulIfSpeed       = 0;
            mib->ulIfDuplex      = 0;
            priv->linkState &= ~mask;

            bcmeapi_EthSetPhyRate(port, 0, mib->ulIfSpeed, pNetDev->priv_flags & IFF_WANDEV);

#if defined(CONFIG_BCM96818)
            /* set spd led for the internal phy */
            bcmenet_set_spdled(vport, 0);
#endif
            printk((KERN_CRIT "%s (%s switch port: %d) (Logical Port: %d) Link DOWN.\n"),
                    pNetDev->name, (LOGICAL_PORT_TO_UNIT_NUMBER(port)?"Ext":"Int"),LOGICAL_PORT_TO_PHYSICAL_PORT(port), port);
#ifdef SKY				
			memset (msgData,0,sizeof(msgData));
		    strcpy (msgData,pNetDev->name);
		    strcat (msgData,",DOWN");
#endif /* SKY */					
        }
#ifndef SKY
        kerSysSendtoMonitorTask(MSG_NETLINK_BRCM_LINK_STATUS_CHANGED,NULL,0);
#else /* SKY */
        kerSysSendtoMonitorTask(MSG_NETLINK_BRCM_LINK_STATUS_CHANGED,msgData, strlen(msgData)+1);
#endif /* SKY */		

    }

    up(&bcm_link_handler_config);
}


#if defined(SUPPORT_SWMDK)
static int link_change_handler_wrapper(void *ctxt)
{
	LinkChangeArgs *args = ctxt;

	BCM_ASSERT(args);
	if (args->activeELink) {
		bcmeapi_aelink_handler(args->linkstatus);
	}
	link_change_handler(args->port,
						args->linkstatus,
						args->speed,
						args->duplex);

	#if defined(CONFIG_BCM_GMAC)
	if ( IsGmacPort(args->port) && IsLogPortWan(args->port) ) {
		gmac_link_status_changed(GMAC_PORT_ID, args->linkstatus, args->speed,
								 args->duplex);
	}
	#endif
	return 0;
}

static void bcm63xx_enet_poll_timer(unsigned long arg)
{
    struct net_device *dev = vnet_dev[0];
    BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(dev);
    int i;

    /* */
    daemonize("bcmsw");
    bcmeapiInterruptEnable(1);

    /* Enable the Phy interrupts of internal Phys */
    for (i = 0; i < TOTAL_SWITCH_PORTS - 1; i++) {
        if ((priv->EnetInfo[0].sw.port_map) & (1<<i)) {
            if (!IsExtPhyId(priv->EnetInfo[0].sw.phy_id[i])) {
                ethsw_phy_intr_ctrl(i, 1);
            } else {
                global.extPhyMask |= (1 << i);
            }
        }
    }
#if defined(CONFIG_BCM_ENET)
    set_current_state(TASK_INTERRUPTIBLE);
    /* Sleep for 1 tick) */
    schedule_timeout(HZ/100);
#endif
    /* Start with virtual interfaces as down */
    for (i = 1; i <= vport_cnt; i++) {
        if ( vnet_dev[i] != NULL )
        {
           if (netif_carrier_ok(vnet_dev[i]) != 0)
               netif_carrier_off(vnet_dev[i]);
        }
    }


    /* */
    while (atomic_read(&poll_lock) > 0)
    {
        bcmeapi_enet_poll_timer();
        /* reclaim tx descriptors and buffers */
        /*   */
        bcmeapi_update_link_status();
        set_current_state(TASK_INTERRUPTIBLE);

        /* Sleep for HZ jiffies (1sec) */
        schedule_timeout(HZ);
    }

    complete_and_exit(&poll_done, 0);
    printk("bcm63xx_enet_poll_timer: thread exits!\n");
}

#else
/*
 * bcm63xx_enet_poll_timer: reclaim transmit frames which have been sent out
 */
static void bcm63xx_enet_poll_timer(unsigned long arg)
{
    IOCTL_MIB_INFO *mib;
    PHY_STAT phys;
    int newstat, tmp, mask, i;
    struct net_device *dev = vnet_dev[0];
    BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(dev);
    int ephy_sleep_delay = 0;
    uint32_t port_map = (uint32_t) priv->EnetInfo[0].sw.port_map;

    /* */
    daemonize("bcmsw");

    /* */
    bcmeapiInterruptEnable(1);

    /* Enable the Phy interrupts of internal Phys */
    for (i = 0; i < EPHY_PORTS; i++) {
        if ((priv->EnetInfo[0].sw.port_map) & (1<<i)) {
            if (!IsExtPhyId(priv->EnetInfo[0].sw.phy_id[i])) {
                ethsw_phy_intr_ctrl(i, 1);
            }
        }
    }

#if defined(CONFIG_BCM_ENET)
    set_current_state(TASK_INTERRUPTIBLE);
    /* Sleep for 1 tick) */
    schedule_timeout(HZ/100);
#endif
    /* Start with virtual interfaces as down */
    for (i = 1; i <= vport_cnt; i++) {
        if ( vnet_dev[i] != NULL )
        {
            if (netif_carrier_ok(vnet_dev[i]) != 0) {
                netif_carrier_off(vnet_dev[i]);
            }
        }
    }

    /* */
    while (atomic_read(&poll_lock) > 0)
    {
        bcmeapi_enet_poll_timer();

        /* Start with New link status of all vports as 0*/
        newstat = 0;

#if defined(CONFIG_BCM_ETH_PWRSAVE)
        ephy_sleep_delay = ethsw_ephy_auto_power_down_wakeup();
#endif

        for (i = 1; i <= vport_cnt; i++)
        {
#if defined(CONFIG_BCM_ETH_PWRSAVE)
            int phyId = priv->EnetInfo[0].sw.phy_id[LOGICAL_PORT_TO_PHYSICAL_PORT(VPORT_TO_LOGICAL_PORT(i))];
#endif
#if defined(CONFIG_BCMGPON)
            /* Skip GPON interface */
            if(LOGICAL_PORT_TO_PHYSICAL_PORT(VPORT_TO_LOGICAL_PORT(i)) == GPON_PORT_ID) {
                continue;
            }
#endif
            /* Mask for this port */
            mask = (1 << VPORT_TO_LOGICAL_PORT(i));

            if (bcmeapi_link_check( i,priv, &newstat) == BCMEAPI_CTRL_CONTINUE)
                continue;

            /* Get the MIB of this vport */
            mib = &((BcmEnet_devctrl *)netdev_priv(vnet_dev[i]))->MibInfo;

            /* Get the status of Phy connected to switch port i */
            phys = ethsw_phy_stat(i - 1);

            /* If link is up, set tmp with the mask of this port */
            tmp = (phys.lnk != 0) ? mask : 0;

            /* Update the new link status */
            newstat |= tmp;

            /* If link status has changed for this switch port i, update
               the interface status */
            if ((priv->linkState & mask) != tmp)
            {
                /* Set the MAC with link/speed/duplex status from Phy */
                /* Note: The parameter i should be the vport id. The
                   ethsw_set_mac maps it to physical port id */
                ethsw_set_mac(i - 1, phys);

                /* If Link has changed from down to up, indicate upper layers
                   and print the link status */
                if (phys.lnk)
                {
#if defined(CONFIG_BCM_ETH_PWRSAVE)
                    /* Link is up, so de-isolate the Phy  */
                    if (IsExtPhyId(phyId)) {
                        ethsw_isolate_phy(phyId, 0);
                    }
#endif

                    /* Just set a flag for EEE because a 1 second delay is required */
                    priv->eee_enable_request_flag[0] |= mask;

                    if (netif_carrier_ok(vnet_dev[i]) == 0)
                        netif_carrier_on(vnet_dev[i]);

                    if (phys.spd100)
                      mib->ulIfSpeed = SPEED_100MBIT;
                    else if (!phys.spd1000)
                      mib->ulIfSpeed = SPEED_10MBIT;

                    mib->ulIfLastChange  = (jiffies * 100) / HZ;

                    {
                        int speed;

                        if (phys.spd1000)
                            speed=1000;
                        else if (phys.spd100)
                            speed=100;
                        else
                            speed=10;

                        printk((KERN_CRIT "%s Link UP %d mbps %s duplex\n"),
                                vnet_dev[i]->name, speed, phys.fdx?"full":"half");
                    }
                }
                else
                {
#if defined(CONFIG_BCM_ETH_PWRSAVE)
                    /* Link is down, so isolate the Phy. To prevent switch rx lockup 
                    because of packets entering switch with DLL/Clock disabled */
                    if (IsExtPhyId(phyId)) {
                        ethsw_isolate_phy(phyId, 1);
                    }
#endif
                    /* Clear any pending request to enable eee and disable it */
                    priv->eee_enable_request_flag[0] &= ~mask;
                    priv->eee_enable_request_flag[1] &= ~mask;
                    ethsw_eee_port_enable(VPORT_TO_LOGICAL_PORT(i), 0, 0);

                    /* If link has changed from up to down, indicate upper
                       layers and print the 'Link Down' message */
                    if (netif_carrier_ok(vnet_dev[i]) != 0)
                        netif_carrier_off(vnet_dev[i]);

                    mib->ulIfLastChange  = 0;
                    mib->ulIfSpeed       = 0;
                    printk((KERN_CRIT "%s Link DOWN.\n"), vnet_dev[i]->name);
                }
            }
        }

        /* If there was a link status change, update linkStatus to newstat */
        if (priv->linkState != newstat)
        {
            bcmeapi_anylink_changed();
          bcmeapiInterruptEnable(0);
          priv->linkState = newstat;
#ifndef SKY		  
          kerSysSendtoMonitorTask(MSG_NETLINK_BRCM_LINK_STATUS_CHANGED,NULL,0);
#else /* SKY */		  
		 if (newstat)
		 {
			   kerSysSendtoMonitorTask(MSG_NETLINK_BRCM_LINK_STATUS_CHANGED,",UP",7);
		 }
		 else
		  {
			   kerSysSendtoMonitorTask(MSG_NETLINK_BRCM_LINK_STATUS_CHANGED,",DOWN",9);
  	       }
#endif /* SKY */  
        }

        /* Collect the statistics  */
        ethsw_counter_collect(port_map, 0);


#if defined(CONFIG_BCM_ETH_PWRSAVE)
        ephy_sleep_delay += ethsw_ephy_auto_power_down_sleep();
#endif

        /* Check for delayed request to enable EEE */
        ethsw_eee_process_delayed_enable_requests();

#if (CONFIG_BCM_EXT_SWITCH_TYPE == 53115)
        extsw_apd_set_compatibility_mode();
#endif

        /*   */
        set_current_state(TASK_INTERRUPTIBLE);

        /* Sleep for HZ jiffies (1sec), minus the time that was already */
        /* spent waiting for EPHY PLL  */
        schedule_timeout(HZ - ephy_sleep_delay);
    }

    complete_and_exit(&poll_done, 0);
    printk("bcm63xx_enet_poll_timer: thread exits!\n");
}
#endif

static struct sk_buff *bcm63xx_skb_put_tag(struct sk_buff *skb,
    struct net_device *dev, unsigned int port_map)
{
    BcmEnet_hdr *pHdr = (BcmEnet_hdr *)skb->data;
    int i, headroom;
    int tailroom;

    if (pHdr->brcm_type == BRCM_TYPE2)
    {
        headroom = 0;
        tailroom = ETH_ZLEN + BRCM_TAG_TYPE2_LEN - skb->len;
    }
    else
    {
        headroom = BRCM_TAG_TYPE2_LEN;
        tailroom = ETH_ZLEN - skb->len;
    }

    if (tailroom < 0)
    {
        tailroom = 0;
    }

    if ((skb_headroom(skb) < headroom) || (skb_tailroom(skb) < tailroom))
    {
        struct sk_buff *oskb = skb;
        skb = skb_copy_expand(oskb, headroom, tailroom, GFP_ATOMIC);
        kfree_skb(oskb);
        if (!skb)
        {
            return NULL;
        }
    }
    else if (headroom != 0)
    {
        skb = skb_unshare(skb, GFP_ATOMIC);
        if (!skb)
        {
            return NULL;
        }
    }

    if (tailroom > 0)
    {
        if (skb_is_nonlinear(skb))
        {
            /* Non linear skb whose skb->len is < minimum Ethernet Packet Length 
                         (ETHZLEN or ETH_ZLEN + BroadcomMgmtTag Length) */
            if (skb_linearize(skb))
            {
                return NULL;
            }
        }
        memset(skb->data + skb->len, 0, tailroom);  /* padding to 0 */
        skb_put(skb, tailroom);
    }

    if (headroom != 0)
    {
        uint16 *to, *from;
        BcmEnet_hdr2 *pHdr2 = (BcmEnet_hdr2 *)skb_push(skb, headroom);
        to = (uint16*)pHdr2;
        from = (uint16*)(skb->data + headroom);
        for ( i=0; i<ETH_ALEN; *to++ = *from++, i++ ); /* memmove 2 * ETH_ALEN */
        /* set ingress brcm tag and TC bit */
        pHdr2->brcm_type = BRCM_TAG2_EGRESS | (SKBMARK_GET_Q_PRIO(skb->mark) << 10);
        pHdr2->brcm_tag = port_map;
    }
    return skb;
}

static inline void bcm63xx_fkb_put_tag(FkBuff_t * fkb_p,
    struct net_device * dev, unsigned int port_map)
{
    int i;
    int tailroom, crc_len = 0;
    uint16 *from = (uint16*)fkb_p->data;
    BcmEnet_hdr2 *pHdr = (BcmEnet_hdr2 *)from;

    if (pHdr->brcm_type != BRCM_TYPE2)
    {
        uint16 * to = (uint16*)fkb_push(fkb_p, BRCM_TAG_TYPE2_LEN);
        pHdr = (BcmEnet_hdr2 *)to;
        for ( i=0; i<ETH_ALEN; *to++ = *from++, i++ ); /* memmove 2 * ETH_ALEN */
        /* set port of ingress brcm tag */
        pHdr->brcm_tag = port_map;

    }
    /* set ingress brcm tag and TC bit */
    pHdr->brcm_type = BRCM_TAG2_EGRESS | (SKBMARK_GET_Q_PRIO(fkb_p->mark) << 10);
    tailroom = ETH_ZLEN + BRCM_TAG_TYPE2_LEN - fkb_p->len;
    tailroom = (tailroom < 0) ? crc_len : crc_len + tailroom;
    fkb_put(fkb_p, tailroom);
}

#if defined(CONFIG_BCM_6802_MoCA)
/* Will be removed once bmoca-6802.c remove the calling */
void bcmenet_register_moca_fc_bits_cb(void (*cb)(void *, unsigned long *), int isWan, void * arg)
{
   return;
}
EXPORT_SYMBOL(bcmenet_register_moca_fc_bits_cb);
#endif

void (*bcm63xx_wlan_txchainhandler_hook)(struct sk_buff *skb, uint32_t brc_hot_ptr, uint8 wlTxChainIdx) = NULL;
void (*bcm63xx_wlan_txchainhandler_complete_hook)(void) = NULL;
EXPORT_SYMBOL(bcm63xx_wlan_txchainhandler_hook); 
EXPORT_SYMBOL(bcm63xx_wlan_txchainhandler_complete_hook); 

#if defined(ENET_GPON_CONFIG)

#define get_first_gemid_to_ifIdx_mapping(gemId) gem_to_gponifid[gemId][0]

#if defined(DEFINE_ME_TO_USE)
static void print_gemid_to_ifIdx_map(const char *pMark, int gemId)
{
    int ifIdx;
    unsigned int map1 = 0;
    unsigned int map2 = 0;

    for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
    {
        if (gem_to_gponifid[gemId][ifIdx] != UNASSIGED_IFIDX_VALUE)
        { /* Occupied */
            if (ifIdx > 31)
            {
                map2 |= (1<< (ifIdx-32));
            }
            else
            {
                map1 |= (1<<ifIdx);
            }
        }
    }
    if (map1 || map2)
    {
        printk("%s : GEM = %02d Map1 = 0x%08X Map2 = 0x%08X\n",pMark,gemId,map1,map2);
    }
}

static int get_next_gemid_to_ifIdx_mapping(int gemId, int ifId)
{
    int ifIdx;
    if (ifId == UNASSIGED_IFIDX_VALUE)
    { /* get first */
        return get_first_gemid_to_ifIdx_mapping(gemId);
    }
    for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
    {
        if (gem_to_gponifid[gemId][ifIdx] == UNASSIGED_IFIDX_VALUE)
        {
            break;
        }
        if ((ifIdx+1 < MAX_GPON_IFS_PER_GEM) &&
            (gem_to_gponifid[gemId][ifIdx] == ifId))
        {
            return gem_to_gponifid[gemId][ifIdx+1];
        }
    }
    return UNASSIGED_IFIDX_VALUE;
}
#endif /* DEFINE_ME_TO_USE */

static void initialize_gemid_to_ifIdx_mapping(void)
{
    int gemId,ifIdx;
    for (gemId = 0; gemId < MAX_GEM_IDS; ++gemId)
    {
        for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
        {
            gem_to_gponifid[gemId][ifIdx] = UNASSIGED_IFIDX_VALUE;
        }
    }
}
static int add_gemid_to_gponif_mapping(int gemId, int ifId)
{
    int ifIdx;
    for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
    {
        if (gem_to_gponifid[gemId][ifIdx] == UNASSIGED_IFIDX_VALUE)
        { /* Empty */
            gem_to_gponifid[gemId][ifIdx] = ifId;
            return 0;
        }
    }
    printk("Out of resources !! No more ifs available for gem<%d>\n",gemId);
    return -1;
}
static void remove_gemid_to_gponif_mapping(int gemId, int ifId)
{
    int idx;
    int usedIdx;
    int moveIdx;
    for (idx = 0; idx < MAX_GPON_IFS_PER_GEM; ++idx)
    {
        if (gem_to_gponifid[gemId][idx] != UNASSIGED_IFIDX_VALUE)
        { /* Occupied */
            if (gem_to_gponifid[gemId][idx] == ifId)
            {
                /* Remove the mapping */
                gem_to_gponifid[gemId][idx] = UNASSIGED_IFIDX_VALUE;
                moveIdx = idx;
                for (usedIdx = idx+1; usedIdx < MAX_GPON_IFS_PER_GEM; ++usedIdx)
                {
                    if (gem_to_gponifid[gemId][usedIdx] != UNASSIGED_IFIDX_VALUE)
                    {
                        gem_to_gponifid[gemId][moveIdx] = gem_to_gponifid[gemId][usedIdx];
                        gem_to_gponifid[gemId][usedIdx] = UNASSIGED_IFIDX_VALUE;
                        moveIdx = usedIdx;
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            break;
        }
    }
}
static BOOL is_gemid_mapped_to_gponif(int gemId, int ifId)
{
    int ifIdx;
    for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
    {
        if (gem_to_gponifid[gemId][ifIdx] == UNASSIGED_IFIDX_VALUE)
        {
            break;
        }
        if (gem_to_gponifid[gemId][ifIdx] == ifId)
        {
            return TRUE;
        }
    }
    return FALSE;
}
#endif /* defined(ENET_GPON_CONFIG) */

#if defined(ENET_GPON_CONFIG) && defined(SUPPORT_HELLO)

static int dropped_xmit_port = 0;
/* --------------------------------------------------------------------------
    Name: bcm63xx_enet_xmit_port
 Purpose: Send ethernet traffic given a port number
-------------------------------------------------------------------------- */
static int bcm63xx_enet_xmit_port(struct sk_buff * skb, int xPort)
{
    struct net_device * dev;

    BCM_ENET_TX_DEBUG("Send<0x%08x> len=%d xport<%d> "
                  "mark<0x%08x> destQ<%d> gemId<%d> channel<%d>\n",
                  (int)skb, skb->len, xPort, skb->mark,
                  SKBMARK_GET_Q_PRIO(skb->mark), SKBMARK_GET_PORT(skb->mark),
                  SKBMARK_GET_Q_CHANNEL(skb->mark));

     //DUMP_PKT(skb->data, skb->len);

    if (xPort == GPON_PORT_ID) {
        int gemid, gponifid;
        gemid = SKBMARK_GET_PORT(skb->mark);
        gponifid = get_first_gemid_to_ifIdx_mapping(gemid);
        BCM_ENET_INFO("gponifid<%d> \n ", gponifid);
        if ( gponifid < 0 )
        {
            BCM_ENET_INFO("-ve gponifid, dropping pkt \n");
            dropped_xmit_port++;
            return -1;
        }
        dev = gponifid_to_dev[gponifid];
        if ( dev == (struct net_device*)NULL )
        {
            dropped_xmit_port++;
            return -1;
        }
        BCM_ENET_INFO("dev<0x%08x:%s>\n", (int)dev, dev->name);
    }
    else
    {
        dev = vnet_dev[ LOGICAL_PORT_TO_VPORT(xPort) ];
    }

    return bcm63xx_enet_xmit( SKBUFF_2_PNBUFF(skb), dev );
}
#endif  /* defined(SUPPORT_HELLO) */

#if defined(CONFIG_SKY_MESH_SUPPORT)

#define HOMEPLUGAV_PKTTYPE      0x88E1
#define AUTOMESH_PKTTYPE        0x7373
#define AUTOMESH_ALETHD_PKTTYPE 0x7374
#define AIR_EOE_PKTTYPE         0x7474
#define AIR_EOE_TUNNELD_PKTTYPE 0x7475
// Sky video
#define AIR_EOE_AF11_PKTTYPE 	0x7428
// sky proximity check
#define AIR_EOE_SPC_PKTTYPE 	0x74D0
// automesh control
#define AIR_EOE_ACP_PKTTYPE 	0x74E0

#define	IPTOS_DSCP_MASK		0xfc
#define	IPTOS_DSCP(x)		((x) & IPTOS_DSCP_MASK)
#define	IPTOS_DSCP_AF11		0x28
#define	IPTOS_DSCP_AF12		0x30
#define	IPTOS_DSCP_AF13		0x38
#define	IPTOS_DSCP_AF21		0x48
#define	IPTOS_DSCP_AF22		0x50
#define	IPTOS_DSCP_AF23		0x58
#define	IPTOS_DSCP_AF31		0x68
#define	IPTOS_DSCP_AF32		0x70
#define	IPTOS_DSCP_AF33		0x78
#define	IPTOS_DSCP_AF41		0x88
#define	IPTOS_DSCP_AF42		0x90
#define	IPTOS_DSCP_AF43		0x98
#define	IPTOS_DSCP_EF		0xb8

/* skb->mark specifies the packet priority. Parse the packet and set the priority */
static inline void set_skb_mark(struct sk_buff *skb)
{
	struct iphdr *iph;
	unsigned char *dest_mac;
	u_int16_t *proto;
	unsigned char *data = skb->data;
	unsigned int len = skb->len;
	u_int8_t prio;
	static const u_int8_t prio2ac[8] = { 0, 1, 1, 0, 2, 2, 3, 3 };

	if (unlikely(len < sizeof(struct ethhdr)))
		return;
	dest_mac = data;
	proto = (u_int16_t *)(data + sizeof(struct ethhdr) - 2);
	data += sizeof(struct ethhdr);
	len -= sizeof(struct ethhdr);

	/* strip the air-eoe tunnel headers and VLAN headers if present */
	while ((*proto == __constant_htons(ETH_P_8021Q)) || (*proto == __constant_htons(AIR_EOE_PKTTYPE))) {
		if (*proto == __constant_htons(AIR_EOE_PKTTYPE)) {
			if (unlikely(len < sizeof(struct ethhdr)))
				return;
			dest_mac = data;
			proto = (u_int16_t *)(data + sizeof(struct ethhdr) - 2);
			data += sizeof(struct ethhdr);
			len -= sizeof(struct ethhdr);
		} else {
			/* VLAN header is 4 bytes, next proto offset is 2 */
			if (unlikely(len < 4))
				return;
			proto = (u_int16_t *)(data + 2);
			data += 4;
			len -= 4;
		}
	}

	/* Prioritise STP protocol packets 01:80:c2:..... */
	if (unlikely((dest_mac[0] == 0x01) && (dest_mac[1] == 0x80) && (dest_mac[2] == 0xc2))) {
		skb->mark = 3;
		return;
	}

	/* Prioritise AirTies and ARP protocol types */
	if (unlikely((*proto == __constant_htons(HOMEPLUGAV_PKTTYPE)) ||
                     (*proto == __constant_htons(AUTOMESH_PKTTYPE)) ||
                     (*proto == __constant_htons(AUTOMESH_ALETHD_PKTTYPE)) ||
                     (*proto == __constant_htons(ETH_P_ARP)) ||
                     (*proto == __constant_htons(AIR_EOE_TUNNELD_PKTTYPE)))) {
		skb->mark = 3;
		return;
	}

	/* prioritise Airties Control packet types */
	if( *proto == __constant_htons(AIR_EOE_ACP_PKTTYPE)){
	    skb->mark = 3;
	    return;
	}

	/* prioritise Sky Proximity Check packet types */
	if( *proto == __constant_htons(AIR_EOE_SPC_PKTTYPE)){
	    skb->mark = 3;
	    return;
	}

	/* prioritise Sky Video/AF11 packet types */
	if( *proto == __constant_htons(AIR_EOE_AF11_PKTTYPE)){
	    skb->mark = 2;
	    return;
	}

	/* Prioritise IP packets by TOS */
	if (unlikely(*proto != __constant_htons(ETH_P_IP)))
		return;
	if (unlikely(len < sizeof(struct iphdr)))
		return;
	iph = (struct iphdr *)data;
	data += sizeof(struct iphdr);
	len -= sizeof(struct iphdr);

	switch (IPTOS_DSCP(iph->tos)) {
	case IPTOS_DSCP_AF11:
		prio = 5;	/* Sky Video Traffic */
		break;
	case 0xD0:	 			/* Sky Proximity Check */
		prio = 7;
		break;
	case IPTOS_DSCP_AF41: 			/* Downgrade non-Sky video trafic priority */
		prio = 4;
		break;
	default:
		prio = iph->tos >> 5;
		break;
	}

	skb->mark = prio2ac[prio & 0x7];
}
#endif // CONFIG_SKY_MESH_SUPPORT

/* --------------------------------------------------------------------------
    Name: bcm63xx_enet_xmit
 Purpose: Send ethernet traffic
-------------------------------------------------------------------------- */
static int bcm63xx_enet_xmit(pNBuff_t pNBuff, struct net_device *dev)
{
    bool is_chained = FALSE;
#if defined(PKTC)
    pNBuff_t pNBuff_next = NULL;
#endif
    EnetXmitParams param = { {0}, 0};
    int ret;
	
#if defined( CONFIG_SKY_MESH_SUPPORT)
    if (IS_SKBUFF_PTR(pNBuff))
        set_skb_mark(PNBUFF_2_SKBUFF(pNBuff));
#endif //CONFIG_SKY_MESH_SUPPORT

#if defined(PKTC)
    /* for PKTC, pNBuff is chained skb */
    if (IS_SKBUFF_PTR(pNBuff))
    {
        is_chained = PKTISCHAINED(pNBuff);
    }
#endif

    do {
        param.pDevPriv = netdev_priv(dev);
        param.vstats   = &param.pDevPriv->stats;
        param.port_id  = port_id_from_dev(dev);
        param.pNBuff = pNBuff;
        param.is_chained = is_chained;
        BCM_ENET_TX_DEBUG("The physical port_id is %d\n", param.port_id);

        if (nbuff_get_params_ext(pNBuff, &param.data, &param.len,
                    &param.mark, &param.priority, &param.r_flags) == NULL)
            return 0;

        if (global.dump_enable)
            DUMP_PKT(param.data, param.len);

#ifdef USE_DEFAULT_EGRESS_QUEUE
        if (param.pDevPriv->use_default_txq)
        {
            BCM_ENET_TX_DEBUG("Using default egress queue %d \n", param.egress_queue);
            param.egress_queue = SKBMARK_GET_Q_PRIO(param.mark);
            bcmeapi_select_tx_def_queue(&param);
        }
        else
#endif
        {
            BCM_ENET_TX_DEBUG("Using mark for channel and queue \n");
            param.egress_queue = SKBMARK_GET_Q_PRIO(param.mark);
            bcmeapi_select_tx_nodef_queue(&param);
        }

        BCM_ENET_TX_DEBUG("The egress queue is %d \n", param.egress_queue);

#if defined(PKTC)
        if (is_chained)
            pNBuff_next = PKTCLINK(pNBuff);
#endif

        ret = bcm63xx_enet_xmit2(dev, &param);

#if defined(PKTC)
        if (is_chained) 
            pNBuff = pNBuff_next;
#endif

    } while (is_chained && pNBuff && IS_SKBUFF_PTR(pNBuff));

    return ret;
}

static inline int bcm63xx_enet_xmit2(struct net_device *dev, EnetXmitParams *pParam)
{
    unsigned int port_map = (1 << pParam->port_id);

    pParam->gemid = -1;
    /* tx request should never be on the bcmsw interface */
    BCM_ASSERT_R((dev != vnet_dev[0]), 0);

    if(IS_FKBUFF_PTR(pParam->pNBuff))
    {
        pParam->pFkb = PNBUFF_2_FKBUFF(pParam->pNBuff);
    }
    else
    {
        pParam->skb = PNBUFF_2_SKBUFF(pParam->pNBuff);
    }
    bcmeapi_get_tx_queue(pParam);

#if defined(ENET_GPON_CONFIG)
    /* If GPON port, get the gemid from mark and put it in the BD */
    if (LOGICAL_PORT_TO_PHYSICAL_PORT(pParam->port_id) == GPON_PORT_ID) {  /* port_id is logical */
        BcmEnet_devctrl *pDevCtrl = pParam->pDevPriv;
        int gemid = -1;

        switch (pDevCtrl->gem_count) {
            case 0:
                BCM_ENET_TX_DEBUG("No gem_ids, so dropping Tx \n");
                global.pVnetDev0_g->stats.tx_dropped++;
                pParam->vstats->tx_dropped++;
                goto drop_exit;

            case 1:
                BCM_ENET_TX_DEBUG("Using the only gem_id\n");
                gemid = default_gemid[pDevCtrl->gponifid];
                break;

            default:
                BCM_ENET_TX_DEBUG("2 or more gem_ids, get gem_id from mark\n");
                gemid = SKBMARK_GET_PORT(pParam->mark);
                if (get_first_gemid_to_ifIdx_mapping(gemid) != pDevCtrl->gponifid) {
                    BCM_ENET_TX_DEBUG("The given gem_id is not associated with"
                                     " the given interface\n");
                    global.pVnetDev0_g->stats.tx_dropped++;
                    pParam->vstats->tx_dropped++;
                    goto drop_exit;
                }
                break;
        }
        BCM_ENET_TX_DEBUG("port_id<%d> gem_count<%d> gemid<%d>\n", pParam->port_id,
                          pDevCtrl->gem_count, gemid );
        /* gemid is set later in bcmPktDma implementation of enet driver */
        pParam->blog_chnl = gemid;
        pParam->blog_phy  = BLOG_GPONPHY;
        pParam->gemid = gemid;
    }
    else    /* ! GPON_PORT_ID */
#endif
#if defined(ENET_EPON_CONFIG)
    if (LOGICAL_PORT_TO_PHYSICAL_PORT(pParam->port_id) == EPON_PORT_ID)  /* use always wan_flow=1 for epon upstream data traffic */        
    {
        pParam->blog_chnl = LLID_SINGLE;
        pParam->blog_phy = BLOG_EPONPHY;
    }
    else
#endif
    {
        pParam->blog_chnl = pParam->port_id;
        pParam->blog_phy  = BLOG_ENETPHY;
    }

#ifdef CONFIG_BLOG
    /*
     * Pass to blog->fcache, so it can construct the customized
     * fcache based execution stack.
     */
    if (pParam->is_chained == FALSE)
    {
        blog_emit( pParam->pNBuff, dev, TYPE_ETH, pParam->blog_chnl, pParam->blog_phy ); /* CONFIG_BLOG */
    }
#endif

    bcmeapi_buf_reclaim(pParam);

    if(bcmeapi_queue_select(pParam) == BCMEAPI_CTRL_BREAK)
    {
        goto unlock_drop_exit;
    }

    bcmeapi_config_tx_queue(pParam);

    if (IsExternalSwitchPort(pParam->port_id))
    {
        if ( pParam->pFkb ) {
            FkBuff_t * pFkbOrig = pParam->pFkb;
            pParam->pFkb = fkb_unshare(pFkbOrig);

            if (pParam->pFkb == FKB_NULL)
            {
                fkb_free(pFkbOrig);
                global.pVnetDev0_g->stats.tx_dropped++;
                pParam->vstats->tx_dropped++;
                goto unlock_exit;
            }
            bcm63xx_fkb_put_tag(pParam->pFkb, dev, GET_PORTMAP_FROM_LOGICAL_PORTMAP(port_map, 1)); /* Portmap for external switch */
            pParam->data = pParam->pFkb->data;
            pParam->len  = pParam->pFkb->len;
            pParam->pNBuff = PBUF_2_PNBUFF((void*)pParam->pFkb,FKBUFF_PTR);
        } else {
#if !defined(CONFIG_BCM968500) && !defined(CONFIG_BCM96838)   /* FIXME: Temporary work around */
            ENET_TX_UNLOCK();
#endif
            pParam->skb = bcm63xx_skb_put_tag(pParam->skb, dev, GET_PORTMAP_FROM_LOGICAL_PORTMAP(port_map,1));    /* Portmap for external switch and also pads to 0 */
#if !defined(CONFIG_BCM968500) && !defined(CONFIG_BCM96838)   /* FIXME: Temporary work around */
            ENET_TX_LOCK();
#endif
            if (pParam->skb == NULL) {
                global.pVnetDev0_g->stats.tx_dropped++;
                pParam->vstats->tx_dropped++;
                goto unlock_exit;
            }
            pParam->data = pParam->skb->data;   /* Re-encode pNBuff for adjusted data and len */
            pParam->len  = pParam->skb->len;
            pParam->pNBuff = PBUF_2_PNBUFF((void*)pParam->skb,SKBUFF_PTR);
        }

    }

    if ( pParam->len < ETH_ZLEN )
    {
        pParam->len = ETH_ZLEN;
    }


#if defined(CONFIG_BCM96368)
    if(kerSysGetSdramWidth() == MEMC_16BIT_BUS)
    {
        pParam->pNBuff = nbuff_align_data(pParam->pNBuff, &pParam->data, pParam->len, NBUFF_ALIGN_MASK_8);
        if(pParam->pNBuff == NULL)
        {
            global.pVnetDev0_g->stats.tx_dropped++;
            pParam->vstats->tx_dropped++;
            goto unlock_exit; 
        }
    }
#endif

    switch(bcmeapi_pkt_xmt_dispatch(pParam))
    {
#if defined(ENET_GPON_CONFIG)
        case BCMEAPI_CTRL_BREAK:
#if !defined(CONFIG_BCM968500) && !defined(CONFIG_BCM96838)   /* FIXME: Temporary work around */
            ENET_TX_UNLOCK();
#endif
            goto drop_exit;
#endif
        case BCMEAPI_CTRL_SKIP:
            goto unlock_drop_exit;
        default:
            break;
    }

#ifdef DYING_GASP_API
	/* If in dying gasp, abort housekeeping since we're about to power down */
	if(dg_in_context)
		return 0;
#endif

    /* update stats */
    pParam->vstats->tx_bytes += pParam->len + ETH_CRC_LEN;
    pParam->vstats->tx_packets++;

    global.pVnetDev0_g->stats.tx_bytes += pParam->len + ETH_CRC_LEN;
    global.pVnetDev0_g->stats.tx_packets++;
    global.pVnetDev0_g->dev->trans_start = jiffies;

unlock_exit:
    bcmeapi_xmit_unlock_exit_post(pParam);
    return 0;

unlock_drop_exit:
    bcmeapi_xmit_unlock_drop_exit_post(pParam);
    return 0;

#if defined(ENET_GPON_CONFIG)
drop_exit:
    nbuff_flushfree(pParam->pNBuff);
    return 0;
#endif
}



#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 1)
/* Driver for kernel 3.4 uses a dedicated thread for rx processing */
static int bcm63xx_enet_rx_thread(void *arg)
{
    struct BcmEnet_devctrl *pDevCtrl=(struct BcmEnet_devctrl *) arg;
    uint32 work_done;
    uint32 ret_done;
    int budget = NETDEV_WEIGHT;


    while (1)
    {
        wait_event_interruptible(pDevCtrl->rx_thread_wqh,
                                 pDevCtrl->rx_work_avail);

        if (kthread_should_stop())
        {
            printk(KERN_INFO "kthread_should_stop detected on bcmsw-rx\n");
            break;
        }

        local_bh_disable();
        /* got some work to do */
        bcmeapi_update_rx_queue(pDevCtrl);

        work_done = bcm63xx_rx(pDevCtrl, budget);
        ret_done = work_done & ENET_POLL_DONE;
        work_done &= ~ENET_POLL_DONE;
        local_bh_enable();

        BCM_ENET_RX_DEBUG("Work Done: %d \n", (int)work_done);

        if (work_done >= budget || ret_done != ENET_POLL_DONE)
        {
            bcmeapi_napi_leave(pDevCtrl);

            /* We have either exhausted our budget or there are
               more packets on the DMA (or both).  Yield CPU to allow
               others to have a chance, then continue to top of loop for more
               work.  */
            if (current->policy == SCHED_FIFO || current->policy == SCHED_RR)
                yield();
        }
        else
        {
            /*
             * No more packets.  Indicate we are done (rx_work_avail=0) and
             * re-enable interrupts (bcmeapi_napi_post) and go to top of
             * loop to wait for more work.
             */
            pDevCtrl->rx_work_avail = 0;
            bcmeapi_napi_post(pDevCtrl);
        }
    }

    return 0;
}
#else
/* Driver for kernel 2.6.30 uses NAPI. */
static int bcm63xx_enet_poll_napi(struct napi_struct *napi, int budget)
{
    struct BcmEnet_devctrl *pDevCtrl = container_of(napi, struct BcmEnet_devctrl, napi);

    uint32 work_done;
    uint32 ret_done;

    bcmeapi_update_rx_queue(pDevCtrl);

    work_done = bcm63xx_rx(pDevCtrl, budget);
    ret_done = work_done & ENET_POLL_DONE;
    work_done &= ~ENET_POLL_DONE;

    BCM_ENET_RX_DEBUG("Work Done: %d \n", (int)work_done);

    if (work_done == budget || ret_done != ENET_POLL_DONE)
    {
        bcmeapi_napi_leave(pDevCtrl);
        /* We have either exhausted our budget or there are
           more packets on the DMA (or both).  Simply
          return, and the framework will reschedule
          this function automatically */
        return work_done;
    }

    /* we are done processing packets */

    napi_complete(napi);
    bcmeapi_napi_post(pDevCtrl);
    return work_done;
}
#endif  /* LINUX_VERSION_CODE */

static inline struct net_device *phyPortId_to_netdev(int phy_port_id, int gemid)
{
    struct net_device *dev = NULL;
#if defined(ENET_GPON_CONFIG)
    /* If packet is from GPON port, get the gemid and find the gpon virtual
       interface with which that gemid is associated */
    if ( phy_port_id == GPON_PORT_ID)
    {
        int gponifid;

        gponifid = get_first_gemid_to_ifIdx_mapping(gemid);
        if (gponifid >= 0)
        {
            dev = gponifid_to_dev[gponifid];
        }

    } else
#endif
#if defined(ENET_EPON_CONFIG)
        /* ************************************************************************
         * If packet is from EPON port, get the link index and find the epon virtual
         * interface with which corresponds to the link 
         **************************************************************************/
        if ( phy_port_id == EPON_PORT_ID)
        {
            dev = eponifid_to_dev[0];
        } else
#endif    
        {
            int vport;
            /*lan ports */

            vport = LOGICAL_PORT_TO_VPORT(phy_port_id);
            BCM_ENET_RX_DEBUG("phy_port_id=%d vport=%d\n", (int)phy_port_id, vport);

            if ((vport > 0) && (vport <= vport_cnt))
            {
                dev = vnet_dev[vport];
            }

        } 
        return dev;
}

/*
 *  bcm63xx_rx: Process all received packets.
 */
static uint32 bcm63xx_rx(void *ptr, uint32 budget)
{
    BcmEnet_devctrl *pDevCtrl = ptr;
    struct net_device *dev = NULL;
    unsigned char *pBuf = NULL;
    struct sk_buff *skb = NULL;
    int len=0, phy_port_id = -1, no_stat = 0, ret;
    uint32 rxpktgood = 0, rxpktprocessed = 0;
    uint32 rxpktmax = budget + (budget / 2);
    struct net_device_stats *vstats;
    FkBuff_t * pFkb = NULL;
    uint32_t blog_chnl, blog_phy; /* used if CONFIG_BLOG enabled */
    uint32 cpuid=0;  /* initialize to silence compiler.  It is correctly set at runtime */
	int msg_id = -1; /* Returned by the received of packet */
	int is_chained = 0;
	uint32 rxContext1 = 0, rxContext2 = 0;
        int is_wifi_port;

    int gemid = 0;
#if defined(CONFIG_BLOG)
    BlogAction_t blogAction;
#endif
    /* bulk blog locking optimization only used in SMP builds */
    int got_blog_lock=0;

    // TBD -- this can be looked into but is not being done for now
    /* When the Kernel is upgraded to 2.6.24 or above, the napi call will
       tell you the received queue to be serviced. So, loop across queues
       can be removed. */
    /* RR loop across channels until either no more packets in any channel or
       we have serviced budget number of packets. The logic is to keep the
       channels to be serviced in next_channel array with channels_tbd
       tracking the number of channels that still need to be serviced. */
    for(bcmeapi_prepare_rx(); budget > 0; dev = NULL, pBuf = NULL, skb = NULL)
    {

        /* as optimization on SMP, hold blog lock across multiple pkts */
        /* must grab blog_lock before enet_rx_lock */
        if (!got_blog_lock)
        {
            blog_lock_bh();
            got_blog_lock=1;
            /*
             * Get the processor id AFTER we acquire the blog_lock.
             * Holding a lock disables preemption and migration, so we know
             * the processor id will not change as long as we hold the lock.
             */
            cpuid = smp_processor_id();
        }

        /* as optimization on SMP, hold rx lock across multiple pkts */
        if (0 == BULK_RX_LOCK_ACTIVE())
        {
            ENET_RX_LOCK();
            RECORD_BULK_RX_LOCK();
        }

        ret = bcmeapi_rx_pkt(pDevCtrl, &pBuf, &pFkb, &len, &gemid, &phy_port_id, &is_wifi_port, &dev, &rxpktgood, &msg_id, &rxContext1, &rxContext2);
        
        if(ret & BCMEAPI_CTRL_FLAG_TRUE)
        {
            no_stat = 1;
            ret &= ~BCMEAPI_CTRL_FLAG_MASK;
        }
        else
        {
            no_stat = 0;
        }

        switch(ret)
        {
            case BCMEAPI_CTRL_BREAK:
                goto after_rx;
            case BCMEAPI_CTRL_CONTINUE:
                goto next_rx;
            case BCMEAPI_CTRL_SKIP:
                continue;
            default:
                break;
        }

        BCM_ENET_RX_DEBUG("Processing Rx packet");
        rxpktprocessed++;


        if (!is_wifi_port)/*TODO remove is_wifi_port from enet driver*/
        {
            dev = phyPortId_to_netdev(phy_port_id, gemid);
        }

        if(dev == NULL)
        {
            /* possibility of corrupted source port in dmaFlag */
            if (!no_stat)
                pDevCtrl->stats.rx_dropped++;
            RECORD_BULK_RX_UNLOCK();
            ENET_RX_UNLOCK();
            bcmeapi_kfree_buf_irq(pDevCtrl, pFkb, pBuf);
            BCM_ENET_INFO("ETH Rcv: Pkt with invalid phy_port_id/vport(0x%x/0x%x) or gemid = 0x%x\n",
                phy_port_id, LOGICAL_PORT_TO_VPORT(phy_port_id), gemid);
            goto next_rx;
        }

        if (!no_stat)
        {
			/* Store packet & byte count in our portion of the device structure */
            vstats = &(((BcmEnet_devctrl *) netdev_priv(dev))->stats);
            vstats->rx_packets ++;
            vstats->rx_bytes += len;

			/* Store packet & byte count in switch structure */
            pDevCtrl->stats.rx_packets++;
            pDevCtrl->stats.rx_bytes += len;
        }

		if (bcmeapi_is_wl_tx_chain_pkt(msg_id)) 
		{

			is_chained |= bcmeapi_wl_tx_chain(pDevCtrl, pBuf, len, rxContext1, rxContext2);
			
			goto next_rx;
		}

#if defined(ENET_GPON_CONFIG)
        if (phy_port_id == GPON_PORT_ID) {
            blog_chnl = gemid;      /* blog rx channel is gemid */
            blog_phy = BLOG_GPONPHY;/* blog rx phy type is GPON */
        }
        else
#endif
#if defined(ENET_EPON_CONFIG)
        if (phy_port_id == EPON_PORT_ID) {
            blog_chnl = LLID_SINGLE;
            blog_phy = BLOG_EPONPHY;/* blog rx phy type is EPON */
        }
        else
#endif
        {
#if 1 /*TODO remove is_wifi_port */
            if (is_wifi_port)
            {
                blog_chnl = 0;
                blog_phy = BLOG_WLANPHY;/* blog rx phy type is WLAN */
            }
            else
#endif
            {
                blog_chnl = phy_port_id;/* blog rx channel is switch port */
                blog_phy = BLOG_ENETPHY;/* blog rx phy type is ethernet */
            }
        }

        /* FkBuff_t<data,len> in-placed leaving headroom */
#if defined(CONFIG_BCM968500) || defined(CONFIG_BCM96838)
    /*fkb is already created by rdpa, just use it */
#else
        pFkb = fkb_init(pBuf, BCM_PKT_HEADROOM,
                pBuf, len - ETH_CRC_LEN );
#endif
        bcmeapi_set_fkb_recycle_hook(pFkb);

#if defined(CONFIG_BCM96818)
        if (blog_phy == BLOG_GPONPHY && gemMirrorCfg[MIRROR_DIR_IN].nStatus &&
                (gemMirrorCfg[MIRROR_DIR_IN].nGemPortMaskArray[blog_chnl/8] & (1 << (blog_chnl % 8))))
        {
            struct sk_buff *skb_m;
            FkBuff_t *fkbC_p;

            fkbC_p = fkb_clone( pFkb );
            skb_m = nbuff_xlate( FKBUFF_2_PNBUFF(fkbC_p) );    /* translate to skb */
            if (skb_m != (struct sk_buff *)NULL)
            {
                MirrorPacket(skb_m, gemMirrorCfg[MIRROR_DIR_IN].szMirrorInterface, 1, 0);
                dev_kfree_skb_any( skb_m );
            }
        }
#endif

#ifdef CONFIG_BLOG
        /* SMP: bulk rx, bulk blog optimization */
        blogAction = blog_finit_locked( pFkb, dev, TYPE_ETH, blog_chnl, blog_phy );

        if ( blogAction == PKT_DROP )
        {
            bcmeapi_blog_drop(pDevCtrl, pFkb, pBuf);

            /* Store dropped packet count in our portion of the device structure */
            vstats = &(((BcmEnet_devctrl *) netdev_priv(dev))->stats);
            vstats->rx_dropped++;
        
            /* CPU is congested and fcache has identified the packet
             * as low prio, and needs to be dropped */
            pDevCtrl->stats.rx_dropped++;
            goto next_rx;
        }
        else
        {
            bcmeapi_buf_alloc(pDevCtrl);
        }

        /* packet consumed, proceed to next packet*/
        if ( blogAction == PKT_DONE )
        {
            goto next_rx;
        }

#endif /* CONFIG_BLOG */
    
        /*allocate skb & initialize it using fkb */

        if (bcmeapi_alloc_skb(pDevCtrl, &skb) == BCMEAPI_CTRL_FALSE) {
            RECORD_BULK_RX_UNLOCK();
            ENET_RX_UNLOCK();
            fkb_release(pFkb);
            pDevCtrl->stats.rx_dropped++;
            bcmeapi_kfree_buf_irq(pDevCtrl, pFkb, pBuf);
            if ( rxpktprocessed < rxpktmax )
                continue;
            break;
        }

        /*
         * We are outside of the fast path and not touching any
         * critical variables, so release all locks.
         */
        RECORD_BULK_RX_UNLOCK();
        ENET_RX_UNLOCK();

        got_blog_lock=0;
        blog_unlock_bh();

        if(bcmeapi_skb_headerinit(len, pDevCtrl, skb, pFkb, pBuf) == BCMEAPI_CTRL_CONTINUE)
        {
            bcmeapi_kfree_buf_irq(pDevCtrl, pFkb, pBuf);
            goto next_rx;
        }
        

#if defined(ENET_GPON_CONFIG)
        skb->mark = SKBMARK_SET_PORT(skb->mark, gemid);
#endif
        skb->protocol = bcm_type_trans(skb, dev);
        skb->dev = dev;
        if (global.dump_enable) {
            DUMP_PKT(skb->data, skb->len);
        }
        netif_receive_skb(skb);

next_rx:
        budget--;
    
        if (bcmeapi_prepare_next_rx(&rxpktgood) != BCMEAPI_CTRL_CONTINUE)
        {
            break;
        }
    } /* end while (budget > 0) */

after_rx:
    pDevCtrl->dev->last_rx = jiffies;

    if (got_blog_lock)
    {
        /*
         * Only need to check for BULK_RX_LOCK_ACTIVE if we have the blog_lock.
         * And we have the blog_lock, then cpuid was correctly set at runtime.
         */
        if (BULK_RX_LOCK_ACTIVE())
        {
            RECORD_BULK_RX_UNLOCK();
            ENET_RX_UNLOCK();
        }

        got_blog_lock=0;
        blog_unlock_bh();
    }

	if (is_chained) 
	{
		bcmapi_wl_tx_chain_post(budget);
	}

    bcmeapi_rx_post(&rxpktgood);

    BCM_ASSERT_C(0 == got_blog_lock);
    BCM_ASSERT_NOT_HAS_SPINLOCK_C(&global.pVnetDev0_g->ethlock_rx);


    return rxpktgood;
}


static int bcm_set_soft_switching(int swPort, int type)
{
    int vport;

    BcmEnet_devctrl *pDevCtrl = netdev_priv(vnet_dev[0]);
    ASSERT(pDevCtrl != NULL);

    vport = LOGICAL_PORT_TO_VPORT(swPort);
    if (type == TYPE_ENABLE) {
        pDevCtrl->wanPort  |= (1 << swPort);
        pDevCtrl->softSwitchingMap  |= (1 << swPort);
        vnet_dev[vport]->priv_flags &= ~IFF_HW_SWITCH;    
    }
    else if (type == TYPE_DISABLE) {
        pDevCtrl->wanPort &= ~(1 << swPort);
        pDevCtrl->softSwitchingMap &= ~(1 << swPort);
        vnet_dev[vport]->priv_flags |= IFF_HW_SWITCH;
    }
    else {
        return -EINVAL;            
    }
#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828) || defined(CONFIG_BCM96318)
    if (IsExternalSwitchPort(swPort))
    {
        extsw_set_wanoe_portmap(GET_PORTMAP_FROM_LOGICAL_PORTMAP(pDevCtrl->wanPort,1));
    }
    else
    {
        ethsw_set_wanoe_portmap(GET_PORTMAP_FROM_LOGICAL_PORTMAP(pDevCtrl->wanPort,0), pDevCtrl->txSoftSwitchingMap);
    }
#else
    if (IsExternalSwitchPort(swPort))
    {
        extsw_set_wanoe_portmap(GET_PORTMAP_FROM_LOGICAL_PORTMAP(pDevCtrl->wanPort,1));
    }
    else
    {
        ethsw_port_based_vlan(pDevCtrl->EnetInfo[0].sw.port_map,
                              pDevCtrl->wanPort,
                              pDevCtrl->txSoftSwitchingMap);
    }
#endif

    return 0;
}

/*
 * Set the hardware MAC address.
 */
static int bcm_set_mac_addr(struct net_device *dev, void *p)
{
    struct sockaddr *addr = p;

    if(netif_running(dev))
        return -EBUSY;

    memcpy(dev->dev_addr, addr->sa_data, dev->addr_len);
    return 0;
}

/*
 * bcm63xx_init_dev: initialize Ethernet MACs,
 * allocate Tx/Rx buffer descriptors pool, Tx header pool.
 * Note that freeing memory upon failure is handled by calling
 * bcm63xx_uninit_dev, so no need of explicit freeing.
 */
static int bcm63xx_init_dev(BcmEnet_devctrl *pDevCtrl)
{
    int rc = 0;

    TRACE(("bcm63xxenet: bcm63xx_init_dev\n"));

    bcmenet_in_init_dev = 1;

    /* Handle pkt rate limiting independently in the FAP. No need for global array */

    if ((rc = bcmeapi_init_queue(pDevCtrl)) < 0)
    {
        return rc;
    }

    pDevCtrl->use_default_txq = 0;

    bcmeapiInterruptEnable(0);

    bcmenet_in_init_dev = 0;
    /* if we reach this point, we've init'ed successfully */
    return 0;
}

/* Uninitialize tx/rx buffer descriptor pools */
static int bcm63xx_uninit_dev(BcmEnet_devctrl *pDevCtrl)
{
    if (pDevCtrl) {

        bcmeapiInterruptEnable(0);
        bcmeapi_free_queue(pDevCtrl);
        bcmeapi_free_irq(pDevCtrl);

        /* Deleate the proc files */
        ethsw_del_proc_files();

        /* unregister and free the net device */
        if (pDevCtrl->dev) {
            if (pDevCtrl->dev->reg_state != NETREG_UNINITIALIZED) {
                kerSysReleaseMacAddress(pDevCtrl->dev->dev_addr);
                unregister_netdev(pDevCtrl->dev);
            }
            free_netdev(pDevCtrl->dev);
        }
    }

    return 0;
}

/*
 *      bcm63xx_enet_probe: - Probe Ethernet switch and allocate device
 */
int __init bcm63xx_enet_probe(void)
{
    static int probed = 0;
    struct net_device *dev = NULL;
    BcmEnet_devctrl *pDevCtrl = NULL;
    unsigned int chipid;
    unsigned int chiprev;
    unsigned char macAddr[ETH_ALEN];
    ETHERNET_MAC_INFO EnetInfo[BP_MAX_ENET_MACS];
    int status = 0, unit = 0, ret;
    BcmEnet_devctrl *pVnetDev0;

    TRACE(("bcm63xxenet: bcm63xx_enet_probe\n"));

    if (probed == 0)
    {
         bcmeapi_get_chip_idrev(&chipid, &chiprev);

        if(BpGetEthernetMacInfo(&EnetInfo[0], BP_MAX_ENET_MACS) != BP_SUCCESS)
        {
            printk(KERN_DEBUG CARDNAME" board id not set\n");
            return -ENODEV;
        }
        probed++;
    }
    else
    {
        /* device has already been initialized */
        return -ENXIO;
    }

    if ((EnetInfo[1].ucPhyType == BP_ENET_EXTERNAL_SWITCH) ||
            (EnetInfo[1].ucPhyType == BP_ENET_SWITCH_VIA_INTERNAL_PHY)) {
        unit = 1;
    }

#ifdef NO_CFE
    ethsw_reset();
    ethsw_configure_ports(EnetInfo[0].sw.port_map, (int *) &EnetInfo[0].sw.phy_id[0]);
#endif

    dev = alloc_etherdev(sizeof(*pDevCtrl));
    if (dev == NULL)
    {
        printk(KERN_ERR CARDNAME": Unable to allocate net_device!\n");
        return -ENOMEM;
    }

    pDevCtrl = netdev_priv(dev);
    memset(netdev_priv(dev), 0, sizeof(BcmEnet_devctrl));
    pDevCtrl->dev = dev;
    pDevCtrl->unit = unit;
    pDevCtrl->chipId  = chipid;
    pDevCtrl->chipRev = chiprev;
#if defined(CONFIG_BCM_GMAC)
    pDevCtrl->gmacPort = 0;
#endif
    pDevCtrl->softSwitchingMap = 0;
    pDevCtrl->txSoftSwitchingMap = 0;
    pDevCtrl->stpDisabledPortMap = 0;
    
    // Create a port map with only end ports. A port connected to external switch is ignored.
    consolidated_portmap = PHYSICAL_PORTMAP_TO_LOGICAL_PORTMAP(EnetInfo[0].sw.port_map, 0); /* Internal Switch portmap */  
    if (unit == 1)
    {
        consolidated_portmap |= PHYSICAL_PORTMAP_TO_LOGICAL_PORTMAP(EnetInfo[1].sw.port_map, 1); /* External Switch portmap */
        extSwInfo.connected_to_internalPort = BpGetPortConnectedToExtSwitch();
        consolidated_portmap &= ~SET_PORT_IN_LOGICAL_PORTMAP(extSwInfo.connected_to_internalPort, 0);
    }        
    
    global.pVnetDev0_g = pDevCtrl;
    global.pVnetDev0_g->extSwitch = &extSwInfo;
#if defined(CONFIG_BCM_PKTDMA_TX_SPLITTING)
    global.pVnetDev0_g->enetTxChannel = PKTDMA_ETH_TX_HOST_IUDMA;   /* default for enet tx on HOST */
#endif
    if (unit == 1) {
        int bus_type, spi_id;
        /* get external switch access details */
        get_ext_switch_access_info(EnetInfo[1].usConfigType, &bus_type, &spi_id);
        extSwInfo.accessType = bus_type;
        extSwInfo.bus_num = (bus_type == MBUS_SPI)?LEG_SPI_BUS_NUM:HS_SPI_BUS_NUM;
        extSwInfo.spi_ss = spi_id;
        extSwInfo.spi_cid = 0;
        extSwInfo.brcm_tag_type = BRCM_TYPE2;
        extSwInfo.present = 1;
        status = bcmeapi_init_ext_sw_if(&extSwInfo);
#if defined(CONFIG_BCM_PORTS_ON_INT_EXT_SW)
        pDevCtrl->wanPort |= 1 << (PHYSICAL_PORT_TO_LOGICAL_PORT(extSwInfo.connected_to_internalPort, 0));
#endif
    }

    spin_lock_init(&pDevCtrl->ethlock_tx);
    spin_lock_init(&pDevCtrl->ethlock_rx);
    spin_lock_init(&bcm_ethlock_phy_access);
    spin_lock_init(&bcm_extsw_access);

    memcpy(&(pDevCtrl->EnetInfo[0]), &EnetInfo[0], sizeof(ETHERNET_MAC_INFO));
    if (unit == 1)
        memcpy(&(pDevCtrl->EnetInfo[1]), &EnetInfo[1], sizeof(ETHERNET_MAC_INFO));

    {
        char buf[BRCM_MAX_CHIP_NAME_LEN];
        printk("Broadcom BCM%s Ethernet Network Device ", kerSysGetChipName(buf, BRCM_MAX_CHIP_NAME_LEN));
        printk(VER_STR);
        printk("\n");
    }

#if defined(CONFIG_BCM963268)
    // Now select ROBO at Phy3
    BCM_ENET_DEBUG( "Select ROBO at Mux (bit18=0x40000)" ); 
    GPIO->RoboswGphyCtrl |= GPHY_MUX_SEL_GMAC;
    GPIO->RoboswGphyCtrl &= ~GPHY_MUX_SEL_GMAC;

    BCM_ENET_DEBUG( "\tGPIORoboswGphyCtrl<0x%p>=0x%x", 
        &GPIO->RoboswGphyCtrl, (uint32_t) GPIO->RoboswGphyCtrl );
#endif

#if defined(CONFIG_BCM_GMAC)
    gmac_init();
#endif

    if ((status = bcm63xx_init_dev(pDevCtrl)))
    {
        printk((KERN_ERR CARDNAME ": device initialization error!\n"));
        bcm63xx_uninit_dev(pDevCtrl);
        return -ENXIO;
    }

    dev_alloc_name(dev, dev->name);
    SET_MODULE_OWNER(dev);
    sprintf(dev->name, "bcmsw");

    dev->base_addr = -1;    /* Set the default invalid address to identify bcmsw device */
    bcmeapi_add_proc_files(dev, pDevCtrl);
    ethsw_add_proc_files(dev);
    vnet_dev[0] = dev;

    dev->netdev_ops = &bcm96xx_netdev_ops;
#if defined(SUPPORT_ETHTOOL)
    dev->ethtool_ops = &bcm63xx_enet_ethtool_ops;
#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 4, 1)
    /*
     * In Linux 3.4, we do not use softirq or NAPI.  We create a thread to
     * do the rx processing work.
     */
    pDevCtrl->rx_work_avail = 0;
    init_waitqueue_head(&pDevCtrl->rx_thread_wqh);
    pDevCtrl->rx_thread = kthread_create(bcm63xx_enet_rx_thread, pDevCtrl, "bcmsw_rx");
    wake_up_process(pDevCtrl->rx_thread);
#else
    netif_napi_add(dev, &pDevCtrl->napi, bcm63xx_enet_poll_napi, NETDEV_WEIGHT);
#endif

    netdev_path_set_hw_port(dev, 0, BLOG_ENETPHY);

    dev->watchdog_timeo     = 2 * HZ;
#if !defined(CONFIG_BCM968500) && !defined(CONFIG_BCM96838)
    /* setting this flag will cause the Linux bridge code to not forward
       broadcast packets back to other hardware ports */
    dev->priv_flags         = IFF_HW_SWITCH;
    dev->mtu = ENET_MAX_MTU_PAYLOAD_SIZE; /* bcmsw dev : Explicitly assign the MTU size based on buffer size allocated */
#endif

#if defined(_CONFIG_BCM_FAP) && defined(CONFIG_BCM_FAP_GSO)
    dev->features           = NETIF_F_SG | NETIF_F_IP_CSUM | NETIF_F_IPV6_CSUM | NETIF_F_TSO | NETIF_F_TSO6 | NETIF_F_UFO;
#endif

    /* Indicate we're supporting extended statistics */
    dev->features           |= NETIF_F_EXTSTATS;

#if defined(CONFIG_SKY_VDSL) || defined(CONFIG_SKY_ETHAN) || defined(CONFIG_SKY_EXTENDER)
    bitcount(vport_cnt, consolidated_portmap);
#else
	vport_cnt = 4;
#endif

    ethsw_init_table(pDevCtrl);
    ethsw_reset_ports(dev);

    status = register_netdev(dev);

    if (status != 0)
    {
        bcm63xx_uninit_dev(pDevCtrl);
        printk(KERN_ERR CARDNAME "bcm63xx_enet_probe failed, returns %d\n", status);
        return status;
    }

#ifdef DYING_GASP_API
	/* Set up dying gasp handler */
    kerSysRegisterDyingGaspHandler(pDevCtrl->dev->name, &ethsw_switch_power_off, dev);
#endif

#if defined(CONFIG_BCM96368) && defined(_CONFIG_BCM_PKTCMF)
    pktCmfSarPortEnable  = ethsw_enable_sar_port;
    pktCmfSarPortDisable = ethsw_disable_sar_port;
    pktCmfSwcConfig();
#endif

#if defined(CONFIG_BCM96818) && defined(_CONFIG_BCM_PKTCMF)
    pktCmfSaveSwitchPortState    = ethsw_save_port_state;
    pktCmfRestoreSwitchPortState = ethsw_restore_port_state;
#endif

    macAddr[0] = 0xff;
    kerSysGetMacAddress(macAddr, dev->ifindex);

    if((macAddr[0] & ETH_MULTICAST_BIT) == ETH_MULTICAST_BIT)
    {
        memcpy(macAddr, "\x00\x10\x18\x63\x00\x00", ETH_ALEN);
        printk((KERN_CRIT "%s: MAC address has not been initialized in NVRAM.\n"), dev->name);
    }

    memmove(dev->dev_addr, macAddr, ETH_ALEN);

    status = create_vport();

    if (status != 0)
      return status;


#ifdef ENET_EPON_CONFIG
    {
       unsigned long optical_wan_type = 0;
       BpGetOpticalWan(&optical_wan_type);
       printk("optical_wan_type = %ld, WAN_GPON=%d, WAN_EPON=%d\n", optical_wan_type, BP_OPTICAL_WAN_GPON, BP_OPTICAL_WAN_EPON);
       // TBD need think about gpon_epon?
       if (optical_wan_type & BP_OPTICAL_WAN_EPON)
       {
          /* Once boardId have EPON then create epon0 linux interface*/
          create_epon_vport(eponif_name);
       }
    }
#endif

    pVnetDev0 = (BcmEnet_devctrl *) netdev_priv(vnet_dev[0]);

    bcmeapi_ethsw_init_hw(pDevCtrl->unit, pDevCtrl->EnetInfo[0].sw.port_map, pVnetDev0->wanPort);

    ethsw_init_config();
    ethsw_phy_config();

 	/* Ethernet Switch init is complete - flush the dynamic ARL entries to start clean */
	fast_age_all(0);
	if (bcm63xx_enet_isExtSwPresent()) {
		fast_age_all_ext(0);
	}

    if( (ret = bcmeapi_init_dev(dev)) < 0)
        return ret;

#if !defined(SUPPORT_SWMDK)
	ethsw_eee_init();
#endif
    bcmeapi_map_interrupt(pDevCtrl);

    poll_pid = kernel_thread((int(*)(void *))bcm63xx_enet_poll_timer, 0, CLONE_KERNEL);
    set_bit(__LINK_STATE_START, &dev->state);
    dev->netdev_ops->ndo_open(dev);
    dev->flags |= IFF_UP;

    return ((poll_pid < 0)? -ENOMEM: 0);
}

int bcm63xx_enet_isExtSwPresent(void)
{
    return extSwInfo.present;
}

int bcm63xx_enet_intSwPortToExtSw(void)
{
    return extSwInfo.connected_to_internalPort;
}

unsigned int bcm63xx_enet_extSwId(void)
{
    return extSwInfo.switch_id;
}

static int bridge_notifier(struct notifier_block *nb, unsigned long event, void *brName);
static void bridge_update_ext_pbvlan(char *brName);
struct notifier_block br_notifier = {
    .notifier_call = bridge_notifier,
};

static int bridge_stp_handler(struct notifier_block *nb, unsigned long event, void *portInfo);
static struct notifier_block br_stp_handler = {
    .notifier_call = bridge_stp_handler,
};

static void __exit bcmenet_module_cleanup(void)
{
    BcmEnet_devctrl *pDevCtrl;
    TRACE(("bcm63xxenet: bcmenet_module_cleanup\n"));

    bcmeapi_enet_module_cleanup();

    delete_vport();
#if defined(ENET_GPON_CONFIG)
    delete_all_gpon_vports();
#endif
#if defined(ENET_EPON_CONFIG)
    delete_all_epon_vports();
#endif
    if (poll_pid >= 0)
    {
      atomic_dec(&poll_lock);
      wait_for_completion(&poll_done);
    }

    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(vnet_dev[0]);

    if (pDevCtrl)
    {
#ifdef DYING_GASP_API
        if(pDevCtrl->EnetInfo[0].ucPhyType == BP_ENET_EXTERNAL_SWITCH)
            kerSysDeregisterDyingGaspHandler(pDevCtrl->dev->name);
#endif
        bcm63xx_uninit_dev(pDevCtrl);
    }

    bcmFun_dereg(BCM_FUN_ID_ENET_LINK_CHG);
    bcmFun_dereg(BCM_FUN_ID_RESET_SWITCH);
    bcmFun_dereg(BCM_FUN_ID_ENET_CHECK_SWITCH_LOCKUP);
    bcmFun_dereg(BCM_FUN_ID_ENET_GET_PORT_BUF_USAGE);
    bcmFun_dereg(BCM_FUN_IN_ENET_CLEAR_ARL_ENTRY);	

    if (extSwInfo.present == 1)
        unregister_bridge_notifier(&br_notifier);

    unregister_bridge_stp_notifier(&br_stp_handler);
}

static int bcmeapi_ioctl_use_default_txq_config(BcmEnet_devctrl *pDevCtrl,
                                             struct ethswctl_data *e)
{
    if (e->type == TYPE_GET) {
        if (copy_to_user((void*)(&e->ret_val),
            (void*)&pDevCtrl->use_default_txq, sizeof(int))) {
            return -EFAULT;
        }
        BCM_ENET_DEBUG("e->ret_val: 0x%02x \n ", e->ret_val);
    } else {
        BCM_ENET_DEBUG("Given use_default_txq: 0x%02x \n ", e->val);
        pDevCtrl->use_default_txq = e->val;
    }

    return 0;
}

static int bcmeapi_ioctl_default_txq_config(BcmEnet_devctrl *pDevCtrl,
                                         struct ethswctl_data *e)
{
    if (e->type == TYPE_GET) {
        if (copy_to_user((void*)(&e->queue),
            (void*)&pDevCtrl->default_txq, sizeof(int))) {
            return -EFAULT;
        }
        BCM_ENET_DEBUG("e->queue: 0x%02x \n ", e->queue);
    } else {
        BCM_ENET_DEBUG("Given queue: 0x%02x \n ", e->queue);
        if ((e->queue >= NUM_EGRESS_QUEUES) || (e->queue < 0)) {
            printk("Invalid queue \n");
            return BCM_E_ERROR;
        }
        pDevCtrl->default_txq = e->queue;
    }

    return 0;
}



#ifdef BCM_ENET_DEBUG_BUILD
static int bcmeapi_ioctl_getrxcounters(void)
{
    int a = 0, b = 0, c = 0, d = 0, f = 0, cnt = 0;

    for (cnt = 0; cnt < ENET_RX_CHANNELS_MAX; cnt++) 
    {
        printk("Rx counters: %d No Rx Pkts counters: %d No Rx BDs counters: %d\n", ch_pkts[cnt],
               ch_no_pkts[cnt], ch_no_bds[cnt]);
    }

    printk("Channels: ");
    for (cnt = 0; cnt < NUM_ELEMS; cnt++) {
        if (ch_serviced[cnt] == WRR_RELOAD) {
            printk("\nCh0 = %d, Ch1 = %d, Ch2 = %d, Ch3 = %d \n", a,b,c,d);
            a = b = c =d = 0;
            printk("\nReloaded WRR weights \n");
        } else if (ch_serviced[cnt] == ISR_START) {
            printk("ISR START (Weights followed by channels serviced) \n");
            printk("x- indicates pkt received \n");
        } else {
            if (ch_serviced[cnt] & (1<<31)) {
                printk("x-");
                f = ch_serviced[cnt] & 0xF;
                if (f == 0) {
                    a++;
                } else if (f == 1) {
                    b++;
                } else if (f == 2) {
                    c++;
                } else if (f == 3) {
                    d++;
                }
            }
            printk("%d ", ch_serviced[cnt] & (~(1<<31)));
        }
    }
    printk("\n");
    return 0;
}

static int bcmeapi_ioctl_setrxcounters(void)
{
    int cnt = 0;

    for (cnt = 0; cnt < ENET_RX_CHANNELS_MAX; cnt++) 
    {
        ch_pkts[cnt] = ch_no_pkts[cnt] = ch_no_bds[cnt] = 0;
    }
    for (cnt=0; cnt<4000; cnt++) {
        ch_serviced[cnt] = 0;
    }
    dbg_index = 0;

    return 0;
}
#endif


void display_software_stats(BcmEnet_devctrl * pDevCtrl)
{
    printk("\n");
    printk("TxPkts:       %10lu \n", pDevCtrl->stats.tx_packets);
    printk("TxOctets:     %10lu \n", pDevCtrl->stats.tx_bytes);
    printk("TxDropPkts:   %10lu \n", pDevCtrl->stats.tx_dropped);
    printk("\n");
    printk("RxPkts:       %10lu \n", pDevCtrl->stats.rx_packets);
    printk("RxOctets:     %10lu \n", pDevCtrl->stats.rx_bytes);
    printk("RxDropPkts:   %10lu \n", pDevCtrl->stats.rx_dropped);
}

#define BIT_15 0x8000
#if defined(CONFIG_BCM96818)
#define MAX_NUM_WAN_IFACES 40
#else
#define MAX_NUM_WAN_IFACES 8
#endif
#define MAX_WAN_IFNAMES_LEN ((MAX_NUM_WAN_IFACES * (IFNAMSIZ + 1)) + 2)

static int bcm63xx_enet_ioctl(struct net_device *dev, struct ifreq *rq, int cmd)
{
    BcmEnet_devctrl *pDevCtrl;
    char *wanifnames;
    int *data=(int*)rq->ifr_data;
    char *chardata = (char *)rq->ifr_data;
#if defined(ENET_GPON_CONFIG)
    struct net_device *pNetDev;
    struct gponif_data g;
#endif // #if defined(ENET_GPON_CONFIG)

#if defined(CONFIG_BCM96818)
    MirrorCfg mirrorCfg;
#endif // #if defined(CONFIG_BCM96818)

    struct ethswctl_data *e=(struct ethswctl_data*)rq->ifr_data;
    struct ethctl_data *ethctl=(struct ethctl_data*)rq->ifr_data;
    struct interface_data *enetif_data=(struct interface_data*)rq->ifr_data;
    struct mii_ioctl_data *mii;
    int val = 0, mask = 0, len = 0, cum_len = 0;
    int i, vport, phy_id, atleast_one_added = 0;
    struct net_device_stats *vstats;
    struct sockaddr sockaddr;
    int swPort;
    int phyId;
    int portmap;

    pDevCtrl = netdev_priv(vnet_dev[0]);
    ASSERT(pDevCtrl != NULL);

#if defined(CONFIG_BCM_ETH_PWRSAVE)
    ethsw_phy_pll_up(0);
#endif

    switch (cmd)
    {
        case SIOCGMIIPHY:       /* Get address of MII PHY in use. */
            mii = (struct mii_ioctl_data *)&rq->ifr_data;
            swPort = port_id_from_dev(dev);
            if (swPort >= MAX_TOTAL_SWITCH_PORTS )
            {
                printk("SIOCGMIIPHY : Invalid swPort: %d \n", swPort);
                return -EINVAL;
            }
            val = 0;
            phy_id = enet_logport_to_phyid(swPort);
            mii->phy_id =  (u16)phy_id;
            /* Let us also return phy flags needed for accessing the phy */
            mii->val_out =  phy_id & CONNECTED_TO_EXTERN_SW? ETHCTL_FLAG_ACCESS_EXTSW_PHY: 0;
            mii->val_out |= IsExtPhyId(phy_id)? ETHCTL_FLAG_ACCESS_EXT_PHY: 0;
            BCM_ENET_DEBUG("%s: swPort/logport %d phy_id: 0x%x flag 0x%x \n", __FUNCTION__,
                    swPort, mii->phy_id, mii->val_out);
            break;

        case SIOCGMIIREG:       /* Read MII PHY register. */
            {
                int flags;
                mii = (struct mii_ioctl_data *)&rq->ifr_data;
                flags = mii->val_out;
                down(&bcm_ethlock_switch_config);
                ethsw_phyport_rreg2(mii->phy_id, mii->reg_num & 0x1f,
                        (uint16 *)&mii->val_out, mii->val_out);
                BCM_ENET_DEBUG("phy_id: %d; reg_num = %d  val = 0x%x\n",
                        mii->phy_id, mii->reg_num, flags);
                up(&bcm_ethlock_switch_config);
                break;
            }

        case SIOCGSWITCHPORT:       /* Get Switch Port. */
            val = -1;
            for (vport = 1; vport <= vport_cnt; vport++) {
                if ((vnet_dev[vport]) &&
                        (strcmp(enetif_data->ifname, vnet_dev[vport]->name) == 0)) {
                    val = ((BcmEnet_devctrl *)netdev_priv(vnet_dev[vport]))->sw_port_id;
                    break;
                }
            }
            if (copy_to_user((void*)&enetif_data->switch_port_id, (void*)&val, sizeof(int)))
                return -EFAULT;
            break;

        case SIOCSMIIREG:       /* Write MII PHY register. */
            {
                mii = (struct mii_ioctl_data *)&rq->ifr_data;
                down(&bcm_ethlock_switch_config);
                BCM_ENET_DEBUG("phy_id: %d; reg_num = %d; val = 0x%x \n", mii->phy_id,
                        mii->reg_num, mii->val_in);
                ethsw_phyport_wreg2(mii->phy_id, mii->reg_num & 0x1f, (uint16 *)&mii->val_in, mii->val_out);
                up(&bcm_ethlock_switch_config);
                break;
            }

        case SIOCGLINKSTATE:
            if (dev == vnet_dev[0])
            {
                mask = 0xffffffff;
            }
            else
            {                
                swPort = port_id_from_dev(dev);
                if (swPort >= MAX_TOTAL_SWITCH_PORTS )
                {
                    printk("SIOCGLINKSTATE : Invalid swPort: %d \n", swPort);
                    return -EINVAL;
                }
                mask = 0x00000001 << swPort;
            }

            val = (pDevCtrl->linkState & mask)? 1: 0;

            if (copy_to_user((void*)data, (void*)&val, sizeof(int)))
                return -EFAULT;

            val = 0;
            break;

        case SIOCSCLEARMIBCNTR:
            ASSERT(pDevCtrl != NULL);

            bcm63xx_enet_query(dev);

            memset(&pDevCtrl->stats, 0, sizeof(struct net_device_stats));
            /* port 0 is bcmsw */
            for (vport = 1; vport <= vport_cnt; vport++)
            {
                if (vnet_dev[vport])
                {
					bcmeapi_reset_mib_cnt(port_id_from_dev(vnet_dev[vport]));
                    vstats = &(((BcmEnet_devctrl *)netdev_priv(vnet_dev[vport]))->stats);
                    memset(vstats, 0, sizeof(struct net_device_stats));
                }
            }
            bcmeapi_reset_mib();
            bcmeapi_reset_mib_ext();
            val = 0;
            break;

        case SIOCMIBINFO:
            // Setup correct port indexes.
            swPort = port_id_from_dev(dev);
            if (swPort >= MAX_TOTAL_SWITCH_PORTS )
            {
                printk("SIOCMIBINFO : Invalid swPort: %d \n", swPort);
                return -EINVAL;
            }
            vport = LOGICAL_PORT_TO_VPORT(swPort);

            if (vnet_dev[vport])
            {
                IOCTL_MIB_INFO *mib;

                // Create MIB address.
                mib = &((BcmEnet_devctrl *)netdev_priv(vnet_dev[vport]))->MibInfo;

                // Copy MIB to caller.
                if (copy_to_user((void*)data, (void*)mib, sizeof(IOCTL_MIB_INFO)))
                    return -EFAULT;
            }
            else
            {
                return -EFAULT;
            }

            val = 0;
            break;

        case SIOCGQUERYNUMPORTS:
            val = 1;
            if (copy_to_user((void*)data, (void*)&val, sizeof(int))) {
                return -EFAULT;
            }
            val = 0;
            break;

#if defined(CONFIG_BCM96818)
        case SIOCPORTMIRROR:
            if(copy_from_user((void*)&mirrorCfg,data,sizeof(MirrorCfg)))
                val = -EFAULT;
            else
            {
                if( mirrorCfg.nDirection == MIRROR_DIR_IN )
                {
                    memcpy(&gemMirrorCfg[0], &mirrorCfg, sizeof(MirrorCfg));
                }
                else /* MIRROR_DIR_OUT */
                {
                    memcpy(&gemMirrorCfg[1], &mirrorCfg, sizeof(MirrorCfg));
                }
            }
            break;
#endif
        case SIOCSWANPORT:
            if (dev == vnet_dev[0])
                return -EFAULT;

            swPort = port_id_from_dev(dev);
            if (swPort >= MAX_TOTAL_SWITCH_PORTS)
            {
                printk("SIOCSWANPORT : Invalid swPort: %d \n", swPort);
                return -EINVAL;
            }

            phyId  = enet_logport_to_phyid(swPort);

            if (phyId >= 0) {
                if(IsWanPort(phyId)) {
                    if ((int)data) {
#if defined(CONFIG_BCM_GMAC)
                        if ( IsGmacPort(swPort) )
                        {
                            gmac_set_wan_port( 1 );
                        }
#endif
                        return 0;
                    }
                    else
                    {
                        BCM_ENET_DEBUG("This port cannot be removed "
                                "from WAN port map");
                        return -EFAULT;
                    }
                }
            }

            if ( (int)data ) {
                BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(vnet_dev[0]);

                pDevCtrl->wanPort |= (1 << swPort);
                
				dev->priv_flags |= IFF_WANDEV;
#ifdef CONFIG_SKY_IPV6
				BCM_ENET_DEBUG("Setting up SKY_WANOE FLAG");
				dev->priv_flags |= IFF_SKY_WANOE;
#endif

#if !defined(CONFIG_BCM968500) && !defined(CONFIG_BCM96838)
                dev->priv_flags &= ~IFF_HW_SWITCH;
#endif
#if defined(CONFIG_BCM_GMAC)
                if ( IsGmacPort(swPort) )
                {
                    int vport = LOGICAL_PORT_TO_VPORT(swPort);

                    if (vnet_dev[vport])
                    {
                        IOCTL_MIB_INFO *mib;

                        mib = &((BcmEnet_devctrl *)netdev_priv(vnet_dev[vport]))->MibInfo;

                        ethsw_eee_port_enable(swPort, 0, 0);
                        gmac_set_wan_port( 1 );
                        gmac_link_status_changed(GMAC_PORT_ID, 
                            (priv->linkState & (1<<swPort)? 1 : 0),
                            mib->ulIfSpeed/1000000, /* speed in Mbps */
                            mib->ulIfDuplex);
                        ethsw_eee_port_enable(swPort, 1, 0);
                    }
                }
#endif
                if (priv->linkState & (1<<swPort))
                {
                    bcmeapi_EthSetPhyRate(swPort, 1, ((BcmEnet_devctrl *)netdev_priv(dev))->MibInfo.ulIfSpeed, 1);
                }
#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
                val = kerSysGetMacAddress(dev->dev_addr, dev->ifindex);
                if (val == 0) {
                    memmove(sockaddr.sa_data, dev->dev_addr, ETH_ALEN);
                    bcmenet_set_dev_mac_addr(dev, &sockaddr);
                }
#endif

#if defined(_CONFIG_BCM_FAP) && \
    (defined(CONFIG_BCM_PKTDMA_RX_SPLITTING) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828) || defined(CONFIG_BCM96818))
                if ( !IsExternalSwitchPort(swPort))
                {
                    struct ethswctl_data e2;
                    int i, j;

                    /* The equivalent of "ethswctl -c cosq -p {i} -q {j} -v 1" */
                    /* where i = all eth ports (0..5) except the WAN port (swPort) */
                    /* This routes packets of all priorities on the WAN eth port to egress queue 0 */
                    /* This routes packets of all priorities on all other eth ports to egress queue 1 */
                    for(i = 0; i < BP_MAX_SWITCH_PORTS; i++)
                    {
                        for(j = 0; j <= MAX_PRIORITY_VALUE; j++)
                        {
                            e2.type = TYPE_SET;
                            e2.port = i;
                            e2.priority = j;

                            if ((LOGICAL_PORT_TO_PHYSICAL_PORT(swPort) == i)
#if defined(CONFIG_BCM96818)
                            || (i == GPON_PORT_ID)
#endif
                            )
                            {
                                e2.queue = PKTDMA_ETH_DS_IUDMA;  /* WAN port mapped to DS FAP */
                            }
                            else
                            {
                                e2.queue = PKTDMA_ETH_US_IUDMA;  /* other ports to US FAP */
                            }

                            bcmeapi_ioctl_ethsw_cosq_port_mapping(&e2);
                        }
                    }
                }
#endif  /* if defined(CONFIG_BCM_PKTDMA_RX_SPLITTING) */
            } else {
                BcmEnet_devctrl *priv = (BcmEnet_devctrl *)netdev_priv(vnet_dev[0]);
#if defined(CONFIG_BCM_GMAC)
                if (IsGmacPort( swPort ) && IsLogPortWan(swPort) )
                {
                    int vport = LOGICAL_PORT_TO_VPORT(swPort);

                    if (vnet_dev[vport])
                    {
                        IOCTL_MIB_INFO *mib;

                        mib = &((BcmEnet_devctrl *)netdev_priv(vnet_dev[vport]))->MibInfo;

                        gmac_link_status_changed(GMAC_PORT_ID, 
                            (priv->linkState & (1<<swPort)? 1 : 0),
                            0, 0 );
                        gmac_set_wan_port( 0 );
                    }
                }
#endif
                if (priv->linkState & (1<<swPort))
                {
                    bcmeapi_EthSetPhyRate(swPort, 1, ((BcmEnet_devctrl *)netdev_priv(dev))->MibInfo.ulIfSpeed, 0);
                }

                pDevCtrl->wanPort &= ~(1 << swPort);
                dev->priv_flags &= (~IFF_WANDEV);

#ifdef CONFIG_SKY_IPV6
				BCM_ENET_DEBUG("Clearing up SKY_WANOE FLAG");
                dev->priv_flags &= (~IFF_SKY_WANOE);
#endif

#if !defined( CONFIG_BCM968500) && !defined(CONFIG_BCM96838)
                dev->priv_flags |= IFF_HW_SWITCH;
#endif

#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
                kerSysReleaseMacAddress(dev->dev_addr);
                memmove(sockaddr.sa_data, vnet_dev[0]->dev_addr, ETH_ALEN);
                bcmenet_set_dev_mac_addr(dev, &sockaddr);
#endif

#if defined(_CONFIG_BCM_FAP) && \
    (defined(CONFIG_BCM_PKTDMA_RX_SPLITTING) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96818))
                {
                    struct ethswctl_data e2;
                    int i, j;

                    /* Return all ethernet ports to be processed on the FAP - Nov 2010 (Jira 7811) */

                    /* The equivalent of "ethswctl -c cosq -p {i} -q {j} -v 0" */
                    /* where i = all eth ports (0..5) including the WAN port (swPort) */
                    for(i = 0; i < BP_MAX_SWITCH_PORTS ; i++)
                    {
                        for(j = 0; j <= MAX_PRIORITY_VALUE; j++)
                        {
                            e2.type = TYPE_SET;
                            e2.port = i;
                            e2.priority = j;
                            /* All ports mapped to default iuDMA - Mar 2011 */
                            /* US iuDMA for 63268/6828 and DS iuDMA (ie FAP owned) for 6362 */
#if defined(CONFIG_BCM96828) && !defined(CONFIG_EPON_HGU)
                            /* Revert to initial config when a WAN port is deleted */
                            e2.queue = restoreEthPortToRxIudmaConfig(e2.port);
#else
                            e2.queue = PKTDMA_DEFAULT_IUDMA;
#if defined(CONFIG_BCM96818)
                            if (i == GPON_PORT_ID)
                            {
                                e2.queue = PKTDMA_ETH_DS_IUDMA;
                            }
#endif
#endif
                            bcmeapi_ioctl_ethsw_cosq_port_mapping(&e2);
                        }
                    }
                }
#endif  /* if defined(CONFIG_BCM_PKTDMA_RX_SPLITTING) */
            }


#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828) || defined(CONFIG_BCM96318)
            if (IsExternalSwitchPort(swPort))
            {
                extsw_set_wanoe_portmap(GET_PORTMAP_FROM_LOGICAL_PORTMAP(pDevCtrl->wanPort,1));
            }
            else
            {
                ethsw_set_wanoe_portmap(GET_PORTMAP_FROM_LOGICAL_PORTMAP(pDevCtrl->wanPort,0), pDevCtrl->txSoftSwitchingMap);
            }
#else
            if (IsExternalSwitchPort(swPort))
            {
                extsw_set_wanoe_portmap(GET_PORTMAP_FROM_LOGICAL_PORTMAP(pDevCtrl->wanPort,1));
            }
            else
            {
                ethsw_port_based_vlan(pDevCtrl->EnetInfo[0].sw.port_map,
                                      pDevCtrl->wanPort,
                                      pDevCtrl->txSoftSwitchingMap);
            }
#endif
            TRACE(("Set %s wan port %d", dev->name, (int)data));
            BCM_ENET_DEBUG("bcmenet:SIOCSWANPORT Set %s wan port %s, wan map 0x%x dev flags 0x%x \n",
                           dev->name, data?"ENB": "DISB",
                           (unsigned int)pDevCtrl->wanPort, (unsigned int)dev->priv_flags);
            val = 0;
            break;

        case SIOCGWANPORT:
            val = 0;
            if (e->up_len.uptr == NULL) {
                val = -EFAULT;
                break;
            }
            wanifnames = kmalloc(MAX_WAN_IFNAMES_LEN, GFP_KERNEL);

            if( wanifnames == NULL )
            {
                printk(KERN_ERR "bcmenet:SIOCGWANPORT: kmalloc of %d bytes failed\n", MAX_WAN_IFNAMES_LEN);
                return -ENOMEM;
            }

            BCM_ENET_DEBUG("pDevCtrl->wanPort = 0x%x \n", pDevCtrl->wanPort);

            /* exclude LAN ports that are configured for RX and TX through MIPS */
            portmap = pDevCtrl->wanPort & ~pDevCtrl->softSwitchingMap;
            for (i = 0; i < MAX_TOTAL_SWITCH_PORTS; i++)
            {
                if ((portmap >> i) & 0x1) // wanPort mask is by logical port.
                {
                    if (LOGICAL_PORT_TO_VPORT(i) > 0)
                    {
                        struct net_device *dev = vnet_dev[LOGICAL_PORT_TO_VPORT(i)];
                        BCM_ENET_DEBUG("GET WANPORT: %s priv_flags = 0x%x \n",
                                   dev->name, (unsigned int)dev->priv_flags);                   

                        len = strlen((vnet_dev[LOGICAL_PORT_TO_VPORT(i)])->name);
                        if ((cum_len + len + 1) < MAX_WAN_IFNAMES_LEN)
                        {
                            if (atleast_one_added)
                            {
                                wanifnames[cum_len] = ',';
                                cum_len += 1;
                            }
                            memcpy(wanifnames+cum_len, (vnet_dev[LOGICAL_PORT_TO_VPORT(i)])->name, len);
                            cum_len += len;
                            atleast_one_added = 1;
                        }
                    }
                }
            }
#if defined(ENET_GPON_CONFIG)
            for (i = 0; i < MAX_GPON_IFS; i++) {
                pNetDev = gponifid_to_dev[i];
                if (pNetDev != NULL) {
                    len = strlen(pNetDev->name);
                    if ((cum_len + len + 1) < MAX_WAN_IFNAMES_LEN) {
                        if (atleast_one_added) {
                            wanifnames[cum_len] = ',';
                            cum_len += 1;
                        }
                        memcpy(wanifnames+cum_len, pNetDev->name, len);
                        cum_len += len;
                        atleast_one_added = 1;
                    }
                }
            }
#endif
            cum_len = (cum_len + 1) <= e->up_len.len? cum_len: len - 1;
            wanifnames[cum_len] = '\0';
            cum_len += 1;
            BCM_ENET_DEBUG("cum_len = %d \n", cum_len);
            if (copy_to_user((void*)e->up_len.uptr, (void*)wanifnames, cum_len)) {
                val = -EFAULT;
            }
            BCM_ENET_DEBUG("WAN interfaces: %s", chardata);
            kfree(wanifnames);
            break;

        case SIOCGGMACPORT:
            val = 0;
            wanifnames = kmalloc(MAX_WAN_IFNAMES_LEN, GFP_KERNEL);
            if( wanifnames == NULL ) {
                printk(KERN_ERR "bcmenet:SIOCGGMACPORT: kmalloc of %d bytes failed\n", MAX_WAN_IFNAMES_LEN);
                return -ENOMEM;
            }
#if defined(CONFIG_BCM_GMAC)
            BCM_ENET_DEBUG("pDevCtrl->gmacPort = 0x%x\n", pDevCtrl->gmacPort);
            for (i = 0; i < MAX_SWITCH_PORTS*2-1; i++) {
                if ((pDevCtrl->gmacPort >> i) & 0x1) {
                    if (LOGICAL_PORT_TO_VPORT(i) > 0) {
                        len = strlen((vnet_dev[LOGICAL_PORT_TO_VPORT(i)])->name);
                        if (atleast_one_added) {
                            wanifnames[cum_len] = ',';
                            cum_len += 1;
                        }
                        memcpy(wanifnames+cum_len, (vnet_dev[LOGICAL_PORT_TO_VPORT(i)])->name, len);
                        cum_len += len;
                        atleast_one_added = 1;
                    }
                }
            }
#endif
            wanifnames[cum_len] = '\0';
            cum_len += 1;
            if (copy_to_user((void*)chardata, (void*)wanifnames, cum_len))
            	val = -EFAULT;
            BCM_ENET_DEBUG("GMAC interfaces: %s\n", chardata);
            kfree(wanifnames);
            break;

#if defined(ENET_GPON_CONFIG)
        case SIOCGPONIF:
            if (copy_from_user((void*)&g, rq->ifr_data, sizeof(struct gponif_data)))
            {
                val = -EFAULT;
                break;
            }
            BCM_ENET_DEBUG("The op is %d (KULI!!!) \n", g.op);
            dumpGemIdxMap(g.gem_map_arr); BCM_ENET_DEBUG("The ifnum is %d \n", g.ifnumber);
            BCM_ENET_DEBUG("The ifname is %s \n", g.ifname);
            switch (g.op) {
                /* Add, Remove, and Show gem_ids */
                case GETFREEGEMIDMAP:
                case SETGEMIDMAP:
                case GETGEMIDMAP:
                val = set_get_gem_map(g.op, g.ifname, g.ifnumber,
                                      g.gem_map_arr);
 
                if (val == 0)
                {
                    if (copy_to_user(rq->ifr_data, (void*)&g, sizeof(struct gponif_data)))
            	        val = -EFAULT;
                }
                break;

                /* Create a gpon virtual interface */
                case CREATEGPONVPORT:
                val = create_gpon_vport(g.ifname);
                break;

                /* Delete the given gpon virtual interface */
                case DELETEGPONVPORT:
                val = delete_gpon_vport(g.ifname);
                break;

                /* Delete all gpon virtual interfaces */
                case DELETEALLGPONVPORTS:
                val = delete_all_gpon_vports();
                break;

                /* Set multicast gem index */
                case SETMCASTGEMID:
                val = set_mcast_gem_id(g.gem_map_arr);
                break;

                default:
                val = -EOPNOTSUPP;
                break;
            }
            break;
#endif

       case SIOCETHSWCTLOPS:

            switch(e->op) {
                case ETHSWDUMPPAGE:
                    BCM_ENET_DEBUG("ethswctl ETHSWDUMPPAGE ioctl");
                    bcmeapi_ethsw_dump_page(e->page);
                    val = 0;
                    break;

                /* Print out enet iuDMA info - Aug 2010 */
                case ETHSWDUMPIUDMA:
                    bcmeapi_dump_queue(e, pDevCtrl);
                    break;

                /* Get/Set the iuDMA rx channel for a specific eth port - Jan 2011 */
                case ETHSWIUDMASPLIT:
                    bcmeapi_config_queue(e);
                    break;

                case ETHSWDUMPMIB:
                    BCM_ENET_DEBUG("ethswctl ETHSWDUMPMIB ioctl");
                    if (e->unit) {
                       val = bcmsw_dump_mib_ext(e->port, e->type);
                    }
                    else
                    {
                       val = bcmeapi_ethsw_dump_mib(e->port, e->type);
                    }
                    break;

                case ETHSWSWITCHING:
                    BCM_ENET_DEBUG("ethswctl ETHSWSWITCHING ioctl");
                    if (e->type == TYPE_ENABLE) {
                        val = ethsw_set_hw_switching(HW_SWITCHING_ENABLED);
                    } else if (e->type == TYPE_DISABLE) {
                        val = ethsw_set_hw_switching(HW_SWITCHING_DISABLED);
                    } else {
                        val = ethsw_get_hw_switching_state();
                        if (copy_to_user((void*)(&e->status), (void*)&val,
                            sizeof(int))) {
                            return -EFAULT;
                        }
                        val = 0;
                    }
                    break;

                case ETHSWRXSCHEDULING:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXSCHEDULING ioctl");
                    return bcmeapi_ioctl_ethsw_rxscheduling(e);
                    break;

                case ETHSWWRRPARAM:
                    BCM_ENET_DEBUG("ethswctl ETHSWWRRPARAM ioctl");
                    return bcmeapi_ioctl_ethsw_wrrparam(e);
                    break;

                case ETHSWUSEDEFTXQ:
                    BCM_ENET_DEBUG("ethswctl ETHSWUSEDEFTXQ ioctl");
                    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
                    return bcmeapi_ioctl_use_default_txq_config(pDevCtrl,e);
                    break;

                case ETHSWDEFTXQ:
                    BCM_ENET_DEBUG("ethswctl ETHSWDEFTXQ ioctl");
                    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
                    return bcmeapi_ioctl_default_txq_config(pDevCtrl, e);
                    break;

#if defined(RXCHANNEL_BYTE_RATE_LIMIT)
                case ETHSWRXRATECFG:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXRATECFG ioctl");
                    return bcmeapi_ioctl_rx_rate_config(e);
                    break;

                case ETHSWRXRATELIMITCFG:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXRATELIMITCFG ioctl");
                    return bcmeapi_ioctl_rx_rate_limit_config(e);
                    break;
#endif /* defined(RXCHANNEL_BYTE_RATE_LIMIT) */

#if defined(RXCHANNEL_PKT_RATE_LIMIT)
                case ETHSWRXPKTRATECFG:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXRATECFG ioctl");
                    return bcmeapi_ioctl_rx_pkt_rate_config(e);
                    break;

                case ETHSWRXPKTRATELIMITCFG:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXRATELIMITCFG ioctl");
                    return bcmeapi_ioctl_rx_pkt_rate_limit_config(e);
                    break;
#endif

                case ETHSWTEST1:
                    BCM_ENET_DEBUG("ethswctl ETHSWTEST1 ioctl");
                    return bcmeapi_ioctl_test_config(e);
                    break;

#if defined(CONFIG_BCM96818)
                case ETHSWPORTTAGREPLACE:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTTAGREPLACE ioctl");
                    return bcmeapi_ioctl_ethsw_port_tagreplace(e);
                    break;

                case ETHSWPORTTAGMANGLE:
                    BCM_ENET_DEBUG("ethswctl ETHSWPPORTTAGMANGLE ioctl");
                    return bcmeapi_ioctl_ethsw_port_tagmangle(e);
                    break;

                case ETHSWPORTTAGMANGLEMATCHVID:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTTAGMANGLEMATCHVID ioctl");
                    return bcmeapi_ioctl_ethsw_port_tagmangle_matchvid(e);
                    break;

                case ETHSWPORTTAGSTRIP:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTTAGSTRIP ioctl");
                    return bcmeapi_ioctl_ethsw_port_tagstrip(e);
                    break;
#endif

                case ETHSWPORTPAUSECAPABILITY:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTPAUSECAPABILITY ioctl");
                    return bcmeapi_ioctl_ethsw_port_pause_capability(e);
                    break;

                case ETHSWCONTROL:
                    BCM_ENET_DEBUG("ethswctl ETHSWCONTROL ioctl");
                    return bcmeapi_ioctl_ethsw_control(e);
                    break;

                case ETHSWPRIOCONTROL:
                    BCM_ENET_DEBUG("ethswctl ETHSWPRIOCONTROL ioctl");
                    return bcmeapi_ioctl_ethsw_prio_control(e);
                    break;

                case ETHSWVLAN:
                    BCM_ENET_DEBUG("ethswctl ETHSWVLAN ioctl");
#ifdef CONFIG_BCM96838
                    if (e->type == TYPE_SET) {
                        return bcmeapi_ioctl_ethsw_port_vlanlist_set(e);
                    }
#else                    
                    return bcmeapi_ioctl_ethsw_vlan(e);
#endif                    
                    break;
#ifdef BCM_ENET_DEBUG_BUILD
                case ETHSWGETRXCOUNTERS:
                    BCM_ENET_DEBUG("ethswctl ETHSWGETRXCOUNTERS ioctl");
                    return bcmeapi_ioctl_getrxcounters();
                    break;

                case ETHSWRESETRXCOUNTERS:
                    BCM_ENET_DEBUG("ethswctl ETHSWRESETRXCOUNTERS ioctl");
                    return bcmeapi_ioctl_setrxcounters();
                    break;
#endif

                case ETHSWPBVLAN:
                    BCM_ENET_DEBUG("ethswctl ETHSWPBVLAN ioctl");
                    return bcmeapi_ioctl_ethsw_pbvlan(e);
                    break;

                case ETHSWCOSCONF:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSCONF ioctl");
                    return bcmeapi_ioctl_ethsw_cosq_config(e);
                    break;

                case ETHSWCOSSCHED:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSSCHED ioctl");
                    return bcmeapi_ioctl_ethsw_cosq_sched(e);
                    break;

                case ETHSWCOSPORTMAP:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSMAP ioctl");
                    val = bcmeapi_ioctl_ethsw_cosq_port_mapping(e);
                    if(val < 0)
                    {
                        if(-BCM_E_ERROR == val)
                            return BCM_E_ERROR;
                        return(val);
                    }

                    if(e->type == TYPE_GET)
                    {
                        /* queue returned from function. Return value to user */
                        if (copy_to_user((void*)(&e->queue), (void*)&val, sizeof(int)))
                        {
                            return -EFAULT;
                        }
                    }
                    break;

#if !defined(CONFIG_BCM96368)
                case ETHSWCOSRXCHMAP:
                    BCM_ENET_DEBUG("ethswctl ETHSWRXCOSCHMAP ioctl");
                    return bcmeapi_ioctl_ethsw_cosq_rxchannel_mapping(e);
                    break;

                case ETHSWCOSTXCHMAP:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSTXCHMAP ioctl");
                    return bcmeapi_ioctl_ethsw_cosq_txchannel_mapping(e);
                    break;
#endif

                case ETHSWCOSTXQSEL:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSTXQSEL ioctl");
                    return bcmeapi_ioctl_ethsw_cosq_txq_sel(e);
                    break;

                case ETHSWSTATCLR:
                    BCM_ENET_DEBUG("ethswctl ETHSWSTATINIT ioctl");
                    return bcmeapi_ioctl_ethsw_clear_stats
                        ((uint32_t)pDevCtrl->EnetInfo[0].sw.port_map);
                    break;

                case ETHSWSTATPORTCLR:
                    BCM_ENET_DEBUG("ethswctl ETHSWSTATCLEAR ioctl");
                    return bcmeapi_ioctl_ethsw_clear_port_stats(e);
                    break;

                case ETHSWSTATSYNC:
                    BCM_ENET_DEBUG("ethswctl ETHSWSTATSYNC ioctl");
                    return ethsw_counter_collect
                        ((uint32_t)pDevCtrl->EnetInfo[0].sw.port_map, 0);
                    break;

                case ETHSWSTATGET:
                    //BCM_ENET_DEBUG("ethswctl ETHSWSTATGET ioctl");
                    return bcmeapi_ioctl_ethsw_counter_get(e);
                    break;
                
                case ETHSWEMACGET:
                    return bcmeapi_ioctl_ethsw_get_port_emac(e);
                    break;

                case ETHSWEMACCLEAR:
                    return bcmeapi_ioctl_ethsw_clear_port_emac(e);
                    break;

                case ETHSWPORTRXRATE:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTRXRATE ioctl");
                    if (e->type == TYPE_GET) {
                        return bcmeapi_ioctl_ethsw_port_irc_get(e);
                    } else {
                        return bcmeapi_ioctl_ethsw_port_irc_set(e);
                    }
                    break;

                case ETHSWPORTTXRATE:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTTXRATE ioctl");
                    if (e->type == TYPE_GET) {
                        return bcmeapi_ioctl_ethsw_port_erc_get(e);
                    } else {
                        return bcmeapi_ioctl_ethsw_port_erc_set(e);
                    }
                    break;
#if defined(CONFIG_BCM968500) || defined(CONFIG_BCM96838)
    case ETHSWPORTSALDAL:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTSALDAL ioctl");
                    if (e->type == TYPE_SET) {
                        return bcmeapi_ioctl_ethsw_sal_dal_set(e);
                    }
                    break;
   case ETHSWPORTMTU:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTMTU ioctl");
                    if (e->type == TYPE_SET) {
                        return bcmeapi_ioctl_ethsw_mtu_set(e);
                    }
                    break;
#endif                    
#if defined(CONFIG_BCM96818)
                case ETHSWPKTPAD:
                    BCM_ENET_DEBUG("ethswctl ETHSWPKTPAD ioctl");
                    return bcmeapi_ioctl_ethsw_pkt_padding(e);
                    break;
#endif /*defined(CONFIG_BCM96818)*/

                case ETHSWJUMBO:
                    BCM_ENET_DEBUG("ethswctl ETHSWJUMBO ioctl");
                    if (e->unit == 0) {
                    return bcmeapi_ioctl_ethsw_port_jumbo_control(e);
                    } else {
                        return bcmeapi_ioctl_extsw_port_jumbo_control(e);
                    }
                    break;

                case ETHSWPORTTRAFFICCTRL:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTTRAFFICCTRL ioctl");
#ifdef CONFIG_BCM96838                    
                    phy_id = pDevCtrl->EnetInfo[0].sw.phy_id[e->port];
                    return bcmeapi_ioctl_ethsw_port_traffic_control(e, phy_id);
#else
					return bcmeapi_ioctl_ethsw_port_traffic_control(e);
#endif
                    break;

                case ETHSWPORTLOOPBACK:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTLOOPBACK ioctl");
                    phy_id = pDevCtrl->EnetInfo[0].sw.phy_id[e->port];
                    return bcmeapi_ioctl_ethsw_port_loopback(e, phy_id);
                    break;

                case ETHSWARLACCESS:
                    BCM_ENET_DEBUG("ethswctl ETHSWARLACCESS ioctl");
                    return bcmeapi_ioctl_ethsw_arl_access(e);
                    break;

                case ETHSWPORTDEFTAG:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTDEFTAG ioctl");
                    return bcmeapi_ioctl_ethsw_port_default_tag_config(e);
                    break;

                case ETHSWCOSPRIORITYMETHOD:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSPRIORITYMETHOD ioctl");
                    return bcmeapi_ioctl_ethsw_cos_priority_method_config(e);
                    break;

                case ETHSWCOSDSCPPRIOMAP:
                    BCM_ENET_DEBUG("ethswctl ETHSWCOSDSCPPRIOMAP ioctl");
                    return bcmeapi_ethsw_dscp_to_priority_mapping(e);
                    break;

                case ETHSWREGACCESS:
                    val = enet_ioctl_ethsw_regaccess(e);
                    break;

                case ETHSWSPIACCESS:
                    //BCM_ENET_DEBUG("ethswctl ETHSWSPIACCESS ioctl");
                    val = bcmeapi_ioctl_ethsw_spiaccess(global.pVnetDev0_g->extSwitch->bus_num, 
                        global.pVnetDev0_g->extSwitch->spi_ss, global.pVnetDev0_g->extSwitch->spi_cid, e);
                    break;

                case ETHSWPSEUDOMDIOACCESS:
                    //BCM_ENET_DEBUG("ethswctl ETHSWPSEUDOMDIOACCESS ioctl");
                    val = enet_ioctl_ethsw_pmdioaccess(dev, e);
                    break;

                case ETHSWINFO:
                    BCM_ENET_DEBUG("ethswctl ETHSWINFO ioctl");
                    val = enet_ioctl_ethsw_info(dev, e);
                    break;

                case ETHSWLINKSTATUS:
                    BCM_ENET_DEBUG("ethswctl ETHSWLINKSTATUS ioctl");
#ifdef CONFIG_BCM96838
                    phy_id = pDevCtrl->EnetInfo[0].sw.phy_id[e->port];
					if (e->type == TYPE_GET)
						val = bcmeapi_ioctl_ethsw_link_status(e, phy_id);
					else
					{
#endif
                    swPort = PHYSICAL_PORT_TO_LOGICAL_PORT(e->port, e->unit);

#if defined(CONFIG_BCM_GMAC)
                    if (IsGmacPort( swPort ) && IsLogPortWan(swPort) )
                    {
                        gmac_link_status_changed(GMAC_PORT_ID, e->status, 
                            e->speed, e->duplex);
                    }
#endif
                    link_change_handler(swPort, e->status, e->speed, e->duplex);
                    val = 0;
#ifdef CONFIG_BCM96838
					}
#endif
                    break;

#if defined(SUPPORT_SWMDK)
                case ETHSWKERNELPOLL:
                    val = bcmeapi_ioctl_kernel_poll(vnet_dev[0], e);
                    break;
#endif
                case ETHSWPHYCFG:
                    BCM_ENET_DEBUG("ethswctl ETHSWPHYCFG ioctl");
                    val = bcmeapi_ioctl_phy_cfg_get(dev, e);
                    break;

                case ETHSWPHYMODE:
                    BCM_ENET_DEBUG("ethswctl ETHSWPHYMODE ioctl");
                    phy_id = pDevCtrl->EnetInfo[e->unit].sw.phy_id[e->port];
                    val = bcmeapi_ioctl_ethsw_phy_mode(e, phy_id);
                    return val;
                    break;


                case ETHSWGETIFNAME:
                    BCM_ENET_DEBUG("ethswctl ETHSWPHYMODE ioctl");
                    if ((LOGICAL_PORT_TO_VPORT(e->port) != -1) && 
                        (vnet_dev[LOGICAL_PORT_TO_VPORT(e->port)] != NULL)) {
                        char *ifname = vnet_dev[LOGICAL_PORT_TO_VPORT(e->port)]->name;
                        unsigned int len = sizeof(vnet_dev[LOGICAL_PORT_TO_VPORT(e->port)]->name);
                        if (copy_to_user((void*)&e->ifname, (void*)ifname, len)) {
                            return -EFAULT;
                        }
                    } else {
                        /* Return error as there is no interface for the given port */
                        return -EFAULT;
                    }
                    return 0;
                    break;

                case ETHSWMULTIPORT:
                    BCM_ENET_DEBUG("ethswctl ETHSWMULTIPORT ioctl");
                    val = bcmeapi_ioctl_set_multiport_address(e);
                    return val;
                    break;

                case ETHSWDOSCTRL:
                    BCM_ENET_DEBUG("ethswctl ETHSWDOSCTRL ioctl");
                    val = enet_ioctl_ethsw_dos_ctrl(e);
                    return val;
                    break;

                case ETHSWDEBUG:
                    bcmeapi_ioctl_debug_conf(e);
                    break;

                case ETHSWUNITPORT:
                    if (bcm63xx_enet_getPortFromName(e->ifname, &e->unit, &e->port)){
                        val = -EFAULT;
                    }
                    return val;
                    break;

                case ETHSWTXSOFTSWITCHING:
                    if ( e->unit > 0 ) {
                        return -EINVAL;
                    }
                    
                    swPort = PHYSICAL_PORT_TO_LOGICAL_PORT(e->port, e->unit);
                    if (swPort >= MAX_TOTAL_SWITCH_PORTS) {
                        return -EINVAL;
                    }

                    if ( e->type == TYPE_GET ) {
                        if (copy_to_user((void*)(&e->status), (void*)&pDevCtrl->txSoftSwitchingMap, sizeof(int))) {
                            return -EFAULT;
                        }
                    }
                    else {
                        vport = LOGICAL_PORT_TO_VPORT(swPort);
                        if (e->type == TYPE_ENABLE) {
                            pDevCtrl->txSoftSwitchingMap  |= (1 << swPort);
                            vnet_dev[vport]->priv_flags &= ~IFF_HW_SWITCH;    
                        }
                        else if (e->type == TYPE_DISABLE) {
                            pDevCtrl->txSoftSwitchingMap &= ~(1 << swPort);
                            vnet_dev[vport]->priv_flags |= IFF_HW_SWITCH;
                        }
                        else {
                            return -EINVAL;            
                        }
 
 #if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828) || defined(CONFIG_BCM96318)
                        ethsw_port_based_vlan(pDevCtrl->EnetInfo[e->unit].sw.port_map, 
                                              0, pDevCtrl->txSoftSwitchingMap);
 #else
                        ethsw_port_based_vlan(pDevCtrl->EnetInfo[e->unit].sw.port_map,
                                              pDevCtrl->wanPort, pDevCtrl->txSoftSwitchingMap);
 #endif
                    }
                    break;

                case ETHSWSOFTSWITCHING:
                    swPort = PHYSICAL_PORT_TO_LOGICAL_PORT(e->port, e->unit);
                    if (swPort >= MAX_TOTAL_SWITCH_PORTS) {
                        return -EINVAL;
                    }

                    if ( e->type == TYPE_GET ) {
                        if (copy_to_user((void*)(&e->status), (void*)&pDevCtrl->softSwitchingMap, sizeof(int))) {
                            return -EFAULT;
                        }
                    }
                    else {
                        val = bcm_set_soft_switching(swPort, e->type);
                    }
                    break;

                case ETHSWHWSTP:
                    swPort = PHYSICAL_PORT_TO_LOGICAL_PORT(e->port, e->unit);
                    if (swPort >= MAX_TOTAL_SWITCH_PORTS) {
                        return -EINVAL;
                    }
                    
                    if ( e->type == TYPE_GET ) {
                        if (copy_to_user((void*)(&e->status), (void*)&pDevCtrl->stpDisabledPortMap, sizeof(int))) {
                            return -EFAULT;
                        }
                    }
                    else {
                        vport = LOGICAL_PORT_TO_VPORT(swPort);
                        if (e->type == TYPE_ENABLE) {
                            pDevCtrl->stpDisabledPortMap &= ~(1 << swPort);
                            if ( vnet_dev[vport]->flags & IFF_UP) {
                                bcmeapi_ethsw_set_stp_mode(e->unit, e->port, REG_PORT_STP_STATE_BLOCKING);
                                dev_change_flags(vnet_dev[vport], (vnet_dev[vport]->flags & ~IFF_UP));
                                dev_change_flags(vnet_dev[vport], (vnet_dev[vport]->flags | IFF_UP));
                            }
                            else {
                                bcmeapi_ethsw_set_stp_mode(e->unit, e->port, REG_PORT_STP_STATE_DISABLED);
                            }
                        } 
                        else if (e->type == TYPE_DISABLE) {
                           pDevCtrl->stpDisabledPortMap |= (1 << swPort);
                           bcmeapi_ethsw_set_stp_mode(e->unit, e->port, REG_PORT_NO_SPANNING_TREE);                       
                        } 
                        else {
                            return -EINVAL;
                        }
                    }
                    break;

#ifdef CONFIG_BCM96838
                case ETHSWPHYAUTONEG:
                    BCM_ENET_DEBUG("ethswctl ETHSWPHYAUTONEG ioctl");
                    phy_id = pDevCtrl->EnetInfo[e->unit].sw.phy_id[e->port];
                    val = bcmeapi_ioctl_ethsw_phy_autoneg_info(e, phy_id);
                    return val;
                break;
                case ETHSWPORTTRANSPARENT:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTTRANSPARENT ioctl");
                    if (e->type == TYPE_SET) {
                        return bcmeapi_ioctl_ethsw_port_transparent_set(e);
                    }
                break;
                case ETHSWPORTVLANISOLATION:
                    BCM_ENET_DEBUG("ethswctl ETHSWPORTVLANISOLATION ioctl");
                    if (e->type == TYPE_SET) {
                        return bcmeapi_ioctl_ethsw_port_vlan_isolation_set(e);
                    }
                break;
#endif					

                default:
                    BCM_ENET_DEBUG("ethswctl unsupported ioctl");
                    val = -EOPNOTSUPP;
                    break;
            }
            break;

        case SIOCETHCTLOPS:
            switch(ethctl->op) {
                case ETHGETNUMTXDMACHANNELS:
                    val = bcmeapi_get_num_txques(ethctl);
                    break;

                case ETHSETNUMTXDMACHANNELS:
                    val = bcmeapi_set_num_txques(ethctl);
                    break;

                case ETHGETNUMRXDMACHANNELS:
                    val = bcmeapi_get_num_rxques(ethctl);
                    break;

                case ETHSETNUMRXDMACHANNELS:
                    val = bcmeapi_set_num_rxques(ethctl);
                    break;

                case ETHGETSOFTWARESTATS:
                    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
                    display_software_stats(pDevCtrl);
                    val = 0;
                    break;

                case ETHSETSPOWERUP:
                    swPort = port_id_from_dev(dev);
                    if (swPort >= MAX_TOTAL_SWITCH_PORTS)
                    {
                        printk("ETHSETSPOWERUP : Invalid swPort: %d \n", swPort);
                        return -EINVAL;
                    }

                    ethsw_switch_manage_port_power_mode(swPort, 1);
                    val = 0;
                    break;

                case ETHSETSPOWERDOWN:
                    swPort = port_id_from_dev(dev);
                    if (swPort >= MAX_TOTAL_SWITCH_PORTS)
                    {
                        printk("ETHSETSPOWERDOWN : Invalid swPort: %d \n", swPort);
                        return -EINVAL;
                    }

                    ethsw_switch_manage_port_power_mode(swPort, 0);
                    val = 0;
                    break;

                case ETHGETMIIREG:       /* Read MII PHY register. */
                    BCM_ENET_DEBUG("phy_id: %d; reg_num = %d \n", ethctl->phy_addr, ethctl->phy_reg);
                    {
                        uint16 val;
                        down(&bcm_ethlock_switch_config);
                        ethsw_phyport_rreg2(ethctl->phy_addr,
                                       ethctl->phy_reg & 0x1f, &val, ethctl->flags);
                        BCM_ENET_DEBUG("phy_id: %d;   reg_num = %d  val = 0x%x\n",
                                                    ethctl->phy_addr, ethctl->phy_reg, val);
                        up(&bcm_ethlock_switch_config);
                        ethctl->ret_val = val;
                    }
                    break;

                case ETHSETMIIREG:       /* Write MII PHY register. */
                    BCM_ENET_DEBUG("phy_id: %d; reg_num = %d; val = 0x%x \n", ethctl->phy_addr,
                            ethctl->phy_reg, ethctl->val);
                    {
                        uint16 val = ethctl->val;
                        down(&bcm_ethlock_switch_config);
                        ethsw_phyport_wreg2(ethctl->phy_addr, 
                                       ethctl->phy_reg & 0x1f, &val, ethctl->flags);
                        BCM_ENET_DEBUG("phy_id: %d; reg_num = %d  val = 0x%x\n",
                                                    ethctl->phy_addr, ethctl->phy_reg, val);
                        up(&bcm_ethlock_switch_config);
                    }
                    break;

                default:
                    val = -EOPNOTSUPP;
                    break;
            }
            break;

        default:
            val = -EOPNOTSUPP;
            break;
    }

    return val;
}


#if defined(ENET_GPON_CONFIG)

static const char* gponif_name = "gpon%d";

/****************************************************************************/
/* Create a new GPON Virtual Interface                                      */
/* Inputs: name = the name for the gpon i/f. If not specified, a name of    */
/*          gponXX will be assigned where XX is the next available number   */
/* Returns: 0 on success; a negative value on failure                       */
/* Notes: 1. The max num gpon virtual interfaces is limited to MAX_GPON_IFS.*/
/****************************************************************************/
static int create_gpon_vport(char *name)
{
    struct net_device *dev;
    struct sockaddr sockaddr;
    BcmEnet_devctrl *pDevCtrl = NULL;
    int status, ifid = 0;
    PHY_STAT phys;

    phys.lnk = 0;
    phys.fdx = 0;
    phys.spd1000 = 0;
    phys.spd100 = 0;

    /* Verify that given name is valid */
    if (name[0] != 0) {
        if (!dev_valid_name(name)) {
            printk("The given interface name is invalid \n");
            return -EINVAL;
        }

        /* Verify that no interface exists with this name */
        dev = dev_get_by_name(&init_net, name);
        if (dev != NULL) {
            dev_put(dev);
            printk("The given interface already exists \n");
            return -EINVAL;
        }
    }

    /* Find a free id for the gponif */
    for (ifid = 0; ifid < MAX_GPON_IFS; ifid++) {
        if (gponifid_to_dev[ifid] == NULL)
            break;
    }
    /* No free id is found. We can't create a new gpon virtual i/f */
    if (ifid == MAX_GPON_IFS) {
        printk("Create Failed as the number of gpon interfaces is "
               "limited to %d\n", MAX_GPON_IFS);
        return -EPERM;
    }

    /* Allocate the dev */
    if ((dev = alloc_etherdev(sizeof(BcmEnet_devctrl))) == NULL) {
        return -ENOMEM;
    }
    /* Set the private are to 0s */
    memset(netdev_priv(dev), 0, sizeof(BcmEnet_devctrl));

    /* Set the pDevCtrl->dev to dev */
    pDevCtrl = netdev_priv(dev);
    pDevCtrl->dev = dev;

    /* Assign name to the i/f */
    if (name[0] == 0) {
        /* Allocate and assign a name to the i/f */
        dev_alloc_name(dev, gponif_name);
    } else {
        /* Assign the given name to the i/f */
        strcpy(dev->name, name);
    }

    SET_MODULE_OWNER(dev);

    dev->netdev_ops       = vnet_dev[0]->netdev_ops;
    dev->priv_flags       = vnet_dev[0]->priv_flags;

    /* Set this flag to block forwarding of traffic between
       GPON virtual interfaces */
    dev->priv_flags       |= IFF_WANDEV;

    bcmeapi_create_vport(dev);
    /* For now, let us use this base_addr field to identify GPON port */
    /* TBD: Change this to a private field in pDevCtrl for all eth and gpon
       virtual ports */
    dev->base_addr        = GPON_PORT_ID;
    pDevCtrl->sw_port_id  = GPON_PORT_ID;

    netdev_path_set_hw_port(dev, GPON_PORT_ID, BLOG_GPONPHY);
    dev->path.hw_subport_mcast_idx = NETDEV_PATH_HW_SUBPORTS_MAX;

    /* The unregister_netdevice will call the destructor
       through netdev_run_todo */
    dev->destructor        = free_netdev;

    /* Note: Calling from ioctl, so don't use register_netdev
       which takes rtnl_lock */
    status = register_netdevice(dev);

    if (status != 0) {
        unregister_netdevice(dev);
        return status;
    }

    /* Indicate that ifid is used */
    freeid_map[ifid] = 1;

    /* Store the dev pointer at the index given by ifid */
    gponifid_to_dev[ifid] = dev;

    /* Store the ifid in dev private area */
    pDevCtrl->gponifid = ifid;

#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
    status = kerSysGetMacAddress(dev->dev_addr, dev->ifindex);
    if (status < 0)
#endif
    {
        memmove(dev->dev_addr, vnet_dev[0]->dev_addr, ETH_ALEN);
    }
    memmove(sockaddr.sa_data, dev->dev_addr, ETH_ALEN);
    dev->netdev_ops->ndo_set_mac_address(dev, &sockaddr);

    printk("%s: MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
            dev->name,
            dev->dev_addr[0],
            dev->dev_addr[1],
            dev->dev_addr[2],
            dev->dev_addr[3],
            dev->dev_addr[4],
            dev->dev_addr[5]
            );

    return 0;
}

/****************************************************************************/
/* Delete GPON Virtual Interface                                            */
/* Inputs: ifname = the GPON virtual interface name                         */
/****************************************************************************/
static int delete_gpon_vport(char *ifname)
{
    int i, ifid = 0;
    struct net_device *dev = NULL;
    BcmEnet_devctrl *pDevCtrl = NULL;

    /* Get the device structure from ifname */
    for (ifid = 0; ifid < MAX_GPON_IFS; ifid++) {
        dev = gponifid_to_dev[ifid];
        if (dev != NULL) {
            if (strcmp(dev->name, ifname) == 0) {
                break;
            }
        }
    }

    if (ifid >= MAX_GPON_IFS) {
        printk("delete_gpon_vport() : No such device \n");
        return -ENXIO;
    }

    /* Get the ifid of this interface */
    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
    ifid =  pDevCtrl->gponifid;

    /* */
    synchronize_net();

    /* Remove the gem_ids supported by this interface */
    for (i = 0; i < MAX_GEM_IDS; i++) {
        remove_gemid_to_gponif_mapping(i, ifid);
    }

    if(memcmp(vnet_dev[0]->dev_addr, dev->dev_addr, ETH_ALEN)) {
        kerSysReleaseMacAddress(dev->dev_addr);
    }

    /* Delete the given gopn virtual interfaces. No need to call free_netdev
       after this as dev->destructor is set to free_netdev */
    unregister_netdevice(dev);
    gponifid_to_dev[ifid] = NULL;

    /* Free the ifid */
    freeid_map[ifid] = 0;

    /* Set the default gemid for this ifid as 0 */
    default_gemid[ifid] = 0;

    return 0;
}

/****************************************************************************/
/* Delete GPON Virtual Interface                                            */
/* Inputs: port = the GPON virtual interface number                         */
/****************************************************************************/
static int delete_all_gpon_vports(void)
{
    int i;
    struct net_device *dev = NULL;

    /* */
    synchronize_net();

    /* Make no gemid is assigned to an interface */
    initialize_gemid_to_ifIdx_mapping();

    /* Delete all gpon virtual interfaces */
    for (i = 0; i < MAX_GPON_IFS; i++) {
        if (gponifid_to_dev[i] == NULL) {
            continue;
        }
        dev = gponifid_to_dev[i];

        if(memcmp(vnet_dev[0]->dev_addr, dev->dev_addr, ETH_ALEN)) {
            kerSysReleaseMacAddress(dev->dev_addr);
        }

        /* No need to call free_netdev after this as dev->destructor
           is set to free_netdev */
        unregister_netdevice(dev);
        gponifid_to_dev[i] = NULL;
    }

    /* Free the ifids */
    for (i = 0; i < MAX_GPON_IFS; i++) {
        freeid_map[i] = 0;
    }

    /* Make default gem_ids for all ifids as 0 */
    for (i = 0; i < MAX_GPON_IFS; i++) {
        default_gemid[i] = 0;
    }

    return 0;
}

/****************************************************************************/
/* Set the multicast gem ID in GPON virtual interface                       */
/* Inputs: multicast gem port index                                         */
/****************************************************************************/
static int set_mcast_gem_id(uint8 *pgem_map_arr)
{
    int i;
    int mcast_gemid;
    int ifIdx;
    int ifid;
    struct net_device *dev = NULL;
    bool found = false;

    for (i = 0; i < MAX_GEM_IDS; i++) {
        if (pgem_map_arr[i]) {
            mcast_gemid = i;
            found = true;
            break;
        }
    }
    if (!found)
    {
        printk("Error - set_mcast_gem_id() : No gemIdx in gem_map\n");
        return -1;
    }
    for (ifIdx = 0; ifIdx < MAX_GPON_IFS_PER_GEM; ++ifIdx)
    {
        if (gem_to_gponifid[mcast_gemid][ifIdx] == UNASSIGED_IFIDX_VALUE)
        {
            break;
        }
        ifid = gem_to_gponifid[mcast_gemid][ifIdx];
        if (ifid >= 0 && ifid < MAX_GPON_IFS) {
            if (gponifid_to_dev[ifid] != NULL) {
                dev = gponifid_to_dev[ifid];
                netdev_path_set_hw_subport_mcast_idx(dev, mcast_gemid);
                printk("mcast_gem <%d> added to if <%s>\n",mcast_gemid,dev->name);
            }
        }
    }
    return 0;
}
/****************************************************************************/
/* Set the GEM Mask or Get the GEM Mask or Get the Free GEM Mask            */
/* Inputs: ifname = the interface name                                      */
/*         pgemmask = ptr to GEM mask if op is Set                          */
/* Outputs: pgem_map = ptr to GEM mask of an interface for Get op           */
/*                     ptr to Free GEM mask for GetFree op                  */
/* Returns: 0 on success; non-zero on failure                               */
/****************************************************************************/
static int set_get_gem_map(int op, char *ifname, int ifnum, uint8 *pgem_map_arr)
{
    int i, ifid = 0, count = 0, def_gem = 0;
    struct net_device *dev = NULL;
    BcmEnet_devctrl *pDevCtrl = NULL;

    /* Check whether ifname is all */
    if (ifname[0] != 0) {
        /* The ifname is not all */

        /* Get the device structure from ifname */
        for (ifid = 0; ifid < MAX_GPON_IFS; ifid++) {
            if (gponifid_to_dev[ifid] != NULL) {
                if (strcmp(gponifid_to_dev[ifid]->name, ifname) == 0) {
                    dev = gponifid_to_dev[ifid];
                    break;
                }
            }
        }

        if (dev == NULL) {
            printk("set_get_gem_map() : No such device \n");
            return -ENXIO;
        }

        /* Get the pointer to DevCtrl private structure */
        pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);

        if (op == GETGEMIDMAP) {
            initGemIdxMap(pgem_map_arr);
            /* Get the gem ids of given interface */
            for (i = 0; i < MAX_GEM_IDS; i++) {
                if (is_gemid_mapped_to_gponif(i, ifid) == TRUE) {
                    pgem_map_arr[i] = 1;
                }
            }
        } else if (op == GETFREEGEMIDMAP) {
            initGemIdxMap(pgem_map_arr);
            /* Get the free gem ids */
            for (i = 0; i < MAX_GEM_IDS; i++) {
                if (get_first_gemid_to_ifIdx_mapping(i) == UNASSIGED_IFIDX_VALUE) {
                    pgem_map_arr[i] = 1;
                }
            }
        } else if (op == SETGEMIDMAP) {
//printk("SETGEMIDMAP: Given gemmap is ");
            dumpGemIdxMap(pgem_map_arr);
            /* Set the gem ids for given interface */
            for (i = 0; i < MAX_GEM_IDS; i++) {
                /* Check if gem_id(=i) is already a member */
                if (is_gemid_mapped_to_gponif(i, ifid) == TRUE) {
                    /* gem_id is already a member */
                    /* Check whether to remove it or not */
                    if (!(pgem_map_arr[i])) {
                        /* It is not a member in the new
                           gem_map_arr, so remove it */
                        remove_gemid_to_gponif_mapping(i, ifid);
                    } else {
                        count++;
                        if (count == 1)
                            def_gem = i;
                    }
                }
                else if (pgem_map_arr[i]) {
                    /* gem_id(=i) is not a member and is in the new
                       gem_map, so add it */
                    if (add_gemid_to_gponif_mapping(i, ifid) < 0)
                    {
                        printk("Error while adding gem<%d> to if<%s>\n",i,ifname);
                        return -ENXIO;
                    }
                    count++;
                    if (count == 1)
                        def_gem = i;
                }
            }
            pDevCtrl->gem_count = count;

            /* Set the default_gemid[ifid] when count is 1 */
            if (count == 1) {
                default_gemid[ifid] = def_gem;
            }
        }
    } else {
        /* ifname is all */
        if (op == GETGEMIDMAP) {
            /* Give the details if there is an interface at given ifnumber */
            initGemIdxMap(pgem_map_arr);
            if (gponifid_to_dev[ifnum] != NULL) {
                dev = gponifid_to_dev[ifnum];
                /* Get the gem ids of given interface */
                for (i = 0; i < MAX_GEM_IDS; i++) {
                    if (is_gemid_mapped_to_gponif(i, ifnum) == TRUE) {
                        pgem_map_arr[i] = 1;
                    }
                }
                /* Get the interface name */
                strcpy(ifname, dev->name);
            }
        }
        else {
            printk("valid ifname is required \n");
            return -ENXIO;
        }
    }

    return 0;
}

/****************************************************************************/
/* Dump the Gem Index Map                                                   */
/* Inputs: Gem Index Map Array                                              */
/* Outputs: None                                                            */
/* Returns: None                                                            */
/****************************************************************************/
static void dumpGemIdxMap(uint8 *pgem_map_arr)
{
    int gemDumpIdx = 0;
    bool gemIdxDumped = false;

    BCM_ENET_DEBUG("GemIdx Map: ");
    for (gemDumpIdx = 0; gemDumpIdx < MAX_GEM_IDS; gemDumpIdx++) 
    {
        if (pgem_map_arr[gemDumpIdx]) 
        {
            BCM_ENET_DEBUG("%d ", gemDumpIdx);
            gemIdxDumped = true;
        }
    }
    if (!gemIdxDumped) 
    {
        BCM_ENET_DEBUG("No gem idx set");
    }
}

/****************************************************************************/
/* Initialize the Gem Index Map                                                   */
/* Inputs: Gem Index Map Array                                              */
/* Outputs: None                                                            */
/* Returns: None                                                            */
/****************************************************************************/
static void initGemIdxMap(uint8 *pgem_map_arr)
{
    int i=0;
    for (i = 0; i < MAX_GEM_IDS; i++) 
    {
        pgem_map_arr[i] = 0;        
    }
}
#endif

#if defined(ENET_EPON_CONFIG)
static int create_epon_vport(const char *name)
 
{
    struct net_device *dev;
    struct sockaddr sockaddr;
    BcmEnet_devctrl *pDevCtrl = NULL;
    int status, ifid = 0;
    PHY_STAT phys;

    phys.lnk = 0;
    phys.fdx = 0;
    phys.spd1000 = 0;
    phys.spd100 = 0;
    /* Verify that given name is valid */
    if (name[0] != 0) {
        if (!dev_valid_name(name)) {
            printk("The given interface name is invalid \n");
            return -EINVAL;
        }

        /* Verify that no interface exists with this name */
        dev = dev_get_by_name(&init_net, name);
        if (dev != NULL) {
            dev_put(dev);
            printk("The given interface already exists \n");
            return -EINVAL;
        }
    }

    /* Find a free id for the eponif */
    for (ifid = 0; ifid < MAX_EPON_IFS; ifid++) {
        if (eponifid_to_dev[ifid] == NULL)
            break;
    }


    /* No free id is found. We can't create a new epon virtual i/f */
    if (ifid == MAX_EPON_IFS) {
        printk("Create Failed as the number of epon interfaces is "
               "limited to %d\n", MAX_EPON_IFS);
        return -EPERM;
    }

    /* Allocate the dev */
    if ((dev = alloc_etherdev(sizeof(BcmEnet_devctrl))) == NULL) {
        return -ENOMEM;
    }
    /* Set the private are to 0s */
    memset(netdev_priv(dev), 0, sizeof(BcmEnet_devctrl));

    /* Set the pDevCtrl->dev to dev */
    pDevCtrl = netdev_priv(dev);
    pDevCtrl->dev = dev;
    pDevCtrl->default_txq = 0;
    pDevCtrl->use_default_txq = 0;

    /* Assign name to the i/f */
    if (name[0] == 0) 
    {
        /* Allocate and assign a name to the i/f */
        dev_alloc_name(dev, eponif_name);
    } 
    else 
    {
        /* Assign the given name to the i/f */
        strcpy(dev->name, name);
	    dev_alloc_name(dev, dev->name);
    }
   	
    SET_MODULE_OWNER(dev);

    dev->netdev_ops  = vnet_dev[0]->netdev_ops;
    dev->priv_flags  = vnet_dev[0]->priv_flags;
    dev->features    = vnet_dev[0]->features;

    /* Set this flag to block forwarding of traffic between
       EPON virtual interfaces */
    dev->priv_flags       |= IFF_WANDEV;

    bcmeapi_create_vport(dev);
    /* For now, let us use this base_addr field to identify EPON port */
    /* TBD: Change this to a private field in pDevCtrl for all eth and gpon
       virtual ports */
    dev->base_addr        = EPON_PORT_ID;
    pDevCtrl->sw_port_id  = EPON_PORT_ID;

    dev->path.hw_subport_mcast_idx = NETDEV_PATH_HW_SUBPORTS_MAX;

    /* The unregister_netdevice will call the destructor
       through netdev_run_todo */
    dev->destructor        = free_netdev;


    /* Note: Calling from ioctl, so don't use register_netdev
       which takes rtnl_lock */
    status = register_netdev(dev);

    if (status != 0) {
        unregister_netdev(dev);
        return status;
    }
    
    /* Store the dev pointer at the index given by ifid */	
    eponifid_to_dev[ifid] = dev;

#ifdef SEPARATE_MAC_FOR_WAN_INTERFACES
    status = kerSysGetMacAddress(dev->dev_addr, dev->ifindex);
    if (status < 0)
#endif
    {
        memmove(dev->dev_addr, vnet_dev[0]->dev_addr, ETH_ALEN);
    }

    memmove(sockaddr.sa_data, dev->dev_addr, ETH_ALEN);
    

    dev->netdev_ops->ndo_set_mac_address(dev, &sockaddr);
    netdev_path_set_hw_port(dev, EPON_PORT_ID, BLOG_EPONPHY);
    
    printk("%s: MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
            dev->name,
            dev->dev_addr[0],
            dev->dev_addr[1],
            dev->dev_addr[2],
            dev->dev_addr[3],
            dev->dev_addr[4],
            dev->dev_addr[5]
            );

    return 0;
}

/****************************************************************************/
/* Delete All EPON Virtual Interface                                            */
/****************************************************************************/
static int delete_all_epon_vports(void)
{
    int i;
    struct net_device *dev = NULL;

    /* */
    synchronize_net();

    /* Delete all epon virtual interfaces */
    for (i = 0; i < MAX_EPON_IFS; i++) {
        if (eponifid_to_dev[i] == NULL) {
            continue;
        }
        dev = eponifid_to_dev[i];

        if(memcmp(vnet_dev[0]->dev_addr, dev->dev_addr, ETH_ALEN)) {
            kerSysReleaseMacAddress(dev->dev_addr);
        }

        /* No need to call free_netdev after this as dev->destructor
           is set to free_netdev */
        unregister_netdev(dev);
        eponifid_to_dev[i] = NULL;
    }

    return 0;
}

int create_oam_vport(char *name)
{
    struct net_device *dev;
    struct sockaddr sockaddr;
    BcmEnet_devctrl *pDevCtrl = NULL;
    int status = 0;
    PHY_STAT phys;

    phys.lnk = 0;
    phys.fdx = 0;
    phys.spd1000 = 0;
    phys.spd100 = 0;
    /* Verify that given name is valid */
    if (name[0] != 0) {
        if (!dev_valid_name(name)) {
            printk("The given interface name is invalid \n");
            return -EINVAL;
        }

        /* Verify that no interface exists with this name */
        dev = dev_get_by_name(&init_net, name);
        if (dev != NULL) {
            dev_put(dev);
            printk("The given interface already exists \n");
            return -EINVAL;
        }
    }

    /* No free id is found. We can't create a new epon virtual i/f */
    if (oam_dev != NULL) {
        printk("Create oam network interface failed: device already exists!\n");
        return -EPERM;
    }

    /* Allocate the dev */
    if ((dev = alloc_etherdev(sizeof(BcmEnet_devctrl))) == NULL) {
        return -ENOMEM;
    }

    /* Set the private are to 0s */
    memset(netdev_priv(dev), 0, sizeof(BcmEnet_devctrl));

    /* Set the pDevCtrl->dev to dev */
    pDevCtrl = netdev_priv(dev);
    pDevCtrl->dev = dev;
    pDevCtrl->default_txq = 0;
    pDevCtrl->use_default_txq = 0;

    /* Assign name to the i/f */
    if (name[0] == 0) {
        /* Allocate and assign a name to the i/f */
        dev_alloc_name(dev, oamif_name);
    } else {
        /* Assign the given name to the i/f */
        strcpy(dev->name, name);
	    dev_alloc_name(dev, dev->name); 
    }
   	
    SET_MODULE_OWNER(dev);

    dev->netdev_ops       = vnet_dev[0]->netdev_ops;
    dev->priv_flags       = vnet_dev[0]->priv_flags;
    dev->features               = vnet_dev[0]->features;

    /* Set this flag to block forwarding of traffic between
       EPON virtual interfaces */
    dev->priv_flags       |= IFF_WANDEV;



    bcmeapi_create_vport(dev);
    /* For now, let us use this base_addr field to identify EPON port */
    /* TBD: Change this to a private field in pDevCtrl for all eth and gpon
       virtual ports */
    dev->base_addr        = EPON_PORT_ID;
    pDevCtrl->sw_port_id  = EPON_PORT_ID;

    dev->path.hw_subport_mcast_idx = NETDEV_PATH_HW_SUBPORTS_MAX;

    /* The unregister_netdevice will call the destructor
       through netdev_run_todo */
    dev->destructor        = free_netdev;

    /* Note: Calling from ioctl, so don't use register_netdev
       which takes rtnl_lock */
    status = register_netdev(dev);

    if (status != 0) {
        unregister_netdev(dev);
        return status;
    }
    
    oam_dev = dev;

    /* oam network interface mac address is to be configured by userspace */
    memmove(dev->dev_addr, vnet_dev[0]->dev_addr, ETH_ALEN);

    memmove(sockaddr.sa_data, dev->dev_addr, ETH_ALEN);
    dev->netdev_ops->ndo_set_mac_address(dev, &sockaddr);

    printk("%s: MAC Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
            dev->name,
            dev->dev_addr[0],
            dev->dev_addr[1],
            dev->dev_addr[2],
            dev->dev_addr[3],
            dev->dev_addr[4],
            dev->dev_addr[5]
            );

    return 0;
}

int delete_oam_vport(char *ifname)
{
    struct net_device *dev = NULL;
    BcmEnet_devctrl *pDevCtrl = NULL;

    if ((oam_dev == NULL) ||  
        (strcmp(oam_dev->name, ifname) != 0)) 
    {
        printk("delete_oam_vport() : No such device \n");
        return -ENXIO;
    }

    dev = oam_dev;

    /* Get the ifid of this interface */
    pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
    
    /* */
    synchronize_net();
    if(memcmp(vnet_dev[0]->dev_addr, dev->dev_addr, ETH_ALEN)) {
        kerSysReleaseMacAddress(dev->dev_addr);
    }

    /* Delete the given epon virtual interfaces. No need to call free_netdev
       after this as dev->destructor is set to free_netdev */
    unregister_netdev(dev);
    oam_dev = NULL;
    return 0;
}

EXPORT_SYMBOL(create_oam_vport);
EXPORT_SYMBOL(delete_oam_vport);
#endif

static int __init bcmenet_module_init(void)
{
    int status;

    TRACE(("bcm63xxenet: bcmenet_module_init\n"));

    bcmeapi_module_init();

    sema_init(&bcm_ethlock_switch_config, 1);
    sema_init(&bcm_link_handler_config, 1);

    if ( BCM_SKB_ALIGNED_SIZE != skb_aligned_size() )
    {
        printk("skb_aligned_size mismatch. Need to recompile enet module\n");
        return -ENOMEM;
    }

#if defined(_CONFIG_BCM_ARL)
    bcm_arl_process_hook_g = enet_hook_for_arl_access;
#endif

#if defined(ENET_GPON_CONFIG)
    initialize_gemid_to_ifIdx_mapping();
#endif

	/* create a slab cache for device descriptors */
    enetSkbCache = kmem_cache_create("bcm_EnetSkbCache",
                                     BCM_SKB_ALIGNED_SIZE,
                                     0, /* align */
                                     SLAB_HWCACHE_ALIGN, /* flags */
                                     NULL); /* ctor */
    if(enetSkbCache == NULL)
    {
        printk(KERN_NOTICE "Eth: Unable to create skb cache\n");

        return -ENOMEM;
    }

#if defined(CONFIG_SKY_MESH_SUPPORT)
	ethsw_set_hw_switching(HW_SWITCHING_DISABLED);
#endif 
    status = bcm63xx_enet_probe();

#if defined(SUPPORT_SWMDK)
    bcmFun_reg(BCM_FUN_ID_ENET_LINK_CHG, link_change_handler_wrapper);
#endif /*SUPPORT_SWMDK*/

    bcmeapi_module_init2();
#if defined(CONFIG_SKY_MESH_SUPPORT)
	/* mustafakaraca: STP should be handled completely in software */
	//register_bridge_stp_notifier(&br_stp_handler);	
#else
    register_bridge_stp_notifier(&br_stp_handler);
#endif
    return status;
}

int bcm63xx_enet_getPortFromDev(struct net_device *dev, int *pUnit, int *pPort)
{
   int port;
   int i;

   for (i = 1; i <= vport_cnt; i++) {
      if (dev == vnet_dev[i]) {
         break;
      }
   }

   if ( i > vport_cnt ) {
       return -1;
   }

   port = port_id_from_dev(dev);
   if ( port >= MAX_TOTAL_SWITCH_PORTS )
   {
      return -1;
   }
   *pUnit = LOGICAL_PORT_TO_UNIT_NUMBER(port);
   *pPort = LOGICAL_PORT_TO_PHYSICAL_PORT(port);

   return 0;

}

int bcm63xx_enet_getPortFromName(char *pIfName, int *pUnit, int *pPort)
{
   struct net_device *dev;
   
   dev = dev_get_by_name(&init_net, pIfName);
   if (NULL == dev)
   {
      return -1;
   }

   if ( bcm63xx_enet_getPortFromDev(dev, pUnit, pPort) < 0 )
   {
      dev_put(dev);
      return -1;
   }

   dev_put(dev);

   return 0;
}

static uint32_t bridge_get_ext_phy_pmap(char *brName)
{
    unsigned int brPort = 0xFFFFFFFF;
    struct net_device *dev;
    uint32_t portMap = 0, port;

    for(;;)
    {
        int unit;
        
        dev = bridge_get_next_port(brName, &brPort);
        if (dev == NULL)
            break;

        if ( bcm63xx_enet_getPortFromDev(dev, &unit, &port) < 0 )
        {
          continue;
        }

        if (0 == unit)
            continue;

        portMap |= (1<<port);
    }

    return portMap;
}

static int bridge_notifier(struct notifier_block *nb, unsigned long event, void *brName)
{
    switch (event)
    {
        case BREVT_IF_CHANGED:
            bridge_update_ext_pbvlan(brName);
            break;
    }
    return NOTIFY_DONE;
}

static int bridge_stp_handler(struct notifier_block *nb, unsigned long event, void *portInfo)
{
    struct stpPortInfo *pInfo = (struct stpPortInfo *)portInfo;
    BcmEnet_devctrl *pDevCtrl = netdev_priv(vnet_dev[0]);

    switch (event)
    {
        case BREVT_STP_STATE_CHANGED:    
        {
            unsigned char stpVal;
            int port;
            int unit;

            if ( bcm63xx_enet_getPortFromName(&pInfo->portName[0], &unit, &port ) < 0 )
            {
                break;
            }

            switch ( pInfo->stpState )
            {
               case BR_STATE_BLOCKING:
                  stpVal = REG_PORT_STP_STATE_BLOCKING;
                  break;
                   
               case BR_STATE_FORWARDING:
                  stpVal = REG_PORT_STP_STATE_FORWARDING;
                  break;
        
               case BR_STATE_LEARNING:
                  stpVal = REG_PORT_STP_STATE_LEARNING;
                  break;
        
               case BR_STATE_LISTENING:
                  stpVal = REG_PORT_STP_STATE_LISTENING;
                  break;
        
               case BR_STATE_DISABLED:
                  stpVal = REG_PORT_STP_STATE_DISABLED;
                  break;
        
               default:
                  stpVal = REG_PORT_NO_SPANNING_TREE;
                  break;
            }

            if ( 0 == (pDevCtrl->stpDisabledPortMap & (1 << PHYSICAL_PORT_TO_LOGICAL_PORT(port, unit))) ) {
               bcmeapi_ethsw_set_stp_mode(unit, port, stpVal);
            }
            break;
        }
    }
    return NOTIFY_DONE;
}


static void bridge_update_ext_pbvlan(char *brName)
{
    unsigned int brPort = 0xFFFFFFFF;
    struct net_device *dev;
    uint32_t portMap, curMap, port;

    if (extSwInfo.present == 0)
    {
        return;
    }

    curMap = bridge_get_ext_phy_pmap(brName);
    if (curMap == 0)
    {
        return;
    }

    for(;;)
    {
        int unit;
        
        dev = bridge_get_next_port(brName, &brPort);
        if (dev == NULL)
        {
           break;
        }

        if ( bcm63xx_enet_getPortFromDev(dev, &unit, &port) < 0 )
        {
           continue;
        }

        if (0 == unit)
        {
           continue;
        }

        portMap = (curMap) & (~(1<<port));
        portMap |= (1<<MIPS_PORT_ID);
        bcmsw_set_ext_switch_pbvlan(port, portMap);
    }
}


/* 
 * We need this function in non-gmac build as well.
 * It searches both the internal and external switch ports.
 */
int enet_logport_to_phyid(int log_port)
{
    struct net_device *dev = vnet_dev[0];
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
    int phy_id = -1;

    ASSERT(log_port < (MAX_TOTAL_SWITCH_PORTS));
    phy_id  = pDevCtrl->EnetInfo[LOGICAL_PORT_TO_UNIT_NUMBER(log_port)].
                sw.phy_id[LOGICAL_PORT_TO_PHYSICAL_PORT(log_port)];
    ASSERT(phy_id != -1);
    return phy_id;
}

unsigned long enet_get_portmap(unsigned char unit)
{
    struct net_device *dev = vnet_dev[0];
    BcmEnet_devctrl *pDevCtrl = (BcmEnet_devctrl *)netdev_priv(dev);
    return pDevCtrl->EnetInfo[unit].sw.port_map;
}

#ifdef DYING_GASP_API
int enet_send_dying_gasp_pkt(void)
{
    struct net_device *dev = NULL;
    int i;

	/* Indicate we are in a dying gasp context and can skip 
	  housekeeping since we're about to power down */
	dg_in_context = 1;

    if (dg_skbp == NULL) {
        BCM_ENET_DEBUG("%s No DG skb to send \n", __FUNCTION__);
        return -1; 
    }
    for (i = 0; i < MAX_TOTAL_SWITCH_PORTS - 1; i++) 
    {
		int iVport = LOGICAL_PORT_TO_VPORT(i);

		/* Is this a port for a valid ethernet device? */
		if(iVport <= 0)
		{
			/* Nope - skip to next port */			
			continue;
		}
			
		/* Get dev pointer for this port */
		dev = vnet_dev[iVport];			
		
		/* Is this a WAN port? */
		if (dev && dev->priv_flags & IFF_WANDEV) {
			int iRetVal;
	
			/* Copy src MAC from dev into dying gasp packet */				
			memcpy(dg_skbp->data + ETH_ALEN, dev->dev_addr, ETH_ALEN);
			
			/* Transmit dying gasp packet */
			iRetVal = bcm63xx_enet_xmit(SKBUFF_2_PNBUFF(dg_skbp), dev);				
			printk("\n%s DG sent out on wan port %s (ret=%d)\n", __FUNCTION__, dev->name, iRetVal);				
			break;
		}
    } // for
    return 0;
}
#endif

/* Physical port to logical port mapping */
struct net_device *enet_phyport_to_vport_dev(int port)
{
    ASSERT(port < TOTAL_SWITCH_PORTS);
    return (vnet_dev[LOGICAL_PORT_TO_VPORT(PHYSICAL_PORT_TO_LOGICAL_PORT(port, extSwInfo.present))]);
}

uint32 ConfigureJumboPort(uint32 regVal, int portVal, unsigned int configVal) // bill
{
  UINT32 controlBit;

  // Test for valid port specifier.
  if ((portVal >= ETHSWCTL_JUMBO_PORT_GPHY_0) && (portVal <= ETHSWCTL_JUMBO_PORT_ALL))
  {
    // Switch on port ID.
    switch (portVal)
    {
    case ETHSWCTL_JUMBO_PORT_MIPS:
      controlBit = ETHSWCTL_JUMBO_PORT_MIPS_MASK;
      break;
    case ETHSWCTL_JUMBO_PORT_GPON:
      controlBit = ETHSWCTL_JUMBO_PORT_GPON_MASK;
      break;
    case ETHSWCTL_JUMBO_PORT_USB:
      controlBit = ETHSWCTL_JUMBO_PORT_USB_MASK;
      break;
    case ETHSWCTL_JUMBO_PORT_MOCA:
      controlBit = ETHSWCTL_JUMBO_PORT_MOCA_MASK;
      break;
    case ETHSWCTL_JUMBO_PORT_GPON_SERDES:
      controlBit = ETHSWCTL_JUMBO_PORT_GPON_SERDES_MASK;
      break;
    case ETHSWCTL_JUMBO_PORT_GMII_2:
      controlBit = ETHSWCTL_JUMBO_PORT_GMII_2_MASK;
      break;
    case ETHSWCTL_JUMBO_PORT_GMII_1:
      controlBit = ETHSWCTL_JUMBO_PORT_GMII_1_MASK;
      break;
    case ETHSWCTL_JUMBO_PORT_GPHY_1:
      controlBit = ETHSWCTL_JUMBO_PORT_GPHY_1_MASK;
      break;
    case ETHSWCTL_JUMBO_PORT_GPHY_0:
      controlBit = ETHSWCTL_JUMBO_PORT_GPHY_0_MASK;
      break;
    default: // ETHSWCTL_JUMBO_PORT_ALL:
      controlBit = ETHSWCTL_JUMBO_PORT_MASK_VAL;  // ALL bits
      break;
    }

    // Test for accept JUMBO frames.
    if (configVal != 0)
    {
      // Setup register value to accept JUMBO frames.
      regVal |= controlBit;
    }
    else
    {
      // Setup register value to reject JUMBO frames.
      regVal &= ~controlBit;
    }
  }

  // Return new JUMBO configuration control register value.
  return regVal;
}
module_init(bcmenet_module_init);
module_exit(bcmenet_module_cleanup);
MODULE_LICENSE("GPL");

