/*
<:copyright-BRCM:2002:DUAL/GPL:standard

   Copyright (c) 2002 Broadcom Corporation
   All Rights Reserved

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License, version 2, as published by
the Free Software Foundation (the "GPL").

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.


A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
Boston, MA 02111-1307, USA.

:>
 */
#ifndef _BCMSWACCESS_H_
#define _BCMSWACCESS_H_

#if defined(CONFIG_BCM968500) || defined(CONFIG_BCM96838)
#include "ethsw_runner.h"   /* This should actually go in bcmswaccess_runner.h */
#else
#include "bcmswaccess_dma.h"
#endif /* !CONFIG_BCM968500 */
void bcmsw_pmdio_rreg(int page, int reg, uint8 *data, int len);
void bcmsw_pmdio_wreg(int page, int reg, uint8 *data, int len);
void get_ext_switch_access_info(int usConfigType, int *bus_type, int *spi_id);
int enet_ioctl_ethsw_regaccess(struct ethswctl_data *e);
int enet_ioctl_ethsw_pmdioaccess(struct net_device *dev, struct ethswctl_data *e);
int enet_ioctl_ethsw_info(struct net_device *dev, struct ethswctl_data *e);
int enet_ioctl_phy_cfg_get(struct net_device *dev, struct ethswctl_data *e);

#define bcmeapi_ioctl_phy_cfg_get enet_ioctl_phy_cfg_get

#endif /* _BCMSWACCESS_H_ */

