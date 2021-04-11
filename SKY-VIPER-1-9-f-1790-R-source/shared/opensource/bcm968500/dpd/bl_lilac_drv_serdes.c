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

/******************************************************************************/
/*                                                                            */
/* File Description:                                                          */
/*                                                                            */
/* This file contains the implementation of the Lilac SERDES driver           */
/*                                                                            */
/******************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/

#include "bl_os_wraper.h"
#include "bl_lilac_drv_serdes.h"
#include "bl_lilac_serdes.h"

#include "bl_lilac_brd.h"
#include "bl_lilac_mips_blocks.h"
#include "bl_lilac_net_if.h"

/******************************************************************************/
/*                                                                            */
/* Types and values definitions                                               */
/*                                                                            */
/******************************************************************************/

#define MS_DELAY( time_in_ms ) mdelay( time_in_ms )


#define CS_GPON_LANE_REG_7_BIST_MODESEL_FOR_PRBS_31         ( 7 )
#define CS_GPON_LANE_REG_7_BIST_MODESEL_FOR_PRBS_23         ( 6 )
#define CS_GPON_LANE_REG_7_BIST_MODESEL_FOR_PRBS_7          ( 4 )
#define CS_GPON_LANE_REG_7_BIST_MODESEL_FOR_PRBS_ALTERNATE  ( 3 )

/* BIST user defined pattern */
#define CS_GPON_CMN_REG_4_BIST_USERDEF_FOR_PRBS_ALTERNATE   ( 0x5555 )


/*---------------------------------------------------------------------------*/

/* Loop counter max tries */
#define CS_SERDES_MAX_TRIES                                          ( 0x00100000 )

/******************************************************************************/
/*                                                                            */
/* Global variables definitions                                               */
/*                                                                            */
/******************************************************************************/

static BL_DRV_SERDES_GENERAL_CONFIG_DTE gs_general_config = 
{
    .wan_mode         = CE_BL_DRV_SERDES_WAN_MODE_GBE ,
    .gbe_wan_emac_id  = CE_BL_DRV_SERDES_EMAC_ID_5 ,
    .emacs_group_mode = CE_BL_DRV_SERDES_EMAC_MODE_SS_SMII ,
    .emac5_mode       = CE_BL_DRV_SERDES_EMAC_MODE_SGMII
};

/******************************************************************************/
/*                                                                            */
/* Functions prototypes                                                       */
/*                                                                            */
/******************************************************************************/

static void p_serdes_netif_configure_and_enable_hisgmii ( BL_DRV_SERDES_EMAC_ID_DTE xi_emac_id ) ;
static void p_serdes_netif_configure_and_enable_qsgmii ( void ) ;
static void p_serdes_netif_configure_and_enable_sgmii ( BL_DRV_SERDES_EMAC_ID_DTE xi_emac_id ) ;
static void pi_configure_lane_reg_8 ( BL_DRV_SERDES_CONFIGURATION * const xi_conf,
                                      SERDES_GPON_LANE_REG_8_DTE * const xo_reg);
static void pi_get_lane_reg_8 ( SERDES_GPON_LANE_REG_8_DTE xi_reg,
                                BL_DRV_SERDES_CONFIGURATION * const xo_conf);

/******************************************************************************/
/*                                                                            */
/* API functions implementations                                              */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_set_general_configuration                               */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   SERDES Driver - Set general configuration                                */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets general configuration of the SERDES block.            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_serdes_general_config - SERDES general configuration.                 */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*     CE_BL_DRV_SERDES_NO_ERROR - No error                                   */
/*     CE_BL_DRV_SERDES_ERROR_INVALID_WAN_MODE_VALUE -                        */
/*           Invalid WAN mode value                                           */
/*     CE_BL_DRV_SERDES_ERROR_INVALID_GBE_WAN_EMAC_ID_VALUE -                 */
/*           Invalid GBE WAN emac ID value                                    */
/*     CE_BL_DRV_SERDES_ERROR_INVALID_EMACS_GROUP_MODE_VALUE -                */
/*           Invalid EMAC group mode value                                    */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_set_general_configuration ( const BL_DRV_SERDES_GENERAL_CONFIG_DTE * xi_serdes_general_config )
{
    if ( xi_serdes_general_config == NULL )
    {
        return ( CE_BL_DRV_SERDES_ERROR_NULL_POINTER ) ;
    }

    if ( ! ME_BL_DRV_SERDES_WAN_MODE_DTE_IN_RANGE( xi_serdes_general_config->wan_mode ) )
    {
        return ( CE_BL_DRV_SERDES_ERROR_INVALID_WAN_MODE_VALUE ) ;
    }

    if ( ( xi_serdes_general_config->gbe_wan_emac_id < CE_BL_DRV_SERDES_EMAC_ID_4 ) ||
         ( xi_serdes_general_config->gbe_wan_emac_id > CE_BL_DRV_SERDES_EMAC_ID_5 ) )
    {
        return ( CE_BL_DRV_SERDES_ERROR_INVALID_GBE_WAN_EMAC_ID_VALUE ) ;
    }

    if ( ! ME_BL_DRV_SERDES_EMAC_MODE_DTE_IN_RANGE( xi_serdes_general_config->emacs_group_mode ) )
    {
        return ( CE_BL_DRV_SERDES_ERROR_INVALID_EMACS_GROUP_MODE_VALUE ) ;
    }

    if ( ( xi_serdes_general_config->emac5_mode != CE_BL_DRV_SERDES_EMAC_MODE_SGMII ) &&
         ( xi_serdes_general_config->emac5_mode != CE_BL_DRV_SERDES_EMAC_MODE_HISGMII ) )
    {
        return ( CE_BL_DRV_SERDES_ERROR_INVALID_EMAC5_MODE_VALUE ) ;
    }

    /* These tests will be removed once HiSGMII is supported */
    if ( ( xi_serdes_general_config->emacs_group_mode == CE_BL_DRV_SERDES_EMAC_MODE_HISGMII ) ||
         ( xi_serdes_general_config->emac5_mode == CE_BL_DRV_SERDES_EMAC_MODE_HISGMII ) )
    {
        return ( CE_BL_DRV_SERDES_ERROR_HISGMII_NOT_SUPPORTED ) ;
    }

    gs_general_config.wan_mode = xi_serdes_general_config->wan_mode ;
    gs_general_config.gbe_wan_emac_id = xi_serdes_general_config->gbe_wan_emac_id ;
    gs_general_config.emacs_group_mode = xi_serdes_general_config->emacs_group_mode ;
    gs_general_config.emac5_mode = xi_serdes_general_config->emac5_mode ;

    return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
}
EXPORT_SYMBOL( fi_bl_drv_serdes_set_general_configuration ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_get_general_configuration                               */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   SERDES Driver - Get general configuration                                */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets general configuration of the SERDE block.             */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   xo_serdes_general_config - IH general configuration.                     */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*     CE_BL_DRV_SERDES_NO_ERROR - No error                                   */
/*     CE_BL_DRV_SERDES_ERROR_NULL_POINTER - NULL pointer                     */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_get_general_configuration ( BL_DRV_SERDES_GENERAL_CONFIG_DTE * const xo_serdes_general_config )
{
    if ( xo_serdes_general_config == NULL )
    {
        return ( CE_BL_DRV_SERDES_ERROR_NULL_POINTER ) ;
    }

    xo_serdes_general_config->wan_mode = gs_general_config.wan_mode ;
    xo_serdes_general_config->gbe_wan_emac_id = gs_general_config.gbe_wan_emac_id ;
    xo_serdes_general_config->emacs_group_mode = gs_general_config.emacs_group_mode ;
    xo_serdes_general_config->emac5_mode = gs_general_config.emac5_mode ;

    return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
}
EXPORT_SYMBOL( fi_bl_drv_serdes_get_general_configuration ) ;

#if  !defined(_CFE_)
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_gpon_rx_path_reset_unreset                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GPON/SERDES Driver - Reset and unreset the RX path                       */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function Resets and unresets the RX path in GPON SERDES and in CDR. */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*     CE_BL_DRV_SERDES_NO_ERROR - No error                                   */
/*     CE_BL_DRV_SERDES_WAN_MODE_IS_NOT_GPON - WAN mode is not GPON           */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_rx_path_reset_unreset ( void )
{
    SERDES_GPON_LANE_REG_7_DTE gpon_lane_reg_7 ;
#ifndef FSSIM
    MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_DTE vpb_bridge_cdr_config ;
#endif

    /* Reset in SERDES */
    BL_SERDES_GPON_LANE_REG_7_READ( gpon_lane_reg_7 ) ;
    gpon_lane_reg_7.rx_rst_n = CE_BL_DRV_SERDES_RESET ;
    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;

#ifndef FSSIM
    /* Reset in CDR */
    BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_READ( vpb_bridge_cdr_config ) ;
    vpb_bridge_cdr_config.gpon_cdr_rxfifo_rst_n = CE_BL_DRV_SERDES_RESET ;
    vpb_bridge_cdr_config.cdr_fifo_rst_n = CE_BL_DRV_SERDES_RESET ;
    BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_WRITE( vpb_bridge_cdr_config ) ;

    /* Unreset in CDR */
    BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_READ( vpb_bridge_cdr_config ) ;
    vpb_bridge_cdr_config.gpon_cdr_rxfifo_rst_n = CE_BL_DRV_SERDES_UNRESET ;
    BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_WRITE( vpb_bridge_cdr_config ) ;
#endif

    /* Unreset in SERDES */
    BL_SERDES_GPON_LANE_REG_7_READ( gpon_lane_reg_7 ) ;
    gpon_lane_reg_7.rx_rst_n = CE_BL_DRV_SERDES_UNRESET ;
    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;


    return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
}
EXPORT_SYMBOL ( fi_bl_drv_serdes_gpon_rx_path_reset_unreset );


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_gpon_tx_path_reset_unreset                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GPON/SERDES Driver - Reset and unreset the TX path                       */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function Resets and unresets the TX path in GPON SERDES and in CDR. */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*     CE_BL_DRV_SERDES_NO_ERROR - No error                                   */
/*     CE_BL_DRV_SERDES_WAN_MODE_IS_NOT_GPON - WAN mode is not GPON           */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_tx_path_reset_unreset ( void )
{
    SERDES_GPON_LANE_REG_7_DTE gpon_lane_reg_7 ;
#ifndef FSSIM
    MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_DTE vpb_bridge_cdr_config ;
#endif

    /* Reset in SERDES */
    BL_SERDES_GPON_LANE_REG_7_READ( gpon_lane_reg_7 ) ;
    gpon_lane_reg_7.tx_rst_n = CE_BL_DRV_SERDES_RESET ;
    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;

#ifndef FSSIM
    /* Reset in CDR */
    BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_READ( vpb_bridge_cdr_config ) ;
    vpb_bridge_cdr_config.cdr_fifo_rst_n        = CE_BL_DRV_SERDES_RESET ;
    vpb_bridge_cdr_config.gpon_cdr_txfifo_rst_n = CE_BL_DRV_SERDES_RESET ;
    BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_WRITE( vpb_bridge_cdr_config ) ;

    /* Unreset in CDR */
    BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_READ( vpb_bridge_cdr_config ) ;
    vpb_bridge_cdr_config.cdr_fifo_rst_n        = CE_BL_DRV_SERDES_UNRESET ;
    vpb_bridge_cdr_config.gpon_cdr_txfifo_rst_n = CE_BL_DRV_SERDES_UNRESET ;
    BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_WRITE( vpb_bridge_cdr_config ) ;
#endif

    /* Unreset in SERDES */
    BL_SERDES_GPON_LANE_REG_7_READ( gpon_lane_reg_7 ) ;
    gpon_lane_reg_7.tx_rst_n = CE_BL_DRV_SERDES_UNRESET ;
    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;


    return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
}
EXPORT_SYMBOL ( fi_bl_drv_serdes_gpon_tx_path_reset_unreset );


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_gpon_configure_prbs_mode                                */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GPON/SERDES Driver - configure PRBS mode                                 */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function configures the PRBS mode (for both US generator and DS     */
/*   comparator).                                                             */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_prbs_mode - PRBS mode.                                                */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_configure_prbs_mode ( BL_DRV_SERDES_PRBS_MODE_DTE xi_prbs_mode )
{
    volatile SERDES_GPON_LANE_REG_7_DTE gpon_lane_reg_7 ;
    volatile SERDES_GPON_CMN_REG_4_DTE gpon_cmn_reg_4 ;


    /* if mode is alternate, need to configure the pattern in Reg 4 */
    if ( xi_prbs_mode == CE_BL_DRV_SERDES_PRBS_MODE_ALTERNATE )
    {
        /* set the BIST user defined pattern to 0x5555 */
        BL_SERDES_GPON_CMN_REG_4_READ( gpon_cmn_reg_4 ) ;
        gpon_cmn_reg_4.bist_userdef = CS_GPON_CMN_REG_4_BIST_USERDEF_FOR_PRBS_ALTERNATE ;
        BL_SERDES_GPON_CMN_REG_4_WRITE( gpon_cmn_reg_4 ) ;
    }


    /* configure the mode in Reg 7 (and reset the error checker) */

    BL_SERDES_GPON_LANE_REG_7_READ( gpon_lane_reg_7 ) ;

    /* reset the error checker */
    gpon_lane_reg_7.bist_errrst = 1 ;

    switch ( xi_prbs_mode )
    {
    case CE_BL_DRV_SERDES_PRBS_MODE_PRBS7:
        gpon_lane_reg_7.bist_modesel = CS_GPON_LANE_REG_7_BIST_MODESEL_FOR_PRBS_7 ;
        break ;

    case CE_BL_DRV_SERDES_PRBS_MODE_PRBS23:
        gpon_lane_reg_7.bist_modesel = CS_GPON_LANE_REG_7_BIST_MODESEL_FOR_PRBS_23 ;
        break ;

    case CE_BL_DRV_SERDES_PRBS_MODE_PRBS31:
        gpon_lane_reg_7.bist_modesel = CS_GPON_LANE_REG_7_BIST_MODESEL_FOR_PRBS_31 ;
        break ;

    case CE_BL_DRV_SERDES_PRBS_MODE_ALTERNATE:
        gpon_lane_reg_7.bist_modesel = CS_GPON_LANE_REG_7_BIST_MODESEL_FOR_PRBS_ALTERNATE ;
        break ;

    default:
        return ( CE_BL_DRV_SERDES_ERROR_INVALID_PRBS_MODE ) ;
    }

    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;

    /* unreset the error checker */
    gpon_lane_reg_7.bist_errrst = 0 ;
    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;


    return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
}
EXPORT_SYMBOL ( fi_bl_drv_serdes_gpon_configure_prbs_mode );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_gpon_control_us_prbs_generator                          */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GPON/SERDES Driver - control US PRBS generator                           */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function enables or disables the upstream PRBS generator.           */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_action - action (enable/disable).                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_control_us_prbs_generator ( BL_DRV_SERDES_ENABLE_DTE xi_action )
{
    volatile SERDES_GPON_LANE_REG_7_DTE gpon_lane_reg_7 ;

    BL_SERDES_GPON_LANE_REG_7_READ( gpon_lane_reg_7 ) ;
    gpon_lane_reg_7.bist_genen = xi_action ;
    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;


    return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
}
EXPORT_SYMBOL ( fi_bl_drv_serdes_gpon_control_us_prbs_generator );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_gpon_control_ds_prbs_comparator                         */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GPON/SERDES Driver - control DS PRBS comparator                          */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function enables or disables the downstream PRBS comparator.        */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_action - action (enable/disable).                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_control_ds_prbs_comparator ( BL_DRV_SERDES_ENABLE_DTE xi_action )
{
    volatile SERDES_GPON_LANE_REG_7_DTE gpon_lane_reg_7 ;

    BL_SERDES_GPON_LANE_REG_7_READ( gpon_lane_reg_7 ) ;
    gpon_lane_reg_7.bist_chken = xi_action ;
    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;


    return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
}
EXPORT_SYMBOL ( fi_bl_drv_serdes_gpon_control_ds_prbs_comparator );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_gpon_get_ds_prbs_comparator_status                      */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GPON/SERDES Driver - DS get PRBS comparator status                       */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the DS PRBS comparator status.                        */
/*   The status is cleared after reading it.                                  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   xo_prbs_status - PRBS status.                                            */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_get_ds_prbs_comparator_status ( BL_DRV_SERDES_PRBS_STATUS_DTE * const xo_prbs_status )
{
    volatile SERDES_GPON_LANE_REG_7_DTE gpon_lane_reg_7 ;

    BL_SERDES_GPON_LANE_REG_7_READ( gpon_lane_reg_7 ) ;

    if ( ( gpon_lane_reg_7.bist_sync == 1 ) && /* BIST sync */
         ( gpon_lane_reg_7.bist_errstat == 0 ) && /* BIST eror status */
         ( gpon_lane_reg_7.bist_errcnt == 0 ) ) /* BIST error counter */
    {
        * xo_prbs_status = CE_BL_DRV_SERDES_PRBS_STATUS_OK ;
    }
    else
    {
        * xo_prbs_status = CE_BL_DRV_SERDES_PRBS_STATUS_ERROR ;
    }

    /* reset the error checker */
    gpon_lane_reg_7.bist_errrst = 1 ;
    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;

    /* unreset the error checker */
    gpon_lane_reg_7.bist_errrst = 0 ;
    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;


    return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
}
EXPORT_SYMBOL ( fi_bl_drv_serdes_gpon_get_ds_prbs_comparator_status );
#endif 

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_gpon_init                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GPON/SERDES Driver - Init GPON SERDES                         			  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function inits the GPON SERDES block (active low reset)   */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*     CE_BL_DRV_SERDES_NO_ERROR - No error                                   */
/*     CE_BL_DRV_SERDES_INVALID_RESET_VALUE - Invalid reset value             */
/*     CE_BL_DRV_SERDES_WAN_MODE_IS_NOT_GPON - WAN mode is not GPON           */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_init(int ref_xtal)
{
    SERDES_GPON_CMN_REG_0_DTE     gpon_cmn_reg_0;
    SERDES_GPON_CMN_REG_1_DTE     gpon_cmn_reg_1;
    SERDES_GPON_CMN_REG_3_DTE     gpon_cmn_reg_3;
    SERDES_GPON_CMN_REG_4_DTE     gpon_cmn_reg_4;
	SERDES_GPON_CMN_REG_6_DTE     gpon_cmn_reg_6;
    SERDES_GPON_LANE_REG_7_DTE    gpon_lane_reg_7;
    SERDES_GPON_LANE_REG_8_DTE    gpon_lane_reg_8;
    SERDES_GPON_LANE_REG_10_DTE    gpon_lane_reg_10;
    SERDES_GPON_JTF_XO_REG_11_DTE gpon_jtf_xo_reg_11;
	unsigned long ver;

#ifndef FSSIM
    MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_DTE vpb_bridge_cdr_config;
    uint32_t tries_num = CS_SERDES_MAX_TRIES;
#endif

    if(gs_general_config.wan_mode != CE_BL_DRV_SERDES_WAN_MODE_GPON)
    {
        return CE_BL_DRV_SERDES_ERROR_WAN_MODE_IS_NOT_GPON;
    }

	/* GPON jtf xo reg_11 */
	BL_SERDES_GPON_JTF_XO_REG_11_READ(gpon_jtf_xo_reg_11);
	gpon_jtf_xo_reg_11.biasj_en = CE_BL_DRV_SERDES_DISABLE;
	gpon_jtf_xo_reg_11.cdrjtf_intthresh = CE_SERDES_GPON_JTF_XO_REG_11_CDRJTF_INTTHRESH_DEFAULT_VALUE;
	gpon_jtf_xo_reg_11.cdrjtf_sdthresh = CE_SERDES_GPON_JTF_XO_REG_11_CDRJTF_SDTHRESH_DEFAULT_VALUE;
	gpon_jtf_xo_reg_11.cdrjtf_gain = CE_SERDES_GPON_JTF_XO_REG_11_CDRJTF_GAIN_DEFAULT_VALUE;
	gpon_jtf_xo_reg_11.cdrjtf_bypass = CE_SERDES_GPON_JTF_XO_REG_11_CDRJTF_BYPASS_DEFAULT_VALUE;
	if(ref_xtal)
		gpon_jtf_xo_reg_11.refmuxd_sel = CE_SERDES_GPON_JTF_XO_REG_11_REFMUXD_SEL_DEFAULT_VALUE;
	else
		gpon_jtf_xo_reg_11.refmuxd_sel = 0;
	gpon_jtf_xo_reg_11.divj_divsel = 0x80;
	gpon_jtf_xo_reg_11.pij_phasesel = CE_SERDES_GPON_JTF_XO_REG_11_PIJ_PHASESEL_DEFAULT_VALUE;
	gpon_jtf_xo_reg_11.tx_tenbit = CE_SERDES_GPON_JTF_XO_REG_11_TX_TENBIT_DEFAULT_VALUE;
	gpon_jtf_xo_reg_11.rx_tenbit = CE_SERDES_GPON_JTF_XO_REG_11_RX_TENBIT_DEFAULT_VALUE;
	gpon_jtf_xo_reg_11.divj_rst_n = CE_SERDES_GPON_JTF_XO_REG_11_DIVJ_RST_N_DEFAULT_VALUE;
	gpon_jtf_xo_reg_11.divj_muxsel = CE_SERDES_GPON_JTF_XO_REG_11_DIVJ_MUXSEL_DEFAULT_VALUE;
	BL_SERDES_GPON_JTF_XO_REG_11_WRITE(gpon_jtf_xo_reg_11);

	MS_DELAY(2);

	/* GPON common reg_1 */
	gpon_cmn_reg_1.pll_dsmbyp = CE_SERDES_GPON_CMN_REG_1_PLL_DSMBYP_DEFAULT_VALUE;
	gpon_cmn_reg_1.pll_fbdivint = CE_SERDES_GPON_CMN_REG_1_PLL_FBDIVINT_DEFAULT_VALUE;
	gpon_cmn_reg_1.pll_fbdivfrac = CE_SERDES_GPON_CMN_REG_1_PLL_FBDIVFRAC_DEFAULT_VALUE;
	BL_SERDES_GPON_CMN_REG_1_WRITE(gpon_cmn_reg_1);

	/* GPON common reg_3 */
    gpon_cmn_reg_3.pll_refclk0trimadj = CE_SERDES_GPON_CMN_REG_3_PLL_REFCLK0TRIMADJ_DEFAULT_VALUE;
    gpon_cmn_reg_3.pll_hibw           = CE_SERDES_GPON_CMN_REG_3_PLL_HIBW_DEFAULT_VALUE;
    gpon_cmn_reg_3.pll_refdivsel      = CE_SERDES_GPON_CMN_REG_3_PLL_REFDIVSEL_DEFAULT_VALUE;
    gpon_cmn_reg_3.pll_refclksel      = CE_SERDES_GPON_CMN_REG_3_PLL_REFCLKSEL_DEFAULT_VALUE;
    gpon_cmn_reg_3.pll_refclk0termen  = CE_BL_DRV_SERDES_DISABLE;
    gpon_cmn_reg_3.pll_refclk0drven   = CE_BL_DRV_SERDES_DISABLE;
    gpon_cmn_reg_3.pll_div1rst_n      = CE_BL_DRV_SERDES_RESET;
    gpon_cmn_reg_3.pll_div1clksel     = 0x80;
    gpon_cmn_reg_3.pll_div0rst_n      = CE_BL_DRV_SERDES_RESET;
    gpon_cmn_reg_3.pll_div0clksel     = 0x80;
	BL_SERDES_GPON_CMN_REG_3_WRITE(gpon_cmn_reg_3);

	/* GPON common reg_4 */
	BL_SERDES_GPON_CMN_REG_4_READ(gpon_cmn_reg_4);
	gpon_cmn_reg_4.pll_cpgain = 0x3;
	gpon_cmn_reg_4.pll_rst_n  = CE_BL_DRV_SERDES_RESET;
	BL_SERDES_GPON_CMN_REG_4_WRITE(gpon_cmn_reg_4 );
	gpon_cmn_reg_4.pll_rst_n = CE_BL_DRV_SERDES_UNRESET;
	BL_SERDES_GPON_CMN_REG_4_WRITE(gpon_cmn_reg_4);

    /* Reset GPON common reg_0 */
    BL_SERDES_GPON_CMN_REG_0_READ(gpon_cmn_reg_0);
    gpon_cmn_reg_0.pll_en        = CE_BL_DRV_SERDES_ENABLE;
    gpon_cmn_reg_0.clkdis_en     = CE_BL_DRV_SERDES_ENABLE;
    gpon_cmn_reg_0.bias_en       = CE_BL_DRV_SERDES_ENABLE;
    gpon_cmn_reg_0.txcal_start   = CE_BL_DRV_SERDES_DISABLE;
    gpon_cmn_reg_0.rxcal_start   = CE_BL_DRV_SERDES_DISABLE;
    gpon_cmn_reg_0.pll_vcalstart = CE_BL_DRV_SERDES_DISABLE;
    BL_SERDES_GPON_CMN_REG_0_WRITE(gpon_cmn_reg_0);

    MS_DELAY(2);

    gpon_cmn_reg_0.pll_vcalstart = CE_BL_DRV_SERDES_ENABLE;
    BL_SERDES_GPON_CMN_REG_0_WRITE(gpon_cmn_reg_0);

    MS_DELAY(1);

    gpon_cmn_reg_0.pll_vcalstart = CE_BL_DRV_SERDES_DISABLE;
    BL_SERDES_GPON_CMN_REG_0_WRITE(gpon_cmn_reg_0);

#ifndef FSSIM
	/* Wait for PLL locked */
	do
	{
		tries_num--;
		BL_SERDES_GPON_CMN_REG_0_READ(gpon_cmn_reg_0);
	}
	while((gpon_cmn_reg_0.pll_locked != CE_BL_DRV_SERDES_PLL_LOCKED) && (tries_num > 0));

	if(tries_num == 0)
	{
		return CE_BL_DRV_SERDES_ERROR_GPON_PLL_LOCKING;
	}
#endif

	/* Reset GPON common reg_0, wait to rxcal_done 0 and txcal_done 0 */
	gpon_cmn_reg_0.rxcal_start = CE_BL_DRV_SERDES_ENABLE;
	gpon_cmn_reg_0.txcal_start = CE_BL_DRV_SERDES_ENABLE;
	BL_SERDES_GPON_CMN_REG_0_WRITE(gpon_cmn_reg_0);

	MS_DELAY(1);

#ifndef FSSIM
	tries_num = CS_SERDES_MAX_TRIES;
    do
    {
        tries_num--;
        BL_SERDES_GPON_CMN_REG_0_READ(gpon_cmn_reg_0);
    }
    while(((gpon_cmn_reg_0.rxcal_done == 0) || (gpon_cmn_reg_0.txcal_done == 0)) && (tries_num > 0));

    if(tries_num == 0)
    {
        return CE_BL_DRV_SERDES_ERROR_GPON_RX_TX_CAL_DONE_LOCKING;
    }
#endif

	/* GPON common reg_3: unreset PLL dividers */
 	BL_SERDES_GPON_CMN_REG_3_READ(gpon_cmn_reg_3);
	gpon_cmn_reg_3.pll_div0rst_n 	 = CE_BL_DRV_SERDES_UNRESET;
	gpon_cmn_reg_3.pll_div1rst_n 	 = CE_BL_DRV_SERDES_UNRESET;
	BL_SERDES_GPON_CMN_REG_3_WRITE(gpon_cmn_reg_3);
	
	/* Configure  GPON common reg_6 */
	gpon_cmn_reg_6.cdr_threshint = 0;
	gpon_cmn_reg_6.cdr_thresh2   = CE_SERDES_GPON_CMN_REG_6_CDR_THRESH2_DEFAULT_VALUE;
	gpon_cmn_reg_6.cdr_thresh1   = CE_SERDES_GPON_CMN_REG_6_CDR_THRESH1_DEFAULT_VALUE;
	gpon_cmn_reg_6.cdr_gain2	 = CE_SERDES_GPON_CMN_REG_6_CDR_GAIN2_DEFAULT_VALUE;
	gpon_cmn_reg_6.cdr_gain1	 = 0;
	BL_SERDES_GPON_CMN_REG_6_WRITE(gpon_cmn_reg_6);
	
	/* GPON lane reg_7: RX enable */
	BL_SERDES_GPON_LANE_REG_7_READ(gpon_lane_reg_7);
	gpon_lane_reg_7.rx_en       = CE_BL_DRV_SERDES_ENABLE;
	gpon_lane_reg_7.rx_msbfirst = 1;
	BL_SERDES_GPON_LANE_REG_7_WRITE(gpon_lane_reg_7);

    /* Configure GPON LANE Reg_10 cdr hold */
    BL_SERDES_GPON_LANE_REG_10_READ(gpon_lane_reg_10);  
    gpon_lane_reg_10.cdr_hold = CE_BL_DRV_SERDES_ENABLE;
    BL_SERDES_GPON_LANE_REG_10_WRITE(gpon_lane_reg_10);  

    /* Configure GPON LANE Reg_10 pi reset*/
    gpon_lane_reg_10.pi_reset= 1;
    BL_SERDES_GPON_LANE_REG_10_WRITE(gpon_lane_reg_10);  

    /* Configure GPON LANE Reg_10 pi unreset*/
    gpon_lane_reg_10.pi_reset= 0 ;
    BL_SERDES_GPON_LANE_REG_10_WRITE(gpon_lane_reg_10);  

	/* Configure GPON LANE Reg_7, reset TX and RX */
	gpon_lane_reg_7.rx_rst_n 	 = CE_BL_DRV_SERDES_RESET ;
	gpon_lane_reg_7.tx_rst_n 	 = CE_BL_DRV_SERDES_RESET ;
	BL_SERDES_GPON_LANE_REG_7_WRITE(gpon_lane_reg_7);  

	/* Configure GPON LANE Reg_7, unreset TX and RX */
	gpon_lane_reg_7.rx_rst_n 	 = CE_BL_DRV_SERDES_UNRESET ;
	gpon_lane_reg_7.tx_rst_n 	 = CE_BL_DRV_SERDES_UNRESET ;
	BL_SERDES_GPON_LANE_REG_7_WRITE(gpon_lane_reg_7);  

	/* GPON jtf xo reg_11 */
	gpon_jtf_xo_reg_11.biasj_en = CE_BL_DRV_SERDES_ENABLE;
	gpon_jtf_xo_reg_11.iqgenj_en = CE_BL_DRV_SERDES_ENABLE;
	gpon_jtf_xo_reg_11.pij_rst = 0;
	gpon_jtf_xo_reg_11.pij_en = CE_BL_DRV_SERDES_ENABLE;
	gpon_jtf_xo_reg_11.pij_hold = CE_BL_DRV_SERDES_ENABLE;
	gpon_jtf_xo_reg_11.refmuxd_drven = CE_BL_DRV_SERDES_ENABLE;
	gpon_jtf_xo_reg_11.divj_rst_n = CE_BL_DRV_SERDES_UNRESET;
	BL_SERDES_GPON_JTF_XO_REG_11_WRITE(gpon_jtf_xo_reg_11);
	
	gpon_jtf_xo_reg_11.pij_rst = 1;
	BL_SERDES_GPON_JTF_XO_REG_11_WRITE(gpon_jtf_xo_reg_11);
	
	gpon_jtf_xo_reg_11.pij_rst = 0;
	BL_SERDES_GPON_JTF_XO_REG_11_WRITE(gpon_jtf_xo_reg_11);
	
#ifndef FSSIM
	BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_READ(vpb_bridge_cdr_config);
	vpb_bridge_cdr_config.cdr_fifo_rst_n        = CE_BL_DRV_SERDES_RESET;
	vpb_bridge_cdr_config.gpon_cdr_txfifo_rst_n = CE_BL_DRV_SERDES_RESET;
	vpb_bridge_cdr_config.gpon_cdr_rxfifo_rst_n = CE_BL_DRV_SERDES_RESET;
	vpb_bridge_cdr_config.cdr_fifo_status_clr   = CE_BL_DRV_SERDES_ENABLE;
	BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_WRITE(vpb_bridge_cdr_config);
	
	vpb_bridge_cdr_config.cdr_fifo_rst_n        = CE_BL_DRV_SERDES_UNRESET;
	vpb_bridge_cdr_config.gpon_cdr_txfifo_rst_n = CE_BL_DRV_SERDES_UNRESET;
	vpb_bridge_cdr_config.gpon_cdr_rxfifo_rst_n = CE_BL_DRV_SERDES_UNRESET;
	vpb_bridge_cdr_config.cdr_fifo_status_clr   = CE_BL_DRV_SERDES_DISABLE;
	BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_WRITE(vpb_bridge_cdr_config);
#endif

	gpon_jtf_xo_reg_11.pij_hold = CE_BL_DRV_SERDES_DISABLE;
	BL_SERDES_GPON_JTF_XO_REG_11_WRITE(gpon_jtf_xo_reg_11);
	
    gpon_lane_reg_10.cdr_hold = CE_BL_DRV_SERDES_DISABLE;
    BL_SERDES_GPON_LANE_REG_10_WRITE(gpon_lane_reg_10);  

	BL_SERDES_GPON_LANE_REG_8_READ(gpon_lane_reg_8);  
	gpon_lane_reg_8.serl_t_dlysel = CE_SERDES_GPON_LANE_REG_8_SERL_T_DLYSEL_DEFAULT_VALUE;
	gpon_lane_reg_8.drv_t_vmarg = CE_SERDES_GPON_LANE_REG_8_DRV_T_VMARG_DEFAULT_VALUE;
	BL_MIPS_BLOCKS_VPB_BRIDGE_VERSION_READ(ver);
	if(ver & 0x1)
		gpon_lane_reg_8.drv_t_slewsel  = 0x3;
	else
		gpon_lane_reg_8.drv_t_slewsel = CE_SERDES_GPON_LANE_REG_8_DRV_T_SLEWSEL_DEFAULT_VALUE;
	gpon_lane_reg_8.drv_t_idle = CE_SERDES_GPON_LANE_REG_8_DRV_T_IDLE_DEFAULT_VALUE;
	gpon_lane_reg_8.drv_t_hiz = CE_SERDES_GPON_LANE_REG_8_DRV_T_HIZ_DEFAULT_VALUE;
	gpon_lane_reg_8.drv_t_drvdaten = CE_BL_DRV_SERDES_ENABLE;
	gpon_lane_reg_8.drv_t_deemp = CE_SERDES_GPON_LANE_REG_8_DRV_T_DEEMP_DEFAULT_VALUE;
	BL_SERDES_GPON_LANE_REG_8_WRITE(gpon_lane_reg_8);  

	gpon_lane_reg_7.tx_en 	 = CE_BL_DRV_SERDES_ENABLE;
	BL_SERDES_GPON_LANE_REG_7_WRITE(gpon_lane_reg_7);  

#ifndef FSSIM
	vpb_bridge_cdr_config.cdr_fifo_rst_n        = CE_BL_DRV_SERDES_RESET;
	vpb_bridge_cdr_config.gpon_cdr_txfifo_rst_n = CE_BL_DRV_SERDES_RESET;
	vpb_bridge_cdr_config.gpon_cdr_rxfifo_rst_n = CE_BL_DRV_SERDES_UNRESET;
	vpb_bridge_cdr_config.cdr_fifo_status_clr   = CE_BL_DRV_SERDES_ENABLE;
	BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_WRITE(vpb_bridge_cdr_config);
	
	vpb_bridge_cdr_config.cdr_fifo_rst_n        = CE_BL_DRV_SERDES_UNRESET;
	vpb_bridge_cdr_config.gpon_cdr_txfifo_rst_n = CE_BL_DRV_SERDES_UNRESET;
	vpb_bridge_cdr_config.gpon_cdr_rxfifo_rst_n = CE_BL_DRV_SERDES_UNRESET;
	vpb_bridge_cdr_config.cdr_fifo_status_clr   = CE_BL_DRV_SERDES_DISABLE;
	BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_WRITE(vpb_bridge_cdr_config);
#endif

    return CE_BL_DRV_SERDES_NO_ERROR;
}
EXPORT_SYMBOL (fi_bl_drv_serdes_gpon_init);

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   pi_bl_drv_serdes_netif_configure_and_enable                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GPON/SERDES Driver - Configure and enable NETIF SERDES                   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function configures and enables the NETIF SERDES block              */
/*   according to the chosen EMACS group mode.                                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/******************************************************************************/
void pi_bl_drv_serdes_netif_configure_and_enable ( BL_DRV_SERDES_EMAC_ID_DTE xi_emac_id )
{
    /* QSGMII, configure only EMAC 0 */
    if ( gs_general_config.emacs_group_mode == CE_BL_DRV_SERDES_EMAC_MODE_QSGMII )
    {
        p_serdes_netif_configure_and_enable_qsgmii ( ) ;
    }
    /* HiSGMII, configure EMACs 0 and 1 */
    else if ( gs_general_config.emacs_group_mode == CE_BL_DRV_SERDES_EMAC_MODE_HISGMII )
    {
        p_serdes_netif_configure_and_enable_hisgmii ( xi_emac_id ) ;
        p_serdes_netif_configure_and_enable_hisgmii ( xi_emac_id ) ;
    }
    else
    {
        /* SS_SMII, configure only EMAC 0 */
        if ( gs_general_config.emacs_group_mode == CE_BL_DRV_SERDES_EMAC_MODE_SS_SMII )
        {
            p_serdes_netif_configure_and_enable_sgmii ( CE_BL_DRV_SERDES_EMAC_ID_0 ) ;
        }
        /* SGMII, configure EMAC 0 to EMAC 3 */
        else
        {
            p_serdes_netif_configure_and_enable_sgmii ( xi_emac_id ) ;
        }
    }
}
EXPORT_SYMBOL ( pi_bl_drv_serdes_netif_configure_and_enable );

#if defined(_CFE_)
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_netif_init                                             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   NETIF/SERDES Driver - Reset NETIF SERDES                         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function inits the NETIF SERDES block (active low reset)  		  */
/*   according to the chosen EMACS group mode.                                */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*     CE_BL_DRV_SERDES_NO_ERROR - No error                                   */
/*     CE_BL_DRV_SERDES_INVALID_RESET_VALUE - Invalid reset value             */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_netif_init(void)
{
    SERDES_NETIF_CMN_REG_0_DTE    netif_cmn_reg_0 ;
    SERDES_NETIF_CMN_REG_1_DTE    netif_cmn_reg_1 ;
    SERDES_NETIF_CMN_REG_3_DTE    netif_cmn_reg_3 ;
    SERDES_NETIF_CMN_REG_4_DTE    netif_cmn_reg_4 ;
    SERDES_NETIF_CMN_REG_6_DTE    netif_cmn_reg_6 ;
    SERDES_GPON_JTF_XO_REG_11_DTE gpon_jtf_xo_reg_11 ;

#ifndef FSSIM
    uint32_t tries_num = CS_SERDES_MAX_TRIES;
#endif

     /* Reset GPON jtf xo reg_11 */
    BL_SERDES_GPON_JTF_XO_REG_11_READ(gpon_jtf_xo_reg_11);
    gpon_jtf_xo_reg_11.refmuxd_drven = CE_BL_DRV_SERDES_ENABLE;
    gpon_jtf_xo_reg_11.osc_en = CE_BL_DRV_SERDES_ENABLE;
    BL_SERDES_GPON_JTF_XO_REG_11_WRITE(gpon_jtf_xo_reg_11);

    MS_DELAY(2);

	/* Configure  NETIF common reg_6 */
	netif_cmn_reg_6.cdr_threshint = 0;
	netif_cmn_reg_6.cdr_thresh2   = CE_SERDES_NETIF_CMN_REG_6_CDR_THRESH2_DEFAULT_VALUE;
	netif_cmn_reg_6.cdr_thresh1   = 0;
	netif_cmn_reg_6.cdr_gain2	  = 7;
	netif_cmn_reg_6.cdr_gain1	  = 0;
	BL_SERDES_NETIF_CMN_REG_6_WRITE( netif_cmn_reg_6 ) ;

     /* Reset NETIF common reg_1 */
    netif_cmn_reg_1.pll_dsmbyp    = CE_SERDES_NETIF_CMN_REG_1_PLL_DSMBYP_DEFAULT_VALUE;
    netif_cmn_reg_1.pll_fbdivint  = CE_SERDES_NETIF_CMN_REG_1_PLL_FBDIVINT_DEFAULT_VALUE;
    netif_cmn_reg_1.pll_fbdivfrac = CE_SERDES_NETIF_CMN_REG_1_PLL_FBDIVFRAC_DEFAULT_VALUE;
    BL_SERDES_NETIF_CMN_REG_1_WRITE(netif_cmn_reg_1);

    /* Reset NETIF common reg_3 */
    netif_cmn_reg_3.pll_refclk0trimadj = CE_SERDES_NETIF_CMN_REG_3_PLL_REFCLK0TRIMADJ_DEFAULT_VALUE;
    netif_cmn_reg_3.pll_hibw           = CE_SERDES_NETIF_CMN_REG_3_PLL_HIBW_DEFAULT_VALUE;
    netif_cmn_reg_3.pll_refdivsel      = CE_SERDES_NETIF_CMN_REG_3_PLL_REFDIVSEL_DEFAULT_VALUE;
    netif_cmn_reg_3.pll_refclksel      = CE_SERDES_NETIF_CMN_REG_3_PLL_REFCLKSEL_DEFAULT_VALUE;
    netif_cmn_reg_3.pll_refclk0termen  = CE_BL_DRV_SERDES_ENABLE;
    netif_cmn_reg_3.pll_refclk0drven   = CE_BL_DRV_SERDES_ENABLE;
    netif_cmn_reg_3.pll_div1rst_n      = CE_BL_DRV_SERDES_RESET;
    netif_cmn_reg_3.pll_div1clksel     = 0x5;
    netif_cmn_reg_3.pll_div0rst_n      = CE_BL_DRV_SERDES_RESET;
    netif_cmn_reg_3.pll_div0clksel     = CE_SERDES_NETIF_CMN_REG_3_PLL_DIV0CLKSEL_DEFAULT_VALUE;
    BL_SERDES_NETIF_CMN_REG_3_WRITE(netif_cmn_reg_3);
	
    /* Reset NETIF common reg_4 */
    BL_SERDES_NETIF_CMN_REG_4_READ( netif_cmn_reg_4 ) ;
    netif_cmn_reg_4.pll_rst_n       = CE_BL_DRV_SERDES_RESET ;
    BL_SERDES_NETIF_CMN_REG_4_WRITE( netif_cmn_reg_4 ) ;
    netif_cmn_reg_4.pll_rst_n       = CE_BL_DRV_SERDES_UNRESET ;
    BL_SERDES_NETIF_CMN_REG_4_WRITE( netif_cmn_reg_4 ) ;

    /* Reset NETIF common reg_0 */
    BL_SERDES_NETIF_CMN_REG_0_READ( netif_cmn_reg_0 ) ;
    netif_cmn_reg_0.pll_en        = CE_BL_DRV_SERDES_ENABLE ;
    netif_cmn_reg_0.clkdis_en     = CE_BL_DRV_SERDES_ENABLE ;
    netif_cmn_reg_0.bias_en       = CE_BL_DRV_SERDES_ENABLE ;
    netif_cmn_reg_0.txcal_start   = CE_BL_DRV_SERDES_DISABLE ;
    netif_cmn_reg_0.rxcal_start   = CE_BL_DRV_SERDES_DISABLE ;
    netif_cmn_reg_0.pll_vcalstart = CE_BL_DRV_SERDES_DISABLE ;
    BL_SERDES_NETIF_CMN_REG_0_WRITE( netif_cmn_reg_0 ) ;

    MS_DELAY(2);

    netif_cmn_reg_0.pll_vcalstart = CE_BL_DRV_SERDES_ENABLE ;
    BL_SERDES_NETIF_CMN_REG_0_WRITE( netif_cmn_reg_0 ) ;

    MS_DELAY( 1 ) ;

    netif_cmn_reg_0.pll_vcalstart = CE_BL_DRV_SERDES_DISABLE ;
    BL_SERDES_NETIF_CMN_REG_0_WRITE( netif_cmn_reg_0 ) ;

#ifndef FSSIM
    do
    {
        tries_num--;
        BL_SERDES_NETIF_CMN_REG_0_READ(netif_cmn_reg_0);
    }
    while((netif_cmn_reg_0.pll_locked != CE_BL_DRV_SERDES_PLL_LOCKED) && (tries_num > 0));

    if(tries_num == 0)
    {
        return CE_BL_DRV_SERDES_ERROR_NETIF_PLL_LOCKING;
    }
#endif

    netif_cmn_reg_0.txcal_start   = CE_BL_DRV_SERDES_ENABLE ;
    netif_cmn_reg_0.rxcal_start   = CE_BL_DRV_SERDES_ENABLE ;
    BL_SERDES_NETIF_CMN_REG_0_WRITE( netif_cmn_reg_0 ) ;

    MS_DELAY( 1 ) ;

#ifndef FSSIM
	tries_num = CS_SERDES_MAX_TRIES;
    do
    {
        tries_num--;
        BL_SERDES_NETIF_CMN_REG_0_READ(netif_cmn_reg_0);
    }
    while(((netif_cmn_reg_0.rxcal_done == 0) || (netif_cmn_reg_0.txcal_done == 0)) && (tries_num > 0));

    if(tries_num == 0)
    {
        return CE_BL_DRV_SERDES_ERROR_NETIF_RX_TX_CAL_DONE_LOCKING;
    }
#endif

	BL_SERDES_NETIF_CMN_REG_3_READ( netif_cmn_reg_3 ) ;
	netif_cmn_reg_3.pll_div0rst_n 	 = CE_BL_DRV_SERDES_UNRESET ;
	netif_cmn_reg_3.pll_div1rst_n 	 = CE_BL_DRV_SERDES_UNRESET ;
	BL_SERDES_NETIF_CMN_REG_3_WRITE( netif_cmn_reg_3 ) ;
	
	/* Configure  NETIF common reg_6 */
	netif_cmn_reg_6.cdr_threshint = 1;
	netif_cmn_reg_6.cdr_thresh2   = 2;
	netif_cmn_reg_6.cdr_thresh1   = CE_SERDES_NETIF_CMN_REG_6_CDR_THRESH1_DEFAULT_VALUE;
	netif_cmn_reg_6.cdr_gain2	  = CE_SERDES_NETIF_CMN_REG_6_CDR_GAIN2_DEFAULT_VALUE;
	netif_cmn_reg_6.cdr_gain1	  = CE_SERDES_NETIF_CMN_REG_6_CDR_GAIN1_DEFAULT_VALUE;
	BL_SERDES_NETIF_CMN_REG_6_WRITE( netif_cmn_reg_6 ) ;

    return CE_BL_DRV_SERDES_NO_ERROR;
}
EXPORT_SYMBOL ( fi_bl_drv_serdes_netif_init );
#endif

#if !defined(_CFE_)
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_gbe_configure_and_enable                                */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GBE/SERDES Driver - Configure and enable GBE SERDES                      */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function configures and enables the GBE SERDES block                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*     CE_BL_DRV_SERDES_NO_ERROR - No error                                   */
/*     CE_BL_DRV_SERDES_WAN_MODE_IS_NOT_GBE - WAN mode is not GBE             */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gbe_configure_and_enable ( void )
{
    SERDES_GPON_CMN_REG_6_DTE  gpon_cmn_reg_6 ;
    SERDES_GPON_LANE_REG_7_DTE gpon_lane_reg_7 ;
    SERDES_GPON_LANE_REG_8_DTE gpon_lane_reg_8 ;
    SERDES_GPON_JTF_XO_REG_11_DTE gpon_jtf_xo_reg_11 ;
	unsigned long ver;

#ifndef FSSIM
    MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_DTE vpb_bridge_cdr_config ;
#endif

    if ( gs_general_config.wan_mode != CE_BL_DRV_SERDES_WAN_MODE_GBE )
    {
        return ( CE_BL_DRV_SERDES_ERROR_WAN_MODE_IS_NOT_GBE ) ;
    }

    /* Nothing to do in this case */
    if ( gs_general_config.gbe_wan_emac_id != CE_BL_DRV_SERDES_EMAC_ID_5 )
    {
        return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
    }

    /* Configure GPON common Reg_6 */
    BL_SERDES_GPON_CMN_REG_6_READ( gpon_cmn_reg_6 ) ;
    gpon_cmn_reg_6.cdr_gain1     = 1;
    gpon_cmn_reg_6.cdr_gain2     = 3;
    gpon_cmn_reg_6.cdr_thresh1   = 2;
    gpon_cmn_reg_6.cdr_thresh2   = 3;
    gpon_cmn_reg_6.cdr_threshint = 3;
    BL_SERDES_GPON_CMN_REG_6_WRITE( gpon_cmn_reg_6 ) ;

    /* Configure GPON lane Reg_8 */
    BL_SERDES_GPON_LANE_REG_8_READ( gpon_lane_reg_8 ) ;
	BL_MIPS_BLOCKS_VPB_BRIDGE_VERSION_READ(ver);
	if(ver & 0x1)
		gpon_lane_reg_8.drv_t_slewsel  = 0x3;
    gpon_lane_reg_8.drv_t_deemp    = 2;
    gpon_lane_reg_8.drv_t_drvdaten = CE_BL_DRV_SERDES_ENABLE ;
    BL_SERDES_GPON_LANE_REG_8_WRITE( gpon_lane_reg_8 ) ;

    /* Unreset */
    BL_SERDES_GPON_LANE_REG_7_READ( gpon_lane_reg_7 ) ;
    gpon_lane_reg_7.rx_rst_n = CE_BL_DRV_SERDES_UNRESET ;
    gpon_lane_reg_7.tx_rst_n = CE_BL_DRV_SERDES_UNRESET ;
    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;

    BL_SERDES_GPON_JTF_XO_REG_11_READ( gpon_jtf_xo_reg_11 ) ;
    gpon_jtf_xo_reg_11.biasj_en        = CE_BL_DRV_SERDES_ENABLE ;
    gpon_jtf_xo_reg_11.cdrjtf_gain     = 0;
    gpon_jtf_xo_reg_11.cdrjtf_sdthresh = 2;
    gpon_jtf_xo_reg_11.iqgenj_en       = CE_BL_DRV_SERDES_ENABLE ;
    gpon_jtf_xo_reg_11.pij_en          = CE_BL_DRV_SERDES_ENABLE ;
    gpon_jtf_xo_reg_11.refmuxd_drven   = CE_BL_DRV_SERDES_ENABLE ;
    gpon_jtf_xo_reg_11.refmuxd_sel     = CE_SERDES_GPON_JTF_XO_REG_11_REFMUXD_SEL_DEFAULT_VALUE ;
    gpon_jtf_xo_reg_11.divj_divsel     = 0;
    gpon_jtf_xo_reg_11.divj_muxsel     = 1;
    gpon_jtf_xo_reg_11.divj_rst_n      = CE_BL_DRV_SERDES_UNRESET ;
    BL_SERDES_GPON_JTF_XO_REG_11_WRITE( gpon_jtf_xo_reg_11 ) ;

    gpon_jtf_xo_reg_11.rx_tenbit       = CE_BL_DRV_SERDES_ENABLE ;
    gpon_jtf_xo_reg_11.tx_tenbit       = CE_BL_DRV_SERDES_ENABLE ;
    BL_SERDES_GPON_JTF_XO_REG_11_WRITE( gpon_jtf_xo_reg_11 ) ;

#ifndef FSSIM
        BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_READ( vpb_bridge_cdr_config ) ;
        vpb_bridge_cdr_config.cdr_fifo_rst_n        = CE_BL_DRV_SERDES_UNRESET ;
        vpb_bridge_cdr_config.gpon_cdr_txfifo_rst_n = CE_BL_DRV_SERDES_UNRESET ;
        vpb_bridge_cdr_config.gpon_cdr_rxfifo_rst_n = CE_BL_DRV_SERDES_UNRESET ;
        BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_WRITE( vpb_bridge_cdr_config ) ;
#endif

    return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
}
EXPORT_SYMBOL ( fi_bl_drv_serdes_gbe_configure_and_enable );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_gbe_reset                                               */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GBE/SERDES Driver - Reset/Unreset GBE SERDES                             */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function resets/unresets the GBE SERDES block (active low reset)    */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_reset - Reset/Unreset                                                 */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*     CE_BL_DRV_SERDES_NO_ERROR - No error                                   */
/*     CE_BL_DRV_SERDES_INVALID_RESET_VALUE - Invalid reset value             */
/*     CE_BL_DRV_SERDES_WAN_MODE_IS_NOT_GBE - WAN mode is not GBE             */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gbe_reset ( BL_DRV_SERDES_RESET_DTE xi_reset )
{
    SERDES_GPON_CMN_REG_0_DTE     gpon_cmn_reg_0 ;
    SERDES_GPON_CMN_REG_1_DTE     gpon_cmn_reg_1 ;
    SERDES_GPON_CMN_REG_2_DTE     gpon_cmn_reg_2 ;
    SERDES_GPON_CMN_REG_3_DTE     gpon_cmn_reg_3 ;
    SERDES_GPON_CMN_REG_4_DTE     gpon_cmn_reg_4 ;
    SERDES_GPON_LANE_REG_7_DTE    gpon_lane_reg_7 ;
    SERDES_GPON_JTF_XO_REG_11_DTE gpon_jtf_xo_reg_11 ;

#ifndef FSSIM
    MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_DTE vpb_bridge_cdr_config ;
    uint32_t tries_num = CS_SERDES_MAX_TRIES ;
#endif

    if ( gs_general_config.wan_mode != CE_BL_DRV_SERDES_WAN_MODE_GBE )
    {
        return ( CE_BL_DRV_SERDES_ERROR_WAN_MODE_IS_NOT_GBE ) ;
    }

    /* Nothing to do in this case */
    if ( gs_general_config.gbe_wan_emac_id != CE_BL_DRV_SERDES_EMAC_ID_5 )
    {
        return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
    }

    /* Proceed according to xi_reset parameter value */
    switch ( xi_reset )
    {
    case CE_BL_DRV_SERDES_UNRESET:

        break;

    case CE_BL_DRV_SERDES_RESET:

#ifndef FSSIM

        BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_READ( vpb_bridge_cdr_config ) ;
        vpb_bridge_cdr_config.cdr_fifo_rst_n        = CE_BL_DRV_SERDES_RESET ;
        vpb_bridge_cdr_config.gpon_cdr_txfifo_rst_n = CE_BL_DRV_SERDES_RESET ;
        vpb_bridge_cdr_config.gpon_cdr_rxfifo_rst_n = CE_BL_DRV_SERDES_RESET ;
        BL_MIPS_BLOCKS_VPB_BRIDGE_CDR_CONFIG_WRITE( vpb_bridge_cdr_config ) ;

#endif

        /* Reset GPON common reg_1 */
        BL_SERDES_GPON_CMN_REG_1_READ( gpon_cmn_reg_1 ) ;
        gpon_cmn_reg_1.pll_dsmbyp    = 0 ;
        gpon_cmn_reg_1.pll_fbdivint  = CE_SERDES_GPON_CMN_REG_1_PLL_FBDIVINT_DEFAULT_VALUE_RESET_VALUE ;
        gpon_cmn_reg_1.pll_fbdivfrac = CE_SERDES_GPON_CMN_REG_1_PLL_FBDIVFRAC_DEFAULT_VALUE_RESET_VALUE ;
        BL_SERDES_GPON_CMN_REG_1_WRITE( gpon_cmn_reg_1 ) ;

        /* Reset GPON common reg_2 */
        BL_SERDES_GPON_CMN_REG_2_READ( gpon_cmn_reg_2 ) ;
        gpon_cmn_reg_2.pll_sscen = CE_SERDES_GPON_CMN_REG_2_PLL_SSCEN_DEFAULT_VALUE_RESET_VALUE ;
        BL_SERDES_GPON_CMN_REG_2_WRITE( gpon_cmn_reg_2 ) ;

        /* Reset GPON common reg_3 */
        BL_SERDES_GPON_CMN_REG_3_READ( gpon_cmn_reg_3 ) ;
        gpon_cmn_reg_3.pll_div0clksel    = 0x14 ;
        gpon_cmn_reg_3.pll_div1clksel    = 0x5 ;
        gpon_cmn_reg_3.pll_div0rst_n     = CE_BL_DRV_SERDES_RESET ;
        gpon_cmn_reg_3.pll_div1rst_n     = CE_BL_DRV_SERDES_RESET ;
        BL_SERDES_GPON_CMN_REG_3_WRITE( gpon_cmn_reg_3 ) ;

        gpon_cmn_reg_3.pll_div0rst_n     = CE_BL_DRV_SERDES_UNRESET ;
        gpon_cmn_reg_3.pll_div1rst_n     = CE_BL_DRV_SERDES_UNRESET ;
        BL_SERDES_GPON_CMN_REG_3_WRITE( gpon_cmn_reg_3 ) ;

        gpon_cmn_reg_3.pll_refclk0drven  = CE_BL_DRV_SERDES_ENABLE ;
        gpon_cmn_reg_3.pll_refclk0termen = CE_BL_DRV_SERDES_ENABLE ;
        BL_SERDES_GPON_CMN_REG_3_WRITE( gpon_cmn_reg_3 ) ;

        /* Reset GPON common reg_4 */
        BL_SERDES_GPON_CMN_REG_4_READ( gpon_cmn_reg_4 ) ;
        gpon_cmn_reg_4.pll_rst_n = CE_BL_DRV_SERDES_RESET ;
        BL_SERDES_GPON_CMN_REG_4_WRITE( gpon_cmn_reg_4 ) ;

        gpon_cmn_reg_4.pll_rst_n = CE_BL_DRV_SERDES_UNRESET ;
        BL_SERDES_GPON_CMN_REG_4_WRITE( gpon_cmn_reg_4 ) ;

        /* Reset GPON common reg_0, enable the calibration */
        BL_SERDES_GPON_CMN_REG_0_READ( gpon_cmn_reg_0 ) ;
        gpon_cmn_reg_0.bias_en   = CE_BL_DRV_SERDES_ENABLE ;
        gpon_cmn_reg_0.clkdis_en = CE_BL_DRV_SERDES_ENABLE ;
        gpon_cmn_reg_0.pll_en    = CE_BL_DRV_SERDES_ENABLE ;
        BL_SERDES_GPON_CMN_REG_0_WRITE( gpon_cmn_reg_0 ) ;

        MS_DELAY( 1 ) ;

        /* Reset GPON common reg_0, wait to PLL locked 1 */
        gpon_cmn_reg_0.pll_vcalstart = CE_BL_DRV_SERDES_ENABLE ;
        BL_SERDES_GPON_CMN_REG_0_WRITE( gpon_cmn_reg_0 ) ;

        MS_DELAY( 1 ) ;

        gpon_cmn_reg_0.pll_vcalstart = CE_BL_DRV_SERDES_DISABLE ;
        BL_SERDES_GPON_CMN_REG_0_WRITE( gpon_cmn_reg_0 ) ;

        MS_DELAY( 1 ) ;

        BL_SERDES_GPON_JTF_XO_REG_11_READ( gpon_jtf_xo_reg_11 ) ;
        gpon_jtf_xo_reg_11.osc_en = CE_BL_DRV_SERDES_ENABLE ;
        BL_SERDES_GPON_JTF_XO_REG_11_WRITE( gpon_jtf_xo_reg_11 ) ;

        MS_DELAY( 1 ) ;

#   ifndef FSSIM
        do
        {
            tries_num -- ;
            BL_SERDES_GPON_CMN_REG_0_READ( gpon_cmn_reg_0 ) ;
        }
        while ( ( gpon_cmn_reg_0.pll_locked != CE_BL_DRV_SERDES_PLL_LOCKED ) &&
                ( tries_num > 0 ) ) ;

        if ( tries_num == 0 )
        {
            return ( CE_BL_DRV_SERDES_ERROR_GPON_PLL_LOCKING ) ;
        }
#   endif


        /* Reset GPON common reg_0, wait to rxcal_done 0 and txcal_done 0 */
        BL_SERDES_GPON_CMN_REG_0_READ( gpon_cmn_reg_0 ) ;
        gpon_cmn_reg_0.rxcal_start = CE_BL_DRV_SERDES_ENABLE ;
        gpon_cmn_reg_0.txcal_start = CE_BL_DRV_SERDES_ENABLE ;
        BL_SERDES_GPON_CMN_REG_0_WRITE( gpon_cmn_reg_0 ) ;

        MS_DELAY( 1 ) ;

#   ifndef FSSIM
        tries_num = CS_SERDES_MAX_TRIES ;
        do
        {
            tries_num -- ;
            BL_SERDES_GPON_CMN_REG_0_READ( gpon_cmn_reg_0 ) ;
        }

        while ( ( gpon_cmn_reg_0.rxcal_done != CE_BL_DRV_SERDES_ENABLE ) &&
                ( gpon_cmn_reg_0.txcal_done != CE_BL_DRV_SERDES_ENABLE ) &&
                ( tries_num > 0 ) ) ;

        if ( tries_num == 0 )
        {
            return ( CE_BL_DRV_SERDES_ERROR_GPON_RX_TX_CAL_DONE_LOCKING ) ;
        }
#   endif

        /* Reset GPON jtf xo reg_11 */
        BL_SERDES_GPON_JTF_XO_REG_11_READ( gpon_jtf_xo_reg_11 ) ;
        gpon_jtf_xo_reg_11.refmuxd_sel      = 0 ;
        BL_SERDES_GPON_JTF_XO_REG_11_WRITE( gpon_jtf_xo_reg_11 ) ;

        /* Reset GPON lane reg_7, init and enable transmitter */
        BL_SERDES_GPON_LANE_REG_7_READ( gpon_lane_reg_7 ) ;
        gpon_lane_reg_7.rx_en       = CE_BL_DRV_SERDES_ENABLE ;
        gpon_lane_reg_7.rx_rst_n    = CE_BL_DRV_SERDES_UNRESET ;
        gpon_lane_reg_7.tx_en       = CE_BL_DRV_SERDES_ENABLE ;
        gpon_lane_reg_7.tx_rst_n    = CE_BL_DRV_SERDES_UNRESET ;
        gpon_lane_reg_7.rx_subrate  = 3;
        BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;

        break;

    default:
        return ( CE_BL_DRV_SERDES_ERROR_INVALID_RESET_VALUE ) ;
    }

    /* Reset or Unreset */
    BL_SERDES_GPON_LANE_REG_7_READ( gpon_lane_reg_7 ) ;
    gpon_lane_reg_7.rx_rst_n = ( uint32_t ) xi_reset ;
    gpon_lane_reg_7.tx_rst_n = ( uint32_t ) xi_reset ;
    BL_SERDES_GPON_LANE_REG_7_WRITE( gpon_lane_reg_7 ) ;

    return ( CE_BL_DRV_SERDES_NO_ERROR ) ;
}
EXPORT_SYMBOL ( fi_bl_drv_serdes_gbe_reset );
#endif

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_configure_serdes_amplitude_reg                                        */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GBE/SERDES Driver - Set SerDes amplitude                                 */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function sets the chosen SerDes amplitude configuration.            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_serdes_id - 0-8 (0=gpon, 1=pci1, 2=pci2, 3-6=sgmii, 7=qsgmii1,        */
/*                  8=hisgmii)                                                */
/*                  | Default: 00                                             */
/*                                                                            */
/*   xi_serdes_conf.ampt_val - 0-15 (0 = max , 15 = roughly half amplitude)   */
/*                                                                            */
/*   xi_serdes_conf.de_emph_val - 0-3 (0 = no de-emphasis, 1 = ~3.5db,        */
/*                                2 = ~6db, 3 = ~8db) | Default: 0.           */
/*                                                                            */
/*   xi_serdes_conf.delay_ctrl - value range: 0-3 (0 = 1 clk cycle,           */
/*                               1 = 2 clk cycle,  2,3 = 4 clk cycle (max.)   */
/*                               | Default: GPON – 3, Rest - 0                */
/*                                                                            */
/* Output:  void                                                                  */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_configure_serdes_amplitude_reg(BL_DRV_SERDES_PHYSICAL_PORT xi_serdes_id,
        BL_DRV_SERDES_CONFIGURATION * const xi_serdes_conf )
{
    SERDES_GPON_LANE_REG_8_DTE reg_8;

    switch (xi_serdes_id) {
    case CE_BL_GPON:
        BL_SERDES_GPON_LANE_REG_8_READ( reg_8 );
        pi_configure_lane_reg_8( xi_serdes_conf, &reg_8);
        BL_SERDES_GPON_LANE_REG_8_WRITE( reg_8 );
        break;
    case CE_BL_PCI_0:
        BL_SERDES_PCIE_LANE_REG_8_READ(0,reg_8);
        pi_configure_lane_reg_8( xi_serdes_conf, &reg_8);
        BL_SERDES_PCIE_LANE_REG_8_WRITE(0,reg_8);
        break;
    case CE_BL_PCI_1:
        BL_SERDES_PCIE_LANE_REG_8_READ(1,reg_8);
        pi_configure_lane_reg_8( xi_serdes_conf, &reg_8);
        BL_SERDES_PCIE_LANE_REG_8_WRITE(1,reg_8);
        break;
    case CE_BL_SGMII_0:
        BL_SERDES_NETIF_LANE_REG_8_READ(0, reg_8);
        pi_configure_lane_reg_8( xi_serdes_conf, &reg_8);
        BL_SERDES_NETIF_LANE_REG_8_WRITE(0, reg_8);
        break;
    case CE_BL_SGMII_1:
        BL_SERDES_NETIF_LANE_REG_8_READ(1, reg_8);
        pi_configure_lane_reg_8( xi_serdes_conf, &reg_8);
        BL_SERDES_NETIF_LANE_REG_8_WRITE(1, reg_8);
        break;
    case CE_BL_SGMII_2:
        BL_SERDES_NETIF_LANE_REG_8_READ(2, reg_8);
        pi_configure_lane_reg_8( xi_serdes_conf, &reg_8);
        BL_SERDES_NETIF_LANE_REG_8_WRITE(2, reg_8);
        break;
    case CE_BL_SGMII_3:
        BL_SERDES_NETIF_LANE_REG_8_READ(3, reg_8);
        pi_configure_lane_reg_8( xi_serdes_conf, &reg_8);
        BL_SERDES_NETIF_LANE_REG_8_WRITE(3, reg_8);
        break;
    case CE_BL_QSGMII:
        BL_SERDES_NETIF_LANE_REG_8_READ(1, reg_8);
        pi_configure_lane_reg_8( xi_serdes_conf, &reg_8);
        BL_SERDES_NETIF_LANE_REG_8_WRITE(1, reg_8);
        break;
    case CE_BL_HISGMII:
        BL_SERDES_NETIF_LANE_REG_8_READ(1, reg_8);
        pi_configure_lane_reg_8( xi_serdes_conf, &reg_8);
        BL_SERDES_NETIF_LANE_REG_8_WRITE(1, reg_8);
        BL_SERDES_NETIF_LANE_REG_8_READ(2, reg_8);
        pi_configure_lane_reg_8( xi_serdes_conf, &reg_8);
        BL_SERDES_NETIF_LANE_REG_8_WRITE(2, reg_8);
        break;
    default:
        return CE_BL_DRV_SERDES_ERROR_INVALID_SERDES_PHYSICAL_PORT;
        break;
    }
    return CE_BL_DRV_SERDES_NO_ERROR;
}
EXPORT_SYMBOL(fi_configure_serdes_amplitude_reg);

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_get_serdes_amplitude                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GBE/SERDES Driver - Get serdes amplitude configuration                   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function gets the chosen serdes amplitude configuration.            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_serdes_id - 1-7 (1=gpon, 2=pci1, 3=pci2, 4=qsgmii, 5-8=sgmii1-4)      */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   xo_serdes_conf - the serdes configuration parameters                     */
/*                                                                            */
/*   BL_ERROR_DTE - Return code                                               */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_get_serdes_amplitude(BL_DRV_SERDES_PHYSICAL_PORT xi_serdes_id,
        BL_DRV_SERDES_CONFIGURATION * const xo_serdes_conf )
{
    SERDES_GPON_LANE_REG_8_DTE reg_8 = {};

    switch (xi_serdes_id) {
        case CE_BL_GPON:
            BL_SERDES_GPON_LANE_REG_8_READ( reg_8 );
            pi_get_lane_reg_8(reg_8, xo_serdes_conf);
            break;
        case CE_BL_PCI_0:
            BL_SERDES_PCIE_LANE_REG_8_READ(0,reg_8);
            pi_get_lane_reg_8(reg_8, xo_serdes_conf);
            break;
        case CE_BL_PCI_1:
            BL_SERDES_PCIE_LANE_REG_8_READ(1,reg_8);
            pi_get_lane_reg_8(reg_8, xo_serdes_conf);
            break;
        case CE_BL_SGMII_0:
            BL_SERDES_NETIF_LANE_REG_8_READ(0, reg_8);
            pi_get_lane_reg_8(reg_8, xo_serdes_conf);
            break;
        case CE_BL_SGMII_1:
            BL_SERDES_NETIF_LANE_REG_8_READ(1, reg_8);
            pi_get_lane_reg_8(reg_8, xo_serdes_conf);
            break;
        case CE_BL_SGMII_2:
            BL_SERDES_NETIF_LANE_REG_8_READ(2, reg_8);
            pi_get_lane_reg_8(reg_8, xo_serdes_conf);
            break;
        case CE_BL_SGMII_3:
            BL_SERDES_NETIF_LANE_REG_8_READ(3, reg_8);
            pi_get_lane_reg_8(reg_8, xo_serdes_conf);
            break;
        case CE_BL_QSGMII:
            BL_SERDES_NETIF_LANE_REG_8_READ(1, reg_8);
            pi_get_lane_reg_8(reg_8, xo_serdes_conf);
            break;
        default:
            return CE_BL_DRV_SERDES_ERROR_INVALID_SERDES_PHYSICAL_PORT;
            break;
    }
    return CE_BL_DRV_SERDES_NO_ERROR;
}
EXPORT_SYMBOL(fi_get_serdes_amplitude);

/******************************************************************************/
/*                                                                            */
/* Internal functions implementation                                          */
/*                                                                            */
/******************************************************************************/

static void p_serdes_netif_configure_and_enable_hisgmii ( BL_DRV_SERDES_EMAC_ID_DTE xi_emac_id )
{
}

static void p_serdes_netif_configure_and_enable_qsgmii(void)
{
    SERDES_NETIF_LANE_REG_7_DTE netif_lane_reg_7 ;
    SERDES_NETIF_LANE_REG_8_DTE netif_lane_reg_8 ;
    SERDES_NETIF_LANE_REG_9_DTE netif_lane_reg_9 ;
    SERDES_NETIF_LANE_REG_10_DTE netif_lane_reg_10 ;
	NET_IF_CORE_GENERAL_SGMII_SRDS_DTE netif_general_sgmii_serdes = {0};
	unsigned long ver;

    /* Configure NETIF LANE Reg_7, enable TX and RX */
    BL_SERDES_NETIF_LANE_REG_7_READ( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_7 );  
    netif_lane_reg_7.rx_en         = CE_BL_DRV_SERDES_ENABLE;
    netif_lane_reg_7.rx_subrate    = 1;
    netif_lane_reg_7.tx_en         = CE_BL_DRV_SERDES_ENABLE;
    netif_lane_reg_7.tx_subrate    = 1;
    BL_SERDES_NETIF_LANE_REG_7_WRITE( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_7 );  

    /* Configure NETIF LANE Reg_10 cdr hold */
    BL_SERDES_NETIF_LANE_REG_10_READ( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_10 ) ;  
    netif_lane_reg_10.cdr_hold = 1 ;
    BL_SERDES_NETIF_LANE_REG_10_WRITE( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_10 ) ;  
    BL_SERDES_NETIF_LANE_REG_10_WRITE( CE_BL_DRV_SERDES_EMAC_ID_1 , netif_lane_reg_10 ) ;  
    BL_SERDES_NETIF_LANE_REG_10_WRITE( CE_BL_DRV_SERDES_EMAC_ID_2 , netif_lane_reg_10 ) ;  
    BL_SERDES_NETIF_LANE_REG_10_WRITE( CE_BL_DRV_SERDES_EMAC_ID_3 , netif_lane_reg_10 ) ;  

    /* Configure NETIF LANE Reg_10 pi reset*/
    netif_lane_reg_10.pi_reset= 1 ;
    BL_SERDES_NETIF_LANE_REG_10_WRITE( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_10 ) ;  
    BL_SERDES_NETIF_LANE_REG_10_WRITE( CE_BL_DRV_SERDES_EMAC_ID_1 , netif_lane_reg_10 ) ;  
    BL_SERDES_NETIF_LANE_REG_10_WRITE( CE_BL_DRV_SERDES_EMAC_ID_2 , netif_lane_reg_10 ) ;  
    BL_SERDES_NETIF_LANE_REG_10_WRITE( CE_BL_DRV_SERDES_EMAC_ID_3 , netif_lane_reg_10 ) ;  

    /* Configure NETIF LANE Reg_10 pi unreset*/
    netif_lane_reg_10.pi_reset= 0 ;
    BL_SERDES_NETIF_LANE_REG_10_WRITE( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_10 ) ;  

	/* Configure NETIF LANE Reg_7, reset TX and RX */
	netif_lane_reg_7.rx_rst_n 	 = CE_BL_DRV_SERDES_RESET ;
	netif_lane_reg_7.tx_rst_n 	 = CE_BL_DRV_SERDES_RESET ;
	BL_SERDES_NETIF_LANE_REG_7_WRITE( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_7 ) ;  

	/* Configure NETIF LANE Reg_7, unreset TX and RX */
	netif_lane_reg_7.rx_rst_n 	 = CE_BL_DRV_SERDES_UNRESET ;
	netif_lane_reg_7.tx_rst_n 	 = CE_BL_DRV_SERDES_UNRESET ;
	BL_SERDES_NETIF_LANE_REG_7_WRITE( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_7 ) ;  

	netif_general_sgmii_serdes.rxfiforst0= 0;
	netif_general_sgmii_serdes.txfiforst0= 1;
    BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
	netif_general_sgmii_serdes.rxfiforst0= 1;
	netif_general_sgmii_serdes.txfiforst0= 0;
    BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
	netif_general_sgmii_serdes.rxfiforst0= 1;
	netif_general_sgmii_serdes.txfiforst0= 1;
    BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
	
    /* Configure NETIF LANE Reg_10 cdr unhold */
    netif_lane_reg_10.cdr_hold = 0 ;
    BL_SERDES_NETIF_LANE_REG_10_WRITE( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_10 ) ;  

    /* Configure NETIF LANE Reg_9 */
    BL_SERDES_NETIF_LANE_REG_9_READ( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_9 ) ;
    netif_lane_reg_9.rcv_r_rcvcalovren = CE_BL_DRV_SERDES_ENABLE;
    BL_SERDES_NETIF_LANE_REG_9_WRITE( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_9 ) ;

    /* Configure NETIF LANE Reg_8 */
    BL_SERDES_NETIF_LANE_REG_8_READ( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_8 ) ;  
    netif_lane_reg_8.drv_t_drvdaten = CE_BL_DRV_SERDES_ENABLE ;
	BL_MIPS_BLOCKS_VPB_BRIDGE_VERSION_READ(ver);
	if(ver & 0x1)
		netif_lane_reg_8.drv_t_slewsel  = 0x3;
	else
		netif_lane_reg_8.drv_t_slewsel  = 0x7 ;
    netif_lane_reg_8.drv_t_vmarg    = 0x10 ;
    netif_lane_reg_8.serl_t_dlysel  = 0x0 ;
    BL_SERDES_NETIF_LANE_REG_8_WRITE( CE_BL_DRV_SERDES_EMAC_ID_0 , netif_lane_reg_8 ) ;
	
	netif_general_sgmii_serdes.rxfiforst0= 1;
	netif_general_sgmii_serdes.txfiforst0= 0;
    BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
	netif_general_sgmii_serdes.rxfiforst0= 1;
	netif_general_sgmii_serdes.txfiforst0= 1;
    BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
}

static void p_serdes_netif_configure_and_enable_sgmii ( BL_DRV_SERDES_EMAC_ID_DTE xi_emac_id )
{
    SERDES_NETIF_LANE_REG_7_DTE netif_lane_reg_7 ;
    SERDES_NETIF_LANE_REG_8_DTE netif_lane_reg_8 ;
    SERDES_NETIF_LANE_REG_9_DTE netif_lane_reg_9 ;
    SERDES_NETIF_LANE_REG_10_DTE netif_lane_reg_10 ;
	NET_IF_CORE_GENERAL_SGMII_SRDS_DTE netif_general_sgmii_serdes;
	unsigned long ver;

    /* Configure NETIF LANE Reg_7, enable TX and RX */
    BL_SERDES_NETIF_LANE_REG_7_READ( xi_emac_id , netif_lane_reg_7 );  
    netif_lane_reg_7.rx_en         = CE_BL_DRV_SERDES_ENABLE;
    netif_lane_reg_7.tx_en         = CE_BL_DRV_SERDES_ENABLE;
   BL_SERDES_NETIF_LANE_REG_7_WRITE( xi_emac_id , netif_lane_reg_7 );  

    /* Configure NETIF LANE Reg_10 cdr hold */
    BL_SERDES_NETIF_LANE_REG_10_READ( xi_emac_id , netif_lane_reg_10 ) ;  
    netif_lane_reg_10.cdr_hold = 1 ;
    BL_SERDES_NETIF_LANE_REG_10_WRITE( xi_emac_id , netif_lane_reg_10 ) ;  

    /* Configure NETIF LANE Reg_10 pi reset*/
    netif_lane_reg_10.pi_reset= 1 ;
    BL_SERDES_NETIF_LANE_REG_10_WRITE( xi_emac_id , netif_lane_reg_10 ) ;  

    /* Configure NETIF LANE Reg_10 pi unreset*/
    netif_lane_reg_10.pi_reset= 0 ;
    BL_SERDES_NETIF_LANE_REG_10_WRITE( xi_emac_id , netif_lane_reg_10 ) ;  

	/* Configure NETIF LANE Reg_7, reset TX and RX */
	netif_lane_reg_7.rx_rst_n 	 = CE_BL_DRV_SERDES_RESET ;
	netif_lane_reg_7.tx_rst_n 	 = CE_BL_DRV_SERDES_RESET ;
	BL_SERDES_NETIF_LANE_REG_7_WRITE( xi_emac_id , netif_lane_reg_7 ) ;  

	/* Configure NETIF LANE Reg_7, unreset TX and RX */
	netif_lane_reg_7.rx_rst_n 	 = CE_BL_DRV_SERDES_UNRESET ;
	netif_lane_reg_7.tx_rst_n 	 = CE_BL_DRV_SERDES_UNRESET ;
	BL_SERDES_NETIF_LANE_REG_7_WRITE( xi_emac_id , netif_lane_reg_7 ) ;  

	BL_NET_IF_CORE_GENERAL_SGMII_SRDS_READ(0, netif_general_sgmii_serdes);
	switch(xi_emac_id)
	{
		case CE_BL_DRV_SERDES_EMAC_ID_0:
			netif_general_sgmii_serdes.rxfiforst0= 0;
			netif_general_sgmii_serdes.txfiforst0= 1;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst0= 1;
			netif_general_sgmii_serdes.txfiforst0= 0;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst0= 1;
			netif_general_sgmii_serdes.txfiforst0= 1;
			break;
			
		case CE_BL_DRV_SERDES_EMAC_ID_1:
			netif_general_sgmii_serdes.rxfiforst1= 0;
			netif_general_sgmii_serdes.txfiforst1= 1;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst1= 1;
			netif_general_sgmii_serdes.txfiforst1= 0;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst1= 1;
			netif_general_sgmii_serdes.txfiforst1= 1;
			break;
			
		case CE_BL_DRV_SERDES_EMAC_ID_2:
			netif_general_sgmii_serdes.rxfiforst2= 0;
			netif_general_sgmii_serdes.txfiforst2= 1;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst2= 1;
			netif_general_sgmii_serdes.txfiforst2= 0;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst2= 1;
			netif_general_sgmii_serdes.txfiforst2= 1;
			break;
			
		case CE_BL_DRV_SERDES_EMAC_ID_3:
			netif_general_sgmii_serdes.rxfiforst3= 0;
			netif_general_sgmii_serdes.txfiforst3= 1;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst3= 1;
			netif_general_sgmii_serdes.txfiforst3= 0;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst3= 1;
			netif_general_sgmii_serdes.txfiforst3= 1;
			break;
		default:
			break;
	}
	BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
	
    /* Configure NETIF LANE Reg_10 cdr unhold */
    netif_lane_reg_10.cdr_hold = 0 ;
    BL_SERDES_NETIF_LANE_REG_10_WRITE( xi_emac_id , netif_lane_reg_10 ) ;  

    /* Configure NETIF LANE Reg_9 */
    BL_SERDES_NETIF_LANE_REG_9_READ( xi_emac_id , netif_lane_reg_9 ) ;
    netif_lane_reg_9.rcv_r_eq          = 0x8 ;
    netif_lane_reg_9.rcv_r_rcvcalovren = CE_BL_DRV_SERDES_ENABLE;
    BL_SERDES_NETIF_LANE_REG_9_WRITE( xi_emac_id , netif_lane_reg_9 ) ;

    /* Configure NETIF LANE Reg_8 */
    BL_SERDES_NETIF_LANE_REG_8_READ( xi_emac_id , netif_lane_reg_8 ) ;  
    netif_lane_reg_8.drv_t_drvdaten = CE_BL_DRV_SERDES_ENABLE ;
	BL_MIPS_BLOCKS_VPB_BRIDGE_VERSION_READ(ver);
	if(ver & 0x1)
		netif_lane_reg_8.drv_t_slewsel  = 0x3;
	else
		netif_lane_reg_8.drv_t_slewsel  = 0x7 ;
    netif_lane_reg_8.drv_t_vmarg    = 0x0 ;
    netif_lane_reg_8.serl_t_dlysel  = 0x0 ;
    BL_SERDES_NETIF_LANE_REG_8_WRITE( xi_emac_id , netif_lane_reg_8 ) ;

	BL_NET_IF_CORE_GENERAL_SGMII_SRDS_READ(0, netif_general_sgmii_serdes);
	switch(xi_emac_id)
	{
		case CE_BL_DRV_SERDES_EMAC_ID_0:
			netif_general_sgmii_serdes.rxfiforst0= 1;
			netif_general_sgmii_serdes.txfiforst0= 0;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst0= 1;
			netif_general_sgmii_serdes.txfiforst0= 1;
			break;
			
		case CE_BL_DRV_SERDES_EMAC_ID_1:
			netif_general_sgmii_serdes.rxfiforst1= 1;
			netif_general_sgmii_serdes.txfiforst1= 0;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst1= 1;
			netif_general_sgmii_serdes.txfiforst1= 1;
			break;
			
		case CE_BL_DRV_SERDES_EMAC_ID_2:
			netif_general_sgmii_serdes.rxfiforst2= 1;
			netif_general_sgmii_serdes.txfiforst2= 0;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst2= 1;
			netif_general_sgmii_serdes.txfiforst2= 1;
			break;
			
		case CE_BL_DRV_SERDES_EMAC_ID_3:
			netif_general_sgmii_serdes.rxfiforst3= 1;
			netif_general_sgmii_serdes.txfiforst3= 0;
			BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
			netif_general_sgmii_serdes.rxfiforst3= 1;
			netif_general_sgmii_serdes.txfiforst3= 1;
			break;
		default:
			break;
	}
	BL_NET_IF_CORE_GENERAL_SGMII_SRDS_WRITE(0, netif_general_sgmii_serdes);
}


static void pi_configure_lane_reg_8 ( BL_DRV_SERDES_CONFIGURATION * const xi_conf,
                                      SERDES_GPON_LANE_REG_8_DTE * const xo_reg )
{
    if ( xi_conf->ampt_val != CE_BL_AMPLITUDE_DEFAULT_VAL )
    {
        xo_reg->drv_t_vmarg = xi_conf->ampt_val;
    }
    if ( xi_conf->de_emph_val != CE_BL_DE_EMPHASIS_DEFAULT_VAL )
    {
        xo_reg->drv_t_deemp = xi_conf->de_emph_val;
    }
    if ( xi_conf->delay_ctrl != CE_BL_DELAY_DEFAULT_VAL )
    {
        xo_reg->serl_t_dlysel = xi_conf->delay_ctrl;
    }
}

static void pi_get_lane_reg_8 ( SERDES_GPON_LANE_REG_8_DTE xi_reg,
                                BL_DRV_SERDES_CONFIGURATION * const xo_conf )
{
    xo_conf->ampt_val = xi_reg.drv_t_vmarg ;
    xo_conf->de_emph_val = xi_reg.drv_t_deemp ;
    xo_conf->delay_ctrl = xi_reg.serl_t_dlysel ;
}
