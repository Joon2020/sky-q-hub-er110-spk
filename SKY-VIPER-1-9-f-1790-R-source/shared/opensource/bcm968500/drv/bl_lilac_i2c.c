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
/*   The driver provides API to access Philips IP I2C                      */
/*   Provides basic fucntionality like read/write in polling and interrupt */
/*   mode. driver does not make any assumptions about underlying OS.       */
/*   in the interrupt mode driver API requires from the application to     */
/*   waitSignal, sendSignal, connectInterrupt functionality                */
/*   in the polling mode Application is expected to spawan a task calling  */
/*   poll function.                                                        */
/*   Device features:                                                      */
/*   Multi-Master - No                                                     */ 
/*   Include Interrupt - Yes                                               */ 
/*   Min. Serial Freq= 100 (100 KHz)                                       */ 
/*   Rx FIFO depth = 16                                                    */ 
/*   Rx FIFO level bus = Register 5                                        */ 
/*   Tx FIFO Depth = 16                                                    */ 
/*   Tx FIFO level bus = Register 6                                        */ 
/*   in the polling mode because of the limited size of the FIFO           */
/*   Application is exptected to run tight loops                           */
/*                                                                         */
/***************************************************************************/

#ifdef __MIPS_C  /* I2C registers can be reached by MIPS_C only*/
#include "bl_lilac_soc.h"
#include "bl_os_wraper.h"
#include "bl_lilac_rst.h"
#include "bl_lilac_i2c.h"
#include "bl_lilac_mips_blocks.h"
#include "bl_lilac_pin_muxing.h"

static BL_LILAC_I2C_STATUS bl_lilac_i2c_wait_tx_ready ( BL_LILAC_I2C_CONF *xo_conf);
static BL_LILAC_I2C_STATUS bl_lilac_i2c_wait_rx_ready ( BL_LILAC_I2C_CONF *xo_conf);
static BL_LILAC_I2C_STATUS bl_lilac_i2c_wait_status   ( BL_LILAC_I2C_CONF *xo_conf);

/***************************************************************************/
/*     initialize the I2C driver. Application should call this function    */ 
/*     before any other function in the driver                             */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_i2c_init(BL_LILAC_I2C_CONF *conf)
{
	volatile MIPS_BLOCKS_VPB_I2C_CLKHI_DTE divider_high;
	volatile MIPS_BLOCKS_VPB_I2C_CLKLO_DTE divider_low;
    unsigned int setting;
    unsigned int periph;
    LILAC_PIN_MUXING_RETURN_CODE retcode;

    /* peripheral clock is in Hz
       number of clocks in an i2c clock =
       peripheral_clock / desired_period(it is in KHz)
    */

    periph = get_lilac_peripheral_clock_rate();
    setting = periph / (conf->rate * 1000);

    if (!setting || ((setting/2) >= 512) )
    {
    	printk("I2C parameter error\n");
    	return BL_LILAC_SOC_INVALID_PARAM;
    }
	/* set divider SCL high - take half of clocks */
    BL_ZERO_REG(&divider_high);
	divider_high.clkdivhi = setting/2;
	BL_MIPS_BLOCKS_VPB_I2C_CLKHI_WRITE(divider_high);

	/* set divider SCL low - take half of clocks */
	BL_ZERO_REG(&divider_low);
	divider_low.clkdivlo = setting/2;
	BL_MIPS_BLOCKS_VPB_I2C_CLKLO_WRITE(divider_low);


	/* tx time of a single bit is one i2c clock 
        = desired clock/2, in sec )
        we need it in ns
    */
	if(conf->byte_tx_time == 0 )
		conf->byte_tx_time = 8 * (ONE_SEC_IN_NANO / setting)*2;

	/* set the byte delay time to 2 clock cycles */
	conf->byte_delay_time = 2000/(conf->rate);

    /* set muxing */
    if(conf->scl_pin == -1)
        conf->scl_pin = IIC_SCL;
	
    retcode = fi_bl_lilac_mux_connect_pin(conf->scl_pin, I2C_SCL_OUT, 0, MIPSC);
    if(retcode != BL_LILAC_SOC_OK)
        return retcode;

    if(conf->sda_out_pin == -1)
        conf->sda_out_pin = IIC_SDA;
	
    retcode = fi_bl_lilac_mux_connect_pin(conf->sda_out_pin, I2C_SDA_OUT, 0, MIPSC);
    if(retcode != BL_LILAC_SOC_OK)
        return retcode;

    if(conf->sda_in_pin == -1)
        conf->sda_in_pin = IIC_SDA;

    retcode = fi_bl_lilac_mux_connect_pin(conf->sda_in_pin, I2C_SDA_IN, 0, MIPSC);
	if(retcode == BL_LILAC_SOC_OK)
    {
        retcode = bl_lilac_cr_device_reset(BL_LILAC_CR_I2C_C, BL_LILAC_CR_ON);
    }
    /* take out of reset */
    return retcode;
}

EXPORT_SYMBOL( bl_lilac_i2c_init );

/***************************************************************************/
/*     Wait for Rx ready status bit                                        */ 
/* Output                                                                  */
/*     return 1 if Ok, return 0 if device remains busy for too long        */
/***************************************************************************/
static BL_LILAC_I2C_STATUS bl_lilac_i2c_wait_rx_ready ( BL_LILAC_I2C_CONF *xo_conf )
{
	volatile MIPS_BLOCKS_VPB_I2C_STS_DTE status_reg ;
	int32_t count;
    
	for (count = xo_conf->byte_tx_time; count > 0; count--)
	{
		BL_MIPS_BLOCKS_VPB_I2C_STS_READ(status_reg);
		if( status_reg.rfe != CE_MIPS_BLOCKS_VPB_I2C_STS_RFE_EMPTY_VALUE )
			return I2C_OK;
	}
	return I2C_RX_NOT_READY;
}


/***************************************************************************/
/*     Wait for Tx ready status bit                                        */ 
/* Output                                                                  */
/*     return 1 if Ok, return 0 if device remains busy for too long        */
/***************************************************************************/
static BL_LILAC_I2C_STATUS bl_lilac_i2c_wait_tx_ready (BL_LILAC_I2C_CONF *xo_conf)
{
	volatile MIPS_BLOCKS_VPB_I2C_STS_DTE status_reg ;
	uint32_t count ;
    
	for(count = xo_conf->byte_tx_time; count > 0; count--)
	{
		BL_MIPS_BLOCKS_VPB_I2C_STS_READ(status_reg);
		if (status_reg.tfe == CE_MIPS_BLOCKS_VPB_I2C_STS_TFE_EMPTY_VALUE)
			return I2C_OK;
	}
	return I2C_TX_NOT_READY;
}


/***************************************************************************/
/*     Check No Acknowledge (NAI) status bit                               */ 
/* Output                                                                  */
/*     return I2C_OK if Ok,                                                */
/*	  return error code otherwise                                          */
/***************************************************************************/
static BL_LILAC_I2C_STATUS bl_lilac_i2c_wait_status (BL_LILAC_I2C_CONF *xo_conf)
{
	volatile MIPS_BLOCKS_VPB_I2C_STS_DTE status_reg;
	int32_t count;

	for (count = xo_conf->byte_tx_time; count > 0; count--)
	{
		/* read I2C status register */
		BL_MIPS_BLOCKS_VPB_I2C_STS_READ(status_reg);

		if(status_reg.drmi == CE_MIPS_BLOCKS_VPB_I2C_STS_DRMI_NEEDDATA_VALUE)
		{
			udelay(xo_conf->byte_delay_time);
			/* I2C master request next byte for send */
			/* correct, we can continue, DRMI bit will be cleaned by writing next byte */
			return I2C_OK;
		}
		if(status_reg.tdi == CE_MIPS_BLOCKS_VPB_I2C_STS_TDI_TRANSCOMPLETE_VALUE)
		{
			/* there is error: STOP condition was sent because error on bus */
			/* clear TDI bit */
			status_reg.tdi = 1;
			BL_MIPS_BLOCKS_VPB_I2C_STS_WRITE(status_reg);
			/* Check cause for STOP condition */
			if(status_reg.nai == CE_MIPS_BLOCKS_VPB_I2C_STS_NAI_NOACK_VALUE) 
			{
				/* ACK sygnal was not received */
				return I2C_NOT_ACK_RCV;
			}
			
			if(status_reg.afi != CE_MIPS_BLOCKS_VPB_I2C_STS_AFI_DEFAULT_VALUE)
			{
				/* Arbitration failure */
				return I2C_ARBITRATION_ERROR;
			}
		}
	}

	/* there is a timeot of unknown reason */
	return I2C_TIMEOUT_ERROR;
}

/***************************************************************************/
/*     Wait for TDI (transaction done) bit to be set                       */ 
/* Output                                                                  */
/*     return 1 if Ok, return 0 if device remains busy for too long        */
/***************************************************************************/
static BL_LILAC_I2C_STATUS bl_lilac_i2c_wait_done (BL_LILAC_I2C_CONF *xo_conf)
{
	volatile MIPS_BLOCKS_VPB_I2C_STS_DTE status_reg ;
	int32_t count ;

	for (count = xo_conf->byte_tx_time; count > 0; count--)
	{
		/* read I2C status register */
		BL_MIPS_BLOCKS_VPB_I2C_STS_READ (status_reg);
		if ( status_reg.tdi == CE_MIPS_BLOCKS_VPB_I2C_STS_TDI_TRANSCOMPLETE_VALUE )
		{
			/* STOP condition was sent */
			/* clear TDI bit */
			status_reg.tdi = 1;
			BL_MIPS_BLOCKS_VPB_I2C_STS_WRITE (status_reg);
			/* Check status of last byte sending */
			if(status_reg.nai == CE_MIPS_BLOCKS_VPB_I2C_STS_NAI_NOACK_VALUE) 
			{
				/* ACK sygnal was not receive */
				return I2C_NOT_ACK_RCV;
			}
			if(status_reg.afi != CE_MIPS_BLOCKS_VPB_I2C_STS_AFI_DEFAULT_VALUE) 
			{
				/* Arbitration failure */
				return I2C_ARBITRATION_ERROR;
			}
			else 
			{
				udelay(xo_conf->byte_delay_time);
				/* transaction was finished successfully */
				return I2C_OK;
			}
		}
	}

    /* we have not got TDI status - there is a timeot of unknown reason */
    return I2C_TIMEOUT_ERROR;
}

/***************************************************************************/
/*     Write a byte to the device                                          */
/* Output                                                                  */
/*     return 1 if Ok, return 0 if device remains busy for too long        */
/***************************************************************************/
BL_LILAC_I2C_STATUS bl_lilac_i2c_write_byte(BL_LILAC_I2C_CONF *xo_conf, uint8_t xi_data, int32_t xi_first, int32_t xi_last)
{
	BL_LILAC_I2C_STATUS result;
    volatile MIPS_BLOCKS_VPB_I2C_FIFODATA_DTE fifo_data;

    result = bl_lilac_i2c_wait_tx_ready(xo_conf);
    if ( result != I2C_OK )
          return result;

    fifo_data.data = xi_data;

    /* first byte */
    if(xi_first != 0)
    {
         fifo_data.startcond = CE_MIPS_BLOCKS_VPB_I2C_FIFODATA_STARTCOND_START_VALUE;
    }
    else
    {
         fifo_data.startcond = CE_MIPS_BLOCKS_VPB_I2C_FIFODATA_STARTCOND_NO_START_VALUE;
    }

    /* last byte */
    if(xi_last != 0)
    {
         fifo_data.stopcond = CE_MIPS_BLOCKS_VPB_I2C_FIFODATA_STOPCOND_STOP_VALUE;
    }
    else
    {
         fifo_data.stopcond = CE_MIPS_BLOCKS_VPB_I2C_FIFODATA_STOPCOND_NO_STOP_VALUE;
    }
      
    BL_MIPS_BLOCKS_VPB_I2C_FIFODATA_WRITE(fifo_data);
      
    result = bl_lilac_i2c_wait_tx_ready(xo_conf);
    if(result != I2C_OK)
          return result;

    /* this is a last byte in the transaction - check TDI bit in the Status Reg */
    if( xi_last )
         result = bl_lilac_i2c_wait_done(xo_conf);
    else
         result = bl_lilac_i2c_wait_status(xo_conf);

   return result;

}
EXPORT_SYMBOL(bl_lilac_i2c_write_byte);


/***************************************************************************/
/*      Read a byte                                                        */
/*                                                                         */
/* Output                                                                  */
/*     return 1 if Ok, return 0 if device remains busy for too long        */
/***************************************************************************/
BL_LILAC_I2C_STATUS bl_lilac_i2c_read_byte(BL_LILAC_I2C_CONF * xi_conf, uint8_t * xo_data)
{
	BL_LILAC_I2C_STATUS result;
	volatile MIPS_BLOCKS_VPB_I2C_FIFODATA_DTE fifo_data ;

	result = bl_lilac_i2c_wait_rx_ready(xi_conf);
	if( result != I2C_OK)
		return result;

	BL_MIPS_BLOCKS_VPB_I2C_FIFODATA_READ(fifo_data);

    /* store the byte in the buffer */
    * xo_data = (uint8_t)fifo_data.data;

	return result ;
}
EXPORT_SYMBOL(bl_lilac_i2c_read_byte);

/***************************************************************************/
/*     Software reset of the device                                        */ 
/* Output                                                                  */
/*     return 1 if Ok, return 0 if fails                                   */
/***************************************************************************/
void bl_lilac_i2c_sw_reset(BL_LILAC_I2C_CONF *xo_conf)
{
	volatile MIPS_BLOCKS_VPB_I2C_CTL_DTE ctr_reg;

	/* write SW reset bit */
	BL_MIPS_BLOCKS_VPB_I2C_CTL_READ(ctr_reg);

	ctr_reg.srst	= 1;
	ctr_reg.rfdaie	= 1;

	BL_MIPS_BLOCKS_VPB_I2C_CTL_WRITE(ctr_reg);

	/* wait in tight loop for self clearing */
	while(1)
	{
		BL_MIPS_BLOCKS_VPB_I2C_CTL_READ(ctr_reg);
		if(ctr_reg.srst == 0)
		{
			break;
		}
	}
}

EXPORT_SYMBOL( bl_lilac_i2c_sw_reset );


/******************************************************************************************/
/** bl_lilac_i2c_write	                                                                ***/
/** The function writes data to address/command.					***/
/** INPUT:										***/
/**	xi_conf      - I2C configuration							***/
/**	slave_id     - slave device address						***/
/**	offset       - The address (command/register) to write to				***/
/**	offset_width - command/register width (8,16,32 bit)				***/
/**	xo_data      - buffer of data to be written					***/
/**	data_len     - number of data bytes to write					***/
/******************************************************************************************/
BL_LILAC_I2C_STATUS bl_lilac_i2c_write(BL_LILAC_I2C_CONF* xi_conf, uint8_t slave_id, uint32_t offset, BL_LILAC_I2C_REG_WIDTH offset_width, uint8_t* xo_data, uint32_t data_len)
{
    int index, start_index = 0;
    BL_LILAC_I2C_STATUS result = I2C_OK;
    uint32_t off = offset;
    uint8_t* off_ptr = (uint8_t*)&off;


    if ( (data_len) == 0  || (xo_data==NULL)  )
        return I2C_UNKNOWN_ERROR;


    /* We send the most significant byte first, so the bytes we send depends on
    the width of the register address. Note that the offset variable is 32 bit,
    but the register address may be 8, 16, 32 bit. */
    if (offset_width == I2C_REG_WIDTH_8)
    	start_index = 3;
    else if (offset_width == I2C_REG_WIDTH_16)
    	start_index = 2;
    else if (offset_width == I2C_REG_WIDTH_32)
    	start_index = 0;
    else
	return I2C_UNKNOWN_ERROR;


    /* write the slave address and write operation */
    result = bl_lilac_i2c_write_byte( xi_conf, ( slave_id & 0xFE ), 1, 0 );
    if(result != I2C_OK)
		goto out;


    /* write the command/register to write to */
    for (index = start_index; index < 4; index++)
    {
	result = bl_lilac_i2c_write_byte( xi_conf, off_ptr[index], 0, 0 );
	if(result != I2C_OK)
		goto out;
    }


    /* write data */
    for(index = 0 ; index < (data_len - 1) ; index ++ )
    {
        result = bl_lilac_i2c_write_byte( xi_conf, xo_data[index], 0, 0 );
        if(result != I2C_OK)
		goto out;
    }

    /* write last data (with stop bit) */
    result = bl_lilac_i2c_write_byte( xi_conf, xo_data[index], 0, 1 );

out:
    bl_lilac_i2c_sw_reset(xi_conf);
    return  result;

}
EXPORT_SYMBOL(bl_lilac_i2c_write);

/******************************************************************************************/
/** bl_lilac_i2c_read           	                                                ***/
/** The function reads data								***/
/** INPUT:										***/
/**	xi_conf      - I2C configuration							***/
/**	slave_id     - slave device address						***/
/**	offset       - The address (command/register) to read from			***/
/**	offset_width - command/register width (8,16,32 bit)				***/
/**	xo_data      - buffer of data to read to.						***/
/**	data_len     - number of data bytes to read (maximum 4)				***/
/******************************************************************************************/
BL_LILAC_I2C_STATUS bl_lilac_i2c_read(BL_LILAC_I2C_CONF* xi_conf, uint8_t slave_id, uint32_t offset, BL_LILAC_I2C_REG_WIDTH offset_width, uint8_t* xo_data, uint32_t data_len)
{
	int i, start_index =0;
	BL_LILAC_I2C_STATUS result = I2C_OK;
	uint32_t off = offset;
    	uint8_t* off_ptr = (uint8_t*)&off;

	if ( (data_len) == 0  || (xo_data==NULL)  )
        	return I2C_UNKNOWN_ERROR;

	/* We send the most significant byte first, so the bytes we send depends on
	   the width of the register address. Note that the offset variable is 32 bit,
	   but the register address may be 8, 16, 32 bit. */
	if (offset_width == I2C_REG_WIDTH_8)
    		start_index = 3;
    	else if (offset_width == I2C_REG_WIDTH_16)
    		start_index = 2;
    	else if (offset_width == I2C_REG_WIDTH_32)
    		start_index = 0;
	else
		return I2C_UNKNOWN_ERROR;


	/* write the slave device id and write operation */
	result = bl_lilac_i2c_write_byte( xi_conf, (slave_id  & 0xFE), 1, 0);
	if(result != I2C_OK)
		goto out;


	 /* write the offset/register to read from */
    	for (i = start_index; i < 4; i++)
    	{
		result = bl_lilac_i2c_write_byte( xi_conf, off_ptr[i], 0, 0 );
		if(result != I2C_OK)
			goto out;
    	}


	/* write the slave address and read operation */
	result = bl_lilac_i2c_write_byte(xi_conf, (slave_id | I2C_READ), 1, 0);
	if(result != I2C_OK)
		goto out;


	/* write dummy
	   because of the fifo, the data_len is linitted to 4,
	   no more than 4 bytes can be read */
    	for(i = 0 ; i < (data_len - 1) ; i ++ )
    	{
        	result = bl_lilac_i2c_write_byte( xi_conf, 0, 0, 0 );
        	if(result != I2C_OK)
			goto out;
 	}
    	/* write last dummy (with stop bit) */
    	result = bl_lilac_i2c_write_byte( xi_conf, 0, 0, 1 );


	if(result != I2C_OK)
		goto out;

	/* read data on groups of 4 bytes = rx fifo depth */
	for(i = 0; i < data_len; i++)
	{
		result = bl_lilac_i2c_read_byte(xi_conf, xo_data++);
		if(result != I2C_OK)
			goto out;
	}


out:
	bl_lilac_i2c_sw_reset(xi_conf);
	return result;

}
EXPORT_SYMBOL(bl_lilac_i2c_read);

#endif /* __MIPS_C  */




