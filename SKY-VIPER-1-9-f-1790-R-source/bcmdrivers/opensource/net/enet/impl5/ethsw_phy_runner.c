/*
 <:copyright-BRCM:2011:DUAL/GPL:standard
 
    Copyright (c) 2011 Broadcom Corporation
    All Rights Reserved
 
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

#include "bcmenet_common.h"
#if defined( CONFIG_BCM968500)
	#include "bl_lilac_eth_common.h"
	#define ReadPHYReg(_phy_id, _reg) bl_lilac_ReadPHYReg(_phy_id, _reg)
	#define WritePHYReg(_phy_id, _reg,_data) bl_lilac_WritePHYReg(_phy_id, _reg, (int)_data)

#elif defined( CONFIG_BCM96838)
	#include "phys_common_drv.h"
	#define ReadPHYReg(_phy_id, _reg) PhyReadRegister(ext_bit ? _phy_id|PHY_EXTERNAL :_phy_id, _reg)
	#define WritePHYReg(_phy_id, _reg,_data) PhyWriteRegister(ext_bit ? _phy_id|PHY_EXTERNAL :_phy_id, _reg, _data)

#endif


void ethsw_phy_read_reg(int phy_id, int reg, uint16 *data, int ext_bit)
{
   int status = ReadPHYReg(phy_id, reg);
   if (status >= 0) 
   {
      *data = (uint16)status;
   }
   else
   {
      printk("ERROR : ethsw_phy_read_reg(%d, %d, **)\n",phy_id,reg);
   }
}

void ethsw_phy_write_reg(int phy_id, int reg, uint16 *data, int ext_bit)
{
   int status = WritePHYReg(phy_id, reg, *data);
   if (status < 0) 
   {
      printk("ERROR : ethsw_phy_wreg(%d, %d, 0x%04x)\n",phy_id,reg,*data);
   }
}

MODULE_LICENSE("GPL");

