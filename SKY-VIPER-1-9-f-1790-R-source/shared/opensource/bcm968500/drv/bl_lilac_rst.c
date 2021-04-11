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

#ifdef __MIPS_C  /* only MIPS_C */

#include "bl_os_wraper.h"
#include "bl_lilac_soc.h"
#include "bl_lilac_brd.h"
#include "bl_lilac_rst.h"
#include "bl_lilac_cr.h"



BL_LILAC_SOC_STATUS bl_lilac_cr_device_reset(BL_LILAC_CR_DEVICE device , BL_LILAC_CR_STATE state)
{
	unsigned long reg_offset = 0;
	unsigned long reg_value = 0;
	BL_LILAC_SOC_STATUS status = BL_LILAC_SOC_OK;
	unsigned long flags = 0;
	
	BL_LOCK_SECTION(flags);

	if(device < BL_LILAC_SW_RST_L_1_LABEL)
	{
		reg_offset = CE_CR_CONTROL_REGS_SW_RST_L_0_OFFSET;
	}
	else if(device < BL_LILAC_SW_RST_H_0_LABEL)
	{
		reg_offset = CE_CR_CONTROL_REGS_SW_RST_L_1_OFFSET;
	}
	else if(device < BL_LILAC_SW_RST_H_1_LABEL)
	{
		reg_offset = CE_CR_CONTROL_REGS_SW_RST_H_0_OFFSET;
	}
	else if(device < BL_LILAC_SW_RST_H_2_LABEL)
	{
		reg_offset = CE_CR_CONTROL_REGS_SW_RST_H_1_OFFSET;
	}
	else if(device < BL_LILAC_SW_RST_H_3_LABEL)
	{
		reg_offset = CE_CR_CONTROL_REGS_SW_RST_H_2_OFFSET;
	}
	else
	{
		reg_offset = CE_CR_CONTROL_REGS_SW_RST_H_3_OFFSET;
	}
	
	BL_READ_32(CE_CR_CONTROL_REGS_ADDRESS + reg_offset, reg_value);
	
	switch(device)
	{
		/* SW_RST_L_0 */
		case BL_LILAC_CR_NAND_FLASH:
			((CR_CONTROL_REGS_SW_RST_L_0_DTE *)&reg_value)->nand_flash = !state;
			break;
		case BL_LILAC_CR_SPI_FLASH:
			((CR_CONTROL_REGS_SW_RST_L_0_DTE *)&reg_value)->spi_flash = !state;
			break;
		case BL_LILAC_CR_OTP:
			((CR_CONTROL_REGS_SW_RST_L_0_DTE *)&reg_value)->otp = !state;
			break;
		
		/* SW_RST_L_1 */

		/* SW_RST_H_0 */
		case BL_LILAC_CR_MIPS_D:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->mips_d = !state;
			break;
		case BL_LILAC_CR_OCP_D:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->ocp_d = !state;
			break;
		case BL_LILAC_CR_OCP_C_DDR:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->ocp_c_ddr = !state;
			break;
		case BL_LILAC_CR_OCP_D_DDR:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->ocp_d_ddr = !state;
			break;
		case BL_LILAC_CR_NIF_0:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->nif_0 = !state;
			break;
		case BL_LILAC_CR_NIF_1:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->nif_1 = !state;
			break;
		case BL_LILAC_CR_NIF_2:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->nif_2 = !state;
			break;
		case BL_LILAC_CR_NIF_3:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->nif_3 = !state;
			break;
		case BL_LILAC_CR_NIF_4:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->nif_4 = !state;
			break;
		case BL_LILAC_CR_NIF_5:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->nif_5 = !state;
			break;
		case BL_LILAC_CR_GPON_SERDES_VPB:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->gpon_serdes_vpbt = !state;
			break;
		case BL_LILAC_CR_PCI_SERDES_VPB:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->pci_serdes_vpb = !state;
			break;
		case BL_LILAC_CR_NIF_SERDES_VPB:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->nif_serdes_vpb = !state;
			break;
		case BL_LILAC_CR_PKT_SRAM_OCP_C:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->pkt_sram_ocp_c = !state;
			break;
		case BL_LILAC_CR_INT_C:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->int_c = !state;
			break;
		case BL_LILAC_CR_GPIO_C:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->gpio_c = !state;
			break;
		case BL_LILAC_CR_TIMERS_C:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->timers_c = !state;
			break;
		case BL_LILAC_CR_UART_C:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->uart_c = !state;
			break;
		case BL_LILAC_CR_SPI_C:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->spi_c = !state;
			break;
		case BL_LILAC_CR_I2C_C:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->i2c_c = !state;
			break;
		case BL_LILAC_CR_PERIPHERAL_C:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->peripheral_c = !state;
			break;
		case BL_LILAC_CR_BOOT_D:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->boot_d = !state;
			break;
		case BL_LILAC_CR_INT_COLLECTOR_D:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->int_collector_d = !state;
			break;
		case BL_LILAC_CR_PKT_SRAM_OCP_D:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->pkt_sram_ocp_dt = !state;
			break;
		case BL_LILAC_CR_INT_D:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->int_d = !state;
			break;
		case BL_LILAC_CR_GPIO_D:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->gpio_d = !state;
			break;
		case BL_LILAC_CR_TIMERS_D:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->timers_d = !state;
			break;
		case BL_LILAC_CR_UART_D:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->uart_d = !state;
			break;
		case BL_LILAC_CR_PERIPHERAL_D:
			((CR_CONTROL_REGS_SW_RST_H_0_DTE *)&reg_value)->peripheral_d = !state;
			break;

		/* SW_RST_H_1 */
		case BL_LILAC_CR_DDR_PHY:
			((CR_CONTROL_REGS_SW_RST_H_1_DTE *)&reg_value)->ddr_phy = !state;
			break;
		case BL_LILAC_CR_DDR:
			((CR_CONTROL_REGS_SW_RST_H_1_DTE *)&reg_value)->ddr = !state;
			break;
		case BL_LILAC_CR_DDR_BRIDGE:
			((CR_CONTROL_REGS_SW_RST_H_1_DTE *)&reg_value)->ddr_bridge = !state;
			break;
		case BL_LILAC_CR_GPON_RX:
			((CR_CONTROL_REGS_SW_RST_H_1_DTE *)&reg_value)->gpon_rx = !state;
			break;
		case BL_LILAC_CR_GPON_8K_CLK:
			((CR_CONTROL_REGS_SW_RST_H_1_DTE *)&reg_value)->gpon_8k_clk = !state;
			break;
		case BL_LILAC_CR_RNG:
			((CR_CONTROL_REGS_SW_RST_H_1_DTE *)&reg_value)->rng = !state;
			break;
		case BL_LILAC_CR_CDR_DIAG_DATA:
			((CR_CONTROL_REGS_SW_RST_H_1_DTE *)&reg_value)->cdr_diag_data = !state;
			break;
		case BL_LILAC_CR_IPCLOCK:
			((CR_CONTROL_REGS_SW_RST_H_1_DTE *)&reg_value)->ipclock = !state;
			break;
		case BL_LILAC_CR_GPON_MAIN:
			((CR_CONTROL_REGS_SW_RST_H_1_DTE *)&reg_value)->gpon_main = !state;
			break;
		case BL_LILAC_CR_CDR_DIAG_FAST:
			((CR_CONTROL_REGS_SW_RST_H_1_DTE *)&reg_value)->cdr_diag_fast = !state;
			break;
		
		/* SW_RST_H_2 */
		case BL_LILAC_CR_RNR_0:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->rnr_0 = !state;
			break;
		case BL_LILAC_CR_RNR_0_DDR:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->rnr_0_ddr = !state;
			break;
		case BL_LILAC_CR_RNR_1:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->rnr_1 = !state;
			break;
		case BL_LILAC_CR_RNR_1_DDR:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->rnr_1_ddr = !state;
			break;
		case BL_LILAC_CR_RNR_SUB:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->rnr_sub = !state;
			break;
		case BL_LILAC_CR_IPSEC_RNR:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->ipsec_rnr = !state;
			break;
		case BL_LILAC_CR_IH_RNR:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->ih_rnr = !state;
			break;
		case BL_LILAC_CR_BB_RNR:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->bb_rnr = !state;
			break;
		case BL_LILAC_CR_BB_MAIN:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->bb_main = !state;
			break;
		case BL_LILAC_CR_GENERAL_MAIN:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->general_main = !state;
			break;
			
		case BL_LILAC_CR_GENERAL_MAIN_BB_RNR_BB_MAIN:
        ((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->general_main = !state;
        ((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->bb_rnr = !state;
        ((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->bb_main = !state;
			break;
		case BL_LILAC_CR_MIPS_D_RNR:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->mips_d_rnr = !state;
			break;
		case BL_LILAC_CR_DMA_DDR:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->dma_ddr = !state;
			break;
		case BL_LILAC_CR_DSP_CORE:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->dsp_core = !state;
			break;
		case BL_LILAC_CR_VOIP_INTERFACE:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->voip_interface = !state;
			break;
		case BL_LILAC_CR_DSP_DDR:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->dsp_ddr = !state;
			break;
		case BL_LILAC_CR_TDM_RX:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->tdm_rx = !state;
			break;
		case BL_LILAC_CR_TDM_TX:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->tdm_tx = !state;
			break;
		case BL_LILAC_CR_PCI_0_CORE:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->pci_0_core = !state;
			break;
		case BL_LILAC_CR_PCI_0_PCS:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->pci_0_pcs = !state;
			break;
		case BL_LILAC_CR_PCI_0_AHB:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->pci_0_ahb = !state;
			break;
		case BL_LILAC_CR_PCI_0_EC:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->pci_0_ec = !state;
			break;
		case BL_LILAC_CR_PCI_0_DDR:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->pci_ddr_0 = !state;
			break;
		case BL_LILAC_CR_PCI_1_CORE:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->pci_1_core = !state;
			break;
		case BL_LILAC_CR_PCI_1_PCS:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->pci_1_pcs = !state;
			break;
		case BL_LILAC_CR_PCI_1_AHB:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->pci_1_ahb = !state;
			break;
		case BL_LILAC_CR_PCI_1_EC:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->pci_1_ec = !state;
			break;
		case BL_LILAC_CR_PCI_1_DDR:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->pci_ddr_1 = !state;
			break;
		case BL_LILAC_CR_IPSEC_MAIN:
			((CR_CONTROL_REGS_SW_RST_H_2_DTE *)&reg_value)->ipsec_main = !state;
			break;

		/* SW_RST_H_3 */
		case BL_LILAC_CR_USB_POWER_ON:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_power_on = !state;
			break;
		case BL_LILAC_CR_USB_0_PHY:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_0_phy = !state;
			break;
		case BL_LILAC_CR_USB_0_OHCI_PLL:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_0_ohci_pll = !state;
			break;
		case BL_LILAC_CR_USB_0_PORT:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_0_port = !state;
			break;
		case BL_LILAC_CR_USB_1_PHY:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_1_phy = !state;
			break;
		case BL_LILAC_CR_USB_1_OHCI_PLL:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_1_ohci_pll = !state;
			break;
		case BL_LILAC_CR_USB_1_PORT:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_1_port = !state;
			break;
		case BL_LILAC_CR_USB_COMMON_HOST:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_common_host = !state;
			break;
		case BL_LILAC_CR_USB_BRIDGE_COMMON_OCP:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_bridge_common_ocp = !state;
			break;
		case BL_LILAC_CR_USB_0_HOST:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_0_host = !state;
			break;
		case BL_LILAC_CR_USB_0_AUX_WELL:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_0_aux_well = !state;
			break;
		case BL_LILAC_CR_USB_0_BRIDGE_HOST:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_0_bridge_host = !state;
			break;
		case BL_LILAC_CR_USB_0_BRIDGE_OCP:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_0_bridge_ocp = !state;
			break;
		case BL_LILAC_CR_USB_0_DDR:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_ddr_0 = !state;
			break;
		case BL_LILAC_CR_USB_1_HOST:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_1_host = !state;
			break;
		case BL_LILAC_CR_USB_1_AUX_WELL:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_1_aux_well = !state;
			break;
		case BL_LILAC_CR_USB_1_BRIDGE_HOST:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_1_bridge_host = !state;
			break;
		case BL_LILAC_CR_USB_1_BRIDGE_OCP:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_1_bridge_ocp = !state;
			break;
		case BL_LILAC_CR_USB_1_DDR:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->usb_ddr_1 = !state;
			break;
		case BL_LILAC_CR_DLL_SEL_RX:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->cr_dll_sel_rx = !state;
			break;
		case BL_LILAC_CR_DLL_SEL_TX:
			((CR_CONTROL_REGS_SW_RST_H_3_DTE *)&reg_value)->cr_dll_sel_tx = !state;
			break;
			
		default:
			status = BL_LILAC_SOC_INVALID_PARAM;
	}
	
	if(status == BL_LILAC_SOC_OK)
		BL_WRITE_32(CE_CR_CONTROL_REGS_ADDRESS + reg_offset, reg_value);

	BL_UNLOCK_SECTION(flags);	
	
	return status;
}
EXPORT_SYMBOL(bl_lilac_cr_device_reset);

BL_LILAC_SOC_STATUS bl_lilac_cr_device_enable_clk(BL_LILAC_CR_DEVICE device , BL_LILAC_CR_STATE state)
{
	unsigned long reg_offset = 0;
	unsigned long reg_value = 0;
	BL_LILAC_SOC_STATUS status = BL_LILAC_SOC_OK;
	unsigned long flags = 0;
	
	BL_LOCK_SECTION(flags);
	
	if(device < BL_LILAC_ENABLE_L_LABEL)
	{
		reg_offset = CE_CR_CONTROL_REGS_ENABLE_H_OFFSET;
	}
	else
	{
		reg_offset = CE_CR_CONTROL_REGS_ENABLE_L_OFFSET;
	}

	BL_READ_32(CE_CR_CONTROL_REGS_ADDRESS + reg_offset, reg_value);

	switch(device)
	{
		/* ENABLE_L */
		case BL_LILAC_CR_MIPS_D:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->mips_d = state;
			break;
		case BL_LILAC_CR_BOOT_D:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->boot_d = state;
			break;
		case BL_LILAC_CR_DDR:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->ddr = state;
			break;
		case BL_LILAC_CR_GPON:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->gpon = state;
			break;
		case BL_LILAC_CR_GPON_SERDES:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->gpon_serdes = state;
			break;
		case BL_LILAC_CR_NIF_0:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->nif_0 = state;
			break;
		case BL_LILAC_CR_NIF_1:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->nif_1 = state;
			break;
		case BL_LILAC_CR_NIF_2:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->nif_2 = state;
			break;
		case BL_LILAC_CR_NIF_3:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->nif_3 = state;
			break;
		case BL_LILAC_CR_NIF_4:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->nif_4 = state;
			break;
		case BL_LILAC_CR_NIF_5:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->nif_5 = state;
			break;
		case BL_LILAC_CR_NIF_SERDES:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->nif_serdes = state;
			break;
		case BL_LILAC_CR_PCI_0:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->pci_0 = state;
			break;
		case BL_LILAC_CR_PCI_1:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->pci_1 = state;
			break;
		case BL_LILAC_CR_PCI_SERDES:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->pci_serdes = state;
			break;
		case BL_LILAC_CR_USB_COMMON:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->usb_common = state;
			break;
		case BL_LILAC_CR_USB_0:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->usb_0 = state;
			break;
		case BL_LILAC_CR_USB_1:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->usb_1 = state;
			break;
		case BL_LILAC_CR_USB_CORE:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->usb_core = state;
			break;
		case BL_LILAC_CR_DSP:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->dsp = state;
			break;
		case BL_LILAC_CR_TM:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->tm = state;
			break;
		case BL_LILAC_CR_CLUSTER_1:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->cluster_1 = state;
			break;
		case BL_LILAC_CR_TDM_RX:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->tdmrx = state;
			break;
		case BL_LILAC_CR_TDM_TX:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->tdmtx = state;
			break;
		case BL_LILAC_CR_CDR_DIAG:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->cdr_diag = state;
			break;
		case BL_LILAC_CR_IPCLOCK:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->ipclock = state;
			break;
		case BL_LILAC_CR_SYNCE:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->synce = state;
			break;
		case BL_LILAC_CR_TM_IPSEC:
			((CR_CONTROL_REGS_ENABLE_L_DTE *)&reg_value)->tm_ipsec = state;
			break;

		/* ENABLE_H */
		case BL_LILAC_CR_NAND_FLASH:
			((CR_CONTROL_REGS_ENABLE_H_DTE *)&reg_value)->nand_flash = state;
			break;
		case BL_LILAC_CR_SPI_FLASH:
			((CR_CONTROL_REGS_ENABLE_H_DTE *)&reg_value)->spi_flash = state;
			break;
		case BL_LILAC_CR_OTP:
			((CR_CONTROL_REGS_ENABLE_H_DTE *)&reg_value)->otp = state;
			break;

		default:
			status = BL_LILAC_SOC_INVALID_PARAM;
	}
	
	if(status == BL_LILAC_SOC_OK)
		BL_WRITE_32(CE_CR_CONTROL_REGS_ADDRESS + reg_offset, reg_value);

	BL_UNLOCK_SECTION(flags);	
	
	return status;
}
EXPORT_SYMBOL(bl_lilac_cr_device_enable_clk);

/***************************************************************************/
/* Title                                                                   */
/*     Set the MIPS-C divider register                                     */
/* Input                                                                   */
/*     pll number : 0 - 2, post divider, peripheral divider, OCP divider   */
/* Output                                                                  */
/*     success or failure                                                  */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_mipsc_divider(int32_t xi_pll, uint32_t xi_ocp, int32_t xi_post_divider, int32_t xi_periph)
{
    volatile CR_CONTROL_REGS_MIPS_C_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_MIPS_C_DIVIDERS_READ(reg);
    reg.mips_c_pll_sel = xi_pll;
    reg.m_periph_c_en = 1;
    reg.m_periph_c = xi_periph;
    reg.m_mips_c = xi_post_divider;
    reg.m_ocp_c = xi_ocp; 
    BL_CR_CONTROL_REGS_MIPS_C_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}

EXPORT_SYMBOL(bl_lilac_set_mipsc_divider);

/***************************************************************************/
/* Title                                                                   */
/*     Set the MIPS-D divider register                                     */
/* Input                                                                   */
/*     pll number : 0 - 2, post divider, peripheral divider, OCP divider   */
/* Output                                                                  */
/*     success or failure                                                  */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_mipsd_divider(int32_t xi_pll, uint32_t xi_ocp, int32_t xi_post_divider, int32_t xi_periph)
{
    volatile CR_CONTROL_REGS_MIPS_D_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_MIPS_D_DIVIDERS_READ(reg);
	reg.m_periph_d_en = 1;
	reg.m_periph_d = xi_periph;
	reg.m_ocp_d_en = 1; 
	reg.m_ocp_d = xi_ocp;
	reg.m_mips_d_en = 1; 
	reg.m_mips_d = xi_post_divider; 
	reg.mips_d_pll_sel = xi_pll;
    BL_CR_CONTROL_REGS_MIPS_D_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}

EXPORT_SYMBOL(bl_lilac_set_mipsd_divider);

/***************************************************************************/
/* Title                                                                   */
/*     Set divider register                                                */
/* Input                                                                   */
/*     pll number : 0 - 2, post divider                                    */
/* Output                                                                  */
/*     success or failure                                                  */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_nand_flash_divider(int32_t xi_pll, int32_t xi_post_divider)
{
    volatile CR_CONTROL_REGS_NAND_FLASH_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_NAND_FLASH_DIVIDERS_READ(reg);
	reg.m_nand_flash_en = 1;
	reg.m_nand_flash = xi_post_divider;
	reg.nand_flash_pll_sel = xi_pll;
    BL_CR_CONTROL_REGS_NAND_FLASH_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_nand_flash_divider);

BL_LILAC_SOC_STATUS bl_lilac_set_spi_flash_divider(int32_t xi_pll, int32_t xi_post_divider)
{
    volatile CR_CONTROL_REGS_SPI_FLASH_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_SPI_FLASH_DIVIDERS_READ(reg);
	reg.m_spi_flash_en = 1;  
	reg.m_spi_flash = xi_post_divider;      
	reg.spi_flash_pll_sel = xi_pll;
    BL_CR_CONTROL_REGS_SPI_FLASH_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_spi_flash_divider);

BL_LILAC_SOC_STATUS bl_lilac_set_dsp_divider(int32_t xi_pll, int32_t xi_post_divider)
{
    volatile CR_CONTROL_REGS_DSP_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_DSP_DIVIDERS_READ(reg);
	reg.m_dsp_en = 1; 
	reg.m_dsp = xi_post_divider;
	reg.dsp_pll_sel = xi_pll;
    BL_CR_CONTROL_REGS_DSP_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_dsp_divider);

BL_LILAC_SOC_STATUS bl_lilac_set_ipclock_divider(int32_t xi_pll, int32_t xi_post_divider)
{
    volatile CR_CONTROL_REGS_IPCLOCK_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_IPCLOCK_DIVIDERS_READ(reg);   
	reg.m_ipclock_en = 1;
	reg.m_ipclock = xi_post_divider;
	reg.ipclock_pll_sel	= xi_pll;
    BL_CR_CONTROL_REGS_IPCLOCK_DIVIDERS_WRITE(reg);   

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_ipclock_divider);

BL_LILAC_SOC_STATUS bl_lilac_set_otp_divider(int32_t xi_pll, int32_t xi_post_divider)
{
    volatile CR_CONTROL_REGS_OTP_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_OTP_DIVIDERS_READ(reg);
	reg.m_otp_en = 1;  	
	reg.m_otp = xi_post_divider; 
	reg.otp_pll_sel = xi_pll;	
    BL_CR_CONTROL_REGS_OTP_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_otp_divider);

BL_LILAC_SOC_STATUS bl_lilac_set_usb_host_divider(int32_t xi_pll, int32_t xi_post_divider)
{
    volatile CR_CONTROL_REGS_USB_HOST_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;
    
    BL_CR_CONTROL_REGS_USB_HOST_DIVIDERS_READ(reg);
    reg.usb_host_pll_sel = xi_pll;
    reg.m_usb_host_en = 1;
    reg.m_usb_host = xi_post_divider;
    BL_CR_CONTROL_REGS_USB_HOST_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_usb_host_divider);

BL_LILAC_SOC_STATUS bl_lilac_set_pci_divider(int32_t xi_pll, int32_t xi_post_divider)
{
    volatile CR_CONTROL_REGS_PCI_HOST_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_PCI_HOST_DIVIDERS_READ(reg);
	reg.m_pci_host_en = 1;
	reg.m_pci_host = xi_post_divider;
	reg.pci_host_pll_sel = xi_pll;
    BL_CR_CONTROL_REGS_PCI_HOST_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_pci_divider);

BL_LILAC_SOC_STATUS bl_lilac_set_cdr_divider(int32_t xi_pll, int32_t xi_post_divider)
{
    volatile CR_CONTROL_REGS_CDR_DIAG_FAST_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_CDR_DIAG_FAST_DIVIDERS_READ(reg);
	reg.m_cdr_diag_fast_en = 1;
	reg.m_cdr_diag_fast = xi_post_divider;
	reg.cdr_diag_fast_pll_sel = xi_pll;
    BL_CR_CONTROL_REGS_CDR_DIAG_FAST_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_cdr_divider);

/***************************************************************************/
/*     Set TM divider register                                             */
/* Input                                                                   */
/*     pll number : 0 - 2, TM post divider, Runner post divider            */
/* Output                                                                  */
/*     success or failure                                                  */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_tm_divider(int32_t xi_pll, int32_t xi_tm_divider, int32_t xi_rnr_divider)
{
    volatile CR_CONTROL_REGS_TM_DIVIDERS_DTE reg;

    BL_CR_CONTROL_REGS_TM_DIVIDERS_READ(reg);
    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

	reg.m_tm_en = 1;   	
	reg.m_tm = xi_tm_divider;  
	reg.m_rnr_en = 1; 
	reg.m_rnr = xi_rnr_divider;
	reg.rnr_pll_sel = xi_pll;  
    BL_CR_CONTROL_REGS_TM_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_tm_divider);

/***************************************************************************/
/*     Set DDR divider register                                            */
/* Input                                                                   */
/*     pll number : 0 - 2, DDR post divider, DDR bridge post divider       */
/* Output                                                                  */
/*     success or failure                                                  */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_ddr_divider(int32_t xi_pll, int32_t xi_post_divider, int32_t xi_bridge_divider)
{
    volatile CR_CONTROL_REGS_DDR_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_DDR_DIVIDERS_READ(reg);
    /*
	reg.ddr_xor2       	
	reg.ddr_bypass_div2	
    */
	reg.m_ddr_bridge_en	= 1 ;
	reg.m_ddr_bridge = xi_bridge_divider ;
	reg.m_ddr_en = 1;
	reg.m_ddr = xi_post_divider;
	reg.ddr_pll_sel = xi_pll;
    BL_CR_CONTROL_REGS_DDR_DIVIDERS_WRITE(reg);
    
    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_ddr_divider);

/***************************************************************************/
/*     Set TDM divider register                                            */
/* Input                                                                   */
/*     pll number : 0 - 2, pre divider, post divider                       */
/* Output                                                                  */
/*     success or failure                                                  */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_tdmrx_divider(int32_t xi_pll, int32_t xi_pre_divider, int32_t xi_post_divider)
{
    volatile CR_CONTROL_REGS_TDMRX_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_TDMRX_DIVIDERS_READ(reg);
	reg.m_tdmrx_en = 1;      	
	reg.dsp_tdm_rxclk_sel = 1;	
	reg.m_tdmrx = xi_post_divider;     
	reg.pre_m_tdmrx = xi_pre_divider;  
	reg.tdmrx_pll_sel = xi_pll;  
    BL_CR_CONTROL_REGS_TDMRX_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_tdmrx_divider);

BL_LILAC_SOC_STATUS bl_lilac_set_tdmtx_divider(int32_t xi_pll, int32_t xi_pre_divider, int32_t xi_post_divider)
{
    volatile CR_CONTROL_REGS_TDMTX_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;
    
    BL_CR_CONTROL_REGS_TDMTX_DIVIDERS_READ(reg);
	reg.m_tdmtx_en = 1;      	
	reg.dsp_tdm_rxclk_sel = 1;	
	reg.m_tdmtx = xi_post_divider;     
	reg.pre_m_tdmtx = xi_pre_divider;  
	reg.tdmtx_pll_sel = xi_pll;  
    BL_CR_CONTROL_REGS_TDMTX_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_tdmtx_divider);

/***************************************************************************/
/*     Set TDM SYNC divider register                                       */
/* Input                                                                   */
/*     pll number : 0 - 2, pre divider, post divider,                      */
/*                  generate 25 Mhz, if equal to -1, no change is done     */
/*           sync on 25, sync on 10, if equal to -1, no change is done     */
/* Output                                                                  */
/*     success or failure                                                  */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_synce_tdm_divider(int32_t xi_pll, int32_t xi_pre_divider, int32_t xi_post_divider,
                                                   int32_t xi_gen, int32_t xi_25, int32_t xi_10)
{
    volatile CR_CONTROL_REGS_SYNCE_TDM_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    BL_CR_CONTROL_REGS_SYNCE_TDM_DIVIDERS_READ(reg);
	
    reg.m_synce_tdm_en = 1; 
    if(xi_gen >= 0)
        reg.gen_25_ref = xi_gen;
	
	if(xi_25 >=0)
    {
        reg.m_synce_25_en = xi_25; 
        reg.m_synce_tdm_en = 0; 
    }
	
	if(xi_10 >=0)
        reg.m_synce_10_en = xi_10;
	
	reg.m_synce_tdm = xi_post_divider;   
	reg.pre_m_synce_tdm = xi_pre_divider;
	reg.synce_tdm_pll_sel = xi_pll;
    BL_CR_CONTROL_REGS_SYNCE_TDM_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_synce_tdm_divider);

/***************************************************************************/
/*     Set USB core divider register                                       */
/* Input                                                                   */
/*     pll number : 0 - 2                                                  */
/*                  pre divider = -1, no change is done                    */
/*                  post divider = -1, no change is done                   */
/* Output                                                                  */
/*     success or failure                                                  */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_usb_core_divider(int32_t xi_pll, int32_t xi_pre_divider, int32_t xi_post_divider)
{
    volatile CR_CONTROL_REGS_USB_CORE_DIVIDERS_DTE reg;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;
    
    BL_CR_CONTROL_REGS_USB_CORE_DIVIDERS_READ(reg);
    reg.usb_core_pll_sel = xi_pll;
	
    if(xi_pre_divider >= 0)
        reg.pre_m_usb_core = xi_pre_divider;
	
    if(xi_post_divider >= 0)
        reg.m_usb_core = xi_post_divider; 
	
    reg.m_usb_core_en = 1;
    reg.m_usb_clk_12_en = 1;
    BL_CR_CONTROL_REGS_USB_CORE_DIVIDERS_WRITE(reg);

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_usb_core_divider);

/***************************************************************************/
/*     Set the required PLL register                                       */
/* Input                                                                   */
/*     pll number : 0 - 2, reference clock : 0 - 4                         */
/*     PLL parameters : k - pre divider, n - feadback divider, m - output  */
/*                      divider                                            */
/* Output                                                                  */
/*     success or failure                                                  */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_set_pll(int32_t xi_pll, int32_t xi_ref, int32_t xi_k, int32_t xi_m, int32_t xi_n)
{
    volatile CR_CONTROL_REGS_PLL_0_DTE reg;
    volatile CR_CONTROL_REGS_PLL_PD_DTE pll_pd;
	int locked = 0;

    if((xi_pll > 2) || (xi_ref > 4))
        return BL_LILAC_SOC_INVALID_PARAM;

	BL_CR_CONTROL_REGS_PLL_PD_READ(pll_pd);
    switch (xi_pll)
    {
    case 0:
        BL_CR_CONTROL_REGS_PLL_0_READ(reg);
        reg.pll_0_src_sel = xi_ref;
        reg.pll_0_m = xi_m;
        reg.pll_0_n = xi_n;
        reg.pll_0_k = xi_k;
        BL_CR_CONTROL_REGS_PLL_0_WRITE(reg);
		pll_pd.pll_0_lk = 0;
        break;
    case 1:
        BL_CR_CONTROL_REGS_PLL_1_READ(reg);
        reg.pll_0_src_sel = xi_ref;
        reg.pll_0_m = xi_m;
        reg.pll_0_n = xi_n;
        reg.pll_0_k = xi_k;
        BL_CR_CONTROL_REGS_PLL_1_WRITE(reg);
		pll_pd.pll_1_lk = 0;
        break;
    case 2:
        BL_CR_CONTROL_REGS_PLL_2_READ(reg);
        reg.pll_0_src_sel = xi_ref;
        reg.pll_0_m = xi_m;
        reg.pll_0_n = xi_n;
        reg.pll_0_k = xi_k;
        BL_CR_CONTROL_REGS_PLL_2_WRITE(reg);
		pll_pd.pll_2_lk = 0;
        break;
    }
	
	do
	{
		BL_CR_CONTROL_REGS_PLL_PD_WRITE(pll_pd);
		BL_CR_CONTROL_REGS_PLL_PD_READ(pll_pd);
		switch (xi_pll)
		{
		case 0:
			locked = pll_pd.pll_0_lk;
			break;
		case 1:
			locked = pll_pd.pll_1_lk;
			break;
		case 2:
			locked = pll_pd.pll_2_lk;
			break;
		}
	} while(!locked);
	
    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_set_pll);

/***************************************************************************/
/* Title                                                                   */
/*     returns the MIPS-C actual output clock and peripheral clock         */ 
/*     takes in account the reference clock and PLL's parameters           */
/* Output                                                                  */
/*     CPU frequency in Hz, after all dividers                             */
/*     Peripherals clock in Hz, after all dividers                         */
/*     success or failure                                                  */
/***************************************************************************/
uint32_t get_lilac_system_clock_rate(void)
{
    volatile CR_CONTROL_REGS_MIPS_C_DIVIDERS_DTE reg;
    volatile CR_CONTROL_REGS_PLL_0_DTE pllreg;
    uint32_t ref;
    uint32_t freq;

    BL_CR_CONTROL_REGS_MIPS_C_DIVIDERS_READ(reg);

    switch(reg.mips_c_pll_sel)
    {
    case 0:
        BL_CR_CONTROL_REGS_PLL_0_READ(pllreg);
        ref = pllreg.pll_0_src_sel;
        break;
    case 1:
        BL_CR_CONTROL_REGS_PLL_1_READ(pllreg);
        ref = pllreg.pll_0_src_sel;
        break;
    case 2:
        BL_CR_CONTROL_REGS_PLL_2_READ(pllreg);
        ref = pllreg.pll_0_src_sel;
        break;
    default: // pll bypass
        return BL_PLL_IN_BYPASS;
    }
    freq = bl_lilac_get_pll_src_freq(ref);

    /* PLL output_frequency =  reference_frequency /((k+1)* (m+1))*(n+1) */
	freq = freq/((pllreg.pll_0_k+1) * (pllreg.pll_0_m+1)) * (pllreg.pll_0_n+1);
     /* output frequency, after mipsc divider */
	return  freq/(reg.m_mips_c+1);
}
EXPORT_SYMBOL(get_lilac_system_clock_rate);


uint32_t get_lilac_rnr_clock_rate(void)
{
    volatile CR_CONTROL_REGS_TM_DIVIDERS_DTE reg;
	
    volatile CR_CONTROL_REGS_PLL_0_DTE pllreg;
    uint32_t ref;
    uint32_t freq;

    BL_CR_CONTROL_REGS_TM_DIVIDERS_READ(reg);

    switch(reg.rnr_pll_sel)
    {
    case 0:
        BL_CR_CONTROL_REGS_PLL_0_READ(pllreg);
        ref = pllreg.pll_0_src_sel;
        break;
    case 1:
        BL_CR_CONTROL_REGS_PLL_1_READ(pllreg);
        ref = pllreg.pll_0_src_sel;
        break;
    case 2:
        BL_CR_CONTROL_REGS_PLL_2_READ(pllreg);
        ref = pllreg.pll_0_src_sel;
        break;
    default: // pll bypass
        return BL_PLL_IN_BYPASS;
    }
    freq = bl_lilac_get_pll_src_freq(ref);

    /* PLL output_frequency =  reference_frequency /((k+1)* (m+1))*(n+1) */
	freq = freq/((pllreg.pll_0_k+1) * (pllreg.pll_0_m+1)) * (pllreg.pll_0_n+1);
     /* output frequency, after rnr m divider */
	if(reg.m_rnr_en)
		freq /= (reg.m_rnr+1);
	
	return  freq;

}
EXPORT_SYMBOL(get_lilac_rnr_clock_rate);

uint32_t get_lilac_ddr_clock_rate(void)
{
    CR_CONTROL_REGS_DDR_DIVIDERS_DTE reg;
    CR_CONTROL_REGS_PLL_0_DTE pllreg;
    uint32_t freq;

    BL_CR_CONTROL_REGS_DDR_DIVIDERS_READ(reg);

    switch(reg.ddr_pll_sel)
    {
    case 0:
        BL_CR_CONTROL_REGS_PLL_0_READ(pllreg);
        break;
    case 1:
        BL_CR_CONTROL_REGS_PLL_1_READ(pllreg);
        break;
    case 2:
        BL_CR_CONTROL_REGS_PLL_2_READ(pllreg);
        break;
    default: // pll bypass
        return BL_PLL_IN_BYPASS;
    }
    freq = bl_lilac_get_pll_src_freq(pllreg.pll_0_src_sel);

    /* PLL output_frequency =  reference_frequency /((k+1)* (m+1))*(n+1) */
	freq = freq/((pllreg.pll_0_k+1) * (pllreg.pll_0_m+1)) * (pllreg.pll_0_n+1);
     /* if m divider enabled, usi it */
	if(reg.m_ddr_en)
		freq /= (reg.m_ddr+1);
	if(!reg.ddr_bypass_div2)
		freq /=2;
		
	return  freq;
}
EXPORT_SYMBOL(get_lilac_ddr_clock_rate);

uint32_t get_lilac_peripheral_clock_rate(void)
{
    volatile CR_CONTROL_REGS_MIPS_C_DIVIDERS_DTE reg;
    uint32_t ocp;
    uint32_t freq;
    uint32_t ret;

    freq = get_lilac_system_clock_rate();
    BL_CR_CONTROL_REGS_MIPS_C_DIVIDERS_READ(reg);
    ocp = (uint32_t)reg.m_ocp_c; 
    /* output OCP frequency */
    if(ocp)
        ret = freq/4;
    else
        ret = freq/2;
    /* output ginal peripheral frequency is the OCP frequency divided by peripheral divider */
    ret = ret/((uint32_t)reg.m_periph_c+1);

    return ret;
}

EXPORT_SYMBOL(get_lilac_peripheral_clock_rate);

BL_LILAC_SOC_STATUS get_lilac_clock_rate(uint32_t *xo_freq, uint32_t *xo_periph)
{
    if(!xo_freq || !xo_periph)
        return BL_LILAC_SOC_INVALID_PARAM;

    *xo_freq = get_lilac_system_clock_rate();
    *xo_periph = get_lilac_peripheral_clock_rate();

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(get_lilac_clock_rate);

/***************************************************************************/
/* Title                                                                   */
/*     Get the required PLL parameters                                     */
/* Input                                                                   */
/*     pll number : 0 - 2                                                  */
/* Output                                                                  */
/*     success or failure                                                  */
/*     PLL parameters : k - pre divider, n - feadback divider, m - output  */
/*                      divider, ref - reference clock                     */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_get_pll_parameters(int32_t xi_pll, int32_t *xo_ref, int32_t *xo_k, int32_t *xo_m, int32_t *xo_n)
{
    volatile CR_CONTROL_REGS_PLL_0_DTE reg;

    if(!xo_ref || !xo_k || !xo_m || !xo_n)
        return BL_LILAC_SOC_INVALID_PARAM;

    if(xi_pll > 2)
        return BL_LILAC_SOC_INVALID_PARAM;

    switch (xi_pll)
    {
    case 0:
        BL_CR_CONTROL_REGS_PLL_0_READ(reg);
        break;
    case 1:
        BL_CR_CONTROL_REGS_PLL_1_READ(reg);
        break;
    case 2:
        BL_CR_CONTROL_REGS_PLL_2_READ(reg);
        break;
    }
    *xo_ref = reg.pll_0_src_sel;
    *xo_m   = reg.pll_0_m;
    *xo_n   = reg.pll_0_n;
    *xo_k   = reg.pll_0_k;

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_get_pll_parameters);

/***************************************************************************/
/* Title                                                                   */
/*     Get the required PLL output clock                                   */
/* Input                                                                   */
/*     pll number : 0 - 2                                                  */
/* Output                                                                  */
/*     PLL clock frequency                                                 */
/***************************************************************************/
int32_t bl_lilac_get_pll_output(int32_t xi_pll)
{
    int32_t output,freq,ref,k,m,n;

    if(bl_lilac_get_pll_parameters(xi_pll,&ref,&k,&m,&n) != BL_LILAC_SOC_OK) 
        return 0;

    freq = bl_lilac_get_pll_src_freq(ref);
    /* PLL output_frequency = [[ reference_frequency / (k+1)] * (n+1) ] / (m+1) */
    output = ((freq/(k+1))*(n+1))/(m+1);

    return output;
}
EXPORT_SYMBOL(bl_lilac_get_pll_output);

BL_LILAC_SOC_STATUS bl_lilac_cr_get_emac_reset_state ( BL_LILAC_CR_DEVICE xi_emac_cr_device_id , BL_LILAC_CR_STATE * xo_reset_state )
{
    CR_CONTROL_REGS_SW_RST_H_0_DTE sw_rst_h_0 ;
    uint8_t bit_value ;

    BL_CR_CONTROL_REGS_SW_RST_H_0_READ( sw_rst_h_0 ) ;

    switch ( xi_emac_cr_device_id )
    {
    case BL_LILAC_CR_NIF_0:
        bit_value = sw_rst_h_0.nif_0 ;
        break ;
    case BL_LILAC_CR_NIF_1:
        bit_value = sw_rst_h_0.nif_1 ;
        break ;
    case BL_LILAC_CR_NIF_2:
        bit_value = sw_rst_h_0.nif_2 ;
        break ;
    case BL_LILAC_CR_NIF_3:
        bit_value = sw_rst_h_0.nif_3 ;
        break ;
    case BL_LILAC_CR_NIF_4:
        bit_value = sw_rst_h_0.nif_4 ;
        break ;
    case BL_LILAC_CR_NIF_5:
        bit_value = sw_rst_h_0.nif_5 ;
        break ;

    default:
        return BL_LILAC_SOC_INVALID_PARAM ;
        break ;
    }

    * xo_reset_state = ! bit_value ;

    return BL_LILAC_SOC_OK ;
}
EXPORT_SYMBOL(bl_lilac_cr_get_emac_reset_state);


void bl_lilac_enable_25M_clock()
{
	CR_CONTROL_REGS_ENABLE_L_DTE enableL;

	BL_CR_CONTROL_REGS_ENABLE_L_READ(enableL);
	enableL.synce = 1;
	BL_CR_CONTROL_REGS_ENABLE_L_WRITE(enableL);
}
EXPORT_SYMBOL(bl_lilac_enable_25M_clock);


#endif /* #ifdef __MIPS_C */

