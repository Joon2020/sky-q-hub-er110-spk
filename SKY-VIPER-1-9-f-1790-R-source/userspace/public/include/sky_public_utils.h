#ifndef SKY_PUBLIC_UTILS_H
#define SKY_PUBLIC_UTILS_H
/*!****************************************************************************
 * Copyright, All rights reserved, for internal use only
*
* FILE: sky_public_utils.h
*
* PROJECT: SkyHub
*
* MODULE: cms_util
*
* Date Created: 26/03/2013
*
* Description:  This file includes  functions created by sky. This is part of cms_util library. This file contains sky specific functions
*  
*
* Notes:
*
*****************************************************************************/
#include "cms_msg.h"
#ifdef SKY_DIAGNOSTICS
/* Name of the file (with absolute path) containing information on the router
 * state/status that can be used by developers to diagnose issues reported on the router
 */
#define SKY_ENG_DIAG_INFO_FILE    "var/auxfs/diags/skyHMdiagsStat*"
#endif //SKY_DIAGNOSTICS

SINT32 sky_getFlashOffset(char *imagePtr, UINT32 imageLen);
CmsImageFormat sky_verifyImageTagAndSignature(const char *imageBuf, UINT32 imageLen);
CmsRet sendStartFlashWriteMsg(const char *imagePtr, UINT32 imageLen, UINT32 ptrLen,void *msgHandle, CmsMsgType msgType);

#endif //SKY_PUBLIC_UTILS_H
