/*
    Copyright 2000-2013 Broadcom Corporation
   <:label-BRCM:2012:DUAL/GPL:standard
   
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

#include "bl_lilac_spi.h"
#include "bl_os_wraper.h"
#include "bcmSpiRes.h"

/* SPI transfer flags */
#define	SPI_LSB_FIRST	0x08			/* per-word bits-on-wire */

static BL_SPI_SLAVE_CFG bl_slave_cfg;

/*-----------------------------------------------------------------------
 * Set up communications parameters for a SPI slave.
 *
 *   bus:     Bus ID of the slave chip.
 *   cs:      Chip select ID of the slave chip on the specified bus.
 *   max_hz:  Maximum SCK rate in Hz.
 *   mode:    Clock polarity, clock phase and other parameters.
 * --------------------------------------------------------------------*/
void lilac_spi_setup_slave(unsigned int bus, unsigned int cs, unsigned int max_hz, unsigned int mode)
{
    bl_spi_bus_init(bus);
	
	bl_slave_cfg.sleep_period = 10000;
	bl_slave_cfg.bus = bus;
	bl_slave_cfg.cs = cs;
	bl_slave_cfg.polarity = (mode & SPI_CPOL) ? 1:0;
	bl_slave_cfg.lsb_msb = (mode & SPI_LSB_FIRST) ? 1:0;
	bl_slave_cfg.edge = (mode & SPI_CPHA) ? 1:0;
	bl_slave_cfg.clock = max_hz;
	bl_slave_cfg.handler = NULL;

	bl_spi_slave_init(&bl_slave_cfg);
}

int BcmSpi_MultibitRead( struct spi_transfer *xfer, int busNum, int devId)
{
    bl_claim_bus(&bl_slave_cfg);
    bl_spi_xfer(&bl_slave_cfg, xfer->prepend_cnt, NULL, xfer->tx_buf, BL_SPI_XFER_BEGIN);
    bl_spi_xfer(&bl_slave_cfg, xfer->len, xfer->rx_buf, NULL, BL_SPI_XFER_END);
    bl_release_bus(&bl_slave_cfg);

    return SPI_STATUS_OK;
}

int BcmSpi_Write( const unsigned char *msg_buf, int nbytes, int busNum, int devId, int freqHz )
{
    bl_claim_bus(&bl_slave_cfg);
    bl_spi_xfer(&bl_slave_cfg, nbytes, NULL, msg_buf, BL_SPI_XFER_BEGIN | BL_SPI_XFER_END);
    bl_release_bus(&bl_slave_cfg);

    return SPI_STATUS_OK;
}
