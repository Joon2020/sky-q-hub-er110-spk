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


#ifndef __LILAC_ETH_COMMON_H__
#define __LILAC_ETH_COMMON_H__

#include "bl_lilac_brd.h"
#include "bl_lilac_vpb_bridge.h"
#include "bl_lilac_drv_mac.h"
#include "bl_lilac_rst.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
	ETH_MODE_SGMII      = BL_LILAC_VPB_ETH_MODE_SGMII,
	ETH_MODE_HISGMII    = BL_LILAC_VPB_ETH_MODE_HISGMII,
    ETH_MODE_QSGMII     = BL_LILAC_VPB_ETH_MODE_QSGMII,
	ETH_MODE_SMII       = BL_LILAC_VPB_ETH_MODE_SMII,
	E4_MODE_RGMII,
	E4_MODE_TMII,
	ETH_MODE_NONE,
	ETH_MODE_LAST
} ETH_MODE;

extern const char* EMAC_MODE_NAME[ETH_MODE_LAST]; 
extern const char* MAC_PHY_RATE_NAME[DRV_MAC_PHY_RATE_LAST];


#define E4_MODE_TOV_PB_E4_MODE(x) (x - E4_MODE_RGMII)

/* error codes of fi_basic_data_path_init */
typedef enum
{
    BDPI_NO_ERROR = 0 ,
    BDPI_ERROR_EMAC_ID_OUT_OF_RANGE ,
    BDPI_ERROR_EMAC_IS_NOT_FUNCTIONAL ,
    BDPI_ERROR_EMAC_IS_NOT_OUT_OF_RESET ,
    BDPI_ERROR_EMACS_GROUP_MODE_OUT_OF_RANGE ,
    BDPI_ERROR_EMAC4_MODE_OUT_OF_RANGE ,
    BDPI_ERROR_RDD_ERROR ,
    BDPI_ERROR_CR_INITIALIZATION_ERROR ,
    BDPI_ERROR_VPB_INITIALIZATION_ERROR ,
    BDPI_ERROR_IH_INITIALIZATION_ERROR ,
    BDPI_ERROR_BBH_INITIALIZATION_ERROR ,
    BDPI_ERROR_BPM_INITIALIZATION_ERROR ,
    BDPI_ERROR_SBPM_INITIALIZATION_ERROR ,
    BDPI_ERROR_SERDES_INITIALIZATION_ERROR,
    BDPI_ERROR_PIN_MUX_ERROR,
    BDPI_ERROR_MEMORY_ERROR,
}LILAC_BDPI_ERRORS;

DRV_MAC_PHY_RATE bl_lilac_auto_mac(int phyId, int macId, ETH_MODE mode, DRV_MAC_PHY_RATE cur_rate);
DRV_MAC_PHY_RATE bl_lilac_read_phy_line_status(int phyAddr);
int bl_lilac_check_mgm_port_cfg(int mac, ETH_MODE emac_group_mode, ETH_MODE emac_mode);
int bl_lilac_get_link_status(int phyAddr);
void bl_lilac_phy_auto_enable(int phyId);
LILAC_BDPI_ERRORS f_align_rgmii (void);
void bl_lilac_start_25M_clock(void);
int bl_lilac_ReadPHYReg(int phyAddr, int reg);
int bl_lilac_WritePHYReg(int phyAddr, int reg, int val);
void config_tbi(DRV_MAC_NUMBER xi_emac_id, ETH_MODE xi_emacs_group_mode);
void enableEmacs(DRV_MAC_NUMBER xi_emac_id,ETH_MODE xi_emacs_group_mode,ETH_MODE xi_emac4_mode);
BL_LILAC_CR_DEVICE bl_lilac_map_emac_id_to_cr_device(DRV_MAC_NUMBER xi_emac_id);
LILAC_BDPI_ERRORS configureEmacs(DRV_MAC_NUMBER xi_emac_id,ETH_MODE xi_emacs_group_mode,ETH_MODE xi_emac4_mode);

#ifdef __cplusplus
}
#endif

#endif /* __LILAC_ETH_COMMON_H__*/
