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
/*                                                                        	*/
/* Title                                                                   	*/
/*                                                                        	*/
/*   NAND driver                                            				*/
/*                                                                         	*/
/* Abstract                                                               	*/
/*                                                                         	*/
/*   Provides NAND flash API used by Linux/U-Boot MTD driver      			*/
/*   Current implementation supports single NAND chip only                  */
/*                                                                         	*/
/****************************************************************************/

#include "bl_os_wraper.h"
#include "linux/mtd/nand.h"
#include "linux/mtd/nand_ecc.h"
#include "bl_lilac_mips_blocks.h"
#include "bl_lilac_pin_muxing.h"
#include "bl_lilac_soc.h"
#include "bl_lilac_rst.h"
#include "bl_lilac_cr.h"


#define log2(num)											\
({ int res;													\
		__asm__ __volatile__(								\
			"clz\t%0, %1\n\t"								\
			: "=r" (res) : "Jr" ((unsigned int)(num)));		\
	31-res;													\
})

/* this layout is for pages from 512B up to 4KB - 6 bytes ECC per each 512B of data */
/* 2 first bytes of every triplet are swaped intentionally to keep right order */
static struct nand_ecclayout lilac_nand_ecclayout = {
	.eccbytes = 6,
	.eccpos = {7, 6, 8, 10, 9, 11, 23, 22, 24, 26, 25, 27, 39, 38, 40, 42, 41, 43, 55, 54, 56, 58, 57, 59,
				71, 70, 72, 74, 73, 75, 87, 86, 88, 90, 89, 91, 103, 102, 104, 106, 105, 107, 119, 118, 120, 122, 121, 123},
	.oobfree = { {2, 4}, {12, 4}, {0, 0} },
};

int Starter = 0;

/* little bit tricky - this function is used not only to calculate ECC, but also to fill block size and spare byte's checksum */
static int lilac_nand_calculate_ecc(struct mtd_info *mtd,
			      const u_char *dat, u_char *ecc_code)
{
	struct nand_chip *this = mtd->priv;
	// offset in spare bytes buffer - 6 bytes ECC, 16 spare bytes per 512B of data
	int offset = (ecc_code - this->buffers->ecccalc)/6 * 16;
	int i, check_sum;

	// calculate ECC for 512B of data - 256Bx2
	// use standard sw ecc calculate function
	this->ecc.size = 256;
 	nand_calculate_ecc(mtd, dat, ecc_code);
 	nand_calculate_ecc(mtd, dat+256, ecc_code+3);
	this->ecc.size = 512;

	if (Starter)
	{
		// update block size in spare bytes
		this->oob_poi[offset+2] = log2(mtd->erasesize/mtd->writesize);

		// calculate spare byte's checksum
		check_sum = 0;
		for(i=0; i<6; i++)
			check_sum += this->oob_poi[offset+i];
		check_sum += this->oob_poi[offset+12];
		check_sum += this->oob_poi[offset+13];
		for(i=0; i<6; i++)
			check_sum += ecc_code[i];

		// update spare byte's checksum
		this->oob_poi[offset+14] = (check_sum>>8)&0xff;
		this->oob_poi[offset+15] = check_sum&0xff;
	}
	
	return 0;
}
	
static int lilac_nand_correct_data(struct mtd_info *mtd, unsigned char *buf,
		      unsigned char *read_ecc, unsigned char *calc_ecc)
{
	struct nand_chip *this = mtd->priv;
	int ret1, ret2;

	this->ecc.size = 256;
	ret1 = nand_correct_data(mtd, buf, read_ecc, calc_ecc);
	if(ret1 >= 0)
	{
		ret2 = nand_correct_data(mtd, buf+256, read_ecc+3, calc_ecc+3);
		if(ret2 >= 0)
			ret1 += ret2;
		else
			ret1 = ret2;
	}
	this->ecc.size = 512;
	
	return ret1;
}

static uint8_t lilac_nand_read_byte(struct mtd_info *mtd)
{
	uint32_t value;

	BL_MIPS_BLOCKS_VPB_NAND_FLASH_DATA_REG_READ(value);
	return (uint8_t)value;
}

static void lilac_nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	int i;

	for(i = 0; i < len; i++)
		buf[i] = lilac_nand_read_byte(mtd);
}

static void lilac_nand_write_byte(struct mtd_info *mtd, const uint8_t byte)
{
	uint32_t value = byte;
	
	BL_MIPS_BLOCKS_VPB_NAND_FLASH_DATA_REG_WRITE(value);
}

static void lilac_nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;

	for(i = 0; i < len; i++)
		lilac_nand_write_byte(mtd, buf[i]);
}

static int lilac_nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;

	for(i = 0; i < len; i++)
		if(buf[i] != lilac_nand_read_byte(mtd))
			return -1;

	return 0;
}

static void lilac_nand_hwcontrol(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	uint32_t value = 0;
	MIPS_BLOCKS_VPB_NAND_FLASH_CTRL_ASSERT_DTE *pval = (MIPS_BLOCKS_VPB_NAND_FLASH_CTRL_ASSERT_DTE *)&value;

	if(ctrl & NAND_CTRL_CHANGE) {
		if(ctrl & NAND_NCE)
		{
			pval->wp = 1;
			pval->ce = 1;
		}

		if(ctrl & NAND_CLE)
			pval->cle = 1;
		
		if(ctrl & NAND_ALE)
			pval->ale = 1;
		
		value = ~value;
		BL_MIPS_BLOCKS_VPB_NAND_FLASH_CTRL_DEASSERT_WRITE(value);
		value = ~value;
		BL_MIPS_BLOCKS_VPB_NAND_FLASH_CTRL_ASSERT_WRITE(value);
	}

	if(cmd != NAND_CMD_NONE)
		lilac_nand_write_byte(mtd, cmd & 0xff);
}

void lilac_nand_select_chip(struct mtd_info *mtd, int chip)
{
}

static int lilac_nand_dev_ready(struct mtd_info *mtd)
{
	MIPS_BLOCKS_VPB_NAND_FLASH_STATUS_DTE value;

	BL_MIPS_BLOCKS_VPB_NAND_FLASH_STATUS_READ(value);
	return value.flash_busy;
}

int lilac_nand_block_bad(struct mtd_info *mtd, loff_t ofs, int getchip)
{
	struct mtd_oob_ops ops;
	uint16_t mark;
	int i;

	// align offset to the block start
	ofs &= ~(mtd->erasesize - 1);

	// block is good when 2 first bytes on 2 first pages are 0xff
	memset(&ops, 0, sizeof(ops));
	ops.oobbuf = (uint8_t *)&mark;
	ops.ooblen = 2;
	for(i=0; i<2; i++)
	{
		mtd->_read_oob(mtd, ofs, &ops);
		if(mark != 0xffff)
			return 1;
		ofs += mtd->writesize;
	}
	
	return 0;
}

int lilac_nand_block_markbad(struct mtd_info *mtd, loff_t ofs)
{
	struct mtd_oob_ops ops;
	uint16_t mark = 0;
	int i;
	
	// align offset to the block start
	ofs &= ~(mtd->erasesize - 1);

	// mark blocks bad writing 0 to  2 first bytes on 2 first pages
	memset(&ops, 0, sizeof(ops));
	ops.oobbuf = (uint8_t *)&mark;
	ops.ooblen = 2;
	for(i=0; i<2; i++)
	{
		mtd->_write_oob(mtd, ofs, &ops);
		ofs += mtd->writesize;
	}
	
	return 0;
}

int lilac_nand_scan_bbt(struct mtd_info *mtd)
{
	// do nothing as we don't use BBT
	return 0;
}

static void lilac_nand_hwctl(struct mtd_info *mtd, int mode)
{
	// do nothing as we perform ecc calculation in sw
}

int board_nand_init(struct nand_chip *nand)
{
	uint32_t value;
	BL_LILAC_PLL_CLIENT_PROPERTIES props;
	MIPS_BLOCKS_VPB_BRIDGE_NFMC_SPIFC_CONFIG_DTE cfg;

	// set pll and m divider
	bl_lilac_pll_client_properties(PLL_CLIENT_NAND, &props);
	bl_lilac_set_nand_flash_divider(props.pll, props.m);
	
	// enable clock and reset
	bl_lilac_cr_device_enable_clk(BL_LILAC_CR_NAND_FLASH, BL_LILAC_CR_ON);
	bl_lilac_cr_device_reset(BL_LILAC_CR_NAND_FLASH, BL_LILAC_CR_ON);
	
	// pin mux config
	fi_bl_lilac_mux_nand();

	// enable NFMC
	BL_MIPS_BLOCKS_VPB_BRIDGE_NFMC_SPIFC_CONFIG_READ(cfg);
	cfg.nfmc_bypass_en = 1;
	cfg.nfmc_bypass_boot_strap = 1;
	BL_MIPS_BLOCKS_VPB_BRIDGE_NFMC_SPIFC_CONFIG_WRITE(cfg);

	// enter register mode
	value = 2;
	BL_MIPS_BLOCKS_VPB_NAND_FLASH_ACCESS_CONTROL_WRITE(value);

	 nand->cmd_ctrl = lilac_nand_hwcontrol;
	 nand->chip_delay = 50;
	 nand->read_byte = lilac_nand_read_byte;
	 nand->read_buf = lilac_nand_read_buf;
	 nand->dev_ready = lilac_nand_dev_ready;
	 nand->ecc.correct = lilac_nand_correct_data;
	 nand->ecc.hwctl = lilac_nand_hwctl;
	 nand->ecc.calculate = lilac_nand_calculate_ecc;
	 // although ECC is software, we define it as HW to be able to hook ECC calculate function as it is non-standard
	 nand->ecc.mode = NAND_ECC_HW;
	 nand->ecc.size = 512;
	 nand->ecc.bytes = 6;

	 BL_CR_CONTROL_REGS_STRPL_READ(value);
	 // adjust spare area location to free up byte 5 for bad block marker on small sector parts
	 if ((value & (3<<17)) == 0)
		lilac_nand_ecclayout.oobfree[0].offset = 1;

	 nand->ecc.layout = &lilac_nand_ecclayout;
	 nand->select_chip = lilac_nand_select_chip;
	 nand->write_buf  = lilac_nand_write_buf;
	 nand->verify_buf = lilac_nand_verify_buf;
	 nand->scan_bbt = lilac_nand_scan_bbt;
	 //nand->block_markbad = lilac_nand_block_markbad;
	 //nand->block_bad = lilac_nand_block_bad;

	 return 0;
}

void board_nand_clock_down(void)
{
	bl_lilac_cr_device_enable_clk(BL_LILAC_CR_NAND_FLASH, BL_LILAC_CR_OFF);
	bl_lilac_cr_device_reset(BL_LILAC_CR_NAND_FLASH, BL_LILAC_CR_OFF);
}
