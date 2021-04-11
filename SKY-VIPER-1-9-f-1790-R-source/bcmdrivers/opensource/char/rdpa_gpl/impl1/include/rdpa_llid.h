/*
* <:copyright-BRCM:2013:DUAL/GPL:standard
* 
*    Copyright (c) 2013 Broadcom Corporation
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


#ifndef _RDPA_LLID_H_
#define _RDPA_LLID_H_

/** \defgroup epon EPON Management
 * Objects in this group control EPON-related configuration
 */

/**
 * \defgroup llid LLID Management
 * \ingroup epon
 * Objects and functions in this group are used for LLID configuration
 * - Associating L1 channels with LLID group
 * - LLID-level operations
 * @{
 */

/** Max number of LLIDs */
#define RDPA_EPON_MAX_LLID      8

/** Max number of L1 data queues per LLID, excluding the management queue */
#define RDPA_EPON_LLID_QUEUES   8

/** @} end of llid Doxygen group */

/** @} end of epon Doxygen group */

#endif /* _RDPA_LLID_H_ */
