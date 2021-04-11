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
/* This file provides Real time timer functionality and low level HW timer  */
/* driver                                                                  */
/*                                                                         */
/*  The BL Real Time Timer is an Generic envelope of the HW Timer.         */
/*  It provide simple software interface for the HW timer with ability to  */
/*  initialize a single HW timer and to register up to                     */
/*  BL_RTTMR_MAX_TYMER_USERS calbacks with diffrent timeout and mode to    */ 
/*  that timer.                                                            */
/*  Note Real Time Timer API supports the utilization of only one,         */
/*  single HW timer.                                                       */
/*                                                                         */
/*  The RT TImer timer support two work mode :                             */
/* 1.Cycle                                                                 */
/* 2.Once                                                                  */
/*                                                                         */
/* In "Cycle" mode after timeout expired and callback called timer reset   */
/* itself to the programmed timeout.To stop timer user must call unregister*/
/* function. To change the timeout the user should unregister its timer    */
/* and register its again with the new value of the timeout.               */                                      
/*                                                                         */
/* In "Once" mode after timeout expired and callback called the Callback   */
/* will be unregisterd automatically. Anyway Callback may be unregistered  */
/* before it called by RealTImeTimer.                                      */
/*                                                                         */
/* WARNING:                                                                */
/* The Callback functions will be called in the interrupt context.         */
/* Do no perform in the Callback functions any opperation that not callable*/
/* from interrupt service routines                                         */
/*                                                                         */
/* In additional still can be direct used "old"  stile  HW timer           */
/* NOTE:                                                                   */
/*        All needed work for configuration of the IC  and                 */
/*        enable/disable interrupt ack will be done by the                 */
/*        driver in case that "interrupt" parameters set to true           */
/*                                                                         */
/* WARNING:                                                                */
/* The Callback functions will be called in the interrupt context.         */
/* Do no perform in the Callback functions any opperation that not callable*/
/* from interrupt service routines                                         */
/*                                                                         */
/*  The program using example                                              */
/*  TBD                                                                    */
/*                                                                         */
/* The HW timer interface                                                  */
/* TBD                                                                     */
/*                                                                         */
/***************************************************************************/

#ifndef BL_TIMER_H_
#define BL_TIMER_H_

#ifdef __cplusplus
extern "C"
{
#endif


/*Priority of the HW timers interrupts */
#define BL_LILAC_HW_TIMET_ISR_PRIORITY	5

/*=========================================================================
                     GENERIC TIMERS RETURN STATUS
 ==========================================================================*/
/* This deffinitions used for both HW and RT TIMERS */
typedef enum 
{
	BL_TIMER_OK				= 0,
	BL_TIMER_ERROR			= 1,
	BL_TIMER_NO_FREE_TIMERS = 2, 
	BL_TIMER_INVALID_PARAM	= 3,
	BL_TIMER_NO_ALLOCATED	= 4,
	BL_TIMER_ALREADY_INITIALIZED = 5
}BL_TIMER_STATUS;

/*Defines the numbers of the existing HW timres*/
typedef enum 
{
    BL_HW_TIMER_INVALID = -1,
    BL_HW_TIMER_0       = 0,
	BL_HW_TIMER_1       = 1,
    BL_HW_TIMER_2       = 2,
    BL_HW_TIMER_3       = 3,
    BL_HW_TIMER_NUM
}BL_HW_TIMER_NUMBER;

/*=========================================================================
                     REAL TIME TIMER INTERFACE
 ==========================================================================*/


/*Max number of timer clients*/
#define BL_RTTMR_MAX_TYMER_USERS 128


/*timer handle type - used for register/unregister functions*/
typedef void*  BL_RTTMR_HANDLE;

/*RT Timers mode  */
typedef enum 
{
	BL_RTTMR_MODE_ONCE = 0,
	BL_RTTMR_MODE_CYCLE 
}BL_RTTMR_MODE;



typedef int RTTMR_BOOL;
#define RTTMR_FALSE		0
#define RTTMR_TRUE		(!(RTTMR_FALSE))


/* User callbacks will be called with parameter given in time registration to RealTImeTimer*/

/* RealTimeTimerCallBack function will be called after  timeout expired.*/
typedef void RealTimeTimerCallBack(void* prm); 

/* RealTimeTimerErrorCallBack function will be called if an error occured. */
/* If this callback is invoked and the user still needs timer              */
/* services, the user should unregister its timer and register it again.   */
typedef void RealTimeTimerErrorCallBack(void* prm , BL_TIMER_STATUS error);


/***************************************************************************/
/*   Initializes rt timer module, allocates HW timer                        */
/*   Should be called only ones with only one single timer during system    */
/*   initialization  proccess,  before first usage of that timer            */
/*   InputParam    timer_number  - HW timer number ,                        */
/*                                  if equals to BL_HW_TIMER_INVALID first  */
/*                                  free HW timer will be allocated.        */                                   
/***************************************************************************/
BL_TIMER_STATUS bl_rttmr_init(BL_HW_TIMER_NUMBER timer_number);


/***************************************************************************/
/*     register user callbaks and start timer                              */
/***************************************************************************/
BL_TIMER_STATUS bl_rttmr_register(
                  RealTimeTimerCallBack*		clbk, 		    /*func ptr ,that will be called after timeout expired                        */ 
			      RealTimeTimerErrorCallBack*	err_clbk,       /*func ptr ,that will be called in case err occur,can be NULL                */  
			      void*                    		uparam,    		/*arg to pass to ClientRtn                                                   */
			      unsigned int             		timeoutInMcsec,	/*timeout in MicroSecond                                                     */
			      BL_RTTMR_MODE              	mode,           /*timer mode Cycle/Once                                                      */ 
			      BL_RTTMR_HANDLE*				timer_hanle,    /*Output,timer handle, should be provided in unreggister function            */
				  RTTMR_BOOL					shouldUnregister/*Input, do not care for the RTTMR_MODE_CYCLE, for the RTTMR_MODE_ONCE mode  */
					  											/* user timer will be automatically unregistered if set to true              */ 
				  );


/***************************************************************************/
/*  restart once timer                              					   */
/***************************************************************************/

BL_TIMER_STATUS bl_rttmr_restart_oncetimer (
						BL_RTTMR_HANDLE					timer_hanle,    /*timer handle                                                                                                              */
						RealTimeTimerCallBack*			clbk, 			/*func ptr ,that will be called after timeout expired,can be NULL -in this case will be used given in rttmr_register        */ 
						RealTimeTimerErrorCallBack*		err_clbk,  	 	/*func ptr ,that will be called in case err occur,can be NULL -in this case will be used given in rttmr_register            */  
						void*                    		uparam,    		/*arg to pass to ClientRtn                                                                                                  */
						unsigned int             		timeoutInMcsec,	/*timeout in MicroSecond                                                                                                    */
						RTTMR_BOOL						shouldUnregister/*Input, do not care for the RTTMR_MODE_CYCLE, for the RTTMR_MODE_ONCE mode user timer will be automatically unregistered if set to true */ 
					  );


/***************************************************************************/
/*     return user parameter for the timer             					   */
/***************************************************************************/
BL_TIMER_STATUS bl_rttmr_getparam(BL_RTTMR_HANDLE timer_hanle, void** uparam);


/***************************************************************************/
/*     unregister user callbaks and stop timer (if need)                   */
/***************************************************************************/
BL_TIMER_STATUS bl_rttmr_unregister(BL_RTTMR_HANDLE  timer_hanle);


/*=========================================================================
                     HW TIMER INTERFACE
 ==========================================================================*/

typedef void* BL_HW_TIMER; 

typedef void (*BL_HW_TIMER_CALLBACK)(int arg);

typedef enum
{
	BL_HW_TIMER_RESOLUTION_MICR = 1000000,
	BL_HW_TIMER_RESOLUTION_MILI = 1000
}BL_HW_TIMER_RESOLUTION;


typedef struct
{
	BL_HW_TIMER_RESOLUTION	resolution;			/* timer resolution 															*/  
	unsigned int			timeout;       		/* timeout in resolution 														*/
	RTTMR_BOOL				interrupt;			/* set to true to enable interrupt    											*/
	RTTMR_BOOL				restart;	        /* set to true to start periodic timer , if intrrupt false - will be ignored 	*/    
	BL_HW_TIMER_CALLBACK	callback;			/* callback  functions , if interrupt false  will be ignored			     	*/
	unsigned int			arg;                /* parameter which will be sent to callback , if interrupt false will be ignored*/
}BL_HW_TIMER_START_PARAMS; 



/***************************************************************************/
/*    allocated new HW timer                                               */
/***************************************************************************/
BL_TIMER_STATUS bl_hw_timer_alloc(
								BL_HW_TIMER*    	timer,      /* return value */
								BL_HW_TIMER_NUMBER	timer_num	/* hw id for timer allocation, if -1 first free HW timer wili be alocated */
							 );



/***************************************************************************/
/*     Free previously allocated timer, if timer run will  Stop the timer  */
/***************************************************************************/
BL_TIMER_STATUS bl_hw_timer_free(BL_HW_TIMER timer);


/***************************************************************************/
/*     Start timer, if timer not provided new one will be alocated and  retunedin the "timer" */
/***************************************************************************/
BL_TIMER_STATUS bl_hw_timer_start(BL_HW_TIMER* timer, BL_HW_TIMER_START_PARAMS* params);


/***************************************************************************/
/*     Stop the timer, if "free" is true wiil free the timer     */ 
/***************************************************************************/
BL_TIMER_STATUS bl_hw_timer_stop(BL_HW_TIMER timer, RTTMR_BOOL free);


/***************************************************************************/
/*     allows to access count register of the allocated timer              */
/***************************************************************************/
BL_TIMER_STATUS  bl_hw_timer_read_count(BL_HW_TIMER timer, unsigned int* count);

/***************************************************************************/
/*     return status for not "cycle" timer                                 */
/***************************************************************************/
BL_TIMER_STATUS  bl_hw_timer_is_expired(BL_HW_TIMER timer , RTTMR_BOOL* expired);



#ifdef __cplusplus
}
#endif
    

#endif /* BL_TIMER_H_ */
