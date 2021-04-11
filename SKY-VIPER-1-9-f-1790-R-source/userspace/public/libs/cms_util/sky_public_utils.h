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

SINT32 sky_getFlashOffset(char *imagePtr, UINT32 imageLen);
CmsImageFormat sky_verifyImageTagAndSignature(const char *imageBuf, UINT32 imageLen);

#endif //SKY_PUBLIC_UTILS_H
