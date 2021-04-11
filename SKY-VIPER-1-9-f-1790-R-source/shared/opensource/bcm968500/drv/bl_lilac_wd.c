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

#include <linux/kernel.h>
#include <linux/module.h>

#include "bl_lilac_brd.h"
#include "bl_lilac_timer.h"
#include "bl_lilac_pin_muxing.h"
#include "bl_lilac_gpio.h"
#include "bl_lilac_ic.h"
#include "bl_lilac_wd.h"

static BL_HW_TIMER_START_PARAMS   wd_params;
static BL_HW_TIMER  wd_timer;


//kick stupid WD  this one should be call by some SW before timer expired 
void bl_lilac_wd_kick(void)
{
    bl_hw_timer_stop(wd_timer, RTTMR_FALSE);
    bl_hw_timer_start(&wd_timer, & wd_params);
}
EXPORT_SYMBOL(bl_lilac_wd_kick);


// start WD
void  bl_lilac_wd_start(int timeout_in_milisec)
{
    BL_TIMER_STATUS stat;

    wd_params.resolution = BL_HW_TIMER_RESOLUTION_MILI;
    wd_params.timeout = timeout_in_milisec;
    wd_params.interrupt = 1;
    wd_params.restart = 0;

    //start timer
    stat = bl_hw_timer_start(&wd_timer, & wd_params);
    if (stat != BL_TIMER_OK)
        printk("failed to start WD\n");

    bl_lilac_ic_mask_irq(BL_LILAC_IC_INTERRUPT_TIMER_0 + WD_TIMER);

}
EXPORT_SYMBOL(bl_lilac_wd_start);

void bl_lilac_wd_stop(void)
{
    bl_hw_timer_stop(wd_timer, RTTMR_FALSE);
}
EXPORT_SYMBOL(bl_lilac_wd_stop);


int bl_lilac_wd_init(void)
{
    BL_TIMER_STATUS stat;
    LILAC_PIN_MUXING_RETURN_CODE ret;
  
    // MUX appropriated pin to timer
    ret = fi_bl_lilac_mux_connect_pin(WD_GPIO_PIN  ,TIMER, 3, MIPSC);
    if (ret != BL_LILAC_SOC_OK)
        printk("failed to mux pin\n");

    //Allocate timer 
    stat = bl_hw_timer_alloc( &wd_timer, WD_TIMER);
    if (stat != BL_TIMER_OK)
        printk("failed to alloc WD\n");



    return 0;
}
EXPORT_SYMBOL(bl_lilac_wd_init);





