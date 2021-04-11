/*
 Copyright 2004-2010 Broadcom Corp. All Rights Reserved.

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
#ifndef _ETHSW_RUNNER_H_
#define _ETHSW_RUNNER_H_

int bcmeapi_ethsw_dump_mib(int port, int type);
#define bcmeapi_ethsw_dscp_to_priority_mapping(e) 0
#define bcmeapi_ioctl_ethsw_set_multiport_address(e) 0

/****************************************************************************
    Prototypes
****************************************************************************/

int ethsw_phy_pll_up(int ephy_and_gphy);
uint32 ethsw_ephy_auto_power_down_wakeup(void);
uint32 ethsw_ephy_auto_power_down_sleep(void);
int ethsw_reset_ports(struct net_device *dev);
int ethsw_enable_sar_port(void);
int ethsw_disable_sar_port(void);
int ethsw_save_port_state(void);
int ethsw_restore_port_state(void);
int ethsw_port_to_phyid(int port);
void ethsw_port_based_vlan(int port_map, int wan_port_bit, int txSoftSwitchingMap);

void ethsw_configure_ports(int port_map, int *pphy_id);

#define ethsw_phy_rreg(phy_id, reg, data_ptr) ethsw_phy_read_reg(phy_id, reg, (data_ptr), 0)
#define ethsw_phy_wreg(phy_id, reg, data_ptr) ethsw_phy_write_reg(phy_id, reg, (data_ptr), 0)

void ethsw_phy_read_reg(int phy_id, int reg, uint16 *data, int ext_bit);
void ethsw_phy_write_reg(int phy_id, int reg, uint16 *data, int ext_bit);

#define bcmeapi_ethsw_init_config() {}

#endif /* _ETHSW_RUNNER_H_ */
