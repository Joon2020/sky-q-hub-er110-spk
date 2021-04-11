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

#define _BCMENET_LOCAL_

#include <linux/types.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/stddef.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <board.h>
#include "boardparms.h"
#include <bcm_map_part.h>
#include "bcm_intr.h"
#include "bcmenet.h"
#include "bcmmii.h"
#ifdef CONFIG_BCM968500
#include "bl_lilac_brd.h"
#include "bl_lilac_eth_common.h"
#else
#include "phys_common_drv.h"
#endif
#include <rdpa_api.h>
#include "bcmsw_runner.h"

#ifdef CONFIG_BCM96838
PHY_STAT ethsw_phy_stat(int port)
{
	PHY_STAT phys;
    PHY_RATE curr_phy_rate;
    BcmEnet_devctrl *pDevCtrl = netdev_priv(vnet_dev[port + 1]);

    curr_phy_rate = PhyGetLineRateAndDuplex(pDevCtrl->sw_port_id);

	memset(&phys, 0, sizeof(phys));
	phys.lnk = curr_phy_rate < PHY_RATE_LINK_DOWN;
	switch (curr_phy_rate)
	{
	case PHY_RATE_10_FULL:
		phys.fdx = 1;
		break;
	case PHY_RATE_10_HALF:
		break;
	case PHY_RATE_100_FULL:
		phys.fdx = 1;
		phys.spd100 = 1;
		break;
	case PHY_RATE_100_HALF:
		phys.spd100 = 1;
		break;
	case PHY_RATE_1000_FULL:
		phys.spd1000 = 1;
		phys.fdx = 1;
		break;
	case PHY_RATE_1000_HALF:
		phys.spd1000 = 1;
		break;
	case PHY_RATE_LINK_DOWN:
		break;
	default:
		break;
	}
	  return phys;
}
#endif
#ifdef CONFIG_BCM968500
PHY_STAT ethsw_phy_stat(int port)
{
    PHY_STAT phys;
    DRV_MAC_PHY_RATE curr_phy_rate;
    BcmEnet_devctrl *pDevCtrl = netdev_priv(vnet_dev[port + 1]);




    curr_phy_rate = bl_lilac_read_phy_line_status(pDevCtrl->phy_addr);

    memset(&phys, 0, sizeof(phys));
    phys.lnk = curr_phy_rate < DRV_MAC_PHY_RATE_LINK_DOWN;
    switch (curr_phy_rate)
    {
    case DRV_MAC_PHY_RATE_10_FULL:
        phys.fdx = 1;
        break;
    case DRV_MAC_PHY_RATE_10_HALF:
        break;
    case DRV_MAC_PHY_RATE_100_FULL:
        phys.fdx = 1;
        phys.spd100 = 1;
        break;
    case DRV_MAC_PHY_RATE_100_HALF:
        phys.spd100 = 1;
        break;
    case DRV_MAC_PHY_RATE_1000_FULL:
        phys.spd1000 = 1;
        phys.fdx = 1;
        break;
    case DRV_MAC_PHY_RATE_1000_HALF:
        phys.spd1000 = 1;
        break;
    case DRV_MAC_PHY_RATE_LINK_DOWN:
        break;
    default:
        break;
    }


    return phys;
}
#endif
void bcmeapi_ethsw_init_config(void)
{
}

int ethsw_reset_ports(struct net_device *dev)
{
    return 0;
}

int ethsw_add_proc_files(struct net_device *dev)
{
    return 0;
}

int ethsw_del_proc_files(void)
{
    return 0;
}

int ethsw_disable_hw_switching(void)
{
    return 0;
}

void ethsw_switch_manage_port_power_mode(int portnumber, int power_mode)
{
}
EXPORT_SYMBOL(ethsw_switch_manage_port_power_mode);

int ethsw_switch_get_port_power_mode(int portnumber)
{
    return 0;
}
EXPORT_SYMBOL(ethsw_switch_get_port_power_mode);

void ethsw_eee_port_enable(int port, int enable)
{
}

void ethsw_eee_init(void)
{
}

void ethsw_eee_process_delayed_enable_requests(void)
{
}

void ethsw_dump_page(int page)
{
}

void fast_age_port(uint8_t port, uint8_t age_static)
{
}

int ethsw_counter_collect(uint32_t portmap, int discard)
{
    return 0;
}

void ethsw_port_based_vlan(int port_map, int wan_port_map, int txSoftSwitchingMap)
{
}

void ethsw_get_txrx_imp_port_pkts(unsigned int *tx, unsigned int *rx)
{
}
EXPORT_SYMBOL(ethsw_get_txrx_imp_port_pkts);

int ethsw_phy_intr_ctrl(int port, int on)
{
    return 0;
}
void ethsw_phy_apply_init_bp(void)
{
}
int ethsw_setup_led(void)
{
    return 0;
}
int ethsw_setup_phys(void)
{
    return 0;
}
int ethsw_enable_hw_switching(void)
{
    return 0;
}

void ethsw_phy_advertise_caps(void)
{
}

int bcmeapi_ethsw_dump_mib(int port, int type)
{
	int					rc;
	rdpa_emac_stat_t 	emac_cntrs;
	bdmf_object_handle 	port_obj;

    if (port + rdpa_emac0  > rdpa_emac4) {  /* Note : Trying to get stats for Port#5 locks up */
        printk("Invalid Runner port number <%d>\n",port);
        return -1;
    }
    rc = rdpa_port_get(port + rdpa_emac0,&port_obj);
    rc = rc ? rc : rdpa_port_emac_stat_get (port_obj, &emac_cntrs);
    if ( rc != BDMF_ERR_OK)
    {
    	printk("failed to get statistic counters from RDPA layer\n");
    	bdmf_put(port_obj);
    	return -1;
    }
    bdmf_put(port_obj);
    printk("\nRunner Stats : Port# %d\n",port);

    /* Display Tx statistics */
    printk("\n");
    printk("TxUnicastPkts:          %10u \n", (unsigned int)emac_cntrs.tx.packet);
    printk("TxMulticastPkts:        %10u \n", (unsigned int)emac_cntrs.tx.multicast_packet);
    printk("TxBroadcastPkts:        %10u \n", (unsigned int)emac_cntrs.tx.broadcast_packet);
    printk("TxDropPkts:             %10u \n", (unsigned int)emac_cntrs.tx.error);

    /* Display remaining tx stats only if requested */
    if (type) {
        printk("TxBytes:                %10u \n", (unsigned int)emac_cntrs.tx.byte);
        printk("TxFragments:            %10u \n", (unsigned int)emac_cntrs.tx.fragments_frame);
        printk("TxCol:                  %10u \n", (unsigned int)emac_cntrs.tx.total_collision);
        printk("TxSingleCol:            %10u \n", (unsigned int)emac_cntrs.tx.single_collision);
        printk("TxMultipleCol:          %10u \n", (unsigned int)emac_cntrs.tx.multiple_collision);
        printk("TxDeferredTx:           %10u \n", (unsigned int)emac_cntrs.tx.deferral_packet);
        printk("TxLateCol:              %10u \n", (unsigned int)emac_cntrs.tx.late_collision);
        printk("TxExcessiveCol:         %10u \n", (unsigned int)emac_cntrs.tx.excessive_collision);
        printk("TxPausePkts:            %10u \n", (unsigned int)emac_cntrs.tx.pause_control_frame);
        printk("TxExcessivePkts:        %10u \n", (unsigned int)emac_cntrs.tx.excessive_deferral_packet);
        printk("TxJabberFrames:         %10u \n", (unsigned int)emac_cntrs.tx.jabber_frame);
        printk("TxFcsError:             %10u \n", (unsigned int)emac_cntrs.tx.fcs_error);
        printk("TxCtrlFrames:           %10u \n", (unsigned int)emac_cntrs.tx.control_frame);
        printk("TxOverSzFrames:         %10u \n", (unsigned int)emac_cntrs.tx.oversize_frame);
        printk("TxUnderSzFrames:        %10u \n", (unsigned int)emac_cntrs.tx.undersize_frame);
        printk("TxUnderrun:             %10u \n", (unsigned int)emac_cntrs.tx.underrun);
        printk("TxPkts64Octets:         %10u \n", (unsigned int)emac_cntrs.tx.frame_64);
        printk("TxPkts65to127Octets:    %10u \n", (unsigned int)emac_cntrs.tx.frame_65_127);
        printk("TxPkts128to255Octets:   %10u \n", (unsigned int)emac_cntrs.tx.frame_128_255);
        printk("TxPkts256to511Octets:   %10u \n", (unsigned int)emac_cntrs.tx.frame_256_511);
        printk("TxPkts512to1023Octets:  %10u \n", (unsigned int)emac_cntrs.tx.frame_512_1023);
        printk("TxPkts1024to1518Octets: %10u \n", (unsigned int)emac_cntrs.tx.frame_1024_1518);
        printk("TxPkts1519toMTUOctets:  %10u \n", (unsigned int)emac_cntrs.tx.frame_1519_mtu);
    }

    /* Display Rx statistics */
    printk("\n");
    printk("RxUnicastPkts:          %10u \n", (unsigned int)emac_cntrs.rx.packet);
    printk("RxMulticastPkts:        %10u \n", (unsigned int)emac_cntrs.rx.multicast_packet);
    printk("RxBroadcastPkts:        %10u \n", (unsigned int)emac_cntrs.rx.broadcast_packet);

    /* Display remaining rx stats only if requested */
    if (type) {
        printk("RxBytes:                %10u \n", (unsigned int)emac_cntrs.rx.byte);
        printk("RxJabbers:              %10u \n", (unsigned int)emac_cntrs.rx.jabber);
        printk("RxAlignErrs:            %10u \n", (unsigned int)emac_cntrs.rx.alignment_error);
        printk("RxFCSErrs:              %10u \n", (unsigned int)emac_cntrs.rx.fcs_error);
        printk("RxFragments:            %10u \n", (unsigned int)emac_cntrs.rx.fragments);
        printk("RxOversizePkts:         %10u \n", (unsigned int)emac_cntrs.rx.oversize_packet);
        printk("RxUndersizePkts:        %10u \n", (unsigned int)emac_cntrs.rx.undersize_packet);
        printk("RxPausePkts:            %10u \n", (unsigned int)emac_cntrs.rx.pause_control_frame);
        printk("RxOverflow:             %10u \n", (unsigned int)emac_cntrs.rx.overflow);
        printk("RxCtrlPkts:             %10u \n", (unsigned int)emac_cntrs.rx.control_frame);
        printk("RxUnknownOp:            %10u \n", (unsigned int)emac_cntrs.rx.unknown_opcode);
        printk("RxLenError:             %10u \n", (unsigned int)emac_cntrs.rx.frame_length_error);
        printk("RxCodeError:            %10u \n", (unsigned int)emac_cntrs.rx.code_error);
        printk("RxCarrierSenseErr:      %10u \n", (unsigned int)emac_cntrs.rx.carrier_sense_error);
        printk("RxPkts64Octets:         %10u \n", (unsigned int)emac_cntrs.rx.frame_64);
        printk("RxPkts65to127Octets:    %10u \n", (unsigned int)emac_cntrs.rx.frame_65_127);
        printk("RxPkts128to255Octets:   %10u \n", (unsigned int)emac_cntrs.rx.frame_128_255);
        printk("RxPkts256to511Octets:   %10u \n", (unsigned int)emac_cntrs.rx.frame_256_511);
        printk("RxPkts512to1023Octets:  %10u \n", (unsigned int)emac_cntrs.rx.frame_512_1023);
        printk("RxPkts1024to1522Octets: %10u \n", (unsigned int)emac_cntrs.rx.frame_1024_1518);
        printk("RxPkts1523toMTU:        %10u \n", (unsigned int)emac_cntrs.rx.frame_1519_mtu);
    }
    return 0;
}

#if defined(CONFIG_BCM_ETH_PWRSAVE) && defined(CONFIG_BCM96838)
unsigned int  eth_auto_power_down_enabled = 1;

#define DEFAULT_BASE_PHYID 1
#define NUM_INT_GPHYS 4
#define CORE_SHD1C_0A 0x1a

void ethsw_setup_hw_apd(unsigned int enable)
{
	int i;
    uint16 v16;
	
    for (i = DEFAULT_BASE_PHYID; i < NUM_INT_GPHYS +DEFAULT_BASE_PHYID; i++) {
        v16 = MII_1C_WRITE_ENABLE | MII_1C_AUTO_POWER_DOWN_SV |
              MII_1C_WAKEUP_TIMER_SEL_84 | MII_1C_APD_COMPATIBILITY;
        if (enable) {
            v16 |= MII_1C_AUTO_POWER_DOWN;
        }
		egPhyWriteRegister(i, RDB_REG_ACCESS|CORE_SHD1C_0A, v16);
    }

}

void BcmPwrMngtSetEthAutoPwrDwn(unsigned int enable)
{
   eth_auto_power_down_enabled = enable;
   ethsw_setup_hw_apd(enable);

   printk("Ethernet Auto Power Down and Sleep is %s\n", enable?"enabled":"disabled");
}
EXPORT_SYMBOL(BcmPwrMngtSetEthAutoPwrDwn);

int BcmPwrMngtGetEthAutoPwrDwn(void)
{
   return (eth_auto_power_down_enabled);
}
EXPORT_SYMBOL(BcmPwrMngtGetEthAutoPwrDwn);

void BcmPwrMngtSetEnergyEfficientEthernetEn(unsigned int enable)
{
   printk("Energy Efficient Ethernet is disabled\n");
}
EXPORT_SYMBOL(BcmPwrMngtSetEnergyEfficientEthernetEn);

int BcmPwrMngtGetEnergyEfficientEthernetEn(void)
{
   return 0;
}
EXPORT_SYMBOL(BcmPwrMngtGetEnergyEfficientEthernetEn);

int ethsw_phy_pll_up(int ephy_and_gphy)
{
	return 0;
}

uint32 ethsw_ephy_auto_power_down_wakeup(void)
{
	return 0;
}

uint32 ethsw_ephy_auto_power_down_sleep(void)
{
	return 0;
}
#endif

