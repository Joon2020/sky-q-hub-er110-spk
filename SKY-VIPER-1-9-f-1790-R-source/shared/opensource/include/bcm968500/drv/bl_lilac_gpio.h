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


#ifndef BL_LILAC_DRV_GPIO_H_  
#define BL_LILAC_DRV_GPIO_H_

#include "bl_lilac_soc.h"
#include "bl_lilac_rst.h"

#ifdef __cplusplus
extern "C"
{
#endif

typedef enum
{
    BL_LILAC_GPIO_MODE_INPUT	= 0,
    BL_LILAC_GPIO_MODE_OUTPUT	= 1,
    BL_LILAC_GPIO_MODE_LAST
    
}BL_LILAC_GPIO_MODE;



#ifdef __MIPS_C 
/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_gpio_init                                                    */
/*                                                                         */
/* initialize GPIO module                                                  */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_gpio_init(BL_LILAC_CONTROLLER id, BL_LILAC_CR_STATE state);

#endif


/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_gpio_set_mode                                                */
/*                                                                         */
/*   set input/output mode for the specified pin                           */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_gpio_set_mode(uint32_t pin_no, BL_LILAC_GPIO_MODE mode); 


/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_gpio_write_pin                                               */
/*                                                                         */
/* Write pin value for an output GPIO pin                                  */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_gpio_write_pin(uint32_t pin_no, uint32_t value); 


/***************************************************************************/
/* Name                                                                    */
/*   bl_bl_lilac_gpio_read                                                 */
/*                                                                         */
/* Read pin value for an input GPIO pin                                    */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_gpio_read_pin(uint32_t pin_no, uint32_t* value); 


#ifdef __cplusplus
}
#endif

#endif /*BL_LILAC_DRV_GPIO_H_ */
