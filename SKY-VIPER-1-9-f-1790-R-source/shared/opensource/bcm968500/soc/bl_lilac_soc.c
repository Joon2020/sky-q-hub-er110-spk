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

#include "bl_os_wraper.h"
#include "bl_lilac_soc.h"
#include "bl_lilac_cr.h"
#include "bl_lilac_mips_blocks.h"


typedef struct
{
	BL_LILAC_PLL_PROPERTIES			plls[BL_PLL_NUM];
	BL_LILAC_PLL_CLIENT_PROPERTIES 	clients[PLL_CLIENT_NUM];
	BL_LILAC_TDM_MODE 				tdm;
	int								periph;
	
}BL_LILAC_CLOCKS_PROPERTIES;

typedef enum
{
	CLOCK_PROFILE_0,
	CLOCK_PROFILE_1,
	CLOCK_PROFILE_2,
	CLOCK_PROFILE_NUMBER
}BL_LILAC_CLOCK_PROFILE_ID;

BL_LILAC_CLOCKS_PROPERTIES  properies[CLOCK_PROFILE_NUMBER] = {
/* CLOCK_PROFILE_0 */
{
.plls 		= 	{  
						/*PLL_0 */
				{.src=BL_XTAL, .k=4 , .n=255, .m=0}, 
						/*PLL_1 */ 
				{.src=BL_XTAL, .k= 0, .n=82 , .m=0},
						/*PLL_2 */
				{.src=BL_NIF_TX_CLK, .k=4,  .n=14,  .m=0}
				},

.clients	=	{
						/* MIPSC */ 
				{.pll=0,  .m=1,   .pre_m=-1},
						/* MIPSD */
				{.pll=0,  .m=1,   .pre_m=-1},
						/* NAND */
				{.pll=0,  .m=2,   .pre_m=-1},
						/* IPCLOCK */
				{.pll=3,  .m=0,   .pre_m=-1},
						/* OTP */
				{.pll=0,  .m=5,   .pre_m=-1}, 
						/* PCI_HOST */
				{.pll=0,  .m=2,   .pre_m=-1},
						/* CDR */
				{.pll=0,  .m=1,   .pre_m=-1},
						/* TM */
				{.pll=1,  .m=1,   .pre_m=-1},
						/* RNR */
				{.pll=1,  .m=3,   .pre_m=-1},
						/* DDR */
				{.pll=1,  .m=3,   .pre_m=-1},
						/* DDR_BRIDGE */
				{.pll=1,  .m=1,   .pre_m=-1}, 
						/* USB_CORE */
				{.pll=2,  .m=24,  .pre_m =4},
						/* USB_HOST */
				{.pll=0,  .m=5,   .pre_m=-1},
						/* DSP */
				{.pll=0,  .m=2,   .pre_m=-1},
						/* SPI_FLASH */
				{.pll=0,  .m=1,   .pre_m=-1},
						/* TDM_SYNCE */
				{.pll=0,  .m=200, .pre_m =0},
						/* TDMRX */
				{.pll=0,  .m=242, .pre_m =1},
						/* TDMTX */
				{.pll=0,  .m=242, .pre_m =1}
				},

.tdm = TDM_MODE_E1,
.periph = 3
},
/* end of CLOCK_PROFILE_0 */

/* CLOCK_PROFILE_1 */
{
.plls 		= 	{  
						/*PLL_0 */
				{.src=BL_XTAL, .k=4 , .n=255, .m=0}, 
						/*PLL_1 */ 
				{.src=BL_XTAL, .k= 0, .n=81 , .m=0},
						/*PLL_2 */
				{.src=BL_NIF_TX_CLK, .k=4,  .n=14,  .m=0}
				},

.clients	=	{
						/* MIPSC */ 
				{.pll=0,  .m=1,   .pre_m=-1},
						/* MIPSD */
				{.pll=0,  .m=1,   .pre_m=-1},
						/* NAND */
				{.pll=0,  .m=2,   .pre_m=-1},
						/* IPCLOCK */
				{.pll=3,  .m=0,   .pre_m=-1},
						/* OTP */
				{.pll=0,  .m=5,   .pre_m=-1}, 
						/* PCI_HOST */
				{.pll=1,  .m=3,   .pre_m=-1},
						/* CDR */
				{.pll=0,  .m=1,   .pre_m=-1},
						/* TM */
				{.pll=1,  .m=1,   .pre_m=-1},
						/* RNR */
				{.pll=1,  .m=1,   .pre_m=-1},
						/* DDR */
				{.pll=1,  .m=2,   .pre_m=-1},
						/* DDR_BRIDGE */
				{.pll=1,  .m=1,   .pre_m=-1}, 
						/* USB_CORE */
				{.pll=2,  .m=24,  .pre_m =4},
						/* USB_HOST */
				{.pll=0,  .m=5,   .pre_m=-1},
						/* DSP */
				{.pll=0,  .m=2,   .pre_m=-1},
						/* SPI_FLASH */
				{.pll=0,  .m=1,   .pre_m=-1},
						/* TDM_SYNCE */
				{.pll=0,  .m=200, .pre_m =0},
						/* TDMRX */
				{.pll=0,  .m=242, .pre_m =1},
						/* TDMTX */
				{.pll=0,  .m=242, .pre_m =1}
				},

.tdm = TDM_MODE_E1,
.periph = 3
},
/* end of CLOCK_PROFILE_1 */

/* CLOCK_PROFILE_2 */
{
.plls 		= 	{  
						/*PLL_0 */
				{.src=BL_XTAL, .k=4 , .n=255, .m=0}, 
						/*PLL_1 */ 
				{.src=BL_XTAL, .k= 0, .n=81 , .m=0},
						/*PLL_2 */
				{.src=BL_NIF_TX_CLK, .k=4,  .n=14,  .m=0}
				},

.clients	=	{
						/* MIPSC */ 
				{.pll=0,  .m=0,   .pre_m=-1},
						/* MIPSD */
				{.pll=0,  .m=0,   .pre_m=-1},
						/* NAND */
				{.pll=0,  .m=2,   .pre_m=-1},
						/* IPCLOCK */
				{.pll=3,  .m=0,   .pre_m=-1},
						/* OTP */
				{.pll=0,  .m=5,   .pre_m=-1}, 
						/* PCI_HOST */
				{.pll=1,  .m=3,   .pre_m=-1},
						/* CDR */
				{.pll=0,  .m=1,   .pre_m=-1},
						/* TM */
				{.pll=1,  .m=1,   .pre_m=-1},
						/* RNR */
				{.pll=1,  .m=1,   .pre_m=-1},
						/* DDR */
				{.pll=1,  .m=2,   .pre_m=-1},
						/* DDR_BRIDGE */
				{.pll=1,  .m=1,   .pre_m=-1}, 
						/* USB_CORE */
				{.pll=2,  .m=24,  .pre_m =4},
						/* USB_HOST */
				{.pll=0,  .m=5,   .pre_m=-1},
						/* DSP */
				{.pll=0,  .m=2,   .pre_m=-1},
						/* SPI_FLASH */
				{.pll=0,  .m=1,   .pre_m=-1},
						/* TDM_SYNCE */
				{.pll=0,  .m=200, .pre_m =0},
						/* TDMRX */
				{.pll=0,  .m=242, .pre_m =1},
						/* TDMTX */
				{.pll=0,  .m=242, .pre_m =1}
				},

.tdm = TDM_MODE_E1,
.periph = 3
}
/* end of CLOCK_PROFILE_2 */
};

static BL_LILAC_SOC_ID socId = BL_XXXXX;

BL_LILAC_SOC_ID bl_lilac_soc_id()
{
	if(socId == BL_XXXXX)
	{
		MIPS_BLOCKS_VPB_OTP_CNTRL_DTE control;
		MIPS_BLOCKS_VPB_OTP_STATUS_DTE status;
		MIPS_BLOCKS_VPB_OTP_ADDR_DTE addr;
		MIPS_BLOCKS_VPB_OTP_DATA_DTE data;
		MIPS_BLOCKS_VPB_OTP_LOCK_DTE lock;

		BL_MIPS_BLOCKS_VPB_OTP_LOCK_READ(lock);
		if(!lock.otp_lock)
		{
			BL_MIPS_BLOCKS_VPB_OTP_STATUS_READ(status);
			if((status.otp_init_done) && (!status.otp_bsy)) 
			{
				BL_MIPS_BLOCKS_VPB_OTP_CNTRL_READ(control);
				addr.otp_byte_addr = 0x1f;
				BL_MIPS_BLOCKS_VPB_OTP_ADDR_WRITE(addr);
				control.otp_rd_wr = 0; /* 0 is read command */
				control.otp_go = 1;
				BL_MIPS_BLOCKS_VPB_OTP_CNTRL_WRITE(control);
				BL_MIPS_BLOCKS_VPB_OTP_STATUS_READ(status);
				
				while(status.otp_bsy) 
				{
					BL_MIPS_BLOCKS_VPB_OTP_STATUS_READ(status);
				}
				
				BL_MIPS_BLOCKS_VPB_OTP_DATA_READ(data);

				socId = data.otp_data;
			}
		}
		
		if(socId == BL_00000)
		{
#if defined CONFIG_BL_23515
			socId = BL_23515;
#elif defined CONFIG_BL_23516
			socId = BL_23516;
#elif defined CONFIG_BL_23530
			socId = BL_23530;
#elif defined CONFIG_BL_23533
			socId = BL_23531;
#elif defined CONFIG_BL_23570
			socId = BL_23570;
#elif defined CONFIG_BL_23571
			socId = BL_23571;
#elif defined CONFIG_BL_25580
			socId = BL_25580;
#elif defined CONFIG_BL_25590
			socId = BL_25590;
#elif defined CONFIG_BL_25550
			socId = BL_25550;
#endif
		}
	}
	
	return socId;
}
EXPORT_SYMBOL(bl_lilac_soc_id);


static BL_LILAC_CLOCK_PROFILE_ID bl_lilac_get_profile_id(void)
{
	switch(bl_lilac_soc_id())
	{
		case BL_23515:
		case BL_23516:
		case BL_23531:
		case BL_23571:
		default:
			return CLOCK_PROFILE_0;
			
		case BL_23530:
		case BL_23570:
			return CLOCK_PROFILE_1;
			
		case BL_25550:
		case BL_25580:
		case BL_25590:
			return CLOCK_PROFILE_2;
			
		case BL_00000:
			if(bl_lilac_ocp_divider())
				return CLOCK_PROFILE_2;
			else
				return CLOCK_PROFILE_1;
			break;
	}
}

BL_LILAC_SOC_STATUS bl_lilac_pll_properties(BL_LILAC_PLL  pll, BL_LILAC_PLL_PROPERTIES* props)
{
	BL_LILAC_CLOCK_PROFILE_ID profile_id;
	
	if(!props || pll < 0 || pll >= BL_PLL_NUM)
		return BL_LILAC_SOC_INVALID_PARAM;
	
	profile_id  = bl_lilac_get_profile_id();
	
	props->src = properies[profile_id].plls[pll].src;
	props->k = properies[profile_id].plls[pll].k;
	props->m = properies[profile_id].plls[pll].m;
	props->n = properies[profile_id].plls[pll].n;
	
	return BL_LILAC_SOC_OK;
	
}
EXPORT_SYMBOL(bl_lilac_pll_properties);


BL_LILAC_SOC_STATUS bl_lilac_pll_client_properties(BL_LILAC_PLL_CLIENT client, BL_LILAC_PLL_CLIENT_PROPERTIES* props)
{
	BL_LILAC_CLOCK_PROFILE_ID profile_id;
	
	if(!props || client < 0 || client >= PLL_CLIENT_NUM)
		return BL_LILAC_SOC_INVALID_PARAM;
	
	profile_id  = bl_lilac_get_profile_id();

	props->pll	= properies[profile_id].clients[client].pll;
	props->m		= properies[profile_id].clients[client].m;
	props->pre_m	= properies[profile_id].clients[client].pre_m;
	
	return BL_LILAC_SOC_OK;

}
EXPORT_SYMBOL(bl_lilac_pll_client_properties);

BL_LILAC_TDM_MODE  bl_lilac_tdm_mode(void)
{
	return properies[bl_lilac_get_profile_id()].tdm;
}
EXPORT_SYMBOL(bl_lilac_tdm_mode);

int bl_lilac_ocp_divider(void)
{
	unsigned long straps;
	
	BL_CR_CONTROL_REGS_STRPL_READ(straps);

	return (straps >> 14) & 1;
}
EXPORT_SYMBOL(bl_lilac_ocp_divider);

int bl_lilac_periph_divider(void)
{
	return properies[bl_lilac_get_profile_id()].periph;
}
EXPORT_SYMBOL(bl_lilac_periph_divider);

