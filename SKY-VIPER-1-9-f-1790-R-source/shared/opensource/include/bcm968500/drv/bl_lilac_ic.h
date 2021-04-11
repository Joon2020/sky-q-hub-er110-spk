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


/***************************************************************************/
/*                                                                         */
/* Interrupt controller Driver                                             */
/*                                                                         */
/* Abstract                                                                */
/*                                                                         */
/*   The driver provides API to access Lilac SOC Interrupt                 */
/*   Controller                                                            */
/*   Supports basic functionality like enable/disable, set priority        */
/*   each interrupt programmable to be                                     */
/*   active on high, low, rising edge, falling edge                        */
/*   Priorities can be dynamically assigned 0-6. Contrioller provides      */
/*   index of the interrupt with highest priority                          */
/*   Each interrupt can be redirected to IRQ or FIQ (FIQ is connectd to    */
/*   the non-maskable interrupt pin on MIPS)                               */
/*   Frequently called functions are implemeted as macros, for example     */
/*   enable/disable interrupt and acknowledge interrupt                    */
/*                                                                         */
/***************************************************************************/

#ifndef BL_LILAC_IC_H_
#define BL_LILAC_IC_H_

#include "bl_lilac_soc.h"

#ifdef __cplusplus
extern "C"
{
#endif


#define BL_LILACIC_INTERRUPT_GHOST 32

#if defined LINUX_KERNEL
#define INT_NUM_IRQ0 (MIPS_CPU_IRQ_BASE + 0x10)
#endif

/* connected devices (pin numbers) */
typedef enum
{
#if defined  __MIPS_C 

	/* IC0 */
    BL_LILAC_IC_INTERRUPT_TIMER_0		= 0,
    BL_LILAC_IC_INTERRUPT_TIMER_1		= 1,
    BL_LILAC_IC_INTERRUPT_TIMER_2		= 2,
    BL_LILAC_IC_INTERRUPT_TIMER_3		= 3,
    BL_LILAC_IC_INTERRUPT_UART			= 4,
    BL_LILAC_IC_INTERRUPT_I2C			= 5,
    BL_LILAC_IC_INTERRUPT_SPI			= 6,
    BL_LILAC_IC_INTERRUPT_RUNNER		= 7,  /* 7 - 16 runner interrupts */ 
    BL_LILAC_IC_INTERRUPT_RUNNER_LAST	= 16, /* 7 - 16 runner interrupts */ 
    BL_LILAC_IC_INTERRUPT_OBIU			= 17,  
    BL_LILAC_IC_INTERRUPT_PCI_0			= 18, /* 18-19 pci interrupts*/ 
    BL_LILAC_IC_INTERRUPT_PCI_1			= 19, /* 18-19 pci interrupts*/ 
    BL_LILAC_IC_INTERRUPT_DSP			= 20, 
    BL_LILAC_IC_INTERRUPT_EMAC			= 21, 
    BL_LILAC_IC_INTERRUPT_BPM			= 22, 
	BL_LILAC_IC_INTERRUPT_SBPM			= 23, 
    BL_LILAC_IC_INTERRUPT_GPONTX		= 24, 
    BL_LILAC_IC_INTERRUPT_GPONRX		= 25,
    BL_LILAC_IC_INTERRUPT_CLK_1588		= 26,
    BL_LILAC_IC_INTERRUPT_NAND_ECC		= 27,
	BL_LILAC_IC_INTERRUPT_USB_0			= 28, /* 28-29 usb interrupts*/ 
	BL_LILAC_IC_INTERRUPT_USB_1			= 29, /* 28-29 usb interrupts*/ 
	BL_LILAC_IC_INTERRUPT_SPI_FLASH		= 30, 
	BL_LILAC_IC_INTERRUPT_MIPS_EARLY_WD	= 31, 
	/* IC1*/
	BL_LILAC_IC_INTERRUPT_DOORBELL		= 32, /* 0 - 15 MIPS to MIPS interrupts */
	BL_LILAC_IC_INTERRUPT_DOORBELL_LAST	= 47, /* 0 - 15 MIPS to MIPS interrupts */
	BL_LILAC_IC_INTERRUPT_EXT			= 48, /* 16 - 25 external interrupts */
	BL_LILAC_IC_INTERRUPT_EXT_LAST		= 57, /* 16 - 25 external interrupts */
	BL_LILAC_IC_INTERRUPT_GPON_RX_8		= 58, 
	BL_LILAC_IC_INTERRUPT_DDR_MSI		= 59, /* 27-30  */
	BL_LILAC_IC_INTERRUPT_DDR_MSI_LAST	= 62, /* 27-30  */
	BL_LILAC_IC_INTERRUPT_NAND_READY_BUSY = 63, 
	BL_LILAC_IC_INTERRUPT_LAST
	
#elif	defined __MIPS_D
	/* IC0 */
	/* IC0 */
    BL_LILAC_IC_INTERRUPT_TIMER_0		= 0,
    BL_LILAC_IC_INTERRUPT_TIMER_1		= 1,
    BL_LILAC_IC_INTERRUPT_TIMER_2		= 2,
    BL_LILAC_IC_INTERRUPT_TIMER_3		= 3,
    BL_LILAC_IC_INTERRUPT_UART			= 4,
    BL_LILAC_IC_INTERRUPT_PSE			= 5,
    BL_LILAC_IC_INTERRUPT_RUNNER		= 7,  /* 7 - 16 runner interrupts */ 
    BL_LILAC_IC_INTERRUPT_RUNNER_LAST	= 16, /* 7 - 16 runner interrupts */ 
    BL_LILAC_IC_INTERRUPT_OBIU			= 17,  
    BL_LILAC_IC_INTERRUPT_PCI_0			= 18, /* 18-19 pci interrupts*/ 
    BL_LILAC_IC_INTERRUPT_PCI_1			= 19, /* 18-19 pci interrupts*/ 
	BL_LILAC_IC_INTERRUPT_USB_0			= 28, /* 28-29 usb interrupts*/ 
	BL_LILAC_IC_INTERRUPT_USB_1			= 29, /* 28-29 usb interrupts*/ 
	BL_LILAC_IC_INTERRUPT_SPCTC1		= 30, 
	BL_LILAC_IC_INTERRUPT_MIPS_EARLY_WD	= 31, 
	/* IC1*/
	BL_LILAC_IC_INTERRUPT_DOORBELL		= 32, /* 0 - 15 MIPS to MIPS interrupts */
	BL_LILAC_IC_INTERRUPT_DOORBELL_LAST	= 47, /* 0 - 15 MIPS to MIPS interrupts */
	BL_LILAC_IC_INTERRUPT_EXT			= 48, /* 16 - 25 external interrupts */
	BL_LILAC_IC_INTERRUPT_EXT_LAST		= 57, /* 16 - 25 external interrupts */
	BL_LILAC_IC_INTERRUPT_GPON_RX_8		= 58, 
	BL_LILAC_IC_INTERRUPT_DDR_MSI		= 59, /* 27-30  */
	BL_LILAC_IC_INTERRUPT_DDR_MSI_LAST	= 62, /* 27-30  */
	/* IC2*/
	BL_LILAC_IC_INTERRUPT_RUNNER_BB	 	= 64, /* 0 - 31  */
	BL_LILAC_IC_INTERRUPT_RUNNER_BB_LAST = 95,/* 0 - 31  */
	BL_LILAC_IC_INTERRUPT_LAST
	
#else
#error "NOT DEFINED MIPS ID"
#endif

} BL_LILAC_IC_INTERRUPT_PINS;

/*types of the interrupts Level high/low and falling/rising edge */
typedef enum
{
    BL_LILAC_IC_INTERRUPT_LEVEL_LOW,    /* level interrupt, active low  */
    BL_LILAC_IC_INTERRUPT_LEVEL_HIGH,   /* level interrupt, active high */
    BL_LILAC_IC_INTERRUPT_EDGE_FALLING, /* falling edge triggered       */
    BL_LILAC_IC_INTERRUPT_EDGE_RISING   /* rising edge triggered        */
    
}BL_LILAC_IC_INTERRUPT_CONFIG ;

/* states of the interrupt - no interrupt, pending, in service */
typedef enum
{
    BL_LILAC_IC_INTERRUPT_STATUS_IDLE,
    BL_LILAC_IC_INTERRUPT_STATUS_PENDING,
    BL_LILAC_IC_INTERRUPT_STATUS_IN_SERVICE
    
}BL_LILAC_IC_INTERRUPT_SATUS ;

/* the structure describes interrupt*/
typedef struct
{
    /* priority of the interrupt 0 - 6 */
    uint32_t						priority;
    
    /* for example, level intterupt, active low */
    BL_LILAC_IC_INTERRUPT_CONFIG	configuration;
    
    /* set to non-zero to qualify interrupt as reentrant.     */
    /* if an interrupt is re-entrant, then his priority       */
    /* has to be different from the other interrupts priority.*/
    uint32_t						reentrant;
    
    /* set to non-zero to connect to FIQ output (non-maskable interrupt in MIPS) */
    uint32_t						fast_interrupt;
	
}BL_LILAC_IC_INTERRUPT_PROPERTIES ;



/***************************************************************************/
/*     Initialize the driver. Application should call this function        */
/*     before any other function in the driver                             */
/***************************************************************************/
#ifdef __MIPS_C
BL_LILAC_SOC_STATUS bl_lilac_ic_init(BL_LILAC_CONTROLLER id);
#endif



/***************************************************************************/
/*     Initialize interrupt. this functios adds the interrupt parameters   */
/*     to the list of the initialized interrupts                           */
/*     Application can call this function more than once if interrupt      */
/*     remains disabled.                                                   */
/* Input                                                                   */
/*   interrupt number                                                      */
/*   interrupt properties (level/edge, reentrant, etc. )                   */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_init_int(uint32_t xi_interrupt, BL_LILAC_IC_INTERRUPT_PROPERTIES* xi_properties);



/***************************************************************************/
/*     Enable interrupt. Application will call init first                  */
/*     This function is not reentrant                                      */
/* Input                                                                   */
/*   interrupt number                                                      */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_unmask_irq(uint32_t xi_interrupt);


/***************************************************************************/
/*     Enable interrupt. Application will call init first                  */
/*     This function is not reentrant. Driver sets all bits set in the     */
/*     specified mask                                                      */
/*     For example do enable all use 0xFFFFFFFF                            */
/* Input                                                                   */
/* index of IC (0 - 2)							                           */
/*   interrupt mask 32 bits                                                */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_enable_mask(uint32_t id, uint32_t xi_mask);



/***************************************************************************/
/*     Disable interrupt. Application will call init first                 */
/* Input                                                                   */
/*   interrupt number                                                      */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_mask_irq(uint32_t xi_interrupt);


/***************************************************************************/
/*     Dsiable interrupts. Application will call init first                */
/*     This function is not reentrant. Driver sets all bits set in the     */
/*     specified mask                                                      */
/*     For example do disable all use 0xFFFFFFFF                           */
/* Input                                                                   */
/* index of IC (0 - 2)							                           */
/*   interrupt mask 32 bits                                                */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_disable_mask(uint32_t id, uint32_t xi_mask);




/***************************************************************************/
/*   Call this function to ack the interrupt                               */
/* Input                                                                   */
/*   interrupt number                                                      */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_isr_ack(uint32_t xi_interrupt);



/***************************************************************************/
/*   returns number of the highest priority currently active interrupt     */
/*   actually it returns 4 times interrupt number or OFFSET of the         */
/*   interrupt in the interrupt table assuming 4 bytes per entry           */
/* Input                                                                   */
/* index of IC (0 - 2)													   */
/* Output                                                                  */
/*   index of the active interrupt                                         */
/***************************************************************************/
int32_t bl_lilac_ic_interrupt_number(uint32_t id);


/***************************************************************************/
/*   set ghost interrupt number                                            */
/*   ghost interrupt is the one bl_lilac_drv_ic_interrupt_number        */
/*   will return if no interrupt is being served                           */
/* Input                                                                   */
/*   index of the ghost interrupt                                          */
/***************************************************************************/
void bl_lilac_ic_set_ghost(uint32_t xi_interrupt);


#ifdef BL_LILAC_IC_DEBUG

/***************************************************************************/
/*   Call this function to get status of the interrupt                     */
/*   Can be used for debug or when simulating interrupts in polling miode  */
/*   You have to enable interrupt to make the function to work correctly   */
/* Input                                                                   */
/*   interrupt number                                                      */
/* Output                                                                  */
/*   BL_LILAC_IC_INTERRUPT_SATUS if Ok                                     */
/***************************************************************************/
BL_LILAC_IC_INTERRUPT_SATUS bl_lilac_ic_isr_status(uint32_t xi_interrupt);

#endif


#ifdef __cplusplus
}
#endif

#endif /*BL_LILAC_DRV_IC_H_ */
