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

#ifndef __IPCLOCK_H_INCLUDED
#define __IPCLOCK_H_INCLUDED

/*  File automatically generated by Reggae at 09/08/2011  17:32:20   */

#include "stt_basic_defs.h"
#include "access_macros.h"
#include "packing.h"

/*****************************************************************************************/
/* 1588 IPCLOCK slave partial registers                                                  */
/*****************************************************************************************/

/*****************************************************************************************/
/* Blocks offsets                                                                        */
/*****************************************************************************************/
#define CE_IPCLOCK_IPCLOCK_OFFSET	( 0x190A6C00 )
/*****************************************************************************************/
/* Functions offsets and addresses                                                       */
/*****************************************************************************************/
#define CE_IPCLOCK_IPCLOCK_TS_REGS_OFFSET 	( 0x00000000 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS	( CE_IPCLOCK_IPCLOCK_OFFSET + CE_IPCLOCK_IPCLOCK_TS_REGS_OFFSET )

/*   'i' is block index    */
/*   'j' is function index */
/*   'e' is function entry */
/*   'k' is register index */

/*****************************************************************************************/
/* TODRDCMD                                                                              */
/* SW should write the packets serial number into that register and read from the approp */
/* riate address accrding to RX/TX and PTP.                                              */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRDCMD_SN_SN_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRDCMD_SN_SN_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRDCMD_OFFSET ( 0x000000C2 )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRDCMD_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODRDCMD_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRDCMD_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRDCMD_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRDCMD_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRDCMD_ADDRESS ), (v) )

typedef struct
{
	/* SN */
	stt_uint16 sn	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODRDCMD_DTE ;


/*****************************************************************************************/
/* TODRX0_SN                                                                             */
/* SN of RX with PTP0                                                                    */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_SN_SN_SN_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_SN_SN_SN_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_SN_OFFSET ( 0x00000102 )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_SN_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_SN_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_SN_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_SN_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_SN_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_SN_ADDRESS ), (v) )

typedef struct
{
	/* SN */
	stt_uint16 sn	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODRX0_SN_DTE ;


/*****************************************************************************************/
/* TODRX0_0                                                                              */
/* TOD[63:48] of RX with PTP0                                                            */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_0_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_0_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_0_OFFSET ( 0x00000108 )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_0_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_0_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_0_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_0_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_0_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_0_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODRX0_0_DTE ;


/*****************************************************************************************/
/* TODRX0_1                                                                              */
/* TOD[47:32] of RX with PTP0                                                            */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_1_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_1_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_1_OFFSET ( 0x0000010A )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_1_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_1_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_1_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_1_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_1_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_1_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODRX0_1_DTE ;


/*****************************************************************************************/
/* TODRX0_2                                                                              */
/* TOD[31:16] of RX with PTP0                                                            */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_2_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_2_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_2_OFFSET ( 0x0000010C )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_2_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_2_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_2_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_2_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_2_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_2_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODRX0_2_DTE ;


/*****************************************************************************************/
/* TODRX0_3                                                                              */
/* TOD[15:0] of RX with PTP0                                                             */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_3_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_3_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_3_OFFSET ( 0x0000010E )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_3_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_3_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_3_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_3_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_3_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX0_3_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODRX0_3_DTE ;


/*****************************************************************************************/
/* TODRX3_SN                                                                             */
/* SN of RX with PTP3                                                                    */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_SN_SN_SN_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_SN_SN_SN_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_SN_OFFSET ( 0x00000112 )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_SN_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_SN_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_SN_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_SN_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_SN_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_SN_ADDRESS ), (v) )

typedef struct
{
	/* SN */
	stt_uint16 sn	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODRX3_SN_DTE ;


/*****************************************************************************************/
/* TODRX3_0                                                                              */
/* TOD[63:48] of RX with PTP3                                                            */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_0_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_0_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_0_OFFSET ( 0x00000118 )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_0_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_0_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_0_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_0_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_0_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_0_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODRX3_0_DTE ;


/*****************************************************************************************/
/* TODRX3_1                                                                              */
/* TOD[47:32] of RX with PTP3                                                            */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_1_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_1_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_1_OFFSET ( 0x0000011A )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_1_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_1_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_1_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_1_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_1_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_1_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODRX3_1_DTE ;


/*****************************************************************************************/
/* TODRX3_2                                                                              */
/* TOD[31:16] of RX with PTP3                                                            */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_2_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_2_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_2_OFFSET ( 0x0000011C )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_2_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_2_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_2_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_2_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_2_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_2_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODRX3_2_DTE ;


/*****************************************************************************************/
/* TODRX3_3                                                                              */
/* TOD[15:0] of RX with PTP3                                                             */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_3_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_3_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_3_OFFSET ( 0x0000011E )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_3_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_3_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_3_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_3_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_3_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODRX3_3_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODRX3_3_DTE ;


/*****************************************************************************************/
/* TODTX_SN                                                                              */
/* SN of TX                                                                              */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_SN_SN_SN_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_SN_SN_SN_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_SN_OFFSET ( 0x00000122 )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_SN_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_SN_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODTX_SN_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_SN_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODTX_SN_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_SN_ADDRESS ), (v) )

typedef struct
{
	/* SN */
	stt_uint16 sn	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODTX_SN_DTE ;


/*****************************************************************************************/
/* TODTX_0                                                                               */
/* TOD[63:48] of TX                                                                      */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_0_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_0_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_0_OFFSET ( 0x00000128 )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_0_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_0_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODTX_0_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_0_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODTX_0_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_0_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODTX_0_DTE ;


/*****************************************************************************************/
/* TODTX_1                                                                               */
/* TOD[47:32] of TX                                                                      */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_1_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_1_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_1_OFFSET ( 0x0000012A )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_1_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_1_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODTX_1_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_1_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODTX_1_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_1_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODTX_1_DTE ;


/*****************************************************************************************/
/* TODTX_2                                                                               */
/* TOD[31:16] of TX                                                                      */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_2_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_2_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_2_OFFSET ( 0x0000012C )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_2_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_2_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODTX_2_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_2_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODTX_2_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_2_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODTX_2_DTE ;


/*****************************************************************************************/
/* TODTX_3                                                                               */
/* TOD[15:0] of TX                                                                       */
/*****************************************************************************************/

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_3_TOD_TOD_VALUE             ( 0x0 )
#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_3_TOD_TOD_VALUE_RESET_VALUE ( 0x0 )


#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_3_OFFSET ( 0x0000012E )

#define CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_3_ADDRESS   	( CE_IPCLOCK_IPCLOCK_TS_REGS_ADDRESS + CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_3_OFFSET )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODTX_3_READ( r ) 	BL_READ_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_3_ADDRESS ), (r) )
#define BL_IPCLOCK_IPCLOCK_TS_REGS_TODTX_3_WRITE( v )	BL_WRITE_16( ( CE_IPCLOCK_IPCLOCK_TS_REGS_TODTX_3_ADDRESS ), (v) )

typedef struct
{
	/* TOD */
	stt_uint16 tod	: 16 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_TODTX_3_DTE ;


typedef struct
{
	/* TODRDCMD */
	IPCLOCK_IPCLOCK_TS_REGS_TODRDCMD_DTE todrdcmd __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* Reserved */
	stt_uint8 reserved1 [ 62 ] __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODRX0_SN */
	IPCLOCK_IPCLOCK_TS_REGS_TODRX0_SN_DTE todrx0_sn __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* Reserved */
	stt_uint8 reserved2 [ 4 ] __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODRX0_0 */
	IPCLOCK_IPCLOCK_TS_REGS_TODRX0_0_DTE todrx0_0 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODRX0_1 */
	IPCLOCK_IPCLOCK_TS_REGS_TODRX0_1_DTE todrx0_1 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODRX0_2 */
	IPCLOCK_IPCLOCK_TS_REGS_TODRX0_2_DTE todrx0_2 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODRX0_3 */
	IPCLOCK_IPCLOCK_TS_REGS_TODRX0_3_DTE todrx0_3 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* Reserved */
	stt_uint8 reserved3 [ 2 ] __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODRX3_SN */
	IPCLOCK_IPCLOCK_TS_REGS_TODRX3_SN_DTE todrx3_sn __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* Reserved */
	stt_uint8 reserved4 [ 4 ] __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODRX3_0 */
	IPCLOCK_IPCLOCK_TS_REGS_TODRX3_0_DTE todrx3_0 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODRX3_1 */
	IPCLOCK_IPCLOCK_TS_REGS_TODRX3_1_DTE todrx3_1 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODRX3_2 */
	IPCLOCK_IPCLOCK_TS_REGS_TODRX3_2_DTE todrx3_2 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODRX3_3 */
	IPCLOCK_IPCLOCK_TS_REGS_TODRX3_3_DTE todrx3_3 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* Reserved */
	stt_uint8 reserved5 [ 2 ] __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODTX_SN */
	IPCLOCK_IPCLOCK_TS_REGS_TODTX_SN_DTE todtx_sn __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* Reserved */
	stt_uint8 reserved6 [ 4 ] __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODTX_0 */
	IPCLOCK_IPCLOCK_TS_REGS_TODTX_0_DTE todtx_0 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODTX_1 */
	IPCLOCK_IPCLOCK_TS_REGS_TODTX_1_DTE todtx_1 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODTX_2 */
	IPCLOCK_IPCLOCK_TS_REGS_TODTX_2_DTE todtx_2 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;

	/* TODTX_3 */
	IPCLOCK_IPCLOCK_TS_REGS_TODTX_3_DTE todtx_3 __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
 __PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_TS_REGS_DTE ;

typedef struct
{
	/* ts_regs function */
	IPCLOCK_IPCLOCK_TS_REGS_DTE ts_regs __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
 __PACKING_ATTRIBUTE_STRUCT_END__
IPCLOCK_IPCLOCK_DTE ;

typedef struct
{
	/* IPCLOCK */
	IPCLOCK_IPCLOCK_DTE ipclock __PACKING_ATTRIBUTE_FIELD_LEVEL__ ;
}
__PACKING_ATTRIBUTE_STRUCT_END__ 
IPCLOCK_DTE ;
#endif /* IPCLOCK_H_INCLUDED */
