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

#ifndef BL_LILAC_OTPM_H_  
#define BL_LILAC_OTPM_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------------------------
        Defines the return code from APIs
-------------------------------------------------*/
#define LILAC_OTPM_RETURN_CODE int32_t

#define CE_OTPM_BUSY                       1
#define CE_OTPM_OUT_OF_RANGE_BIT           2
#define CE_OTPM_NULL_POINTER               3
#define CE_OTPM_FAIL_WRITE                 4
#define CE_OTPM_LOCK_WRITE                 5
#define CE_OTPM_LOCKED                     6

#define CE_USER_LOW         0
#define CE_USER_HIGH        229
#define CE_EJTAG_BIT        230
#define CE_LOCK_USER_BIT    231
#define CE_AES256_BIT       232
#define CE_AES128_BIT       233
#define CE_SHA2_BIT         234
#define CE_SHA1_BIT         235
#define CE_IPSEC_BIT        236
#define CE_RESERVED0_BIT    237
#define CE_RESERVED1_BIT    238
#define CE_LOCK_BL_BIT      239
#define CE_MIPSD_BIT        240
#define CE_VOIP_BIT         241
#define CE_GPON_BIT         242
#define CE_USB_BIT          243
#define CE_PCIE_BIT         244
#define CE_BL_RESERVED_LOW  245
#define CE_BL_RESERVED_HIGH 255

LILAC_OTPM_RETURN_CODE fi_bl_lilac_otpm_init(void);
LILAC_OTPM_RETURN_CODE fi_bl_lilac_read_user_e_fuse  (uint32_t e_bit, uint8_t *read_data );
LILAC_OTPM_RETURN_CODE fi_bl_lilac_read_bl_e_fuse    (uint32_t e_bit, uint8_t *read_data );
LILAC_OTPM_RETURN_CODE fi_bl_lilac_write_user_e_fuse (uint32_t e_bit );
LILAC_OTPM_RETURN_CODE fi_bl_lilac_write_bl_e_fuse   (uint32_t e_bit );
void pi_lilac_lock_e_fuse (void);

#ifdef __cplusplus
}
#endif

#endif 
