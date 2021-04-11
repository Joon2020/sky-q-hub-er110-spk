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

/****************************************************************************/
/*                                                                         	*/
/* GPIO driver                     											*/
/*                                                                         	*/
/****************************************************************************/

#include "bl_os_wraper.h"
#include "bl_lilac_gpio.h"
#include "bl_lilac_mips_blocks.h"
#include "bl_lilac_pin_muxing.h"



static int gpio_useable(uint32_t pin_no)
{
	if(pin_no >= LAST_PHYSICAL_PIN)
		return 0;
	
	/* skip not used pads */
	if((pin_no == SKIP_PAD_49) || (pin_no == SKIP_PAD_50))
		return 0;

	return 1;
}


/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_gpio_set_mode                                             */
/*                                                                         */
/*   set input/output mode for the specified pin                           */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_gpio_set_mode(uint32_t pin_no, BL_LILAC_GPIO_MODE mode)
{
	uint32_t pins;

    if(!gpio_useable(pin_no) || (mode >= BL_LILAC_GPIO_MODE_LAST))
		return BL_LILAC_SOC_INVALID_PARAM;

	if((pin_no >= GPIO_ONLY_IN_MIN) && (pin_no <= GPIO_ONLY_IN_MAX))
		return BL_LILAC_SOC_OK;
	
	if(pin_no > 31)
	{
        if (mode == BL_LILAC_GPIO_MODE_OUTPUT) // output
        {
            if ((pin_no >= GPIO_ONLY_IN_MIN) && (pin_no <= GPIO_ONLY_IN_MAX))
            {
                printk("ERROR: attempt to configureread in only pin as out # %d, %s", (int)pin_no, __FUNCTION__);
                return BL_LILAC_SOC_INVALID_PARAM;
            }
        }
        else //input
        {
            if ((pin_no >= GPIO_ONLY_OUT_MIN) && (pin_no <= GPIO_ONLY_OUT_MAX))
            {
				printk("ERROR: attempt to configureread out only pin as in # %d, %s", (int)pin_no, __FUNCTION__);
				return BL_LILAC_SOC_INVALID_PARAM;
            }
        }
		
		BL_MIPS_BLOCKS_VPB_GPIO_B_DR_B_READ(pins);
		
		if (pin_no >= GPIO_ONLY_OUT_MIN) 
			pin_no -= 38;
		else 
			pin_no -= 32;
		
		if(mode)
			pins |= (1 << pin_no);
		else
			pins &= ~(1 << pin_no);
		
		BL_MIPS_BLOCKS_VPB_GPIO_B_DR_B_WRITE(pins);
	}
	else
	{
		BL_MIPS_BLOCKS_VPB_GPIO_A_DR_A_READ(pins);
		
		if(mode)
			pins |= (1 << pin_no);
		else
			pins &= ~(1 << pin_no);
		
		BL_MIPS_BLOCKS_VPB_GPIO_A_DR_A_WRITE(pins);
	}

	return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_gpio_set_mode);

/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_gpio_write_pin                                            */
/*                                                                         */
/* Write pin value for an output GPIO pin                                  */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_gpio_write_pin(uint32_t pin_no, uint32_t value) 
{
	uint32_t pins;

    if(!gpio_useable(pin_no))
		return BL_LILAC_SOC_INVALID_PARAM;
	
	if(pin_no > 31)
	{
		if ((pin_no >= GPIO_ONLY_IN_MIN) && (pin_no <= GPIO_ONLY_IN_MAX))
		{
			printk("ERROR: attempt to write to read only pin # %d, %s", (int)pin_no, __FUNCTION__);
			return BL_LILAC_SOC_INVALID_PARAM;
		}

		BL_MIPS_BLOCKS_VPB_GPIO_B_OR_REG_B_READ(pins);
		
		if (pin_no >= GPIO_ONLY_OUT_MIN) 
			pin_no -= 38;
		else 
			pin_no -= 32;
		
		if(value)
			pins |= (1 << pin_no);
		else
			pins &= ~(1 << pin_no);
		
		BL_MIPS_BLOCKS_VPB_GPIO_B_OR_REG_B_WRITE(pins);
	}
	else
	{
		BL_MIPS_BLOCKS_VPB_GPIO_A_OR_REG_A_READ(pins);
		
		if(value)
			pins |= (1 << pin_no);
		else
			pins &= ~(1 << pin_no);
		BL_MIPS_BLOCKS_VPB_GPIO_A_OR_REG_A_WRITE(pins);
	}
	
	return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_gpio_write_pin);

/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_gpio_read_pin                                                 */
/*                                                                         */
/* Read pin value for an input GPIO pin                                    */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_gpio_read_pin(uint32_t pin_no, uint32_t* value) 
{
	uint32_t pins;
	
    if(!gpio_useable(pin_no))
		return BL_LILAC_SOC_INVALID_PARAM;

	if(pin_no > 31 )
	{
        if ((pin_no >= GPIO_ONLY_OUT_MIN) && (pin_no <= GPIO_ONLY_OUT_MAX))
        {
			printk("ERROR: attempt to read from write only pin # %d, %s", (int)pin_no, __FUNCTION__);
        	return BL_LILAC_SOC_INVALID_PARAM;
        }

		BL_MIPS_BLOCKS_VPB_GPIO_B_PINS_B_READ(pins);
    	*value = (pins >> (pin_no - 32)) & 0x1;
	}
	else
	{
		BL_MIPS_BLOCKS_VPB_GPIO_A_PINS_A_READ(pins);
    	*value = (pins >> pin_no) & 0x1;
	}
	
	return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL( bl_lilac_gpio_read_pin );


