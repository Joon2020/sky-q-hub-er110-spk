/*!****************************************************************************
 * Copyright, All rights reserved, for internal use only
 *
 * FILE: oal_timestamp_sky.c
 *
 * PROJECT: SkyHub
 *
 * MODULE: cms_util
 *
 * Description:  This file contains OS dependent functions that are required
 *               for Sky specific implementation. This is part of cms_util
 *               library.
 *               
 * Notes:        It is organised as a separate file mailnly for the use of
 *               "X_OPEN_SOURCE" macro, which is required before inclusion of
 *               "time.h" file, required for using "strptime" function. Using
 *               it in the common file containing other time functions,
 *               creates conflict.
 *
 *****************************************************************************/

#define _XOPEN_SOURCE /* Required for using strptime function */
#include <time.h>     /* For time related API definitions */
#include <stdlib.h>   /* For using utilities like memset */
#include "cms.h"      /* For return code definition */
#include "cms_log.h"  /* For using log messages */

CmsRet oalTms_parseXSIDateTime(char *buf, struct tm *timeInfo)
{
   CmsRet ret = CMSRET_SUCCESS;
   char *parseRet = NULL;

   cmsLog_debug ("Enter %s", __FUNCTION__);

   if ((NULL == buf) || (NULL == timeInfo))
   {
       cmsLog_error ("Invalid input arguments. Exit %s", __FUNCTION__);
       return (CMSRET_INVALID_ARGUMENTS);
   }

   memset (timeInfo, 0, sizeof (struct tm));
   parseRet = strptime(buf, "%Y-%m-%dT%H:%M:%S%z", timeInfo);
   if (NULL == parseRet)
   {
       cmsLog_error ("Failure to parse XSIDateTime string. Exit %s", __FUNCTION__);
       return (CMSRET_INTERNAL_ERROR);
   }

   cmsLog_debug ("HH:MM:SS = %d:%d:%d", timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);

   cmsLog_debug ("Exit %s", __FUNCTION__);

   return ret;
} /* End of oalTms_parseXSIDateTime() */

