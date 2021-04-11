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
/* Generic SOC definitions                									*/
/*                                                                         	*/
/****************************************************************************/

#ifndef _BL_LILAC_SOC_H_
#define _BL_LILAC_SOC_H_
/*--------------------------------------------------------
    Supported  LILAC SOC IDs 
 --------------------------------------------------------*/
typedef enum
{
	BL_00000 = 0x00, // generic sample
	BL_23515 = 0x0c,
	BL_23516 = 0x0d,
	BL_23530 = 0x03,
	BL_23531 = 0x0a,
	BL_23570 = 0x01,
	BL_23571 = 0x0b,
	
	BL_25550 = 0x82, 
	BL_25580 = 0x81,
	BL_25590 = 0x85,
	
	BL_XXXXX = 0xffff // read ID error
}BL_LILAC_SOC_ID;

/*--------------------------------------------------------
     LILAC Inernal controller ID 
 --------------------------------------------------------*/
typedef enum {
    MIPSC,
    MIPSD,
    RUNNER,
    DSP
} BL_LILAC_CONTROLLER;

/*--------------------------------------------------------
     generic return codes 
 --------------------------------------------------------*/
typedef enum
{
	BL_LILAC_SOC_OK				= 0,
	BL_LILAC_SOC_INVALID_PARAM	= 1,
	BL_LILAC_SOC_ERROR			= 200
	
} BL_LILAC_SOC_STATUS;

/*--------------------------------------------------------
     LILAC PLL IDs 
 --------------------------------------------------------*/
typedef enum
{
	BL_PLL_0 = 0,
	BL_PLL_1,
	BL_PLL_2,
	BL_PLL_NIF_TX_CLK,
	BL_PLL_NUM
}BL_LILAC_PLL;

/*--------------------------------------------------------
     LILAC PLL SOURCE IDs (values matter) 
 --------------------------------------------------------*/
typedef enum
{
	BL_XTAL = 0,
	BL_TCXO,
	BL_GPON_REF,
	BL_NIF_FREE_RUN_CLK,
	BL_NIF_TX_CLK,
	BL_PLL_SRC_NUM
}BL_LILAC_PLL_SRC;

/*--------------------------------------------------------
 LILAC PLL deviders , resalt_clock = input ((k+1)*(m+1))/(n+1) 
 --------------------------------------------------------*/
typedef struct 
{
	int src;
	int k;
	int n;
	int m;
	
}BL_LILAC_PLL_PROPERTIES;

/*--------------------------------------------------------
 LILAC PLL clients properties definitions
 --------------------------------------------------------*/
typedef struct
{
	BL_LILAC_PLL 		pll;     /* source pll 						*/
	int 				m;		 /* m divider 						*/
	int 				pre_m;	 /* pre_m divider , if applicable 	*/ 
} BL_LILAC_PLL_CLIENT_PROPERTIES;

/*--------------------------------------------------------
 LILAC PLL clients ID, make sure that in case of change in this definitions you 
 also adjusted BL_LILAC_CLOCKS_PROPERTIES initialization according 
 --------------------------------------------------------*/
typedef enum
{
	PLL_CLIENT_MIPSC,
	PLL_CLIENT_MIPSD,
	PLL_CLIENT_NAND,
	PLL_CLIENT_IPCLOCK,
	PLL_CLIENT_OTP,
	PLL_CLIENT_PCI_HOST,
	PLL_CLIENT_CDR,
	PLL_CLIENT_TM,
	PLL_CLIENT_RNR,
	PLL_CLIENT_DDR,
	PLL_CLIENT_DDR_BRIDGE,
	PLL_CLIENT_USB_CORE,
	PLL_CLIENT_USB_HOST,
	PLL_CLIENT_DSP,
	PLL_CLIENT_SPI_FLASH,
	PLL_CLIENT_TDM_SYNCE,
	PLL_CLIENT_TDMRX,
	PLL_CLIENT_TDMTX,
	PLL_CLIENT_NUM
	
}BL_LILAC_PLL_CLIENT;

/*--------------------------------------------------------
 LILAC SOC TDM MODE
 --------------------------------------------------------*/
typedef enum
{
	TDM_MODE_E1,
	TDM_MODE_T1
}BL_LILAC_TDM_MODE;

/***************************************************************************
     Return LILAC SOC Id                                               
***************************************************************************/
BL_LILAC_SOC_ID  bl_lilac_soc_id(void);


/***************************************************************************
     Return pll deviders according to curren profile                      
***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_pll_properties(BL_LILAC_PLL pll, BL_LILAC_PLL_PROPERTIES* props);


/***************************************************************************
     Return pll client definitions  according to curren profile                                    
***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_pll_client_properties(BL_LILAC_PLL_CLIENT client, BL_LILAC_PLL_CLIENT_PROPERTIES* props);

/***************************************************************************
     Return tdm mode  according to curren profile                                    
***************************************************************************/
BL_LILAC_TDM_MODE   bl_lilac_tdm_mode(void);

/***************************************************************************
     Return ocp divider encoding according to strap options                                    
***************************************************************************/
int bl_lilac_ocp_divider(void);


/***************************************************************************
     Return peripheral bus devider encoding                                  
***************************************************************************/
int bl_lilac_periph_divider(void);





#endif
