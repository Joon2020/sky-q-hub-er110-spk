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
/* Timer driver                                                            */
/*                                                                         */
/*                                                                         */
/***************************************************************************/

#include <linux/module.h>
#include <linux/irqflags.h>
#include <linux/interrupt.h>
#include <linux/irqreturn.h>
#include <linux/spinlock.h>
#include <bcm_map_part.h>
#include <bcm_intr.h>
#include <bcm_ext_timer.h>

typedef struct
{
    int						allocated;     			/* if non-zero this timer is in use */
    unsigned int*			timer_ctrl_reg;
    unsigned int*			timer_cnt_reg;
    unsigned int			callback_arg;
    ExtTimerHandler			callback;
}TimerT ;

static TimerT   timers[EXT_TIMER_NUM];
static int      initialized = 0;

static DEFINE_SPINLOCK(timer_spinlock);

static irqreturn_t ext_timer_isr(int irq, unsigned int param)
{
    unsigned long flags;
    unsigned char int_status;
    unsigned int timer_num;
	
    spin_lock_irqsave(&timer_spinlock, flags); 

    /* get which timer produced the interrupt */
    int_status = TIMER->TimerInts;
    if (int_status & TIMER0EN)
        timer_num = 0;
    else if (int_status & TIMER1EN)
        timer_num = 1;
    else
        timer_num = 2;

    /* clear the interrupt */
    TIMER->TimerInts |= (int_status & 0x7);

    BcmHalInterruptEnable(INTERRUPT_ID_TIMER);

    /* call the appropriate function */
    if(timers[timer_num].allocated && timers[timer_num].callback)
        timers[timer_num].callback(timers[timer_num].callback_arg);
	
    spin_unlock_irqrestore(&timer_spinlock, flags); 
    return IRQ_HANDLED;
}

/***************************************************************************/
/*     Initialize external timers mechanism                                     */
/***************************************************************************/
int init_hw_timers(void)
{
    unsigned long flags;
	
    if(initialized)
        return -1;
	
    spin_lock_irqsave(&timer_spinlock, flags); 

    timers[0].allocated = 0;
    timers[1].allocated = 0;
    timers[2].allocated = 0;

    /* mask external timer interrupts */
    TIMER->TimerMask = 0;

    /* clear external timer interrupts */
    TIMER->TimerInts |= TIMER0 | TIMER1 | TIMER2;

    timers[0].timer_ctrl_reg = (unsigned int*)&(TIMER->TimerCtl0);
    timers[1].timer_ctrl_reg = (unsigned int*)&(TIMER->TimerCtl1);
    timers[2].timer_ctrl_reg = (unsigned int*)&(TIMER->TimerCtl2);

    timers[0].timer_cnt_reg = (unsigned int*)&(TIMER->TimerCnt0);
    timers[1].timer_cnt_reg = (unsigned int*)&(TIMER->TimerCnt1);
    timers[2].timer_cnt_reg = (unsigned int*)&(TIMER->TimerCnt2);

    /* Connect and enable interrupt */
    if (BcmHalMapInterrupt((FN_HANDLER)ext_timer_isr, INTERRUPT_ID_TIMER, INTERRUPT_ID_TIMER))
    {
        spin_unlock_irqrestore(&timer_spinlock, flags); 
        return -1;
    }
    BcmHalInterruptEnable(INTERRUPT_ID_TIMER);
	
    initialized = 1;
	
    spin_unlock_irqrestore(&timer_spinlock, flags); 

    return 0;
}
EXPORT_SYMBOL(init_hw_timers);


/*=========================================================================
                     HW TIMER INTERFACE
 ==========================================================================*/


/***************************************************************************/
/*    allocated and start new HW timer                                               */
/*    timer_period is in microseconds                                                     */
/***************************************************************************/
int ext_timer_alloc(EXT_TIMER_NUMBER timer_num, unsigned int timer_period, ExtTimerHandler timer_callback, unsigned int param )
{
    unsigned long flags;
    unsigned long timer_count;

    if(timer_num >= EXT_TIMER_NUM)
    {
        return -1;
    }

    spin_lock_irqsave(&timer_spinlock, flags); 

    if(timer_num < 0)
    {
        for(timer_num = EXT_TIMER_NUM-1; timer_num >= 0 ; timer_num--)
        {
            if(!timers[timer_num].allocated)
              break;
        }
    }

    if( (timer_num < 0) || timers[timer_num].allocated )
    {
        spin_unlock_irqrestore(&timer_spinlock, flags); 
        return -1;
    }

    timers[timer_num].callback = timer_callback;
    timers[timer_num].callback_arg = param;

    timer_count = ((timer_period-20))*1000/20; //rbus_freq=50MHz
    *(timers[timer_num].timer_ctrl_reg) = timer_count;
    TIMER->TimerMask |= (TIMER0EN << timer_num);
    *(timers[timer_num].timer_ctrl_reg) |= TIMERENABLE ;
    timers[timer_num].allocated	= 1;

    spin_unlock_irqrestore(&timer_spinlock, flags); 
    return timer_num;

}
EXPORT_SYMBOL(ext_timer_alloc);


/***************************************************************************/
/*     Free previously allocated timer                                     */
/***************************************************************************/
int ext_timer_free(EXT_TIMER_NUMBER timer_num)
{
    unsigned long flags;

    if( (timer_num >= EXT_TIMER_NUM) || (timer_num < 0) )
        return -1;

    spin_lock_irqsave(&timer_spinlock, flags); 

    if ( !timers[timer_num].allocated )
    {
        spin_unlock_irqrestore(&timer_spinlock, flags); 
        return -1;
    }

    timers[timer_num].callback = NULL;
    /* disable the timer */
    *(timers[timer_num].timer_ctrl_reg) &= ~TIMERENABLE ;
    /* mask the interrupt */
    TIMER->TimerMask &= ~(TIMER0EN << timer_num);
    /* clear interrupt */
    TIMER->TimerInts |= (TIMER0 << timer_num);
    timers[timer_num].allocated	= 0;

    spin_unlock_irqrestore(&timer_spinlock, flags); 

    return 0;
}
EXPORT_SYMBOL(ext_timer_free);

/***************************************************************************/
/*     Stop the timer 							     */ 
/***************************************************************************/
int ext_timer_stop(EXT_TIMER_NUMBER timer_num)
{
    unsigned long flags;

    if( (timer_num >= EXT_TIMER_NUM) || (timer_num < 0) )
        return -1;

    spin_lock_irqsave(&timer_spinlock, flags); 

    if ( !timers[timer_num].allocated )
    {
        spin_unlock_irqrestore(&timer_spinlock, flags); 
        return -1;
    }

    /* disable the timer */
    *(timers[timer_num].timer_ctrl_reg) &= ~TIMERENABLE ;
    /* mask the interrupt */
    TIMER->TimerMask &= ~(TIMER0EN << timer_num);
    /* clear interrupt */
    TIMER->TimerInts |= (TIMER0 << timer_num);

    spin_unlock_irqrestore(&timer_spinlock, flags); 

    return 0;

}
EXPORT_SYMBOL(ext_timer_stop);


/***************************************************************************/
/*     Start timer					 */
/***************************************************************************/
int ext_timer_start(EXT_TIMER_NUMBER timer_num)
{
    unsigned long flags;

    if( (timer_num >= EXT_TIMER_NUM) || (timer_num < 0) )
        return -1;

    spin_lock_irqsave(&timer_spinlock, flags); 

    if ( !timers[timer_num].allocated )
    {
        spin_unlock_irqrestore(&timer_spinlock, flags); 
        return -1;
    }
	
    /* enable the timer */
    *(timers[timer_num].timer_ctrl_reg) |= TIMERENABLE ;
    /* unmask the interrupt */
    TIMER->TimerMask |= (TIMER0EN << timer_num);
	
    spin_unlock_irqrestore(&timer_spinlock, flags); 

    return 0;


}
EXPORT_SYMBOL(ext_timer_start);

/***************************************************************************/
/*     allows to access count register of the allocated timer              */
/***************************************************************************/
int  ext_timer_read_count(EXT_TIMER_NUMBER timer_num, unsigned int* count)
{
    unsigned long	flags;

    if( (timer_num >= EXT_TIMER_NUM) || (timer_num < 0) )
        return -1;

    spin_lock_irqsave(&timer_spinlock, flags); 

    if ( !timers[timer_num].allocated )
    {
        spin_unlock_irqrestore(&timer_spinlock, flags); 
        return -1;
    }

    *count = *(timers[timer_num].timer_cnt_reg) & TIMER_COUNT_MASK;

    spin_unlock_irqrestore(&timer_spinlock, flags); 
    return 0;
	
}
EXPORT_SYMBOL(ext_timer_read_count);

