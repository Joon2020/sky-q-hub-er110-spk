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

#include <linux/device.h>
#include <linux/init.h>
#include <linux/usb.h>
#include <linux/resource.h>
#include <linux/usb/hcd.h>

#include "bl_os_wraper.h"
#include "bl_lilac_brd.h"
#include "bl_lilac_usb.h" // register file of BL USB SOC
#include "bl_lilac_soc.h"
#include "bl_lilac_ic.h"
#include "bl_lilac_cr.h"  // clock and resets of BL SOC
#include "bl_lilac_mips_blocks.h" // mips blocks of BL SOC
#include "bl_lilac_rst.h" // clock and resets APIs
#include "bl_lilac_drv_serdes.h"
#include "bl_lilac_usb_hc.h"

struct bl_usb_hcd_s * blusbhcd[2] = {NULL,NULL};
extern irqreturn_t ohci_irq(struct usb_hcd *hcd);
extern irqreturn_t ehci_irq(struct usb_hcd *hcd);

#ifdef CONFIG_BLUSB_DEBUG
#include "bl_lilac_usb_debug.c"
#ifndef WITH_BLPROC
struct proc_dir_entry *bl_dir = NULL;
#endif
#endif

static struct resource bl_usb_ehci_0_resources[] = 
{
    { // USB0 - ehci
        .start = CE_USB_BLOCK_EHCI0_CAP_REGS_ADDRESS,
        .end   = CE_USB_BLOCK_OHCI0_OPT_REGS_ADDRESS - 4,
        .flags = IORESOURCE_MEM
    },
    { // USB0 - irq
        .start = BL_LILAC_IC_INTERRUPT_USB_0,
        .end   = BL_LILAC_IC_INTERRUPT_USB_0,
        .flags = IORESOURCE_IRQ
    }
};
static struct resource bl_usb_ohci_0_resources[] = 
{
    {   // USB0 - ohci
        .start = CE_USB_BLOCK_OHCI0_OPT_REGS_ADDRESS,
        .end   = CE_USB_BLOCK_EHCI1_CAP_REGS_ADDRESS - 4,
        .flags = IORESOURCE_MEM
    },
    { // USB0 - irq
        .start = BL_LILAC_IC_INTERRUPT_USB_0,
        .end   = BL_LILAC_IC_INTERRUPT_USB_0,
        .flags = IORESOURCE_IRQ
    }
};

static u64 dummy_mask = DMA_BIT_MASK(32);
static struct platform_device bl_lilac_usb_ehci_0_platform =
{
	.name = "bl-lilac-ehci",
	.id = 0,
	.num_resources = ARRAY_SIZE(bl_usb_ehci_0_resources),
	.resource =	bl_usb_ehci_0_resources,
	.dev = {
		.dma_mask		= &dummy_mask,
	},
};
static struct platform_device bl_lilac_usb_ohci_0_platform =
{
	.name = "bl-lilac-ohci",
	.id = 0,
	.num_resources = ARRAY_SIZE(bl_usb_ohci_0_resources),
	.resource =	bl_usb_ohci_0_resources,
	.dev = {
		.dma_mask		= &dummy_mask,
	},
};


static struct resource bl_usb_ehci_1_resources[] = 
{
    {   // USB1 - ehci
        .start = CE_USB_BLOCK_EHCI1_CAP_REGS_ADDRESS,
        .end   = CE_USB_BLOCK_OHCI1_OPT_REGS_ADDRESS - 4,
        .flags = IORESOURCE_MEM
    },
    { // USB1 - irq
        .start = BL_LILAC_IC_INTERRUPT_USB_1,
        .end   = BL_LILAC_IC_INTERRUPT_USB_1,
        .flags = IORESOURCE_IRQ
    }
};
static struct resource bl_usb_ohci_1_resources[] = 
{
    {   // USB1 - ohci
        .start = CE_USB_BLOCK_OHCI1_OPT_REGS_ADDRESS,
        .end   = CE_USB_BLOCK_REGFILE_ADDRESS - 4,
        .flags = IORESOURCE_MEM
    },
    { // USB1 - irq
        .start = BL_LILAC_IC_INTERRUPT_USB_1,
        .end   = BL_LILAC_IC_INTERRUPT_USB_1,
        .flags = IORESOURCE_IRQ
    }
};
static struct platform_device bl_lilac_usb_ehci_1_platform =
{
	.name = "bl-lilac-ehci",
	.id = 1,
	.num_resources = ARRAY_SIZE(bl_usb_ehci_1_resources),
	.resource =	bl_usb_ehci_1_resources,
	.dev = {
		.dma_mask		= &dummy_mask,
	},
};
static struct platform_device bl_lilac_usb_ohci_1_platform =
{
	.name = "bl-lilac-ohci",
	.id = 1,
	.num_resources = ARRAY_SIZE(bl_usb_ohci_1_resources),
	.resource =	bl_usb_ohci_1_resources,
	.dev = {
		.dma_mask		= &dummy_mask,
	},
};


/********************************************************/
/* set into USB controller glue logic the right         */
/* selection - EHCI/OHCI                                */
/********************************************************/
void bl_lilac_ohci_link(int usb,int action)
{
    volatile USB_BLOCK_REGFILE_USB0_AHB_OHCI_EHCI_SEL_DTE reg1;
    volatile USB_BLOCK_REGFILE_OHCI_0_CNTSEL_I_N_DTE reg2;

    reg1.usb0_ahb_ohci_ehci_sel = action;
    reg2.ohci_0_cntsel_i_n = action;
    if (!usb)
    {
        /* 0x1800-4A00 */
        BL_USB_BLOCK_REGFILE_USB0_AHB_OHCI_EHCI_SEL_WRITE(reg1);
        /* 0x1800-4980 */
        BL_USB_BLOCK_REGFILE_OHCI_0_CNTSEL_I_N_WRITE(reg2);
    }
    else
    {
        /* 0x1800-4A04 */
        BL_USB_BLOCK_REGFILE_USB1_AHB_OHCI_EHCI_SEL_WRITE(reg1);
        /* 0x1800-4984 */
        BL_USB_BLOCK_REGFILE_OHCI_1_CNTSEL_I_N_WRITE(reg2);
    }
}

/********************************************************/
/* register USB controller into Linux platform          */
/* one instance of EHCI and one of OHCI                 */
/********************************************************/
static int __init bl_lilac_usb_devinit(void)
{
	if(!bl_lilac_usb_hcd_enabled(0) && !bl_lilac_usb_hcd_enabled(1))
	    return 1;
	
#if defined(CONFIG_BLUSB_DEBUG) && !defined(WITH_BLPROC)
        bl_dir = proc_mkdir("blutil", NULL);
#endif

	if(bl_lilac_usb_hcd_enabled(0))
	{
	    platform_device_register(&bl_lilac_usb_ehci_0_platform);
	    platform_device_register(&bl_lilac_usb_ohci_0_platform);
	}

	if(bl_lilac_usb_hcd_enabled(1))
	{
	    platform_device_register(&bl_lilac_usb_ehci_1_platform);
	    platform_device_register(&bl_lilac_usb_ohci_1_platform);
	}

    return 0;
}


/********************************************************/
/* interrupt callback                                   */
/*  calls the correspondent interrupt routine, according*/
/*  to the type of USB protocol                         */
/*  clear interrupt into interrupt controller           */
/********************************************************/
irqreturn_t bl_ehci_irq(struct usb_hcd *hcd)
{
    irqreturn_t ret = IRQ_HANDLED;
    int ctrl = 0;

    if (!hcd)
    {
        printk("NULL HCD\n");
        return ret;
    }

    if (hcd->irq - INT_NUM_IRQ0 == BL_LILAC_IC_INTERRUPT_USB_1)
        ctrl = 1;
    if (blusbhcd[ctrl])
    {
        if (blusbhcd[ctrl]->ohcihcd == hcd)
            ret = ohci_irq(hcd);
        else if (blusbhcd[ctrl]->ehcihcd == hcd)
            ret = ehci_irq(hcd);
        else 
            printk("WRONG hcd for %d\n",ctrl);
    }
    else 
        printk("NULL for %d\n",ctrl);

	return ret;
}
EXPORT_SYMBOL(bl_ehci_irq);



#define SYNOPSYS_INSREG03_OFFSET    0xC /* 12=3*4 */

/**********************************************************/
/* does all the needed intialization to un-reset or reset */
/* USB controller - it works only on MIPS_C               */
/* intialize all USB clocks and take the controller out   */
/* of reset, along with all the common parts              */
/**********************************************************/


void bl_usb_reset(BL_USB_HOST_CONTROLLER usb, BL_USB_HC_RESET action)
{
#ifdef __MIPS_C
    BL_LILAC_PLL_CLIENT_PROPERTIES profile_core;
    BL_LILAC_PLL_CLIENT_PROPERTIES profile_host;
    volatile CR_CONTROL_REGS_SW_RST_H_3_DTE reg;

    if (bl_lilac_pll_client_properties(PLL_CLIENT_USB_CORE,&profile_core) != BL_LILAC_SOC_OK)
        return;
    if (bl_lilac_pll_client_properties(PLL_CLIENT_USB_HOST,&profile_host) != BL_LILAC_SOC_OK)
        return;

    if (action == OUT_OF_RESET_USB)
    {
        /* enable common clocks 0x02078000 (both usb) */
        /*                      **********            */
        /* 0x190A-1018 bits : 15, 18, 25 */
        bl_lilac_cr_device_enable_clk(BL_LILAC_CR_USB_COMMON,BL_LILAC_CR_ON);
        bl_lilac_cr_device_enable_clk(BL_LILAC_CR_USB_CORE,BL_LILAC_CR_ON);
        bl_lilac_cr_device_enable_clk(BL_LILAC_CR_IPCLOCK,BL_LILAC_CR_ON);
        /* out of reset the common part */
        /* 0x190A-1014 bits : 0, 7, 9 */
        bl_lilac_cr_device_reset(BL_LILAC_CR_USB_POWER_ON,BL_LILAC_CR_ON);
        bl_lilac_cr_device_reset(BL_LILAC_CR_USB_COMMON_HOST,BL_LILAC_CR_ON);
        bl_lilac_cr_device_reset(BL_LILAC_CR_USB_BRIDGE_COMMON_OCP,BL_LILAC_CR_ON);
        if (usb == USB0)
        {
            /* enable specific clock */
            /* 0x190A-1018 bits : 16 */
            bl_lilac_cr_device_enable_clk(BL_LILAC_CR_USB_0,BL_LILAC_CR_ON);
            /* usb core dividers */
            bl_lilac_set_usb_core_divider(profile_core.pll,profile_core.pre_m,profile_core.m);
            /* usb host dividers */
            bl_lilac_set_usb_host_divider(profile_host.pll,profile_host.m);
            /* out of reset the specific USB = 0x00700381 (both usb) */
            /*                                 **********            */
            /* 0x190A-1014 bits : 1, 2, 3, 10, 11, 12, 13, 14 */
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_PHY,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_OHCI_PLL,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_PORT,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_HOST,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_AUX_WELL,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_BRIDGE_HOST,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_BRIDGE_OCP,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_DDR,BL_LILAC_CR_ON);
        }
        else
        {
            /* enable specific clock */
            /* 0x190A-1018 bits : 17 */
            bl_lilac_cr_device_enable_clk(BL_LILAC_CR_USB_1,BL_LILAC_CR_ON);
            /* usb core dividers */
            bl_lilac_set_usb_core_divider(profile_core.pll,profile_core.pre_m,profile_core.m);
            /* usb host dividers */
            bl_lilac_set_usb_host_divider(profile_host.pll,profile_host.m);
            /* out of reset the specific USB */
            /* 0x190A-1014 bits : 4, 5, 6, 15, 16, 17, 18, 19 */
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_PHY,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_OHCI_PLL,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_PORT,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_HOST,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_AUX_WELL,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_BRIDGE_HOST,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_BRIDGE_OCP,BL_LILAC_CR_ON);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_DDR,BL_LILAC_CR_ON);
        }
    }
    else
    {
        if (usb == USB0)
        {
            /* reset the specific USB */
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_PHY,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_OHCI_PLL,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_PORT,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_HOST,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_AUX_WELL,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_BRIDGE_HOST,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_BRIDGE_OCP,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_0_DDR,BL_LILAC_CR_OFF);
            /* disable the specific clock USB */
            bl_lilac_cr_device_enable_clk(BL_LILAC_CR_USB_0,BL_LILAC_CR_OFF);
        }
        else
        {
            /* reset the specific USB */
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_PHY,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_OHCI_PLL,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_PORT,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_HOST,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_AUX_WELL,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_BRIDGE_HOST,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_BRIDGE_OCP,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_1_DDR,BL_LILAC_CR_OFF);
            /* disable the specific clock USB */
            bl_lilac_cr_device_enable_clk(BL_LILAC_CR_USB_1,BL_LILAC_CR_OFF);
        }
        /* if both usb's are in reset, reset and disable clocks for the common part */
        BL_CR_CONTROL_REGS_SW_RST_H_3_READ(reg);
        if (reg.usb_0_host && reg.usb_1_host)
        {
            /* reset the common part */
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_POWER_ON,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_COMMON_HOST,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_reset(BL_LILAC_CR_USB_BRIDGE_COMMON_OCP,BL_LILAC_CR_OFF);
            /* disable common clocks */
            bl_lilac_cr_device_enable_clk(BL_LILAC_CR_USB_CORE,BL_LILAC_CR_OFF);
            bl_lilac_cr_device_enable_clk(BL_LILAC_CR_USB_COMMON,BL_LILAC_CR_OFF);
        }
    }
#endif
}
EXPORT_SYMBOL(bl_usb_reset);

static unsigned int ahb_reg_list [] =
{ 
    0xf,        // INTERRUPT_MASK
    0xff,       // AHB_HADDR_MASK
    1,          // SS_ENA_INCR16
    0,          // SS_ENA_INCRX_ALIGN_I
    0,          // WORDINTERFACE
    0xffffffff, // AHB_CFG_0 
    0x881,      // AHB_CFG_1 
    0xaaaaaaaa, // AHB_CFG_2
    0x181,      // AHB_CFG_3
    0x55555555, // AHB_CFG_4
    0x23,       // AHB_CFG_5 
    0x0,        // AHB_CFG_6 
    0x7,        // AHB_CFG_7 
    0,          // AHB_CFG_8 
    0,          // AHB_CFG_9 
    0,          // AHB_CFG_10 
    0,          // AHB_CFG_11
    0,          // AHB_CFG_12
    0,          // AHB_CFG_13
    0x1080      // AHB_CFG_14 - acceleration on prefetch : 0x103f081
};
/********************************************************/
/* intialize AHB interface for each controller          */
/********************************************************/
void bl_usb_ahb(BL_USB_HOST_CONTROLLER usb)
{
    unsigned int value;
    int i;

    value = 0x60000;
    /* 0x1800-4B40 = 0x60000*/
    BL_USB_BLOCK_REGFILE_USB_EC_CFG_0_WRITE(value); 
    if (usb == USB0)
    {
        value = 1;
        /* 0x180048BC */
        BL_USB_BLOCK_REGFILE_TXPREEMPHASISTUNE0_WRITE(value);
        /* 0x1800494C */
        BL_USB_BLOCK_REGFILE_USB0_SS_FLADJ_VAL_5_I_WRITE(value);
        mdelay(1); 
        /* 0x1800-4B80 = 0xF */
        BL_USB_BLOCK_REGFILE_USB0_INTERRUPT_MASK_WRITE(ahb_reg_list[0]);
        /* 0x1800-4B74 = 0xFF */
        BL_USB_BLOCK_REGFILE_USB0_AHB_HADDR_MASK_WRITE(ahb_reg_list[1]);
        /* 0x1800-4910 = 0x1 */
        BL_USB_BLOCK_REGFILE_USB0_SS_ENA_INCR16_I_WRITE(ahb_reg_list[2]);
        /* 0x1800-4920 = 0x0 */
        BL_USB_BLOCK_REGFILE_USB0_SS_ENA_INCRX_ALIGN_I_WRITE(ahb_reg_list[3]);
        // this register MUST be initialize at same value on both controllers
        /* 0x1800-4808 = 0x0 */
        BL_USB_BLOCK_REGFILE_WORDINTERFACE0_WRITE(ahb_reg_list[4]);
        /* 0x1800-4A80 ... */
        for (i = 5; i < sizeof(ahb_reg_list)/4; i++)
        {
            BL_WRITE_32( ( CE_USB_BLOCK_REGFILE_USB0_AHB_CFG_0_ADDRESS + (i-5) * 4), ahb_reg_list[i] );
        }
        /* 0x1800-4C38 = 0x103f081 */
        // last value ( to cgf_14) must be written to cfg_14_swap also
        BL_USB_BLOCK_REGFILE_USB0_AHB_CFG_14_SWAP_WRITE(ahb_reg_list[i-1]);
        /* Synopsys specific registers */
        value = 0x401;
        BL_WRITE_32((CE_USB_BLOCK_EHCI0_SYN_REGS_ADDRESS + SYNOPSYS_INSREG03_OFFSET),value);
    }
    else
    {
        value = 1;
        /* 0x180048C0 */
        BL_USB_BLOCK_REGFILE_TXPREEMPHASISTUNE1_WRITE(value);
        /* 0x18004964 */
        BL_USB_BLOCK_REGFILE_USB1_SS_FLADJ_VAL_5_I_WRITE(value);
        mdelay(1); 
        /* 0x1800-4B84 = 0xF */
        BL_USB_BLOCK_REGFILE_USB1_INTERRUPT_MASK_WRITE(ahb_reg_list[0]);
        /* 0x1800-4B78 = 0xFF */
        BL_USB_BLOCK_REGFILE_USB1_AHB_HADDR_MASK_WRITE(ahb_reg_list[1]);
        /* 0x1800-4914 = 0x1 */
        BL_USB_BLOCK_REGFILE_USB1_SS_ENA_INCR16_I_WRITE(ahb_reg_list[2]);
        /* 0x1800-4924 = 0x0 */
        BL_USB_BLOCK_REGFILE_USB1_SS_ENA_INCRX_ALIGN_I_WRITE(ahb_reg_list[3]);
        /* 0x1800-480C = 0x0 */
        BL_USB_BLOCK_REGFILE_WORDINTERFACE1_WRITE(ahb_reg_list[4]);
        /* 0x1800-4ABC ... */
        for (i = 5; i < sizeof(ahb_reg_list)/4; i++)
        {
            BL_WRITE_32( ( CE_USB_BLOCK_REGFILE_USB1_AHB_CFG_0_ADDRESS + (i-5) * 4), ahb_reg_list[i] );
        }
        /* 0x1800-4C78 = 0x103f081 */
        // last value ( to cgf_14) must be written to cfg_14_swap also
        BL_USB_BLOCK_REGFILE_USB1_AHB_CFG_14_SWAP_WRITE(ahb_reg_list[i-1]);
        value = 0x401;
        BL_WRITE_32((CE_USB_BLOCK_EHCI1_SYN_REGS_ADDRESS + SYNOPSYS_INSREG03_OFFSET),value);
    }
}
EXPORT_SYMBOL(bl_usb_ahb);

arch_initcall(bl_lilac_usb_devinit);




