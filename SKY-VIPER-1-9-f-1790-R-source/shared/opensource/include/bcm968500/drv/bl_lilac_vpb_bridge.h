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
/*                                                                         	*/
/* VPB BRIDGE driver                     									*/
/*                                                                         	*/
/****************************************************************************/

#ifndef _BL_LILAC_VPB_H_
#define _BL_LILAC_VPB_H_

#ifdef __cplusplus
extern "C" {
#endif

/*----------------------------------------------------------------------*/
/* ERROR DEFINITIONS */
/*----------------------------------------------------------------------*/
typedef enum
{
	BL_LILAC_VPB_OK				= 0,
	BL_LILAC_VPB_INVALID_PARAM	= 1
} BL_LILAC_VPB_STATUS;


/*----------------------------------------------------------------------*/
/* Mode Select register - MODSEL                                        */
/*----------------------------------------------------------------------*/
typedef enum
{
    BL_LILAC_VPB_ETH_MODE 	= 0,
    BL_LILAC_VPB_WAN_UBB_SEL,
    BL_LILAC_VPB_E4_MODE

} BL_LILAC_VPB_MODESEL_FIELD;

typedef enum {
	/* Field BL_LILAC_VPB_ETH_MODE values */
	BL_LILAC_VPB_ETH_MODE_SGMII		= 0,
	BL_LILAC_VPB_ETH_MODE_HISGMII	= 1,
    BL_LILAC_VPB_ETH_MODE_QSGMII	= 2,
	BL_LILAC_VPB_ETH_MODE_SMII		= 3,

	/* Field BL_LILAC_VPB_WAN_UBB_SEL values */
	BL_LILAC_VPB_WAN_UBB_SEL_GPON	= 0,
	BL_LILAC_VPB_WAN_UBB_SEL_UBB	= 1,

	/* Field BL_LILAC_VPB_E4_MODE values */
	BL_LILAC_VPB_E4_MODE_RGMII		= 0,
	BL_LILAC_VPB_E4_MODE_TMII		= 1

} BL_LILAC_VPB_MODESEL_FIELD_VALUE;

typedef enum
{
	BL_LILAC_VPB_AMD2SEL_S1588_VALUE	= 0,
	BL_LILAC_VPB_AMD2SEL_AMD2_VALUE		= 1
	
} BL_LILAC_VPB_AMD2SEL_VALUE;

/*----------------------------------------------------------------------*/
/* Set MODESEL register field value */
/*----------------------------------------------------------------------*/
BL_LILAC_VPB_STATUS bl_lilac_vpb_modesel_field_set(BL_LILAC_VPB_MODESEL_FIELD		field,
                                                   BL_LILAC_VPB_MODESEL_FIELD_VALUE	value);

/*----------------------------------------------------------------------*/
/* Query MODESEL register field value */
/*----------------------------------------------------------------------*/
BL_LILAC_VPB_STATUS bl_lilac_vpb_modesel_field_query(BL_LILAC_VPB_MODESEL_FIELD			field,
                                                     BL_LILAC_VPB_MODESEL_FIELD_VALUE*	value);
/*----------------------------------------------------------------------*/
/* Set amd2sel enable/disable */
/*----------------------------------------------------------------------*/
BL_LILAC_VPB_STATUS bl_lilac_vpb_amd2sel_set( BL_LILAC_VPB_AMD2SEL_VALUE	value);

/*----------------------------------------------------------------------*/
/* Query amd2sel */
/*----------------------------------------------------------------------*/
BL_LILAC_VPB_AMD2SEL_VALUE bl_lilac_vpb_amd2sel_query(void);




#ifdef __cplusplus
}
#endif

#endif /* #ifndef _BL_LILAC_VPB_H_ */
