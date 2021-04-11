/*
* <:copyright-BRCM:2012:DUAL/GPL:standard
* 
*    Copyright (c) 2012 Broadcom Corporation
*    All Rights Reserved
* 
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License, version 2, as published by
* the Free Software Foundation (the "GPL").
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* 
* A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
* writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
* Boston, MA 02111-1307, USA.
* 
* :> 
*/

/****************************************************************************/
/**                                                                        **/
/** Software unit Wlan accelerator dev                                     **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   Wlan accelerator interface.                                          **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   Mediation layer between the wifi / PCI interface and the RDPA        **/
/**                                                                        **/
/**                                                                        **/
/** Allocated requirements:                                                **/
/**                                                                        **/
/** Allocated resources:                                                   **/
/**                                                                        **/
/**   A thread.                                                            **/
/**   An interrupt.                                                        **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/


/****************************************************************************/
/******************** Operating system include files ************************/
/****************************************************************************/
#include <linux/types.h>
#include <linux/ip.h>
#include <linux/in.h>
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <net/route.h>
#include <linux/moduleparam.h>
#include <linux/netdevice.h>
#include <linux/if_ether.h>
#include <linux/etherdevice.h>
#include <linux/netdevice.h>
#include <linux/kthread.h>
#include "bcmnet.h"

#if defined(PKTC) && defined(CONFIG_BCM_WFD_CHAIN_SUPPORT)
/*chaining support*/
extern void (*bcm63xx_wlan_txchainhandler_hook)(struct sk_buff *skb, uint32_t brc_hot_ptr, uint8_t wlTxChainIdx);
//EXPORT_SYMBOL(bcm63xx_wlan_txchainhandler_hook);

extern void (*bcm63xx_wlan_txchainhandler_complete_hook)(void);
//EXPORT_SYMBOL(bcm63xx_wlan_txchainhandler_complete_hook);

#include <linux_osl_dslcpe_pktc.h>

#endif
/****************************************************************************/
/******************* Other software units include files *********************/
/****************************************************************************/
#include "rdpa_api.h"
#include "rdpa_mw_blog_parse.h"
/****************************************************************************/
/***************************** Module Version *******************************/
/****************************************************************************/
static const char *version = "Wifi Forwarding Driver";

#define WIFI_MW_MAX_NUM_IF    ( 16 )
#define WIFI_IF_NAME_STR_LEN  ( IFNAMSIZ )
#define WLAN_CHAINID_OFFSET 8
/****************************************************************************/
/*********************** Multiple SSID FUNCTIONALITY ************************/
/****************************************************************************/
static struct net_device *wifi_net_devices[WIFI_MW_MAX_NUM_IF]={NULL, };


/****************************************************************************/
/***************************** Definitions  *********************************/
/****************************************************************************/
static bdmf_object_handle rdpa_cpu_obj;


static int wifi_netdev_event (struct notifier_block *n, unsigned long event, void *v);

static struct notifier_block wifi_netdev_notifer = {
    .notifier_call = wifi_netdev_event ,
};

//static struct tasklet_struct wifi_rx_tasklet ;

static struct proc_dir_entry *proc_directory, *proc_file_conf;

static struct task_struct *rx_thread;
static wait_queue_head_t   rx_thread_wqh;
static int                 rx_work_avail = 0;
extern void replace_upper_layer_packet_destination( void * cb, void * napi_cb );
extern void unreplace_upper_layer_packet_destination( void );

static int wlan_accelerator_tasklet_handler(void  *context);
//static void wifi_accelerator_rx_timeout_callback(int queue_id);

#define WIFIMW_WAKEUP_RXWORKER do { \
           if (rx_work_avail == 0) { \
               rx_work_avail = 1; \
               wake_up_interruptible(&rx_thread_wqh); }} while (0)
/****************************************************************************/
/***************************** Module parameters*****************************/
/****************************************************************************/
/* Initial maximum queue size */
static int packet_threshold = RDPA_CPU_WLAN_QUEUE_MAX_SIZE;
module_param (packet_threshold, int, 0);
/* Number of packets to read in each tasklet iteration */
static int num_packets_to_read = 128;
module_param (num_packets_to_read, int, 0);
/* Timer initial value, interval unit is milisec*/
static int timer_timeout = 500;
module_param (timer_timeout, int, 0);
/* Initial number of configured PCI queues */
static int number_of_queues = 1;
module_param (number_of_queues, int, 0);
/* first Cpu ring queue - Currently pci CPU ring queues must be sequentioal */
static int first_pci_queue = 8;
module_param (first_pci_queue,int,0);
/* wifi Broadcom prefix */
static char wifi_prefix [WIFI_IF_NAME_STR_LEN] = "wl";
module_param_string (wifi_prefix, wifi_prefix, WIFI_IF_NAME_STR_LEN, 0);

static int wifi_prefix_len = 0 ;

/* Counters */
static unsigned int gs_count_rx_no_wifi_interface = 0 ;
static unsigned int gs_count_rx_invalid_ssid_vector = 0 ;
static unsigned int gs_count_rx_queue_packets [WIFI_MW_MAX_NUM_IF] = {0, } ;
static unsigned int gs_count_tx_packets [WIFI_MW_MAX_NUM_IF] = {0, } ;
static unsigned int gs_count_wl_chained_packets [WIFI_MW_MAX_NUM_IF] = {0, } ;

/*****************************************************************************/
/****************** Wlan Accelerator Device implementation *******************/
/*****************************************************************************/

static inline void map_ssid_vector_to_ssid_index (uint16_t *bridge_port_ssid_vector, uint32_t *wifi_drv_ssid_index)
{
   *wifi_drv_ssid_index = __ffs ( * bridge_port_ssid_vector) ;
}

static void rdpa_port_ssid_update(int index, int create)
{
    BDMF_MATTR(rdpa_port_attrs, rdpa_port_drv());
    bdmf_object_handle rdpa_port_obj;
    int rc;

    if (create)
    {
        rdpa_port_index_set(rdpa_port_attrs, rdpa_if_ssid0 + index);
        rc = bdmf_new_and_set(rdpa_port_drv(), NULL, rdpa_port_attrs, &rdpa_port_obj);
        if (rc)
            printk("%s %s Failed to create rdpa port ssid object rc(%d)\n", __FILE__, __FUNCTION__, rc);
    }
    else
    {
        rc = rdpa_port_get(rdpa_if_ssid0 + index, &rdpa_port_obj);
        if (!rc)
        {
            bdmf_put(rdpa_port_obj);
            bdmf_destroy(rdpa_port_obj);
        }
    }
}

static int wifi_netdev_event (struct notifier_block *n, unsigned long event, void *v)
{
    struct net_device *dev = (struct net_device *) v ;
    int ret;
    uint32_t wifi_dev_index ;

    ret = NOTIFY_DONE;

    /*Check for wifi net device*/
    if (!strncmp ( wifi_prefix, dev->name, wifi_prefix_len))
    {
        wifi_dev_index = netdev_path_get_hw_port(dev);

        switch (event)
        {
           case NETDEV_REGISTER:
               if (! wifi_net_devices [wifi_dev_index])
               {
                   wifi_net_devices [wifi_dev_index] = dev;
                   dev_hold (dev) ;
                   rdpa_port_ssid_update(wifi_dev_index, 1);
               }
               ret = NOTIFY_OK ;
               break;
            case NETDEV_UNREGISTER  :
               if (wifi_net_devices [wifi_dev_index])
               {
                   dev_put(wifi_net_devices[wifi_dev_index]);
                   wifi_net_devices [wifi_dev_index] = NULL;
                   rdpa_port_ssid_update(wifi_dev_index, 0);
               }
               ret = NOTIFY_OK;
               break;
        }
    }

    return ret ;
}
static int _rdpa_cfg_cpu_rx_queue(int queue_id, uint32_t queue_size, rdpa_cpu_rxq_rx_isr_cb_t rx_isr)
{
    rdpa_cpu_rxq_cfg_t rxq_cfg;
    int rc=0;
    
    /* Read current configuration, set new drop threshold and ISR and write back. */
    bdmf_lock();
    rc = rdpa_cpu_rxq_cfg_get(rdpa_cpu_obj, queue_id+first_pci_queue, &rxq_cfg);
    if (rc)
        goto unlock_exit;
    rxq_cfg.size = queue_size;
    rxq_cfg.isr_priv = queue_id;
    rxq_cfg.rx_isr = rx_isr;
    rc = rdpa_cpu_rxq_cfg_set(rdpa_cpu_obj, queue_id+first_pci_queue, &rxq_cfg);

unlock_exit:
    bdmf_unlock();
    return rc;
}


static void release_wlan_accelerator_interfaces(void)
{
    int wifi_index ;

    for (wifi_index=0; wifi_index<WIFI_MW_MAX_NUM_IF; wifi_index++)
    {
        if (wifi_net_devices[wifi_index])
        {
            rdpa_port_ssid_update(wifi_index, 0);
            dev_put(wifi_net_devices[wifi_index]) ;
            wifi_net_devices [wifi_index] = NULL;
        }
    }
}

/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   send_packet_to_bridge                                                **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wlan accelerator - Rx PCI path                                       **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   Sends packet to bridge and free skb buffer                           **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static void send_packet_to_bridge(struct sk_buff *skb)
{
    rdpa_if ssid ;
    rdpa_cpu_tx_info_t cpu_tx_info= {};

    /*The SA and DA MAC addresses length are part of the data content*/
    /*Therefore adding 14 bytes of the SA and DA MAC addresses length and align the data pointer*/
    skb_push(skb, ETH_HLEN) ;

    /* Send the packet to the RDPA bridge for fast path forwarding */
    rdpa_mw_root_dev2rdpa_if(skb->dev, &ssid);
    cpu_tx_info.method = rdpa_cpu_tx_bridge;
    cpu_tx_info.port = ssid;
    rdpa_cpu_send_sysb(skb, &cpu_tx_info);

    gs_count_tx_packets[ssid-rdpa_if_ssid0]++ ;
}


#define NO_MORE_PACKETS ( 1<<31 )
#define PACKETS_CHAINED  (1 << 30 )
#define STATUS_BITS (NO_MORE_PACKETS|PACKETS_CHAINED)

static inline uint32_t wlan_accelerator_read_budget(unsigned long budget,unsigned long queue_id)
{
	int status;
    uint32_t wifi_drv_if_index ;
    uint16_t wifi_ssid_vector ;
    uint32_t  len;
    rdpa_cpu_rx_info_t cpu_rx_info ;
	struct sk_buff *skb = NULL;
	struct sk_buff *out_skb = NULL ;
	unsigned int rx_good_pkt = 0;
#if defined(PKTC) && defined(CONFIG_BCM_WFD_CHAIN_SUPPORT)
	uint8_t		wl_chainid;
	uint32_t	brc_hot_ptr = 0;
#endif

	while (budget--)
	{
		status = rdpa_cpu_packet_get(rdpa_cpu_wlan0, queue_id,(bdmf_sysb *)&skb ,&cpu_rx_info);

		if (!status)
		{
			rx_good_pkt++;
			len = skb->len;
			skb_trim(skb, len);
			wifi_ssid_vector = (uint16_t)(cpu_rx_info.reason_data);

			if (!wifi_ssid_vector)
			{
				gs_count_rx_invalid_ssid_vector++;
				dev_kfree_skb_any(skb);
				continue;
			}
			else
			{
				map_ssid_vector_to_ssid_index (&wifi_ssid_vector, &wifi_drv_if_index);
				if (!wifi_net_devices[wifi_drv_if_index])
				{
					gs_count_rx_no_wifi_interface++;
					dev_kfree_skb_any(skb);
					continue;
				}
				skb->protocol = eth_type_trans(skb, wifi_net_devices[wifi_drv_if_index]) ;
				skb_push(skb, ETH_HLEN);

				do
				{
					map_ssid_vector_to_ssid_index (&wifi_ssid_vector, &wifi_drv_if_index) ;

					/*Clear the bit we found*/
					wifi_ssid_vector &= ~ (1<<wifi_drv_if_index) ;

					/*Check if device was initialized */
					if (!wifi_net_devices[wifi_drv_if_index])
					{
						gs_count_rx_no_wifi_interface++;
						dev_kfree_skb_any(skb);
						continue;
					}

					if (wifi_ssid_vector)
					{
						/* clone skb */
						out_skb = skb_clone(skb, GFP_ATOMIC) ;

						if (!out_skb)
						{
							printk( "%s %s: Failed to clone skb \n", __FILE__, __FUNCTION__ ) ;

							return rx_good_pkt;
						}
					}
					else
					{
					   out_skb = skb;
					}

					/*set the priority of the skb*/
					skb->mark = SKBMARK_SET_Q_PRIO(skb->mark,cpu_rx_info.wl_chain_priority & 0x0f);

					gs_count_rx_queue_packets[wifi_drv_if_index]++;

#if defined(PKTC) && defined(CONFIG_BCM_WFD_CHAIN_SUPPORT)
					/*perform chain if needed*/
					wl_chainid = (cpu_rx_info.wl_chain_priority & 0xff00) >> WLAN_CHAINID_OFFSET;
					if ( wl_chainid != INVALID_CHAIN_IDX)
					{
						assert( wl_pktc_req_hook != NULL );
						brc_hot_ptr = wl_pktc_req_hook(BRC_HOT_GET_BY_IDX, wl_chainid, 0, 0);
						assert( bcm63xx_wlan_txchainhandler_hook != NULL );
						bcm63xx_wlan_txchainhandler_hook(skb,brc_hot_ptr,wl_chainid);
						rx_good_pkt |= PACKETS_CHAINED;
						gs_count_wl_chained_packets[wifi_drv_if_index]++;
					}
					else
#endif
						wifi_net_devices[wifi_drv_if_index]->netdev_ops->ndo_start_xmit(out_skb,wifi_net_devices[wifi_drv_if_index]);

				} while (wifi_ssid_vector);
			}
		}
		else if ( status == BDMF_ERR_NO_MORE)
		{
			rx_good_pkt |= NO_MORE_PACKETS;
			break;
		}
		else
		{
			printk("WIFIMW:Error reading packet with status of %d\n",status);
			break;
		}
	}
	return rx_good_pkt;
}
/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wlan_accelerator_tasklet_handler.                                    **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wlan accelerator - tasklet handler                                   **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   Reads all the packets from the Rx queue and send it to the wifi      **/
/**   interface.                                                           **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static int wlan_accelerator_tasklet_handler(void *context)
{
	int status;
	int workdone=0;
	int pci_queue=0 , queue_id;


    while(1)
    {
    	wait_event_interruptible(rx_thread_wqh, rx_work_avail);

    	if (kthread_should_stop())
		{
			printk(KERN_INFO "kthread_should_stop detected on wifi_mw_rx\n");
			break;
		}

			for (pci_queue=0; pci_queue<number_of_queues; pci_queue++)
			{
				queue_id = pci_queue + first_pci_queue;

				/* Reads all configured PCI queues in strict priority */

				status = wlan_accelerator_read_budget(num_packets_to_read,queue_id);
				workdone = status & STATUS_BITS;
				status &=~STATUS_BITS;

#if defined(PKTC) && defined(CONFIG_BCM_WFD_CHAIN_SUPPORT)
				if (workdone & PACKETS_CHAINED)
				{
					bcm63xx_wlan_txchainhandler_complete_hook();
				}
#endif
				if (status >= num_packets_to_read  )
				{
					if (current->policy == SCHED_FIFO || current->policy == SCHED_RR)
						yield();

				}
				else
				{
					/* If the queue got full while the network driver was handling previous
					 * packets, then new packets will not cause interrupt (they will be
					 * simply dropped by Runner without interrupt). In this case, no one
					 * will wake up the network driver again, and traffic will stop. So, the
					 * solution is to schedule another NAPI round that will flush the queue. */
					/*just check if ring is full*/
					if (!rdpa_cpu_queue_not_empty(rdpa_cpu_wlan0,queue_id)  )
					{
						/*no packets in queue just re-arm the queue interrupt and continue to the next queue*/
						rx_work_avail = 0;
						rdpa_cpu_int_enable(rdpa_cpu_wlan0, pci_queue);
					}
				}
			}/*for pci queue*/

		}/*for packet_threshold*/
return 0;
}

/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wlan_accelerator_dev_rx_isr_callback.                                **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   Wlan accelerator - ISR callback                                      **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   ISR callback for the PCI queues handler                              **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static void wlan_accelerator_dev_rx_isr_callback(long queue_id)
{
    /* Disable PCI interrupt */
    rdpa_cpu_int_disable (rdpa_cpu_wlan0, queue_id );
    rdpa_cpu_int_clear (rdpa_cpu_wlan0, queue_id );
    /* Call the RDPA receiving packets handler (thread or tasklet) */
    WIFIMW_WAKEUP_RXWORKER;
}





/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wifi_mw_proc_read_func_conf                                          **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wifi mw - proc read                                                  **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   Procfs callback method.                                              **/
/**      Called when someone reads proc command                            **/
/**   using: cat /proc/wifi_mw                                             **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/**   page  -  Buffer where we should write                                **/
/**   start -  Never used in the kernel.                                   **/
/**   off   -  Where we should start to write                              **/
/**   count -  How many character we could write.                          **/
/**   eof   -  Used to signal the end of file.                             **/
/**   data  -  Only used if we have defined our own buffer                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static int wifi_mw_proc_read_func_conf(char* page, char** start, off_t off, int count, int* eof, void* data)
{
    int wifi_index ;
    unsigned int count_rx_queue_packets_total=0 ;
    unsigned int count_tx_bridge_packets_total=0 ;
    int len = 0 ;

    page+=off;
    page[0]=0;
#if defined(PKTC) && defined(CONFIG_BCM_WFD_CHAIN_SUPPORT)
    len += sprintf((page+len),"bcm63xx_wlan_txchainhandler_hook = 0x%x\n",(uint32_t)bcm63xx_wlan_txchainhandler_hook);
    len += sprintf((page+len),"bcm63xx_wlan_txchainhandler_complete_hook = 0x%x\n",(uint32_t)bcm63xx_wlan_txchainhandler_complete_hook);
#endif
    for(wifi_index=0;wifi_index < WIFI_MW_MAX_NUM_IF;wifi_index++)
    {
        if( wifi_net_devices[wifi_index] != NULL )
        {
            len += sprintf((page+len),"WFD Registered Interface %d:%s \n",wifi_index,wifi_net_devices[wifi_index]->name);
        }
    }

    /*RX-MW from WiFi queues*/
    for (wifi_index=0; wifi_index<WIFI_MW_MAX_NUM_IF; wifi_index++)
    {
        if (gs_count_rx_queue_packets[wifi_index]!=0)
        {
            count_rx_queue_packets_total += gs_count_rx_queue_packets[wifi_index] ;
            if (wifi_index==0)
            {
                len += sprintf((page+len), "RX-MW from WiFi queues      [WiFi %d] = %d\n", 
                    wifi_index, gs_count_rx_queue_packets[wifi_index]) ;
            }
            else
            {
                len += sprintf((page +len), "                            [WiFi %d] = %d\n", 
                    wifi_index, gs_count_rx_queue_packets[wifi_index]) ;
            }
           if (gs_count_wl_chained_packets[wifi_index])
        	   len += sprintf((page+len), "wl chained packets                = %d\n", gs_count_wl_chained_packets[wifi_index]) ;
        }

    }

    /*TX-MW to bridge*/
    for (wifi_index=0; wifi_index<WIFI_MW_MAX_NUM_IF; wifi_index++)
    {
        if ( gs_count_tx_packets[wifi_index]!=0)
        {
            count_tx_bridge_packets_total += gs_count_tx_packets[wifi_index] ;
            if (wifi_index == 0)
            {
                len += sprintf((page+len), "TX-MW to bridge             [WiFi %d] = %d\n", 
                    wifi_index, gs_count_tx_packets[wifi_index]) ;
            }
            else
            {
                len += sprintf((page+len ), "                            [WiFi %d] = %d\n",      
                    wifi_index, gs_count_tx_packets[wifi_index]) ;
            }
        }
    }


    len += sprintf((page+len), "\nRX-MW from WiFi queues      [SUM] = %d\n", count_rx_queue_packets_total) ;
    len += sprintf((page+len), "TX-MW to bridge             [SUM] = %d\n", count_tx_bridge_packets_total) ;
    len += sprintf((page+len), "No WIFI interface                 = %d\n", gs_count_rx_no_wifi_interface) ;
    len += sprintf((page+len), "Invalid SSID vector               = %d\n", gs_count_rx_invalid_ssid_vector) ;


    memset(gs_count_rx_queue_packets, 0, sizeof(gs_count_rx_queue_packets));
    memset(gs_count_tx_packets, 0, sizeof(gs_count_tx_packets));
    memset(gs_count_wl_chained_packets, 0, sizeof(gs_count_wl_chained_packets));
    gs_count_rx_no_wifi_interface = 0 ;
    gs_count_rx_invalid_ssid_vector = 0 ;

    *eof = 1;
    return len;
}


/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wifi_proc_init.                                                      **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wifi mw - proc init                                                  **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   The function initialize the proc entry                               **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static int wifi_proc_init(void)
{
    /* make a directory in /proc */
    proc_directory = proc_mkdir("wifi_mw", NULL) ;

    if (proc_directory==NULL) goto fail_dir ;

    /* make conf file */
    proc_file_conf = create_proc_entry("conf", 0444, proc_directory) ;

    if (proc_file_conf==NULL ) goto fail_entry ;

    /* set callback function on file */
    proc_file_conf->read_proc = wifi_mw_proc_read_func_conf ;

    return (0) ;

fail_entry:
    printk("%s %s: Failed to create proc entry in wifi_mw\n", __FILE__, __FUNCTION__);
    remove_proc_entry("wifi_mw" ,NULL); /* remove already registered directory */

fail_dir:
    printk("%s %s: Failed to create directory wifi_mw\n", __FILE__, __FUNCTION__) ;
    return (-1) ;
}


/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   create_wlan_accelerator_os_resources                                 **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wifi mw - Init                                                       **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   The function initializes the OS resources                            **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**   bool - Error code:                                                   **/
/**             true - No error                                            **/
/**             false - Error                                              **/
/**                                                                        **/
/****************************************************************************/
static int create_wlan_accelerator_os_resources(void)
{
    /* Init wifi driver callback */

	replace_upper_layer_packet_destination(send_packet_to_bridge, send_packet_to_bridge);

    /* Initialize the tasklet according to the module parameter */
    rx_work_avail = 0;
    init_waitqueue_head(&rx_thread_wqh);
    rx_thread = kthread_create(wlan_accelerator_tasklet_handler, NULL, "wfd");

    wake_up_process(rx_thread);

    return (0);
}

/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   release_wlan_accelerator_os_resources                                **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   The function releases the OS resources                               **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**   bool - Error code:                                                   **/
/**             true - No error                                            **/
/**             false - Error                                              **/
/**                                                                        **/
/****************************************************************************/
static int release_wlan_accelerator_os_resources(void)
{
    /* Replace the Wifi callback to the original function */

    unreplace_upper_layer_packet_destination() ;

    /* stop kernel thread */
    kthread_stop(rx_thread);

    return (0) ;
}

/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   init_wifi_drv_resources.                                             **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   The function initializes the driver resources                        **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**   int -  Error code:                                                   **/
/**             true - No error                                            **/
/**             false - Error                                              **/
/**                                                                        **/
/****************************************************************************/
static int init_wifi_drv_resources(void)
{
    int rc = 0 ;
    int revert_queue_index=-1;
    int rx_queue_index;

    /* Initialize all the drivers OS dependant resources */
    rc = create_wlan_accelerator_os_resources() ;

    if (rc)
    {
        printk("%s %s: Failed to create WiFi mw os resources, status %d\n",   
            __FILE__, __FUNCTION__, rc);
        return (rc) ;
    }

    for (rx_queue_index=0; rx_queue_index<number_of_queues; rx_queue_index++)
    {
        /* Configure PCI RX queue */
        rc = _rdpa_cfg_cpu_rx_queue(rx_queue_index, packet_threshold,
            wlan_accelerator_dev_rx_isr_callback);
      
        if (rc)
        {
            printk("%s %s: Cannot configure PCI CPU Rx queue (%d), status (%d)\n",
               __FILE__,__FUNCTION__,rx_queue_index+first_pci_queue, rc);

           release_wlan_accelerator_os_resources();

           revert_queue_index = rx_queue_index;
           goto end_of_wifi_drv_resources;
        }
        rdpa_cpu_int_enable (rdpa_cpu_wlan0, rx_queue_index); 
    }

    return (rc);

end_of_wifi_drv_resources:
    /* Revert enabling interrupts in case of failure*/
    for (rx_queue_index=0; rx_queue_index<=revert_queue_index; rx_queue_index++)
    {
        rdpa_cpu_int_disable(rdpa_cpu_wlan0, rx_queue_index);
    }
    return (rc) ;
}


/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wlan_accelerator_dev_close                                           **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wifi accelerator - close                                             **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   The function closes all the driver resources.                        **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static void wlan_accelerator_dev_close(void)
{
    int rx_queue_index;

    /* Disable the interrupt */
    for (rx_queue_index=0; rx_queue_index<number_of_queues; rx_queue_index++)
    {
        //int queue_id = rx_queue_index + first_pci_queue;
        /* Configure PCI RX queue */
        _rdpa_cfg_cpu_rx_queue(rx_queue_index, 0, NULL);
      
        rdpa_cpu_int_disable (rdpa_cpu_wlan0, rx_queue_index); 
    }
     
    /* Release the OS driver resources */
    release_wlan_accelerator_os_resources();

    remove_proc_entry("conf", proc_directory);

    remove_proc_entry("wifi_mw", NULL);

    /*Unregister for NETDEV_REGISTER and NEtdEV_UNREGISTER for wifi driver*/
    unregister_netdevice_notifier (&wifi_netdev_notifer) ;

    /*Free PCI resources*/
    release_wlan_accelerator_interfaces();

    bdmf_destroy(rdpa_cpu_obj); 
}


/****************************************************************************/
/**                                                                        **/
/** Name:                                                                  **/
/**                                                                        **/
/**   wlan_accelerator_dev_init                                            **/
/**                                                                        **/
/** Title:                                                                 **/
/**                                                                        **/
/**   wifi accelerator - init                                              **/
/**                                                                        **/
/** Abstract:                                                              **/
/**                                                                        **/
/**   The function initialize all the driver resources.                    **/
/**                                                                        **/
/** Input:                                                                 **/
/**                                                                        **/
/** Output:                                                                **/
/**                                                                        **/
/**                                                                        **/
/****************************************************************************/
static int wlan_accelerator_dev_init(void)
{
    int error ;
    char wifi_if_name[WIFI_IF_NAME_STR_LEN] ;
    int wifi_dev_index;
    BDMF_MATTR(cpu_wlan0_attrs, rdpa_cpu_drv());

    wifi_prefix_len = strlen( wifi_prefix ) ;

    /* create cpu */
    rdpa_cpu_index_set(cpu_wlan0_attrs, rdpa_cpu_wlan0);
    if (bdmf_new_and_set(rdpa_cpu_drv(), NULL, cpu_wlan0_attrs, &rdpa_cpu_obj))
    {
        printk("%s %s Failed to create cpu wlan0 object\n",__FILE__, __FUNCTION__);    
        return -1;
    }
     


    /* Initialize the proc interface for debugging information */
    if (wifi_proc_init()!=0)
    {
        printk("\n%s %s: wifi_proc_init() failed \n", __FILE__, __FUNCTION__) ;
        goto cpu_release;
    }

    /* Initialize the middleware */
    if( init_wifi_drv_resources()!=0)
    {
        printk("%s %s: init_wifi_drv_resources() failed \n", __FILE__, __FUNCTION__) ;
        goto proc_release;
    }

    for (wifi_dev_index=0; wifi_dev_index<WIFI_MW_MAX_NUM_IF; wifi_dev_index++)
    {
        if (wifi_dev_index % 8 )
            error = (wifi_dev_index > 8) ?
                snprintf(wifi_if_name, WIFI_IF_NAME_STR_LEN, "%s1.%u", wifi_prefix, wifi_dev_index-7) :
                snprintf(wifi_if_name, WIFI_IF_NAME_STR_LEN, "%s0.%u", wifi_prefix, wifi_dev_index) ;
        else
            error = (wifi_dev_index == 0) ?
                snprintf(wifi_if_name, WIFI_IF_NAME_STR_LEN, "%s0", wifi_prefix) :
                snprintf(wifi_if_name, WIFI_IF_NAME_STR_LEN, "%s1", wifi_prefix) ;

        if (error == -1)
        {
            printk("%s %s: wifi interface name retrieval failed \n", __FILE__, __FUNCTION__) ;
            goto error_handling;
        }

        if (!wifi_net_devices[wifi_dev_index])
        {
           wifi_net_devices[wifi_dev_index] = dev_get_by_name(&init_net, wifi_if_name) ;
            if (wifi_net_devices[wifi_dev_index])
                rdpa_port_ssid_update(wifi_dev_index, 1);
        }
    }

    /*Register for NETDEV_REGISTER and NETDEV_UNREGISTER for wifi driver*/
    register_netdevice_notifier(&wifi_netdev_notifer);

    printk ("\033[1m\033[34m %s initialized !\n\r", version);
    printk("timer = %d, threshold = %d number_of_packets_to_read = %d fist pci queue %d number_of_queues = %d\n\033[0m",
        timer_timeout, packet_threshold, num_packets_to_read, first_pci_queue, number_of_queues);
    
    return 0;

error_handling:
    unregister_netdevice_notifier ( & wifi_netdev_notifer ) ;
    release_wlan_accelerator_interfaces ( ) ;

proc_release:
    remove_proc_entry("conf", proc_directory);
    remove_proc_entry("wifi_mw", NULL);     

cpu_release:
    bdmf_put(rdpa_cpu_obj);

    return -1;
}

MODULE_DESCRIPTION("WLAN Forwarding Driver");
MODULE_AUTHOR("Broadcom");
MODULE_LICENSE("GPL");

module_init(wlan_accelerator_dev_init);
module_exit(wlan_accelerator_dev_close);

