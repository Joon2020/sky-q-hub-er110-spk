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
/* CR driver                     											*/
/*                                                                         	*/
/****************************************************************************/

#ifndef _BL_LILAC_RST_H_
#define _BL_LILAC_RST_H_

#ifdef __MIPS_C  /* only MIPS_C */

#include "bl_lilac_soc.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BL_PLL_IN_BYPASS 100


typedef enum
{
    BL_LILAC_CR_OFF	= 0, 
    BL_LILAC_CR_ON	= 1, 
    BL_LILAC_CR_STATE_LAST 
	
} BL_LILAC_CR_STATE;


/* list of the devices connected to the reset registers */
typedef enum
{
	/* SW_RST_L_0 or ENABLE_H */
	BL_LILAC_SW_RST_L_0_LABEL				= 0,
	BL_LILAC_ENABLE_H_LABEL					= BL_LILAC_SW_RST_L_0_LABEL,
	BL_LILAC_CR_NAND_FLASH 					= BL_LILAC_SW_RST_L_0_LABEL,
	BL_LILAC_CR_SPI_FLASH,
	BL_LILAC_CR_OTP,
	
	/* SW_RST_L_1 or ENABLE_L*/
	BL_LILAC_SW_RST_L_1_LABEL,
	BL_LILAC_ENABLE_L_LABEL					= BL_LILAC_SW_RST_L_1_LABEL,

	/* SW_RST_H_0 or ENABLE_L */
	BL_LILAC_SW_RST_H_0_LABEL,
	BL_LILAC_CR_MIPS_D						= BL_LILAC_SW_RST_H_0_LABEL,
	BL_LILAC_CR_OCP_D,
	BL_LILAC_CR_OCP_C_DDR,
	BL_LILAC_CR_OCP_D_DDR,
	BL_LILAC_CR_NIF_0,
	BL_LILAC_CR_NIF_1,
	BL_LILAC_CR_NIF_2,
	BL_LILAC_CR_NIF_3,
	BL_LILAC_CR_NIF_4,
	BL_LILAC_CR_NIF_5,
	BL_LILAC_CR_GPON_SERDES_VPB,
	BL_LILAC_CR_GPON_SERDES					= BL_LILAC_CR_GPON_SERDES_VPB,
	BL_LILAC_CR_PCI_SERDES_VPB,
	BL_LILAC_CR_PCI_SERDES					= BL_LILAC_CR_PCI_SERDES_VPB,
	BL_LILAC_CR_NIF_SERDES_VPB,
	BL_LILAC_CR_NIF_SERDES					= BL_LILAC_CR_NIF_SERDES_VPB,
	BL_LILAC_CR_PKT_SRAM_OCP_C,
	BL_LILAC_CR_INT_C,
	BL_LILAC_CR_GPIO_C,
	BL_LILAC_CR_TIMERS_C,
	BL_LILAC_CR_UART_C,
	BL_LILAC_CR_SPI_C,
	BL_LILAC_CR_I2C_C,
	BL_LILAC_CR_PERIPHERAL_C,
	BL_LILAC_CR_BOOT_D,
	BL_LILAC_CR_INT_COLLECTOR_D,
	BL_LILAC_CR_PKT_SRAM_OCP_D,
	BL_LILAC_CR_INT_D,
	BL_LILAC_CR_GPIO_D,
	BL_LILAC_CR_TIMERS_D,
	BL_LILAC_CR_UART_D,
	BL_LILAC_CR_PERIPHERAL_D,

	/* SW_RST_H_1 or ENABLE_L */
	BL_LILAC_SW_RST_H_1_LABEL,
	BL_LILAC_CR_DDR_PHY						= BL_LILAC_SW_RST_H_1_LABEL,
	BL_LILAC_CR_DDR,
	BL_LILAC_CR_DDR_BRIDGE,
	BL_LILAC_CR_GPON_RX,
	BL_LILAC_CR_GPON_8K_CLK,
	BL_LILAC_CR_RNG,
	BL_LILAC_CR_CDR_DIAG_DATA,
	BL_LILAC_CR_CDR_DIAG					= BL_LILAC_CR_CDR_DIAG_DATA,
	BL_LILAC_CR_IPCLOCK,
	BL_LILAC_CR_GPON_MAIN,
	BL_LILAC_CR_GPON						= BL_LILAC_CR_GPON_MAIN,
	BL_LILAC_CR_CDR_DIAG_FAST,
	
	/* SW_RST_H_2 or ENABLE_L */
	BL_LILAC_SW_RST_H_2_LABEL,
	BL_LILAC_CR_RNR_0						= BL_LILAC_SW_RST_H_2_LABEL,
	BL_LILAC_CR_RNR_0_DDR,
	BL_LILAC_CR_RNR_1,
	BL_LILAC_CR_RNR_1_DDR,
	BL_LILAC_CR_RNR_SUB,
	BL_LILAC_CR_IPSEC_RNR,
	BL_LILAC_CR_TM_IPSEC					= BL_LILAC_CR_IPSEC_RNR,
	BL_LILAC_CR_IH_RNR,
	BL_LILAC_CR_BB_RNR,
	BL_LILAC_CR_BB_MAIN,
	BL_LILAC_CR_GENERAL_MAIN,
	BL_LILAC_CR_GENERAL_MAIN_BB_RNR_BB_MAIN,
	BL_LILAC_CR_MIPS_D_RNR,
	BL_LILAC_CR_TM							= BL_LILAC_CR_MIPS_D_RNR,
	BL_LILAC_CR_DMA_DDR,
	BL_LILAC_CR_DSP_CORE,
	BL_LILAC_CR_DSP							= BL_LILAC_CR_DSP_CORE,
	BL_LILAC_CR_VOIP_INTERFACE,
	BL_LILAC_CR_DSP_DDR,
	BL_LILAC_CR_TDM_RX,
	BL_LILAC_CR_TDM_TX,
	BL_LILAC_CR_PCI_0_CORE,
	BL_LILAC_CR_PCI_0						= BL_LILAC_CR_PCI_0_CORE,
	BL_LILAC_CR_PCI_0_PCS,
	BL_LILAC_CR_PCI_0_AHB,
	BL_LILAC_CR_PCI_0_EC,
	BL_LILAC_CR_PCI_0_DDR,
	BL_LILAC_CR_PCI_1_CORE,
	BL_LILAC_CR_PCI_1						= BL_LILAC_CR_PCI_1_CORE,
	BL_LILAC_CR_PCI_1_PCS,
	BL_LILAC_CR_PCI_1_AHB,
	BL_LILAC_CR_PCI_1_EC,
	BL_LILAC_CR_PCI_1_DDR,
	BL_LILAC_CR_IPSEC_MAIN,

	/* SW_RST_H_3 or ENABLE_L */
	BL_LILAC_SW_RST_H_3_LABEL,
	BL_LILAC_CR_USB_POWER_ON				= BL_LILAC_SW_RST_H_3_LABEL,
	BL_LILAC_CR_USB_CORE					= BL_LILAC_CR_USB_POWER_ON,
	BL_LILAC_CR_USB_0_PHY,
	BL_LILAC_CR_USB_0_OHCI_PLL,
	BL_LILAC_CR_USB_0_PORT,
	BL_LILAC_CR_USB_1_PHY,
	BL_LILAC_CR_USB_1_OHCI_PLL,
	BL_LILAC_CR_USB_1_PORT,
	BL_LILAC_CR_USB_COMMON_HOST,
	BL_LILAC_CR_USB_COMMON					= BL_LILAC_CR_USB_COMMON_HOST,
	BL_LILAC_CR_USB_BRIDGE_COMMON_OCP,
	BL_LILAC_CR_USB_0_HOST,
	BL_LILAC_CR_USB_0						= BL_LILAC_CR_USB_0_HOST,
	BL_LILAC_CR_USB_0_AUX_WELL,
	BL_LILAC_CR_USB_0_BRIDGE_HOST,
	BL_LILAC_CR_USB_0_BRIDGE_OCP,
	BL_LILAC_CR_USB_0_DDR,
	BL_LILAC_CR_USB_1_HOST,
	BL_LILAC_CR_USB_1						= BL_LILAC_CR_USB_1_HOST,
	BL_LILAC_CR_USB_1_AUX_WELL,
	BL_LILAC_CR_USB_1_BRIDGE_HOST,
	BL_LILAC_CR_USB_1_BRIDGE_OCP,
	BL_LILAC_CR_USB_1_DDR,
	BL_LILAC_CR_DLL_SEL_RX,
	BL_LILAC_CR_DLL_SEL_TX,
	
	/* ENABLE_L */
	BL_LILAC_CR_CLUSTER_1,
	BL_LILAC_CR_SYNCE
	
} BL_LILAC_CR_DEVICE;


/***************************************************************************/
/* 		reset/release from reset specified device                          */ 
/* Input                                                                   */
/*     device to reset/release                                             */
/*     device state - BL_LILAC_CR_OFF to reset, BL_LILAC_CR_ON to release  */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_cr_device_reset(BL_LILAC_CR_DEVICE device, BL_LILAC_CR_STATE state);

/***************************************************************************/
/*     enable/disable specified device                                     */ 
/* Input                                                                   */
/*     device to enable/disable                                            */
/*     device state - BL_LILAC_CR_OFF to disable, BL_LILAC_CR_ON to enable */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_cr_device_enable_clk(BL_LILAC_CR_DEVICE device, BL_LILAC_CR_STATE state);



/***************************************************************************/
/*     returns the MIPS-C actual output clock and peripheral clock         */ 
/*     takes in account the reference clock and PLL's parameters           */
/* Output                                                                  */
/*     CPU frequency in Hz, after all dividers                             */
/*     Peripherals clock in Hz, after all dividers                         */
/*     success or failure                                                  */
/***************************************************************************/
BL_LILAC_SOC_STATUS get_lilac_clock_rate(uint32_t *xo_freq, uint32_t *xo_periph);
uint32_t get_lilac_system_clock_rate(void);
uint32_t get_lilac_ddr_clock_rate(void);
uint32_t get_lilac_peripheral_clock_rate(void);
uint32_t get_lilac_rnr_clock_rate(void);


/***************************************************************************/
/*     Set the MIPS-C divider register                                     */
/* Input                                                                   */
/*     pll number : 0 - 2, post divider, peripheral divider, OCP divider   */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_mipsc_divider(int32_t xi_pll, uint32_t xi_ocp, int32_t xi_post_divider, int32_t xi_periph);

/***************************************************************************/
/*     Set the MIPS-D divider register                                     */
/* Input                                                                   */
/*     pll number : 0 - 2, post divider, peripheral divider, OCP divider   */
/***************************************************************************/

BL_LILAC_SOC_STATUS bl_lilac_set_mipsd_divider(int32_t xi_pll, uint32_t xi_ocp, int32_t xi_post_divider, int32_t xi_periph);
/***************************************************************************/
/*     Set divider register                                                */
/* Input                                                                   */
/*     pll number : 0 - 2, post divider                                    */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_nand_flash_divider(int32_t xi_pll, int32_t xi_post_divider);
BL_LILAC_SOC_STATUS bl_lilac_set_spi_flash_divider (int32_t xi_pll, int32_t xi_post_divider);
BL_LILAC_SOC_STATUS bl_lilac_set_dsp_divider       (int32_t xi_pll, int32_t xi_post_divider);
BL_LILAC_SOC_STATUS bl_lilac_set_otp_divider       (int32_t xi_pll, int32_t xi_post_divider);
BL_LILAC_SOC_STATUS bl_lilac_set_ipclock_divider   (int32_t xi_pll, int32_t xi_post_divider);
BL_LILAC_SOC_STATUS bl_lilac_set_pci_divider       (int32_t xi_pll, int32_t xi_post_divider);
BL_LILAC_SOC_STATUS bl_lilac_set_cdr_divider       (int32_t xi_pll, int32_t xi_post_divider);
BL_LILAC_SOC_STATUS bl_lilac_set_usb_host_divider  (int32_t xi_pll, int32_t xi_post_divider);

/***************************************************************************/
/*     Set TM divider register                                             */
/* Input                                                                   */
/*     pll number : 0 - 2, TM post divider, Runner post divider            */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_tm_divider(int32_t xi_pll, int32_t xi_tm_divider, int32_t xi_rnr_divider);

/***************************************************************************/
/*     Set DDR divider register                                            */
/* Input                                                                   */
/*     pll number : 0 - 2, DDR post divider, DDR bridge post divider       */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_ddr_divider(int32_t xi_pll, int32_t xi_post_divider, int32_t xi_bridge_divider);

/***************************************************************************/
/*     Set TDM divider register                                            */
/* Input                                                                   */
/*     pll number : 0 - 2, pre divider, post divider                       */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_tdmrx_divider(int32_t xi_pll, int32_t xi_pre_divider, int32_t xi_post_divider);
BL_LILAC_SOC_STATUS bl_lilac_set_tdmtx_divider(int32_t xi_pll, int32_t xi_pre_divider, int32_t xi_post_divider);

/***************************************************************************/
/*     Set USB core divider register                                       */
/* Input                                                                   */
/*     pll number : 0 - 2                                                  */
/*                  pre divider = -1, no change is done                    */
/*                  post divider = -1, no change is done                   */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_usb_core_divider(int32_t xi_pll, int32_t xi_pre_divider, int32_t xi_post_divider);

/***************************************************************************/
/*     Set TDM SYNC divider register                                       */
/* Input                                                                   */
/*     pll number : 0 - 2, pre divider, post divider,                      */
/*                  generate 25 Mhz, if equal to -1, no change is done     */
/*           sync on 25, sync on 10, if equal to -1, no change is done     */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_synce_tdm_divider(int32_t xi_pll, int32_t xi_pre_divider, int32_t xi_post_divider,
                                                   int32_t xi_gen, int32_t xi_25, int32_t xi_10);

/***************************************************************************/
/*     Set the required PLL register                                       */
/* Input                                                                   */
/*     pll number : 0 - 2, reference clock : 0 - 4                         */
/*     PLL parameters : k - pre divider, n - feadback divider, m - output  */
/*                      divider                                            */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_pll(int32_t xi_pll, int32_t xi_ref, int32_t k, int32_t m, int32_t n);

/***************************************************************************/
/*     Get the required PLL parameters                                     */
/* Input                                                                   */
/*     pll number : 0 - 2                                                  */
/* Output                                                                  */
/*     PLL parameters : k - pre divider, n - feadback divider, m - output  */
/*                      divider, ref - reference clock                     */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_get_pll_parameters(int32_t xi_pll, int32_t *xo_ref, int32_t *xo_k, int32_t *xo_m, int32_t *xo_n);

/***************************************************************************/
/*     Get the required PLL output clock                                   */
/* Input                                                                   */
/*     pll number : 0 - 2                                                  */
/* Output                                                                  */
/*     PLL clock frequency                                                 */
/***************************************************************************/
int32_t bl_lilac_get_pll_output(int32_t xi_pll);

BL_LILAC_SOC_STATUS bl_lilac_cr_get_emac_reset_state( BL_LILAC_CR_DEVICE xi_emac_cr_device_id , BL_LILAC_CR_STATE * xo_reset_state);


void bl_lilac_enable_25M_clock(void);


#ifdef __cplusplus
}
#endif

#endif /* #ifdef __MIPS_C */

#endif /* #ifndef _BL_LILAC_RST_H_ */

