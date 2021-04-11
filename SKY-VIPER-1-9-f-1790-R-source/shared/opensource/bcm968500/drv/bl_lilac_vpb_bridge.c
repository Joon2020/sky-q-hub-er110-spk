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
#include "bl_lilac_vpb_bridge.h"
#include "bl_lilac_mips_blocks.h"

BL_LILAC_VPB_STATUS bl_lilac_vpb_modesel_field_set(BL_LILAC_VPB_MODESEL_FIELD field, BL_LILAC_VPB_MODESEL_FIELD_VALUE value)
{
	volatile MIPS_BLOCKS_VPB_BRIDGE_MODESEL_DTE modesel;
	unsigned long flags = 0;
	
	BL_LOCK_SECTION(flags);
	
	BL_MIPS_BLOCKS_VPB_BRIDGE_MODESEL_READ(modesel);
	
	switch(field)
	{
		case BL_LILAC_VPB_ETH_MODE:
			modesel.ethm = value;
			break;
			
		case BL_LILAC_VPB_WAN_UBB_SEL:
			modesel.wan_ubb_sel = value;
			break;
			
		case BL_LILAC_VPB_E4_MODE:
			modesel.e4_mode = value;
			break;
			
		default:
			BL_UNLOCK_SECTION(flags);
			return BL_LILAC_VPB_INVALID_PARAM;
	}
	
	BL_MIPS_BLOCKS_VPB_BRIDGE_MODESEL_WRITE(modesel);
	
	BL_UNLOCK_SECTION(flags);
	
	return BL_LILAC_VPB_OK;
}
EXPORT_SYMBOL(bl_lilac_vpb_modesel_field_set);

/* Query MODESEL register field value */
BL_LILAC_VPB_STATUS bl_lilac_vpb_modesel_field_query(BL_LILAC_VPB_MODESEL_FIELD field, BL_LILAC_VPB_MODESEL_FIELD_VALUE* value)
{
	volatile MIPS_BLOCKS_VPB_BRIDGE_MODESEL_DTE modesel;
	
	BL_MIPS_BLOCKS_VPB_BRIDGE_MODESEL_READ(modesel);
	
	switch(field)
	{
		case BL_LILAC_VPB_ETH_MODE:
			*value = modesel.ethm;
			break;
			
		case BL_LILAC_VPB_WAN_UBB_SEL:
			*value = modesel.wan_ubb_sel;
			break;
			
		case BL_LILAC_VPB_E4_MODE:
			*value = modesel.e4_mode;
			break;
			
		default:
			return BL_LILAC_VPB_INVALID_PARAM;
	}
	
	return BL_LILAC_VPB_OK;
}
EXPORT_SYMBOL(bl_lilac_vpb_modesel_field_query);


/*----------------------------------------------------------------------*/
/* Set amd2sel enable/disable */
/*----------------------------------------------------------------------*/
BL_LILAC_VPB_STATUS bl_lilac_vpb_amd2sel_set( BL_LILAC_VPB_AMD2SEL_VALUE	value)
{
	volatile MIPS_BLOCKS_VPB_BRIDGE_AMD2SEL_DTE amd2sel;
	
	BL_MIPS_BLOCKS_VPB_BRIDGE_AMD2SEL_READ(amd2sel);
	amd2sel.amd2sel = value;
	BL_MIPS_BLOCKS_VPB_BRIDGE_AMD2SEL_WRITE(amd2sel);
	
	return BL_LILAC_VPB_OK;
}
EXPORT_SYMBOL(bl_lilac_vpb_amd2sel_set);

/*----------------------------------------------------------------------*/
/* Query amd2sel */
/*----------------------------------------------------------------------*/
BL_LILAC_VPB_AMD2SEL_VALUE bl_lilac_vpb_amd2sel_query(void)
{
	volatile MIPS_BLOCKS_VPB_BRIDGE_AMD2SEL_DTE amd2sel;
	BL_MIPS_BLOCKS_VPB_BRIDGE_AMD2SEL_READ(amd2sel);
	return amd2sel.amd2sel;
}
EXPORT_SYMBOL(bl_lilac_vpb_amd2sel_query);



