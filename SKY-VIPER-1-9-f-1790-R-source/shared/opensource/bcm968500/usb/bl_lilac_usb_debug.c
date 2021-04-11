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

#ifdef WITH_BLPROC
 #include "../../../../../../blmisc/blproc.h"
 #define printk(fmt, args...) blprintf(fmt, ##args)
#endif
extern void p_ohci_stat(const unsigned char *ffile);
extern void p_ehci_stat(const unsigned char *ffile);
            
char helpstrp [] = 
   "\n\r\tgetstat - read status registers\
   \n\r\tmsgs <on | von | off> - trace messages, v = verbose\
   \n\r\tregs <on | off> - trace registers\n\n\r";

#define bTypeStrLength   10
static char * bTypeStr [bTypeStrLength] =
{
    "USB_REQ_GET_STATUS ",
    "USB_REQ_CLEAR_FEATURE ",
    " ",
    "USB_REQ_SET_FEATURE ",
    " ",
    "USB_REQ_SET_ADDRESS ",
    "USB_REQ_GET_DESCRIPTOR ",
    "USB_REQ_SET_DESCRIPTOR ",
    "USB_REQ_GET_CONFIGURATION ",
    "USB_REQ_SET_CONFIGURATION "
};
#define wValueStrLength   5
static char * wValueStr [wValueStrLength] =
{
    "USB_PORT_FEAT_CONNECTION",	
    "USB_PORT_FEAT_ENABLE",		
    "USB_PORT_FEAT_SUSPEND",		
    "USB_PORT_FEAT_OVER_CURRENT",	
    "USB_PORT_FEAT_RESET"
};
#define wValueStrLength1   4
static char * wValueStr1 [wValueStrLength1] =
{
    "USB_PORT_FEAT_POWER",	//8	
    "USB_PORT_FEAT_LOWSPEED",
    "USB_PORT_FEAT_HIGHSPEED",
    "USB_PORT_FEAT_SUPERSPEED"
};
#define wValueStrLength2   5
static char * wValueStr2 [wValueStrLength2] =
{
    "USB_PORT_FEAT_C_CONNECTION",//16	
    "USB_PORT_FEAT_C_ENABLE",		
    "USB_PORT_FEAT_C_SUSPEND",		
    "USB_PORT_FEAT_C_OVER_CURRENT",	
    "USB_PORT_FEAT_C_RESET"		
};
/********************************************************/
/* print usb_hcd structure - almost all its fields      */
/********************************************************/
char * dump_hcd(struct usb_hcd * hcd)
{
#define BUFFER_SIZE 500
    int l=0;
    char *temp = kmalloc(BUFFER_SIZE,GFP_KERNEL);
    if (!temp)
        return NULL;
    memset(temp,0,BUFFER_SIZE);

    { // print bus struct
        l += sprintf(&temp[l],"\n\rBus struct:\n\r");
        l += sprintf(&temp[l],"\tbusnum = %d\n\r",hcd->self.busnum);
        if (!hcd->self.hs_companion)
            l += sprintf(&temp[l],"\tNo HighSpeed companion defined\n\r");
        else
        {
            l += sprintf(&temp[l],"\tHighSpeed companion Bus struct:\n\r");
            l += sprintf(&temp[l],"\t\tbusnum = %d\n\r",hcd->self.hs_companion->busnum);
        }
        l += sprintf(&temp[l],"\tDevice Map: 0x%08x 0x%08x 0x%08x 0x%08x\n\r",
                 (unsigned int)hcd->self.devmap.devicemap[0],
                 (unsigned int)hcd->self.devmap.devicemap[1],
                 (unsigned int)hcd->self.devmap.devicemap[2],
                 (unsigned int)hcd->self.devmap.devicemap[3]);
    }
    { // current urb
        l += sprintf(&temp[l],"Current URB pointer = 0x%08x\n\r",(unsigned int)hcd->status_urb);
    }
#ifdef CONFIG_USB_SUSPEND
    { // remote wakeup
        l += sprintf(&temp[l],"Current Work : 0x%08x 0x%08x\n\r",
                 hcd->wakeup_work.data.counter,
                 (unsigned int)hcd->wakeup_work.func);
    }
#endif
    { // resources
        l += sprintf(&temp[l],"IRQ = %d, Regs = 0x%08x\n\r",hcd->irq,(unsigned int)hcd->regs);
    }
    l += sprintf(&temp[l],"State = 0x%08x ",(unsigned int)hcd->state);
    if (!hcd->state)
    {
        l += sprintf(&temp[l],"( HALT )\n\r");    
    }
    else if (HC_IS_RUNNING(hcd->state))
    {
        l += sprintf(&temp[l],"( ACTIVE )\n\r");    
    }
    else if (HC_IS_SUSPENDED(hcd->state))
    {
        l += sprintf(&temp[l],"( SUSPENDED )\n\r");    
    }
    sprintf(&temp[l],"\n\r");

#undef BUFFER_SIZE
    return temp;
}
/********************************************************/
/* print the read/write to registers of EHCI/OHCI       */
/*  - if trace is ON                                    */
/********************************************************/
void bl_ehci_regs_debug(int action,__u32 __iomem * regs, unsigned int val)
{
    int i = 0;

    if (((unsigned int)regs >= BL_DEVICE_ADDRESS(CE_USB_BLOCK_EHCI1_CAP_REGS_ADDRESS)) &&
        ((unsigned int)regs <= BL_DEVICE_ADDRESS(CE_USB_BLOCK_OHCI1_SYN_REGS_ADDRESS)))
        i = 1;
    if (!blusbhcd[i])
        return;
    
    if (blusbhcd[i]->debug_on_registers) 
    {
        switch (action)
        {
        case 0:
            printk("EHCI_%d 0x%08x -> 0x%08x\n",i,(unsigned int)regs,val);
            break;
        case 1:
            printk("EHCI_%d 0x%08x <- 0x%08x\n",i,(unsigned int)regs,val);
            break;
        case 2:
            printk("OHCI_%d 0x%08x -> 0x%08x\n",i,(unsigned int)regs,val);
            break;
        case 3:
            printk("OHCI_%d 0x%08x <- 0x%08x\n",i,(unsigned int)regs,val);
            break;
        }
    }
}
/********************************************************/
/* decode control messages                              */
/********************************************************/
static void decode_pipe(unsigned int pipe, char * dir, char * type, int * devaddr, int * endp)
{
    if (usb_pipein(pipe))
        strcpy(dir,"IN");
    else
        strcpy(dir,"OUT");

    if (usb_pipecontrol(pipe))
        strcpy(type,"CONTROL");
    else if (usb_pipebulk(pipe))
        strcpy(type,"BULK");
    else if (usb_pipeint(pipe))
        strcpy(type,"INTERRUPT");
    else
        strcpy(type,"ISOCHRONOUS");
    *devaddr = usb_pipedevice(pipe);
    *endp = usb_pipeendpoint(pipe);
}
/********************************************************/
/* print the size of the received message - if trace ON */
/* print the received message - if trace is verbose     */
/********************************************************/
void bl_ehci_rcvd_debug(struct usb_device *dev,void * data, u16 size)
{
    unsigned char *pp = (unsigned char *)data;
    int y,ctrl;

    // for the case the registration failed
    if (!dev) return;
    ctrl = dev->slot_id;
    if (!blusbhcd[ctrl]) return;
    if (blusbhcd[ctrl]->debug_on_messages)
    {
        printk(KERN_DEBUG "Received Message from BLUSB_%d : size=%d\n",ctrl,size);
        if (blusbhcd[ctrl]->debug_on_messages == TRACE_VON)
        {
            if (data)
            {
                if (size >= 0)
                {
                    for (y=0; y<size; y++)
                    {
                        printk(KERN_DEBUG "%02x ",*pp++);
                    }
                    printk(KERN_DEBUG "\n");
                }
                else
                    printk("Data's length < 0\n");
            }
            else
                printk(KERN_DEBUG "Data's buffer == 0\n");
        }
    }
}
/********************************************************/
/* decode control messages - if trace is on             */
/* print the sent message - if trace is verbose         */
/********************************************************/
void bl_ehci_msgs_debug(struct usb_device *dev,unsigned int pipe, u16 size,struct usb_ctrlrequest *dr)
{
    char dir[4];
    char type[12];
    int da,ep,ctrl;
    int y,extra = 0;
    unsigned char *pp = (unsigned char *)dr;

    // for the case the registration failed
    if (!dev) return;
    ctrl = dev->slot_id;
    if (!blusbhcd[ctrl]) return;
    if ((blusbhcd[ctrl]->debug_on_messages) && dr)
    {
        y = dr->bRequestType & 0x7F;
        decode_pipe(pipe, dir, type, &da, &ep);
        printk(KERN_DEBUG "Message for BLUSB_%d (%s %s %d:%d)\n",
               ctrl,dir,type,da,ep);
        if (usb_pipecontrol(pipe))
        {
            extra = sizeof(struct usb_ctrlrequest);
            if (y == USB_RT_HUB) 
            {
                printk(KERN_DEBUG "bRequestType=HUB (0x%x), ",dr->bRequestType);
            }
            else if (y == USB_RT_PORT)
            {
                printk(KERN_DEBUG "bRequestType=PORT (0x%x), ",dr->bRequestType);
            }
            else if (y == 0x00)
            {
                printk(KERN_DEBUG "bRequestType=STD (0x%x), ",dr->bRequestType);
            }
            if (dr->bRequest < bTypeStrLength)
            {
                printk(KERN_DEBUG "bRequest=%s(0x%x), ",bTypeStr[dr->bRequest],dr->bRequest);
            }
            if (dr->wValue < wValueStrLength)
            {
                printk(KERN_DEBUG "wValue=%s (0x%x), ",wValueStr[dr->wValue],dr->wValue);
            }
            else if (dr->wValue-8 < wValueStrLength1)
            {
                printk(KERN_DEBUG "wValue=%s (0x%x), ",wValueStr1[dr->wValue-8],dr->wValue);
            }
            else if (dr->wValue-16 < wValueStrLength2)
            {
                printk(KERN_DEBUG "wValue=%s (0x%x), ",wValueStr2[dr->wValue-16],dr->wValue);
            }
            else
                printk(KERN_DEBUG "wValue=(0x%x), ",dr->wValue);
            printk(KERN_DEBUG "wIndex=0x%x, wLength=0x%x\n",dr->wIndex, dr->wLength);
        }
        if (blusbhcd[ctrl]->debug_on_messages == TRACE_VON)
        {
            for (y=0; y<size+extra; y++)
            {
                printk(KERN_DEBUG "%02x ",*pp++);
            }
            printk(KERN_DEBUG "\n");
        }
    }
}

/********************************************************/
/* callback for 'echo' commands into ehciX, ohciX files */
/********************************************************/
int p_commands(struct file *file, char *buffer,unsigned long count, void *data)
{
    char *f,*tk;

    f = kmalloc(count,GFP_KERNEL);
    if (!f)
    {
        USB_EPRINTK ("No allocated buffers\n");
        goto out;
    }
    memset(f,count,0);
    strncpy(f,buffer,count);
    f[count] = 0;
    f = strim(f);
    tk = strsep(&f," ");
    if (tk == 0)
    {
        USB_EPRINTK ("token ( %s ) does not exist\n",f);
        goto out;
    }
    /* token contains the required function */
    if (!strcmp(strim(tk),"help"))
    {
        printk(helpstrp);
        if (!strcmp(file->f_path.dentry->d_name.name,"ehci0") ||
             !strcmp(file->f_path.dentry->d_name.name,"ohci0"))
        {
            printk("TRACES : regs=%d msgs=%d\n\r",
                     blusbhcd[0]->debug_on_registers,
                     blusbhcd[0]->debug_on_messages);
            if (blusbhcd[0]->works_as_ohci) 
                printk("Works as OHCI\n\r");
            else
                printk("Works as EHCI\n\r");
        }
        else
        {
            printk("TRACES : regs=%d msgs=%d\n\r",
                     blusbhcd[1]->debug_on_registers,
                     blusbhcd[1]->debug_on_messages);
            if (blusbhcd[1]->works_as_ohci) 
                printk("Works as OHCI\n\r");
            else
                printk("Works as EHCI\n\r");
        }
        goto out;
    }
    if (!strcmp(strim(tk),"getstat"))
    {
        if (!strcmp(file->f_path.dentry->d_name.name,"ehci0") ||
            !strcmp(file->f_path.dentry->d_name.name,"ehci1"))
            p_ehci_stat(file->f_path.dentry->d_name.name);
        //else
            //p_ohci_stat(file->f_path.dentry->d_name.name);
    }
    else if (!strcmp(strim(tk),"regs"))
    {
        tk = strsep(&f," ");
        if (!strcmp(strim(tk),"on"))
        {
            if (!strcmp(file->f_path.dentry->d_name.name,"ehci0") ||
                 !strcmp(file->f_path.dentry->d_name.name,"ohci0"))
                blusbhcd[0]->debug_on_registers = TRACE_ON;
            else
                blusbhcd[1]->debug_on_registers = TRACE_ON;
        }
        else
        {
            if (!strcmp(file->f_path.dentry->d_name.name,"ehci0") ||
                 !strcmp(file->f_path.dentry->d_name.name,"ohci0"))
                blusbhcd[0]->debug_on_registers = TRACE_OFF;
            else
                blusbhcd[1]->debug_on_registers = TRACE_OFF;
        }
    }
    else if (!strcmp(strim(tk),"msgs"))
    {
        tk = strsep(&f," ");
        if (!strcmp(strim(tk),"on"))
        {
            if (!strcmp(file->f_path.dentry->d_name.name,"ehci0") ||
                 !strcmp(file->f_path.dentry->d_name.name,"ohci0"))
                blusbhcd[0]->debug_on_messages = TRACE_ON;
            else
                blusbhcd[1]->debug_on_messages = TRACE_ON;
        }
        else if (!strcmp(strim(tk),"von"))
        {
            if (!strcmp(file->f_path.dentry->d_name.name,"ehci0") ||
                 !strcmp(file->f_path.dentry->d_name.name,"ohci0"))
                blusbhcd[0]->debug_on_messages = TRACE_VON;
            else
                blusbhcd[1]->debug_on_messages = TRACE_VON;
        }
        else
        {
            if (!strcmp(file->f_path.dentry->d_name.name,"ehci0") ||
                 !strcmp(file->f_path.dentry->d_name.name,"ohci0"))
                blusbhcd[0]->debug_on_messages = TRACE_OFF;
            else
                blusbhcd[1]->debug_on_messages = TRACE_OFF;
        }
    }
    else
        USB_EPRINTK("Wrong command\n");
out:
    kfree(f);
    return count;
}
// l is the number of words to be printed
// level is the number of words per line
char *dumpstruct(char *txt, void *p, int l, int level)
{
    int i,j,length,sz;
    unsigned int *t ;
    unsigned int tv;
    char *temp;

    if (!p)
        return NULL;
    // i is the number of lines
    // each line has also 10 + 3 chars for address and level-1 spaces + \n
    i = l / level;
    if (l % level)
        i++;
    // number of chars on line : each word has 10 chars, space after every word and \n
    j = level * 10 + level - 1 + 1;
    j += 13; // number of chars of the address and separators
    sz = strlen(txt) + j*i; // each word is 10 chars
    temp = kmalloc(sz,GFP_KERNEL);
    if (!temp)
        return NULL;
    memset(temp,0,sz);
    t = (unsigned int *)p;
    length = 0;
    if (strlen(txt))
        length += sprintf(temp,"%s",txt);
    i = l / level;
    for (j = 0; j < i; j++)
    {
        length += sprintf(&temp[length],"0x%08X : ",(unsigned int)t); // address
        for (sz = 0; sz < level; sz++)
        {
            tv = *t;
            length += sprintf(&temp[length]," 0x%08X",tv);
            t++;
        }
        length += sprintf(&temp[length],"\n");
    }
    i = l % level;
    if (i)
    {
       length += sprintf(&temp[length],"0x%08X : ",(unsigned int)t); // address
       for (sz = 0; sz < i; sz++)
       {
           tv = *t;
           length += sprintf(&temp[length]," 0x%08X",tv);
           t++;
       }
       length += sprintf(&temp[length],"\n");
    }
    return temp;
}
