#if defined(CONFIG_BCM_KF_MIPS_BCM9685XX)
#ifndef _LILAC_ETH_H_
#define _LILAC_ETH_H_

#include <linux/etherdevice.h>
#include <linux/ethtool.h>
#include <linux/delay.h>
#include "bl_lilac_drv_mac.h"
#include "bl_lilac_eth_common.h"


struct bl_lilac_netdev 
{
    /* Used on datapath by lilac_net_dev.c - should be grouped together in
     * order to have it in dcache. */
    struct net_device*		eth;
    int 					macId; 
    int 					(*send_packet) (struct sk_buff *skb, struct bl_lilac_netdev *lilac_dev);

    /* Not used on datapath by lilac_net_dev.c - should be in the middle in
     * order to avoid many fetching into dcache. */
    int 					phyAddr;
    DRV_MAC_PHY_RATE	phyRate;
    struct timer_list		poll_timer;

    /* Not used by lilac_net_dev.c - should be last in order to avoid fetching
     * into dcache. */
    spinlock_t 				lock;
    struct net_device_stats	stats;
    u32 					msg_enable;
    ETH_MODE				port_emac_mode;
    char					name[16];

};

typedef struct bl_lilac_netdev	bl_lilac_netdev;

#endif /* _LILAC_ETH_H_ */
#endif /* CONFIG_BCM_KF_MIPS_BCM9685XX */

