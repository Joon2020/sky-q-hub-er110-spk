
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

#include "boardparms.h"
#include "shared_utils.h"

#ifdef _CFE_                                                
#include "lib_types.h"
#include "lib_printf.h"
#include "lib_string.h"
#include "bcm_map.h"
#define printk  printf
#else // Linux
#include <linux/kernel.h>
#include <linux/module.h>
#include <bcm_map_part.h>
#include <linux/string.h>
#endif

#if defined (CONFIG_BCM96838) || defined(_BCM96838_)
#ifndef _CFE_             
#include <linux/spinlock.h>                                   
static spinlock_t brcm_sharedUtilslock;
#endif
#endif

#if defined (CONFIG_BCM968500) || defined(_BCM968500_)
#include "bl_lilac_soc.h"
#endif

#if !defined(CONFIG_BCM968500) && !defined(_BCM968500_)
unsigned int UtilGetChipRev(void)
{
    unsigned int revId;
    revId = PERF->RevID & REV_ID_MASK;

    return  revId;
}
#if !defined(_CFE_)
EXPORT_SYMBOL(UtilGetChipRev);
#endif
#endif

char *UtilGetChipName(char *buf, int len) {
#if defined (CONFIG_BCM968500) || defined(_BCM968500_)
		int chipId =  bl_lilac_soc_id();
		char *mktname = NULL;
#else
    unsigned int chipId = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;
    unsigned int revId;
    char *mktname = NULL;
    revId = (int) (PERF->RevID & REV_ID_MASK);
#endif

#if  defined (_BCM96818_) || defined(CONFIG_BCM96818)
   unsigned int var = (BRCM_VARIANT_REG & BRCM_VARIANT_REG_MASK) >> BRCM_VARIANT_REG_SHIFT;

    switch ((chipId << 8) | var) {
	case(0x681100):
		mktname = "6812B";
		break;
	case(0x681101):
		mktname = "6812R";
		break;
	case(0x681503):
		mktname = "6812GU";
		break;
	case(0x681500):
		mktname = "6818SR";
		break;
	case(0x681700):
		mktname = "6818G";
		break;
	case(0x681701):
		mktname = "6818GS";
		break;
	case(0x681501):
		mktname = "6818GR";
		break;
	case(0x681502):
		mktname = "6820IAD";
		break;
	default:
		mktname = NULL;
    }

#elif  defined (_BCM96828_) || defined(CONFIG_BCM96828)
#if defined(CHIP_VAR_MASK)
        unsigned int var = (PERF->RevID & CHIP_VAR_MASK) >> CHIP_VAR_SHIFT;
#endif
    switch ((chipId << 8) | var) {
	case(0x682100):
		mktname = "6821F";
		break;
	case(0x682101):
		mktname = "6821G";
		break;
	case(0x682200):
		mktname = "6822F";
		break;
	case(0x682201):
		mktname = "6822G";
		break;
	case(0x682800):
		mktname = "6828F";
		break;
	case(0x682801):
		mktname = "6828G";
		break;
	default:
		mktname = NULL;
		break;
    }
#elif  defined (_BCM96318_) || defined(CONFIG_BCM96318)
#if defined(CHIP_VAR_MASK)
        unsigned int var = (PERF->RevID & CHIP_VAR_MASK) >> CHIP_VAR_SHIFT;
#endif
    switch ((chipId << 8) | var) {
	case(0x63180b):
		mktname = "6318B";
		break;
	default:
		mktname = NULL;
		break;
    }
#elif  defined (_BCM96838_) || defined(CONFIG_BCM96838)
    switch (chipId) {
    case(1):
        mktname = "68380";
        break;
    case(3):
        mktname = "68380F";
        break;
    case(4):
        mktname = "68385";
        break;
    default:
        mktname = NULL;
    }	
#elif  defined (CONFIG_BCM968500) || defined(_BCM968500_)
		switch (chipId)
		{
		case BL_00000:
			mktname = "BL_00000";
			break;
		case BL_23515:
			mktname = "BL_23515";
			break;
		case BL_23516:
			mktname = "BL_23516";
			break;
		case BL_23530:
			mktname = "BL_23530";
			break;
		case BL_23531:
			mktname = "BL_23531";
			break;
		case BL_23570:
			mktname = "BL_23570";
			break;
		case BL_23571:
			mktname = "BL_23571";
			break;
		case BL_25550:
			mktname = "BL_25550";
			break;
		case BL_25580:
			mktname = "BL_25580";
			break;
		case BL_25590:
			mktname = "BL_25590";
			break;
		case BL_XXXXX:
			mktname = "ChipId Error";
			break;
		}
#endif

#if  defined (CONFIG_BCM968500) || defined(_BCM968500_)
		sprintf(buf,"%s",mktname);	
#else
    if (mktname == NULL) {
	sprintf(buf,"%X%X",chipId,revId);
    } else {
#if  defined (_BCM96838_) || defined(CONFIG_BCM96838)
		sprintf(buf,"%s_", mktname);
		switch(revId)
		{
		case 0:
            strcat(buf,"A0");
			break;
		case 1:
            strcat(buf,"A1");
			break;
		case 4:
            strcat(buf,"B0");
			break;
		case 5:
            strcat(buf,"B1");
			break;
		}
#else
        sprintf(buf,"%s_%X",mktname,revId);
#endif
    }
#endif
    return(buf);
}

int UtilGetChipIsPinCompatible(void) 
{

    int ret = 0;
#if  defined (_BCM96818_) || defined(CONFIG_BCM96818)
    unsigned int chipId = (PERF->RevID & CHIP_ID_MASK) >> CHIP_ID_SHIFT;
    unsigned int var = (BRCM_VARIANT_REG & BRCM_VARIANT_REG_MASK) >> BRCM_VARIANT_REG_SHIFT;
    unsigned int sw;
    sw =  ((chipId << 8) | var);
    switch (sw) {
	case(0x681503): //  "6812GU";
	case(0x681500): //  "6818SR";
	case(0x681501): //  "6818GR";
		ret = 1;
		break;
	default:
		ret = 0;
    }
#endif

    return(ret);
}

#if defined (CONFIG_BCM968500) || defined(_BCM968500_)
void Get968500EmacMode(ETHERNET_MAC_INFO *pE, int port, unsigned long *mac_type, unsigned long *port_flags) {
    *mac_type = pE[0].sw.phy_id[port] & MAC_IFACE;
    *port_flags = pE[0].sw.port_flags[port];
}
#ifndef _CFE_                                                
EXPORT_SYMBOL(Get968500EmacMode);
#endif

int UtilGetLanLedGpio( int n, unsigned short *gpio_num ) {
    unsigned short s;
    ETHERNET_MAC_INFO EnetInfo;
    BpGetEthernetMacInfo( &EnetInfo, 1);
    s = EnetInfo.sw.ledInfo[n].LedLan;
    if (s == BP_GPIO_NONE) {
        return(BP_VALUE_NOT_DEFINED);
    } else {
	*gpio_num = s;
        return(BP_SUCCESS);
    }
}

int UtilGetLan1LedGpio( unsigned short *gpio_num ) {
    return(UtilGetLanLedGpio( 0, gpio_num ));
}
int UtilGetLan2LedGpio( unsigned short *gpio_num ) {
    return(UtilGetLanLedGpio( 1, gpio_num ));
}
int UtilGetLan3LedGpio( unsigned short *gpio_num ) {
    return(UtilGetLanLedGpio( 2, gpio_num ));
}
int UtilGetLan4LedGpio( unsigned short *gpio_num ) {
    return(UtilGetLanLedGpio( 3, gpio_num ));
}
    
#endif

#if defined (CONFIG_BCM96838) || defined(_BCM96838_)
unsigned int gpio_get_dir(unsigned int gpio_num)
{
	if( gpio_num < 32 )
		return (GPIO->GPIODir_low & (1<<gpio_num));
	else if( gpio_num < 64 )
		return (GPIO->GPIODir_mid0 & (1<<(gpio_num-32)));
	else
		return (GPIO->GPIODir_mid1 & (1<<(gpio_num-64)));
}

void gpio_set_dir(unsigned int gpio_num, unsigned int dir)
{
	if(gpio_num<32)
	{ 				
		if(dir)	
			GPIO->GPIODir_low |= GPIO_NUM_TO_MASK(gpio_num);	
		else												
			GPIO->GPIODir_low &= ~GPIO_NUM_TO_MASK(gpio_num);	
	}				
	else if(gpio_num<64) 
	{				
		if(dir)	
			GPIO->GPIODir_mid0 |= GPIO_NUM_TO_MASK(gpio_num-32);	
		else													
			GPIO->GPIODir_mid0 &= ~GPIO_NUM_TO_MASK(gpio_num-32);	
	}				
	else			
	{				
		if(dir)	
			GPIO->GPIODir_mid1 |= GPIO_NUM_TO_MASK(gpio_num-64);	
		else													
			GPIO->GPIODir_mid1 &= ~GPIO_NUM_TO_MASK(gpio_num-64);	
	}
}

unsigned int gpio_get_data(unsigned int gpio_num)
{
	if( gpio_num < 32 )
		return (GPIO->GPIOData_low & (1<<gpio_num));
	else if( gpio_num < 64 )
		return (GPIO->GPIOData_mid0 & (1<<(gpio_num-32)));
	else
		return (GPIO->GPIOData_mid1 & (1<<(gpio_num-64)));
}

void gpio_set_data(unsigned int gpio_num, unsigned int data)
{
	if(gpio_num<32)
	{ 				
		if(data)	
			GPIO->GPIOData_low |= GPIO_NUM_TO_MASK(gpio_num);	
		else												
			GPIO->GPIOData_low &= ~GPIO_NUM_TO_MASK(gpio_num);	
	}				
	else if(gpio_num<64) 
	{				
		if(data)	
			GPIO->GPIOData_mid0 |= GPIO_NUM_TO_MASK(gpio_num-32);	
		else													
			GPIO->GPIOData_mid0 &= ~GPIO_NUM_TO_MASK(gpio_num-32);	
	}				
	else			
	{				
		if(data)	
			GPIO->GPIOData_mid1 |= GPIO_NUM_TO_MASK(gpio_num-64);	
		else													
			GPIO->GPIOData_mid1 &= ~GPIO_NUM_TO_MASK(gpio_num-64);	
	}
}
                                               
void set_pinmux(unsigned int pin_num, unsigned int mux_num)
{
   unsigned int tp_blk_data_lsb;

#ifndef _CFE_ 
   int flags;
   spin_lock_irqsave(&brcm_sharedUtilslock, flags);
#endif

   tp_blk_data_lsb= 0;
   tp_blk_data_lsb |= pin_num;
   tp_blk_data_lsb |= (mux_num << PINMUX_DATA_SHIFT);
   GPIO->port_block_data1 = 0;
   GPIO->port_block_data2 = tp_blk_data_lsb;
   GPIO->port_command = LOAD_MUX_REG_CMD;

#ifndef _CFE_ 
   spin_unlock_irqrestore(&brcm_sharedUtilslock, flags);	
#endif
}

#ifndef _CFE_                                                
EXPORT_SYMBOL(set_pinmux);
EXPORT_SYMBOL(gpio_get_dir);
EXPORT_SYMBOL(gpio_set_dir);
EXPORT_SYMBOL(gpio_get_data);
EXPORT_SYMBOL(gpio_set_data);
#endif
    
#endif

