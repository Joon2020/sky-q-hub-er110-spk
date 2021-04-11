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

#ifndef BL_LILAC_USB_H_  
#define BL_LILAC_USB_H_

#ifdef __cplusplus
extern "C"
{
#endif

/*-------------------------------------------------
        Defines the return code from APIs
-------------------------------------------------*/
//#define CONFIG_BLUSB_DEBUG
//#define WITH_BLPROC

typedef enum
{
    USB0,
    USB1
} BL_USB_HOST_CONTROLLER;

typedef enum
{
    RESET_USB,
    OUT_OF_RESET_USB
} BL_USB_HC_RESET;

enum 
{
    TRACE_OFF,
    TRACE_ON,
    TRACE_VON
};

typedef struct bl_usb_hcd_s
{
    struct usb_hcd * ehcihcd;
    struct usb_hcd * ohcihcd;
    u8 works_as_ohci;
    u8 debug_on_registers;
    u8 debug_on_messages;
} bl_usb_hcd;

#define USB_DPRINTK(fmt, args...)
#define USB_INFO(fmt, args...) printk(KERN_INFO  "BLUSB : %s(%d): " fmt, __FUNCTION__ ,__LINE__ , ## args)
#define USB_EPRINTK(fmt, args...) printk(KERN_ERR  "BLUSB : %s(%d): " fmt, __FUNCTION__ ,__LINE__ , ## args)

extern int usb_disabled(void);
void bl_lilac_ohci_link(int usb,int action);

extern struct bl_usb_hcd_s * blusbhcd[];
extern irqreturn_t bl_ehci_irq(struct usb_hcd *hcd);
extern void bl_usb_reset(BL_USB_HOST_CONTROLLER usb, BL_USB_HC_RESET action);
extern void bl_usb_ahb(BL_USB_HOST_CONTROLLER usb);
#ifdef CONFIG_BLUSB_DEBUG
char *dumpstruct(char * txt, void *p, int l, int level);
int p_commands(struct file *file, char *buffer,unsigned long count, void *data);
char *dump_hcd(struct usb_hcd * hcd);
void bl_ehci_msgs_debug(struct usb_device *dev,unsigned int pipe, u16 size,struct usb_ctrlrequest *dr);
void bl_ehci_rcvd_debug(struct usb_device *dev,void * data, u16 size);
#endif

#ifdef __cplusplus
}
#endif

#endif 
