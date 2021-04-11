/*
 Copyright 2007-2010 Broadcom Corp. All Rights Reserved.

 <:label-BRCM:2011:DUAL/GPL:standard
 
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
#if defined(CONFIG_BCM_KF_MIPS_BCM9685XX)
#include "bl_os_wraper.h"
#include "lilac_eth.h"
#include "bl_lilac_rdd.h"
#include "shared_utils.h" 

int kerSysGetMacAddress( unsigned char *pucaMacAddr, unsigned long ulId );
int kerSysReleaseMacAddress( unsigned char *pucaMacAddr );

void bl_lilac_put_all_data_devs_to_reset(int emac_id);
LILAC_BDPI_ERRORS fi_basic_data_path_init( DRV_MAC_NUMBER xi_emac_id, ETH_MODE xi_emacs_group_mode, ETH_MODE xi_emac4_mode, int do_rst );

static void bl_device_release(struct device *dev)
{
}
/* Platform devices list */
struct platform_device lilac_eth_platform_device = 
{
	.name			= "bl_lilac_emac",
	.id				= 0,
	.num_resources	= 0,
	.dev.release    = bl_device_release
};

static 	bl_lilac_netdev*	net_devs;
static const char *version = "Lilac Enhanced Network Driver\n";

#define nodprintk(args...) 				/* */
#define RX_POLLING_INTERVAL  			1

#define BL234X_ETH_DRV_USE_HW_TIMER

/* poll link ststus every 500 mSec */
#ifndef BL234X_ETH_DRV_USE_HW_TIMER
	#define BL234X_ETH_DRV_AN_PERIOD (HZ/2)
#else
	#include "bl_lilac_timer.h"
	#define BL234X_ETH_DRV_HW_TIMER_TYME	2000 /* micro second */
	#define BL234X_ETH_DRV_AN_PERIOD ((1000000/(BL234X_ETH_DRV_HW_TIMER_TYME))/2)
	static struct tasklet_struct bl24x_net_tasklet ;
	static BL_RTTMR_HANDLE timer_handle = NULL;
	static void net_dev_timer_callback(void * prm)
	{
	    /* send event to tasklet */
	    tasklet_schedule(&bl24x_net_tasklet);
	}
#endif


static int lilac_get_mgm_port(int* mac_id, ETH_MODE* group_mode, ETH_MODE* emac_mode)
{

	int	port_emac_mode		= -1;

	ETHERNET_MAC_INFO enetInfo[DRV_MAC_NUMBER_OF_EMACS-1];
	unsigned long mac_type;
	unsigned long port_flags;
	int i;
	
    *mac_id		= DRV_MAC_EMAC_0;
	*emac_mode	= ETH_MODE_NONE;
	*group_mode	= ETH_MODE_NONE;
	
	if(BpGetEthernetMacInfo(enetInfo, DRV_MAC_NUMBER_OF_EMACS-1) != BP_SUCCESS)
		return -1;
	
	Get968500EmacMode(enetInfo, 0, &mac_type, &port_flags);
	switch(mac_type)
	{
		case MAC_IF_QSGMII:
			*group_mode = ETH_MODE_QSGMII;
			break;
		case MAC_IF_SGMII:
			*group_mode = ETH_MODE_SGMII;
			break;
		default:
			Get968500EmacMode(enetInfo, 1, &mac_type, &port_flags);
			if(mac_type == MAC_IF_SS_SMII)
				*group_mode = ETH_MODE_SMII;
			else
				printk("group mode is undefined, may be it is HISGMII\n");
			break;
	}
	
	if(*group_mode == MAC_IF_SS_SMII)
	{
		Get968500EmacMode(enetInfo, 0, &mac_type, &port_flags);
		if(mac_type == MAC_IF_SGMII)
			*emac_mode = ETH_MODE_SGMII;
	}
	else
	{
		Get968500EmacMode(enetInfo, 4, &mac_type, &port_flags);
		switch(mac_type)
		{
			case MAC_IF_RGMII:
				*emac_mode = E4_MODE_RGMII;
				break;
				
			case  MAC_IF_TMII:
				*emac_mode = E4_MODE_TMII;
				break;
		}
	}
	 
	for(i=0; i<DRV_MAC_NUMBER_OF_EMACS-1; i++)
	{
		Get968500EmacMode(enetInfo, i, &mac_type, &port_flags);
		if(IsPortMgmt(port_flags))
		{
			*mac_id = i;
			break;
		}
	}
    if (i == DRV_MAC_NUMBER_OF_EMACS-1) {
        printk("no mgmt port defined assuming emac0\n");
    }
	
    port_emac_mode = bl_lilac_check_mgm_port_cfg(*mac_id, *group_mode, *emac_mode);
	
	if(port_emac_mode < 0)
	{
		printk("Illegal  EMAC#/Mode combination, Please check Environment parameters!\n");
	}
	else
	{
		printk("MAC configurations: EMAC%d, mode = %s\n", *mac_id, EMAC_MODE_NAME[port_emac_mode]);
		printk("MAC configurations: emacs_group_mode = %s, emac_mode = %s\n",EMAC_MODE_NAME[*group_mode], EMAC_MODE_NAME[*emac_mode]);
		
	}

    return port_emac_mode;

}

/*==============================================================*/
void rx_poll_handler(unsigned long netdev)
{
	bl_lilac_netdev*	bl_dev	=  (bl_lilac_netdev *)netdev;
	struct net_device*	eth		= (struct net_device *)net_devs->eth;
	uint32_t 			len 	= 0;
	uint32_t 			flowId 	= 0;
	struct sk_buff*		skb = NULL;
	RDD_CPU_RX_REASON	reason;
    RDD_BRIDGE_PORT	    src_bridge_port;
    int i;

	for(i = 0; i < 32; i++)
	{
		if(!skb)
		{
			skb = dev_alloc_skb(1600); 
			if(!skb)
				return; 

			skb_reserve(skb, 4 - (((unsigned long)skb->data) & 0x3) );
		}

		len = 0;
		/* call runner driver to receive the packet - driver will copy the data */
       	if(bl_rdd_cpu_rx_read_packet(RDD_CPU_RX_QUEUE_0, (uint8_t*)skb->data, &len, &src_bridge_port, &flowId, &reason)!= RDD_OK)
   		{
	       	/* nothing new for the NET task */
			dev_kfree_skb_irq(skb);
			return;
		}
		
		if(!len)
		{
	   		printk("%s ERROR: Got packet with zero length \n", __FUNCTION__);
			return;
		}
		
		skb->dev = eth;
		skb_put(skb, len);
		skb->protocol = eth_type_trans(skb, eth);
		
		bl_dev->stats.rx_packets++;
		bl_dev->stats.rx_bytes += len;
		netif_rx(skb);
		
		skb = NULL;
	}
}

void bl_lilac_rx_timer(unsigned long dev)
{
	static int count = 0;
	bl_lilac_netdev*	bl_dev = (bl_lilac_netdev *)dev;
	
#ifndef BL234X_ETH_DRV_USE_HW_TIMER
	mod_timer(&bl_dev->poll_timer, jiffies + RX_POLLING_INTERVAL);
#endif

	++count;

	rx_poll_handler(dev);
	
	if(count >= BL234X_ETH_DRV_AN_PERIOD)
	{
		bl_dev->phyRate = bl_lilac_auto_mac(bl_dev->phyAddr, bl_dev->macId,  bl_dev->port_emac_mode, bl_dev->phyRate);
		count = 0;
	}
}

 int bl_lilac_eth_open(struct net_device *eth)
{
#ifndef BL234X_ETH_DRV_USE_HW_TIMER
	bl_lilac_netdev *bl_dev	= (bl_lilac_netdev *)netdev_priv(eth);
	printk("%s: OPEN port %u\n", eth->name, bl_dev->macId );        
	netif_start_queue(eth);

	init_timer(&bl_dev->poll_timer);
	bl_dev->poll_timer.function	= bl_lilac_rx_timer;
	bl_dev->poll_timer.data		= (unsigned long)bl_dev;
	mod_timer(&bl_dev->poll_timer, jiffies + RX_POLLING_INTERVAL);
#else
	bl_rttmr_register(net_dev_timer_callback, NULL, NULL, BL234X_ETH_DRV_HW_TIMER_TYME, BL_RTTMR_MODE_CYCLE, &timer_handle, 0);
#endif

	return 0;
	
}

static int bl_lilac_eth_hard_start_xmit(struct sk_buff *skb, struct net_device *eth)
{
    unsigned char *pBuf = (unsigned char*)skb->data;
	unsigned long flags;
	RDD_ERROR errorCode =0;

	bl_lilac_netdev *dev = (bl_lilac_netdev *)netdev_priv(eth);

	nodprintk("TX: %08X %08X %08X  %08X %d\n", *(unsigned *)(skb->data),
          *(unsigned *)(skb->data + 4), *(unsigned *)(skb->data + 8),*(unsigned *)(skb->data + 12),
          skb->len);

   	spin_lock_irqsave(&dev->lock, flags);


    /* add padding to get 64 bytes frame. padding is zeroes up to 60 bytes */
	if (skb->len < 60)
	{
		memset((pBuf + skb->len), 0, 60 - skb->len);
		skb->len = 60;
	}

	/* call runner driver to send the packet - driver will copy the data */
	errorCode = bl_rdd_cpu_tx_write_eth_packet((uint8_t *)skb->data, skb->len,
				RDD_LAN0_BRIDGE_PORT + dev->macId, RDD_QUEUE_0);

	/* call to Tx can fail only if Tx busy (unlikely) or the Runner is not running */
	if (errorCode != RDD_OK)
	{
		spin_unlock_irqrestore(&dev->lock, flags);
		printk("%s: attempt to store skb in busy slot\n", __FUNCTION__);
		dev_kfree_skb_irq(skb);
		return 0;
	}
	
	eth->trans_start = jiffies;
	dev->stats.tx_packets++;
	dev->stats.tx_bytes += skb->len;

	dev_kfree_skb_irq(skb);
	spin_unlock_irqrestore(&dev->lock, flags);
	
	if (netif_queue_stopped(eth))
    {
		printk("eth: restart queue\n");
       	netif_wake_queue(eth);
    }

	return 0;
}


static int bl_lilac_eth_stop(struct net_device *eth)
{
#ifndef BL234X_ETH_DRV_USE_HW_TIMER
	bl_lilac_netdev *bl_dev = (bl_lilac_netdev *)netdev_priv(eth);
	int macId = bl_dev->macId;
	printk("%s: STOP port = %u\n", eth->name, macId);
	netif_stop_queue(eth);
	del_timer_sync(&bl_dev->poll_timer);
#else
	if(timer_handle)
		bl_rttmr_unregister(timer_handle);
	timer_handle = NULL;
#endif
	return 0;
}

static struct net_device_stats *bl_lilac_eth_get_stats(struct net_device *eth)
{
	bl_lilac_netdev *bl_dev = (bl_lilac_netdev *)netdev_priv(eth);
	return &bl_dev->stats;
}

static int netdev_ethtool_ioctl(struct net_device *eth, void *useraddr)
{
	bl_lilac_netdev *dev = (bl_lilac_netdev *)netdev_priv(eth);
	u32 ethcmd;
	if (copy_from_user(&ethcmd, useraddr, sizeof(ethcmd)))
		 return -EFAULT;
	
	switch(ethcmd)
	{
      /* get driver-specific version/etc. info */
		case ETHTOOL_GDRVINFO:
		{
         	struct ethtool_drvinfo info = {ETHTOOL_GDRVINFO};
         	strncpy(info.driver, "EMAC", sizeof(info.driver)-1);
         	strncpy(info.version, version, sizeof(info.version)-1);
         	if (copy_to_user(useraddr, &info, sizeof(info)))
            		return -EFAULT;
         	return 0;
		}
         /* get settings */
		case ETHTOOL_GSET:
		{
     		struct ethtool_cmd ecmd = { ETHTOOL_GSET };
     		if (copy_to_user(useraddr, &ecmd, sizeof(ecmd)))
				return -EFAULT;
     		return 0;
		}
         /* set settings */
		case ETHTOOL_SSET:
		{
     		struct ethtool_cmd ecmd;
     		if (copy_from_user(&ecmd, useraddr, sizeof(ecmd)))
				return -EFAULT;
     		return 0;
		}
         /* get link status */
		case ETHTOOL_GLINK:
		{
     		struct ethtool_value edata = {ETHTOOL_GLINK};
     		if (copy_to_user(useraddr, &edata, sizeof(edata)))
				return -EFAULT;
     		return 0;
		}
         /* get message-level */
		case ETHTOOL_GMSGLVL:
		{
     		struct ethtool_value edata = {ETHTOOL_GMSGLVL};
     		edata.data = dev->msg_enable;
     		if (copy_to_user(useraddr, &edata, sizeof(edata)))
				return -EFAULT;
     		return 0;
		}
         /* set message-level */
		case ETHTOOL_SMSGLVL:
		{
     		struct ethtool_value edata;
     		if (copy_from_user(&edata, useraddr, sizeof(edata)))
				return -EFAULT;
     		dev->msg_enable = edata.data;
         	return 0;
		}
	     default:
     		return -ENODEV;
	}
	
	return -EFAULT;
}

static int bl_lilac_eth_do_ioctl(struct net_device *eth, struct ifreq *req, int cmd)
{
	switch (cmd)
	{
		case SIOCETHTOOL:
			return  netdev_ethtool_ioctl(eth, (void *) req->ifr_data);

		default:
			return -EOPNOTSUPP;
	}
	return -EOPNOTSUPP;
	
}


static const struct net_device_ops netdev_ops = {
        .ndo_open               = bl_lilac_eth_open,
        .ndo_stop               = bl_lilac_eth_stop,
        .ndo_get_stats          = bl_lilac_eth_get_stats,
        .ndo_start_xmit         = bl_lilac_eth_hard_start_xmit,
        .ndo_do_ioctl           = bl_lilac_eth_do_ioctl,
};

//=========================================================================================
//=========================================================================================
int bl_lilac_eth_probe(struct platform_device *pdev)
{
	int					rc = 0;
	unsigned char		ethaddr[6];
	bl_lilac_netdev*	bl_dev;
	struct net_device*	eth;
    ETH_MODE			emac_mode;
    ETH_MODE			group_mode;
	RDD_ERROR			errorCode = RDD_OK;

	eth = alloc_etherdev(sizeof(bl_lilac_netdev));
	if (eth == NULL)
	{
	   	return -ENOMEM;
	}

	SET_NETDEV_DEV(eth, &pdev->dev);
	platform_set_drvdata(pdev, eth);

	bl_dev		= netdev_priv(eth);
	net_devs	= bl_dev;
	bl_dev->eth = eth;

	bl_dev->port_emac_mode = lilac_get_mgm_port(&bl_dev->macId, &group_mode, &emac_mode);
	if(bl_dev->port_emac_mode < 0) 
	{
        printk("Data Path Initialization Error - wrong EMAC mode\n");
		return -1;
	}
	spin_lock_init(&bl_dev->lock);

	if (!kerSysGetMacAddress(ethaddr,eth->ifindex))
	{
		/* Copy the station address into the dev structure, */
		memcpy(eth->dev_addr, ethaddr, 6 );
	}
	else
	{
		ether_setup(eth);
	}
	
    eth->netdev_ops         = &netdev_ops;
	bl_dev->msg_enable 		= NETIF_MSG_LINK;
	bl_dev->phyAddr 		= BpGetPhyAddr(0,bl_dev->macId);
	bl_dev->phyRate 		= DRV_MAC_PHY_RATE_LINK_DOWN;
	sprintf(bl_dev->name, "ETH on MAC%d", bl_dev->macId);

	printk("BL lilac ethernet driver initialization: group_mode %s  emac_mode %s macId EMAC%d\n",EMAC_MODE_NAME[group_mode], EMAC_MODE_NAME[emac_mode], bl_dev->macId); 

    errorCode =  fi_basic_data_path_init(bl_dev->macId, group_mode, emac_mode, 1);
    if(errorCode != (RDD_ERROR)BDPI_NO_ERROR)
	{
  		printk("Data Path Initialization Error (error code: %d) \n",(int)errorCode);
		return -1;
	}
	
	printk("Data Path successfuly initialized\n");
	
	bl_lilac_phy_auto_enable(bl_dev->phyAddr);
	
	bl_dev->phyRate = bl_lilac_auto_mac(bl_dev->phyAddr, bl_dev->macId, bl_dev->port_emac_mode, bl_dev->phyRate);
	
	rc = register_netdev(eth);
	if(rc)
	{
		printk("register_netdev %s failed \n",  bl_dev->name);
		free_netdev(eth);
	}
	
#ifdef BL234X_ETH_DRV_USE_HW_TIMER
		tasklet_init( &bl24x_net_tasklet, bl_lilac_rx_timer, (unsigned long)bl_dev);
#endif

	return rc;
}


static int bl_lilac_eth_remove(struct platform_device *device)
{
#ifdef BL234X_ETH_DRV_USE_HW_TIMER
   tasklet_kill(&bl24x_net_tasklet) ;
#endif
	return 0;
}


MODULE_ALIAS("platform:bl_lilac_emac");

/* Structure for a device driver */
static struct platform_driver bl_lilac_eth_driver = {
	.probe  = bl_lilac_eth_probe,
	.remove = bl_lilac_eth_remove,
	.driver	= {
		.name	= "bl_lilac_emac",
		.owner	= THIS_MODULE,
	},
};

static int __init bl_lilac_eth_init_module(void)
{
	int rc = 0;

	rc = platform_driver_register(&bl_lilac_eth_driver);
	if (rc)
	{
		printk("lilac_eth: driver registration failed with rc=%d\n", rc);
		return rc;
    }

	rc = platform_device_register(&lilac_eth_platform_device);
	if (rc)
	{
		printk("lilac_eth: unable to register platform device rc=%d\n", rc);
        platform_driver_unregister(&bl_lilac_eth_driver);
	}
	printk("lilac_eth: successfuly added\n");
	return rc;
}


static void __exit bl_lilac_eth_cleanup_module(void)
{
	struct net_device *eth = net_devs->eth;
	bl_lilac_eth_stop(eth);
	mdelay(300);
	kerSysReleaseMacAddress(eth->dev_addr);
	bl_lilac_put_all_data_devs_to_reset(net_devs->macId);
	unregister_netdev(eth);
	free_netdev(eth);
	bl_lilac_eth_remove(&lilac_eth_platform_device);
	platform_driver_unregister(&bl_lilac_eth_driver);
	platform_device_unregister(&lilac_eth_platform_device);
	printk("lilac_eth: successfuly removed\n");
}

module_init(bl_lilac_eth_init_module);
module_exit(bl_lilac_eth_cleanup_module);
MODULE_LICENSE("GPL");

#endif /* CONFIG_BCM_KF_MIPS_BCM9685XX */

