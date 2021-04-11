/*
   Copyright (c) 2013 Broadcom Corporation
   All Rights Reserved

    <:label-BRCM:2013:DUAL/GPL:standard
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2, as published by
    the Free Software Foundation (the "GPL").
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    
    A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
    writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
    
    :> 
*/             

#ifndef __968500_RDP_MAP_H
#define __968500_RDP_MAP_H


/*****************************************************************************************/
/* BBH Blocks offsets                                                                        */
/*****************************************************************************************/
#define BBH_TX_0_OFFSET				( 0x1909B000 )
#define BBH_TX_1_OFFSET				( 0x1909B800 )
#define BBH_TX_2_OFFSET				( 0x1909C000 )
#define BBH_TX_3_OFFSET				( 0x1909C800 )
#define BBH_TX_4_OFFSET				( 0x1909D000 )
#define BBH_TX_5_OFFSET				( 0x1909D800 )
#define BBH_TX_6_OFFSET				( 0x1909E000 )
#define BBH_RX_0_OFFSET				( 0x190A1800 )
#define BBH_RX_1_OFFSET				( 0x190A2000 )
#define BBH_RX_2_OFFSET				( 0x190A2800 )
#define BBH_RX_3_OFFSET				( 0x190A3000 )
#define BBH_RX_4_OFFSET				( 0x190A3800 )
#define BBH_RX_5_OFFSET				( 0x190A4000 )
#define BBH_RX_6_OFFSET				( 0x190A4800 )
/*****************************************************************************************/
/* BPM Blocks offsets                                                                        */
/*****************************************************************************************/
#define BPM_MODULE_OFFSET			( 0x19080000 )
/*****************************************************************************************/
/* SBPM Blocks offsets                                                                        */
/*****************************************************************************************/
#define SBPM_BLOCK_OFFSET			( 0x19084000 )
/*****************************************************************************************/
/* DMA Blocks offsets                                                                        */
/*****************************************************************************************/
#define DMA_REGS_0_OFFSET			( 0x190A5800 )
#define DMA_REGS_1_OFFSET			( 0x190A5C00 )
/*****************************************************************************************/
/* GPON Blocks offsets                                                                        */
/*****************************************************************************************/
#define GPON_RX_OFFSET				( 0x19096000 )
#define GPON_TX_OFFSET				( 0x19097000 )
/*****************************************************************************************/
/* IH Blocks offsets                                                                        */
/*****************************************************************************************/
#define IH_REGS_OFFSET				( 0x19098000 )
/*****************************************************************************************/
/* MS1588 Blocks offsets                                                                        */
/*****************************************************************************************/
#define MS1588_SLAVE_OFFSET			( 0x190A8C00 )
#define MS1588_MASTER_OFFSET		( 0x190A8D00 )
#define MS1588_LOCAL_TS_OFFSET		( 0x190A8E00 )
/*****************************************************************************************/
/* PSRAM Blocks offsets                                                                        */
/*****************************************************************************************/
#define PSRAM_BLOCK_OFFSET			( 0x18100000 )
/*****************************************************************************************/
/* RUNNER Blocks offsets                                                                        */
/*****************************************************************************************/
#define RUNNER_COMMON_0_OFFSET		( 0x19000000 )
#define RUNNER_COMMON_1_OFFSET		( 0x19040000 )
#define RUNNER_PRIVATE_0_OFFSET		( 0x19010000 )
#define RUNNER_PRIVATE_1_OFFSET		( 0x19050000 )
#define RUNNER_INST_MAIN_0_OFFSET	( 0x19020000 )
#define RUNNER_INST_MAIN_1_OFFSET	( 0x19060000 )
#define RUNNER_CNTXT_MAIN_0_OFFSET	( 0x19028000 )
#define RUNNER_CNTXT_MAIN_1_OFFSET	( 0x19068000 )
#define RUNNER_PRED_MAIN_0_OFFSET	( 0x1902C000 )
#define RUNNER_PRED_MAIN_1_OFFSET	( 0x1906C000 )
#define RUNNER_INST_PICO_0_OFFSET	( 0x19030000 )
#define RUNNER_INST_PICO_1_OFFSET	( 0x19070000 )
#define RUNNER_CNTXT_PICO_0_OFFSET	( 0x19038000 )
#define RUNNER_CNTXT_PICO_1_OFFSET	( 0x19078000 )
#define RUNNER_PRED_PICO_0_OFFSET	( 0x1903C000 )
#define RUNNER_PRED_PICO_1_OFFSET	( 0x1907C000 )
#define RUNNER_REGS_0_OFFSET		( 0x19099000 )
#define RUNNER_REGS_1_OFFSET		( 0x1909A000 )  

#endif
