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


#ifndef __USB_HC_OHCI_INCLUDED
#define __USB_HC_OHCI_INCLUDED

#include <linux/platform_device.h>
#include "bl_lilac_soc.h"
#include "bl_lilac_mips_blocks.h"
#include "bl_lilac_usb_hc.h"
#include "bl_lilac_ic.h"

#ifdef CONFIG_BLUSB_DEBUG
#include <linux/syscalls.h>
extern struct proc_dir_entry *bl_dir;
static int registerForDebug(int usb);
#endif

extern int usb_hcd_request_irqs(struct usb_hcd *hcd,
unsigned int irqnum, unsigned long irqflags);
static int irqnum;

/********************************************************/
/* callback for start of OHCI part of the controller    */
/********************************************************/
int bl_ohci_run (struct usb_hcd *hcd)
{
    struct bl_usb_hcd_s *current_ohci;
	struct ohci_hcd	*ohci = hcd_to_ohci (hcd);
    int ret = -1;
    int ctrl = 0;

    if (irqnum - INT_NUM_IRQ0 == BL_LILAC_IC_INTERRUPT_USB_1)
        ctrl = 1;
    current_ohci = blusbhcd[ctrl];
	if ((ret = ohci_init(ohci)) < 0)
		goto end;

    if (usb_hcd_is_primary_hcd(hcd) && irqnum) {
		ret = usb_hcd_request_irqs(hcd, irqnum, IRQF_SHARED);
		if (ret)
			goto end;
	}
    
	ret = ohci_run (ohci);
	if (ret < 0) {
		ohci_err (ohci, "can't start\n");
		ohci_stop (hcd);
	}

end:
    bl_lilac_ohci_link(ctrl,0);
	return ret;
}
/********************************************************/
/* callback for free device - called on disconnect      */
/* - returns all structures and hw to EHCI function     */
/********************************************************/
static void bl_free_dev(struct usb_hcd *hcd, struct usb_device *udev)
{
    int ctrl = 0;

    if (hcd->irq - INT_NUM_IRQ0 == BL_LILAC_IC_INTERRUPT_USB_1)
        ctrl = 1;
    bl_lilac_ohci_link(ctrl,0);
    blusbhcd[ctrl]->works_as_ohci = 0;
    USB_INFO("BL USB_%d Controller works as EHCI\n",ctrl);
}
static const struct hc_driver bl_lilac_ohci_driver = {
	.description        = "bl_lilac_ohci",
	.product_desc       = "LILAC USB OHCI Host Controller",
	.hcd_priv_size      = sizeof(struct ohci_hcd),
	.irq                = bl_ehci_irq,
	.flags              = (int)(HCD_LOCAL_MEM | HCD_MEMORY | HCD_USB11),
	.start              = bl_ohci_run,
	.stop               = ohci_stop,
	.shutdown           = ohci_shutdown,
	.urb_enqueue        = ohci_urb_enqueue,
	.urb_dequeue        = ohci_urb_dequeue,
	.endpoint_disable   = ohci_endpoint_disable,
	.get_frame_number   = ohci_get_frame,
	.hub_status_data    = ohci_hub_status_data,
	.hub_control        = ohci_hub_control,
    .free_dev           = bl_free_dev,
#ifdef	CONFIG_PM
	.bus_suspend        = ohci_bus_suspend,
	.bus_resume         = ohci_bus_resume,
#endif
	.start_port_reset   = ohci_start_port_reset,
};

#ifdef CONFIG_BLUSB_DEBUG
static char * dump_ohci(struct ohci_hcd * ohcihcd)
{
#define BUFFER_SIZE 500
    int l=0;
    char *temp = kmalloc(BUFFER_SIZE,GFP_KERNEL);
    if (!temp)
        return NULL;
    memset(temp,0,BUFFER_SIZE);
    l += sprintf(&temp[l],"\n\rOHCI regs   = 0x%08x\n\r",(unsigned int)ohcihcd->regs);
    l += sprintf(&temp[l],"HCCA        = 0x%08x\n\r",(unsigned int)ohcihcd->hcca);
    l += sprintf(&temp[l],"BulkTail    = 0x%08x\n\r",(unsigned int)ohcihcd->ed_bulktail);
    l += sprintf(&temp[l],"ControlTail = 0x%08x\n\r",(unsigned int)ohcihcd->ed_controltail);
    l += sprintf(&temp[l],"TD          = 0x%08x\n\r",(unsigned int)ohcihcd->td_cache);
    sprintf(&temp[l],"ED          = 0x%08x\n\r",(unsigned int)ohcihcd->ed_cache);

#undef BUFFER_SIZE
    return temp;
}
static int p_show_ohci(void *b, char **start, off_t off,int count, int *eof, void *data)
{
    char *buf,*temp;
    int length;

    buf = (char *)b;
    length = 0;
    temp = NULL;
    if (data)
    {
        if (!strcmp(data,"ohci0") && blusbhcd[0])
        {
            temp = dumpstruct("\nOHCI 0\n=======\n",(void *)hcd_to_ohci(blusbhcd[0]->ohcihcd),sizeof(struct ohci_hcd)/4,5);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dump_ohci(hcd_to_ohci(blusbhcd[0]->ohcihcd));
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dumpstruct("\nHCD 0\n=====\n",(void *)blusbhcd[0]->ohcihcd,sizeof(struct usb_bus)/4,5);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dumpstruct("",(void *)((unsigned int)blusbhcd[0]->ohcihcd+sizeof(struct usb_bus)),(sizeof(struct usb_hcd)-sizeof(struct usb_bus))/4,5);
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
        if (!strcmp(data,"ohci1") && blusbhcd[1])
        {
            temp = dumpstruct("\nOHCI 1\n=======\n",(void *)hcd_to_ohci(blusbhcd[1]->ohcihcd),sizeof(struct ohci_hcd)/4,5);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dump_ohci(hcd_to_ohci(blusbhcd[1]->ohcihcd));
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dumpstruct("\nHCD 1\n=====\n",(void *)blusbhcd[1]->ohcihcd,sizeof(struct usb_bus)/4,5);
            if (temp)
            {
                length += sprintf(&buf[length],"%s",temp);
                kfree(temp);
            }
            temp = dumpstruct("",(void *)((unsigned int)blusbhcd[1]->ohcihcd+sizeof(struct usb_bus)),(sizeof(struct usb_hcd)-sizeof(struct usb_bus))/4,5);
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

void p_ohci_stat(const unsigned char *ffile)
{
    u32	status[4];
    struct ohci_hcd	*ohci;

    if (!strcmp(ffile,"ohci0"))
        ohci = hcd_to_ohci(blusbhcd[0]->ohcihcd);
    else
        ohci = hcd_to_ohci(blusbhcd[1]->ohcihcd);
    status[0] = ohci_readl(ohci, &ohci->regs->control);
    status[1] = ohci_readl(ohci, &ohci->regs->cmdstatus);
    status[2] = ohci_readl(ohci, &ohci->regs->intrstatus);
    status[3] = ohci_readl(ohci, &ohci->regs->intrenable);
    printk("HcControl : 0x%08x\n\r",status[0]);
    printk("HcCommandStatus : 0x%08x\n\r",status[1]);
    printk("HcInterruptStatus : 0x%08x\n\r",status[2]);
    printk("HcInterruptEnable : 0x%08x\n\r",status[3]);
}

#ifdef WITH_BLPROC
static int p_show_ohci_seq(struct seq_file *f,void *data)
{
    char *temp = NULL;
    int len = 0;

    if (data)
    {
        if (!strcmp(data,"ohci0") && blusbhcd[0])
        {
            temp = dump_ohci(hcd_to_ohci(blusbhcd[0]->ohcihcd));
            len += strlen(temp);
            seq_puts(f,temp);
            kfree(temp);
        }
        else if (!strcmp(data,"ohci1") && blusbhcd[1])
        {
            temp = dump_ohci(hcd_to_ohci(blusbhcd[1]->ohcihcd));
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
        sprintf(log->name,"ohci%d",usb);
        log->proc_read = (read_proc_t *)p_show_ohci;
        log->seq_proc_read = (seq_read_t *)p_show_ohci_seq;
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
#define MAX_NAME_LENGTH  20
    struct stat sts;
    char path[MAX_NAME_LENGTH*2];

    sprintf(path,"/proc/blutil/ohci%d",usb);
    if (sys_newstat(path, &sts) != 0)
    {
        if(bl_dir) 
        {
            struct proc_dir_entry *bfile = NULL;
            sprintf(path,"ohci%d",usb);
            bfile = create_proc_entry(path, 0666, bl_dir);
            if(!bfile) 
                return -ENOMEM;
            bfile->read_proc = (read_proc_t *)p_show_ohci;
            bfile->write_proc = (write_proc_t *)p_commands;
            bfile->data = (void *)path;
        }
        else return -ENOMEM;
    }
} // else
#endif // with_blproc
    return 0;
}
#endif // blusb_debug
/********************************************************/
/* create structures for OHCI part of the controller    */
/* perform first step initializations                   */
/********************************************************/
static int bl_lilac_probe(struct platform_device *pdev)
{
	struct usb_hcd *hcd;
    struct ohci_hcd *ohci;
	struct resource *res;
    char name [60];
    int ret = 0;
    BL_USB_HOST_CONTROLLER usb = pdev->id;

    sprintf(name,"bl_usb_ohci_%d",usb);
	hcd = usb_create_hcd(&bl_lilac_ohci_driver, &pdev->dev, name);
	if (!hcd) {
        USB_EPRINTK("Cannot allocate USB structure for %s\n",name);
		return -ENOMEM;
	}
    
    blusbhcd[usb]->ohcihcd = hcd;
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0 );
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
	hcd->regs = ioremap(hcd->rsrc_start, hcd->rsrc_len);
	if (!hcd->regs) {
        USB_EPRINTK("Failed on remap USB address for %s\n",name);
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
    ohci = hcd_to_ohci(hcd);
	ohci_hcd_init(ohci);
    bl_lilac_ohci_link(usb,1);
	ret = usb_add_hcd(hcd, 0, IRQF_SHARED);
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
    blusbhcd[usb]->ohcihcd = NULL;

    return 0;
}
/********************************************************/
/* callback for remove USB controller (OHCI part of it) */
/********************************************************/
static int ohci_hcd_bl_lilac_drv_remove(struct platform_device *pdev)
{
	struct usb_hcd *hcd = platform_get_drvdata(pdev);
    char name[10];

	usb_remove_hcd(hcd);
	iounmap(hcd->regs);
	usb_put_hcd(hcd);
	platform_set_drvdata(pdev, NULL);
    sprintf(name,"ohci%d",pdev->id);
#ifdef CONFIG_BLUSB_DEBUG
#ifdef WITH_BLPROC
        bl_unregister(name);
#else
        if (bl_dir)
            remove_proc_entry(name, bl_dir);
#endif
#endif

	return 0;
}
/********************************************************/
/* callback for probe  USB controller (OHCI part of it) */
/********************************************************/
static int ohci_hcd_bl_lilac_drv_probe(struct platform_device *pdev)
{
	if (usb_disabled())
		return -ENODEV;

    if (!bl_lilac_probe(pdev))
    {
#ifdef CONFIG_BLUSB_DEBUG
        registerForDebug(pdev->id);
#endif
        return 0;
    }

    return 1;
}

#ifdef CONFIG_PM
/********************************************************/
/* callback for suspend USB 1.1 device                  */
/********************************************************/
static int ohci_hcd_bl_drv_suspend(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);
	struct ohci_hcd	*ohci = hcd_to_ohci(hcd);
	unsigned long flags;
	int rc;

	rc = 0;
	spin_lock_irqsave(&ohci->lock, flags);
	if (hcd->state != HC_STATE_SUSPENDED) {
		rc = -EINVAL;
		goto bail;
	}
	ohci_writel(ohci, OHCI_INTR_MIE, &ohci->regs->intrdisable);
	(void)ohci_readl(ohci, &ohci->regs->intrdisable);

	clear_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
bail:
	spin_unlock_irqrestore(&ohci->lock, flags);
	return rc;
}
/********************************************************/
/* callback for resume USB 1.1 device                   */
/********************************************************/
static int ohci_hcd_bl_drv_resume(struct device *dev)
{
	struct usb_hcd *hcd = dev_get_drvdata(dev);

	set_bit(HCD_FLAG_HW_ACCESSIBLE, &hcd->flags);
	ohci_finish_controller_resume(hcd);

	return 0;
}
static const struct dev_pm_ops bl_ohci_pmops = {
	.suspend	= ohci_hcd_bl_drv_suspend,
	.resume		= ohci_hcd_bl_drv_resume,
};


#endif
static struct platform_driver ohci_hcd_bl_lilac_driver = {
	.probe		= ohci_hcd_bl_lilac_drv_probe,
	.remove		= ohci_hcd_bl_lilac_drv_remove,
	.shutdown	= usb_hcd_platform_shutdown,
	.driver = {
		.name	= "bl-lilac-ohci",
#ifdef CONFIG_PM
        .pm	= (struct dev_pm_ops *)&bl_ohci_pmops,
#endif
	}
};

MODULE_ALIAS("platform:bl-lilac-ohci");

#else
#warning "The module was always included"
#endif
