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
#include "bl_lilac_pci.h"
#include "bl_lilac_pcie.h"
#include "bl_lilac_cr.h"
#include "bl_lilac_serdes.h"
#include "bl_lilac_mips_blocks.h"

#define SLV_REQ_MISC_INFO_REG_VALUE		0xc00b
#define EC2AHB_EC_CFG_0_REG_VALUE		0x2347

#define ___swab32(x) (\
		(((x) & 0x000000ffUL) << 24) | \
		(((x) & 0x0000ff00UL) <<  8) | \
		(((x) & 0x00ff0000UL) >>  8) | \
		(((x) & 0xff000000UL) >> 24) )

#define ___swab16(x) (\
		(((x) & 0x00ffUL) << 8) | \
		(((x) & 0xff00UL) >> 8) )
		
void bl_lilac_pci_init(void)
{
	CR_CONTROL_REGS_SW_RST_H_0_DTE rst0;
	CR_CONTROL_REGS_ENABLE_L_DTE enableL;
	CR_CONTROL_REGS_PCI_HOST_DIVIDERS_DTE pciDivs;
	BL_LILAC_PLL_CLIENT_PROPERTIES props;
	SERDES_PCIE_CMN_REG_0_DTE pcie0; 
	SERDES_PCIE_CMN_REG_1_DTE pcie1;
	SERDES_PCIE_CMN_REG_3_DTE pcie3;
	SERDES_PCIE_CMN_REG_4_DTE pcie4;
	SERDES_PCIE_CMN_REG_6_DTE pcie6;

	// enable pci serdes
	BL_CR_CONTROL_REGS_ENABLE_L_READ(enableL);
	enableL.pci_serdes = 1;
	BL_CR_CONTROL_REGS_ENABLE_L_WRITE(enableL);
	
	// bring pci serdes from reset
	BL_CR_CONTROL_REGS_SW_RST_H_0_READ(rst0);
	rst0.pci_serdes_vpb = 0;
	BL_CR_CONTROL_REGS_SW_RST_H_0_WRITE(rst0);
	

	// set pci host dividers
	bl_lilac_pll_client_properties(PLL_CLIENT_PCI_HOST, &props);
	pciDivs.pci_host_pll_sel = props.pll;
	pciDivs.m_pci_host = props.m;
	pciDivs.m_pci_host_en = 1;
	BL_CR_CONTROL_REGS_PCI_HOST_DIVIDERS_WRITE(pciDivs);

	// pci serdes common configuration
	pcie6.cdr_threshint = 0;
	pcie6.cdr_thresh2   = CE_SERDES_PCIE_CMN_REG_6_CDR_THRESH2_DEFAULT_VALUE;
	pcie6.cdr_thresh1   = 0;
	pcie6.cdr_gain2	  = 7;
	pcie6.cdr_gain1	  = 0;
	BL_SERDES_PCIE_CMN_REG_6_WRITE(pcie6);

    pcie1.pll_dsmbyp    = CE_SERDES_PCIE_CMN_REG_1_PLL_DSMBYP_DEFAULT_VALUE;
    pcie1.pll_fbdivint  = CE_SERDES_PCIE_CMN_REG_1_PLL_FBDIVINT_DEFAULT_VALUE;
    pcie1.pll_fbdivfrac = CE_SERDES_PCIE_CMN_REG_1_PLL_FBDIVFRAC_DEFAULT_VALUE;
	BL_SERDES_PCIE_CMN_REG_1_WRITE(pcie1);
	
	BL_SERDES_PCIE_CMN_REG_3_READ(pcie3);
	pcie3.pll_div0rst_n = 0;
	pcie3.pll_div1rst_n = 0;
	BL_SERDES_PCIE_CMN_REG_3_WRITE(pcie3);
	
	BL_SERDES_PCIE_CMN_REG_4_READ(pcie4);
	pcie4.pll_rst_n = 1;
	BL_SERDES_PCIE_CMN_REG_4_WRITE(pcie4);

	BL_SERDES_PCIE_CMN_REG_0_READ(pcie0);
	pcie0.pll_en = 1;
	pcie0.clkdis_en = 1;
	pcie0.bias_en = 1;
	BL_SERDES_PCIE_CMN_REG_0_WRITE(pcie0);
	
	mdelay(2);
	
	pcie0.pll_vcalstart = 1;
	BL_SERDES_PCIE_CMN_REG_0_WRITE(pcie0);

	pcie0.pll_vcalstart = 0;
	BL_SERDES_PCIE_CMN_REG_0_WRITE(pcie0);

	do BL_SERDES_PCIE_CMN_REG_0_READ(pcie0);
	while(!pcie0.pll_locked);
	
	pcie0.txcal_start = 1;
	pcie0.rxcal_start = 1;
	BL_SERDES_PCIE_CMN_REG_0_WRITE(pcie0);
	
	mdelay(1);

	do BL_SERDES_PCIE_CMN_REG_0_READ(pcie0);
	while(!pcie0.txcal_done || !pcie0.rxcal_done || !pcie0.pll_locked);
	
	pcie3.pll_div0rst_n = 1;
	pcie3.pll_div1rst_n = 1;
	BL_SERDES_PCIE_CMN_REG_3_WRITE(pcie3);
	
	pcie6.cdr_threshint = 1;
	pcie6.cdr_thresh2   = 2;
	pcie6.cdr_thresh1   = CE_SERDES_PCIE_CMN_REG_6_CDR_THRESH1_DEFAULT_VALUE;
	pcie6.cdr_gain2	  = CE_SERDES_PCIE_CMN_REG_6_CDR_GAIN2_DEFAULT_VALUE;
	pcie6.cdr_gain1	  = CE_SERDES_PCIE_CMN_REG_6_CDR_GAIN1_DEFAULT_VALUE;
	BL_SERDES_PCIE_CMN_REG_6_WRITE(pcie6);
}

void bl_lilac_pci_init_lane(int id)
{
	CR_CONTROL_REGS_ENABLE_L_DTE enableL;
	CR_CONTROL_REGS_SW_RST_H_2_DTE rst2;
	MIPS_BLOCKS_VPB_BRIDGE_PCIE_FIFO_RST_N_DTE fifoRst;
	SERDES_PCIE_LANE_REG_7_DTE pcie7; 
	SERDES_PCIE_LANE_REG_8_DTE pcie8;
	SERDES_PCIE_LANE_REG_9_DTE pcie9;
	SERDES_PCIE_LANE_REG_10_DTE pcie10;
	unsigned long val;
	
	// enable pci controller
	BL_CR_CONTROL_REGS_ENABLE_L_READ(enableL);
	if(id)
		enableL.pci_1 = 1;
	else
		enableL.pci_0 = 1;
	BL_CR_CONTROL_REGS_ENABLE_L_WRITE(enableL);

	// bring pci lane blocks from reset
	BL_CR_CONTROL_REGS_SW_RST_H_2_READ(rst2);
	if(id)
	{
		rst2.pci_1_core = 0;
		rst2.pci_1_pcs = 0;
		rst2.pci_1_ahb = 0;
		rst2.pci_1_ec = 0;
		rst2.pci_ddr_1 = 0;
	}
	else
	{
		rst2.pci_0_core = 0;
		rst2.pci_0_pcs = 0;
		rst2.pci_0_ahb = 0;
		rst2.pci_0_ec = 0;
		rst2.pci_ddr_0 = 0;
	}
	BL_CR_CONTROL_REGS_SW_RST_H_2_WRITE(rst2);
	
	// pci serdes lane configuration
	BL_SERDES_PCIE_LANE_REG_7_READ(id, pcie7);
	pcie7.rx_en = 1;
	pcie7.rx_subrate = 1;
	pcie7.tx_en = 1;
	pcie7.tx_subrate = 1;
	BL_SERDES_PCIE_LANE_REG_7_WRITE(id, pcie7);
	
	BL_SERDES_PCIE_LANE_REG_10_READ(id, pcie10);
	pcie10.cdr_hold = 1;
	BL_SERDES_PCIE_LANE_REG_10_WRITE(id, pcie10);
	
	pcie10.pi_reset = 1;
	BL_SERDES_PCIE_LANE_REG_10_WRITE(id, pcie10);

	pcie10.pi_reset = 0;
	BL_SERDES_PCIE_LANE_REG_10_WRITE(id, pcie10);
	
	pcie7.tx_rst_n = 0;
	pcie7.rx_rst_n = 0;
	pcie7.rx_subrate = 2;
	pcie7.tx_subrate = 2;
	BL_SERDES_PCIE_LANE_REG_7_WRITE(id, pcie7);

	pcie7.tx_rst_n = 1;
	pcie7.rx_rst_n = 1;
	BL_SERDES_PCIE_LANE_REG_7_WRITE(id, pcie7);

	BL_MIPS_BLOCKS_VPB_BRIDGE_PCIE_FIFO_RST_N_READ(fifoRst);
	if(id)
	{
		fifoRst.tx_rst_n_1 = 0;
		fifoRst.rx_rst_n_1 = 0;
		BL_MIPS_BLOCKS_VPB_BRIDGE_PCIE_FIFO_RST_N_WRITE(fifoRst);
		fifoRst.tx_rst_n_1 = 0;
		fifoRst.rx_rst_n_1 = 1;
		BL_MIPS_BLOCKS_VPB_BRIDGE_PCIE_FIFO_RST_N_WRITE(fifoRst);
		fifoRst.tx_rst_n_1 = 1;
		fifoRst.rx_rst_n_1 = 1;
	}
	else
	{
		fifoRst.tx_rst_n_0 = 0;
		fifoRst.rx_rst_n_0 = 0;
		BL_MIPS_BLOCKS_VPB_BRIDGE_PCIE_FIFO_RST_N_WRITE(fifoRst);
		fifoRst.tx_rst_n_0 = 0;
		fifoRst.rx_rst_n_0 = 1;
		BL_MIPS_BLOCKS_VPB_BRIDGE_PCIE_FIFO_RST_N_WRITE(fifoRst);
		fifoRst.tx_rst_n_0 = 1;
		fifoRst.rx_rst_n_0 = 1;
	}
	BL_MIPS_BLOCKS_VPB_BRIDGE_PCIE_FIFO_RST_N_WRITE(fifoRst);
	
	pcie10.cdr_hold = 0;
	BL_SERDES_PCIE_LANE_REG_10_WRITE(id, pcie10);

	BL_SERDES_PCIE_LANE_REG_9_READ(id, pcie9);
	pcie9.rcv_r_rcvcalovr = 8;
	pcie9.rcv_r_eq = 9;
	BL_SERDES_PCIE_LANE_REG_9_WRITE(id, pcie9);

	BL_SERDES_PCIE_LANE_REG_8_READ(id, pcie8);
	pcie8.drv_t_deemp = 0;
	pcie8.drv_t_drvdaten = 1;
	pcie8.drv_t_hiz = 0;
	pcie8.drv_t_idle = 0;
	BL_MIPS_BLOCKS_VPB_BRIDGE_VERSION_READ(val);
	if(val & 0x1)
		pcie8.drv_t_slewsel = 3;
	else
		pcie8.drv_t_slewsel = 0;
	pcie8.drv_t_vmarg = 0x10;
	pcie8.serl_t_dlysel = 0;
	BL_SERDES_PCIE_LANE_REG_8_WRITE(id, pcie8);

	if(id)
	{
		fifoRst.tx_rst_n_1 = 0;
		fifoRst.rx_rst_n_1 = 1;
		BL_MIPS_BLOCKS_VPB_BRIDGE_PCIE_FIFO_RST_N_WRITE(fifoRst);
		fifoRst.tx_rst_n_1 = 1;
		fifoRst.rx_rst_n_1 = 1;
	}
	else
	{
		fifoRst.tx_rst_n_0 = 0;
		fifoRst.rx_rst_n_0 = 1;
		BL_MIPS_BLOCKS_VPB_BRIDGE_PCIE_FIFO_RST_N_WRITE(fifoRst);
		fifoRst.tx_rst_n_0 = 1;
		fifoRst.rx_rst_n_0 = 1;
	}
	BL_MIPS_BLOCKS_VPB_BRIDGE_PCIE_FIFO_RST_N_WRITE(fifoRst);

	// pci bridges configuration
	if(id)
	{
		val = 0xe00018;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_1_REG_WRITE(val);
		val = 0xffff0018;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_7_REG_WRITE(val);
		val = 0xc00018;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_2_REG_WRITE(val);
		val = 0xffdf0018;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_8_REG_WRITE(val);
		val = 0x14;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_3_REG_WRITE(val);
		val = 0xffffff17;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_9_REG_WRITE(val);
		val = 0xa00018;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_4_REG_WRITE(val);
		val = 0xffbf0018;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_10_REG_WRITE(val);
		val = 0x800018;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_5_REG_WRITE(val);
		val = 0xff9f0018;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_11_REG_WRITE(val);
		val = 0x10;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_6_REG_WRITE(val);
		val = 0xffffff13;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_12_REG_WRITE(val);
	}
	else
	{
		val = 0xa00018;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_1_REG_WRITE(val);
		val = 0xffbf0018;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_7_REG_WRITE(val);
		val = 0x800018;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_2_REG_WRITE(val);
		val = 0xff9f0018;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_8_REG_WRITE(val);
		val = 0x10;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_3_REG_WRITE(val);
		val = 0xffffff13;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_9_REG_WRITE(val);
		val = 0xe00018;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_4_REG_WRITE(val);
		val = 0xffff0018;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_10_REG_WRITE(val);
		val = 0xc00018;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_5_REG_WRITE(val);
		val = 0xffdf0018;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_11_REG_WRITE(val);
		val = 0x14;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_6_REG_WRITE(val);
		val = 0xffffff17;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_12_REG_WRITE(val);
	}
	val = EC2AHB_EC_CFG_0_REG_VALUE;
	BL_PCIE_REGFILE_EC2AHB_EC_CFG_0_REG_WRITE(id, val);
	val = 0x20100000;
	BL_PCIE_REGFILE_AHB2EC_AHB_CFG_14_REG_WRITE(id, val);

	// to write registers missing in reggae
	#define BL_PCIE_CORE_REG(id, reg) *(volatile unsigned long *)(0xb8000000 + reg + (id ? 0xc000:0x8000))

	// pci core configuration
	BL_PCIE_CORE_REG(id, 0x10) = 0x101011;
	BL_PCIE_CORE_REG(id, 0x14) = 0x201011;
	val = 0x1ff00;
	BL_PCIE_CORE_RC_LT_SP_BUS_N_WRITE(id, val);
	val = 0x0;
	BL_PCIE_CORE_RC_SS_IO_LT_BAS_WRITE(id, val);
	val = 0x20222f22;
	BL_PCIE_CORE_RC_MEM_LT_BAS_WRITE(id, val);
	val = 0x30223f22;
	BL_PCIE_CORE_RC_PF_MEM_LT_BAS_WRITE(id, val);
	BL_PCIE_CORE_REG(id, 0x28) = 0x0;
	BL_PCIE_CORE_REG(id, 0x2c) = 0x0;
	val = 0x100010;
	BL_PCIE_CORE_RC_IO_LT_BAS_WRITE(id, val);
	BL_PCIE_CORE_REG(id, 0x38) = 0xa5a5a5a5;
	val = 0x100300;
	BL_PCIE_CORE_RC_BRDG_INT_WRITE(id, val);
	BL_PCIE_CORE_REG(id, 0x78) = 0x3f180000;
	BL_PCIE_CORE_REG(id, 0x70c) = 0xf0f1b;
	BL_PCIE_CORE_REG(id, 0x720) = 0x0;
	BL_PCIE_CORE_REG(id, 0x80) = 0x1110;
	BL_PCIE_CORE_REG(id, 0x8c) = 0x1f000000;
	BL_PCIE_CORE_REG(id, 0x90) = 0x0;
	BL_PCIE_CORE_REG(id, 0x104) = 0x0;
	BL_PCIE_CORE_REG(id, 0x110) = 0x0;
	BL_PCIE_CORE_REG(id, 0x12c) = 0x7000000;
	BL_PCIE_CORE_REG(id, 0x130) = 0x0;
	val = 0xc40000;
	BL_PCIE_REGFILE_CORE_SYSTEM_REG_WRITE(id, val);
	val = 0x7011000;
	BL_PCIE_CORE_RC_STS_CMD_RGSTR_WRITE(id, val);
	
	// split IO/MEM spaces
	if(id)
	{
		val = 0x0c000000;
		BL_MIPS_BLOCKS_OBIU_CFG_PCI_1_MASK_BASE_WRITE(val);
		
		val = ___swab32(BL_PCI_IO_START_1);
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_15_REG_WRITE(val);
		val = ___swab32(~(BL_PCI_IO_END_1 - BL_PCI_IO_START_1));
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_19_REG_WRITE(val);
		val = 0xffffffff;
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_16_REG_WRITE(val);
		BL_PCIE_REGFILE_1_EC2AHB_EC_CFG_20_REG_WRITE(val);
	}
	else
	{
		val = 0x0c000000;
		BL_MIPS_BLOCKS_OBIU_CFG_PCI_0_MASK_BASE_WRITE(val);
		
		val = ___swab32(BL_PCI_IO_START_0);
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_15_REG_WRITE(val);
		val = ___swab32(~(BL_PCI_IO_END_0 - BL_PCI_IO_START_0));
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_19_REG_WRITE(val);
		val = 0xffffffff;
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_16_REG_WRITE(val);
		BL_PCIE_REGFILE_0_EC2AHB_EC_CFG_20_REG_WRITE(val);
	}
	val = SLV_REQ_MISC_INFO_REG_VALUE;
	BL_PCIE_REGFILE_CORE_SLV_REQ_MISC_INFO_REG_WRITE(id, val);
}

BL_LILAC_SOC_STATUS bl_lilac_pci_rw_config(int id, int write, unsigned int devfn, int where, int size, unsigned long  *val)
{
	unsigned long  reg;
	BL_LILAC_SOC_STATUS ret = BL_LILAC_SOC_OK;
	unsigned long addr = 0xa0000000 | (id ? BL_PCI_MEM_START_1 : BL_PCI_MEM_START_0) | (devfn << 16) | where;

	// we support only 2 buses
	if((id > 1) || (((devfn >> 3) & 0x1f) != 0))
		return BL_LILAC_SOC_INVALID_PARAM;
	
	// this is the way to check if device is present
	// if link to the device occasionally fails, we'll return error as if device not present
	BL_PCIE_REGFILE_CORE_CONFIGURATION_11_REG_READ(id, reg);
	if(!(reg & 0x2000000))
		return BL_LILAC_SOC_ERROR;
	
	// calculate byte enable by size and address low bits
	switch(size)
	{
		case 1:
			switch(where &0x3)
			{
				case 0:
					reg = 0x4000;
				break;
				case 1:
					reg = 0x8000;
				break;
				case 2:
					reg = 0x1;
				break;
				case 3:
					reg = 0x2;
				break;
			}
		break;
		case 2:
			switch(where &0x3)
			{
				case 0:
					reg = 0xC000;
				break;
				case 2:
					reg = 0x3;
				break;
				default:
					return BL_LILAC_SOC_INVALID_PARAM;
			}
		break;
		case 4:
			switch(where &0x3)
			{
				case 0:
					reg = 0xC003;
				break;
				default:
					return BL_LILAC_SOC_INVALID_PARAM;
			}
		break;
		default:
			return BL_LILAC_SOC_INVALID_PARAM;
	}
	
	// set CFG TLP
	reg |= 0x4000000;
	BL_PCIE_REGFILE_CORE_SLV_REQ_MISC_INFO_REG_WRITE(id, reg);
	
	// enable high address bits substitution
	reg = 0x40000 | EC2AHB_EC_CFG_0_REG_VALUE;
	BL_PCIE_REGFILE_EC2AHB_EC_CFG_0_REG_WRITE(id, reg);
	
	if(write)
	{
		switch(size)
		{
			case 1:
				*(volatile unsigned char *)addr = (unsigned char)*val;
			break;
			case 2:
				*(volatile unsigned short *)addr = ___swab16((unsigned short)*val);
			break;
			case 4:
				*(volatile unsigned long *)addr = ___swab32(*val);
			break;
		}
	}
	else
	{
		switch(size)
		{
			case 1:
				*(unsigned char *)val = *(volatile unsigned char *)addr;
			break;
			case 2:
				*(unsigned short *)val = *(volatile unsigned short *)addr;
			break;
			case 4:
				*(unsigned long *)val = *(volatile unsigned long *)addr;
			break;
		}
		*val = ___swab32(*val);
	}
	
	// disable high address bits substitution
	reg = EC2AHB_EC_CFG_0_REG_VALUE;
	BL_PCIE_REGFILE_EC2AHB_EC_CFG_0_REG_WRITE(id, reg);

	// set MEM TLP
	reg = SLV_REQ_MISC_INFO_REG_VALUE;
	BL_PCIE_REGFILE_CORE_SLV_REQ_MISC_INFO_REG_WRITE(id, reg);
	
	return ret;
}

void bl_lilac_pci_set_int_mask(int id, BL_LILAC_PCI_INTERRUPTS irq, int mask)
{
	unsigned long val;
	
	BL_PCIE_REGFILE_CORE_INT_MASK_REG_READ(id, val);
	if(mask)
		val |= irq;
	else
		val &= ~irq;
	val = ___swab32(val);
	BL_PCIE_REGFILE_CORE_INT_MASK_REG_WRITE(id, val);
}

void bl_lilac_pci_ack_int(int id, BL_LILAC_PCI_INTERRUPTS irq)
{
}

void bl_lilac_pci_ack_int_mask(int id, unsigned long mask)
{
}

unsigned long bl_lilac_pci_get_int_status(int id)
{
	unsigned long val;
	
	BL_PCIE_REGFILE_CORE_INTERRUPT_0_REG_READ(id, val);
	return ___swab32(val);
}
