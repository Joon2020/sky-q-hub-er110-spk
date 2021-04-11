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

/*******************************************************************************/
/*                                                                             */
/* packing.h                                                                   */
/*                                                                             */
/* This file allows setting the packing options for the compiler in use.       */
/*                                                                             */
/* The packing option controls how the compiler organizes structures in        */
/* memory. When a structure is defined the compiler uses the packing option    */
/* for setting the alignment of the structure's members.                       */
/*                                                                             */
/* Controlling the packing option is different across compilers. Although      */
/* ANSI suggests using #pargma, several other compiler, GNU for instance, use  */
/* other definitions from many reasons.                                        */
/*                                                                             */
/* A set of five #define declarations are used in the header (.h)              */
/* files that allows fitting to virtually any kind of compiler. The default    */
/* declarations suit the GNU compiler. If another compiler is used, one has    */
/* to modify these definitions.                                                */
/*                                                                             */
/* The five definitions allows setting the packing option in three locations.  */
/* 1. In file level - at the beginning and end of the file.                    */
/* 2. In structure level - before and after a structure definition.            */
/* 3. In member level - along each structure member.                           */
/*                                                                             */
/* Each header (.h) file for Lilac default its packing options                 */
/* to the GNU compiler but right after that, this packing.h file is included.  */
/* This way, the customer can make the necessary changes in this file without  */
/* modifying the original file.                                                */
/*                                                                             */
/* The following remarks shows the default definition for GNU compiler in      */
/* order to pack the structures to byte addressing.                            */
/*                                                                             */
/* NOTE:                                                                      */
/*                                                                            */
/* You must specify these packing options to instruct the compiler to fully   */
/* pack the structures, that is, no alignment and no padding at all!!!        */
/*                                                                            */
/******************************************************************************/

#ifndef __PACKING_H_
#define __PACKING_H_


#define __PACKING_ATTRIBUTE_FILE_HEADER__
#define __PACKING_ATTRIBUTE_FILE_CLOSE__
#define __PACKING_ATTRIBUTE_STRUCT_HEADER__
#define __PACKING_ATTRIBUTE_STRUCT_CLOSE__ 

#ifndef VXWORKS
#define __PACKING_ATTRIBUTE_STRUCT_END__  __attribute__ ((packed))
 #define __PACKING_ATTRIBUTE_FIELD_LEVEL__ 
#else 
#define __PACKING_ATTRIBUTE_STRUCT_END__  
#define __PACKING_ATTRIBUTE_FIELD_LEVEL__ __attribute__ ((packed))
#endif

#endif


