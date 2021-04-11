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

/***************************************************************************/
/*                                                                         */
/* MAC driver for Lilac Project                                            */
/*                                                                         */
/* Title                                                                   */
/*                                                                         */
/*   MAC driver C File                                                     */
/*                                                                         */
/* Abstract                                                                */
/*                                                                         */
/*   The driver provides API to access Mentor Graphics device. Driver      */
/*   supports intialization of the device.                                 */
/*   Mode of the device is specified in the configuration structure        */
/*   Initialization function resets the MAC, releases the MAC from reset,  */
/*   setups required by the mode and specified configuration registers     */
/*   This API is not reentrant. Typically application calls the            */
/*   initialization function only once.                                    */
/*   Make sure to reset Ethernet PHY before calling the initialization     */
/*   function.                                                             */
/***************************************************************************/

/*  Revision history                                                  */
/*  01/01/10: Creation, Asi Bieda (Version 1.0.0)*/
/*  2/03/10: Fixes after review, Asi Bieda (Version 1.0.1)*/



#include "bl_os_wraper.h"
#include "bl_lilac_drv_mac.h"
#include "bl_lilac_net_if.h"
#include "bl_lilac_mips_blocks.h"


#define DEBUG

#ifdef DEBUG
#  define PRINT_DEBUG(fmt, args...) printk(fmt, ##args);
#else
#  define PRINT_DEBUG(fmt, args...)
#endif

#define CS_NIBBLE_MODE		1 
#define CS_BYTE_MODE		2


/*********************************************************************/
/*                   Default values Definishens                      */
/*********************************************************************/

#define CS_DRV_MAC_MAX_MTU      ( 0x600 )
#define CS_DRV_MAC_CFGFTTH      ( 0x200 )
#define CS_DRV_MAC_HSTFLTRFRM   ( 0x18019 )
#define CS_DRV_MAC_HSTFLTRFRMDC ( 0x2ffe7 )
#define CS_DRV_MAC_CFGFRTH      ( 0x4 )

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_configuration                                    */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Set Configuration                                  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function     sets the configuration for specific EMAC               */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2 , IPG_IFG , FIFO_REG_0 , FIFO_REG_1, FIFO_REG_3, FIFO_REG_4,   */
/*     FIFO_REG_5, GR_0, GR_1 , GR_2                                          */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_mac_conf - EMAC configuration (struct)                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_configuration ( DRV_MAC_NUMBER xi_index, 
                                             const DRV_MAC_CONF * xi_mac_conf )
{
    NET_IF_CORE_MENTOR_MACCFG2_DTE    maccfg2   ;
    NET_IF_CORE_MENTOR_IPG_IFG_DTE    ipg_ifg   ;
    NET_IF_CORE_MENTOR_FIFO_REG_0_DTE fifo_reg  ;
    NET_IF_CORE_MENTOR_FIFO_REG_1_DTE fifo1_reg ;
    NET_IF_CORE_MENTOR_FIFO_REG_3_DTE fifo3_reg ;
    NET_IF_CORE_MENTOR_FIFO_REG_4_DTE fifo4_reg ;
    NET_IF_CORE_MENTOR_FIFO_REG_5_DTE fifo5_reg ;
    NET_IF_CORE_GENERAL_ETH_GR_0_DTE  gr0_reg   ;
    NET_IF_CORE_GENERAL_ETH_GR_1_DTE  gr1_reg   ;
    NET_IF_CORE_GENERAL_ETH_GR_2_DTE  gr2_reg   ;

    /*Set Rate*/
    pi_bl_drv_mac_set_rate ( xi_index, xi_mac_conf->rate );

    /*Set Duplex*/
    pi_bl_drv_mac_set_duplex ( xi_index , xi_mac_conf->full_duplex );

    /*Set Flow control*/
    pi_bl_drv_mac_set_flow_control ( xi_index ,xi_mac_conf->flow_control_tx, xi_mac_conf->flow_control_rx ) ;

    /* MTU  configuration */ 
    pi_bl_drv_mac_set_max_frame_length (xi_index ,  xi_mac_conf ->max_frame_length ); 

	/* =============  maccfg2 configuration ==============*/ 

    BL_NET_IF_CORE_MENTOR_MACCFG2_READ( xi_index, maccfg2 );

    maccfg2.len_check  = ( xi_mac_conf->disable_check_frame_length == 0 ? DRV_MAC_ENABLE : DRV_MAC_DISABLE ) ;
    maccfg2.crc_en     = ( xi_mac_conf->disable_crc == 0 ? DRV_MAC_ENABLE : DRV_MAC_DISABLE ) ;
    maccfg2.pad_crc_en = ( xi_mac_conf->disable_pading == 0  ? DRV_MAC_ENABLE : DRV_MAC_DISABLE ) ;
    maccfg2.prmbl_len = ( xi_mac_conf->enable_preamble_length == 0  ? CE_NET_IF_CORE_MENTOR_MACCFG2_PRMBL_LEN_DEFAULT_VALUE : ( xi_mac_conf->preamble_length & 0x0f ) ) ;
    maccfg2.huge_frm_en = xi_mac_conf->allow_huge_frame ;	

    BL_NET_IF_CORE_MENTOR_MACCFG2_WRITE( xi_index, maccfg2 );

	/* =============  ipg_ifg configuration ==============*/ 

    BL_NET_IF_CORE_MENTOR_IPG_IFG_READ( xi_index, ipg_ifg );

	ipg_ifg.nbtb_ipg_p1 = CE_NET_IF_CORE_MENTOR_IPG_IFG_NBTB_IPG_P1_DEFAULT_VALUE;
	ipg_ifg.nbtb_ipg_p2 = CE_NET_IF_CORE_MENTOR_IPG_IFG_NBTB_IPG_P2_DEFAULT_VALUE;
	ipg_ifg.min_ifg_enf = CE_NET_IF_CORE_MENTOR_IPG_IFG_MIN_IFG_ENF_DEFAULT_VALUE;
	ipg_ifg.btb_ipg		= CE_NET_IF_CORE_MENTOR_IPG_IFG_BTB_IPG_DEFAULT_VALUE;

	if ( xi_mac_conf -> set_back2back_gap !=  0 )
	{
		ipg_ifg.btb_ipg = xi_mac_conf->back2back_gap;
	}

	if ( xi_mac_conf -> set_non_back2back_gap  !=  0 )
	{
		ipg_ifg.nbtb_ipg_p2 = xi_mac_conf->non_back2back_gap;
	}

	if ( xi_mac_conf -> set_min_interframe_gap  !=  0 )
	{
		ipg_ifg.min_ifg_enf = xi_mac_conf->min_interframe_gap;
	}
	
    BL_NET_IF_CORE_MENTOR_IPG_IFG_WRITE( xi_index, ipg_ifg );


	/* =============  fifo reg 0 configuration ==============*/ 

    BL_NET_IF_CORE_MENTOR_FIFO_REG_0_READ( xi_index, fifo_reg );
	fifo_reg.ftfenreq = DRV_MAC_ENABLE ;
	fifo_reg.stfenreq = DRV_MAC_ENABLE;
	fifo_reg.frfenreq = DRV_MAC_ENABLE;
	fifo_reg.srfenreq = DRV_MAC_ENABLE;
	fifo_reg.wtmenreq = DRV_MAC_ENABLE;
	fifo_reg.hstrstft = DRV_MAC_DISABLE ;
	fifo_reg.hstrstst = DRV_MAC_DISABLE;
	fifo_reg.hstrstfr = DRV_MAC_DISABLE;
	fifo_reg.hstrstsr = DRV_MAC_DISABLE;
	fifo_reg.hstrstwt = DRV_MAC_DISABLE;
    BL_NET_IF_CORE_MENTOR_FIFO_REG_0_WRITE( xi_index, fifo_reg );
	
	/* =============  fifo reg 1 configuration ==============*/ 

    BL_NET_IF_CORE_MENTOR_FIFO_REG_1_READ( xi_index, fifo1_reg );

    /*This hex value represents the number of pause quanta (64 bit times) after an XOFF pause frame has been acknowledged (psack)
    until the amcxfif will reassert tcrq if the A-MCXFIF receive storage level has remained higher than the low watermark.*/
    fifo1_reg.cfgxoffrtx = CE_NET_IF_CORE_MENTOR_FIFO_REG_1_CFGXOFFRTX_DEFAULT_VALUE ;

    /*This hex value represents the minimum number of 4 byte locations that will be simultaneously stored in the receive RAM, 
     relative to the beginning of the frame being input, before frrdy may be asserted. Note that frrdy will be latent a certain
     amount of time due to fabric transmit clock to system transmit clock time domain crossing, and conditional on fracpt 
     assertion. When set to maximum value, frrd may be asserted only after the completion of the input frame. 
     The value of this register must be greater than 18d when hstdrplt64 is asserted.*/
    fifo1_reg.cfgfrth = CS_DRV_MAC_CFGFRTH; 

    BL_NET_IF_CORE_MENTOR_FIFO_REG_1_WRITE( xi_index, fifo1_reg );
		
	/* =============  fifo reg 3 configuration ==============*/ 

    BL_NET_IF_CORE_MENTOR_FIFO_REG_3_READ( xi_index, fifo3_reg );
    /*This hex value represents the maximum number of 4 byte locations that will be simultaneously stored in the transmit RAM 
     before fthwm will be asserted. Note that fthwm has two ftclk clock periods of latency before assertion or negation.
     This should be considered when calculating any headroom required for maximum size packets.*/

	fifo3_reg.cfghwmft = CE_NET_IF_CORE_MENTOR_FIFO_REG_3_CFGHWMFT_DEFAULT_VALUE;

    /*This hex value represents the minimum number of 4 byte locations that will be simultaneously stored in the transmit RAM, 
    relative to the beginning of the frame being input, before tpsf will be asserted. Note that tpsf will be latent a certain 
    amount of time due to fabric transmit clock to system transmit clock time domain crossing. When set to maximum value, 
    tpsf will be asserted only after the completion of the input frame.*/

    fifo3_reg.cfgftth = CE_NET_IF_CORE_MENTOR_FIFO_REG_3_CFGFTTH_DEFAULT_VALUE;  

    BL_NET_IF_CORE_MENTOR_FIFO_REG_3_WRITE( xi_index, fifo3_reg );

	/* =============  fifo reg 4 configuration ==============*/ 

    BL_NET_IF_CORE_MENTOR_FIFO_REG_4_READ( xi_index, fifo4_reg );

    /*These configuration bits are used to signal the drop frame conditions internal to the A-MCXFIF*/
	if ( xi_mac_conf -> drop_mask == 0 )
	{
		fifo4_reg.hstfltrfrm = CS_DRV_MAC_HSTFLTRFRM; 
	}
	else
	{
		fifo4_reg.hstfltrfrm = xi_mac_conf -> drop_mask ; 
	}
    BL_NET_IF_CORE_MENTOR_FIFO_REG_4_WRITE( xi_index, fifo4_reg );
	
	/* =============  fifo reg 5 configuration ==============*/ 

    BL_NET_IF_CORE_MENTOR_FIFO_REG_5_READ( xi_index, fifo5_reg );

	/* These configuration bits indicate which Receive Statistics Vectors - 
	all bits should always be set to 1.
	are dont cares for A-MCXFIF frame drop circuitry. The bits
	correspond to the Receive Statistics Vector on a one per one
	basis. For example bit 0 corresponds to RSV[16], and bit 1
	corresponds to RSV[17]. Setting of a hstfltrfrmdc bit, will indicate
	a dont care for that RSV bit. Clearing the bit will look for a
	matching level on the corresponding hstfltrfrm bit. If a match is
	made then the frame is dropped. All bits should be set when
	cfgfrth bits are not all set. */

	fifo5_reg.hstfltrfrmdc 	= CS_DRV_MAC_HSTFLTRFRMDC; 
	fifo5_reg.hstdrplt64 	= DRV_MAC_DISABLE;

    BL_NET_IF_CORE_MENTOR_FIFO_REG_5_WRITE( xi_index, fifo5_reg );
	
	/* =============  GR 0 configuration ==============*/ 

    BL_NET_IF_CORE_GENERAL_ETH_GR_0_READ( xi_index, gr0_reg );
	gr0_reg.bbh_thdf_en	= xi_mac_conf->flow_half_duplex_en;
	gr0_reg.bbh_tcrq_en	= xi_mac_conf->flow_full_duplex_en;
	gr0_reg.autoz		= DRV_MAC_ENABLE;
    BL_NET_IF_CORE_GENERAL_ETH_GR_0_WRITE( xi_index, gr0_reg );
	
	/* =============  GR 1 configuration ==============*/ 

    BL_NET_IF_CORE_GENERAL_ETH_GR_1_READ( xi_index, gr1_reg );    
	gr1_reg.sten	= DRV_MAC_ENABLE;
    BL_NET_IF_CORE_GENERAL_ETH_GR_1_WRITE( xi_index, gr1_reg );
	
	/* =============  GR 2 configuration ==============*/ 

    BL_NET_IF_CORE_GENERAL_ETH_GR_2_READ( xi_index, gr2_reg );    
    gr2_reg.host_cfpt	= CE_NET_IF_CORE_GENERAL_ETH_GR_2_HOST_CFPT_DEFAULT_VALUE;
    BL_NET_IF_CORE_GENERAL_ETH_GR_2_WRITE( xi_index, gr2_reg );   
			
}
EXPORT_SYMBOL ( pi_bl_drv_mac_set_configuration );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_configuration                                    */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Get Configuration                                  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the configuration for specific EMAC                   */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2 , IPG_IFG , FIFO_REG_0 , FIFO_REG_1, FIFO_REG_3, FIFO_REG_4,   */
/*     FIFO_REG_5, GR_0, GR_1 , GR_2                                          */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_mac_conf - EMAC configuration (struct)                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_configuration ( DRV_MAC_NUMBER xi_index, 
                                             DRV_MAC_CONF * const xo_mac_conf ) 
{

    NET_IF_CORE_MENTOR_MACCFG2_DTE    maccfg2   ;
    NET_IF_CORE_MENTOR_IPG_IFG_DTE    ipg_ifg   ;
    NET_IF_CORE_MENTOR_FIFO_REG_3_DTE fifo3_reg ;
    NET_IF_CORE_MENTOR_FIFO_REG_4_DTE fifo4_reg ;
    NET_IF_CORE_MENTOR_IFC_CONTROL_DTE ifc_control;
    NET_IF_CORE_GENERAL_XOFF_CTRL_DTE xoff_control ;

    BL_NET_IF_CORE_MENTOR_MACCFG2_READ( xi_index, maccfg2 );
    BL_NET_IF_CORE_MENTOR_IPG_IFG_READ( xi_index, ipg_ifg );
    BL_NET_IF_CORE_MENTOR_FIFO_REG_3_READ( xi_index, fifo3_reg );
    BL_NET_IF_CORE_MENTOR_FIFO_REG_4_READ( xi_index, fifo4_reg );
    BL_NET_IF_CORE_MENTOR_IFC_CONTROL_READ( xi_index, ifc_control );
    BL_NET_IF_CORE_GENERAL_XOFF_CTRL_READ( xi_index, xoff_control );
#if !defined(_CFE_)
    /*Get Rate*/
    pi_bl_drv_mac_get_rate ( xi_index, &xo_mac_conf->rate );

    /*Get Duplex*/
    pi_bl_drv_mac_get_duplex (xi_index , &xo_mac_conf->full_duplex );

    /*Get Flow control*/
    pi_bl_drv_mac_get_flow_control ( xi_index ,&xo_mac_conf->flow_control_tx, &xo_mac_conf->flow_control_rx ) ;

    /* Get MTU  */ 
    pi_bl_drv_mac_get_max_frame_length (xi_index , &xo_mac_conf->max_frame_length ); 
#endif
    /* Padding  */
    xo_mac_conf->disable_pading = ( maccfg2.pad_crc_en == 0 ? DRV_MAC_ENABLE : DRV_MAC_DISABLE ) ;	

    /* Check Frame Length , Preamble */
    xo_mac_conf->disable_check_frame_length = ( maccfg2.len_check == 0 ? DRV_MAC_ENABLE : DRV_MAC_DISABLE ) ;
    xo_mac_conf->disable_crc = ( maccfg2.crc_en == 0 ? DRV_MAC_ENABLE : DRV_MAC_DISABLE ) ;
	xo_mac_conf->enable_preamble_length = DRV_MAC_DISABLE;
    xo_mac_conf->preamble_length  = maccfg2.prmbl_len;
    xo_mac_conf->allow_huge_frame = maccfg2.huge_frm_en;
	xo_mac_conf->set_back2back_gap		= DRV_MAC_DISABLE;
    xo_mac_conf->back2back_gap			= ipg_ifg.btb_ipg;
    xo_mac_conf->set_non_back2back_gap	= DRV_MAC_DISABLE;
    xo_mac_conf->non_back2back_gap		= ipg_ifg.nbtb_ipg_p2;
    xo_mac_conf->set_min_interframe_gap	= DRV_MAC_DISABLE;
    xo_mac_conf->min_interframe_gap		= ipg_ifg.min_ifg_enf;
    
    /* FIFO_REG_3 */
    xo_mac_conf->max4bytes = fifo3_reg.cfghwmft;
    xo_mac_conf->min4bytes = fifo3_reg.cfgftth;
    
    /* FIFO_REG_4 */
    xo_mac_conf->drop_mask = fifo4_reg.hstfltrfrm;

    /* XOFF_CTRL */
    xo_mac_conf->xoff_periodic	= xoff_control.priodic_xoff_en;
    xo_mac_conf->xoff_timer		= xoff_control.timr_strt_val;
	
}
EXPORT_SYMBOL ( pi_bl_drv_mac_get_configuration );
#if !defined(_CFE_)
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_status                                           */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Get Emac Status                                    */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the RX and TX status for specific EMAC                */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_tx_enable - TX enable : 0 (Disable) | 1 Enable                        */
/*                                                                            */
/*   xo_rx_enable - RX enable : 0 (Disable) | 1 Enable                        */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_status ( DRV_MAC_NUMBER xi_index, 
                                      E_DRV_MAC_ENABLE * const xo_tx_enable, 
                                      E_DRV_MAC_ENABLE * const xo_rx_enable )
{
    NET_IF_CORE_MENTOR_MACCFG1_DTE maccfg1 ;

    BL_NET_IF_CORE_MENTOR_MACCFG1_READ( xi_index , maccfg1 ) ;
    * xo_tx_enable = maccfg1.tr_en ;
    * xo_rx_enable = maccfg1.rcv_enb ;
}
EXPORT_SYMBOL ( pi_bl_drv_mac_get_status );
#endif

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_enable                                               */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Enable MAC                                         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function Enables specific EMAC                                      */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_enable - MAC enable : 0 (Disable) | 1 Enable                          */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_enable ( DRV_MAC_NUMBER xi_index,
                                  E_DRV_MAC_ENABLE xi_enable ) 
{

    NET_IF_CORE_MENTOR_MACCFG1_DTE maccfg1 ;

    BL_NET_IF_CORE_MENTOR_MACCFG1_READ( xi_index , maccfg1 ) ;
    maccfg1.tr_en   = xi_enable ;
    maccfg1.rcv_enb = xi_enable ;
    BL_NET_IF_CORE_MENTOR_MACCFG1_WRITE( xi_index , maccfg1 ) ;

}
EXPORT_SYMBOL ( pi_bl_drv_mac_enable );


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_sw_reset                                             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Software reset                                     */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function set the SW reset bit of specific EMAC                      */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_reset - MAC reset : 0 (out of reset) | 1 (reset )                     */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_sw_reset ( DRV_MAC_NUMBER xi_index,
                                    E_DRV_MAC_ENABLE xi_reset ) 
{
    NET_IF_CORE_MENTOR_MACCFG1_DTE maccfg1 ;

    BL_NET_IF_CORE_MENTOR_MACCFG1_READ( xi_index , maccfg1 ) ;
    maccfg1.soft_rst = xi_reset ;
    BL_NET_IF_CORE_MENTOR_MACCFG1_WRITE( xi_index , maccfg1 ) ;
	
}
EXPORT_SYMBOL ( pi_bl_drv_mac_sw_reset );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_reset_bit                                        */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Get reset bit                                      */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function returns the SW reset bit of specific EMAC                  */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_reset - MAC reset : 0 (out of reset) | 1 (reset )                     */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_reset_bit ( DRV_MAC_NUMBER xi_index,
                                         E_DRV_MAC_ENABLE * const xo_reset_bit ) 
{
    NET_IF_CORE_MENTOR_MACCFG1_DTE maccfg1 ;

    BL_NET_IF_CORE_MENTOR_MACCFG1_READ( xi_index , maccfg1 ) ;
    * xo_reset_bit = maccfg1.soft_rst  ;
	
}
EXPORT_SYMBOL ( pi_bl_drv_mac_get_reset_bit );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_mac_mgmt_write                                           */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - MDIO Write                                         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function write data to the MDIO controling the PHY                  */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MII_MGMT_ADDR , MII_MGMT_IND , MII_MGMT_CMD                            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_phy_address - represents the 5-bit PHY Address of Mgmt cycles         */
/*                                                                            */
/*   xi_reg_address - 5-bit Register Address of Mgmt cycles                   */
/*                                                                            */
/*   xi_value - 16 bits value to write to the connected PHY                   */
/*                                                                            */
/* Output:   DRV_MAC_ERROR_DTE - Return code                                  */
/*                                                                            */
/******************************************************************************/
DRV_MAC_ERROR fi_bl_drv_mac_mgmt_write ( DRV_MAC_NUMBER xi_index, 
                                                            uint32_t xi_phy_address , 
                                                            uint32_t xi_reg_address , 
                                                            uint32_t xi_value ) 
{
	int		delay = 20000 ;
	volatile NET_IF_CORE_MENTOR_MII_MGMT_CTRL_DTE	mii_mgmt_cntrl_reg ;
	volatile NET_IF_CORE_MENTOR_MII_MGMT_ADDR_DTE	mii_mgmt_addr_reg ;
	volatile NET_IF_CORE_MENTOR_MII_MGMT_IND_DTE	indication ;

	BL_NET_IF_CORE_MENTOR_MII_MGMT_IND_READ ( xi_index , indication );
	
	while( ( indication.busy != 0 ) && ( delay-- > 0 ) )
	{
		udelay(10);
		BL_NET_IF_CORE_MENTOR_MII_MGMT_IND_READ ( xi_index , indication );
	}

	if( indication.busy != 0)
	{
		return DRV_MAC_ERROR_WRITE_MDIO_FAILED;
	}

	*(( uint32_t* )&mii_mgmt_cntrl_reg) = 0;
	*(( uint32_t* )&mii_mgmt_addr_reg)  = 0;

	/* set address register and initiate write cycle */
	mii_mgmt_addr_reg.phy_addr = xi_phy_address & 0x1F ;
	mii_mgmt_addr_reg.reg_addr = xi_reg_address & 0x1F ;
	BL_NET_IF_CORE_MENTOR_MII_MGMT_ADDR_WRITE( xi_index, mii_mgmt_addr_reg );
	udelay(20);

	mii_mgmt_cntrl_reg.mii_mgmt_ctrl = xi_value ;
	BL_NET_IF_CORE_MENTOR_MII_MGMT_CTRL_WRITE( xi_index, mii_mgmt_cntrl_reg );
	udelay(20);

	BL_NET_IF_CORE_MENTOR_MII_MGMT_IND_READ ( xi_index , indication );

	delay = 20000 ;
	while( ( indication.busy != 0 ) && ( delay-- > 0 ) )
	{
		udelay(10);
		BL_NET_IF_CORE_MENTOR_MII_MGMT_IND_READ( xi_index , indication );
	}
    
	if( indication.busy == 0 )
	{
		return ( DRV_MAC_NO_ERROR ) ;
	}

	return ( DRV_MAC_ERROR_WRITE_MDIO_FAILED ) ;
}
EXPORT_SYMBOL( fi_bl_drv_mac_mgmt_write );


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_mac_mgmt_read                                            */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - MDIO Read                                          */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function reads data from the MDIO controling the PHY                */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MII_MGMT_ADDR , MII_MGMT_IND , MII_MGMT_CMD                            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_phy_address - represents the 5-bit PHY Address of Mgmt cycles         */
/*                                                                            */
/*   xi_reg_address - 5-bit Register Address of Mgmt cycles                   */
/*                                                                            */
/*   xo_value - 16 bits value read from the connected PHY                     */
/*                                                                            */
/* Output:   DRV_MAC_ERROR_DTE - Return code                                  */
/*                                                                            */
/******************************************************************************/
DRV_MAC_ERROR  fi_bl_drv_mac_mgmt_read  ( DRV_MAC_NUMBER xi_index, 
                                                 int32_t xi_phy_address , 
                                                 int32_t xi_reg_address , 
                                                 uint32_t * const xo_value ) 
{
	int      delay = 20000 ;
	volatile NET_IF_CORE_MENTOR_MII_MGMT_ADDR_DTE	mii_mgmt_addr_reg ;
	volatile NET_IF_CORE_MENTOR_MII_MGMT_IND_DTE	indication ;
	volatile NET_IF_CORE_MENTOR_MII_MGMT_CMD_DTE	mii_mgmt_cmd_reg ;
	volatile NET_IF_CORE_MENTOR_MII_MGMT_STAT_DTE   mii_mgmt_stat_reg ;


	BL_NET_IF_CORE_MENTOR_MII_MGMT_IND_READ ( xi_index , indication );

	/* tight loop polling the completion status bit */
	while( ( indication.busy != 0 ) && ( delay-- > 0 ) )
	{
		udelay(10);
		BL_NET_IF_CORE_MENTOR_MII_MGMT_IND_READ ( xi_index , indication );
	}
	if( indication.busy != 0)
	{
		return DRV_MAC_ERROR_READ_MDIO_FAILED;
	}

	*(uint32_t*)&mii_mgmt_addr_reg = 0;
	*(uint32_t*)&mii_mgmt_cmd_reg = 0;

	/* load PHY address and regsiter address */ 
	mii_mgmt_addr_reg.phy_addr = xi_phy_address & 0x1F ;
	mii_mgmt_addr_reg.reg_addr = xi_reg_address & 0x1F ;
	BL_NET_IF_CORE_MENTOR_MII_MGMT_ADDR_WRITE ( xi_index, mii_mgmt_addr_reg );
	udelay(20);

	/* execute single read cylce */
	mii_mgmt_cmd_reg.read_cycle = 1 ;
	BL_NET_IF_CORE_MENTOR_MII_MGMT_CMD_WRITE ( xi_index , mii_mgmt_cmd_reg );
	udelay(20);

	mii_mgmt_cmd_reg.read_cycle = 0;
	BL_NET_IF_CORE_MENTOR_MII_MGMT_CMD_WRITE ( xi_index , mii_mgmt_cmd_reg );
	udelay(20);

	BL_NET_IF_CORE_MENTOR_MII_MGMT_IND_READ ( xi_index , indication );

	delay = 20000 ;
	while( ( indication.busy != 0 ) && ( delay-- > 0 ) )
	{
		udelay(10);
		BL_NET_IF_CORE_MENTOR_MII_MGMT_IND_READ ( xi_index , indication );
	}

	/* tight loop polling the completion status bit */
	if( ( indication.not_valid != 0 ) || ( indication.busy != 0 ) )
	{
		return ( DRV_MAC_ERROR_READ_MDIO_FAILED ) ;
	}

	BL_NET_IF_CORE_MENTOR_MII_MGMT_STAT_READ ( xi_index , mii_mgmt_stat_reg ); 
	* xo_value = mii_mgmt_stat_reg.mii_mgmt_stat;

	return ( DRV_MAC_NO_ERROR ) ;

}
EXPORT_SYMBOL ( fi_bl_drv_mac_mgmt_read );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_read_rx_counters                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Read RX Counters                                   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets all RX counters                                       */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_counters - RX counters (struct)                                       */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_read_rx_counters ( DRV_MAC_NUMBER xi_index, 
                                            DRV_MAC_RX_COUNTERS * const xo_counters ) 
{

    BL_NET_IF_CORE_MENTOR_RBYT_READ( xi_index , xo_counters -> bytes );  
    BL_NET_IF_CORE_MENTOR_RPKT_READ( xi_index , xo_counters -> packets);
    BL_NET_IF_CORE_MENTOR_RFCS_READ( xi_index , xo_counters -> fcs_errors );
    BL_NET_IF_CORE_MENTOR_RMCA_READ( xi_index , xo_counters -> multicast_packets );
    BL_NET_IF_CORE_MENTOR_RBCA_READ( xi_index , xo_counters -> broadcat_packets );
    BL_NET_IF_CORE_MENTOR_RXCF_READ( xi_index , xo_counters -> control_packets );
    BL_NET_IF_CORE_MENTOR_RXPF_READ( xi_index ,  xo_counters -> pause_packets);
    BL_NET_IF_CORE_MENTOR_RXUO_READ( xi_index , xo_counters -> unknown_op );  
    BL_NET_IF_CORE_MENTOR_RALN_READ( xi_index , xo_counters -> alignment_error);
    BL_NET_IF_CORE_MENTOR_RFLR_READ( xi_index , xo_counters -> length_error );
    BL_NET_IF_CORE_MENTOR_RCDE_READ( xi_index , xo_counters -> code_error );
    BL_NET_IF_CORE_MENTOR_RCSE_READ( xi_index , xo_counters -> carrier_sense_error );
    BL_NET_IF_CORE_MENTOR_RUND_READ( xi_index , xo_counters -> undersize_frames );
    BL_NET_IF_CORE_MENTOR_ROVR_READ( xi_index , xo_counters -> oversize_frames );
    BL_NET_IF_CORE_MENTOR_RFRG_READ( xi_index ,  xo_counters -> fragmets);
    BL_NET_IF_CORE_MENTOR_RJBR_READ( xi_index , xo_counters -> jabber_frames );  
    BL_NET_IF_CORE_MENTOR_ROFC_READ( xi_index , xo_counters -> overflow);
    BL_NET_IF_CORE_MENTOR_R64_READ( xi_index , xo_counters -> frame_64_bytes );
    BL_NET_IF_CORE_MENTOR_R127_READ( xi_index , xo_counters -> frame_65_127_bytes );
    BL_NET_IF_CORE_MENTOR_R255_READ( xi_index , xo_counters -> frame_128_255_bytes );
    BL_NET_IF_CORE_MENTOR_R511_READ( xi_index , xo_counters -> frame_256_511_bytes );
    BL_NET_IF_CORE_MENTOR_R1K_READ( xi_index , xo_counters -> frame_512_1023_bytes );
    BL_NET_IF_CORE_MENTOR_R1P5K_READ( xi_index ,  xo_counters -> frame_1024_1518_bytes);
    BL_NET_IF_CORE_MENTOR_RMTU_READ( xi_index ,  xo_counters -> frame_1519_mtu_bytes);

}
EXPORT_SYMBOL ( pi_bl_drv_mac_read_rx_counters );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_read_tx_counters                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Read TX Counters                                   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets all TX counters                                       */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*                                                                            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_counters - TX counters (struct)                                       */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_read_tx_counters ( DRV_MAC_NUMBER xi_index, 
                                            DRV_MAC_TX_COUNTERS * const xo_counters ) 
{

    BL_NET_IF_CORE_MENTOR_TBYT_READ( xi_index , xo_counters -> bytes );  
    BL_NET_IF_CORE_MENTOR_TPKT_READ( xi_index , xo_counters -> packets);
    BL_NET_IF_CORE_MENTOR_TFCS_READ( xi_index , xo_counters -> fcs_errors );
    BL_NET_IF_CORE_MENTOR_TMCA_READ( xi_index , xo_counters -> multicast_packets );
    BL_NET_IF_CORE_MENTOR_TBCA_READ( xi_index , xo_counters -> broadcat_packets );
    BL_NET_IF_CORE_MENTOR_TXCF_READ( xi_index , xo_counters -> control_frames );
    BL_NET_IF_CORE_MENTOR_TXPF_READ( xi_index ,  xo_counters -> pause_packets);
    BL_NET_IF_CORE_MENTOR_TDFR_READ( xi_index , xo_counters -> deferral_packets );  
    BL_NET_IF_CORE_MENTOR_TEDF_READ( xi_index , xo_counters -> excessive_packets);
    BL_NET_IF_CORE_MENTOR_TSCL_READ( xi_index , xo_counters -> single_collision );
    BL_NET_IF_CORE_MENTOR_TMCL_READ( xi_index , xo_counters -> multiple_collision );
    BL_NET_IF_CORE_MENTOR_TLCL_READ( xi_index , xo_counters -> late_collision );
    BL_NET_IF_CORE_MENTOR_TXCL_READ( xi_index , xo_counters -> excessive_collision );
    BL_NET_IF_CORE_MENTOR_TNCL_READ( xi_index ,  xo_counters -> total_collision);
    BL_NET_IF_CORE_MENTOR_TUND_READ( xi_index , xo_counters -> undersize_frames );
    BL_NET_IF_CORE_MENTOR_TOVR_READ( xi_index , xo_counters -> oversize_frames );
    BL_NET_IF_CORE_MENTOR_TFRG_READ( xi_index ,  xo_counters -> fragmets);
    BL_NET_IF_CORE_MENTOR_TJBR_READ( xi_index , xo_counters -> jabber_frames );  
    BL_NET_IF_CORE_MENTOR_TURC_READ( xi_index , xo_counters -> underrun );
    BL_NET_IF_CORE_MENTOR_TERC_READ( xi_index , xo_counters -> transmission_errors ); 
    BL_NET_IF_CORE_MENTOR_T64_READ( xi_index , xo_counters -> frame_64_bytes );
    BL_NET_IF_CORE_MENTOR_T127_READ( xi_index , xo_counters -> frame_65_127_bytes );
    BL_NET_IF_CORE_MENTOR_T255_READ( xi_index , xo_counters -> frame_128_255_bytes );
    BL_NET_IF_CORE_MENTOR_T511_READ( xi_index , xo_counters -> frame_256_511_bytes );
    BL_NET_IF_CORE_MENTOR_T1K_READ( xi_index , xo_counters -> frame_512_1023_bytes );
    BL_NET_IF_CORE_MENTOR_T1P5K_READ( xi_index ,  xo_counters -> frame_1024_1518_bytes);
    BL_NET_IF_CORE_MENTOR_TMTU_READ( xi_index ,  xo_counters -> frame_1519_mtu_bytes);	

    /*workaround*/
    xo_counters -> frame_1024_1518_bytes -=  xo_counters -> frame_1519_mtu_bytes ; 

}
EXPORT_SYMBOL ( pi_bl_drv_mac_read_tx_counters );
#if  !defined(_CFE_)
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_loopback                                         */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Enable MAC Loopback                                */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function Enables loopback on specific EMAC                          */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_loopback - MAC loopback : 0 (Disable) | 1 Enable                      */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_loopback ( DRV_MAC_NUMBER xi_index, 
                                        E_DRV_MAC_ENABLE xi_loopback ) 
{
    NET_IF_CORE_MENTOR_MACCFG1_DTE maccfg1 ;

    BL_NET_IF_CORE_MENTOR_MACCFG1_READ( xi_index , maccfg1 ) ;
    maccfg1.loopback = xi_loopback  ;
    BL_NET_IF_CORE_MENTOR_MACCFG1_WRITE( xi_index , maccfg1 ) ;
}
EXPORT_SYMBOL ( pi_bl_drv_mac_set_loopback );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_loopback                                         */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Get MAC Loopback                                   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function returns the loopback mode for specific EMAC                */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_loopback - MAC loopback : 0 (Disable) | 1 Enable                      */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_loopback ( DRV_MAC_NUMBER xi_index , 
                                        E_DRV_MAC_ENABLE * const xo_loopback ) 
{
    NET_IF_CORE_MENTOR_MACCFG1_DTE maccfg1 ;

    BL_NET_IF_CORE_MENTOR_MACCFG1_READ( xi_index , maccfg1 ) ;
    * xo_loopback =  maccfg1.loopback  ;
}
EXPORT_SYMBOL ( pi_bl_drv_mac_get_loopback );
#endif
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_max_frame_length                                 */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Set MAC MTU                                        */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the MTU for specific EMAC                             */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MAX_FRM_LEN ,  MAX_CNT_VAL                                             */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_length - MTU : decimal                                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_max_frame_length ( DRV_MAC_NUMBER xi_index, 
                                                uint32_t xi_length ) 
{
    NET_IF_CORE_MENTOR_MAX_FRM_LEN_DTE maxframe ;
    NET_IF_CORE_GENERAL_MAX_CNT_VAL_DTE max_cnt_val ;

    BL_NET_IF_CORE_MENTOR_MAX_FRM_LEN_READ( xi_index, maxframe );
    BL_NET_IF_CORE_GENERAL_MAX_CNT_VAL_READ( xi_index,max_cnt_val );

    if ( xi_length != 0 )
    {
        maxframe.max_frm_len    = xi_length ;
        max_cnt_val.max_cnt_val = xi_length ;
    }
    else
    {
        maxframe.max_frm_len    = CS_DRV_MAC_MAX_MTU ;
        max_cnt_val.max_cnt_val = CS_DRV_MAC_MAX_MTU ;
    }
	
    BL_NET_IF_CORE_MENTOR_MAX_FRM_LEN_WRITE( xi_index, maxframe );
    BL_NET_IF_CORE_GENERAL_MAX_CNT_VAL_WRITE( xi_index,max_cnt_val );
	
}
EXPORT_SYMBOL ( pi_bl_drv_mac_set_max_frame_length );
#if  !defined(_CFE_)
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_max_frame_length                                 */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Get MAC MTU                                        */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the MTU for specific EMAC                             */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MAX_FRM_LEN ,  MAX_CNT_VAL                                             */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_length - MTU : decimal                                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_max_frame_length ( DRV_MAC_NUMBER xi_index, 
                                                uint32_t * const xo_length ) 
{
    NET_IF_CORE_MENTOR_MAX_FRM_LEN_DTE maxframe ;

    BL_NET_IF_CORE_MENTOR_MAX_FRM_LEN_READ( xi_index, maxframe );
    * xo_length	= maxframe.max_frm_len ;
}
EXPORT_SYMBOL ( pi_bl_drv_mac_get_max_frame_length );
#endif
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_rate                                             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Set MAC Rate                                       */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the rate for specific EMAC                            */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_rate - 0 10Mbps | 1 100bps | 2 1Gbps                                  */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_rate (DRV_MAC_NUMBER xi_index, 
                                   DRV_MAC_RATE   xi_rate )
{
    NET_IF_CORE_MENTOR_MACCFG2_DTE		   maccfg2 ;
    NET_IF_CORE_MENTOR_IFC_CONTROL_DTE	   ifc_control ;
    MIPS_BLOCKS_VPB_BRIDGE_ETH_PARAMS_DTE  eth_params;

    BL_NET_IF_CORE_MENTOR_MACCFG2_READ( xi_index, maccfg2 ); 
    BL_MIPS_BLOCKS_VPB_BRIDGE_ETH_PARAMS_READ(eth_params);

    if ( xi_rate == DRV_MAC_RATE_1000 )
    {
         maccfg2.ifc_mod = CS_BYTE_MODE ;
         switch (xi_index)
         {
         case DRV_MAC_EMAC_0:
             eth_params.core0_freq = 0;
             break;
         case DRV_MAC_EMAC_1:
             eth_params.core1_freq = 0;
             break;
         case DRV_MAC_EMAC_2:
             eth_params.core2_freq = 0;
             break;
         case DRV_MAC_EMAC_3:
             eth_params.core3_freq = 0;
             break;
         case DRV_MAC_EMAC_4:
             eth_params.core4_freq = 0;
             break;
         case DRV_MAC_EMAC_5:
             eth_params.core5_freq = 0;
             break;
         /* we are not supposed to get here */
         default:
             break;
         }
    }
    else 
    {
         maccfg2.ifc_mod = CS_NIBBLE_MODE ;
         if ( xi_rate == DRV_MAC_RATE_100 )
         {
              switch (xi_index)
              {
              case DRV_MAC_EMAC_0:
                  eth_params.core0_freq = 1;
                  break;
              case DRV_MAC_EMAC_1:
                  eth_params.core1_freq = 1;
                  break;
              case DRV_MAC_EMAC_2:
                  eth_params.core2_freq = 1;
                  break;
              case DRV_MAC_EMAC_3:
                  eth_params.core3_freq = 1;
                  break;
              case DRV_MAC_EMAC_4:
                  eth_params.core4_freq = 1;
                  break;
              case DRV_MAC_EMAC_5:
                  eth_params.core5_freq = 1;
                  break;
              /* we are not supposed to get here */
              default:
                  break;
              }
         }
         else
         {
             switch (xi_index)
             {
             case DRV_MAC_EMAC_0:
                 eth_params.core0_freq = 2;
                 break;
             case DRV_MAC_EMAC_1:
                 eth_params.core1_freq = 2;
                 break;
             case DRV_MAC_EMAC_2:
                 eth_params.core2_freq = 2;
                 break;
             case DRV_MAC_EMAC_3:
                 eth_params.core3_freq = 2;
                 break;
             case DRV_MAC_EMAC_4:
                 eth_params.core4_freq = 2;
                 break;
             case DRV_MAC_EMAC_5:
                 eth_params.core5_freq = 2;
                 break;
             /* we are not supposed to get here */
             default:
                 break;
             }
         }
         BL_NET_IF_CORE_MENTOR_IFC_CONTROL_READ( xi_index, ifc_control );
         ifc_control.speed = ( xi_rate == DRV_MAC_RATE_100 ? 1 : 0 ) ;
         BL_NET_IF_CORE_MENTOR_IFC_CONTROL_WRITE( xi_index, ifc_control );
    }

    BL_MIPS_BLOCKS_VPB_BRIDGE_ETH_PARAMS_WRITE(eth_params);
    BL_NET_IF_CORE_MENTOR_MACCFG2_WRITE( xi_index, maccfg2 ); 
	
}
EXPORT_SYMBOL ( pi_bl_drv_mac_set_rate );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_rate                                             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Get MAC Rate                                       */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the rate for specific EMAC                            */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_rate - 0 10Mbps | 1 100bps | 2 1Gbps                                  */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_rate (DRV_MAC_NUMBER xi_index, 
                                   DRV_MAC_RATE * const xo_rate )
{
    NET_IF_CORE_MENTOR_MACCFG2_DTE		   maccfg2 ;
    NET_IF_CORE_MENTOR_IFC_CONTROL_DTE	   ifc_control ;

    BL_NET_IF_CORE_MENTOR_MACCFG2_READ( xi_index, maccfg2 ); 

    if ( maccfg2.ifc_mod == CS_BYTE_MODE ) 
    {
        * xo_rate = DRV_MAC_RATE_1000 ; 
    }
    else 
    {
        BL_NET_IF_CORE_MENTOR_IFC_CONTROL_READ( xi_index, ifc_control );
        if (ifc_control.speed)
        {
            * xo_rate = DRV_MAC_RATE_100;
        }
        else
        {
            * xo_rate = DRV_MAC_RATE_10;
        } 
    }
}
EXPORT_SYMBOL ( pi_bl_drv_mac_get_rate );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_duplex                                           */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Set MAC Duplex                                     */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the duplex mode for specific EMAC                     */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_duplex - 0 Half-Duplex | 1 Full-Duplex                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_duplex (DRV_MAC_NUMBER xi_index, 
                                     DRV_MAC_DUPLEX_MODE xi_duplex)
{
    NET_IF_CORE_MENTOR_MACCFG2_DTE		   maccfg2 ;

    BL_NET_IF_CORE_MENTOR_MACCFG2_READ( xi_index, maccfg2 ); 
    maccfg2.full_dup = (xi_duplex == DRV_MAC_DUPLEX_MODE_FULL ? 1:0);
    BL_NET_IF_CORE_MENTOR_MACCFG2_WRITE( xi_index, maccfg2 ); 

}
EXPORT_SYMBOL ( pi_bl_drv_mac_set_duplex );
#if  !defined(_CFE_)
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_duplex                                           */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Get MAC Duplex                                     */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the duplex mode for specific EMAC                     */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG2                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_duplex - 0 Half-Duplex | 1 Full-Duplex                                */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_duplex (DRV_MAC_NUMBER xi_index, 
                                     DRV_MAC_DUPLEX_MODE * const xo_duplex)
{
    NET_IF_CORE_MENTOR_MACCFG2_DTE		   maccfg2 ;

    BL_NET_IF_CORE_MENTOR_MACCFG2_READ( xi_index, maccfg2 ); 
    * xo_duplex	= maccfg2.full_dup ;
}
EXPORT_SYMBOL ( pi_bl_drv_mac_get_duplex );
#endif
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_flow_control                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Set MAC Flow Control                               */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the flow control mode for specific EMAC               */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_tx_flow_control - 0 Disable| 1 Enable                                 */
/*                                                                            */
/*   xi_rx_flow_control - 0 Disable| 1 Enable                                 */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_flow_control ( DRV_MAC_NUMBER xi_index, 
                                            E_DRV_MAC_ENABLE xi_tx_flow_control, 
                                            E_DRV_MAC_ENABLE xi_rx_flow_control ) 
{
    NET_IF_CORE_MENTOR_MACCFG1_DTE maccfg1 ;

    BL_NET_IF_CORE_MENTOR_MACCFG1_READ( xi_index , maccfg1 ) ;
    maccfg1.tx_fc_en	= xi_tx_flow_control;
	maccfg1.rx_fc_en	= xi_rx_flow_control;
    BL_NET_IF_CORE_MENTOR_MACCFG1_WRITE( xi_index , maccfg1 ) ;
    /* configure the bbh tcrq_en and mac runneren */
    pi_bl_drv_mac_ctrl_pause_packets_generation_bit(xi_index, xi_tx_flow_control);
}
EXPORT_SYMBOL ( pi_bl_drv_mac_set_flow_control );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_modify_flow_control_pause_pkt_addr                         */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Modify the Flow Control pause pkt source address   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function modifies the flow control pause pkt source address         */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_1_TO_4_STATION_ADDR_1                  */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_1_TO_4_STATION_ADDR_2                  */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_1_TO_4_STATION_ADDR_3                  */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_1_TO_4_STATION_ADDR_4                  */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_5_TO_6_STATION_ADDR_5                  */
/*     NET_IF_CORE_MENTOR_STATION_ADDR_5_TO_6_STATION_ADDR_6                  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_mac_addr - Flow Control mac address                                   */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_modify_flow_control_pause_pkt_addr ( DRV_MAC_NUMBER xi_index,
    DRV_MAC_ADDRESS xi_mac_addr)
{
    /* Local Variables */
    NET_IF_CORE_MENTOR_STATION_ADDR_1_TO_4_DTE station_addr_1_to_4_reg ;
    NET_IF_CORE_MENTOR_STATION_ADDR_5_TO_6_DTE station_addr_5_to_6_reg ;

    station_addr_1_to_4_reg.station_addr_1 = xi_mac_addr.b[ 5 ] ;
    station_addr_1_to_4_reg.station_addr_2 = xi_mac_addr.b[ 4 ] ;
    station_addr_1_to_4_reg.station_addr_3 = xi_mac_addr.b[ 3 ] ;
    station_addr_1_to_4_reg.station_addr_4 = xi_mac_addr.b[ 2 ] ;
    station_addr_5_to_6_reg.station_addr_5 = xi_mac_addr.b[ 1 ] ;
    station_addr_5_to_6_reg.station_addr_6 = xi_mac_addr.b[ 0 ] ;

    switch ( xi_index )
    {
    case DRV_MAC_EMAC_0 :
        BL_NET_IF_CORE_0_MENTOR_STATION_ADDR_1_TO_4_WRITE ( station_addr_1_to_4_reg ) ;
        BL_NET_IF_CORE_0_MENTOR_STATION_ADDR_5_TO_6_WRITE ( station_addr_5_to_6_reg ) ;
        break ;
    case DRV_MAC_EMAC_1 :
        BL_NET_IF_CORE_1_MENTOR_STATION_ADDR_1_TO_4_WRITE ( station_addr_1_to_4_reg ) ;
        BL_NET_IF_CORE_1_MENTOR_STATION_ADDR_5_TO_6_WRITE ( station_addr_5_to_6_reg ) ;
        break ;
    case DRV_MAC_EMAC_2 :
        BL_NET_IF_CORE_2_MENTOR_STATION_ADDR_1_TO_4_WRITE ( station_addr_1_to_4_reg ) ;
        BL_NET_IF_CORE_2_MENTOR_STATION_ADDR_5_TO_6_WRITE ( station_addr_5_to_6_reg ) ;
        break ;
    case DRV_MAC_EMAC_3 :
        BL_NET_IF_CORE_3_MENTOR_STATION_ADDR_1_TO_4_WRITE ( station_addr_1_to_4_reg ) ;
        BL_NET_IF_CORE_3_MENTOR_STATION_ADDR_5_TO_6_WRITE ( station_addr_5_to_6_reg ) ;
        break ;
    case DRV_MAC_EMAC_4 :
        BL_NET_IF_CORE_4_MENTOR_STATION_ADDR_1_TO_4_WRITE ( station_addr_1_to_4_reg ) ;
        BL_NET_IF_CORE_4_MENTOR_STATION_ADDR_5_TO_6_WRITE ( station_addr_5_to_6_reg ) ;
        break ;
    case DRV_MAC_EMAC_5 :
        BL_NET_IF_CORE_5_MENTOR_STATION_ADDR_1_TO_4_WRITE ( station_addr_1_to_4_reg ) ;
        BL_NET_IF_CORE_5_MENTOR_STATION_ADDR_5_TO_6_WRITE ( station_addr_5_to_6_reg ) ;
        break ;
    default:
        return;
    }
}
EXPORT_SYMBOL ( pi_bl_drv_mac_modify_flow_control_pause_pkt_addr );

#if  !defined(_CFE_)
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_get_flow_control                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Get MAC Flow Control                               */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the flow control mode for specific EMAC               */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     MACCFG1                                                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xo_tx_flow_control - 0 Disable| 1 Enable                                 */
/*                                                                            */
/*   xo_rx_flow_control - 0 Disable| 1 Enable                                 */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_get_flow_control ( DRV_MAC_NUMBER xi_index, 
                                            E_DRV_MAC_ENABLE * const xo_tx_flow_control, 
                                            E_DRV_MAC_ENABLE * const xo_rx_flow_control ) 
{
    NET_IF_CORE_MENTOR_MACCFG1_DTE maccfg1 ;

    BL_NET_IF_CORE_MENTOR_MACCFG1_READ( xi_index , maccfg1 ) ;

    * xo_tx_flow_control	= maccfg1.tx_fc_en ;
    * xo_rx_flow_control	= maccfg1.rx_fc_en ;

}
EXPORT_SYMBOL ( pi_bl_drv_mac_get_flow_control );
#endif
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_set_pause_packets_transmision_parameters             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Set MAC pause packets transmition parameters       */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the transmision of pause packets :enable and interval */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     XOFF_CTRL                                                              */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_enable_tranmision - 0 Disable| 1 Enable                               */
/*                                                                            */
/*   xi_timer_interval - interval for sending pause packets                   */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_set_pause_packets_transmision_parameters ( DRV_MAC_NUMBER xi_index, 
                                                                    E_DRV_MAC_ENABLE xi_enable_trasnmision ,
                                                                    uint32_t xi_timer_interval ) 
{
    NET_IF_CORE_GENERAL_XOFF_CTRL_DTE xoff_control ;
    NET_IF_CORE_GENERAL_ETH_GR_0_DTE  eth_gr_0 ;

    BL_NET_IF_CORE_GENERAL_XOFF_CTRL_READ( xi_index, xoff_control );
    BL_NET_IF_CORE_GENERAL_ETH_GR_0_READ( xi_index, eth_gr_0 );

    /* Enable Full duplex back pressure after congestion input from BBH*/
    eth_gr_0.bbh_tcrq_en = xi_enable_trasnmision;
    eth_gr_0.bbh_thdf_en = DRV_MAC_DISABLE ;

    if ( xi_enable_trasnmision == DRV_MAC_ENABLE ) 
    {
        xoff_control.priodic_xoff_en = DRV_MAC_ENABLE ;
        xoff_control.timr_strt_val =   xi_timer_interval;
    }

    BL_NET_IF_CORE_GENERAL_XOFF_CTRL_WRITE( xi_index, xoff_control );
    BL_NET_IF_CORE_GENERAL_ETH_GR_0_WRITE( xi_index, eth_gr_0 );

}
EXPORT_SYMBOL ( pi_bl_drv_mac_set_pause_packets_transmision_parameters );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_mac_ctrl_pause_packets_generation_bit                          */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   BL_Lilac_Driver_MAC - Set MAC pause packets generation bit.              */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the generation bit of pause packets                   */
/*                                                                            */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*     NET_IF_CORE_GENERAL_ETH_GR_0_DTE                                       */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_index - EMAC id (0-5)                                                 */
/*                                                                            */
/*   xi_bbh_tcrq_en_bit - 0 Disable| 1 Enable                                 */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_mac_ctrl_pause_packets_generation_bit ( DRV_MAC_NUMBER xi_index,
                                                       E_DRV_MAC_ENABLE xi_bbh_tcrq_en_bit )
{
    NET_IF_CORE_GENERAL_ETH_GR_0_DTE gr0_reg;

    /* Enable full duplex flow control (pause generation) after congestion input from bbh. */
    BL_NET_IF_CORE_GENERAL_ETH_GR_0_READ(xi_index, gr0_reg);
    gr0_reg.bbh_tcrq_en = xi_bbh_tcrq_en_bit;
    BL_NET_IF_CORE_GENERAL_ETH_GR_0_WRITE(xi_index, gr0_reg);
}
EXPORT_SYMBOL ( pi_bl_drv_mac_ctrl_pause_packets_generation_bit );


