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
/* I2C driver for the Philips I2C IP                                       */
/*                                                                         */
/* Title                                                                   */
/*                                                                         */
/*   I2C driver                                                            */
/*                                                                         */
/* Abstract                                                                */
/*                                                                         */
/*   The driver provides API to access I2C                                 */
/*   Provides basic fucntionality like read/write byte in  polling mode    */
/*   Device features:                                                      */
/*   Multi-Master - No                                                     */
/*   Include Interrupt - No                                               */ 
/*   Rx FIFO depth = 16                                                    */ 
/*   Rx FIFO level bus = Register 5                                        */ 
/*   Tx FIFO Depth = 16                                                    */ 
/*   Tx FIFO level bus = Register 6                                        */ 
/*                                                                         */
/***************************************************************************/

#ifndef BL_LILAC_I2C_H_
#define BL_LILAC_I2C_H_

#ifdef __MIPS_C 

#include "bl_lilac_soc.h"

#ifdef __cplusplus
extern "C"
{
#endif
#define I2C_WRITE 0
#define I2C_READ  1


typedef struct
{
    /* rate is set to the desired I2C frequency */
    int32_t rate ;
 
    /* time which takes to tx one byte in microseconds if zero driver will used aiutomatic value */
    uint32_t byte_tx_time ;

    uint32_t byte_delay_time ;

    /* pins used by I2C : -1 means, use the default pins */
    int32_t scl_pin;
    int32_t sda_out_pin;
    int32_t sda_in_pin;

} BL_LILAC_I2C_CONF ;

typedef enum
{
	I2C_OK					=   BL_LILAC_SOC_OK,
	I2C_TX_NOT_READY			,
	I2C_RX_NOT_READY			,
	I2C_NOT_ACK_RCV				,
	I2C_TRANSACTION_ERROR		,
	I2C_TIMEOUT_ERROR			,
	I2C_ARBITRATION_ERROR		,
	I2C_UNKNOWN_ERROR

}BL_LILAC_I2C_STATUS;

typedef enum
{
	I2C_REG_WIDTH_8		=	1,
	I2C_REG_WIDTH_16	=	2,
	I2C_REG_WIDTH_32	=	4

}BL_LILAC_I2C_REG_WIDTH;


/***************************************************************************/
/*     initialize the I2C device. Application should call this function    */ 
/*     before any I2C transaction                                          */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_i2c_init(BL_LILAC_I2C_CONF *conf);

/***************************************************************************/
/*     Write a byte to the device                                          */ 
/***************************************************************************/
BL_LILAC_I2C_STATUS bl_lilac_i2c_write_byte(BL_LILAC_I2C_CONF *xo_conf, uint8_t xi_data, int32_t xi_first, int32_t xi_last);

/***************************************************************************/
/*     Read a byte from the device                                         */ 
/***************************************************************************/
BL_LILAC_I2C_STATUS bl_lilac_i2c_read_byte(BL_LILAC_I2C_CONF *xi_conf, uint8_t *xo_data); 

/***************************************************************************/
/*     Software reset of the device                                        */ 
/***************************************************************************/
void bl_lilac_i2c_sw_reset( BL_LILAC_I2C_CONF *xo_conf ); 

BL_LILAC_I2C_STATUS bl_lilac_i2c_write(BL_LILAC_I2C_CONF* xi_conf, uint8_t slave_id, uint32_t offset, BL_LILAC_I2C_REG_WIDTH offset_width, uint8_t* xo_data, uint32_t data_len);
BL_LILAC_I2C_STATUS bl_lilac_i2c_read(BL_LILAC_I2C_CONF* xi_conf, uint8_t slave_id, uint32_t offset, BL_LILAC_I2C_REG_WIDTH offset_width, uint8_t* xo_data, uint32_t data_len);


#ifdef __cplusplus
}
#endif

#endif /*__MIPS_C */

#endif /* BL_LILAC_I2C_H_ */

