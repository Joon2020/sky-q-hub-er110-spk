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

#ifndef _BL_LILAC_BRD_H_
#define _BL_LILAC_BRD_H_

#include "bl_lilac_soc.h"

/*WD definition below depended to board HW design
 and should be adjusted depended to you board*/
#define WD_GPIO_PIN     47
#define WD_TIMER        BL_HW_TIMER_3

/* Timer used for rttimer, rttimer is initialized in network driver */
#define DEFAULT_RTTIMER BL_HW_TIMER_0

// board types, 
// the enum values defined according to HW defined strap options
// change the value only if HW definition changed 


#define TM_BASE_ADDR_STR				"tm"
#define TM_MC_BASE_ADDR_STR				"mc"
#define DSP_BASE_ADDR_STR				"dsp"
#define BL_LILAC_TM_DEF_DDR_SIZE 		0x1400000
#define BL_LILAC_TM_MC_DEF_DDR_SIZE		0x0400000
#define BL_LILAC_DSP_DEF_DDR_SIZE		0x00200000


typedef enum
{
    TULIP_TBI_SGMII_0 = 0x1a,
    TULIP_TBI_SGMII_1 = 0x1b,
    TULIP_TBI_SGMII_2 = 0x1c,
    TULIP_TBI_SGMII_3 = 0x1d,
    TULIP_TBI_SGMII_4 = 0x14,
    TULIP_TBI_SGMII_5 = 0x1f
}TBI_SGMII_PHY_ADDRESSES;

#define TULIP_SPI_CHIP_SELECT		41 
#define TULIP_SPI_CLOCK				5000000

typedef enum
{
	BL_LILAC_USB_OC_ON,
	BL_LILAC_USB_OC_OFF
}BL_LILAC_USB_OC_PRESENT;

BL_LILAC_USB_OC_PRESENT  bl_lilac_get_usb_oc_properties(int usb_port_id, int *oc_input_pin, int *oc_output_pin);
BL_LILAC_SOC_STATUS bl_lilac_mem_rsrv_get_by_name(char *name, void **addr, unsigned long  *size);
void  lilac_restart(char *command);
void  bl_lilac_plat_rgmii_phy_init(int phyId);
void  bl_lilac_init_qsgmii_phy(void);

int   bl_lilac_pci_lane_enabled(int id);
int   bl_lilac_usb_hcd_enabled(int id);

static inline unsigned long bl_lilac_get_pll_src_freq(BL_LILAC_PLL_SRC src)
{
	switch(src)
	{
		case BL_XTAL:
			return 19440000;
		case BL_TCXO:
			return 20000000;
		case BL_GPON_REF:
			return 19440000;
		case BL_NIF_FREE_RUN_CLK:
			return 125000000;
		case BL_NIF_TX_CLK:
			return 500000000;
		default:
			return 0;
	}
}

int bl_lilac_read_BCM53101_switch_register(int page, int reg, unsigned char *data, int len_in_bytes);
int bl_lilac_write_BCM53101_switch_register(int page, int reg, unsigned char *data, int len_in_bytes); 


#endif //#ifndef _BL_LILAC_BRD_H_

