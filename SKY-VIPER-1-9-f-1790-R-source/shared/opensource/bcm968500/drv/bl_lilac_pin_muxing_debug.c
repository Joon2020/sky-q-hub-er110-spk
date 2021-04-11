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

#ifdef __MIPS_C

#include "bl_lilac_soc.h"
#include "bl_lilac_pin_muxing.h"
#include "blproc.h"
#define TEMP_LENGTH 3000

#define PIN_INFO(fmt, args...)    printk("BLMUX : %s(%d): ", fmt, __FUNCTION__ ,__LINE__ , ## args)

extern stt_uint32 pin_connections [LAST_PHYSICAL_PIN][REAL_NUMBER_OF_FUNCTIONS];
static char helpstrp [] = 
   "\n\r\tsetpad <pin> <strength> <pull or slew>\
   \n\r\tconnect <pin> <function> <number> <controller>\
   \n\r\toptions <number> (-1=all -2=rgmii -3=io)\
   \n\r\tmeanings\n\n\r";

static char *realfunctions [REAL_NUMBER_OF_FUNCTIONS] =
{
   "1588_ALARM",
   "1588_1PPS",
   "1588_CAPTURE",
   "1588_VCXO",
   "I2C_SCL_OUT",
   "I2C_SDA_OUT",
   "I2C_SCL_IN",
   "I2C_SDA_IN",
   "RX_GPON_SYNC",
   "NFMC_CEN",
   "NFMC_WPN",
   "NFMC_BOOT_FAIL", 
   "SPI_FLASH_CSN",
   "SPI_FLASH_WPN",
   "GRX_TDM_DATA",
   "GRX_TDM_CLK",
   "DSP_TDM_TXCLK",
   "DSP_TDM_RXCLK",
   "GTX_FROM_GPIO",
   "MIPSD_UART_TX",
   "MIPSD_UART_RX",
   "CLK_GPON_8KHz",
   "CLK_25_MHz",
   "CLK10_MHz_1588",
   "CLK10_MHz_SYNC",
   "SYNC_CLK_F",
   "CDR_TX_EN_P_F",
   "CDR_TX_EN_N_F",
   "FMI",
   "FSPI",
   "MDC_MDIO",
   "RGMII",
   "MII",
   "SPI",
   "TDM",
   "GPIO-MIPSC",
   "GPIO-MIPSD",
   "GPIO-RUNNER",
   "GPIO-VOIP",
   "TIMER0-MIPSC",
   "TIMER1-MIPSC",
   "TIMER2-MIPSC",
   "TIMER3-MIPSC", 
   "TIMER0-MIPSD",
   "TIMER1-MIPSD",
   "TIMER2-MIPSD",
   "TIMER3-MIPSD",
   "USB0_PRT_WR",
   "USB1_PRT_WR",
   "USB0_OVRCUR",
   "USB1_OVRCUR",
   "VOIP_TIMER0-IRQ",
   "VOIP_TIMER0-PWM",
   "VOIP_TIMER1-IRQ",
   "VOIP_TIMER1-PWM",
   "INTERRUPT0-MIPSC",
   "INTERRUPT1-MIPSC",
   "INTERRUPT2-MIPSC",
   "INTERRUPT3-MIPSC",
   "INTERRUPT4-MIPSC",
   "INTERRUPT5-MIPSC",
   "INTERRUPT6-MIPSC",
   "INTERRUPT7-MIPSC",
   "INTERRUPT8-MIPSC",
   "INTERRUPT9-MIPSC",
   "INTERRUPT0-MIPSD",
   "INTERRUPT1-MIPSD",
   "INTERRUPT2-MIPSD",
   "INTERRUPT3-MIPSD",
   "INTERRUPT4-MIPSD",
   "INTERRUPT5-MIPSD",
   "INTERRUPT6-MIPSD",
   "INTERRUPT7-MIPSD",
   "INTERRUPT8-MIPSD",
   "INTERRUPT9-MIPSD"
};
/*----------------------------------------------------
                   Static procedures
----------------------------------------------------*/
static int get_token(const char *buff);
static int p_show_pins(void *page, char **start, off_t off,int count, int *eof, void *data);
static int p_parse_pins(struct file *file, char *buffer,unsigned long count, void *data);
static int p_show_pins_seq(struct seq_file *f,void *data);

static int get_token(const char *buff)
{
    int len = 0;
    while (len < strlen(buff))
    {
        if ((buff[len] == ' ') || (buff[len] == '\n') || (buff[len] == '\r'))
            break;
        else 
            len ++;
    }
    return len;
}

static int print_io(char *f,int i)
{
    PIN_CONFIGURATION param;
    char function[20];
    char input[20];
    int length = 0;

    if (!fi_bl_lilac_mux_get_config_pin ((LILAC_PHYSICAL_PIN)i, &param, function, input))
    {
        length += sprintf(&f[length]," %d\t",i);
        length += sprintf(&f[length],"%s, (%s)\t\t",function,input);
        switch (param.strength)
        {
        case S6MA:
            length += sprintf(&f[length]," 6 mA\t");
            break;
        case S6_8MA:
            length += sprintf(&f[length]," 6-8 mA\t");
            break;
        case S8MA:
            length += sprintf(&f[length]," 8 mA\t");
            break;
        case S8_12MA:
            length += sprintf(&f[length]," 8-12 mA,");
            break;
        case S8_6MA:
            length += sprintf(&f[length]," 8-6 mA\t");
            break;
        case S8_6MA1: 
            length += sprintf(&f[length]," 8-6 mA\t");
            break;
        case S10_8MA:
            length += sprintf(&f[length]," 10-8 mA\t");
            break;
        case S10MA:
            length += sprintf(&f[length]," 10 mA\t");
            break;
        default:
            length += sprintf(&f[length],"\nUnkown strength : %d\n",param.strength);
            break;
        }
        switch (param.pull_slew)
        {
        case PULL_DOWN:
            length += sprintf(&f[length]," down\n");
            break;
        case PULL_UP:
            length += sprintf(&f[length]," up\n");
            break;
        case PULL_Z:
            length += sprintf(&f[length]," disable\n");
            break;
        default:
            length += sprintf(&f[length],"\nUnkown pull : %d\n",param.pull_slew);
            break;
        }
    }
    return length ;
}

static int print_rgmii(char *f,int i)
{
    PIN_CONFIGURATION param;
    char function[20];
    char input[20];
    int length = 0;

    if (!fi_bl_lilac_mux_get_config_pin ((LILAC_PHYSICAL_PIN)i, &param, function, input))
    {
        length += sprintf(&f[length]," %d\t",i);
        length += sprintf(&f[length],"%s, (%s)\t\t",function,input);
        switch (param.strength)
        {
        case DS6MA:
            length += sprintf(&f[length]," 6 mA\t");
            break;
        case DS8MA:
            length += sprintf(&f[length]," 8 mA\t");
            break;
        default:
            length += sprintf(&f[length],"\nUnkown strength : %d\n",param.strength);
        }
        length += sprintf(&f[length],"  %d\n",param.pull_slew); 
    }
    return length ; 
}

static int p_show_pins_seq(struct seq_file *f,void *data)
{
    int i,length;
    char *temp;

    length = 1;
    temp = kmalloc(TEMP_LENGTH, GFP_KERNEL);
    if (!temp)
        return -1;

    temp[0] = '\n';
    for (i=0; i< 49 && length < TEMP_LENGTH; i++)
    {
       length += print_io(&temp[length],i);
    }
    length += sprintf(&temp[length],"%s"," 49 not used\n 50 not used\n");
    for (i=RGMII_TX_D_0; i<= RGMII_TX_CTL && length < TEMP_LENGTH ; i++)
    {
        length += print_rgmii(&temp[length],i);
    }

    seq_puts(f,temp);
    kfree (temp);

    return length;
}
static int p_show_pins(void *b, char **start, off_t off,int count, int *eof, void *data)
{
    int i,l;
    char *buf;

    buf = (char *)b;
    l = 1;
    buf[0] = '\n';
    for (i=0; i< 49 && l < TEMP_LENGTH; i++)
    {
        l += print_io(&buf[l],i);
    }
    l += sprintf(&buf[l],"%s"," 49 not used\n 50 not used\n");
    for (i=RGMII_TX_D_0; i<= RGMII_TX_CTL && l < TEMP_LENGTH ; i++)
    {
        l += print_rgmii(&buf[l],i);
    }
    *eof = 1;
    
	return l;
}

static int p_parse_pins(struct file *file, char *buffer,unsigned long count, void *data)
{
    char token [MAX_NAME_LENGTH + 1];
    char *f,*tk;
    int p1,p2,p3,p4;
    PIN_CONFIGURATION config;
    LILAC_PIN_MUXING_RETURN_CODE retcode;
    int hh;

    f = kmalloc(count,GFP_KERNEL);
    if (!f)
    {
        PIN_INFO ("No allocated buffers\n");
        goto out;
    }
    memset(f,count,0);
    strcpy(f,buffer);
    hh = get_token(f);
    if (hh == 0)
    {
        PIN_INFO ("token ( %s ) does not exist\n",f);
        goto out;
    }
    memcpy(token,f,hh);
    token[hh] = '\0';
    
    /* token contains the required function */
    if (!strcmp(token,"help"))
    {
        blprintf(helpstrp);
        goto out;
    }
    if (!strcmp(token,"setpad"))
    {
        tk = &f[strlen(token)+1];
        hh = sscanf(tk,"%d %d %d",&p1,&p2,&p3);
        if (hh == 3)
        {
            config.strength = p2;
            config.pull_slew = p3;
            retcode = fi_bl_lilac_mux_config_pin(p1,&config);
            if (retcode)
                PIN_INFO("Set command failed : %d\n", (int)retcode);
            else
                blprintf("Done\n\r");
        }
        else
            PIN_INFO("Wrong command parameters : %d\n",hh);
        goto out;
    }
    if (!strcmp(token,"connect"))
    {
        tk = &f[strlen(token)+1];
        hh = sscanf(tk,"%d %d %d %d",&p1,&p2,&p3,&p4);
        if (hh == 4)
        {
             retcode = fi_bl_lilac_mux_connect_pin(p1,p2,p3,p4);
             if (retcode)
                 PIN_INFO("Connect command failed : %d\n", (int)retcode);
             else
                 blprintf("Done\n\r");
        }
        else
            PIN_INFO("Wrong command parameters : %d\n",hh);
        goto out;
    }
    if (!strcmp(token,"meanings"))
    {
        int j;
        for (j = 0;j < REAL_NUMBER_OF_FUNCTIONS/2; j+=2)
        {
            if (strlen(realfunctions[j]) >= 13)
                blprintf("%d\t= %s,\t%d\t= %s\n\r",j,realfunctions[j],j+1,realfunctions[j+1]);
            else    
                if (strlen(realfunctions[j]) <= 3)
                    blprintf("%d\t= %s,\t\t\t%d\t= %s\n\r",j,realfunctions[j],j+1,realfunctions[j+1]);
                else
                    blprintf("%d\t= %s,\t\t%d\t= %s\n\r",j,realfunctions[j],j+1,realfunctions[j+1]);
        }
        goto out;
    }
    if (!strcmp(token,"options"))
    {
        tk = &f[strlen(token)+1];
        hh = sscanf(tk,"%d",&p1);
        if (hh == 1)
        {
            int j;
            if ((p1 >= 0) && (p1 < LAST_PHYSICAL_PIN))
            {
                blprintf("\n\rFunctions options for pin %d :\n\r",p1);
                for (j = 0; j < REAL_NUMBER_OF_FUNCTIONS; j++)
                {
                    if (pin_connections[p1][j])
                    {
                        blprintf("%d ",j);
                    }
                }
                blprintf("\n\n\r");
            }
            else 
            {
                int k,start,end;
                switch (p1)
                {
                case -1:
                    start = 0;
                    end = LAST_PHYSICAL_PIN;
                    break;
                case -2:
                    start = RGMII_TX_D_0;
                    end = LAST_PHYSICAL_PIN;
                    break;
                case -3:
                    start = 0;
                    end = RGMII_TX_D_0;
                    break;
                default:
                    PIN_INFO("Wrong command parameter : %d",p1);
                    start = 0;
                    end = 0;
                    break;
                }
                blprintf("\n\rFunctions options\n\r");
                for (k = start; k < end; k ++)
                {
                    blprintf("\n\r%d :\t",k);
                    for (j = 0; j < REAL_NUMBER_OF_FUNCTIONS; j++)
                    {
                        if (pin_connections[k][j])
                        {
                            blprintf("%d ",j);
                        }
                    }
                }
                blprintf("\n\n\r");
            }
        }
        else
            PIN_INFO("Wrong command parameters : %d\n",hh);
        goto out;
    }
    else
        PIN_INFO("Wrong command : %s\n",token);

out:
    kfree(f);
    return count;
}

static int __init init_blpin(void)
{
    struct bl_driver_log *log;
    log = kmalloc(sizeof(struct bl_driver_log),GFP_KERNEL);
    if (log)
    {
        memset(log,0,sizeof(struct bl_driver_log));
        strcpy(log->name,"pinmux");
        log->seq_proc_read = (seq_read_t *)p_show_pins_seq;
        log->proc_read = (read_proc_t *)p_show_pins;
        log->proc_write = (write_proc_t *)p_parse_pins;
        bl_register(log);
    }
    else
    {
        if (log)
            kfree(log);
    }
    return 0;
}

static void __exit cleanup_blpin(void)
{
    bl_unregister("pinmux");
}
module_init(init_blpin);
module_exit(cleanup_blpin);
#endif

MODULE_DESCRIPTION("pinmuxing");
MODULE_LICENSE("GPL");
