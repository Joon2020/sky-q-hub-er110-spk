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

#ifndef BL_LILAC_BAT_H_  
#define BL_LILAC_BAT_H_

#include "bl_lilac_soc.h"

#ifdef __cplusplus
extern "C"
{
#endif

enum
{
	BL_LILAC_BAT_WRITE_DISABLE = 1,
	BL_LILAC_BAT_READ_DISABLE  = 2
};

typedef struct 
{
	unsigned long src; // original physical address
	unsigned long trg; // translated physical address
	unsigned long size; // size of the block, should be power of 2 and not less then 64k
	unsigned long permissions; // read and/or write disable
} BL_LILAC_BAT_DESCRIPTOR;

/***************************************************************************/
/*     Set translation entry in BAT by index                               */
/* Input                                                                   */
/*  which mips - mipsc or mipsd                                            */
/*  index of entry to set (0..11)                                          */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_bat_set_translation(BL_LILAC_CONTROLLER proc, int index, BL_LILAC_BAT_DESCRIPTOR *desc);

/***************************************************************************/
/*     Return BAT translation entry value by index                         */
/* Input                                                                   */
/*  which mips - mipsc or mipsd                                            */
/*  index of entry to get (0..11)                                          */
/*  translation descriptor to fill                                         */
/***************************************************************************/
void bl_bat_get_translation(BL_LILAC_CONTROLLER proc, int index, BL_LILAC_BAT_DESCRIPTOR *desc);

/***************************************************************************/
/*     Set BAT translation entry permissions by index                      */
/* Input                                                                   */
/*  which mips - mipsc or mipsd                                            */
/*  index of entry to set (0..11)                                          */
/*  permissions to set                                                     */
/* Output                                                                  */
/*  old permissons                                                         */
/***************************************************************************/
unsigned long bl_bat_set_permissions(BL_LILAC_CONTROLLER proc, int index, unsigned long permissions);

/***************************************************************************/
/* Title                                                                   */
/*     Get BAT translation entry permissions by index                      */
/* Input                                                                   */
/*  which mips - mipsc or mipsd                                            */
/*  index of entry to get (0..11)                                          */
/* Output                                                                  */
/*  entry permissons                                                       */
/***************************************************************************/
unsigned long bl_bat_get_permissions(BL_LILAC_CONTROLLER proc, int index);

/***************************************************************************/
/*     Set default permissions - for addresses not maped via BAT           */
/* Input                                                                   */
/*  which mips - mipsc or mipsd                                            */
/*  permissions to set                                                     */
/* Output                                                                  */
/*  old permissons                                                         */
/***************************************************************************/
unsigned long bl_bat_set_default_permissions(BL_LILAC_CONTROLLER proc, unsigned long permissions);

/***************************************************************************/
/*     Get default permissions - for addresses not maped via BAT           */
/* Input                                                                   */
/*  which mips - mipsc or mipsd                                            */
/* Output                                                                  */
/*  default permissons                                                     */
/***************************************************************************/
unsigned long bl_bat_get_default_permissions(BL_LILAC_CONTROLLER proc);

/***************************************************************************/
/*     Check by index if BAT entry is in use                               */
/* Input                                                                   */
/*   which mips - mipsc or mipsd                                            */
/*  index of entry to check (0..11)                                        */
/* Output                                                                  */
/*  0 if free, otherwise is in use                                         */
/***************************************************************************/
int bl_bat_entry_in_use(BL_LILAC_CONTROLLER proc, int index);

/***************************************************************************/
/*     Clear BAT entry by index                                            */
/* Input                                                                   */
/*  which mips - mipsc or mipsd                                            */
/*  index of entry to check (0..11)                                        */
/***************************************************************************/
void bl_bat_entry_clear(BL_LILAC_CONTROLLER proc, int index);

#ifdef __cplusplus
}
#endif

#endif /*BL_LILAC_BAT_H_ */
