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
/*   Serial Peripheral Interface driver                                    */
/*                                                                         */
/***************************************************************************/

#ifndef BL_LILAC_SPI_H_  
#define BL_LILAC_SPI_H_

#include "bl_lilac_mips_blocks.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum 
{
	BL_SPI_XFER_BEGIN = 1,
	BL_SPI_XFER_END = 2,
} BL_SPI_XFER_FLAGS;

typedef void (*BL_SPI_INT_HANDLER)(void * param);

typedef struct
{
    unsigned int bus; // 0 - SPI flash, 1 - general purpose SPI
    unsigned int cs; // chip select equals GPIO pin number
	BL_SPI_INT_HANDLER handler; // interrupt handler, if NULL-working in polling mode (should be NULL in current version)
    unsigned long clock; // desired operation frequency
    unsigned char polarity;
    unsigned char edge;
    unsigned char lsb_msb;
    unsigned long sleep_period;
	// for driver internal use
	unsigned long addr;
	unsigned long clkcnt; // controller clock devider, calculated according to desired operation frequency
} BL_SPI_SLAVE_CFG;

/***************************************************************************/
/*     Initialize SPI controller                                           */ 
/***************************************************************************/
int bl_spi_bus_init(unsigned int bus);

/***************************************************************************/
/*     Initialize the SPI slave                                           */ 
/***************************************************************************/
int bl_spi_slave_init(BL_SPI_SLAVE_CFG *slave_cfg);

/***************************************************************************/
/*     Claim SPI bus for specified slave transefer                         */ 
/***************************************************************************/
int bl_claim_bus(BL_SPI_SLAVE_CFG *slave_cfg);

/***************************************************************************/
/*     Release SPI bus previously acquired by   bl_claim_bus                */ 
/***************************************************************************/
void bl_release_bus(BL_SPI_SLAVE_CFG *slave_cfg);

/***************************************************************************/
/*     Returns the SPI status register                                     */ 
/***************************************************************************/
void bl_spi_get_errors (BL_SPI_SLAVE_CFG *slave_cfg, unsigned long * const xo_write_collision,
                                          unsigned long * const xo_read_overrun,
                                          unsigned long * const xo_transfer_end,                                          
                                          unsigned long * const xo_mode_fault,
                                          unsigned long * const xo_slave_abort); 

/***************************************************************************/
/*     Activate a SPI chipselect.                                          */ 
/***************************************************************************/
void bl_spi_cs_activate(BL_SPI_SLAVE_CFG *slave_cfg);

/***************************************************************************/
/*     Deactivate a SPI chipselect.                                        */ 
/***************************************************************************/
void bl_spi_cs_deactivate(BL_SPI_SLAVE_CFG *slave_cfg);

/***************************************************************************/
/*     Transfer data to/from SPI device,                                   */ 
/*    writes "bitlen" bits out the SPI MOSI port and simultaneously        */
/*    clocks "len" bytes in the SPI MISO port.  That's just the way SPI    */
/*    works.                                                               */
/* Input                                                                   */
/*    len:	 How many bytes to write and read                              */
/*    dout:	 Pointer to a string of bytes to send out                      */
/*    din:	 Pointer to a string of bytes that will be filled in           */
/*    flags: A bitwise combination of BL_SPI_XFER_FLAGS flags              */
/***************************************************************************/
int bl_spi_xfer(BL_SPI_SLAVE_CFG *slave_cfg, unsigned long len, unsigned char *din, const unsigned char *dout, unsigned long flags);

/***************************************************************************/
/*      Read a byte from SPI without/with CS playing                       */ 
/*     CS should be down/up in user device specific driver      		   */
/*														                   */ 
/***************************************************************************/
int bl_spi_read_byte(BL_SPI_SLAVE_CFG *slave_cfg, unsigned char *data);
int bl_spi_read_byte_cs(BL_SPI_SLAVE_CFG *slave_cfg, unsigned char *data);

/***************************************************************************/
/*     Write a byte to SPI without/with CS playing. 				       */
/*     CS should be down/up in user device specific driver                 */ 
/***************************************************************************/
int bl_spi_write_byte(BL_SPI_SLAVE_CFG *slave_cfg, unsigned char data);
int bl_spi_write_byte_cs(BL_SPI_SLAVE_CFG *slave_cfg, unsigned char data);

#ifdef __cplusplus
}
#endif

#endif /*BL_LILAC_SPI_H_ */
