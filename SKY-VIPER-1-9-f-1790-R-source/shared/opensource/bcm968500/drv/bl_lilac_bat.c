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
/*   BAT block driver                                    				   */
/*                                                                         */
/***************************************************************************/

#include "bl_os_wraper.h"
#include "bl_lilac_bat.h"
#include "bl_lilac_mips_blocks.h"

// there are 12 entries in BAT - indices 0..11
#define BL_BAT_VALID_INDEX(index) ((index >= 0) && (index <= 11))

#define log2(num)											\
({ int res;													\
		__asm__ __volatile__(								\
			"clz\t%0, %1\n\t"								\
			: "=r" (res) : "Jr" ((unsigned int)(num)));		\
	31-res;													\
})

BL_LILAC_SOC_STATUS bl_bat_set_translation(BL_LILAC_CONTROLLER proc, int index, BL_LILAC_BAT_DESCRIPTOR *desc)
{
	unsigned long flags;
	unsigned long val;
	
	BL_ASSERT(BL_BAT_VALID_INDEX(index));
	BL_ASSERT((proc == MIPSC) || (proc == MIPSD));

	// memory block size should be power of 2 and not less then 64K
	// original and translated addresses should be 64K aligned
	if((desc->size < 0x10000) || ((1<<log2(desc->size)) != desc->size) ||
		(desc->src & 0xffff) || (desc->trg & 0xffff))
		return BL_LILAC_SOC_INVALID_PARAM;
	
	BL_LOCK_SECTION(flags);
	
	BL_MIPS_BLOCKS_VPB_BAT_RW_WRITE(proc, index, desc->permissions);
	val = 0x10000 - (desc->size >> 16);
	BL_MIPS_BLOCKS_VPB_BAT_SIZE_MSK_WRITE(proc, index, val);
	val = desc->trg >> 16;
	BL_MIPS_BLOCKS_VPB_BAT_TRG_ADDR_WRITE(proc, index, val);
	val = desc->src >> 16;
	BL_MIPS_BLOCKS_VPB_BAT_SRC_ADDR_WRITE(proc, index, val);
	
	BL_UNLOCK_SECTION(flags);
	
	return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_bat_set_translation);

void bl_bat_get_translation(BL_LILAC_CONTROLLER proc, int index, BL_LILAC_BAT_DESCRIPTOR *desc)
{
	unsigned long flags;

	BL_ASSERT(BL_BAT_VALID_INDEX(index));
	BL_ASSERT((proc == MIPSC) || (proc == MIPSD));
	
	BL_LOCK_SECTION(flags);
	
	BL_MIPS_BLOCKS_VPB_BAT_RW_READ(proc, index, desc->permissions);
	BL_MIPS_BLOCKS_VPB_BAT_SIZE_MSK_READ(proc, index, desc->size);
	BL_MIPS_BLOCKS_VPB_BAT_TRG_ADDR_READ(proc, index, desc->trg);
	BL_MIPS_BLOCKS_VPB_BAT_SRC_ADDR_READ(proc, index, desc->src);
	
	BL_UNLOCK_SECTION(flags);
	
	desc->size = (0x10000 - desc->size) << 16;
	desc->trg = desc->trg << 16;
	desc->src = desc->src <<16;
}
EXPORT_SYMBOL(bl_bat_get_translation);

unsigned long bl_bat_set_permissions(BL_LILAC_CONTROLLER proc, int index, unsigned long permissions)
{
	unsigned long flags;
	unsigned long old_permissions;
	
	BL_ASSERT(BL_BAT_VALID_INDEX(index));
	BL_ASSERT((proc == MIPSC) || (proc == MIPSD));

	BL_LOCK_SECTION(flags);
	
	BL_MIPS_BLOCKS_VPB_BAT_RW_READ(proc, index, old_permissions);
	BL_MIPS_BLOCKS_VPB_BAT_RW_WRITE(proc, index, permissions);
	
	BL_UNLOCK_SECTION(flags);

	return old_permissions;
}
EXPORT_SYMBOL(bl_bat_set_permissions);

unsigned long bl_bat_get_permissions(BL_LILAC_CONTROLLER proc, int index)
{
	unsigned long permissions;

	BL_ASSERT(BL_BAT_VALID_INDEX(index));
	BL_ASSERT((proc == MIPSC) || (proc == MIPSD));

	BL_MIPS_BLOCKS_VPB_BAT_RW_READ(proc, index, permissions);
	
	return permissions;
}
EXPORT_SYMBOL(bl_bat_get_permissions);

unsigned long bl_bat_set_default_permissions(BL_LILAC_CONTROLLER proc, unsigned long permissions)
{
	unsigned long flags;
	unsigned long old_permissions;
	
	BL_LOCK_SECTION(flags);
	BL_ASSERT((proc == MIPSC) || (proc == MIPSD));
	
	BL_MIPS_BLOCKS_VPB_BAT_RW_DEFAULT_READ(proc, old_permissions);
	BL_MIPS_BLOCKS_VPB_BAT_RW_DEFAULT_WRITE(proc, permissions);
	
	BL_UNLOCK_SECTION(flags);

	return old_permissions;
}
EXPORT_SYMBOL(bl_bat_set_default_permissions);

unsigned long bl_bat_get_default_permissions(BL_LILAC_CONTROLLER proc)
{
	unsigned long permissions;

	BL_ASSERT((proc == MIPSC) || (proc == MIPSD));

	BL_MIPS_BLOCKS_VPB_BAT_RW_DEFAULT_READ(proc, permissions);
	
	return permissions;
}
EXPORT_SYMBOL(bl_bat_get_default_permissions);

int bl_bat_entry_in_use(BL_LILAC_CONTROLLER proc, int index)
{
	unsigned long src;
	
	BL_ASSERT(BL_BAT_VALID_INDEX(index));
	BL_ASSERT((proc == MIPSC) || (proc == MIPSD));

	BL_MIPS_BLOCKS_VPB_BAT_SRC_ADDR_READ(proc, index, src);
	
	return src == 0xffff ? 0:1;
}
EXPORT_SYMBOL(bl_bat_entry_in_use);

void bl_bat_entry_clear(BL_LILAC_CONTROLLER proc, int index)
{
	unsigned long src = 0xffff;
	
	BL_ASSERT(BL_BAT_VALID_INDEX(index));
	BL_ASSERT((proc == MIPSC) || (proc == MIPSD));

	BL_MIPS_BLOCKS_VPB_BAT_SRC_ADDR_WRITE(proc, index, src);
}
EXPORT_SYMBOL(bl_bat_entry_clear);
