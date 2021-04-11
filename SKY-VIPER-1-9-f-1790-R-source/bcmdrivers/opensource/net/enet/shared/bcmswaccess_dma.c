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

#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/netdevice.h>
#include <linux/mii.h>
#include <asm/uaccess.h>
#include <board.h>
#include "boardparms.h"
#include <bcm_map_part.h>
#include "bcmSpiRes.h"
#include <linux/bcm_log.h>
#include <bcm/bcmswapitypes.h>
#include "bcmtypes.h"
#include "bcmswshared.h"
#include "bcmswaccess.h"
#include "bcmmii.h"
#include "ethsw_phy.h"
#include "bcmsw.h"

#define IS_PHY_ADDR_FLAG 0x80000000
#define IS_SPI_ACCESS    0x40000000

#define PORT_ID_M 0xF
#define PORT_ID_S 0
#define PHY_REG_M 0x1F
#define PHY_REG_S 4

extern int dump_enable;

extern struct semaphore bcm_ethlock_switch_config;
#if defined(CONFIG_BCM_ETH_PWRSAVE)
extern spinlock_t bcmsw_pll_control_lock;
#endif
extern spinlock_t bcm_extsw_access;
/* The external switch physical port to phyid mapping */
extern int switch_pport_phyid[TOTAL_SWITCH_PORTS];
extern int ext_switch_pport_phyid[TOTAL_SWITCH_PORTS];


#define SWITCH_ADDR_MASK 0xFFFF
extern void ethsw_phy_rreg(int phy_id, int reg, uint16 *data);
extern void ethsw_phy_wreg(int phy_id, int reg, uint16 *data);

static void bcmsw_spi_select(int bus_num, int spi_ss, int chip_id, int page)
{
    unsigned char buf[3];
    int tryCount = 0;
    static int spiRdyErrCnt = 0;

    /* SPIF status bit must be clear */
    while(1)
    {
       buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_READ |
           ((chip_id & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);
       buf[1] = (unsigned char)BCM5325_SPI_STS;
       BcmSpiSyncTrans(buf, buf, BCM5325_SPI_PREPENDCNT, 1, bus_num, spi_ss);
       if (buf[0] & BCM5325_SPI_CMD_SPIF)
       {
          if ( spiRdyErrCnt < 10 )
          {
             spiRdyErrCnt++;
             printk("bcmsw_spi_select: SPIF set, not ready\n");
          }
          else if ( 10 == spiRdyErrCnt )
          {
             spiRdyErrCnt++;
             printk("bcmsw_spi_select: SPIF set, not ready - suppressing prints\n");
          }
          tryCount++;
          if (tryCount > 10)
          {
             return;
          }
       }
       else
       {
          break;
       }
    }

    /* Select new chip */
    buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_WRITE |
        ((chip_id & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);

    /* Select new page */
    buf[1] = PAGE_SELECT;
    buf[2] = (char)page;
    BcmSpiSyncTrans(buf, NULL, 0, sizeof(buf), bus_num, spi_ss);
}

void bcmsw_spi_rreg(int bus_num, int spi_ss, int chip_id, int page, int reg, uint8 *data, int len)
{
    unsigned char buf[64];
    int rc;
    int i;
    int max_check_spi_sts;

    BCM_ENET_LINK_DEBUG("%s, spi_ss = %d, chip_id = %d, page = %d, " 
        "reg = %d, len = %d \n", (bus_num == LEG_SPI_BUS_NUM)?"Legacy SPI":"High Speed SPI", spi_ss, chip_id, page, reg, len);
    if (bus_num > HS_SPI_BUS_NUM) {
        printk("Invalid SPI bus number: %d \n", bus_num);
        return;
    }

    spin_lock_bh(&bcm_extsw_access);

    bcmsw_spi_select(bus_num, spi_ss, chip_id, page);

    /* write command byte and register address */
    buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_READ |
        ((chip_id & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);
    buf[1] = (unsigned char)reg;
    rc = BcmSpiSyncTrans(buf, buf, BCM5325_SPI_PREPENDCNT, 1, bus_num, spi_ss);

    if (rc == SPI_STATUS_OK) {
        max_check_spi_sts = 0;
        do {
            /* write command byte and read spi_sts address */
            buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_READ |
                ((chip_id & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);
            buf[1] = (unsigned char)BCM5325_SPI_STS;
            rc = BcmSpiSyncTrans(buf, buf, BCM5325_SPI_PREPENDCNT, 1, bus_num, spi_ss);
            if (rc == SPI_STATUS_OK) {
                /* check the bit 0 RACK bit is set */
                if (buf[0] & BCM5325_SPI_CMD_RACK) {
                    break;
                }
                mdelay(1);
            } else {
                printk("BcmSpiSyncTrans failure \n");
                break;
            }
        } while (max_check_spi_sts++ < 10);


        if (rc == SPI_STATUS_OK) {
            buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_READ |
                ((chip_id & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);
            buf[1] = (unsigned char)0xf0;
            rc = BcmSpiSyncTrans(buf, buf, BCM5325_SPI_PREPENDCNT, len, bus_num, spi_ss);
            if (rc == SPI_STATUS_OK) {
                /* Write the data out in LE format to the switch */
                for (i = 0; i < len; i++)
                    *(data + i) = buf[i];
            } else {
                printk("BcmSpiSyncTrans failure \n");
            }
        }
        BCM_ENET_LINK_DEBUG( 
            "Data: %02x%02x%02x%02x %02x%02x%02x%02x \n", *(data+7), *(data+6),
            *(data+5), *(data+4), *(data+3), *(data+2), *(data+1), *(data+0));
   }
   spin_unlock_bh(&bcm_extsw_access);
}

void bcmsw_spi_wreg(int bus_num, int spi_ss, int chip_id, int page, int reg, uint8 *data, int len)
{
    unsigned char buf[64];
    int i;

    BCM_ENET_LINK_DEBUG("%s, spi_ss = %d, chip_id = %d, page = %d, " 
        "reg = %d, len = %d \n", (bus_num == LEG_SPI_BUS_NUM)?"Legacy SPI":"High Speed SPI", spi_ss, chip_id, page, reg, len);
    BCM_ENET_LINK_DEBUG("Data: %02x%02x%02x%02x %02x%02x%02x%02x \n",
        *(data+7), *(data+6), *(data+5), *(data+4), *(data+3), *(data+2),
        *(data+1), *(data+0));

    if (bus_num > HS_SPI_BUS_NUM) {
        printk("Invalid SPI bus number: %d \n", bus_num);
        return;
    }

    spin_lock_bh(&bcm_extsw_access);

    bcmsw_spi_select(bus_num, spi_ss, chip_id, page);

    buf[0] = BCM5325_SPI_CMD_NORMAL | BCM5325_SPI_CMD_WRITE |
        ((chip_id & BCM5325_SPI_CHIPID_MASK) << BCM5325_SPI_CHIPID_SHIFT);

    buf[1] = (char)reg;

    for (i = 0; i < len; i++) {
        /* Write the data out in LE format to the switch */
        buf[BCM5325_SPI_PREPENDCNT+i] = *(data + i);
    }

    BCM_ENET_LINK_DEBUG("%02x:%02x:%02x:%02x%02x%02x\n", buf[5], buf[4], buf[3],
                    buf[2], buf[1], buf[0]);
    BcmSpiSyncTrans(buf, NULL, 0, len+BCM5325_SPI_PREPENDCNT, bus_num, spi_ss);

    spin_unlock_bh(&bcm_extsw_access);
}

static int bcmsw_phy_access(struct ethswctl_data *e, int access_type)
{
    uint16 phy_reg_val;
    uint8 data[8] = {0};
    int ext_bit = 0, phy_id, reg_offset; 

    if (access_type == MBUS_UBUS)
        phy_id = switch_pport_phyid[e->offset & PORT_ID_M];
    else
        phy_id = ext_switch_pport_phyid[e->offset & PORT_ID_M];

    reg_offset = (e->offset >> PHY_REG_S) & PHY_REG_M;
    
    if (e->type == TYPE_GET) {
        down(&bcm_ethlock_switch_config);
        if (access_type == MBUS_UBUS) {
            ethsw_phy_rreg(phy_id, reg_offset, &phy_reg_val);
        } else {
            ext_bit = 1;
            ethsw_phy_read_reg(phy_id, reg_offset, &phy_reg_val, ext_bit);
        }
        up(&bcm_ethlock_switch_config);
        BCM_ENET_LINK_DEBUG("phy_reg_val =0x%x \n", phy_reg_val);
        data[1] = phy_reg_val >> 8;
        data[0] = phy_reg_val & 0xFF;
        if (copy_to_user((void*)(&e->data), (void*)&data, 4)) {
            return -EFAULT;
        }
        BCM_ENET_LINK_DEBUG("e->data:%x %x  %x %x \n", e->data[3], e->data[2], e->data[1], e->data[0]);
    } else {
        BCM_ENET_LINK_DEBUG("Phy Data: %x %x %x %x \n", e->data[3], e->data[2], 
            e->data[1], e->data[0]);
        phy_reg_val = (e->data[1] << 8) | e->data[0];
        BCM_ENET_LINK_DEBUG("phy_reg_val = %x \n", phy_reg_val);
        down(&bcm_ethlock_switch_config);
        if (access_type == MBUS_UBUS) {
            ethsw_phy_wreg(phy_id, reg_offset, &phy_reg_val);
        } else {
            ext_bit = 1;
            ethsw_phy_write_reg(phy_id, reg_offset, &phy_reg_val, ext_bit);
        }
        up(&bcm_ethlock_switch_config);
    }
    return 0;
}

int bcmeapi_ioctl_ethsw_spiaccess(int bus_num, int spi_id, int chip_id, struct ethswctl_data *e) 
{
    int page, reg;
    uint8 data[8] = {0};

    if (e->offset & IS_PHY_ADDR_FLAG) {
        return bcmsw_phy_access(e, MBUS_SPI);
    } else {
        page = (e->offset >> 8) & 0xFF;
        reg = e->offset & 0xFF;
        if (e->type == TYPE_GET) {
            bcmsw_spi_rreg(bus_num, spi_id, chip_id, page,
                           reg, data, e->length);
            if (copy_to_user((void*)(e->data), (void*)data, e->length))
                return -EFAULT;
        } else {
            bcmsw_spi_wreg(bus_num, spi_id, chip_id, page,
                           reg, e->data, e->length);
        }
    }
    return 0;
}

void ethsw_read_reg(int addr, uint8 *data, int len)
{
    volatile uint8 *base = (volatile uint8 *)
       (SWITCH_BASE + (addr & SWITCH_ADDR_MASK));

#if defined(CONFIG_BCM_ETH_PWRSAVE)
    unsigned long flags;
    spin_lock_irqsave(&bcmsw_pll_control_lock, flags);
    ethsw_phy_pll_up(0);
#endif

    if (len == 1) {
        *data = *base;
    } else if (len == 2) {
        *(uint16 *)data = swab16(*(uint16 *)base);
    } else if (len == 4) {
        *(uint32 *)data = swab32(*(uint32 *)base);
    } else if (len == 6) {
        if ((addr % 4) == 0) {
            *(uint32 *)data = swab32(*(uint32 *)base);
            *(uint16 *)(data + 4) = swab16(*(uint16 *)(base + 4));
        } else {
            *(uint16 *)data = swab16(*(uint16 *)base);
            *(uint32 *)(data + 2) = swab32(*(uint32 *)(base + 2));
        }
    } else if (len == 8) {
        *(uint32 *)data = swab32(*(uint32 *)base);
        *(uint32 *)(data + 4) = swab32(*(uint32 *)(base + 4));
    }
#if defined(CONFIG_BCM_ETH_PWRSAVE)
    spin_unlock_irqrestore(&bcmsw_pll_control_lock, flags);
#endif
}

void ethsw_write_reg(int addr, uint8 *data, int len)
{
    volatile uint8 *base = (volatile uint8 *)
       (SWITCH_BASE + (addr & SWITCH_ADDR_MASK));
    int val32;

#if defined(CONFIG_BCM_ETH_PWRSAVE)
    unsigned long flags;
    spin_lock_irqsave(&bcmsw_pll_control_lock, flags);
    ethsw_phy_pll_up(0);
#endif

    if (len == 1) {
        *base = *data;
    } else if (len == 2) {
        *(uint16 *)base = swab16(*(uint16 *)data);
    } else if (len == 4) {
        if ( (int)data & 0x3 )
        {
           val32 = ((*(uint16 *)data) << 16) | ((*(uint16 *)(data+2)) << 0);
           *(uint32 *)base = swab32(val32);
        }
        else
        {
           *(uint32 *)base = swab32(*(uint32 *)data);
        }
    } else if (len == 6) {
        if (addr % 4 == 0) {
            if ( (int)data & 0x3 )
            {
               val32 = ((*(uint16 *)data) << 16) | ((*(uint16 *)(data+2)) << 0);
               *(uint32 *)base = swab32(val32);
            }
            else
            {
               *(uint32 *)base = swab32(*(uint32 *)data);
            }
            *(uint16 *)(base + 4) = swab16(*(uint16 *)(data + 4));
        } else {
            *(uint16 *)base = swab16(*(uint16 *)data);
            if ( (int)(data+2) & 0x3 )
            {
               val32 = ((*(uint16 *)(data+2)) << 16) | ((*(uint16 *)(data+4)) << 0);
               *(uint32 *)(base + 2) = swab32(val32);
            }
            else
            {
               *(uint32 *)(base + 2) = swab32(*(uint32 *)(data + 2));
            }
        }
    } else if (len == 8) {
         if ( (int)data & 0x3 )
         {
            val32 = ((*(uint16 *)data) << 16) | ((*(uint16 *)(data+2)) << 0);
            *(uint32 *)base = swab32(val32);
            val32 = ((*(uint16 *)(data+4)) << 16) | ((*(uint16 *)(data+6)) << 0);
            *(uint32 *)(base + 4) = swab32(val32);
         }
         else
         {
            *(uint32 *)base = swab32(*(uint32 *)data);
            *(uint32 *)(base + 4) = swab32(*(uint32 *)(data + 4));
         }
    }
#if defined(CONFIG_BCM_ETH_PWRSAVE)
    spin_unlock_irqrestore(&bcmsw_pll_control_lock, flags);
#endif
}

