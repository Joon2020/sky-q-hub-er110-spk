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

#ifdef __MIPS_C

#include "bl_lilac_soc.h"
#include "bl_lilac_mips_blocks.h"
#include "bl_lilac_otpm.h"


LILAC_OTPM_RETURN_CODE fi_bl_lilac_otpm_init()
{
	volatile MIPS_BLOCKS_VPB_OTP_TIMER_TPGM_DTE tpgm;
	volatile MIPS_BLOCKS_VPB_BRIDGE_PIN_MUX_IO_CFG_DTE data;
	volatile MIPS_BLOCKS_VPB_OTP_STATUS_DTE status;

	/* set program pulse width */
	tpgm.otp_timer_tpgm = 0x64;
	BL_MIPS_BLOCKS_VPB_OTP_TIMER_TPGM_WRITE(tpgm);

	/* Enable 1.8v for programming */
	data.pri = 1;
	data.sec = 0x1f;
	data.clk = 0;
	data.pe = 1;
	data.ps = 0;
	data.ds = 0;

	BL_MIPS_BLOCKS_VPB_BRIDGE_PIN_MUX_IO_CFG_WRITE_I(data, 0x30);

	/* check if initialization done */
	BL_MIPS_BLOCKS_VPB_OTP_STATUS_READ(status);

	if (status.otp_init_done)
		return BL_LILAC_SOC_OK;
	else
		return CE_OTPM_BUSY;
	
}



/********************************************************
    E-Fuse byte is read by the following sequence:
1.	Check BSY bit is cleared and DONE bit is set 
2.	Write the Byte Address to the ADDR register
3.	Set the READ bit
4.	Poll the CNTRL register until the BSY bit is cleared
5.	Read the value from the DATA register.

********************************************************/
static LILAC_OTPM_RETURN_CODE read_e_fuse(uint32_t e_address, uint8_t *read_data )
{
    volatile MIPS_BLOCKS_VPB_OTP_CNTRL_DTE control;
    volatile MIPS_BLOCKS_VPB_OTP_STATUS_DTE status;
    volatile MIPS_BLOCKS_VPB_OTP_ADDR_DTE addr;
    volatile MIPS_BLOCKS_VPB_OTP_DATA_DTE data;
    volatile MIPS_BLOCKS_VPB_OTP_LOCK_DTE lock;

    *read_data = 0;
    BL_MIPS_BLOCKS_VPB_OTP_LOCK_READ(lock);
	
    if(lock.otp_lock)
        return CE_OTPM_LOCKED;

    BL_MIPS_BLOCKS_VPB_OTP_STATUS_READ(status);
	
    if ((status.otp_init_done) && (!status.otp_bsy)) 
    {
        BL_MIPS_BLOCKS_VPB_OTP_CNTRL_READ(control);
        addr.otp_byte_addr = e_address;
        BL_MIPS_BLOCKS_VPB_OTP_ADDR_WRITE(addr);
        control.otp_rd_wr = 0; /* 0 is read command */
        control.otp_go = 1;
        BL_MIPS_BLOCKS_VPB_OTP_CNTRL_WRITE(control);
        BL_MIPS_BLOCKS_VPB_OTP_STATUS_READ(status);
		
        while (status.otp_bsy) 
        {
            BL_MIPS_BLOCKS_VPB_OTP_STATUS_READ(status);
        }
		
        BL_MIPS_BLOCKS_VPB_OTP_DATA_READ(data);
        *read_data = data.otp_data;

        return BL_LILAC_SOC_OK;
    }

    return CE_OTPM_BUSY;
}
LILAC_OTPM_RETURN_CODE fi_bl_lilac_read_bl_e_fuse(uint32_t e_bit, uint8_t *read_data )
{
    LILAC_OTPM_RETURN_CODE ret;
    uint32_t e_byte, e_bit_of_byte;
    uint8_t data;

    if(!read_data) 
        return CE_OTPM_NULL_POINTER;
	
    if(e_bit < CE_EJTAG_BIT) 
        return CE_OTPM_OUT_OF_RANGE_BIT;

    e_byte = e_bit >> 3 ;      /* /8 */
    e_bit_of_byte = e_bit & 7; /* %8 */

    ret = read_e_fuse (e_byte, &data);
	
    if(ret == BL_LILAC_SOC_OK) 
        *read_data = (data >> e_bit_of_byte) & 1;
    else
        *read_data = 0;

    return ret;
}
LILAC_OTPM_RETURN_CODE fi_bl_lilac_read_user_e_fuse(uint32_t e_bit, uint8_t *read_data )
{
    LILAC_OTPM_RETURN_CODE ret;
    uint32_t e_byte, e_bit_of_byte;
    uint8_t data;

    if(!read_data) 
        return CE_OTPM_NULL_POINTER;
	
    if(e_bit > CE_USER_HIGH) 
        return CE_OTPM_OUT_OF_RANGE_BIT;

    e_byte = e_bit >> 3 ;      /* /8 */
    e_bit_of_byte = e_bit & 7; /* %8 */

    ret = read_e_fuse (e_byte, &data);
	
    if(ret == BL_LILAC_SOC_OK)
        *read_data = (data >> e_bit_of_byte) & 1;
    else
   	 *read_data = 0;

    return ret;
}
/********************************************************
E-Fuse bit is programmed by the following sequence:
1.	Check BSY bit is cleared and DONE bit is set .
2.	Write the Byte Address and Bit Address to the ADDR register
3.	Set the WRITE bit
4.	Poll the CNTRL register until the BSY bit is cleared
5.	Perform a read to validate that programmed has finished successfully

********************************************************/
static LILAC_OTPM_RETURN_CODE write_e_fuse(uint32_t e_address, uint32_t e_bit )
{
    volatile MIPS_BLOCKS_VPB_OTP_CNTRL_DTE control;
    volatile MIPS_BLOCKS_VPB_OTP_STATUS_DTE status;
    volatile MIPS_BLOCKS_VPB_OTP_ADDR_DTE addr;
    volatile MIPS_BLOCKS_VPB_OTP_LOCK_DTE lock;
    uint8_t read_data;
    LILAC_OTPM_RETURN_CODE ret;

    BL_MIPS_BLOCKS_VPB_OTP_LOCK_READ(lock);
    if(lock.otp_lock)
        return CE_OTPM_LOCKED;

    BL_MIPS_BLOCKS_VPB_OTP_STATUS_READ(status);
    if ((status.otp_init_done) && (!status.otp_bsy)) 
    {
        BL_MIPS_BLOCKS_VPB_OTP_CNTRL_READ(control);
        addr.otp_byte_addr = e_address;
	addr.otp_bit_addr = e_bit;

        BL_MIPS_BLOCKS_VPB_OTP_ADDR_WRITE(addr);
        control.otp_rd_wr = 1; /* 1 is write command */
        control.otp_go = 1;
        BL_MIPS_BLOCKS_VPB_OTP_CNTRL_WRITE(control);
        BL_MIPS_BLOCKS_VPB_OTP_STATUS_READ(status);
		
        while (status.otp_bsy && !status.otp_error) 
        {
            BL_MIPS_BLOCKS_VPB_OTP_STATUS_READ(status);
        }

		
        if(status.otp_error)
        {
            // read to clear the error bit
            read_e_fuse(e_address, &read_data);
            ret = CE_OTPM_FAIL_WRITE;
        }
        else
        {
            ret = read_e_fuse(e_address, &read_data); 
            if (ret == BL_LILAC_SOC_OK)
            {
                if((read_data >> e_bit) & 0x1)
                    return BL_LILAC_SOC_OK;
                else
                    return CE_OTPM_FAIL_WRITE;
            }
        }
		
        return ret;


    }

    return CE_OTPM_BUSY;
}
LILAC_OTPM_RETURN_CODE fi_bl_lilac_write_bl_e_fuse(uint32_t e_bit )
{
    uint32_t e_byte, e_bit_of_byte;
    uint8_t data;
    LILAC_OTPM_RETURN_CODE ret;

    if(e_bit < CE_EJTAG_BIT)
        return CE_OTPM_OUT_OF_RANGE_BIT;

    ret = fi_bl_lilac_read_bl_e_fuse(CE_LOCK_BL_BIT, &data);
    if((ret == BL_LILAC_SOC_OK) && data)
            return CE_OTPM_LOCK_WRITE;

    e_byte = e_bit >> 3 ;      /* /8 */
    e_bit_of_byte = e_bit & 7; /* %8 */
	
    return write_e_fuse (e_byte, e_bit_of_byte);
}
LILAC_OTPM_RETURN_CODE fi_bl_lilac_write_user_e_fuse (uint32_t e_bit )
{
    uint32_t e_byte, e_bit_of_byte;
    uint8_t data;
    LILAC_OTPM_RETURN_CODE ret;

    if(e_bit > CE_USER_HIGH) 
        return CE_OTPM_OUT_OF_RANGE_BIT;

    ret = fi_bl_lilac_read_bl_e_fuse (CE_LOCK_USER_BIT, &data);
    if((ret == BL_LILAC_SOC_OK) && data)
            return CE_OTPM_LOCK_WRITE;

    e_byte = e_bit >> 3 ;      /* /8 */
    e_bit_of_byte = e_bit & 7; /* %8 */

	
    return write_e_fuse (e_byte, e_bit_of_byte);
}
void pi_lilac_lock_e_fuse (void)
{
    volatile MIPS_BLOCKS_VPB_OTP_LOCK_DTE lock;

    lock.otp_lock = 1; /* unlock ONLY on reset */
    BL_MIPS_BLOCKS_VPB_OTP_LOCK_WRITE(lock);
}
#endif
