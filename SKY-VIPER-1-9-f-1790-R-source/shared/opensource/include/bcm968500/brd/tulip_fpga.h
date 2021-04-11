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


#ifndef _TULIP_FPGA_H
#define _TULIP_FPGA_H

#include "bl_lilac_soc.h"

#define TULIP_FPGA_M0LCDR_QSGMII	0x8
#define TULIP_FPGA_M0LCDR_4SGMII	0x9
#define TULIP_FPGA_M0LCDR_2HiSGMII	0xa

#define TULIP_FPGA_M1LCDR_MII		0x4
#define TULIP_FPGA_M1LCDR_RGMII		0x5
#define TULIP_FPGA_M1LCDR_4SS_MII	0x6

#define TULIP_FPGA_CDLCDR_SGMII		0x10
#define TULIP_FPGA_CDLCDR_SFF		0x11
#define TULIP_FPGA_CDLCDR_ONU		0x12

/* FPGA registers */
typedef enum
{
	TULIP_BRD_REV,  /* BRD_REV 8/8 R   0 Board Revision Register */
	TULIP_AS_REV,  	/* AS_REV  8/8 R   1 Assembly Revision Register*/
	TULIP_FPG_REV,  /* FPG_REV 8/8 R   2 FPGA Revision Register*/
	TULIP_DSSR,  	/* DSSR    8/8 R   3 Dip-Switch Status Register*/
	TULIP_M0LCDR,  	/* M0LCDR  8/8 R   4 MAC0 Line Card Detection Register*/
	TULIP_M1LCDR,  	/* M1LCDR  8/8 R   5 MAC1 Line Card Detection Register*/
	TULIP_CDLCDR,  	/* CDLCDR  8/8 R   6 CDR Line Card Detection Register */
	TULIP_LRR,  	/* LRR     8/8 R/W 7 LED Row Register*/
	TULIP_RESET,    /* TBD */
	TULIP_FPGA_REGS_NUM
}TULIP_FPGA_REGISTERS;

#define TULIP_RESET_VALUE  0x55

/* bit #7 (MS bit) of the address indicates access type (1 -write, 0 read). */
#define FPGA_ACCESS_TYPE_BIT     		(1 << 7)
#define SET_FPGA_ACCESS_TYPE_READ(_a)	(_a &= ~FPGA_ACCESS_TYPE_BIT )
#define SET_FPGA_ACCESS_TYPE_WRITE(_a)	(_a |= FPGA_ACCESS_TYPE_BIT  )

BL_LILAC_SOC_STATUS lilac_init_tulip_fpga(void);
BL_LILAC_SOC_STATUS lilac_read_tulip_fpga(unsigned char* inbuf, unsigned char reg);
BL_LILAC_SOC_STATUS lilac_write_tulip_fpga(unsigned char outval, unsigned char reg);
void lilac_tulip_read_fpga_registers(int do_print);
void lilac_tulip_fpga_print_version(void);
void lilac_tulip_fpga_print_connected_line_cards(void);

#endif /* _TULIP_FPGA_H */


