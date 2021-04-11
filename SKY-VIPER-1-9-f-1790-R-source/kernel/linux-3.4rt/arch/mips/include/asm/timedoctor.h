/*****************************************************************************/
/* Copyright (c) 2009 NXP B.V.                                               */
/*                                                                           */
/* This program is free software; you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by      */
/* the Free Software Foundation, using version 2 of the License.             */
/*                                                                           */
/* This program is distributed in the hope that it will be useful,           */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              */
/* GNU General Public License for more details.                              */
/*                                                                           */
/* You should have received a copy of the GNU General Public License         */
/* along with this program; if not, write to the Free Software               */
/* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307       */
/* USA.                                                                      */
/*                                                                           */
/*****************************************************************************/
#ifndef _ASM_TIMEDOCTOR_H
#define _ASM_TIMEDOCTOR_H

/* This file is just included for backwards compatiblity, new drivers should
 * just include linux/timedoctor.h
 */
#include <linux/timedoctor.h>

/* Legacy functions maintained for backwards compatiblity */
extern void timeDoctor_Info(unsigned int data1, unsigned int data2, unsigned int data3);
extern void timeDoctor_SetLevel(int level);
extern void timeDoctor_SetLevelAfter(int level, unsigned int samples);
extern void timeDoctor_Reset(void);


#endif /* _ASM_TIMEDOCTOR_H */
