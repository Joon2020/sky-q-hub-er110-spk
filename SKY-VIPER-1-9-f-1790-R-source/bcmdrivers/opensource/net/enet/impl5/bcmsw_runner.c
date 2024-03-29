/*
    Copyright 2000-2010 Broadcom Corporation

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
#include "ethsw.h"
#include "bcmmii.h"
#include "bcmsw_runner.h"
#ifdef CONFIG_BCM968500
#include "bl_lilac_brd.h"
#endif
#include "bcmswshared.h"
#include <rdpa_api.h>
#include <rdpa_types.h>

extern spinlock_t bcm_extsw_access;
extern struct net_device* vnet_dev[];
static rdpa_emac_stat_t cached_rdpa_emac_stat[rdpa_emac__num_of];

void bcmsw_pmdio_rreg(int page, int reg, uint8 *data, int len)
{
    BCM_ENET_LINK_DEBUG("read op; page = %x; reg = %x; len = %d \n", 
        (unsigned int) page, (unsigned int) reg, len);
    
    spin_lock_bh(&bcm_extsw_access);
#ifdef CONFIG_BCM968500
    if (!bl_lilac_read_BCM53101_switch_register(page, reg, data, len)) /* zero return is error */
    {
       printk("Ext sw read failed page<%d> reg<%d> len<%d>\n",page,reg,len);
    }
#endif
    spin_unlock_bh(&bcm_extsw_access);

}
void bcmsw_pmdio_wreg(int page, int reg, uint8 *data, int len)
{
    BCM_ENET_LINK_DEBUG("write op; page = %x; reg = %x; len = %d \n", 
        (unsigned int) page, (unsigned int) reg, len);
    BCM_ENET_LINK_DEBUG("given data = %02x %02x %02x %02x \n", 
        data[0], data[1], data[2], data[3]);

    spin_lock_bh(&bcm_extsw_access);
#ifdef CONFIG_BCM968500
    if (!bl_lilac_write_BCM53101_switch_register(page, reg, data, len)) /* zero return is error */
    {
       printk("Ext sw write failed page<%d> reg<%d> len<%d>\n",page,reg,len);
    }
#endif
    spin_unlock_bh(&bcm_extsw_access);
}

int ethsw_set_mac_hw(int port, PHY_STAT ps)
{
    bdmf_object_handle port_obj = NULL;
    rdpa_port_emac_cfg_t port_emac_cfg;
    rdpa_if rdpa_port;
    bdmf_error_t rc = BDMF_ERR_OK;

    rdpa_port = rdpa_port_map_from_hw_port(port, 0);
    if (rdpa_port == rdpa_if_none)
    {
        printk("\n\n\r **** invalid port %d - has no EMAC related\n", port);
        return -1;
    }

    rc = rdpa_port_get(rdpa_port, &port_obj);
    if (rc)
        return rc;

    rc = rdpa_port_emac_cfg_get(port_obj, &port_emac_cfg);
    if (rc)
        goto error;

    port_emac_cfg.enable = 0;

    /* Set EMAC enable false */
    rc = rdpa_port_emac_cfg_set(port_obj, &port_emac_cfg);
    if (rc)
        goto error;

    if (ps.fdx)
        port_emac_cfg.emac_param.full_duplex = 1;
    else
        port_emac_cfg.emac_param.full_duplex = 0;

    if (ps.spd100)
        port_emac_cfg.emac_param.rate = rdpa_emac_rate_100m;
    else if (ps.spd1000)
        port_emac_cfg.emac_param.rate = rdpa_emac_rate_1g;
    else
        port_emac_cfg.emac_param.rate = rdpa_emac_rate_10m;

    /* Set EMAC configuration */
    rc = rdpa_port_emac_cfg_set(port_obj, &port_emac_cfg);
    if (rc)
        goto error;

    port_emac_cfg.enable = 1;

    /* Set EMAC enable true */
    rc = rdpa_port_emac_cfg_set(port_obj, &port_emac_cfg);
    if (rc)
        goto error;

error:
    bdmf_put(port_obj);
    return rc;
}

int bcmeapi_init_ext_sw_if(extsw_info_t *extSwInfo)
{
   ETHERNET_MAC_INFO EnetInfo[BP_MAX_ENET_MACS];
   ETHERNET_MAC_INFO *info;
   uint32 sw_port, port_map;
   uint8 v8 = 0;

   if (BpGetEthernetMacInfo(&EnetInfo[0], BP_MAX_ENET_MACS) != BP_SUCCESS)
   {
      printk(KERN_DEBUG " bcmeapi_init_ext_sw_if() : board id not set\n");
      return -ENODEV;
   }
   /* Check if external switch is configured in boardparams */
   info = &EnetInfo[1];
   if (!((info->ucPhyType == BP_ENET_EXTERNAL_SWITCH) || 
         (info->ucPhyType == BP_ENET_SWITCH_VIA_INTERNAL_PHY)))
   {
      printk("No External switch connected\n");
      return -ENODEV;
   }
#if 0
   /* Runner external switch config is done through board init gatemakerpro_init()
    * Keeping the code in for later use - in case the same is not done through RDPA */
   {
      /* configure Runner for External switch */
      BL_SWITCH_CFG_DTE  xi_switch_cfg;
      BL_ERROR_DTE retVal;
      xi_switch_cfg.bridge_port_id        = CE_BL_BRIDGE_PORT_LAN_4;
      xi_switch_cfg.switch_hdr_type       = BRCM_HDR_OPCODE_0;
      xi_switch_cfg.remove_hdr_when_trap  = CE_STT_TRUE;
      retVal =  bl_api_cfg_ext_switch(&xi_switch_cfg);   
      if ( retVal != CE_BL_NO_ERROR )
      {
         printk("\n\nERROR !! Runner External Switch cfg failed <%d>\n\n",(int)retVal);
      }
   }
#endif /* 0 */

/* Get the internal switch/runner MAC info */
   info = &EnetInfo[0];
   if (info->sw.port_map & (1<<extSwInfo->connected_to_internalPort))
   { /* Not needed but a valid check */
      unsigned long runner_phy_id = info->sw.phy_id[extSwInfo->connected_to_internalPort];
      PHY_STAT ps = {0};

      switch (runner_phy_id & (PHY_LNK_CFG_M << PHY_LNK_CFG_S))
      {
      
      case FORCE_LINK_10HD:
         ps.lnk = 1;
         break;

      case FORCE_LINK_10FD:
         ps.lnk = 1;
         ps.fdx = 1;
         break;

      case FORCE_LINK_100HD:
         ps.lnk = 1;
         ps.spd100 = 1;
         break;

      case FORCE_LINK_100FD:
         ps.lnk = 1;
         ps.spd100 = 1;
         ps.fdx = 1;
         break;

      case FORCE_LINK_1000FD:
         ps.lnk = 1;
         ps.spd1000 = 1;
         ps.fdx = 1;
         break;

      default:
         printk("Invalid Link/PHY config for internal port connected to external switch <0x%x>\n",(unsigned int)runner_phy_id);
         return -1;
      }

      /* Now set the MAC */
      ethsw_set_mac_hw(extSwInfo->connected_to_internalPort, ps);
   }

   info = &EnetInfo[1]; /* External Switch Info from Boardparams */
   sw_port = 0; /* Start with first port */
   port_map = info->sw.port_map;
   for (; port_map; sw_port++, port_map /= 2 )
   {
      /* Skip the switch ports which are not in the port_map */
      while ((port_map % 2) == 0)
      {
         port_map /= 2;  
         sw_port++;
      }
#ifdef CONFIG_BCM968500
      /* Read the current Port Traffic Control Register */
      if (!bl_lilac_read_BCM53101_switch_register(PAGE_CONTROL, REG_PORT_CTRL+sw_port, &v8, sizeof(v8))) /* zero return is error */
      {
         printk("Ext sw read failed page<%d> reg<%d> len<%d>\n",PAGE_CONTROL, (int)(REG_PORT_CTRL+sw_port),sizeof(v8));
         continue;
      }
#endif
      /* Enable RX and TX - these gets disabled in board driver (something different in Lilac based designs)*/
      v8 &= (~REG_PORT_CTRL_DISABLE);
#ifdef CONFIG_BCM968500
      /* Write the Port Traffic Control Register */
      if (!bl_lilac_write_BCM53101_switch_register(PAGE_CONTROL, REG_PORT_CTRL+sw_port, &v8, sizeof(v8))) /* zero return is error */
      {
         printk("Ext sw write failed page<%d> reg<%d> len<%d> val<0x%x>\n",PAGE_CONTROL,(int)(REG_PORT_CTRL+sw_port),sizeof(v8),(unsigned int)v8);
         continue;
      }
#endif
   }

   return 0; /* Success */
}

int bcmeapi_ioctl_ethsw_clear_port_emac(struct ethswctl_data *e)
{
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    rdpa_emac_stat_t emac_stat;
    int rc = 0;

    if ( (rdpa_emac0 + port) >= rdpa_emac__num_of )
	{
		BCM_ENET_ERROR("invalid lan port id %d \n", port);
		return -1;
	}
    
    /* clear local data */
    memset(&cached_rdpa_emac_stat[rdpa_emac0 + port], 0, sizeof(rdpa_emac_stat_t));
    
    /* clear counters in rdpa */
    bdmf_lock();
	
    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
    if (rc )
        goto unlock_exit;
    rc = rdpa_port_emac_stat_get(port_obj, &emac_stat);
    if (rc )
        goto unlock_exit;
   
unlock_exit:
    if(port_obj)
        bdmf_put(port_obj);
																			 
    bdmf_unlock();
    return rc; 
}

int bcmeapi_ioctl_ethsw_get_port_emac(struct ethswctl_data *e)
{
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    rdpa_emac_stat_t emac_stat;
    struct emac_stats temp;
    int rc;
    
    BCM_ENET_INFO("Port = %2d", e->port);
        
	if ( (rdpa_emac0 + port) >= rdpa_emac__num_of )
	{
		BCM_ENET_ERROR("invalid lan port id %d \n", port);
		return -1;
	}

	bdmf_lock();
	
    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
	if (rc )
		goto unlock_exit;
    rc = rdpa_port_emac_stat_get(port_obj, &emac_stat);
	if (rc )
		goto unlock_exit;

   	/* rdpa counters are read and clear, need to store data into local cache */
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.byte += emac_stat.rx.byte;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.packet += emac_stat.rx.packet;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_64 += emac_stat.rx.frame_64;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_65_127 += emac_stat.rx.frame_65_127;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_128_255 += emac_stat.rx.frame_128_255;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_256_511 += emac_stat.rx.frame_256_511;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_512_1023 += emac_stat.rx.frame_512_1023;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_1024_1518 += emac_stat.rx.frame_1024_1518;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_1519_mtu += emac_stat.rx.frame_1519_mtu;
    cached_rdpa_emac_stat[rdpa_emac0 + port].rx.unicast_packet += emac_stat.rx.unicast_packet;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.multicast_packet += emac_stat.rx.multicast_packet;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.broadcast_packet += emac_stat.rx.broadcast_packet;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.alignment_error += emac_stat.rx.alignment_error;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_length_error += emac_stat.rx.frame_length_error;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.code_error += emac_stat.rx.code_error;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.carrier_sense_error += emac_stat.rx.carrier_sense_error;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.fcs_error += emac_stat.rx.fcs_error;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.control_frame += emac_stat.rx.control_frame;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.pause_control_frame += emac_stat.rx.pause_control_frame;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.unknown_opcode += emac_stat.rx.unknown_opcode;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.undersize_packet += emac_stat.rx.undersize_packet;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.oversize_packet += emac_stat.rx.oversize_packet;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.fragments += emac_stat.rx.fragments;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.jabber += emac_stat.rx.jabber;
	cached_rdpa_emac_stat[rdpa_emac0 + port].rx.overflow += emac_stat.rx.overflow;


    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.byte += emac_stat.tx.byte;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.packet += emac_stat.tx.packet;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_64 += emac_stat.tx.frame_64;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_65_127 += emac_stat.tx.frame_65_127;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_128_255 += emac_stat.tx.frame_128_255;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_256_511 += emac_stat.tx.frame_256_511;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_512_1023 += emac_stat.tx.frame_512_1023;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_1024_1518 += emac_stat.tx.frame_1024_1518;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_1519_mtu += emac_stat.tx.frame_1519_mtu;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.fcs_error += emac_stat.tx.fcs_error;
    cached_rdpa_emac_stat[rdpa_emac0 + port].tx.unicast_packet += emac_stat.tx.unicast_packet;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.multicast_packet += emac_stat.tx.multicast_packet;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.broadcast_packet += emac_stat.tx.broadcast_packet;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.excessive_collision += emac_stat.tx.excessive_collision;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.late_collision += emac_stat.tx.late_collision;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.single_collision += emac_stat.tx.single_collision;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.multiple_collision += emac_stat.tx.multiple_collision;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.total_collision += emac_stat.tx.total_collision;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.pause_control_frame += emac_stat.tx.pause_control_frame;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.deferral_packet += emac_stat.tx.deferral_packet;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.excessive_deferral_packet += emac_stat.tx.excessive_deferral_packet;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.jabber_frame += emac_stat.tx.jabber_frame;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.control_frame += emac_stat.tx.control_frame;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.oversize_frame += emac_stat.tx.oversize_frame;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.undersize_frame += emac_stat.tx.undersize_frame;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.fragments_frame += emac_stat.tx.fragments_frame;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.error += emac_stat.tx.error;
	cached_rdpa_emac_stat[rdpa_emac0 + port].tx.underrun += emac_stat.tx.underrun;

    temp.rx_byte = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.byte;
    temp.rx_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.packet;
	temp.rx_frame_64 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_64;
	temp.rx_frame_65_127 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_65_127;
	temp.rx_frame_128_255 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_128_255;
	temp.rx_frame_256_511 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_256_511;
	temp.rx_frame_512_1023 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_512_1023;
	temp.rx_frame_1024_1518 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_1024_1518;
    temp.rx_unicast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.unicast_packet;
	temp.rx_multicast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.multicast_packet;
	temp.rx_broadcast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.broadcast_packet;
	temp.rx_alignment_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.alignment_error;
	temp.rx_frame_length_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.frame_length_error;
    temp.rx_code_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.code_error;
	temp.rx_carrier_sense_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.carrier_sense_error;
	temp.rx_fcs_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.fcs_error;
	temp.rx_undersize_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.undersize_packet;
	temp.rx_oversize_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.oversize_packet;
	temp.rx_fragments = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.fragments;
	temp.rx_jabber = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.jabber;
	temp.rx_overflow = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].rx.overflow;
   
    temp.tx_byte = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.byte;
	temp.tx_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.packet;
	temp.tx_frame_64 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_64;
	temp.tx_frame_65_127 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_65_127;
	temp.tx_frame_128_255 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_128_255;
	temp.tx_frame_256_511 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_256_511;
	temp.tx_frame_512_1023 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_512_1023;
	temp.tx_frame_1024_1518 = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.frame_1024_1518;
	temp.tx_fcs_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.fcs_error;
    temp.tx_unicast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.unicast_packet;
	temp.tx_multicast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.multicast_packet;
	temp.tx_broadcast_packet = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.broadcast_packet;
	temp.tx_total_collision = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.total_collision;
	temp.tx_jabber_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.jabber_frame;
	temp.tx_oversize_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.oversize_frame;
	temp.tx_undersize_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.undersize_frame;
	temp.tx_fragments_frame = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.fragments_frame;
	temp.tx_error = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.error;
	temp.tx_underrun = (unsigned long long)cached_rdpa_emac_stat[rdpa_emac0 + port].tx.underrun;
 
    if (copy_to_user((void*)(&e->emac_stats_s), (void*)&temp, sizeof(struct emac_stats))) {
        rc = -EFAULT;
        goto unlock_exit;
    }

unlock_exit:
	if(port_obj)
		bdmf_put(port_obj);
																			 
	 bdmf_unlock();
	 return rc; 
}

#ifdef CONFIG_BCM96838
int bcmeapi_ioctl_ethsw_port_pause_capability(struct ethswctl_data *e)
{
	int retval = 0;
    int rc = 0;
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    rdpa_port_emac_cfg_t port_emac_cfg;

	BCM_ENET_INFO("Port = %2d", e->port);

	if ( (rdpa_emac0 + port) >= rdpa_emac__num_of )
	{
		BCM_ENET_ERROR("invalid lan port id %d \n", port);
		return -1;
	}


	bdmf_lock();

	rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
	if (rc )
		goto unlock_exit;

	rc = rdpa_port_emac_cfg_get(port_obj, &port_emac_cfg);
	if (rc )
		goto unlock_exit;
	
	if (e->type == TYPE_GET)	
	{
		retval = port_emac_cfg.emac_param.rx_flow_control & port_emac_cfg.emac_param.tx_flow_control;

		if (copy_to_user((void*)(&e->ret_val), (void*)&retval, sizeof(int))) {
			rc = -EFAULT;
			goto unlock_exit;
		}
	}
	else
	{
    	rdpa_port_flow_ctrl_t flowCtrl = {0};
    	// emac_cfg has no use for flow control.
    	//port_emac_cfg.emac_param.rx_flow_control = port_emac_cfg.emac_param.tx_flow_control = e->val;
        struct net_device *dev = enet_phyport_to_vport_dev(port);
        if(NULL != dev)
        {
            memcpy(flowCtrl.src_address.b, dev->dev_addr, ETH_ALEN);
        }
        else
        {
            rc = -1;
            goto unlock_exit;
        }


        if (e->val == TRUE)
		{
			flowCtrl.rate = 100000000;
			flowCtrl.mbs = 250000;
			flowCtrl.threshold = 125000;
		}
		else
		{
			flowCtrl.rate = 0;
			flowCtrl.mbs = 0;
			flowCtrl.threshold = 0;
		}
		rc = rdpa_port_flow_control_set(port_obj, &flowCtrl);
		if (rc )
			goto unlock_exit;
	}
	

unlock_exit:
	if(port_obj)
		bdmf_put(port_obj);
																			 
	 bdmf_unlock();
	 return rc; 
}

#ifndef READ_32
#define READ_32(a, r)			( *(volatile uint32_t*)&(r) = *(volatile uint32_t*) DEVICE_ADDRESS(a) )
#endif
#ifndef UNIMAC_READ32_REG
#define UNIMAC_READ32_REG(e,r,o) READ_32( UNIMAC_CONFIGURATION_UMAC_0_RDP_##r + (UNIMAC_CONF_EMAC_OFFSET * (e)) , (o) )
#endif

int bcmeapi_ioctl_ethsw_port_traffic_control(struct ethswctl_data *e, int phy_id)
{
	uint32_t port = e->port;
	bdmf_object_handle port_obj = NULL;
	rdpa_port_emac_cfg_t port_emac_cfg;
	int rc = 0;

	BCM_ENET_INFO("Port = %2d, enable %2d", e->port, e->val);
		
	if ( (rdpa_emac0 + port) >= rdpa_emac__num_of )
	{
		BCM_ENET_ERROR("invalid lan port id %d \n", port);
		return -1;
	}

	bdmf_lock();

	rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
	if (rc )
		goto unlock_exit;

	rc = rdpa_port_emac_cfg_get(port_obj, &port_emac_cfg);
	if (rc )
		goto unlock_exit;
	
	if (e->type == TYPE_GET)	
	{
#if 0	// It is not accurate for mac status
		retval = port_emac_cfg.enable;

		if (copy_to_user((void*)(&e->ret_val), (void*)&retval, sizeof(int))) {
			rc = -EFAULT;
			goto unlock_exit;
		}
#else
		unsigned int regVal;
		int macStatus;
		// Wait for the extension of RDPA
		// Use e->port for phy_id here
		UNIMAC_READ32_REG(e->port, MODE, regVal);

		macStatus = (regVal & 0x20) >> 5;
		if (copy_to_user((void*)(&e->ret_val), (void*)&macStatus, sizeof(int))) {
			rc = -EFAULT;
		}
#endif
	}
	else
	{
		unsigned short bmcr;
        
		port_emac_cfg.enable = e->val;
		rc = rdpa_port_emac_cfg_set(port_obj, &port_emac_cfg);
		if (rc )
			goto unlock_exit;

		ethsw_phyport_rreg2(phy_id, MII_BMCR, &bmcr, 0);

		if (port_emac_cfg.enable == FALSE)
		{
			bmcr |= BMCR_PDOWN;
			ethsw_phyport_wreg2(phy_id, MII_BMCR, &bmcr, 0);
		}
		else if (port_emac_cfg.enable == TRUE)
		{
			bmcr &= ~BMCR_PDOWN;
			ethsw_phyport_wreg2(phy_id, MII_BMCR, &bmcr, 0);
		}
	}
	

unlock_exit:
	if(port_obj)
		bdmf_put(port_obj);
																			 
	 bdmf_unlock();
	 return rc; 
}

int bcmeapi_ioctl_ethsw_phy_autoneg_info(struct ethswctl_data *e, int phy_id)
{
	int rc = 0;
	int retval = 0;

	if (e->type == TYPE_GET)
	{
		unsigned short bmcr, bmsr, nway_advert, gig_ctrl, gig_status;
		
		ethsw_phyport_rreg2(phy_id, MII_BMCR, &bmcr, 0);
		ethsw_phyport_rreg2(phy_id, MII_BMSR, &bmsr, 0);
		
		if (bmcr == 0xffff	||	bmsr == 0x0000) {
			retval = -1;
		}
		
		/* Get the status not set the control */
		if (bmcr & BMCR_ANENABLE) 
			e->autoneg_info = TRUE;
		else
			e->autoneg_info = FALSE;
			
		ethsw_phyport_rreg2(phy_id, MII_ADVERTISE, &nway_advert, 0);
		ethsw_phyport_rreg2(phy_id, MII_CTRL1000, &gig_ctrl, 0);
		ethsw_phyport_rreg2(phy_id, MII_STAT1000, &gig_status, 0);
		
		if (gig_ctrl & ADVERTISE_1000FULL)
			e->autoneg_ad |= AN_1000M_FULL;

		if (gig_ctrl & ADVERTISE_1000HALF)
			e->autoneg_ad |= AN_1000M_HALF;

		if (nway_advert & ADVERTISE_10HALF)
			e->autoneg_ad |= AN_10M_HALF;

		if (nway_advert & ADVERTISE_10FULL)
			e->autoneg_ad |= AN_10M_FULL;

		if (nway_advert & ADVERTISE_100HALF)
			e->autoneg_ad |= AN_100M_HALF;

		if (nway_advert & ADVERTISE_100FULL)
			e->autoneg_ad |= AN_100M_FULL;

		// To be confirmed here
		e->autoneg_local = e->autoneg_ad;
		
		if (copy_to_user((void*)(&e->ret_val), (void*)&retval, sizeof(int))) {
			rc = -EFAULT;
		}
	}
	else
	{
		unsigned short bmcr;
		ethsw_phyport_rreg2(phy_id, MII_BMCR, &bmcr, 0);
		if (e->autoneg_info & AUTONEG_RESTART_MASK)
		{
			bmcr |= BMCR_ANRESTART;
			ethsw_phyport_wreg2(phy_id, MII_BMCR, &bmcr, 0);
		}
		else
		{
			if (e->autoneg_info & AUTONEG_CTRL_MASK)
				bmcr |= BMCR_ANENABLE;
			else
				bmcr &= ~BMCR_ANENABLE;
			ethsw_phyport_wreg2(phy_id, MII_BMCR, &bmcr, 0);
		}
	}
	return rc;
}

int bcmeapi_ioctl_ethsw_link_status(struct ethswctl_data *e, int phy_id)
{
	int rc = 0;

	if (e->type == TYPE_GET)
	{
		int linkStatus;
		unsigned short bmsr;
		ethsw_phyport_rreg2(phy_id, MII_BMSR, &bmsr, 0);
		
		linkStatus = (bmsr & BMSR_LSTATUS) >> 2;
		if (copy_to_user((void*)(&e->status), (void*)&linkStatus, sizeof(int))) 
			rc = -EFAULT;
	}
#if 0	     // Only worked in auto-config disabled status
	else
	{
		uint32_t port = e->port;
		bdmf_object_handle port_obj = NULL;
		rdpa_port_emac_cfg_t port_emac_cfg;
		
		BCM_ENET_INFO("Port = %2d, status %d, speed %d, duplex %d", e->port, e->status, e->speed, e->duplex);
			
		if ( (rdpa_emac0 + port) >= rdpa_emac__num_of )
		{
			BCM_ENET_ERROR("invalid lan port id %d \n", port);
			return -1;
		}
		
		bdmf_lock();
		
		rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
		if (rc )
			goto unlock_exit;
		
		rc = rdpa_port_emac_cfg_get(port_obj, &port_emac_cfg);
		if (rc )
			goto unlock_exit;

		port_emac_cfg.emac_param.full_duplex = e->duplex;
		port_emac_cfg.enable= e->status;
		if (e->speed == 10)
			port_emac_cfg.emac_param.rate= rdpa_emac_rate_10m;
		else if (e->speed == 100)
			port_emac_cfg.emac_param.rate= rdpa_emac_rate_100m;
		else if (e->speed == 1000)
			port_emac_cfg.emac_param.rate= rdpa_emac_rate_1g;
		
		rc = rdpa_port_emac_cfg_set(port_obj, &port_emac_cfg);
		if (rc )
			goto unlock_exit;
			
unlock_exit:
		if(port_obj)
			bdmf_put(port_obj);
																				 
		 bdmf_unlock();
		
	}
#endif
	
	return rc;
}


int bcmeapi_ioctl_ethsw_port_transparent_set (struct ethswctl_data *e)
{
   uint32_t port = e->port;
   bdmf_object_handle port_obj = NULL;
   rdpa_if rdpaif;
   rdpa_port_vlan_isolation_t   vlan_isolation;   
   int rc = 0;

    rdpaif = rdpa_if_lan0 + port;
    if ( rdpa_emac0 + port>= rdpa_emac__num_of )
    {
        BCM_ENET_ERROR("invalid lan port id %d \n", port);
        return -1;
    }

    rc = rdpa_port_get(rdpaif, &port_obj);
    if (rc )
        goto unlock_exit;

    rc = rdpa_port_transparent_set(port_obj,e->transparent);
    if (rc )
               goto unlock_exit;

    
    rc =  rdpa_port_vlan_isolation_get(port_obj , &vlan_isolation);
    if (rc >= 0)
    {
        vlan_isolation.ds = !e->transparent;
        vlan_isolation.us = !e->transparent;
        rc = rdpa_port_vlan_isolation_set(port_obj, &vlan_isolation);
    }
    unlock_exit:
    if(port_obj)
        bdmf_put(port_obj);
    return rc; 
}


int bcmeapi_ioctl_ethsw_port_vlanlist_set(struct ethswctl_data *e)
{
    uint32_t port = e->port;
    bdmf_object_handle port_obj = NULL;
    int rc = 0;
    bdmf_object_handle vlan_obj = NULL;
    char vlanName[BDMF_OBJ_NAME_LEN];
    bcm_vlan_t vid = e->vid & BCM_NET_VLAN_VID_M;
    struct net_device *dev = NULL;
    BDMF_MATTR(vlan_attrs, rdpa_vlan_drv());

    for(port=0;port<rdpa_emac__num_of;port++)
    {
        if(e->fwd_map & (1<<port))
            break;
    }

    if ((rdpa_emac0 + port) >= rdpa_emac__num_of )
    {
        BCM_ENET_ERROR( " Invalid port id %d \n", port);
        return -1;
    }

    dev = enet_phyport_to_vport_dev(port);
    if(NULL != dev)
        snprintf(vlanName, BDMF_OBJ_NAME_LEN, "vlan_%s_def", dev->name);
    else
        return -1;

    bdmf_lock();
    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
    if (rc )
        goto unlock_exit;

    rdpa_vlan_get(vlanName,&vlan_obj);
    if(!vlan_obj)
    {
        rdpa_vlan_name_set(vlan_attrs,vlanName);
        rc = bdmf_new_and_set( rdpa_vlan_drv(), port_obj, vlan_attrs, &vlan_obj);
        if (rc )
            goto unlock_exit;
    }
    if(e->untag_map & (1<<port))
        rc = rdpa_vlan_vid_enable_set(vlan_obj,vid,0);
    else
        rc = rdpa_vlan_vid_enable_set(vlan_obj,vid,1);

    unlock_exit:
    if(port_obj)
        bdmf_put(port_obj);
    bdmf_unlock();
  
    return rc; 
}


int bcmeapi_ioctl_ethsw_port_vlan_isolation_set (struct ethswctl_data *e)
{
   uint32_t port = e->port;
   bdmf_object_handle port_obj = NULL;
   rdpa_if rdpaif;
   rdpa_port_vlan_isolation_t   vlan_isolation;   
   int rc = 0;

    rdpaif = rdpa_if_lan0 + port;
    if ( rdpa_emac0 + port>= rdpa_emac__num_of )
    {
        BCM_ENET_ERROR("invalid lan port id %d \n", port);
        return -1;
    }
    
    rc = rdpa_port_get(rdpaif, &port_obj);
    if (rc )
        goto unlock_exit;

    rc =  rdpa_port_vlan_isolation_get(port_obj , &vlan_isolation);
    if (rc >= 0)
    {
        vlan_isolation.ds = e->vlan_isolation.ds_enable;
        vlan_isolation.us = e->vlan_isolation.us_enable;
        rc = rdpa_port_vlan_isolation_set(port_obj, &vlan_isolation);
    }
    unlock_exit:
    if(port_obj)
        bdmf_put(port_obj);
    return rc; 
}
#endif // CONFIG_BCM96838

int ethsw_get_hw_stats(int port, struct net_device_stats *stats)
{
	bdmf_object_handle port_obj = NULL;
	bdmf_object_handle gem = NULL;
	bdmf_object_handle iptv = NULL;
	rdpa_emac_stat_t emac_stat;
	rdpa_gem_stat_t gem_stat;
	rdpa_iptv_stat_t iptv_stat;
	struct net_device_stats *port_stats;
	int rc;

	/*validate device is already initialized first device is the bcmsw*/
	if (!vnet_dev[port + 1])
		return 0;

	/*take the stats pointer*/
	port_stats = &((BcmEnet_devctrl *) netdev_priv(vnet_dev[port + 1] ))->stats;

	/*now we accumulate our counters into port_stats since our HW is clear on read*/

	BCM_ENET_INFO("Port = %2d", port);


	if (port == GPON_PORT_ID || port == EPON_PORT_ID)
	{
	    while((gem = bdmf_get_next(rdpa_gem_drv(), gem, NULL)))
	    {
	        rc = rdpa_gem_stat_get(gem, &gem_stat);
	        if (rc)
	            goto gem_exit;

	        port_stats->rx_bytes += gem_stat.rx_bytes;
	        port_stats->rx_packets += gem_stat.rx_packets;
	        port_stats->rx_dropped += gem_stat.rx_packets_discard;
	        port_stats->tx_bytes += gem_stat.tx_bytes;
	        port_stats->tx_packets += gem_stat.tx_packets;
	        port_stats->tx_dropped += gem_stat.tx_packets_discard;
	    }

gem_exit:
	    if (gem)
	        bdmf_put(gem);

	    rc = rdpa_iptv_get(&iptv);
	    if (rc)
	        goto iptv_exit;

	    rc = rdpa_iptv_iptv_stat_get(iptv, &iptv_stat);
	    if (rc)
	        goto iptv_exit;

	    stats->multicast += iptv_stat.rx_valid_pkt;

iptv_exit:
	    if (iptv)
	        bdmf_put(iptv);
	}
	else
	{
	    rc = rdpa_port_get(rdpa_if_lan0 + port, &port_obj);
	    if (rc)
	        goto port_unlock_exit;
	    rc = rdpa_port_emac_stat_get(port_obj, &emac_stat);
	    if (rc)
	        goto port_unlock_exit;

	    port_stats->collisions += emac_stat.tx.total_collision;
	    port_stats->multicast += emac_stat.rx.multicast_packet;
	    port_stats->rx_bytes += emac_stat.rx.byte;
	    port_stats->rx_packets += emac_stat.rx.packet;
	    port_stats->rx_crc_errors += emac_stat.rx.fcs_error;
	    port_stats->rx_errors += emac_stat.rx.alignment_error +
	            emac_stat.rx.code_error +
	            emac_stat.rx.frame_length_error ;
	    port_stats->rx_length_errors += emac_stat.rx.frame_length_error;
	    port_stats->tx_bytes += emac_stat.tx.byte;
	    port_stats->tx_errors += emac_stat.tx.error;
	    port_stats->tx_packets += emac_stat.tx.packet;

#if defined(CONFIG_BCM_KF_EXTSTATS)
	    port_stats->tx_multicast_packets += emac_stat.tx.multicast_packet;
#endif
	    /*copy the stats to the output pointer*/
	    memcpy(stats,port_stats,sizeof(*stats));
port_unlock_exit:
	    if (port_obj)
	        bdmf_put(port_obj);
	}

	return rc;
}

int bcmeapi_ioctl_ethsw_sal_dal_set (struct ethswctl_data *e)
{
   uint32_t port = e->port;
   bdmf_object_handle port_obj = NULL;
   rdpa_if rdpaif;
   rdpa_port_dp_cfg_t cfg;    
   int rc = 0;

   if(e->unit) /*unit 0 - emacs, unit 1 - wan*/
   {
      rdpaif = rdpa_if_wan0;
   }
   else
   {
      rdpaif = rdpa_if_lan0 + port;
      if ( rdpa_emac0 + port>= rdpa_emac__num_of )
      {
         BCM_ENET_ERROR("invalid lan port id %d \n", port);
         return -1;
      }
   } 
   rc = rdpa_port_get(rdpaif, &port_obj);
   if (rc )
      goto unlock_exit;
   rc = rdpa_port_cfg_get(port_obj , &cfg);
   if (rc >= 0)
   {
      cfg.sal_enable = e->  sal_dal_en;
      cfg.dal_enable = e->  sal_dal_en;
      rc = rdpa_port_cfg_set(port_obj, &cfg);
   }
   unlock_exit:
   if(port_obj)
      bdmf_put(port_obj);
   return rc; 
}

int bcmeapi_ioctl_ethsw_mtu_set  (struct ethswctl_data *e)
{
   bdmf_object_handle system = NULL; 
   rdpa_system_cfg_t cfg;
   int rc=0;

   bdmf_lock();
   rc = rdpa_system_get(&system);
    if (rc < 0 || !system)
    {
        BCM_LOG_NOTICE(BCM_LOG_ID_PLOAM_FSM, "Failed to set mtu :  rdpa_system_get return error: %d", rc);
        return rc;
    }

     rc = rdpa_system_cfg_get(system, &cfg);
      if (rc < 0 || !system)
      {
         BCM_LOG_NOTICE(BCM_LOG_ID_PLOAM_FSM, "Failed to set mtu :  rdpa_system_get return error: %d", rc);
         goto out;
      }

       cfg.mtu_size = e->mtu;
        rdpa_system_cfg_set(system, &cfg);
out:
   bdmf_put(system);
   bdmf_unlock();

   return rc; 
}
