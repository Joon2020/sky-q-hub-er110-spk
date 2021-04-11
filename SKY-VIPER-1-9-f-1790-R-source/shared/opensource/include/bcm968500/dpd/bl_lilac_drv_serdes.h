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
/* This header file defines all datatypes and functions exported for the      */
/* Lilac SERDES driver.                                                       */
/*                                                                            */
/******************************************************************************/


#ifndef BL_DRV_SERDES_H_INCLUDED
#define BL_DRV_SERDES_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif


/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/

/******************************************************************************/
/*                                                                            */
/* Types and values definitions                                               */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/* Error codes returned by SERDES driver APIs                                 */
/******************************************************************************/
typedef enum
{
    CE_BL_DRV_SERDES_NO_ERROR ,

    CE_BL_DRV_SERDES_ERROR_INVALID_RESET_VALUE ,
    CE_BL_DRV_SERDES_ERROR_INVALID_EMACS_GROUP_MODE_VALUE ,
    CE_BL_DRV_SERDES_ERROR_INVALID_GBE_WAN_EMAC_ID_VALUE ,
    CE_BL_DRV_SERDES_ERROR_INVALID_WAN_MODE_VALUE ,
    CE_BL_DRV_SERDES_ERROR_NULL_POINTER ,
    CE_BL_DRV_SERDES_ERROR_INVALID_EMAC5_MODE_VALUE ,
    CE_BL_DRV_SERDES_ERROR_GPON_PLL_LOCKING ,
    CE_BL_DRV_SERDES_ERROR_GPON_RX_TX_CAL_DONE_LOCKING ,
    CE_BL_DRV_SERDES_ERROR_NETIF_PLL_LOCKING ,
	CE_BL_DRV_SERDES_ERROR_NETIF_RX_TX_CAL_DONE_LOCKING ,
    CE_BL_DRV_SERDES_ERROR_WAN_MODE_IS_NOT_GBE ,
    CE_BL_DRV_SERDES_ERROR_WAN_MODE_IS_NOT_GPON ,
    CE_BL_DRV_SERDES_ERROR_HISGMII_NOT_SUPPORTED ,
    CE_BL_DRV_SERDES_ERROR_INVALID_PRBS_MODE ,
    CE_BL_DRV_SERDES_ERROR_INVALID_SERDES_PHYSICAL_PORT ,

    CE_BL_DRV_SERDES_NUMBER_OF_ERRORS
}
BL_DRV_SERDES_ERROR_DTE ;

/******************************************************************************/
/* Boolean                                                                    */
/******************************************************************************/
typedef enum
{
    CE_BL_DRV_SERDES_RESET    = 0 ,
    CE_BL_DRV_SERDES_UNRESET  = 1
}
BL_DRV_SERDES_RESET_DTE ;

/******************************************************************************/
/* PLL Locked / Not locked                                                    */
/******************************************************************************/
typedef enum
{
    CE_BL_DRV_SERDES_PLL_NOT_LOCKED = 0 ,
    CE_BL_DRV_SERDES_PLL_LOCKED     = 1
}
BL_DRV_SERDES_LOCKED_DTE ;

/******************************************************************************/
/* Enable / Disable                                                           */
/******************************************************************************/
typedef enum
{
    CE_BL_DRV_SERDES_DISABLE = 0 ,
    CE_BL_DRV_SERDES_ENABLE = 1
}
BL_DRV_SERDES_ENABLE_DTE ;

/******************************************************************************/
/* EMACs group mode enum                                                      */
/******************************************************************************/
#define CE_BL_DRV_SERDES_EMAC_MODE_MIN ( 0 )

typedef enum
{
    CE_BL_DRV_SERDES_EMAC_MODE_SGMII = CE_BL_DRV_SERDES_EMAC_MODE_MIN ,
    CE_BL_DRV_SERDES_EMAC_MODE_HISGMII ,
    CE_BL_DRV_SERDES_EMAC_MODE_QSGMII ,
    CE_BL_DRV_SERDES_EMAC_MODE_SS_SMII ,

    CE_BL_DRV_SERDES_NUMBER_OF_EMAC_MODES
}
BL_DRV_SERDES_EMAC_MODE_DTE ;

#define ME_BL_DRV_SERDES_EMAC_MODE_DTE_IN_RANGE(v) ( (v) >= CE_BL_DRV_SERDES_EMAC_MODE_MIN && \
                                                     (v) <  CE_BL_DRV_SERDES_NUMBER_OF_EMAC_MODES )

/******************************************************************************/
/* EMAC id enum                                                               */
/******************************************************************************/
#define CE_BL_DRV_SERDES_EMAC_ID_MIN ( 0 )

typedef enum
{
    CE_BL_DRV_SERDES_EMAC_ID_3 = CE_BL_DRV_SERDES_EMAC_ID_MIN ,
    CE_BL_DRV_SERDES_EMAC_ID_0 ,
    CE_BL_DRV_SERDES_EMAC_ID_1 ,
    CE_BL_DRV_SERDES_EMAC_ID_2 ,
    CE_BL_DRV_SERDES_EMAC_ID_4 ,
    CE_BL_DRV_SERDES_EMAC_ID_5 ,

    CE_BL_DRV_SERDES_NUMBER_OF_EMAC_IDS
}
BL_DRV_SERDES_EMAC_ID_DTE ;

#define ME_BL_DRV_SERDES_EMAC_ID_DTE_IN_RANGE(v) ( (v) >= CE_BL_DRV_SERDES_EMAC_ID_MIN && \
                                                   (v) <  CE_BL_DRV_SERDES_NUMBER_OF_EMAC_IDS )

/******************************************************************************/
/* WAN mode enum                                                              */
/******************************************************************************/
#define CE_BL_DRV_SERDES_WAN_MODE_MIN ( 0 )

typedef enum
{
    CE_BL_DRV_SERDES_WAN_MODE_GPON = CE_BL_DRV_SERDES_WAN_MODE_MIN ,
    CE_BL_DRV_SERDES_WAN_MODE_GBE ,

    CE_BL_DRV_SERDES_NUMBER_OF_WAN_MODES
}
BL_DRV_SERDES_WAN_MODE_DTE ;

#define ME_BL_DRV_SERDES_WAN_MODE_DTE_IN_RANGE(v) ( (v) >= CE_BL_DRV_SERDES_WAN_MODE_MIN && \
                                                    (v) <  CE_BL_DRV_SERDES_NUMBER_OF_WAN_MODES )


/******************************************************************************/
/* PRBS mode enum                                                             */
/******************************************************************************/
typedef enum
{
    CE_BL_DRV_SERDES_PRBS_MODE_MIN = 0 ,

    CE_BL_DRV_SERDES_PRBS_MODE_PRBS7 = CE_BL_DRV_SERDES_PRBS_MODE_MIN ,
    CE_BL_DRV_SERDES_PRBS_MODE_PRBS23 ,
    CE_BL_DRV_SERDES_PRBS_MODE_PRBS31 ,
    CE_BL_DRV_SERDES_PRBS_MODE_ALTERNATE ,

    CE_BL_DRV_SERDES_NUMBER_OF_PRBS_MODES
}
BL_DRV_SERDES_PRBS_MODE_DTE ;

/******************************************************************************/
/* PRBS status enum                                                           */
/******************************************************************************/
typedef enum
{
    CE_BL_DRV_SERDES_PRBS_STATUS_MIN = 0 ,

    CE_BL_DRV_SERDES_PRBS_STATUS_OK = CE_BL_DRV_SERDES_PRBS_STATUS_MIN ,
    CE_BL_DRV_SERDES_PRBS_STATUS_ERROR
}
BL_DRV_SERDES_PRBS_STATUS_DTE ;


/******************************************************************************/
/* General configuration structure                                            */
/******************************************************************************/
typedef struct
{
    /* wan mode, either GPON or GBE */
    BL_DRV_SERDES_WAN_MODE_DTE wan_mode ;

    /* physical ethernet interface for WAN size in case of GBE mode, either EMAC_4 or EMAC_5 */
    BL_DRV_SERDES_EMAC_ID_DTE gbe_wan_emac_id ;

    /* EMACs group mode, either SGMII, HiSGMII, QSGMII or SS_SMII */
    BL_DRV_SERDES_EMAC_MODE_DTE emacs_group_mode ;

    /* EMAC5 mode. either SGMII or HiSGMII */
    BL_DRV_SERDES_EMAC_MODE_DTE emac5_mode ;
}
BL_DRV_SERDES_GENERAL_CONFIG_DTE ;

/******************************************************************************/
 /* This type defines the serdes physical ports                      */
 /******************************************************************************/
 typedef enum
 {
     CE_BL_GPON,
     CE_BL_PCI_0,
     CE_BL_PCI_1,
     CE_BL_SGMII_0,
     CE_BL_SGMII_1,
     CE_BL_SGMII_2,
     CE_BL_SGMII_3,
     CE_BL_QSGMII,
     CE_BL_HISGMII
 }BL_DRV_SERDES_PHYSICAL_PORT;

 /******************************************************************************/
 /* This type defines the serdes amplitude value                      */
 /******************************************************************************/
 typedef enum
 {
     CE_BL_MAX_AMPLITUDE,
     CE_BL_AMPLITUDE_1,
     CE_BL_AMPLITUDE_2,
     CE_BL_AMPLITUDE_3,
     CE_BL_AMPLITUDE_4,
     CE_BL_AMPLITUDE_5,
     CE_BL_AMPLITUDE_6,
     CE_BL_AMPLITUDE_7,
     CE_BL_AMPLITUDE_8,
     CE_BL_AMPLITUDE_9,
     CE_BL_AMPLITUDE_10,
     CE_BL_AMPLITUDE_11,
     CE_BL_AMPLITUDE_12,
     CE_BL_AMPLITUDE_13,
     CE_BL_AMPLITUDE_14,
     CE_BL_MIN_AMPLITUDE,
     CE_BL_AMPLITUDE_DEFAULT_VAL,
 }BL_DRV_SERDES_AMPLITUDE_VAL;

 /******************************************************************************/
 /* This type defines the serdes de_emphasis value                      */
 /******************************************************************************/
 typedef enum
 {
     CE_BL_NO_DE_EMPHASIS,
     CE_BL_3_5_DB,
     CE_BL_6_DB,
     CE_BL_8_DB,
     CE_BL_DE_EMPHASIS_DEFAULT_VAL
 }BL_DRV_SERDES_DE_EMPHASIS_VAL;

 /******************************************************************************/
 /* This type defines the serdes delay control value                      */
 /******************************************************************************/
 typedef enum
 {
     CE_BL_1_CLK_CYCLE,
     CE_BL_2_CLK_CYCLE,
     CE_BL_DELAY_DEFAULT_GPON_VAL = 3,
     CE_BL_DELAY_DEFAULT_VAL = CE_BL_1_CLK_CYCLE
 }BL_DRV_SERDES_DELAY_CTRL_VAL;

 /************************************************************************************/
 /* These type defines the serdes configuration                                */
 /************************************************************************************/
 typedef struct
 {
     BL_DRV_SERDES_AMPLITUDE_VAL ampt_val;
     BL_DRV_SERDES_DE_EMPHASIS_VAL de_emph_val;
     BL_DRV_SERDES_DELAY_CTRL_VAL delay_ctrl;
 }
 BL_DRV_SERDES_CONFIGURATION;

/******************************************************************************/
/*                                                                            */
/* Functions prototypes                                                       */
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
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_set_general_configuration ( const BL_DRV_SERDES_GENERAL_CONFIG_DTE * xi_serdes_general_config ) ;

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
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_get_general_configuration ( BL_DRV_SERDES_GENERAL_CONFIG_DTE * const xo_serdes_general_config ) ;

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
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_rx_path_reset_unreset ( void ) ;

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
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_tx_path_reset_unreset ( void ) ;

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
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_configure_prbs_mode ( BL_DRV_SERDES_PRBS_MODE_DTE xi_prbs_mode ) ;

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
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_control_us_prbs_generator ( BL_DRV_SERDES_ENABLE_DTE xi_action ) ;

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
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_control_ds_prbs_comparator ( BL_DRV_SERDES_ENABLE_DTE xi_action ) ;

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
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_get_ds_prbs_comparator_status ( BL_DRV_SERDES_PRBS_STATUS_DTE * const xo_prbs_status ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_gpon_init                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   GPON/SERDES Driver - Init GPON SERDES                           */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function resets/unresets the GPON SERDES block (active low reset)   */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*     CE_BL_DRV_SERDES_NO_ERROR - No error                                   */
/*     CE_BL_DRV_SERDES_INVALID_RESET_VALUE - Invalid reset value             */
/*     CE_BL_DRV_SERDES_WAN_MODE_IS_NOT_GPON - WAN mode is not GPON           */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gpon_init (int ref_xtal) ;

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
void pi_bl_drv_serdes_netif_configure_and_enable ( BL_DRV_SERDES_EMAC_ID_DTE xi_emac_id ) ;

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   fi_bl_drv_serdes_netif_init                                             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   NETIF/SERDES Driver - Init NETIF SERDES                         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function resets the NETIF SERDES block (active low reset)  */
/*   according to the chosen EMACS group mode.                                */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   BL_DRV_SERDES_ERROR_DTE - Return code                                    */
/*     CE_BL_DRV_SERDES_NO_ERROR - No error                                   */
/*     CE_BL_DRV_SERDES_INVALID_RESET_VALUE - Invalid reset value             */
/*                                                                            */
/******************************************************************************/
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_netif_init(void);

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
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gbe_configure_and_enable ( void ) ;

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
BL_DRV_SERDES_ERROR_DTE fi_bl_drv_serdes_gbe_reset ( BL_DRV_SERDES_RESET_DTE xi_reset ) ;

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
        BL_DRV_SERDES_CONFIGURATION * const xi_serdes_conf );

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
        BL_DRV_SERDES_CONFIGURATION * const xo_serdes_conf );

#ifdef __cplusplus
}
#endif

#endif /* BL_DRV_SERDES_H_INCLUDED */

