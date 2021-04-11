/*****************************************************************************
 * Copyright, All rights reserved, for internal use only
*
* FILE: sky_routermodel_util.h
*
* PROJECT: SkyHub
*
* MODULE: 
*
* Date Created: 03/06/2013
*
* Description:  This file contains utility functions to handle ROI H/W
*
* Notes:
*
*****************************************************************************/
#ifndef SKY_ROUTER_MODEL_UTIL_H
#define SKY_ROUTER_MODEL_UTIL_H

#define SKY_ROUTER_MODEL_FILENAME "/var/skymodel"
#define SKY_ROUTER_MODEL_EXTENDED_FILENAME "/var/skymodel.extended"

typedef enum
{
	eSkyUnKnown = 0,
	eSkyHub,
	eSkyHubROI,
	eSkyVDSL,
	eSkyVDSLROI,
	eSkyVDSLDB,
	eSkyVDSLDBROI,

    eSkyViper, 
    eSkyViperROI,

    eSkyExtender
}SkyRouterModel;

/* Function to get the router model as UK version or ROI version */
SkyRouterModel sky_getRouterModel(); 

#endif //SKY_ROUTER_MODEL_UTIL_H
