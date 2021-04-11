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


#include "bl_os_wraper.h"
#include "bl_lilac_timer.h"
#include "bl_lilac_rst.h"
#include "bl_lilac_cr.h"
#include "bl_lilac_ic.h"
#include "bl_lilac_mips_blocks.h"


typedef struct
{
    volatile unsigned int  interrupt ;
    volatile unsigned int  timer_control ;
    volatile unsigned int  timer_counter ;
    volatile unsigned int  prescale ;
    volatile unsigned int  prescale_counter ;
    volatile unsigned int  match_control ;
    volatile unsigned int  match ;
} TimerRegs ;

#define BL_HW_TIMER_MAGIC		7777

#define PRESCALE_MAX			0xFFFF
#define MATCH_MAX				0xFFFFFFFF

#define M_WRITE_MATCH(reg_base, val)		(((TimerRegs*)reg_base)->match			= (val))
#define M_WRITE_PRESCALE(reg_base, val)		(((TimerRegs*)reg_base)->prescale		= (val))
#define M_WRITE_TCR(reg_base, val)			(((TimerRegs*)reg_base)->timer_control	= (val))
#define M_WRITE_MCR(reg_base, val)			(((TimerRegs*)reg_base)->match_control	= (val))
#define M_WRITE_TCNT(reg_base, val)			(((TimerRegs*)reg_base)->timer_counter	= (val))

#define M_ENABLE_INT(reg_base)				(((TimerRegs*)reg_base)->match_control |=  1)
#define M_DISABLE_INT(reg_base)				(((TimerRegs*)reg_base)->match_control &= ~1) 
#define M_CLEAR_INT(reg_base)				(((TimerRegs*)reg_base)->interrupt = 1)
#define M_READ_INT(reg_base)				(((TimerRegs*)reg_base)->interrupt)

typedef struct
{
    int						allocated;     			/* if non-zero this timer is in use */
    unsigned int			reg_base;				/* address of the registers */
    unsigned int			callback_arg;
    BL_HW_TIMER_CALLBACK	callback;
}TimerT ;

static TimerT timers[BL_HW_TIMER_NUM];
static int				initialized = 0;
static unsigned long	timer_clock;

static irqreturn_t bl_lilac_hw_timer_isr(int irq, TimerT* timer)
{
//	unsigned long flags = 0;
//	BL_LOCK_SECTION(flags);

        M_CLEAR_INT(timer->reg_base);

	if(timer->allocated && timer->callback)
		timer->callback(timer->callback_arg);
	

//	BL_UNLOCK_SECTION(flags);
	return IRQ_HANDLED;
}

static inline void init_hw_timers(void)
{
	int i = 0;
	unsigned long flags = 0;
	static char name[4][16];
	
    BL_LILAC_IC_INTERRUPT_PROPERTIES intProperties = {
		.priority		= BL_LILAC_HW_TIMET_ISR_PRIORITY,
		.configuration	= BL_LILAC_IC_INTERRUPT_LEVEL_LOW,
		.reentrant		= 0,
		.fast_interrupt	= 0
	};

	timer_clock  = get_lilac_peripheral_clock_rate();
	
	if(initialized)
		return;
	
	BL_LOCK_SECTION(flags);

	timers[0].reg_base = DEVICE_ADDRESS(CE_MIPS_BLOCKS_VPB_TIMER_0_ADDRESS);
	timers[1].reg_base = DEVICE_ADDRESS(CE_MIPS_BLOCKS_VPB_TIMER_1_ADDRESS);
	timers[2].reg_base = DEVICE_ADDRESS(CE_MIPS_BLOCKS_VPB_TIMER_2_ADDRESS);
	timers[3].reg_base = DEVICE_ADDRESS(CE_MIPS_BLOCKS_VPB_TIMER_3_ADDRESS);


	for(i = 0; i < BL_HW_TIMER_NUM ; i++)
	{
		M_WRITE_TCR(timers[i].reg_base, 0);
		M_WRITE_TCNT(timers[i].reg_base, 0);
		M_DISABLE_INT(timers[i].reg_base);
		M_CLEAR_INT(timers[i].reg_base);
		bl_lilac_ic_init_int(BL_LILAC_IC_INTERRUPT_TIMER_0 + i , &intProperties);
		bl_lilac_ic_isr_ack(BL_LILAC_IC_INTERRUPT_TIMER_0 + i);
		sprintf(name[i], "%s%d","bl_hw_timer",i);
		request_irq(INT_NUM_IRQ0 + BL_LILAC_IC_INTERRUPT_TIMER_0 + i, (irq_handler_t)bl_lilac_hw_timer_isr, 0, name[i], (void*)&timers[i]);
		bl_lilac_ic_mask_irq(BL_LILAC_IC_INTERRUPT_TIMER_0 + i);
	}
	
	initialized = 1;
	
	BL_UNLOCK_SECTION(flags);
}


/*=========================================================================
                     HW TIMER INTERFACE
 ==========================================================================*/


/***************************************************************************/
/*    allocated new HW timer                                               */
/***************************************************************************/
BL_TIMER_STATUS bl_hw_timer_alloc(BL_HW_TIMER* timer, BL_HW_TIMER_NUMBER timer_num)
{
	unsigned long flags = 0;

	if(timer_num >= BL_HW_TIMER_NUM)
	{
		return BL_TIMER_INVALID_PARAM;
	}

	BL_LOCK_SECTION(flags);

	init_hw_timers();

	if(timer_num < 0)
	{
        for(timer_num = BL_HW_TIMER_NUM-1; timer_num >= 0 ; timer_num--)
        {
            if(!timers[timer_num].allocated)
              break;
        }
	}

	if( (timer_num < 0) || timers[timer_num].allocated )
	{
		BL_UNLOCK_SECTION(flags);
		*timer = (void*)BL_HW_TIMER_INVALID;
		return BL_TIMER_NO_FREE_TIMERS;
	}
	

	timers[timer_num].allocated	= 1;
	*timer = (void*)timer_num +BL_HW_TIMER_MAGIC; 

	BL_UNLOCK_SECTION(flags);
	return BL_TIMER_OK;

}
EXPORT_SYMBOL(bl_hw_timer_alloc);


/***************************************************************************/
/*     Free previously allocated timer                                     */
/***************************************************************************/
BL_TIMER_STATUS bl_hw_timer_free(BL_HW_TIMER timer)
{
	return bl_hw_timer_stop(timer, 1);
}
EXPORT_SYMBOL(bl_hw_timer_free);


/***************************************************************************/
/*     Stop the timer, if "free" is true wiil free the timer     */ 
/***************************************************************************/
BL_TIMER_STATUS bl_hw_timer_stop(BL_HW_TIMER timer_handle, RTTMR_BOOL free)
{
	TimerT*			timer = NULL;
	int				index = -1;
	unsigned long	flags = 0;

	if(!timer_handle)
		return BL_TIMER_INVALID_PARAM;

	index = (int)timer_handle - BL_HW_TIMER_MAGIC ;
	
	if((index < 0) || (index >= BL_HW_TIMER_NUM)) 
		return BL_TIMER_INVALID_PARAM;

	BL_LOCK_SECTION(flags);

	timer = &timers[index];

	if(!timer->allocated)
	{
		BL_UNLOCK_SECTION(flags);
		return BL_TIMER_NO_ALLOCATED;
	}

	/* disable interrupt */
	M_DISABLE_INT(timer->reg_base);

	/* disable the timer */
	M_WRITE_TCR(timer->reg_base, 0);

	bl_lilac_ic_mask_irq(BL_LILAC_IC_INTERRUPT_TIMER_0 + index);
	M_CLEAR_INT(timer->reg_base);
	bl_lilac_ic_isr_ack(BL_LILAC_IC_INTERRUPT_TIMER_0 + index);

	if(free)
	{
		timer->allocated	= 0;
	    timer->callback_arg	= 0;
	    timer->callback		= NULL;
	}
	
	BL_UNLOCK_SECTION(flags);
	return BL_TIMER_OK;

}
EXPORT_SYMBOL(bl_hw_timer_stop);

/***************************************************************************/
/*     Stop the timer, if "free" is true wiil free the timer     */ 
/***************************************************************************/
static inline BL_TIMER_STATUS bl_hw_timer_stop_and_reset(BL_HW_TIMER timer_handle, RTTMR_BOOL free)
{
	TimerT*			timer = NULL;
	int				index = -1;
	unsigned long	flags = 0;

	if(!timer_handle)
		return BL_TIMER_INVALID_PARAM;

	index = (int)timer_handle - BL_HW_TIMER_MAGIC;
	
	if((index < 0) || (index >= BL_HW_TIMER_NUM)) 
		return BL_TIMER_INVALID_PARAM;

	BL_LOCK_SECTION(flags);

	timer = &timers[index];

	if(!timer->allocated)
	{
		BL_UNLOCK_SECTION(flags);
		return BL_TIMER_NO_ALLOCATED;
	}

	/* disable interrupt */
	M_DISABLE_INT(timer->reg_base);

	/* disable the timer */
	M_WRITE_TCR(timer->reg_base, 0);

	bl_lilac_ic_mask_irq(BL_LILAC_IC_INTERRUPT_TIMER_0 + index);
	M_CLEAR_INT(timer->reg_base);
	bl_lilac_ic_isr_ack(BL_LILAC_IC_INTERRUPT_TIMER_0 + index);

	if(free)
	{
		timer->allocated	= 0;
	    timer->callback_arg	= 0;
	    timer->callback		= NULL;
	}


	/* reset timer counter to 0 */ 
	M_WRITE_TCNT(timer->reg_base, 0);

	
	BL_UNLOCK_SECTION(flags);
	return BL_TIMER_OK;

}


/***************************************************************************/
/*     Start timer, if timer not provided new one will be alocated and  retunedin the "timer" */
/***************************************************************************/
BL_TIMER_STATUS bl_hw_timer_start(BL_HW_TIMER* timer_handle, BL_HW_TIMER_START_PARAMS* params)
{

	TimerT*			timer		= NULL;
	int				index		= -1;
	unsigned long	flags		= 0;
	unsigned long 	prescale	= 0;
	unsigned long	timeout; 
	BL_TIMER_STATUS ret_code	= BL_TIMER_OK;

	if(!timer_handle || !params)
		return BL_TIMER_INVALID_PARAM;

	timeout	= params->timeout;

	if(!timeout || ((params->resolution != BL_HW_TIMER_RESOLUTION_MICR) && (params->resolution != BL_HW_TIMER_RESOLUTION_MILI)))
	{
		return BL_TIMER_INVALID_PARAM;
	}

	if(!*timer_handle)
	{
		ret_code = bl_hw_timer_alloc(timer_handle, BL_HW_TIMER_INVALID);
		if(BL_TIMER_OK != ret_code)
			return ret_code;
	}

	index = (int)*timer_handle - BL_HW_TIMER_MAGIC;
	
	if((index < 0) || (index >= BL_HW_TIMER_NUM)) 
		return BL_TIMER_INVALID_PARAM;
	
	BL_LOCK_SECTION(flags);

	timer = &timers[index];

	if(!timer->allocated)
	{
		BL_UNLOCK_SECTION(flags);
		return BL_TIMER_NO_ALLOCATED;
	}

	timer->callback = params->callback;
	timer->callback_arg	= params->arg;
	
	
	prescale = timer_clock/params->resolution;

	if(prescale >= PRESCALE_MAX)
	{
		prescale /= 8;
		timeout  *= 8;
	}
	

	if(timeout == 1)
	{
		prescale /=2;
	}
	else
	{
		timeout--;
	}

	/* reset timer counter to 0 */ 
	M_WRITE_TCNT(timer->reg_base, 0);
	
	M_CLEAR_INT(timer->reg_base);

	/* set timeout */
	M_WRITE_MATCH(timer->reg_base,    timeout);
	M_WRITE_PRESCALE(timer->reg_base, prescale);
	
    if(params->restart)
    {
		M_WRITE_MCR(timer->reg_base, 0x02);
    }
    else
    {
		M_WRITE_MCR(timer->reg_base, 0);
    }
	
	/* enable interrupt */
	if(params->interrupt)
	{
		M_ENABLE_INT(timer->reg_base);
	}
        
	/* enable the timer */
	M_WRITE_TCR(timer->reg_base, 1);

	bl_lilac_ic_unmask_irq(BL_LILAC_IC_INTERRUPT_TIMER_0 + index);

	BL_UNLOCK_SECTION(flags);
    return BL_TIMER_OK;

}
EXPORT_SYMBOL(bl_hw_timer_start);


/***************************************************************************/
/*     allows to access count register of the allocated timer              */
/***************************************************************************/
BL_TIMER_STATUS  bl_hw_timer_read_count(BL_HW_TIMER timer_handle, unsigned int* count)
{
	TimerT*			timer = NULL;
	int				index = -1;
	unsigned long	flags = 0;

	if(!timer_handle)
		return BL_TIMER_INVALID_PARAM;

	index = (int)timer_handle - BL_HW_TIMER_MAGIC;
	
	if((index < 0) || (index >= BL_HW_TIMER_NUM)) 
		return BL_TIMER_INVALID_PARAM;

	BL_LOCK_SECTION(flags);

	timer = &timers[index];

	if(!timer->allocated)
	{
		BL_UNLOCK_SECTION(flags);
		return BL_TIMER_NO_ALLOCATED;
	}

	*count = ((TimerRegs*)timer->reg_base)->timer_counter;

	BL_UNLOCK_SECTION(flags);
    return BL_TIMER_OK;
	
}
EXPORT_SYMBOL(bl_hw_timer_read_count);

/***************************************************************************/
/*     return status for not "cycle" timer                                 */
/***************************************************************************/
BL_TIMER_STATUS  bl_hw_timer_is_expired(BL_HW_TIMER timer_handle, RTTMR_BOOL* expired)
{

	TimerT*			timer = NULL;
	int				index = -1;
	unsigned long	flags = 0;

	if(!timer_handle)
		return BL_TIMER_INVALID_PARAM;

	index = (int)timer_handle - BL_HW_TIMER_MAGIC;
	
	if((index < 0) || (index >= BL_HW_TIMER_NUM)) 
		return BL_TIMER_INVALID_PARAM;

	BL_LOCK_SECTION(flags);

	timer = &timers[index];

	if(!timer->allocated)
	{
		BL_UNLOCK_SECTION(flags);
		return BL_TIMER_NO_ALLOCATED;
	}
	
	*expired = M_READ_INT(timer->reg_base) & 0x1;
	M_CLEAR_INT( timer->reg_base);

	BL_UNLOCK_SECTION(flags);
    return BL_TIMER_OK;
}



//==================================================================================
//
//	RTT timer
//
//==================================================================================


#define REAL_TIMER_INT_DELAY  	3
#define RTTMR_MCSEC_INSEC 		1000000
#define RTTMR_INT_PRIORITY 		5

typedef struct 
{
	BL_RTTMR_MODE		Mode;
	unsigned int		Timeout;
	unsigned int		CurTimeout;
	RTTMR_BOOL			IsUsed;
	RTTMR_BOOL			IsApply;
	RTTMR_BOOL			IsAllocated;
	RTTMR_BOOL			IsNeedFree;
	void*				Prm;
	RealTimeTimerCallBack* 		ClientRtn;
	RealTimeTimerErrorCallBack*	ClientErrRtn;
		
}RTTMR_entry;

BL_HW_TIMER			timer_id			= NULL;

static int 			NumEntry			= 0;
static unsigned int	CurrentTimeout		= -1;
static unsigned int	ResolutionInMcSec	= 2*(REAL_TIMER_INT_DELAY);
static int			is_not_initialized	= 1;

BL_HW_TIMER_NUMBER	timernum 			= BL_HW_TIMER_INVALID;

static BL_HW_TIMER_START_PARAMS rttmr_timer_params;

static RTTMR_entry 	TimersEntry[BL_RTTMR_MAX_TYMER_USERS]; 

#define IS_NOT_INITILIZED()	{if(is_not_initialized){return BL_TIMER_ERROR;}}


/*--------------------------------------------------------------------*/ 
static inline void QueryTimer(unsigned int timeout)
{
	int i;
	
	for(i = 0; i < BL_RTTMR_MAX_TYMER_USERS; i++)
	{
		if(TimersEntry[i].IsUsed == RTTMR_FALSE)
			continue;
		
		 if(TimersEntry[i].CurTimeout > timeout + REAL_TIMER_INT_DELAY )
         {
			 TimersEntry[i].CurTimeout -= (timeout + REAL_TIMER_INT_DELAY);
         }
		 else
			 TimersEntry[i].CurTimeout = 0;
			
		if(TimersEntry[i].CurTimeout <= ResolutionInMcSec)
		{
			TimersEntry[i].IsApply = RTTMR_TRUE;
			
			if(TimersEntry[i].Mode == BL_RTTMR_MODE_ONCE)
			{
				TimersEntry[i].IsUsed = RTTMR_FALSE;
				
				if(TimersEntry[i].IsNeedFree == RTTMR_TRUE)
				{
					NumEntry--;		
					TimersEntry[i].IsAllocated	= RTTMR_FALSE;
				}
			}
			else
			{
				TimersEntry[i].CurTimeout = TimersEntry[i].Timeout;
			}
		}
	}

}

/*--------------------------------------------------------------------*/ 
static inline void StartTimer(void)
{
	int ToSet = -1;
	int i;
	unsigned int min = 0xffffffff;
	
	for(i = 0; i < BL_RTTMR_MAX_TYMER_USERS; i++)
	{
		if((TimersEntry[i].IsUsed == RTTMR_TRUE) && (TimersEntry[i].CurTimeout < min ))
		{
			min = TimersEntry[i].CurTimeout;
			ToSet = i;
		}
	}
	if(ToSet >= 0)
	{
	    CurrentTimeout 				= min - REAL_TIMER_INT_DELAY;
		rttmr_timer_params.timeout	= CurrentTimeout;

		if(bl_hw_timer_start(&timer_id, &rttmr_timer_params) != BL_TIMER_OK)
		{
            if(TimersEntry[ToSet].ClientErrRtn)
                TimersEntry[ToSet].ClientErrRtn(TimersEntry[ToSet].Prm, BL_TIMER_ERROR);
		}
	}
    else
    {
        bl_hw_timer_stop_and_reset(timer_id, 0);
    }
}

/*--------------------------------------------------------------------*/ 
static inline void ApplyUserCallBack(void)
{
	int i;
	for(i = 0; i < BL_RTTMR_MAX_TYMER_USERS; i++)
	{
		if(TimersEntry[i].IsApply == RTTMR_TRUE )
		{
			TimersEntry[i].IsApply = RTTMR_FALSE; 
			TimersEntry[i].ClientRtn(TimersEntry[i].Prm);
		}
	}
}

/*--------------------------------------------------------------------*/ 
static void  TimerCallback(int prm)
{
	 QueryTimer(CurrentTimeout);
	 StartTimer();
	 ApplyUserCallBack();
}




/***************************************************************************/
/*   Initializes rt timer module, allocates HW timer                        */
/*   Should be called only ones with only one single timer during system    */
/*   initialization  proccess,  before first usage of that timer            */
/*   InputParam    timer_number  - HW timer number ,                        */
/*                                  if equals to BL_HW_TIMER_INVALID first  */
/*                                  free HW timer will be allocated.        */                                   
/***************************************************************************/
BL_TIMER_STATUS bl_rttmr_init(BL_HW_TIMER_NUMBER timer_number)
{

	int i;
	unsigned long	flags = 0;

	if((timer_number >= BL_HW_TIMER_NUM) || (timer_number < 0) ) 
	{
		return BL_TIMER_INVALID_PARAM;
	}
	
	if(!is_not_initialized)
	{
		if(timer_number == timernum)
			return BL_TIMER_OK;
		
		return BL_TIMER_ALREADY_INITIALIZED;
	}

	/* Allocate timer */
    if(bl_hw_timer_alloc(&timer_id, timer_number) != BL_TIMER_OK)
    {
		return BL_TIMER_ERROR;
    }

	timernum = timer_number;
	
	BL_LOCK_SECTION(flags);


	for(i = 0; i < BL_RTTMR_MAX_TYMER_USERS; i++)
	{
		memset((void*)&TimersEntry[i], 0, sizeof(RTTMR_entry)); 
	}
	
    /* Setting the HW high resolution timer */
	rttmr_timer_params.resolution	= BL_HW_TIMER_RESOLUTION_MICR;
	rttmr_timer_params.interrupt	= 1;
	rttmr_timer_params.restart		= 0;
	rttmr_timer_params.callback		= &TimerCallback;
	rttmr_timer_params.arg			= 0;
	
	is_not_initialized	= 0;
	
	BL_UNLOCK_SECTION(flags);

	return BL_TIMER_OK;
}
EXPORT_SYMBOL(bl_rttmr_init);



/***************************************************************************/
/*     register user callbaks and start timer                              */
/***************************************************************************/
BL_TIMER_STATUS bl_rttmr_register(
                  RealTimeTimerCallBack*		clbk, 		    /*func ptr ,that will be called after timeout expired                        */ 
			      RealTimeTimerErrorCallBack*	err_clbk,       /*func ptr ,that will be called in case err occur,can be NULL                */  
			      void*                    		uparam,    		/*arg to pass to ClientRtn                                                   */
			      unsigned int             		timeoutInMcsec,	/*timeout in MicroSecond                                                     */
			      BL_RTTMR_MODE              	mode,           /*timer mode Cycle/Once                                                      */ 
			      BL_RTTMR_HANDLE*				timer_hanle,     /*Output,timer handle, should be provided in unreggister function            */
				  RTTMR_BOOL					shouldUnregister/*Input, do not care for the RTTMR_MODE_CYCLE, for the BL_RTTMR_MODE_ONCE mode  */
					  											/* user timer will be automatically unregistered if set to true              */ 
				  )
{
	unsigned long	flags = 0;
	int i;
	unsigned int timerCount;
	
	IS_NOT_INITILIZED();	
		
	if(BL_RTTMR_MAX_TYMER_USERS <= NumEntry)
	{
		return BL_TIMER_ERROR;
	}
	
	if(timeoutInMcsec > 0xffffffff || timeoutInMcsec <= ResolutionInMcSec)
	{
		return BL_TIMER_INVALID_PARAM;
	}
	
	BL_LOCK_SECTION(flags);

	bl_hw_timer_read_count(timer_id, &timerCount);
	
	bl_hw_timer_stop_and_reset(timer_id, 0);	
	
	if(CurrentTimeout > timerCount)
		CurrentTimeout = CurrentTimeout - timerCount ;
	else
		CurrentTimeout = 0;
	
	
	QueryTimer(timerCount);
	ApplyUserCallBack();
	
	for(i = 0; i < BL_RTTMR_MAX_TYMER_USERS; i++)
	{
		if(TimersEntry[i].IsAllocated == RTTMR_FALSE)
			break;
	}
	
	if(i >= BL_RTTMR_MAX_TYMER_USERS)
	{
		BL_UNLOCK_SECTION(flags);
		return BL_TIMER_ERROR;
	}
	
	TimersEntry[i].ClientRtn	= clbk;
	TimersEntry[i].ClientErrRtn = err_clbk;
 	TimersEntry[i].CurTimeout 	= timeoutInMcsec;
 	TimersEntry[i].IsUsed 		= RTTMR_TRUE;
	TimersEntry[i].Mode 		= mode;
	TimersEntry[i].Prm 			= uparam;
	TimersEntry[i].Timeout 		= timeoutInMcsec;
	TimersEntry[i].IsApply		= RTTMR_FALSE;
	TimersEntry[i].IsAllocated  = RTTMR_TRUE;
	
	if(mode == BL_RTTMR_MODE_ONCE) 
		TimersEntry[i].IsNeedFree = shouldUnregister;
	else
		TimersEntry[i].IsNeedFree = RTTMR_FALSE;
	
	NumEntry++;
	
	*timer_hanle = (BL_RTTMR_HANDLE) &TimersEntry[i];

	StartTimer();

	BL_UNLOCK_SECTION(flags);
	
	return BL_TIMER_OK;

}
EXPORT_SYMBOL(bl_rttmr_register);


/***************************************************************************/
/*  restart once timer                              					   */
/***************************************************************************/

BL_TIMER_STATUS bl_rttmr_restart_oncetimer (
						BL_RTTMR_HANDLE					timer_hanle,    /*timer handle                                                                                                              */
						RealTimeTimerCallBack*			clbk, 			/*func ptr ,that will be called after timeout expired,can be NULL -in this case will be used given in rttmr_register        */ 
						RealTimeTimerErrorCallBack*		err_clbk,  	 	/*func ptr ,that will be called in case err occur,can be NULL -in this case will be used given in rttmr_register            */  
						void*                    		uparam,    		/*arg to pass to ClientRtn                                                                                                  */
						unsigned int             		timeoutInMcsec,	/*timeout in MicroSecond                                                                                                    */
						RTTMR_BOOL						shouldUnregister/*Input, do not care for the RTTMR_MODE_CYCLE, for the BL_RTTMR_MODE_ONCE mode user timer will be automatically unregistered if set to true */ 
					  )
{

	unsigned int 	timerCount;
	unsigned long	flags = 0;
	BL_TIMER_STATUS	ret = BL_TIMER_ERROR;

	IS_NOT_INITILIZED();
	
	if(!timer_hanle)
		return BL_TIMER_INVALID_PARAM;
	
	if(timeoutInMcsec > 0xffffffff || timeoutInMcsec <= ResolutionInMcSec)
	{
		return BL_TIMER_INVALID_PARAM;
	}
	
	BL_LOCK_SECTION(flags);
	
	
	bl_hw_timer_read_count(timer_id, &timerCount);
	
	bl_hw_timer_stop_and_reset(timer_id, 0);	
	
	if(CurrentTimeout > timerCount)
		CurrentTimeout = CurrentTimeout - timerCount ;
	else
		CurrentTimeout = 0;
    	
	
	if((((RTTMR_entry*)timer_hanle)->IsAllocated != RTTMR_TRUE) || (((RTTMR_entry*)timer_hanle)->Mode != BL_RTTMR_MODE_ONCE))
	{
		ret = BL_TIMER_ERROR;
	}
	else
	{
		if(clbk)
			((RTTMR_entry*)timer_hanle)->ClientRtn		= clbk;
		
		if(err_clbk)
			((RTTMR_entry*)timer_hanle)->ClientErrRtn	= err_clbk;

		((RTTMR_entry*)timer_hanle)->Prm 			= uparam;
	 	((RTTMR_entry*)timer_hanle)->CurTimeout 	= timeoutInMcsec;
	 	((RTTMR_entry*)timer_hanle)->IsUsed 		= RTTMR_TRUE;
		((RTTMR_entry*)timer_hanle)->Timeout 		= timeoutInMcsec;
		((RTTMR_entry*)timer_hanle)->IsApply		= RTTMR_FALSE;
		((RTTMR_entry*)timer_hanle)->IsNeedFree 	= shouldUnregister;
		
		ret = BL_TIMER_OK;
	}

	QueryTimer(timerCount);
	ApplyUserCallBack();
	StartTimer();

	BL_UNLOCK_SECTION(flags);
	return ret;

}
EXPORT_SYMBOL(bl_rttmr_restart_oncetimer);


/***************************************************************************/
/*     return user parameter for the timer             					   */
/***************************************************************************/
BL_TIMER_STATUS rttmr_getparam(BL_RTTMR_HANDLE timer_hanle , void** uparam)
{
	unsigned long	flags = 0;
	BL_TIMER_STATUS ret = BL_TIMER_ERROR;

	IS_NOT_INITILIZED();
	
	*uparam = NULL;
	
	if(!timer_hanle)
		return BL_TIMER_ERROR;
	
	BL_LOCK_SECTION(flags);
	
	if(((RTTMR_entry*)timer_hanle)->IsAllocated == RTTMR_TRUE)
	{
		*uparam = ((RTTMR_entry*)timer_hanle)->Prm;
		ret = BL_TIMER_OK;
	}
	
	BL_UNLOCK_SECTION(flags);
	
	return ret;
}
EXPORT_SYMBOL(rttmr_getparam);


/***************************************************************************/
/*     unregister user callbaks and stop timer (if need)                   */
/***************************************************************************/
BL_TIMER_STATUS bl_rttmr_unregister(BL_RTTMR_HANDLE  timer_hanle)
{

	unsigned long	flags = 0;
	unsigned int 	timerCount;
	BL_TIMER_STATUS	ret = BL_TIMER_ERROR;
		
	IS_NOT_INITILIZED();
	
	BL_LOCK_SECTION(flags);
	
	bl_hw_timer_read_count(timer_id, &timerCount);
	
	bl_hw_timer_stop_and_reset(timer_id, 0);	
	
	if(CurrentTimeout > timerCount)
		CurrentTimeout = CurrentTimeout - timerCount ;
	else
		CurrentTimeout = 0;
	
	if(((RTTMR_entry*)timer_hanle)->IsAllocated == RTTMR_TRUE)
	{
		((RTTMR_entry*)timer_hanle)->IsUsed 		= RTTMR_FALSE;
		((RTTMR_entry*)timer_hanle)->IsApply		= RTTMR_FALSE;
		((RTTMR_entry*)timer_hanle)->IsAllocated	= RTTMR_FALSE;
		NumEntry--;
		ret = BL_TIMER_OK;
	}
	
	QueryTimer(timerCount);
	StartTimer();
	ApplyUserCallBack();
	
	BL_UNLOCK_SECTION(flags);

	return ret;
}
EXPORT_SYMBOL(bl_rttmr_unregister);



