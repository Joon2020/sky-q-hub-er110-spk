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
/* Peripheral Bus Interface                                                */
/*                                                                         */
/* Title                                                                   */
/*                                                                         */
/*   Serial Peripheral Interface driver for Philips Motorola compliant SPI */
/*                                                                         */
/* Abstract                                                                */
/*                                                                         */
/*   The driver provides API to access SPI HDL in Goldenrod chip           */
/*   The data transfer may be done by polling - into a dedicated task      */
/*   OR by polling and interrupt                                           */
/*   the dedicated task is created when SPI starts and it is killed when   */
/*   SPI stops.                                                            */
/*                                                                         */
/***************************************************************************/

#include "bl_os_wraper.h"
#include "bl_lilac_gpio.h"
#include "bl_lilac_spi.h"
#include "bl_lilac_pin_muxing.h"
#include "bl_lilac_mips_blocks.h"
#include "bl_lilac_rst.h"
#include "bl_lilac_soc.h"

int bl_spi_bus_init(unsigned int bus) 
{
	if(bus == 0) // SPI Flash
	{
		int value;
		MIPS_BLOCKS_VPB_BRIDGE_NFMC_SPIFC_CONFIG_DTE cfg;
		BL_LILAC_PLL_CLIENT_PROPERTIES props;
		
		// set pll and m divider
		bl_lilac_pll_client_properties(PLL_CLIENT_SPI_FLASH, &props);
		bl_lilac_set_spi_flash_divider(props.pll, props.m);
		
		// enable clock and reset
		bl_lilac_cr_device_enable_clk(BL_LILAC_CR_SPI_FLASH, BL_LILAC_CR_ON);
		bl_lilac_cr_device_reset(BL_LILAC_CR_SPI_FLASH, BL_LILAC_CR_ON);

		// pin mux config
		fi_bl_lilac_mux_fspi();
		
		// enable SPIFC
		BL_MIPS_BLOCKS_VPB_BRIDGE_NFMC_SPIFC_CONFIG_READ(cfg);
		cfg.spifc_bypass_strap_en = 1;
		cfg.r1 = 1; // should be cfg.spifc_bypass_boot_strap = 1;
		BL_MIPS_BLOCKS_VPB_BRIDGE_NFMC_SPIFC_CONFIG_WRITE(cfg);
		value = 1;
		BL_MIPS_BLOCKS_VPB_BRIDGE_FSPI_STRAP_OVRD_WRITE(value);

		// enable register access mode to SPI flash
		value = 1;
		BL_MIPS_BLOCKS_VPB_SPI_FLASH_SPIFC_ACCESS_CONTROL_WRITE(value);
	}
	else if(bus == 1) // SPI slow bus
	{
		// enable clock and reset
		bl_lilac_cr_device_enable_clk(BL_LILAC_CR_SPI_C, BL_LILAC_CR_ON);
		bl_lilac_cr_device_reset(BL_LILAC_CR_SPI_C, BL_LILAC_CR_ON);
		
		// pin mux config
        fi_bl_lilac_mux_spi();
	}
	else
	{
		// invalid bus number
		return -1;
	}
	
	return 0;
}
EXPORT_SYMBOL(bl_spi_bus_init);

int bl_spi_slave_init(BL_SPI_SLAVE_CFG *slave_cfg) 
{
	// interrupt mode currently is not supported
	if(slave_cfg->handler)
		return -1;
	
	// gpio pin config
	bl_lilac_gpio_write_pin(slave_cfg->cs, 1);
	bl_lilac_gpio_set_mode(slave_cfg->cs, BL_LILAC_GPIO_MODE_OUTPUT);
	// pin mux config
	fi_bl_lilac_mux_connect_pin(slave_cfg->cs,  GPIO, 0, MIPSC);
	
	if(slave_cfg->bus == 0)
	{
		BL_LILAC_PLL_CLIENT_PROPERTIES props;

		slave_cfg->addr = CE_MIPS_BLOCKS_VPB_SPI_FLASH_ADDRESS;
		bl_lilac_pll_client_properties(PLL_CLIENT_SPI_FLASH, &props);
		slave_cfg->clkcnt = (bl_lilac_get_pll_output(props.pll)/(props.m+1) + slave_cfg->clock/2)/slave_cfg->clock;
	}
	else
	{   
        MIPS_BLOCKS_VPB_BRIDGE_SPI_EN_DTE spi;

        spi.spi_en = 1;
        BL_MIPS_BLOCKS_VPB_BRIDGE_SPI_EN_WRITE(spi);
		slave_cfg->addr = CE_MIPS_BLOCKS_VPB_SPI_ADDRESS;
		slave_cfg->clkcnt = (get_lilac_peripheral_clock_rate() + slave_cfg->clock/2)/slave_cfg->clock;
	}

	return 0;
}
EXPORT_SYMBOL(bl_spi_slave_init);

int bl_claim_bus(BL_SPI_SLAVE_CFG *slave_cfg) 
{
    MIPS_BLOCKS_VPB_SPI_SPCR_DTE spcr;
    MIPS_BLOCKS_VPB_SPI_SPCCR_DTE spccr;

    spcr.cpol = slave_cfg->polarity;
    spcr.cpha = slave_cfg->edge;
    spcr.mstr = 1; /* always master */
    spcr.lsbf = slave_cfg->lsb_msb;
    spcr.spie = 0;
    BL_WRITE_32(slave_cfg->addr + CE_MIPS_BLOCKS_VPB_SPI_SPCR_OFFSET, spcr);

    spccr.clkcnt = (slave_cfg->clkcnt < 8) ? 8:slave_cfg->clkcnt; // couldn't be less then 8
    BL_WRITE_32(slave_cfg->addr + CE_MIPS_BLOCKS_VPB_SPI_SPCCR_OFFSET, spccr);

	return 0;
}
EXPORT_SYMBOL(bl_claim_bus);

void bl_release_bus(BL_SPI_SLAVE_CFG *slave_cfg) 
{
	slave_cfg = slave_cfg;
}

EXPORT_SYMBOL(bl_release_bus);

int bl_spi_xfer(BL_SPI_SLAVE_CFG *slave_cfg, unsigned long len, unsigned char *din, const unsigned char *dout, unsigned long flags)
{
	volatile unsigned long value = 0;
    MIPS_BLOCKS_VPB_SPI_FLASH_SPSR_DTE stat;
    unsigned long counter = slave_cfg->sleep_period;
	int res = 0;
	
	if (flags & BL_SPI_XFER_BEGIN)
		bl_spi_cs_activate(slave_cfg);

	while(len--)
	{
		if(dout)
			value = *dout++;
		
		BL_WRITE_32(slave_cfg->addr + CE_MIPS_BLOCKS_VPB_SPI_SPDR_OFFSET, value); 
		do
		{	
			BL_READ_32(slave_cfg->addr + CE_MIPS_BLOCKS_VPB_SPI_SPSR_OFFSET, stat);
			counter--;
		}while((stat.spif != CE_MIPS_BLOCKS_VPB_SPI_SPSR_SPIF_COMPLETE_VALUE) && (counter > 0));

		if(stat.spif != CE_MIPS_BLOCKS_VPB_SPI_SPSR_SPIF_COMPLETE_VALUE)
		{
			res = -1;
			break;
		}
		
		BL_READ_32(slave_cfg->addr + CE_MIPS_BLOCKS_VPB_SPI_SPDR_OFFSET, value);
		
		if(din)
			*din++ = (uint8_t)(value & 0xFF);
	
	}
	
	if (flags & BL_SPI_XFER_END)
		bl_spi_cs_deactivate(slave_cfg);

	return res;
}
EXPORT_SYMBOL(bl_spi_xfer);

void bl_spi_cs_activate(BL_SPI_SLAVE_CFG *slave_cfg)
{
	bl_lilac_gpio_write_pin(slave_cfg->cs, 0);
}

void bl_spi_cs_deactivate(BL_SPI_SLAVE_CFG *slave_cfg)
{
	bl_lilac_gpio_write_pin(slave_cfg->cs, 1);
}

void bl_spi_get_errors(BL_SPI_SLAVE_CFG *slave_cfg, unsigned long * const xo_write_collision,
                                          unsigned long * const xo_read_overrun,
                                          unsigned long * const xo_transfer_end,                                          
                                          unsigned long * const xo_mode_fault,
                                          unsigned long * const xo_slave_abort) 
{
    MIPS_BLOCKS_VPB_SPI_SPSR_DTE spsr;

    BL_READ_32(slave_cfg->addr + CE_MIPS_BLOCKS_VPB_SPI_SPSR_OFFSET, spsr);

    *xo_write_collision = spsr.wcol;
    *xo_read_overrun = spsr.rovr;  
    *xo_transfer_end = spsr.spif;   
    *xo_mode_fault = spsr.modf;     
    *xo_slave_abort = spsr.abrt;
}
EXPORT_SYMBOL(bl_spi_get_errors);

int bl_spi_write_byte(BL_SPI_SLAVE_CFG *slave_cfg, unsigned char data) 
{
	return bl_spi_xfer(slave_cfg, 1, 0, &data, 0);
}
EXPORT_SYMBOL(bl_spi_write_byte);

int bl_spi_write_byte_cs(BL_SPI_SLAVE_CFG *slave_cfg, unsigned char data) 
{
	return bl_spi_xfer(slave_cfg, 1, 0, &data, BL_SPI_XFER_BEGIN | BL_SPI_XFER_END);
}
EXPORT_SYMBOL(bl_spi_write_byte_cs);

int bl_spi_read_byte( BL_SPI_SLAVE_CFG *slave_cfg, unsigned char *data) 
{
    if(!data)
    {
        return -1;
    }
	
	return bl_spi_xfer(slave_cfg, 1, data, 0, 0);
}
EXPORT_SYMBOL(bl_spi_read_byte);

int bl_spi_read_byte_cs(BL_SPI_SLAVE_CFG *slave_cfg, unsigned char *data) 
{
    if(!data)
    {
        return -1;
    }
	
	return bl_spi_xfer(slave_cfg, 1, data, 0, BL_SPI_XFER_BEGIN | BL_SPI_XFER_END);
}
EXPORT_SYMBOL(bl_spi_read_byte_cs);

