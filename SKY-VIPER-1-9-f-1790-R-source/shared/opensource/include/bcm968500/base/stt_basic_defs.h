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

#ifndef __STT_BASIC_DEFS_H_
#define __STT_BASIC_DEFS_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Basic data types */
typedef char				stt_char;

typedef signed	 char		stt_int8;
typedef unsigned char		stt_uint8;

typedef signed	 short int	stt_int16;
typedef unsigned short int	stt_uint16;

typedef signed	 long int	stt_int32;
typedef unsigned long int	stt_uint32;

typedef stt_uint32			STT_BOOLEAN_DTE;

#define CE_STT_OK			0
#define CE_STT_ERROR		1

#define CE_STT_FALSE		0
#define CE_STT_TRUE			1

#define CE_NULL 0


#ifndef VXWORKS
 typedef stt_int32			STATUS;
 #ifndef ERROR
  #define ERROR -1
 #endif
#endif

typedef stt_int32 STT_ERROR_DTE;
typedef stt_int32 STT_TIMEOUT_DTE;

#ifndef LINUX_KERNEL
#define EXPORT_SYMBOL(m)   /*  */
#endif

#ifdef __cplusplus
}
#endif	/* __cplusplus */

#endif	/* __STT_BASIC_DEFS_H_ */

