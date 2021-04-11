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

#ifndef _BL_LILAC_PCI_H_
#define _BL_LILAC_PCI_H_

#include "bl_lilac_soc.h"

#define BL_PCI_MEM_START_0 	0x10000000
#define BL_PCI_MEM_END_0	0x13feffff
#define BL_PCI_IO_START_0	0x13ff0000
#define BL_PCI_IO_END_0		0x13ffffff

#define BL_PCI_MEM_START_1	0x14000000
#define BL_PCI_MEM_END_1	0x17feffff
#define BL_PCI_IO_START_1	0x17ff0000
#define BL_PCI_IO_END_1		0x17ffffff

// should match interrupt bits definition in hw
typedef enum
{
	PCI_INT_A						= 0x00000001,
	PCI_INT_B						= 0x00000002,
	PCI_INT_C						= 0x00000004,
	PCI_INT_D						= 0x00000008,
	PCI_RADM_PM_TO_ACK				= 0x00000010, 
	PCI_RADM_PM_PME					= 0x00000020,  
	PCI_RADM_FATAL_ERR				= 0x00000040,
	PCI_RADM_NONFATAL_ERR			= 0x00000080,
	PCI_RADM_CORRECTABLE_ERR		= 0x00000100,
	PCI_CFG_SYS_ERR_RC				= 0x00000200,
	PCI_CFG_PME_INT					= 0x00000400,
	PCI_CFG_PME_MSI					= 0x00000800,
	PCI_RADM_CPL_TIMEOUT			= 0x00001000,
	PCI_TRGT_CPL_TIMEOUT			= 0x00002000,
	PCI_CFG_AER_RC_ERR_MSI			= 0x00004000,
	PCI_CFG_AER_RC_ERR_INT			= 0x00008000,
	PCI_RADM_VENDOR_MSG				= 0x00010000,
	PCI_CLK_REQ_N					= 0x00020000,
	PCI_RADMX_CMPOSER_LOOKUP_ERR	= 0x00040000,
	PCI_GM_CMPOSER_LOOKUP_ERR		= 0x00080000,
	PCI_LINK_REQ_RST_NOT			= 0x00100000,
	PCI_WAKE						= 0x00200000,
	PCI_TLH_RFC_UPD					= 0x00400000,
	PCI_HP_MSI						= 0x00800000,
	PCI_HP_INT						= 0x01000000,
	PCI_HP_PME						= 0x02000000,
	PCI_CFG_EML_CONTROL				= 0x04000000,
} BL_LILAC_PCI_INTERRUPTS;

void bl_lilac_pci_init(void);
void bl_lilac_pci_init_lane(int id);

BL_LILAC_SOC_STATUS bl_lilac_pci_rw_config(int id, int write, unsigned int devfn, int where, int size, unsigned long  *val);

void bl_lilac_pci_set_int_mask(int id, BL_LILAC_PCI_INTERRUPTS irq, int mask);
void bl_lilac_pci_ack_int(int id, BL_LILAC_PCI_INTERRUPTS irq);
void bl_lilac_pci_ack_int_mask(int id, unsigned long mask);
unsigned long bl_lilac_pci_get_int_status(int id);

#endif //#ifndef _BL_LILAC_PCI_H_
