/*
 Copyright 2007-2010 Broadcom Corp. All Rights Reserved.

 <:label-BRCM:2011:DUAL/GPL:standard
 
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

#define _BCMENET_LOCAL_
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/mii.h>
#include <linux/stddef.h>
#include <linux/ctype.h>
#include <linux/proc_fs.h>
#include <linux/string.h>
#include <linux/nbuff.h>
#include <board.h>
#include "boardparms.h"
#include <bcm_map_part.h>
#include "bcm_intr.h"
#include "bcmenet.h"
#include "bcmmii.h"
#include "ethswdefs.h"
#include "ethsw.h"
#include "bcmswshared.h"
#include "ethsw_phy.h"
#include "bcmswaccess.h"
#include "bcmsw.h"

#if defined(ENET_GPON_CONFIG)
extern struct net_device *gponifid_to_dev[MAX_GEM_IDS];
#endif
extern struct semaphore bcm_ethlock_switch_config;
extern uint8_t port_in_loopback_mode[TOTAL_SWITCH_PORTS];
extern atomic_t phy_write_ref_cnt;
extern atomic_t phy_read_ref_cnt;
extern int vport_cnt;  /* number of vports: bitcount of Enetinfo.sw.port_map */

// Software port index ---> real hardware param.
int switch_port_index[TOTAL_SWITCH_PORTS];
int switch_port_phyid[TOTAL_SWITCH_PORTS];
int switch_pport_phyid[TOTAL_SWITCH_PORTS] = {[0 ... (TOTAL_SWITCH_PORTS-1)] = -1};
int ext_switch_pport_phyid[TOTAL_SWITCH_PORTS] = {[0 ... (TOTAL_SWITCH_PORTS-1)] = -1};

BcmEnet_devctrl *pVnetDev0_g;

static uint8_t  hw_switching_state = HW_SWITCHING_ENABLED;

extern extsw_info_t extSwInfo;

int ethsw_port_to_phyid(int port)
{
    return switch_port_phyid[port];
}

void ethsw_init_table(BcmEnet_devctrl *pDevCtrl)
{
   ETHERNET_SW_INFO *sw;
   int map, cnt;
   int i, j;

   pVnetDev0_g = pDevCtrl;  /* Just a place to initialize the global variable */

   /* Initialize the External switch mapping table first */
   if (pDevCtrl->unit == 1) {
      for (i = 0; i < TOTAL_SWITCH_PORTS-1; i++) {
         ext_switch_pport_phyid[i] = pDevCtrl->EnetInfo[1].sw.phy_id[i];
      }
   }
   /* Now initialize the Internal switch mapping table */
   sw = &pDevCtrl->EnetInfo[0].sw;
   map = sw->port_map;
   bitcount(cnt, map);

   if ((cnt <= 0) || (cnt > BP_MAX_SWITCH_PORTS))
      return;

   for (i = 0, j = 0; i < cnt; i++, j++, map /= 2) {
      while ((map % 2) == 0) {
         map /= 2;
         j++;
      }
      switch_port_index[i] = j;
      switch_port_phyid[i] = sw->phy_id[j];
      switch_pport_phyid[j] = sw->phy_id[j];
//        ETHSW_PHY_SET_PHYID(j/*port*/, sw->phy_id[j] /*phy id*/);
   }

   return;
}

int ethsw_set_mac(int port, PHY_STAT ps)
{
    /* port = vport - 1 */
    uint16 sw_port = (uint16)switch_port_index[port]; 

    if (port_in_loopback_mode[sw_port])
    {
        printk("port_in_loopback_mode %d\n", sw_port);
        return 0;
    }

    return ethsw_set_mac_hw(sw_port, ps);
}

#if defined(CONFIG_BCM_ETH_PWRSAVE)
void ethsw_isolate_phy(int phyId, int isolate)
{
    uint16 v16;
    ethsw_phy_rreg(phyId, MII_CONTROL, &v16);
    if (isolate) {
        v16 |= MII_CONTROL_ISOLATE_MII;
    } else {
        v16 &= ~MII_CONTROL_ISOLATE_MII;
    }
    ethsw_phy_wreg(phyId, MII_CONTROL, &v16);
}
#endif



void ethsw_switch_power_off(void *context)
{
#ifdef DYING_GASP_API
    enet_send_dying_gasp_pkt();
#endif
}

// end power management routines

void ethsw_phyport_rreg(int port, int reg, uint16 *data)
{
   int unit = LOGICAL_PORT_TO_UNIT_NUMBER(port);
   int phy_id = enet_logport_to_phyid(port);
   int phys_port;

   if (unit > 0 && pVnetDev0_g->extSwitch->accessType != MBUS_MDIO)
   {
       phys_port = LOGICAL_PORT_TO_PHYSICAL_PORT(port);
       extsw_rreg_wrap(PAGE_INTERNAL_PHY_MII + phys_port, reg*2, (uint8 *)data, 2);
   }
   else
   {
       ethsw_phy_rreg(phy_id, reg, data);
   }
}

void ethsw_phyport_wreg(int port, int reg, uint16 *data)
{
   int unit = LOGICAL_PORT_TO_UNIT_NUMBER(port);
   int phy_id = enet_logport_to_phyid(port);
   int phys_port;

   if (unit > 0 && pVnetDev0_g->extSwitch->accessType != MBUS_MDIO)
   {
       phys_port = LOGICAL_PORT_TO_PHYSICAL_PORT(port);
       extsw_wreg_wrap(PAGE_INTERNAL_PHY_MII+phys_port, reg*2, (uint8 *)data, 2);
   }
   else
   {
       ethsw_phy_wreg(phy_id, reg, data);
   }
}

/*
**  caution: when unit = 0; the phy_ids for Internal and External PHY
**  could be duplicated duplicated,  thus the further restriction  mapping of 
** phy id -> phys_port  is not unique. 
*/    
static int ethsw_phyid_to_phys_port(int phy_id, int unit)
{
    int i;

    for (i = 0; i < MAX_SWITCH_PORTS && 
            (pVnetDev0_g->EnetInfo[unit].sw.phy_id[i] & 0x1f) != (phy_id & 0x1f) ; i++);

    if (i == MAX_SWITCH_PORTS)
    {
        BCM_ENET_DEBUG("%s phy-to-port association not found \n", __FUNCTION__);
        return -1;
    }

    return i;
}

void ethsw_phyport_rreg2(int phy_id, int reg, uint16 *data, int flags)
{
    int unit = flags & ETHCTL_FLAG_ACCESS_EXTSW_PHY? 1: 0;
    int phys_port;

    if (unit > 0 && pVnetDev0_g->extSwitch->accessType != MBUS_MDIO)
    {
        phys_port = ethsw_phyid_to_phys_port(phy_id, unit);
        if(phys_port != -1)
        {
            extsw_rreg_wrap(PAGE_INTERNAL_PHY_MII + phys_port, 
                reg*2, (uint8 *)data, 2);
        }
    }
    else
    {
        ethsw_phy_read_reg(phy_id, reg, data, flags & ETHCTL_FLAG_ACCESS_EXT_PHY);
    }
}

void ethsw_phyport_wreg2(int phy_id, int reg, uint16 *data, int flags)
{
    int unit = flags & ETHCTL_FLAG_ACCESS_EXTSW_PHY? 1: 0;
    int phys_port;

    if (unit > 0 && pVnetDev0_g->extSwitch->accessType != MBUS_MDIO)
    {
        phys_port = ethsw_phyid_to_phys_port(phy_id, unit);
        if(phys_port != -1)
        {
            extsw_wreg_wrap(PAGE_INTERNAL_PHY_MII + phys_port, reg*2, (uint8 *)data, 2);
        }
    }
    else
    {
        ethsw_phy_write_reg(phy_id, reg, data, flags & ETHCTL_FLAG_ACCESS_EXT_PHY);
    }
}

void ethsw_phy_config()
{
    ethsw_setup_led();
#if defined(CONFIG_BCM_ETH_HWAPD_PWRSAVE)
    ethsw_setup_hw_apd(1);
#endif

    ethsw_setup_phys();

    ethsw_phy_advertise_caps();

    ethsw_phy_apply_init_bp();
}

void ethsw_init_config(void)
{
    /* Initialize the Internal switch config */
    bcmeapi_ethsw_init_config();

    /* Initialize the external switch config */
    extsw_init_config();
}

/*
 * Function:
 *      bcmeapi_ioctl_ethsw_arl_access
 * Purpose:
 *      ARL table accesses
 * Returns:
 *      BCM_E_XXX
 */
int bcmeapi_ioctl_ethsw_arl_access(struct ethswctl_data *e)
{
    if (e->type == TYPE_GET) {
        BCM_ENET_DEBUG("e->mac: %02x %02x %02x %02x %02x %02x", e->mac[5],
                  e->mac[4], e->mac[3], e->mac[2], e->mac[1], e->mac[0]);
        BCM_ENET_DEBUG("e->vid: %d", e->vid);

        /* search internal switch */
        e->unit = 0;
        enet_arl_read( e->mac, &e->vid, &e->val );
        /* try external switch if entry is not found */
        if (bcm63xx_enet_isExtSwPresent() && (0 == e->val)) {
            e->unit = 1;
            enet_arl_read_ext( e->mac, &e->vid, &e->val );
        }
    } else if (e->type == TYPE_SET) {
        BCM_ENET_DEBUG("e->mac: %02x %02x %02x %02x %02x %02x", e->mac[5],
                   e->mac[4], e->mac[3], e->mac[2], e->mac[1], e->mac[0]);
        BCM_ENET_DEBUG("e->vid: %d", e->vid);

        /* if an external switch is present, e->unit will determine the
           access function */
        if (bcm63xx_enet_isExtSwPresent() && (1 == e->unit)) {
            enet_arl_write_ext(e->mac, e->vid, e->val);
        }
        else {
            enet_arl_write(e->mac, e->vid, e->val);
        }
    } else if (e->type == TYPE_DUMP) {
        enet_arl_access_dump();
        enet_arl_dump_multiport_arl();
        if (bcm63xx_enet_isExtSwPresent()) {
            enet_arl_access_dump_ext();
            enet_arl_dump_ext_multiport_arl();
        }
    } else if (e->type == TYPE_FLUSH) {
        /* Flush the ARL table */
        fast_age_all(1);
        if (bcm63xx_enet_isExtSwPresent()) {
            fast_age_all_ext(1);
        }
    } else {
        return BCM_E_PARAM;
    }

    return BCM_E_NONE;
}

int ethsw_set_hw_switching(uint32 state)
{
    down(&bcm_ethlock_switch_config);
/*Don't do anything if already enabled/disabled.
 *Enable is implemented by restoring values saved by disable_hw_switching().
 *This check is necessary to make sure we get correct behavior when
 *enable_hw_switching() is called without a preceeding disable_hw_switching() call.
 */

    if (hw_switching_state != state) {
        if (bcm63xx_enet_isExtSwPresent()) {
            if (state == HW_SWITCHING_ENABLED) {
                bcmsw_enable_hw_switching();
            }
            else {
                bcmsw_disable_hw_switching();
            }
        }
        else {
            if (state == HW_SWITCHING_ENABLED) {
                ethsw_enable_hw_switching();
            }
            else {
                ethsw_disable_hw_switching();
            }
        }
        hw_switching_state = state;
    }

    up(&bcm_ethlock_switch_config);
    return 0;
}
int ethsw_get_hw_switching_state(void)
{
    return hw_switching_state;
}

MODULE_LICENSE("GPL");
