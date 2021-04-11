/*
    Copyright 2000-2011 Broadcom Corporation

    <:label-BRCM:2011:DUAL/GPL:standard
    
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
/***************************************************************************
 * File Name  : bcm63xx_led.c
 *
 * Description: 
 *    This file contains bcm963xx board led control API functions. 
 *
 ***************************************************************************/

/* Includes. */
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/capability.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/bcm_assert_locks.h>
#include <asm/uaccess.h>

#include <bcm_map_part.h>
#include <board.h>
#include <boardparms.h>
#include <shared_utils.h>
#include <bcmtypes.h>
#if defined(CONFIG_BCM968500)
#include "bl_lilac_pin_muxing.h"
#include "bl_lilac_gpio.h"
#endif

extern spinlock_t bcm_gpio_spinlock;

#define k125ms              (HZ / 8)   // 125 ms
#define kFastBlinkCount     0          // 125ms
#define kSlowBlinkCount     1          // 250ms

#define kLedOff             0
#define kLedOn              1

#define kLedGreen           0
#define kLedRed             1

// uncomment // for debug led
// #define DEBUG_LED

typedef int (*BP_LED_FUNC) (unsigned short *);

typedef struct {
    BOARD_LED_NAME ledName;
    BP_LED_FUNC bpFunc;
    BP_LED_FUNC bpFuncFail;
} BP_LED_INFO, *PBP_LED_INFO;

typedef struct {
    short ledGreenGpio;             // GPIO # for LED
    short ledRedGpio;               // GPIO # for Fail LED
    BOARD_LED_STATE ledState;       // current led state
#if defined(SKY) && !defined(CONFIG_SKY_ETHAN)
    BOARD_LED_NAME  ledName; // This is newly added -Nk
#endif /* SKY */	
    short blinkCountDown;           // Count for blink states
} LED_CTRL, *PLED_CTRL;

static BP_LED_INFO bpLedInfo[] =
{
    {kLedAdsl, BpGetAdslLedGpio, BpGetAdslFailLedGpio},
    {kLedSecAdsl, BpGetSecAdslLedGpio, BpGetSecAdslFailLedGpio},
    {kLedWanData, BpGetWanDataLedGpio, BpGetWanErrorLedGpio},
    {kLedSes, BpGetWirelessSesLedGpio, BpGetWirelessSesErrorLedGpio},
    {kLedVoip, BpGetVoipLedGpio, NULL},
    {kLedVoip1, BpGetVoip1LedGpio, BpGetVoip1FailLedGpio},
    {kLedVoip2, BpGetVoip2LedGpio, BpGetVoip2FailLedGpio},
    {kLedPots, BpGetPotsLedGpio, NULL},
    {kLedDect, BpGetDectLedGpio, NULL},
    {kLedGpon, BpGetGponLedGpio, BpGetGponFailLedGpio},
    {kLedMoCA, BpGetMoCALedGpio, BpGetMoCAFailLedGpio},
#if defined(CONFIG_BCM96838)
    {kLedOpticalLink,  NULL, BpGetOpticalLinkFailLedGpio},
    {kLedUSB, BpGetUSBLedGpio, NULL},
    {kLedSim, BpGetGpioLedSim, NULL},
    {kLedSimITMS, BpGetGpioLedSimITMS, NULL},
    {kLedEpon, BpGetEponLedGpio, BpGetEponFailLedGpio},
#endif
#if defined(CONFIG_BCM968500)
    {kLedOpticalLink,  NULL, BpGetOpticalLinkFailLedGpio},
    {kLedLan1, UtilGetLan1LedGpio, NULL},
    {kLedLan2, UtilGetLan2LedGpio, NULL},
    {kLedLan3, UtilGetLan3LedGpio, NULL},
    {kLedLan4, UtilGetLan4LedGpio, NULL},
    {kLedUSB, BpGetUSBLedGpio, NULL}, 
#endif	
#ifdef SKY		
    {kLedPwr, BpGetBootloaderPowerOnLedGpio,BpGetBootloaderStopLedGpio}, // Added newly for pwr led blinking 
#ifndef CONFIG_SKY_ETHAN
    {kLedWll,BpGetwirelessLedGpio,NULL},
#else
    {kLedWll,BpGetwirelessLedGpio,BpGetwirelessErrorLedGpio},
#endif
    {kLedSkyHd, BpGetSkyHdLedGpio, BpGetSkyHdErrorLedGpio},
#endif /* SKY */	
    {kLedEnd, NULL, NULL}
};

// global variables:
static struct timer_list gLedTimer;
static PLED_CTRL gLedCtrl = NULL;
static int gTimerOn = FALSE;
static int gTimerOnRequests = 0;
static unsigned int gLedRunningCounter = 0;  // only used by WLAN

void (*ethsw_led_control)(unsigned long ledMask, int value) = NULL;
EXPORT_SYMBOL(ethsw_led_control);

/** This spinlock protects all access to gLedCtrl, gTimerOn
 *  gTimerOnRequests, gLedRunningCounter, and ledTimerStart function.
 *  Use spin_lock_irqsave to lock the spinlock because ledTimerStart
 *  may be called from interrupt handler (WLAN?)
 */
static spinlock_t brcm_ledlock;
static void ledTimerExpire(void);

#ifndef CONFIG_SKY_ETHAN
void SetPowerLedOn(void);
static void setLed_Gpio(PLED_CTRL pLed, unsigned short led_state, unsigned short led_type);
static void ledToggle_g(PLED_CTRL pLed,unsigned short led_type);
void SetPowerLedOn(void);
#endif /* SKY */
//**************************************************************************************
// LED operations
//**************************************************************************************

#if defined(CONFIG_SKY_ETHAN) || !defined(SKY)
// turn led on and set the ledState
static void setLed (PLED_CTRL pLed, unsigned short led_state, unsigned short led_type)
{
    short led_gpio;
    unsigned short gpio_state;
    unsigned long flags;

#if defined(CONFIG_BCM968500)
	BL_LILAC_SOC_STATUS status;
#endif

    if (led_type == kLedRed)
        led_gpio = pLed->ledRedGpio;
    else
        led_gpio = pLed->ledGreenGpio;

    if (led_gpio == -1)
        return;

    if (((led_gpio & BP_ACTIVE_LOW) && (led_state == kLedOn)) || 
        (!(led_gpio & BP_ACTIVE_LOW) && (led_state == kLedOff)))
        gpio_state = 0;
    else
        gpio_state = 1;

    /* spinlock to protect access to GPIODir, GPIOio */
    spin_lock_irqsave(&bcm_gpio_spinlock, flags);

#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96318)
    /* Enable LED controller to drive this GPIO */
    if (!(led_gpio & BP_GPIO_SERIAL))
        GPIO->GPIOMode |= GPIO_NUM_TO_MASK(led_gpio);
#endif

#if defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96828)
    /* Enable LED controller to drive this GPIO */
    if (!(led_gpio & BP_GPIO_SERIAL))
        GPIO->LEDCtrl |= GPIO_NUM_TO_MASK(led_gpio);
#endif

#if defined(CONFIG_BCM968500)
        status = bl_lilac_gpio_write_pin(led_gpio, gpio_state);
        if (status != BL_LILAC_SOC_OK)
                printk("Error writing %d to GPIO %d\n", gpio_state ,led_gpio);
#endif

#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318)|| defined(CONFIG_BCM96828)
    LED->ledMode &= ~(LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
    if( gpio_state )
        LED->ledMode |= (LED_MODE_OFF << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
    else
        LED->ledMode |= (LED_MODE_ON << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));

#elif defined(CONFIG_BCM96838)
    if ( (led_gpio&BP_LED_PIN) || (led_gpio&BP_GPIO_SERIAL) )
    {
        LED->ledMode &= ~(LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
        if( gpio_state )
            LED->ledMode |= (LED_MODE_OFF << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
        else
            LED->ledMode |= (LED_MODE_ON << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
    }
    else
    {
        led_gpio &= BP_GPIO_NUM_MASK;
        gpio_set_dir(led_gpio, 1);
        gpio_set_data(led_gpio, gpio_state);
    }

#elif !defined(CONFIG_BCM968500)
    if (led_gpio & BP_GPIO_SERIAL) {
        while (GPIO->SerialLedCtrl & SER_LED_BUSY);
        if( gpio_state )
            GPIO->SerialLed |= GPIO_NUM_TO_MASK(led_gpio);
        else
            GPIO->SerialLed &= ~GPIO_NUM_TO_MASK(led_gpio);
    }
    else {
        GPIO->GPIODir |= GPIO_NUM_TO_MASK(led_gpio);
        if( gpio_state )
            GPIO->GPIOio |= GPIO_NUM_TO_MASK(led_gpio);
        else
            GPIO->GPIOio &= ~GPIO_NUM_TO_MASK(led_gpio);
    }
#endif

    spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);

}

// toggle the LED
static void ledToggle(PLED_CTRL pLed)
{
    short led_gpio;
    short green_led_gpio , red_led_gpio;
#if defined(CONFIG_BCM968500)
	BL_LILAC_SOC_STATUS status;
	uint32_t  prev_led_state ;
	uint32_t  new_led_state ;
	unsigned long flags;
#endif

   green_led_gpio = pLed->ledGreenGpio ;
   red_led_gpio = pLed->ledRedGpio ;

    if ((-1== green_led_gpio) && (-1== red_led_gpio))
        return;

 /* Currently all the chips other than 968500 don't support blinking of RED LED */
#if !defined(CONFIG_BCM968500)
    if (-1== green_led_gpio)
        return;
#endif
  
   led_gpio = green_led_gpio ;

#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318)|| defined(CONFIG_BCM96828)
    LED->ledMode ^= (LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
#elif defined(CONFIG_BCM968500)
	if (-1== green_led_gpio)
		 led_gpio = red_led_gpio ;	
	status =  bl_lilac_gpio_read_pin(led_gpio, &prev_led_state) ;
	if (status != BL_LILAC_SOC_OK)
	{
		printk("Error Reading value from GPIO  %d\n", led_gpio);
		return;
	}
	/* Toggle */
	new_led_state = 1 - prev_led_state ;
	spin_lock_irqsave(&bcm_gpio_spinlock, flags);
        status = bl_lilac_gpio_write_pin(led_gpio, new_led_state);
	spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);
        if (status != BL_LILAC_SOC_OK)
                printk("Error writing %ld to GPIO  %d\n", (unsigned long)new_led_state ,pLed->ledGreenGpio);

#elif defined(CONFIG_BCM96838)
    if ( (led_gpio&BP_LED_PIN) || (led_gpio&BP_GPIO_SERIAL) )
        LED->ledMode ^= (LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
    else
    {
        unsigned long flags;
        spin_lock_irqsave(&bcm_gpio_spinlock, flags);
        gpio_set_data(led_gpio, 1^gpio_get_data(led_gpio));
        spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);
    }
	
#else

    if (led_gpio & BP_GPIO_SERIAL) {
        while (GPIO->SerialLedCtrl & SER_LED_BUSY);
        GPIO->SerialLed ^= GPIO_NUM_TO_MASK(led_gpio);
    }
    else {
        unsigned long flags;

        spin_lock_irqsave(&bcm_gpio_spinlock, flags);
        GPIO->GPIODir |= GPIO_NUM_TO_MASK(led_gpio);
        GPIO->GPIOio ^= GPIO_NUM_TO_MASK(led_gpio);
        spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);
    }

#endif
}   

#endif /* SKY */

#ifdef SKY
// toggle the Amber LED
static void ledToggleAmber(PLED_CTRL pLed)
{
    short led_gpio;
    short green_led_gpio , red_led_gpio;

    green_led_gpio = pLed->ledGreenGpio ;
    red_led_gpio = pLed->ledRedGpio ;

    if ((-1== green_led_gpio) && (-1== red_led_gpio))
        return;

    /* Currently all the chips other than 968500 don't support blinking of RED LED */
#if !defined(CONFIG_BCM968500)
    if (-1== green_led_gpio)
        return;
#endif


#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318)|| defined(CONFIG_BCM96828)
    led_gpio = green_led_gpio ;

    LED->ledMode ^= (LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));

    led_gpio = red_led_gpio ;

    LED->ledMode ^= (LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
#else

    printk("led: Invalid Build...\n");

#endif

}  
#endif // SKY

#ifdef CONFIG_SKY_ETHAN
//toggle the Red LED
static void ledToggleRed(PLED_CTRL pLed)
{
    short led_gpio;
    short red_led_gpio;

    red_led_gpio = pLed->ledRedGpio;

    if (-1 == red_led_gpio)
    {
        return;
    }

    led_gpio = red_led_gpio;
    LED->ledMode ^= (LED_MODE_MASK << GPIO_NUM_TO_LED_MODE_SHIFT(led_gpio));
}
#endif // CONFIG_SKY_ETHAN

/** Start the LED timer
 *
 * Must be called with brcm_ledlock held
 */
static void ledTimerStart(void)
{
#if defined(DEBUG_LED)
    printk("led: add_timer\n");
#endif

    BCM_ASSERT_HAS_SPINLOCK_C(&brcm_ledlock);

    init_timer(&gLedTimer);
    gLedTimer.function = (void*)ledTimerExpire;
    gLedTimer.expires = jiffies + k125ms;        // timer expires in ~100ms
    add_timer (&gLedTimer);
} 


// led timer expire kicks in about ~100ms and perform the led operation according to the ledState and
// restart the timer according to ledState
static void ledTimerExpire(void)
{
    int i;
    PLED_CTRL pCurLed;
    unsigned long flags;

    BCM_ASSERT_NOT_HAS_SPINLOCK_V(&brcm_ledlock);

    for (i = 0, pCurLed = gLedCtrl; i < kLedEnd; i++, pCurLed++)
    {
#if defined(DEBUG_LED)
        printk("led[%d]: Mask=0x%04x, State = %d, blcd=%d\n", i, pCurLed->ledGreenGpio, pCurLed->ledState, pCurLed->blinkCountDown);
#endif
        spin_lock_irqsave(&brcm_ledlock, flags);        // LEDs can be changed from ISR
        switch (pCurLed->ledState)
        {
        case kLedStateOn:
        case kLedStateOff:
        case kLedStateFail:
            pCurLed->blinkCountDown = 0;            // reset the blink count down
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;

        case kLedStateSlowBlinkContinues:
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kSlowBlinkCount;
#ifdef CONFIG_SKY_ETHAN
                ledToggle(pCurLed);
#else /* SKY */	   		
		     ledToggle_g(pCurLed,kLedGreen);			
#endif /* SKY */			 
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;

        case kLedStateFastBlinkContinues:
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kFastBlinkCount;
#ifdef CONFIG_SKY_ETHAN
                ledToggle(pCurLed);
#else /* SKY */	   						
		    ledToggle_g(pCurLed,kLedGreen);			
#endif /* SKY */				
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;

        case kLedStateUserWpsInProgress:          /* 200ms on, 100ms off */
#ifdef CONFIG_SKY_ETHAN
            if (pCurLed->blinkCountDown-- == 0)
            {
                // pCurLed->blinkCountDown = (((gLedRunningCounter++)&1)? kFastBlinkCount: kSlowBlinkCount);
                // ledToggle(pCurLed);
                pCurLed->blinkCountDown = ( ( (gLedRunningCounter++) & 0x1 ) ? 0x3 : 0x6);
                ledToggleAmber(pCurLed);
            }
#else /* SKY */
/* This is based on requirement 1000.0070
The product's Wireless LED shall be a slow blinking amber colour for the WPS connection state when the
product is within the WPS Push Button mode enablement window period
Note: Slow Blinking amber is a repeating flash cadence of 1000ms ON and 500ms OFF
 timer will be fired every 125msec so for ON=8x125=1000msec
 OFF=4x125=500msec
 Since this is software timers so will give less time and adjust it in context switch delay 
 -NK
 */
	     if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = (((gLedRunningCounter++)&0x1)? 0x3: 0x6);
		  ledToggle_g(pCurLed,kLedRed);
		  //printk("\n MAKING ON  Count:%d\n",pCurLed->blinkCountDown);
	     }
 	     //printk("\n Outside Count:%d\n",pCurLed->blinkCountDown);  	 
#endif /* SKY */		 
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;             

#ifdef CONFIG_SKY_ETHAN
        case kLedStateUserWpsSessionOverLap:
#endif // CONFIG_SKY_ETHAN
        case kLedStateUserWpsError:               /* 100ms on, 100ms off */
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kFastBlinkCount;
#ifdef CONFIG_SKY_ETHAN
                ledToggleRed(pCurLed);
#else /* SKY */				
                /* Stops the link activity at white led -NK */
                //printk("\n WPS User Error \n");
                ledToggle_g(pCurLed, kLedRed);	
#endif /* SKY */		  
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;        

#ifdef SKY
		case kLedStateAmberFastBlinkContinues: 
            if (pCurLed->blinkCountDown-- == 0)
            {
                pCurLed->blinkCountDown = kFastBlinkCount;
                ledToggleAmber(pCurLed);
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;
#endif

#ifndef CONFIG_SKY_ETHAN
        case kLedStateUserWpsSessionOverLap:      /* 100ms on, 100ms off, 5 times, off for 500ms */        
            if (pCurLed->blinkCountDown-- == 0)
            {
                if(((++gLedRunningCounter) % 10) == 0) {
                    pCurLed->blinkCountDown = 4;
                    /* Stops the link activity at white led -NK */
                    //printk("\n WPS session in presss \n");	
                    setLed_Gpio(pCurLed, kLedOff, kLedRed);	
                    pCurLed->ledState = kLedStateUserWpsSessionOverLap;
                }
                else
                {
                    pCurLed->blinkCountDown = kFastBlinkCount;
                    ledToggle_g(pCurLed,kLedRed); 	
                }
            }
            gTimerOnRequests++;
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            break;
#endif // CONFIG_SKY_ETHAN   

        default:
            spin_unlock_irqrestore(&brcm_ledlock, flags);
            printk("Invalid state = %d\n", pCurLed->ledState);
        }
    }

    // Restart the timer if any of our previous LED operations required
    // us to restart the timer, or if any other threads requested a timer
    // restart.  Note that throughout this function, gTimerOn == TRUE, so
    // any other thread which want to restart the timer would only
    // increment the gTimerOnRequests and not call ledTimerStart.
    spin_lock_irqsave(&brcm_ledlock, flags);
    if (gTimerOnRequests > 0)
    {
        ledTimerStart();
        gTimerOnRequests = 0;
    }
    else
    {
        gTimerOn = FALSE;
    }
    spin_unlock_irqrestore(&brcm_ledlock, flags);
}




#if defined(CONFIG_BCM968500)
LILAC_PIN_MUXING_RETURN_CODE boardLedPinMux (unsigned short gpio_num)
{
        if ( (gpio_num&BP_GPIO_NUM_MASK) == BP_GPIO_NONE )
	        return BL_LILAC_SOC_ERROR;

	if ( BL_LILAC_SOC_OK != fi_bl_lilac_mux_connect_pin( gpio_num,  GPIO, 0, MIPSC)  )
	{
		printk("Error muxing pin %d\n",gpio_num);
		return 	BL_LILAC_SOC_ERROR;
	}
	if ( BL_LILAC_SOC_OK != bl_lilac_gpio_set_mode( gpio_num , BL_LILAC_GPIO_MODE_OUTPUT))
	{
		printk("Error setting pin %d as output \n",gpio_num );
		return	BL_LILAC_SOC_ERROR;
	}
	/* GPIO muxing and GPIO seting mode as output went well */
	return BL_LILAC_SOC_OK ;
}
#endif



void __init boardLedInit(void)
{
    PBP_LED_INFO pInfo;
    unsigned short i;
    short gpio;
#if defined(CONFIG_BCM96816) || defined(CONFIG_BCM96818)
    ETHERNET_MAC_INFO EnetInfo;
#endif
#if defined(CONFIG_BCM968500)
    int rc, token;
#endif

    spin_lock_init(&brcm_ledlock);

#if !(defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318) || defined(CONFIG_BCM96828) || defined(CONFIG_BCM968500) || defined(CONFIG_BCM96838) )
    /* Set blink rate for hardware LEDs. */
    GPIO->LEDCtrl &= ~LED_INTERVAL_SET_MASK;
    GPIO->LEDCtrl |= LED_INTERVAL_SET_80MS;
#endif

#if defined(CONFIG_BCM968500)
    i = 0; 
    token = 0;
    /* Set pin muxing for all the defined LEDs */
    for(;;)
    {
        rc = BpGetLedGpio(i, &token, &gpio);
       	if( rc == BP_MAX_ITEM_EXCEEDED )
            break;
        /* If some LED was set as BP_GPIO_NONE, DO increment i, so the loop will avoid searching another occurrence of
         * this LED. This is especially important if using bp_elemTemplate (inheritance), and the inheriting profile
         * wants to override the LED GPIO with BP_GPIO_NONE value (we don't want the loop to find more occurrences from
         * the profile we inherit from). */
        else if( rc == BP_SUCCESS && ((gpio & BP_GPIO_NUM_MASK) != BP_GPIO_NONE ))
        {
            gpio &= BP_GPIO_NUM_MASK; 
	    boardLedPinMux(gpio);
            //printk("Lilac set pin mux for gpio %d\n", gpio); 
        }
        else
	{
	    token = 0;
      	    i++;
        }  
    }
#endif

    gLedCtrl = (PLED_CTRL) kmalloc((kLedEnd * sizeof(LED_CTRL)), GFP_KERNEL);

    if( gLedCtrl == NULL ) {
        printk( "LED memory allocation error.\n" );
        return;
    }

    /* Initialize LED control */
    for (i = 0; i < kLedEnd; i++) {
        gLedCtrl[i].ledGreenGpio = -1;
        gLedCtrl[i].ledRedGpio = -1;
        gLedCtrl[i].ledState = kLedStateOff;
        gLedCtrl[i].blinkCountDown = 0;            // reset the blink count down
    }

    for( pInfo = bpLedInfo; pInfo->ledName != kLedEnd; pInfo++ ) {
        if( pInfo->bpFunc && (*pInfo->bpFunc) (&gpio) == BP_SUCCESS )
        {
            gLedCtrl[pInfo->ledName].ledGreenGpio = gpio;
        }
        if( pInfo->bpFuncFail && (*pInfo->bpFuncFail)(&gpio)==BP_SUCCESS )
        {
            gLedCtrl[pInfo->ledName].ledRedGpio = gpio;
        }
#ifdef CONFIG_SKY_ETHAN
        setLed(&gLedCtrl[pInfo->ledName], kLedOff, kLedGreen);
        setLed(&gLedCtrl[pInfo->ledName], kLedOff, kLedRed);
#else /* SKY */				
	    setLed_Gpio(&gLedCtrl[pInfo->ledName], kLedOff, kLedGreen);
        setLed_Gpio(&gLedCtrl[pInfo->ledName], kLedOff, kLedRed);
#endif /* SKY */		
    }

#if defined(CONFIG_BCM96816) || defined(CONFIG_BCM96818)
    if( BpGetEthernetMacInfo( &EnetInfo, 1 ) == BP_SUCCESS )
    {
        if ( EnetInfo.sw.ledInfo[0].duplexLed != BP_NOT_DEFINED )
        {
            gLedCtrl[kLedEth0Duplex].ledGreenGpio = EnetInfo.sw.ledInfo[0].duplexLed;
            setLed(&gLedCtrl[kLedEth0Duplex], kLedOff, kLedGreen);
        }
        if ( EnetInfo.sw.ledInfo[0].speedLed100 != BP_NOT_DEFINED )
        {
            gLedCtrl[kLedEth0Spd100].ledGreenGpio = EnetInfo.sw.ledInfo[0].speedLed100;
            setLed(&gLedCtrl[kLedEth0Spd100], kLedOff, kLedGreen);
        }
        if ( EnetInfo.sw.ledInfo[0].speedLed1000 != BP_NOT_DEFINED )
        {
            gLedCtrl[kLedEth0Spd1000].ledGreenGpio = EnetInfo.sw.ledInfo[0].speedLed1000;
            setLed(&gLedCtrl[kLedEth0Spd1000], kLedOff, kLedGreen);
        }
     
        if ( EnetInfo.sw.ledInfo[1].duplexLed != BP_NOT_DEFINED )
        {
            gLedCtrl[kLedEth1Duplex].ledGreenGpio = EnetInfo.sw.ledInfo[1].duplexLed;
            setLed(&gLedCtrl[kLedEth1Duplex], kLedOff, kLedGreen);
        }
        if ( EnetInfo.sw.ledInfo[1].speedLed100 != BP_NOT_DEFINED )
        {
            gLedCtrl[kLedEth1Spd100].ledGreenGpio = EnetInfo.sw.ledInfo[1].speedLed100;
            setLed(&gLedCtrl[kLedEth1Spd100], kLedOff, kLedGreen);
        }
        if ( EnetInfo.sw.ledInfo[1].speedLed1000 != BP_NOT_DEFINED )
        {
            gLedCtrl[kLedEth1Spd1000].ledGreenGpio = EnetInfo.sw.ledInfo[1].speedLed1000;
            setLed(&gLedCtrl[kLedEth1Spd1000], kLedOff, kLedGreen);
        }
    }
#endif

#ifdef CONFIG_SKY_ETHAN
	// On Only the Power LED
	setLed(&gLedCtrl[kLedPwr], kLedOn, kLedGreen);
#else
    SetPowerLedOn();
#endif //SKY

#if defined(DEBUG_LED)
    for (i=0; i < kLedEnd; i++)
        printk("initLed: led[%d]: Gpio=0x%04x, FailGpio=0x%04x\n", i, gLedCtrl[i].ledGreenGpio, gLedCtrl[i].ledRedGpio);
#endif
}

// led ctrl.  Maps the ledName to the corresponding ledInfoPtr and perform the led operation
void boardLedCtrl(BOARD_LED_NAME ledName, BOARD_LED_STATE ledState)
{
    unsigned long flags;
    PLED_CTRL pLed;

#ifndef CONFIG_SKY_ETHAN
    static UBOOL8  wll_set=FALSE;
	unsigned short led_gpio;
#endif /* SKY */

    BCM_ASSERT_NOT_HAS_SPINLOCK_V(&brcm_ledlock);

    spin_lock_irqsave(&brcm_ledlock, flags);     // LED can be changed from ISR

    if( (int) ledName < kLedEnd ) {

        pLed = &gLedCtrl[ledName];

        // If the state is kLedStateFail and there is not a failure LED defined
        // in the board parameters, change the state to kLedStateSlowBlinkContinues.
        if( ledState == kLedStateFail && pLed->ledRedGpio == -1 )
            ledState = kLedStateSlowBlinkContinues;

        // Save current LED state
        pLed->ledState = ledState;

#ifdef CONFIG_SKY_ETHAN
        // Start from LED Off and turn it on later as needed
        setLed (pLed, kLedOff, kLedGreen);
        setLed (pLed, kLedOff, kLedRed);
#ifndef SKY //NOT REQD

        // Disable HW control for WAN Data LED. 
        // It will be enabled only if LED state is On
#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318)|| defined(CONFIG_BCM96828) || defined(CONFIG_BCM96838)
        if (ledName == kLedWanData)
            LED->ledHWDis |= GPIO_NUM_TO_MASK(pLed->ledGreenGpio);
#elif defined(CONFIG_BCM96368)
        if (ledName == kLedWanData) {
            GPIO->AuxLedCtrl |= AUX_HW_DISAB_2;
            if (pLed->ledGreenGpio & BP_ACTIVE_LOW)
                GPIO->AuxLedCtrl |= (LED_STEADY_ON << AUX_MODE_SHFT_2);
            else
                GPIO->AuxLedCtrl &= ~(LED_STEADY_ON << AUX_MODE_SHFT_2);
        }
#endif
#endif //SKY

#else /* SKY */		
	 pLed->ledName = ledName;

	//printk("\nboardLedCtrl)ledName=%d ledState=%d\n",ledName,ledState);	

     /* This part of code is added for wan and adsl led handling using gpio 27July -NK */

	     /* First making it off then decide which one to be on */
		setLed_Gpio(pLed, kLedOff, kLedGreen);
	 	setLed_Gpio (pLed, kLedOff, kLedRed);

          if(ledName==kLedSes && ledState!=kLedOff)
    	   {
             /* This means we hecve to check the wireless LED state and restore it after 
              wps action is over */
            /*To be Generic use BpGetwirelessLedGpio to get wireless GPIO number
            * because vlaue is based on Board ID and chipset used. 
            */
              if( BpGetwirelessLedGpio( &led_gpio ) == BP_SUCCESS ) {
                  if(GPIO->GPIOio & GPIO_NUM_TO_MASK(led_gpio)) 
          	  {
		      wll_set=TRUE;	
		      GPIO->GPIOio &=~GPIO_NUM_TO_MASK(led_gpio);
          	  }
              }
          }
          if(ledName==kLedSes && ledState == kLedOff)
    	  {
    	    /* Restore the state of wireless LEd */
              if(wll_set)
              {
                 /*To be Generic use BpGetwirelessLedGpio to get wireless GPIO numbe */
                  if( BpGetwirelessLedGpio( &led_gpio ) == BP_SUCCESS ) {
	              GPIO->GPIOio |=GPIO_NUM_TO_MASK(led_gpio);	  	
		      wll_set=FALSE;	  
                  }
              }

    	  }
#endif /* SKY */

        switch (ledState) {
        case kLedStateOn:
#ifndef SKY //NOT REQD			
            // Enable SAR to control INET LED
#if defined(CONFIG_BCM96328) || defined(CONFIG_BCM96362) || defined(CONFIG_BCM963268) || defined(CONFIG_BCM96318)|| defined(CONFIG_BCM96828) || defined(CONFIG_BCM96838)
            if (ledName == kLedWanData)
                LED->ledHWDis &= ~GPIO_NUM_TO_MASK(pLed->ledGreenGpio);
#elif defined(CONFIG_BCM96368)
            if (ledName == kLedWanData) {
                GPIO->AuxLedCtrl &= ~AUX_HW_DISAB_2;
                if (pLed->ledGreenGpio & BP_ACTIVE_LOW)
                    GPIO->AuxLedCtrl &= ~(LED_STEADY_ON << AUX_MODE_SHFT_2);
                else
                    GPIO->AuxLedCtrl |= (LED_STEADY_ON << AUX_MODE_SHFT_2);
            }
#endif
#endif //SKY

#ifdef CONFIG_SKY_ETHAN
            setLed (pLed, kLedOn, kLedGreen);
#else  /* SKY */
                setLed_Gpio(pLed,kLedOn,kLedGreen);
#endif  /* SKY */				
            break;

        case kLedStateOff:

            break;
		case kLedStateGreenOnRedOn:
		case kLedStateAmber:
			{
#ifdef CONFIG_SKY_ETHAN
				setLed (pLed, kLedOn, kLedGreen);
				setLed (pLed, kLedOn, kLedRed);
#else
                setLed_Gpio(pLed,kLedOn,kLedGreen);
                setLed_Gpio(pLed,kLedOn,kLedRed);
#endif
			}
			break;
#ifdef SKY_LED_TEST	
        case kLedStateGreenOn:
			{
		        setLed (pLed, kLedOn, kLedGreen);
			}
            break;

        case kLedStateGreenOff:
			{
		        setLed (pLed, kLedOff, kLedGreen);
			}
            break;

        case kLedStateRedOn:
			{
		        setLed (pLed, kLedOn, kLedRed);
			}
            break;

        case kLedStateRedOff:
			{
		        setLed (pLed, kLedOff, kLedRed);
			}
            break;
		case kLedStateGreenOnRedOff:
			{
				setLed (pLed, kLedOn, kLedGreen);
				setLed (pLed, kLedOff, kLedRed);
			}
			break;
			
		case kLedStateGreenOffRedOn:
			{
				setLed (pLed, kLedOff, kLedGreen);
				setLed (pLed, kLedOn, kLedRed);
			}
			break;
			
		case kLedStateGreenOffRedOff:
			{
				setLed (pLed, kLedOff, kLedGreen);
				setLed (pLed, kLedOff, kLedRed);
			}
			break;
			
#endif //SKY_LED_TEST
        case kLedStateFail:
#ifdef CONFIG_SKY_ETHAN
            setLed (pLed, kLedOn, kLedRed);
#else  /* SKY */
		   setLed_Gpio(pLed,kLedOn,kLedRed);	
#endif  /* SKY */	
            break;

        case kLedStateSlowBlinkContinues:
            pLed->blinkCountDown = kSlowBlinkCount;
            gTimerOnRequests++;
            break;

        case kLedStateFastBlinkContinues:
            pLed->blinkCountDown = kFastBlinkCount;
            gTimerOnRequests++;
            break;

        case kLedStateUserWpsInProgress:          /* 200ms on, 100ms off */
#ifdef CONFIG_SKY_ETHAN
            pLed->blinkCountDown = kSlowBlinkCount;
#else  /* SKY */
            pLed->blinkCountDown = kFastBlinkCount;
#endif  /* SKY */	
            gLedRunningCounter = 0;
            gTimerOnRequests++;
            break;             

        case kLedStateUserWpsError:               /* 100ms on, 100ms off */
            pLed->blinkCountDown = kFastBlinkCount;
            gLedRunningCounter = 0;
            gTimerOnRequests++;
            break;        

#ifdef SKY
		case kLedStateAmberFastBlinkContinues:
            pLed->blinkCountDown = kFastBlinkCount;
            gLedRunningCounter = 0;
            gTimerOnRequests++;
            break;
#endif

        case kLedStateUserWpsSessionOverLap:      /* 100ms on, 100ms off, 5 times, off for 500ms */
            pLed->blinkCountDown = kFastBlinkCount;
            gTimerOnRequests++;
            break;        

        default:
            printk("Invalid led state\n");
        }
    }

    // If gTimerOn is false, that means ledTimerExpire has already finished
    // running and has not restarted the timer.  Then we can start it here.
    // Otherwise, we only leave the gTimerOnRequests > 0 which will be
    // noticed at the end of the ledTimerExpire function.
    if (!gTimerOn && gTimerOnRequests > 0)
    {
        ledTimerStart();
        gTimerOn = TRUE;
        gTimerOnRequests = 0;
    }
    spin_unlock_irqrestore(&brcm_ledlock, flags);
}
#ifndef CONFIG_SKY_ETHAN
static void setLed_Gpio(PLED_CTRL pLed, unsigned short led_state, unsigned short led_type)
{
    short led_gpio;
    unsigned long flags;
	
/* This is to read the GPIO active levels */
   if(led_type==kLedRed)
   	led_gpio=pLed->ledRedGpio;
   else
   	led_gpio=pLed->ledGreenGpio;

 
//   printk("\n setLed_Gpio)led_gpio= 0x%x state=%s type=%s \n",led_gpio,led_state?"LED_ON":"LED_OFF",led_type?"AMBER":"WHITE");

   spin_lock_irqsave(&bcm_gpio_spinlock, flags);

/* First check whether led controller ->gpio pad enable bit is set if yes mask it */
    if(GPIO->LEDCtrl & GPIO_NUM_TO_MASK(led_gpio))
	GPIO->LEDCtrl &= ~GPIO_NUM_TO_MASK(led_gpio);	
   /* Set the gpio Dir register */
   GPIO->GPIODir |= GPIO_NUM_TO_MASK(led_gpio);

   if ((led_gpio & BP_GPIO_NUM_MASK) > 31)
        GPIO->GPIOCtrl |= GPIO_NUM_TO_MASK(led_gpio - 32);

   
  /* This is to clear the GPIO Mode ctrl bit to clear so that 
   alternate activity should not happen */
   if(GPIO->GPIOMode & GPIO_NUM_TO_MASK(led_gpio))
       GPIO->GPIOMode&=~GPIO_NUM_TO_MASK(led_gpio);
   
   if((led_state == kLedOn && !(led_gpio & BP_ACTIVE_LOW)) ||
        (led_state == kLedOff && (led_gpio & BP_ACTIVE_LOW)))
        GPIO->GPIOio |= GPIO_NUM_TO_MASK(led_gpio);
    else
        GPIO->GPIOio &= ~GPIO_NUM_TO_MASK(led_gpio);

   /* Release the spinlock */	
     spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);   

}

void ledToggle_fromFirmwareUpdate(short gpio,unsigned short led_type)
{
   short led_gpio = gpio;
   unsigned long flags;
/* This part is added to make wps requirement which is amber blink to work-NK */

    	if (led_gpio == -1)
       	return;
	
        spin_lock_irqsave(&bcm_gpio_spinlock, flags);

        GPIO->GPIODir |= GPIO_NUM_TO_MASK(led_gpio);
        GPIO->GPIOio ^= GPIO_NUM_TO_MASK(led_gpio);

        spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);
}

static void ledToggle_g(PLED_CTRL pLed,unsigned short led_type)
{
   short led_gpio;
   unsigned long flags;
/* This part is added to make wps requirement which is amber blink to work-NK */

        if (led_type == kLedRed)
        	led_gpio = pLed->ledRedGpio;
    	else
        	led_gpio = pLed->ledGreenGpio;


    	if (led_gpio == -1)
       	return;

        spin_lock_irqsave(&bcm_gpio_spinlock, flags);

        GPIO->GPIODir |= GPIO_NUM_TO_MASK(led_gpio);
        GPIO->GPIOio ^= GPIO_NUM_TO_MASK(led_gpio);

        spin_unlock_irqrestore(&bcm_gpio_spinlock, flags);



}

/* Added Newly for power on LED -NK */
void SetPowerLedOn(void)
{
    unsigned short gpio;
    PLED_CTRL pLed;	
   /* OFF ALL Led -NK */
     pLed=(PLED_CTRL) kmalloc((sizeof(LED_CTRL)), GFP_KERNEL);

/* On only Power led -Nk */
 
    if( BpGetBootloaderStopLedGpio( &gpio ) == BP_SUCCESS )
        pLed->ledRedGpio=gpio;setLed_Gpio( pLed, kLedOff,kLedRed);
       boardLedCtrl(kLedPwr,kLedStateOn);
    if(LED->ledMode & (LED_MODE_ON << GPIO_NUM_TO_LED_MODE_SHIFT(4)))
    {
        LED->ledMode &=~(LED_MODE_ON << GPIO_NUM_TO_LED_MODE_SHIFT(4));	
    }
	// This is for pin 6
    if(LED->ledMode & (LED_MODE_ON << GPIO_NUM_TO_LED_MODE_SHIFT(6)))
    {
        LED->ledMode &=~(LED_MODE_ON << GPIO_NUM_TO_LED_MODE_SHIFT(6));	
    }	
	/* This is for the sky+HD */
//	 boardLedCtrl(kLedSkyHd,kLedStateOn);
	kfree(pLed);
}


#else

void ledToggle_fromFirmwareUpdate(short gpio,unsigned short led_type)
{
    PLED_CTRL pLed;
	pLed = &gLedCtrl[kLedPwr];
	ledToggle(pLed);
}

#endif

#ifdef SKY
EXPORT_SYMBOL(ledToggle_fromFirmwareUpdate);
#endif /* SKY */
