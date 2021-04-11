/*!****************************************************************************
 * Copyright, All rights reserved, for internal use only
*
* FILE: skyLed.c
*
* PROJECT: VIPER
*
* MODULE: cms_core
*
* Date Created: 04/11/2013
*
* Description:  This file includes  functions Related LEDs  added by sky
*
* Notes:
*
*****************************************************************************/
#if 0
#include "sky_led.h"
#include "cms_boardioctl.h"


static void skyLed_setPowerOn(void)
{
	devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedPwr, kLedStateOn, NULL);
}

#ifndef SKY_EXTENDER
static void skyLed_setWanLinkUp(void)
{
#ifdef CONFIG_SKY_ETHAN
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedAdsl, kLedStateAmber, NULL);
#else
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedAdsl, kLedStateFail, NULL);
#endif //CONFIG_SKY_ETHAN
}

static void skyLed_setWanConnected(void)
{
	devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedAdsl, kLedStateOn, NULL);
}

static void skyLed_setWanLinkDown(void)
{
	devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedWanData, kLedStateOff, NULL);
}

static void skyLed_setWanTraining(void)
{
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedAdsl, kLedStateUserWpsError, NULL);
}
#endif // SKY_EXTENDER

void skyLed_setWirelessOn(void)
{
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedWll, kLedStateOn, NULL);
}

void skyLed_setWirelessOff(void)
{
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedWll, kLedStateOff, NULL);
}

void skyLed_setSkyHDConnected(void)
{
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedSkyHd, kLedStateOn, NULL);
}

void skyLed_setSkyHDDisconnected(void)
{
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedSkyHd, kLedStateOff, NULL);
}

void skyLed_setSkyHDDataTransfer(void)
{
   devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedSkyHd, kLedStateSlowBlinkContinues, NULL);
}

void skyLed_setFirmwareUpgradeStart()
{
	// OFF DSL and WIRELESS LEDs
	devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedAdsl, kLedStateOff, NULL);
	devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedWll, kLedStateOff, NULL);

	//GREEN - BLINK, RED - OFF
	devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedPwr, kLedStateSlowBlinkContinues, NULL);

}

void skyLed_setFirmwareUpgradeFail()
{
	
	// GREEN - OFF, RED-ON
	devCtl_boardIoctl(BOARD_IOCTL_LED_CTRL, 0, NULL, kLedPwr, kLedStateFail, NULL);
}

void skyLed_updateWanLed(CmsMsgType wanStatus, char *ifName)
{
#ifndef SKY_EXTENDER
    cmsLog_debug(" Entered: wanStatus = %d, interface=%s", wanStatus, ifName);
	int checkInterface=IF_ALL;
	char interface[10];

	switch (wanStatus) {
		case CMS_MSG_WAN_LINK_UP:
			if(rutIpt_isWanConnected(interface)) {
				cmsLog_debug("Setting Wan LED to CONNECTED state");
				skyLed_setWanConnected();
			}
			else {
				cmsLog_debug("Setting Wan LED to FAILED state");
				skyLed_setWanLinkUp();
			}
			break;

		case CMS_MSG_WAN_LINK_DOWN:
		case CMS_MSG_WAN_CONNECTION_DOWN:			
			if(cmsUtl_strstr(ifName,"DSL"))		
				checkInterface = IF_ETH;
			else if(cmsUtl_strstr(ifName,"eth"))		
				checkInterface = IF_DSL;
			else
				checkInterface = IF_ALL;
			
			if(rutIpt_isWanConnected(interface))
			{
				cmsLog_debug("Setting Wan LED to CONNECTED state");
				skyLed_setWanConnected();
			}
			else if(rut_isWanLinkUp(checkInterface))
			{
				cmsLog_debug("Setting Wan LED to FAILED state");
				skyLed_setWanLinkUp();
			}
			else
			{
				cmsLog_debug("Setting Wan LED to DISCONNECTED state");
				skyLed_setWanLinkDown();
			}

			break;
		case CMS_MSG_SKY_WAN_TRAINING:
			cmsLog_debug("Setting Wan LED to TRAINING state");
			skyLed_setWanTraining();
			break;


		case CMS_MSG_WAN_CONNECTION_UP:
			cmsLog_debug("Setting Wan LED to CONNECTED state");
			skyLed_setWanConnected();
			break;
			
		default:
			break;
		
	}
#endif // SKY_EXTENDER
}

void skyLed_restoreLEDState()	
{
	InstanceIdStack iidStack = EMPTY_INSTANCE_ID_STACK;
	_WlBaseCfgObject *wlBaseCfgObj;
	UBOOL8 wirelessOn  = FALSE;
	InstanceIdStack iidStack2 = EMPTY_INSTANCE_ID_STACK;
	SkyAnyTimeListObject *anytimeObj;

	cmsLog_error("Entry");

	//On Power  LED State
	cmsLog_error(" Power LED On");
	skyLed_setPowerOn();

#ifndef SKY_EXTENDER
	//restore DSL LED status
	cmsLog_error(" restoring DSL LED Status");
	skyLed_updateWanLed(CMS_MSG_WAN_CONNECTION_DOWN, "NONE");
#endif // SKY_EXTENDER

	//Restore Wireless LED status

	cmsLog_error(" restoring Wireless LED Status");
	while(cmsObj_getNext(MDMOID_WL_BASE_CFG, &iidStack, (void **) &wlBaseCfgObj) == CMSRET_SUCCESS)
	{
		cmsLog_error("wlBaseCfgObj->WlEnbl - %d",wlBaseCfgObj->wlEnbl);
		if(wlBaseCfgObj->wlEnbl)
		{
			wirelessOn = TRUE;
			skyLed_setWirelessOn();
			break;
		}
		cmsObj_free((void **) &wlBaseCfgObj);
	}

	if(wirelessOn == TRUE)
		skyLed_setWirelessOn();
	else
		skyLed_setWirelessOff();

	//Restore SkyHD LED status
	cmsLog_error(" restoring SkyHD LED Status");
	if(cmsObj_getNext(MDMOID_SKY_ANY_TIME_LIST, &iidStack2, (void **) &anytimeObj) == CMSRET_SUCCESS)
	{
		skyLed_setSkyHDConnected(); 	
		cmsObj_free((void **) &anytimeObj);
	}
	else
		skyLed_setSkyHDDisconnected();
}
#endif
