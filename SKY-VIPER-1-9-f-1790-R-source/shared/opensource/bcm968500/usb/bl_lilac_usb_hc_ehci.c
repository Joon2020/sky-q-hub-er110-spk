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


#ifndef __USB_HC_EHCI_C_
#define __USB_HC_EHCI_C_

#include <linux/platform_device.h>
#include "bl_lilac_soc.h"
#include "bl_lilac_brd.h"
#include "bl_lilac_mips_blocks.h"
#include "bl_lilac_usb_hc.h"
#include "bl_lilac_ic.h"
#include "bl_lilac_usb.h"
#include "bl_lilac_pin_muxing.h"
#include "bl_lilac_gpio.h"

#ifdef CONFIG_BLUSB_DEBUG
#include <linux/syscalls.h>
extern struct proc_dir_entry *bl_dir;
static int registerForDebug(int usb);
#endif
       
/********************************************************/
/* change port owner callback                           */
/*  change the selection into glue logic                */
/********************************************************/
/* called from core/hub.c/hub_port_connect_change */
static void bl_lilac_ohci_relink(struct usb_hcd * hcd, int portnum)
{
    int ctrl = 0;

    if (hcd->irq - INT_NUM_IRQ0 == BL_LILAC_IC_INTERRUPT_USB_1)
        ctrl = 1;
    bl_lilac_ohci_link(ctrl,1);
    blusbhcd[ctrl]->works_as_ohci = 1;
    USB_INFO("BL USB_%d Controller works as OHCI\n",ctrl);
    ehci_relinquish_port(hcd,portnum);
}


static const struct hc_driver bl_lilac_ehci_driver = {
	.description        = "bl_lilac_ehci",
	.product_desc       = "LILAC USB EHCI Host Controller",
	.hcd_priv_size      = sizeof(struct ehci_hcd),
	/* generic hardware linkage */
	.irq                = bl_ehci_irq,
	.flags              = (int)(HCD_LOCAL_MEM | HCD_MEMORY | HCD_USB2),
	/* basic lifecycle operations */
	.reset              = ehci_init, // called by usb_add_hcd, as once time reset
	.start              = ehci_run,  // called by usb_add_hcd, defined in ehci-hcd.c
    /*
     1. ehci_run calls ehci_reset; wait (handshake) unti bit 1 (reset) 
     of USBCMD register is 0 or 240*1000 usec as timeout
     2. read configuration registers
     3. set USBINTR register
     4. write run in USBCMD register
    */
	.stop = ehci_stop, // called by usb_add_hcd, on error and by usb_remove
	.shutdown           = ehci_shutdown,
	/* managing i/o requests and associated device resources */
	.urb_enqueue        = ehci_urb_enqueue,
	.urb_dequeue        = ehci_urb_dequeue,
	.endpoint_disable   = ehci_endpoint_disable,
	/* scheduling support */
	.get_frame_number   = ehci_get_frame,
	/* root hub support */
	.hub_status_data    = ehci_hub_status_data,
	.hub_control        = ehci_hub_control,
    .relinquish_port    = bl_lilac_ohci_relink,
    .port_handed_over   = ehci_port_handed_over,
#ifdef	CONFIG_PM
	.bus_suspend        = ehci_bus_suspend,
	.bus_resume         = ehci_bus_resume,
#endif
};

/********************************************************/
/* create structures for EHCI part of the controller    */
/* perform first step initializations                   */
/********************************************************/
static int bl_lilac_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
    struct ehci_hcd *ehci;
	struct resource *res;
    char name [60];
    int irqnum;
    int ret = 0;
    BL_USB_HOST_CONTROLLER usb = pdev->id;

    sprintf(name,"bl_usb_ehci_%d",usb);
	hcd = usb_create_hcd(&bl_lilac_ehci_driver, &pdev->dev, name);
	if (!hcd) {
        USB_EPRINTK("Cannot allocate USB structure for %s\n",name);
		return -ENOMEM;
	}
    
    blusbhcd[usb]->ehcihcd = hcd;
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res)
    {
        hcd->rsrc_start = res->start;
        hcd->rsrc_len = resource_size(res);
    }
    else
    {
        USB_EPRINTK("Failed to get MEM resource for %s\n",name);
		ret = -ENOMEM;
        goto error;
    }

    res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
    if (res)
        irqnum = res->start + INT_NUM_IRQ0;
    else
    {
        USB_EPRINTK("Failed to get IRQ resource for %s\n",name);
    	ret = -ENOMEM;
        goto error;
    }
	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);

	if (!hcd->regs) {
        USB_EPRINTK("Failed on remap USB address for %s\n",name);
		ret = -ENOMEM;
        goto error;
	}
	ehci = hcd_to_ehci(hcd);
	ehci->caps = hcd->regs;
	ehci->regs = hcd->regs + HC_LENGTH(ehci, ehci_readl(ehci,(__u32 *)(ehci->caps + offsetof(struct ehci_caps,hc_capbase))));
    {
        unsigned int temp = (unsigned int)(ehci->caps) + offsetof(struct ehci_caps,hcs_params);
        ehci->hcs_params = ehci_readl(ehci,(__u32 *)temp);
    }
	ehci->sbrn = 0x20 | usb;
	ret = usb_add_hcd(hcd, irqnum, IRQF_SHARED);
	if (ret)
    {
        USB_EPRINTK("Failed to add %s\n",name);
        goto error;
    }
	platform_set_drvdata(pdev, hcd);
    hcd->self.root_hub->slot_id = usb;
    USB_INFO("%s is probed (hcd=0x%08x)\n",name,(unsigned int)hcd);

	return ret;

error:
    usb_put_hcd(hcd);
    blusbhcd[usb]->ehcihcd = NULL;

    return 1;
}

/********************************************************/
/* callback for remove USB controller (EHCI part of it) */
/********************************************************/
static int ehci_hcd_bl_lilac_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
    int actual_cpu = MIPSC; // 0 for MIPS_C ; 1 for MIPS_D

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	usb_put_hcd(hcd);
    
    if (actual_cpu == MIPSC)
    {
        char name[10];

        bl_usb_reset(pdev->id,RESET_USB);
        sprintf(name,"ehci%d",pdev->id);
#ifdef CONFIG_BLUSB_DEBUG
#ifdef WITH_BLPROC
        bl_unregister(name);
#else
        if (bl_dir)
            remove_proc_entry(name, bl_dir);
#endif
#endif
        kfree(blusbhcd[pdev->id]);
    }
    // else tbd 
	platform_set_drvdata(pdev, NULL);

	return 0;
}


void bl_lilac_usb_ref_clk(void)
{
    volatile USB_BLOCK_REGFILE_REFCLKSEL_DTE refclksel;

    BL_USB_BLOCK_REGFILE_REFCLKSEL_READ(refclksel);
    refclksel.refclksel = 3;
    BL_USB_BLOCK_REGFILE_REFCLKSEL_WRITE(refclksel);
}

/********************************************************/
/* callback for probe  USB controller (EHCI part of it) */
/* - allocate BL structures                             */
/* - un-reset controller                                */
/* - select the MIPS ( C or D )                         */
/* - define and connect interrupt to controller         */
/* - intialize the glue logic and AHB interface         */
/********************************************************/
static int ehci_hcd_bl_lilac_drv_probe(struct platform_device *pdev)
{
    int actual_cpu = MIPSC; // 0 for MIPSC ; 1 for MIPSD
    volatile MIPS_BLOCKS_VPB_BRIDGE_USB_SEL_OBIUC_OBIUD_DTE sel;
    BL_LILAC_IC_INTERRUPT_PROPERTIES intProperties;
    int gin,gout;

	if (usb_disabled())
    {
		return -ENODEV;
    }
    blusbhcd[pdev->id] = kmalloc(sizeof(struct bl_usb_hcd_s),GFP_KERNEL);
    if (!blusbhcd[pdev->id])
    {
        USB_EPRINTK("Cannot allocate USB structure for device %d\n",pdev->id);
        return 1;
    }
    blusbhcd[pdev->id]->works_as_ohci = 0;
    blusbhcd[pdev->id]->debug_on_registers = TRACE_OFF;
    blusbhcd[pdev->id]->debug_on_messages = TRACE_OFF;
    // select CPU interface
    sel.usb_sel_obiuc_obiud = actual_cpu;
    BL_MIPS_BLOCKS_VPB_BRIDGE_USB_SEL_OBIUC_OBIUD_WRITE(sel);

    if (actual_cpu == MIPSC)
    {
        bl_usb_reset(pdev->id,OUT_OF_RESET_USB);
    }
    // else tbd 

    /* set interrupt characteristics */
    memset(&intProperties, 0, sizeof( intProperties)); 
    intProperties.priority			= 3;
    intProperties.configuration		= BL_LILAC_IC_INTERRUPT_LEVEL_HIGH;
    intProperties.reentrant			= 0;
    intProperties.fast_interrupt	= 0;

    bl_lilac_usb_ref_clk();
    bl_usb_ahb(pdev->id);

#ifdef CONFIG_BLUSB_DEBUG
    registerForDebug(pdev->id);
#endif

    if(!bl_lilac_get_usb_oc_properties(pdev->id, &gin, &gout) == BL_LILAC_USB_OC_ON)
    {
		unsigned int val;
		val = 0x1;
		BL_MIPS_BLOCKS_VPB_BRIDGE_SPAREOUT_WRITE(val);

        fi_bl_lilac_mux_config_pin_pull(gin, PULL_DOWN);
        fi_bl_lilac_mux_config_pin_strength(gout, S6MA);
        fi_bl_lilac_mux_connect_pin(gout, USB_PRT_WR, pdev->id, actual_cpu);
        fi_bl_lilac_mux_connect_pin(gin,  USB_OVRCUR, pdev->id, actual_cpu);
        USB_INFO("OverCurrentFeature for USB_%d : OC=%d WP=%d\n",pdev->id,gin,gout);
    }
    else USB_INFO("No OverCurrentFeature for USB_%d\n",pdev->id);

    if (pdev->id)
        bl_lilac_ic_init_int(BL_LILAC_IC_INTERRUPT_USB_1, &intProperties);
    else
        bl_lilac_ic_init_int(BL_LILAC_IC_INTERRUPT_USB_0, &intProperties);

    return bl_lilac_probe(pdev);

}



#ifdef CONFIG_PM
/********************************************************/
/* callback for suspend USB 2.0 device                  */
/********************************************************/
static int ehci_hcd_bl_drv_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);
	unsigned long flags;

    if (time_before(jiffies, ehci->next_statechange))
		msleep(10);

	/* Root hub was already suspended. Disable irq emission and
	 * mark HW unaccessible.  The PM and USB cores make sure that
	 * the root hub is either suspended or stopped.
	 */
	spin_lock_irqsave(&ehci->lock, flags);
	ehci_bus_suspend(hcd);
	ehci_writel(ehci, 0, &ehci->regs->intr_enable);
	(void)ehci_readl(ehci, &ehci->regs->intr_enable);

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	spin_unlock_irqrestore(&ehci->lock, flags);

	return 0;
}

/********************************************************/
/* callback for resume USB 2.0 device                   */
/********************************************************/
static int ehci_hcd_bl_drv_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct ehci_hcd *ehci = hcd_to_ehci(hcd);

	if (time_before(jiffies, ehci->next_statechange))
		msleep(100);

	/* Mark hardware accessible again as we are out of D3 state by now */
	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);

	if (ehci_readl(ehci, &ehci->regs->configured_flag) == FLAG_CF) 
    {
		int	mask = INTR_MASK;

        ehci_bus_resume(hcd);
		if (!hcd->self.root_hub->do_remote_wakeup)
			mask &= ~STS_PCD;
		ehci_writel(ehci, mask, &ehci->regs->intr_enable);
		ehci_readl(ehci, &ehci->regs->intr_enable);
		return 0;
    }
	usb_root_hub_lost_power(hcd->self.root_hub);

   (void) ehci_halt(ehci);
   (void) ehci_reset(ehci);
   ehci_port_power (ehci, 1);

   /* emptying the schedule aborts any urbs */
   spin_lock_irq(&ehci->lock);
   if (ehci->reclaim)
   	end_unlink_async(ehci);
   ehci_work(ehci);
   spin_unlock_irq(&ehci->lock);

   ehci_writel(ehci, ehci->command, &ehci->regs->command);
   ehci_writel(ehci, FLAG_CF, &ehci->regs->configured_flag);
   ehci_readl(ehci, &ehci->regs->command);	/* unblock posted writes */

   /* here we "know" root ports should always stay powered */
   ehci_port_power(ehci, 1);
   hcd->state = HC_STATE_SUSPENDED;

   return 0;
}
static const struct dev_pm_ops bl_ehci_pmops = {
	.suspend	= ehci_hcd_bl_drv_suspend,
	.resume		= ehci_hcd_bl_drv_resume,
};


#endif /*  CONFIG_PM */

static struct platform_driver ehci_hcd_bl_lilac_driver = {
	.probe		= ehci_hcd_bl_lilac_drv_probe,
	.remove		= ehci_hcd_bl_lilac_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver = {
		.name	= "bl-lilac-ehci",
#ifdef CONFIG_PM		
        .pm	= (struct dev_pm_ops *)&bl_ehci_pmops,
#endif
	}
};

#ifdef CONFIG_BLUSB_DEBUG
/********************************************************/
/* print ehci_hcd structure - almost all its fields     */
/********************************************************/
static char * dump_ehci(struct ehci_hcd * ehcihcd)
{
#define BUFFER_SIZE 500
    int l=0;
    char *temp = kmalloc(BUFFER_SIZE,GFP_KERNEL);
    if (!temp)
        return NULL;
    memset(temp,0,BUFFER_SIZE);

    l += sprintf(&temp[l],"\n\rEHCI regs = 0x%08x\n\r",(unsigned int)ehcihcd->regs);
    l += sprintf(&temp[l],"HCSPARAMS = 0x%08x\n\r",(unsigned int)ehcihcd->hcs_params);
    l += sprintf(&temp[l],"Async     = 0x%08x\n\r",(unsigned int)ehcihcd->async);
    l += sprintf(&temp[l],"Reclaim   = 0x%08x\n\r",(unsigned int)ehcihcd->reclaim);
    l += sprintf(&temp[l],"Periodic  = 0x%08x\n\r",(unsigned int)ehcihcd->periodic);
    l += sprintf(&temp[l],"Ports suspended : 0x%08x\n\r",(unsigned int)ehcihcd->suspended_ports);
    l += sprintf(&temp[l],"Ports companion : 0x%08x\n\r",(unsigned int)ehcihcd->companion_ports);
    l += sprintf(&temp[l],"Ports owned     : 0x%08x\n\r",(unsigned int)ehcihcd->owned_ports);
    l += sprintf(&temp[l],"Ports change-suspend feature on : 0x%08x\n\r",(unsigned int)ehcihcd->port_c_suspend);
    l += sprintf(&temp[l],"Ports already suspended at start of bus suspend : 0x%08x\n\r",(unsigned int)ehcihcd->bus_suspended);
	sprintf(&temp[l],"Actions : 0x%08x\n\r",(unsigned int)ehcihcd->actions);

#undef BUFFER_SIZE
    return temp;
}
/********************************************************/
/* callback for 'cat' command of  ehciX files           */
/* print structures and interrpret some fields          */
/********************************************************/
static int p_show_ehci(void *b, char **start, off_t off,int count, int *eof, void *data)
{
    char *buf,*temp;
    int length;

    buf = (char *)b;
    length = 0;
    temp = NULL;
    if (data)
    {
        if (!strcmp(data,"ehci0") && blusbhcd[0])
        {
            temp = dumpstruct("\nEHCI 0\n=======\n",(void *)hcd_to_ehci(blusbhcd[0]->ehcihcd),sizeof(struct ehci_hcd)/4,5);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dump_ehci(hcd_to_ehci(blusbhcd[0]->ehcihcd));
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dumpstruct("\nHCD 0\n=====\n",(void *)blusbhcd[0]->ehcihcd,sizeof(struct usb_bus)/4,5);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dumpstruct("",(void *)((unsigned int)blusbhcd[0]->ehcihcd+sizeof(struct usb_bus)),(sizeof(struct usb_hcd)-sizeof(struct usb_bus))/4,5);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dump_hcd(blusbhcd[0]->ehcihcd);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
        }
        if (!strcmp(data,"ehci1") && blusbhcd[1])
        {
            temp = dumpstruct("\nEHCI 1\n=======\n",(void *)hcd_to_ehci(blusbhcd[1]->ehcihcd),sizeof(struct ehci_hcd)/4,5);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dump_ehci(hcd_to_ehci(blusbhcd[1]->ehcihcd));
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dumpstruct("\nHCD 1\n=====\n",(void *)blusbhcd[1]->ehcihcd,sizeof(struct usb_bus)/4,5);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dumpstruct("",(void *)((unsigned int)blusbhcd[1]->ehcihcd+sizeof(struct usb_bus)),(sizeof(struct usb_hcd)-sizeof(struct usb_bus))/4,5);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dump_hcd(blusbhcd[1]->ehcihcd);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
        }
    }
    *eof = 1;
    return length;
}
/********************************************************/
/* read and print EHCI standard registers               */
/* also print some fields from EHCI structure           */
/********************************************************/
void p_ehci_stat(const unsigned char *ffile)
{
    u32	status[6];
    struct ehci_hcd	*ehci;

    if (!strcmp(ffile,"ehci0")) 
        ehci = hcd_to_ehci(blusbhcd[0]->ehcihcd);
    else
        ehci = hcd_to_ehci(blusbhcd[1]->ehcihcd);
    status[0] = ehci_readl(ehci, &ehci->regs->command);
    status[1] = ehci_readl(ehci, &ehci->regs->status);
    status[2] = ehci_readl(ehci, &ehci->regs->intr_enable);
    status[3] = ehci_readl(ehci, &ehci->regs->port_status[0]);
    status[4] = ehci_readl(ehci, &ehci->regs->async_next);
    status[5] = ehci_readl(ehci, &ehci->regs->frame_list);
    printk("HCCPARAMS: 0x%08x\n\r",ehci->caps->hcc_params);
    printk("HCSPARAMS: 0x%08x\n\r",ehci->caps->hcs_params);
    printk("USBCMD   : 0x%08x\n\r",status[0]);
    printk("USBSTS   : 0x%08x\n\r",status[1]);
    printk("USBINTR  : 0x%08x\n\r",status[2]);
    printk("PORTSC   : 0x%08x\n\r",status[3]);
    printk("ASYNC    : 0x%08x : 0x%08x\n\r",status[4],(unsigned int)ehci->async->hw);
    printk("PERIODIC : 0x%08x\n\r",status[5]);
}

#ifdef WITH_BLPROC
static int p_show_ehci_seq(struct seq_file *f,void *data)
{
    char *temp = NULL;
    int len = 0;

    if (data)
    {
        if (!strcmp(data,"ehci0") && blusbhcd[0])
        {
            temp = dump_ehci(hcd_to_ehci(blusbhcd[0]->ehcihcd));
            len += strlen(temp);
            seq_puts(f,temp);
            kfree(temp);
        }
        else if (!strcmp(data,"ehci1") && blusbhcd[1])
        {
            temp = dump_ehci(hcd_to_ehci(blusbhcd[1]->ehcihcd));
            len += strlen(temp);
            seq_puts(f,temp);
            kfree(temp);
        }
    }

    return len;
}
#endif
static int registerForDebug(int usb)
{
#ifdef WITH_BLPROC
    struct bl_driver_log *log;

    log = kmalloc(sizeof(struct bl_driver_log),GFP_KERNEL);
    if (log)
    {
        memset(log,0,sizeof(struct bl_driver_log));
        sprintf(log->name,"ehci%d",usb);
        log->proc_read = (read_proc_t *)p_show_ehci;
        log->seq_proc_read = (seq_read_t *)p_show_ehci_seq;
        log->proc_write = (write_proc_t *)p_commands;
        bl_register(log);
        return 0;
    }
    else
    {
        kfree(log);
        return 1;
    }
#else
{
#define MAX_NAME_LENGTH  40
    struct stat sts;
    char path[MAX_NAME_LENGTH];

    sprintf(path,"/proc/blutil/ehci%d",usb);
    if (sys_newstat(path, &sts) != 0)
    {
        if (bl_dir)
        {
            struct proc_dir_entry *bfile = NULL;
            sprintf(path,"ehci%d",usb);
            bfile = create_proc_entry(path, 0666, bl_dir);
            if(!bfile) 
                return -ENOMEM;
            bfile->read_proc = (read_proc_t *)p_show_ehci;
            bfile->write_proc = (write_proc_t *)p_commands;
            bfile->data = (void *)path;
        }
        else return -ENOMEM;
    }
}
#endif
    return 0;
}
#endif

MODULE_ALIAS("platform:bl-lilac-ehci");
#else
#warning "The module was already included"
#endif /* __USB_HC_EHCI_C_*/

