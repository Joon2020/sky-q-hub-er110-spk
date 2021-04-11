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

#include "bl_os_wraper.h"
#include "bl_lilac_brd.h"
#include "bl_lilac_drv_mac.h"
#include "bl_lilac_mips_blocks.h"
#include "bl_lilac_gpio.h"
#include "bl_lilac_eth_common.h"
#include "bl_lilac_pin_muxing.h"
#include "boardparms.h"

#define DELAY_FOR_PHY_PATCH 3

uint8_t tbi_phy_addr [] =
{
    TULIP_TBI_SGMII_0,
    TULIP_TBI_SGMII_1,
    TULIP_TBI_SGMII_2,
    TULIP_TBI_SGMII_3,
    TULIP_TBI_SGMII_4,
    TULIP_TBI_SGMII_5
};
EXPORT_SYMBOL( tbi_phy_addr ) ;

extern BL_LILAC_SOC_STATUS bl_lilac_plat_mem_rsrv_get_by_name(char *name, void **addr, unsigned long *size);

/*=======================================================================*/
BL_LILAC_SOC_STATUS bl_lilac_mem_rsrv_get_by_name(char *name, void **addr, unsigned long *size)
{
	return bl_lilac_plat_mem_rsrv_get_by_name(name, addr, size);
}
EXPORT_SYMBOL( bl_lilac_mem_rsrv_get_by_name );


/*=======================================================================*/
void lilac_restart(char *command)
{
	unsigned long flags;
	char boardName[BP_BOARD_ID_LEN];
	unsigned short pin,reset;

	if (!BpGetBoardId(boardName))
	{
		if (BpGetBoardResetGpio( &pin ) == BP_SUCCESS)
		{
			reset = (pin & BP_ACTIVE_LOW) >> 15;
			pin = pin & ~BP_ACTIVE_LOW;
			BL_LOCK_SECTION(flags);
			fi_bl_lilac_mux_connect_pin(pin,  GPIO, 0, MIPSC);
			bl_lilac_gpio_set_mode(pin, BL_LILAC_GPIO_MODE_OUTPUT);
			/* command is boot once partition number, "0" - partition 0, "1" - partition 1 */
			if(command)
			{
				/* write boot once partition number to $S8 register along with 0xcafecaf0 pattern */
				flags = 0xcafecaf0 | (*command - 0x30);
				__asm__ __volatile__("move\t$30, %z0\n\t" : : "Jr" ((unsigned int)(flags)));
			}
			bl_lilac_gpio_write_pin(pin, reset);
			while(1);
		}
		else
			printk("restart functionality not supported for %s board!\n", boardName);
	}
	else
		printk("restart functionality not supported for Unknown board!\n");
}
EXPORT_SYMBOL( lilac_restart );

static int bl_lilac_lane_enabled(int id)
{
	char boardName[BP_BOARD_ID_LEN];

	switch(bl_lilac_soc_id())
	{
		case BL_00000:
			if (!BpGetBoardId(boardName))
				return 1;
			else
				return 0;

		case BL_23515:
		case BL_23516:
		case BL_23530:
		case BL_23531:
		default:
			return 0;

		case BL_23571:
			if(id)
				return 0;
			else
				return 1;

		case BL_23570:
		case BL_25550:
		case BL_25580:
		case BL_25590:
			return 1;
	}
}

int bl_lilac_pci_lane_enabled(int id)
{
	return bl_lilac_lane_enabled(id);
}
EXPORT_SYMBOL(bl_lilac_pci_lane_enabled);

int bl_lilac_usb_hcd_enabled(int id)
{
	return bl_lilac_lane_enabled(id);
}
EXPORT_SYMBOL(bl_lilac_usb_hcd_enabled);

void bl_lilac_init_qsgmii_phy()
{
	char boardName[BP_BOARD_ID_LEN];
	int xi_phy_address;

	if (!BpGetBoardId(boardName))
		if (!bpstrcmp(boardName,"968751LUPINE2"))
			return;

	xi_phy_address = BpGetPhyAddr(0,0);
    fi_bl_drv_mac_mgmt_write(0, xi_phy_address , 0x1f , 0x000f); mdelay( DELAY_FOR_PHY_PATCH);
    fi_bl_drv_mac_mgmt_write(0, xi_phy_address , 0x1e , 0x000d); mdelay( DELAY_FOR_PHY_PATCH);
    fi_bl_drv_mac_mgmt_write(0, xi_phy_address , 0x18 , 0x4088); mdelay( DELAY_FOR_PHY_PATCH);

    fi_bl_drv_mac_mgmt_write(0, xi_phy_address , 0x1f , 0x000f); mdelay( DELAY_FOR_PHY_PATCH);
    fi_bl_drv_mac_mgmt_write(0, xi_phy_address , 0x1e , 0x0008); mdelay( DELAY_FOR_PHY_PATCH);
    fi_bl_drv_mac_mgmt_write(0, xi_phy_address , 0x17 , 0x7100); mdelay( DELAY_FOR_PHY_PATCH);
    fi_bl_drv_mac_mgmt_write(0, xi_phy_address , 0x1f , 0x0008); mdelay( DELAY_FOR_PHY_PATCH);

    /* EEE patch*/
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+1, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+1, 27, 0x01D1 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+1, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+1, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+1, 16, 0x0F00 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+1, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 24, 0x0001 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x000F ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 30, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 23, 0x7100 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x000F ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 30, 0x0009 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 23, 0x0FA4 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 30, 0x0006 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 21, 0x0616 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 30, 0x0016 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 21, 0x0616 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x000F ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 30, 0x000C ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 27, 0xBE03 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0007 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 30, 0x0023 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 23, 0x1111 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0005 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 15, 0x0100 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 15, 0x0000 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0007 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 30, 0x0023 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 23, 0x1910 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0005 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0005 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0007 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 30, 0x0020 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 21, 0x1100 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 27, 0xA0BA ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0007 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 30, 0x0020 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 24, 0x61FF ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0007 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 30, 0x002C ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 25, 0x0130 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0007 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 30, 0x002D ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 22, 0x0084 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0000 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 13, 0x0007 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 14, 0x003C ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 13, 0x4007 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 14, 0x0006 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0002 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 11, 0x17A7 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 24, 0x0000 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+1, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+1, 16, 0x0000 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+1, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 28, 0xFF00 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 28, 0x0000 ); mdelay( DELAY_FOR_PHY_PATCH);
	fi_bl_drv_mac_mgmt_write(0, xi_phy_address+3, 31, 0x0008 ); mdelay( DELAY_FOR_PHY_PATCH);
}
EXPORT_SYMBOL(bl_lilac_init_qsgmii_phy);


void  bl_lilac_plat_rgmii_phy_init(int xi_phy_address)
{
	char boardName[BP_BOARD_ID_LEN];

	if (!BpGetBoardId(boardName))
	{
		if (!bpstrcmp(boardName,"968500CEDAR"))
		{
			//On Cedar board we have Realtek RGMII  phy which need WA
			/* Disable EEE function*/
			fi_bl_drv_mac_mgmt_write(0, xi_phy_address, 0x1f, 0x0005);mdelay( DELAY_FOR_PHY_PATCH);
			fi_bl_drv_mac_mgmt_write(0, xi_phy_address, 0x05, 0x8b84);mdelay( DELAY_FOR_PHY_PATCH);
			fi_bl_drv_mac_mgmt_write(0, xi_phy_address, 0x06, 0x00c2);mdelay( DELAY_FOR_PHY_PATCH);
			fi_bl_drv_mac_mgmt_write(0, xi_phy_address, 0x1f, 0x0007);mdelay( DELAY_FOR_PHY_PATCH);
			fi_bl_drv_mac_mgmt_write(0, xi_phy_address, 0x1e, 0x0020);mdelay( DELAY_FOR_PHY_PATCH);
			fi_bl_drv_mac_mgmt_write(0, xi_phy_address, 0x15, 0x0000);mdelay( DELAY_FOR_PHY_PATCH);
			fi_bl_drv_mac_mgmt_write(0, xi_phy_address, 0x1f, 0x0000);mdelay( DELAY_FOR_PHY_PATCH);
			fi_bl_drv_mac_mgmt_write(0, xi_phy_address, 0x00, 0x9200);mdelay( DELAY_FOR_PHY_PATCH);
		}
		else if (!bpstrcmp(boardName,"968751LUPINE2"))
		{
			unsigned char val = 0x8b;
		
			if (!bl_lilac_write_BCM53101_switch_register(0, 14, (unsigned char*)&val, 1))
				printk("Error write initializing switch\n");
		
			if (!bl_lilac_read_BCM53101_switch_register(0, 32, (unsigned char*)&val, 1))
				printk("Error read initializing switch\n");
		
			val |= 1;
			if (!bl_lilac_write_BCM53101_switch_register(0, 32, (unsigned char*)&val, 1))
				printk("Error initializing switch\n");
		
			/* disable all phys, beside phy 0 */
			val = 3; /* tx & rx disable */
			bl_lilac_write_BCM53101_switch_register(0, 1, (unsigned char*)&val, 1);
			bl_lilac_write_BCM53101_switch_register(0, 2, (unsigned char*)&val, 1);
			bl_lilac_write_BCM53101_switch_register(0, 3, (unsigned char*)&val, 1);
			bl_lilac_write_BCM53101_switch_register(0, 4, (unsigned char*)&val, 1);
			bl_lilac_write_BCM53101_switch_register(0, 5, (unsigned char*)&val, 1); 
		}
	}
}

EXPORT_SYMBOL(bl_lilac_plat_rgmii_phy_init);

/***************************************************************************
	 Return gpio pins for over current function
***************************************************************************/
BL_LILAC_USB_OC_PRESENT bl_lilac_get_usb_oc_properties(int usb_port_id, int *oc_input_pin, int *oc_output_pin)
{
	BL_LILAC_USB_OC_PRESENT ret = BL_LILAC_USB_OC_ON;

	GPIO_USB_INFO gpios;

	if (BpGetUSBGpio(usb_port_id, &gpios) == BP_SUCCESS)
	{
		*oc_input_pin  = gpios.gpio_for_oc_detect;
		*oc_output_pin = gpios.gpio_for_oc_output;
	}
	else 
		ret = BL_LILAC_USB_OC_OFF;

	return ret;
}
EXPORT_SYMBOL(bl_lilac_get_usb_oc_properties);

int bl_lilac_read_BCM53101_switch_register(int page, int reg, unsigned char *data, int len_in_bytes)
{
    uint32_t val    = 0;
    int counter     = 50000;
    int ret         = 0;
    int phyId;

    if (!data)
        return 0;

    phyId = BpGetPhyAddr(0,0);
    if (phyId < 0)
        return 0;
    val = (page << 8) | 1;
    if (fi_bl_drv_mac_mgmt_write(0, phyId, 16, val) != DRV_MAC_NO_ERROR)
        return 0;

    val = (reg << 8) | 2; // read operation
    if (fi_bl_drv_mac_mgmt_write(0, phyId, 17, val) != DRV_MAC_NO_ERROR)
        goto close_read;

    if (fi_bl_drv_mac_mgmt_read(0, phyId, 17, &val) != DRV_MAC_NO_ERROR)
        goto close_read;

    while ((val & 0x3) && counter--)
    {
        udelay(10);
        if (fi_bl_drv_mac_mgmt_read(0, phyId, 17, &val) != DRV_MAC_NO_ERROR)
            goto close_read;
    }

    if ((val & 0x3) == 0)
    {
        switch (len_in_bytes)
        {
        case 8:
            if (fi_bl_drv_mac_mgmt_read(0, phyId, 27, &val) != DRV_MAC_NO_ERROR)
                break;
            ((unsigned short *)data)[2] = (unsigned short)(val);

            if (fi_bl_drv_mac_mgmt_read(0, phyId, 26, &val) != DRV_MAC_NO_ERROR)
                break;
            ((unsigned short *)data)[3] = (unsigned short)val;
            ((unsigned int  *)data)[1] = swab32(((unsigned int  *)data)[1]);
            if (fi_bl_drv_mac_mgmt_read(0, phyId, 25, &val) != DRV_MAC_NO_ERROR)
                break;
            ((unsigned short *)data)[0] = (unsigned short)val;


            if (fi_bl_drv_mac_mgmt_read(0, phyId, 24, &val) != DRV_MAC_NO_ERROR)
                break;
            ((unsigned short *)data)[1] = (unsigned short)val;
            ((unsigned int  *)data)[0] = swab32(((unsigned int  *)data)[0]);
            ret = 1;
            break;

        case 6:
            if (fi_bl_drv_mac_mgmt_read(0, phyId, 26, &val) != DRV_MAC_NO_ERROR)
                break;
            ((unsigned short *)data)[0] = (unsigned short)(val);

            if (fi_bl_drv_mac_mgmt_read(0, phyId, 25, &val) != DRV_MAC_NO_ERROR)
                break;
            ((unsigned short *)data)[1] = (unsigned short)val;


            if (fi_bl_drv_mac_mgmt_read(0, phyId, 24, &val) != DRV_MAC_NO_ERROR)
                break;
            ((unsigned short *)data)[2] = (unsigned short)val;

            if ((reg %4) == 0)
            {
                *(unsigned int *)data = swab32(*(unsigned int *)data);
                *((unsigned short *)(data + 4)) = swab16(*((unsigned short *)(data + 4)));
            }
            else
            {
                *(unsigned short *)data = swab32(*(unsigned short *)data);
                *((unsigned int *)(data + 2)) = swab16(*((unsigned int *)(data + 2)));
            }
            ret = 1;
            break;

        case 4:
            if (fi_bl_drv_mac_mgmt_read(0, phyId, 25, &val) != DRV_MAC_NO_ERROR)
                break;
            ((unsigned short *)data)[0] = (unsigned short)val;


            if (fi_bl_drv_mac_mgmt_read(0, phyId, 24, &val) != DRV_MAC_NO_ERROR)
                break;
            ((unsigned short *)data)[1] = (unsigned short)val;
            ((unsigned int *)data)[0] = swab32(((unsigned int *)data)[0]); 
            ret = 1;
            break;

        case 2:
            if (fi_bl_drv_mac_mgmt_read(0, phyId, 24, &val) != DRV_MAC_NO_ERROR)
                break;
            ((unsigned short *)data)[0] = swab16((unsigned short)val);

            ret =1;
            break;

        case 1:
            if (fi_bl_drv_mac_mgmt_read(0, phyId, 24, &val) != DRV_MAC_NO_ERROR)
                break;
            data[0] = (unsigned char)val;

            ret = 1;
            break;

        default:
            return 0;
        }
    }

    close_read: 
    val = (page << 8);
    if (fi_bl_drv_mac_mgmt_write(0, phyId, 16, val) != DRV_MAC_NO_ERROR)
        return 0;

    return ret;
}
EXPORT_SYMBOL(bl_lilac_read_BCM53101_switch_register);

int bl_lilac_write_BCM53101_switch_register(int page, int reg, unsigned char *data, int len_in_bytes)
{
    uint32_t val    = 0;
    int counter     = 50000;
    int ret         = 0;
    int phyId;

    if (!data)
        return 0;

    phyId = BpGetPhyAddr(0,0);
    if (phyId < 0)
        return 0;
    val = (page << 8) | 1;
    if (fi_bl_drv_mac_mgmt_write(0, phyId, 16, val) != DRV_MAC_NO_ERROR)
        return 0;

    switch (len_in_bytes)
    {
    case 1:
        val = data[0];
        if (fi_bl_drv_mac_mgmt_write(0, phyId, 24, val) != DRV_MAC_NO_ERROR)
            goto close_write;
        break;

    case 2:
        val = swab16(((unsigned short *)data)[0]);
        if (fi_bl_drv_mac_mgmt_write(0, phyId, 24, val) != DRV_MAC_NO_ERROR)
            goto close_write;
        break;

    case 4:
        ((unsigned int*)data)[0] = swab32(((unsigned int *)data)[0]);
        val = (((unsigned short *)data)[0]);
        if (fi_bl_drv_mac_mgmt_write(0, phyId, 24, val) != DRV_MAC_NO_ERROR)
            goto close_write;
        val = (((unsigned short *)data)[1]);
        if (fi_bl_drv_mac_mgmt_write(0, phyId, 25, val) != DRV_MAC_NO_ERROR)
            goto close_write;
        break;

    case 6:
        if ((reg %4) == 0)
        {
            *(unsigned int *)data = swab32(*(unsigned int *)data);
            *((unsigned short *)(data + 4)) = swab16(*((unsigned short *)(data + 4)));
        }
        else
        {
            *(unsigned short *)data = swab32(*(unsigned short *)data);
            *((unsigned int *)(data + 2)) = swab16(*((unsigned int *)(data + 2)));
        }
        val = (((unsigned short *)data)[0]);
        if (fi_bl_drv_mac_mgmt_write(0, phyId, 24, val) != DRV_MAC_NO_ERROR)
            goto close_write;
        val = (((unsigned short *)data)[1]);
        if (fi_bl_drv_mac_mgmt_write(0, phyId, 25, val) != DRV_MAC_NO_ERROR)
            goto close_write;
        val = (((unsigned short *)data)[2]);
        if (fi_bl_drv_mac_mgmt_write(0, phyId, 26, val) != DRV_MAC_NO_ERROR)
            goto close_write;

        break;


    case 8:
        ((unsigned int *)data)[0] = swab32(((unsigned int *)data)[0]);
        val = (((unsigned short *)data)[0]);
        if (fi_bl_drv_mac_mgmt_write(0, phyId, 24, val) != DRV_MAC_NO_ERROR)
            goto close_write;
        val = (((unsigned short *)data)[1]);
        if (fi_bl_drv_mac_mgmt_write(0, phyId, 25, val) != DRV_MAC_NO_ERROR)
            goto close_write;
        ((unsigned int *)data)[1] = swab32(((unsigned int *)data)[1]);
        val = (((unsigned short *)data)[2]);
        if (fi_bl_drv_mac_mgmt_write(0, phyId, 26, val) != DRV_MAC_NO_ERROR)
            goto close_write;
        val = (((unsigned short *)data)[3]);
        if (fi_bl_drv_mac_mgmt_write(0, phyId, 27, val) != DRV_MAC_NO_ERROR)
            goto close_write;

        break;

    default:
        goto close_write;
    }

    val = (reg << 8) | 1; // write operation
    if (fi_bl_drv_mac_mgmt_write(0, phyId, 17, val) != DRV_MAC_NO_ERROR)
        goto close_write;

    if (fi_bl_drv_mac_mgmt_read(0, phyId, 17, &val) != DRV_MAC_NO_ERROR)
        goto close_write;

    while ((val & 0x3) && counter--)
    {
        udelay(500);
        if (fi_bl_drv_mac_mgmt_read(0, phyId, 17, &val) != DRV_MAC_NO_ERROR)
            goto close_write;
    }

    if ((val & 0x3) == 0)
        ret = 1;

    close_write:    
    val = (page << 8);
    if (fi_bl_drv_mac_mgmt_write(0, phyId, 16, val) != DRV_MAC_NO_ERROR)
        return 0;

    return ret;
}

EXPORT_SYMBOL(bl_lilac_write_BCM53101_switch_register);



