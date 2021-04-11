/*****************************************************************************
 * Copyright, All rights reserved, for internal use only
*
* FILE: sky_dhcp6.h
*
* PROJECT: VDSL_Router
*
* MODULE: DHCPv6
*
* Date Created: 17/01/2014
*
* Description:  This file contains declarations of functions used in skydhcpc
*               application for use by other public applications.
*
* Notes:
*
*****************************************************************************/
#ifndef SKY_DHCP6_H
#define SKY_DHCP6_H

#include "os_defs.h"
#include "cms.h"

#define SKY_DHCPV6_OPTIONS_FILE             "/var/skydhcp6_options.txt" /* changed from /tmp to /var according to ticket SV-707 */
                                                      /* Name of file used to store the sky options to be used in DHCPv6
                                                       * messages. The file is used as a means of sharing data between
                                                       * components since the skydhcpc application and the relevant CMS
                                                       * objects required are private and hence cannot be used from the
                                                       * public dhcpv6 application.
                                                       */
#define SKY_DHCPV6_OPTION_STR_LEN          (288)      /* Maximum length of the buffer used to hold Sky DHCPv6 option strings
                                                       * (either Option 15 or Option 16). As a maximum of 4 MDM attributes
                                                       * are used in Option16, and each of them can be of maximum length
                                                       * 64 (DEFAULT_SER_FIELD_LEN), the size is (4 * 64) + 3. The '|'
                                                       * character is added between the attributes and hence addition of
                                                       * 3. When rounded to nearest word size, it is 288 (32 is considered
                                                       * as the word size).
                                                       */

CmsRet sky_getDhcp6cWaitState (UBOOL8 *pWaitState);

#endif //SKY_DHCP6_H
