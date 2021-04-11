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
/* Interrupt controller driver for LILAC SOC                               */
/*                                                                         */
/***************************************************************************/

#include "bl_os_wraper.h"
#include "bl_lilac_ic.h"
#include "bl_lilac_mips_blocks.h"
#include "bl_lilac_rst.h"

#define BL_LILAC_IC_PRIO_FIQ 7

#ifndef BL_LILAC_IC_NO_CHECK
#define BL_LILAC_IC_CHECK_ISR_NUM(num)					\
	do{													\
		if( (num) >= BL_LILAC_IC_INTERRUPT_LAST)		\
			return BL_LILAC_SOC_ERROR;					\
	}while(0)
#else
#define BL_LILAC_IC_CHECK_ISR_NUM(num)	/* */
#endif

#define BL_LILAC_IC_CRACK_NUMBER(id, num)  do{ id = (num) >> 5; num &= 0x1F; }while(0)



#define CLEAR_REGISTER(val)   (*((uint32_t *)(&val)) = 0)

/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_ic_init                                                      */
/*                                                                         */
/*     Initialize the driver. Application should call this function        */
/*     before any other function in the driver                             */
/*                                                                         */
/***************************************************************************/
#ifdef __MIPS_C
BL_LILAC_SOC_STATUS bl_lilac_ic_init(BL_LILAC_CONTROLLER id)
{
	if(id >= RUNNER)
		return BL_LILAC_SOC_INVALID_PARAM;
	
	if(id == MIPSC)
	{
		return bl_lilac_cr_device_reset(BL_LILAC_CR_INT_C, BL_LILAC_CR_ON);
	}

	return bl_lilac_cr_device_reset(BL_LILAC_CR_INT_D, BL_LILAC_CR_ON);

}
EXPORT_SYMBOL(bl_lilac_ic_init);
#endif

/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_ic_init_int                                              */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_init_int(uint32_t xi_interrupt, BL_LILAC_IC_INTERRUPT_PROPERTIES* xi_properties)
{
    MIPS_BLOCKS_VPB_IC_INTCNTL_DTE		interrupt_control;
	uint8_t 							id;

	BL_LILAC_IC_CHECK_ISR_NUM(xi_interrupt);

	if(xi_properties->priority > 6)
		return BL_LILAC_SOC_ERROR;

	BL_LILAC_IC_CRACK_NUMBER(id, xi_interrupt);
    	
	/* build value of the interrupt_control register  */
	
    CLEAR_REGISTER(interrupt_control);
	
	switch(xi_properties->configuration)
	{
        case BL_LILAC_IC_INTERRUPT_LEVEL_LOW:
			interrupt_control.edge = CE_MIPS_BLOCKS_VPB_IC_INTCNTL_EDGE_LOW_VALUE;
			break;
        
        case BL_LILAC_IC_INTERRUPT_LEVEL_HIGH:
	        interrupt_control.edge = CE_MIPS_BLOCKS_VPB_IC_INTCNTL_EDGE_HIGH_VALUE;
	        break; 
        
        case BL_LILAC_IC_INTERRUPT_EDGE_FALLING:
	        interrupt_control.edge = CE_MIPS_BLOCKS_VPB_IC_INTCNTL_EDGE_NEGEDGE_VALUE;
	        break;
        
        case BL_LILAC_IC_INTERRUPT_EDGE_RISING: 
	        interrupt_control.edge = CE_MIPS_BLOCKS_VPB_IC_INTCNTL_EDGE_POSEDGE_VALUE;
	        break;
        
        default:  /* bad value */
	        return BL_LILAC_SOC_ERROR;
 	}

    interrupt_control.r1 = CE_MIPS_BLOCKS_VPB_IC_INTCNTL_R1_DEFAULT_VALUE;
    interrupt_control.r2 = CE_MIPS_BLOCKS_VPB_IC_INTCNTL_R2_DEFAULT_VALUE;
    	
	if(xi_properties->reentrant != 0)
	{
        interrupt_control.reent = CE_MIPS_BLOCKS_VPB_IC_INTCNTL_REENT_RE_ENTRY_VALUE;
	}
    else
    {
        interrupt_control.reent = CE_MIPS_BLOCKS_VPB_IC_INTCNTL_REENT_NO_RE_ENTRY_VALUE;
    }

	if(xi_properties->fast_interrupt == 0)
    {
		
		interrupt_control.prio = xi_properties->priority;
    }
    else
    {
    	interrupt_control.prio = BL_LILAC_IC_PRIO_FIQ;
    }
    
    /* finally write interrupt control regsiter */
	if(id == 0)
	{
        BL_MIPS_BLOCKS_VPB_IC_0_INTCNTL_WRITE_I(interrupt_control, xi_interrupt);
	}
	else if(id == 1)
	{
		BL_MIPS_BLOCKS_VPB_IC_1_INTCNTL_WRITE_I(interrupt_control, xi_interrupt);
	}
#ifdef __MIPS_D
	else if(id == 2)
	{
		BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTCNTL_WRITE_I(interrupt_control, xi_interrupt);
	}
#endif

    /* return Ok */
    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_ic_init_int);


/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_ic_unmask_irq                                                */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_unmask_irq(uint32_t xi_interrupt) 
{
    MIPS_BLOCKS_VPB_IC_INTMASK_DTE	int_mask;
	uint8_t 						id;
   
	BL_LILAC_IC_CHECK_ISR_NUM(xi_interrupt);

	BL_LILAC_IC_CRACK_NUMBER(id, xi_interrupt);

	if(id == 0)
	{
        /* read mask register  */
        BL_MIPS_BLOCKS_VPB_IC_0_INTMASK_READ(int_mask);
        /* clear bit */
        int_mask.msk &= ~(1 << xi_interrupt);
        /*, write mask */
        BL_MIPS_BLOCKS_VPB_IC_0_INTMASK_WRITE(int_mask);
	}
	else if(id == 1)
	{
        /* read mask register  */
        BL_MIPS_BLOCKS_VPB_IC_1_INTMASK_READ(int_mask);
        /* clear bit */
        int_mask.msk &= ~(1 << xi_interrupt);
        /*, write mask */
        BL_MIPS_BLOCKS_VPB_IC_1_INTMASK_WRITE(int_mask);
	}
#ifdef __MIPS_D
	else if(id == 2)
	{
        /* read mask register  */
		BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTMASK_READ(int_mask);
        /* clear bit */
        int_mask.msk &= ~(1 << xi_interrupt);
        /*, write mask */
		BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTMASK_WRITE(int_mask);
	}
#endif

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_ic_unmask_irq);


/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_ic_enable_mask                                          	   */
/*                                                                         */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_enable_mask(uint32_t id, uint32_t xi_mask)
{
    MIPS_BLOCKS_VPB_IC_INTMASK_DTE int_mask;

	if(id == 0)
	{
	    /* read mask */
	    BL_MIPS_BLOCKS_VPB_IC_0_INTMASK_READ(int_mask);
	    /* clear bits */
	    int_mask.msk &= ~(xi_mask);
	    /* write mask */
	    BL_MIPS_BLOCKS_VPB_IC_0_INTMASK_WRITE(int_mask);
	}
	else if(id == 1)
	{
	    /* read mask */
	    BL_MIPS_BLOCKS_VPB_IC_1_INTMASK_READ(int_mask);
	    /* clear bits */
	    int_mask.msk &= ~(xi_mask);
	    /* write mask */
	    BL_MIPS_BLOCKS_VPB_IC_1_INTMASK_WRITE(int_mask);
	}
#ifdef __MIPS_D
	else if(id == 2)
	{
	    /* read mask */
	    BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTMASK_READ(int_mask);
	    /* clear bits */
	    int_mask.msk &= ~(xi_mask);
	    /* write mask */
	    BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTMASK_WRITE(int_mask);
	}
#endif
	else
	{
		return BL_LILAC_SOC_INVALID_PARAM;
	}
	
    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_ic_enable_mask);

/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_ic_mask_irq                                                   */
/*                                                                         */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_mask_irq(uint32_t xi_interrupt)
{
    MIPS_BLOCKS_VPB_IC_INTMASK_DTE	int_mask;
	uint8_t 						id;

	BL_LILAC_IC_CHECK_ISR_NUM(xi_interrupt);

	BL_LILAC_IC_CRACK_NUMBER(id, xi_interrupt);

	if(id == 0)
	{
	    /* read mask register  */
	    BL_MIPS_BLOCKS_VPB_IC_0_INTMASK_READ(int_mask);
	    /* set bit */
	    int_mask.msk |= (1 << xi_interrupt);
	    /* write mask */
	    BL_MIPS_BLOCKS_VPB_IC_0_INTMASK_WRITE(int_mask);
	}
	else if(id == 1)
	{
	    /* read mask register  */
	    BL_MIPS_BLOCKS_VPB_IC_1_INTMASK_READ(int_mask);
	    /* set bit */
	    int_mask.msk |= (1 << xi_interrupt);
	    /* write mask */
	    BL_MIPS_BLOCKS_VPB_IC_1_INTMASK_WRITE(int_mask);
	}
#ifdef __MIPS_D
	else if(id == 2)
	{
	    /* read mask */
	    BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTMASK_READ(int_mask);
	    /* set bit */
	    int_mask.msk |= (1 << xi_interrupt);
	    /* write mask */
	    BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTMASK_WRITE(int_mask);
	}
#endif

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_ic_mask_irq);


/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_ic_disable_mask                                              */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_disable_mask(uint32_t id, uint32_t xi_mask) 
{
    MIPS_BLOCKS_VPB_IC_INTMASK_DTE int_mask;

	if(id == 0)
	{
	    /* read mask */
	    BL_MIPS_BLOCKS_VPB_IC_0_INTMASK_READ(int_mask);
	    /* set bits */
	    int_mask.msk |= xi_mask;
	    /* write mask */
	    BL_MIPS_BLOCKS_VPB_IC_0_INTMASK_WRITE(int_mask);
	}
	else if(id == 1)
	{
	    /* read mask */
	    BL_MIPS_BLOCKS_VPB_IC_1_INTMASK_READ(int_mask);
	    /* set bits */
	    int_mask.msk |= xi_mask;
	    /* write mask */
	    BL_MIPS_BLOCKS_VPB_IC_1_INTMASK_WRITE(int_mask);
	}
#ifdef __MIPS_D
	else if(id == 2)
	{
	    /* read mask */
	    BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTMASK_READ(int_mask);
	    /* set bits */
	    int_mask.msk |= xi_mask;
	    /* write mask */
	    BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTMASK_WRITE(int_mask);
	}
#endif
	else
	{
		return BL_LILAC_SOC_INVALID_PARAM;
	}
		
    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_ic_disable_mask);



/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_ic_isr_ack                                                   */
/***************************************************************************/
BL_LILAC_SOC_STATUS bl_lilac_ic_isr_ack(uint32_t xi_interrupt)
{
    MIPS_BLOCKS_VPB_IC_INTSTAT_INTACK_DTE	reg_ack;
    MIPS_BLOCKS_VPB_IC_INTEOI_DTE			reg_eoi;
	uint32_t								id;

	BL_LILAC_IC_CHECK_ISR_NUM(xi_interrupt);

	BL_LILAC_IC_CRACK_NUMBER(id, xi_interrupt);
	
    reg_eoi.r2	= CE_MIPS_BLOCKS_VPB_IC_INTEOI_R2_DEFAULT_VALUE;
    reg_eoi.r1	= CE_MIPS_BLOCKS_VPB_IC_INTEOI_R2_DEFAULT_VALUE;
    reg_eoi.eoi	= 1;

	reg_ack.state = (1 << xi_interrupt);
	
	if(id == 0)
	{
		/* write one to the ACK bit (Interrupt Acknowledge Register) */
	    /* required for edge interrupts */
	    BL_MIPS_BLOCKS_VPB_IC_0_INTSTAT_INTACK_WRITE(reg_ack);

	    /* write EOI bit (0x80) to ack the interrupt */
	    BL_MIPS_BLOCKS_VPB_IC_0_INTEOI_WRITE(reg_eoi);
	}
	else if(id == 1)
	{
		/* write one to the ACK bit (Interrupt Acknowledge Register) */
	    /* required for edge interrupts */
	    BL_MIPS_BLOCKS_VPB_IC_1_INTSTAT_INTACK_WRITE(reg_ack);

	    /* write EOI bit (0x80) to ack the interrupt */
	    BL_MIPS_BLOCKS_VPB_IC_1_INTEOI_WRITE(reg_eoi);
	}
#ifdef __MIPS_D
	else if(id == 2)
	{
		/* write one to the ACK bit (Interrupt Acknowledge Register) */
	    /* required for edge interrupts */
	    BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTSTAT_INTACK_WRITE(reg_ack);

	    /* write EOI bit (0x80) to ack the interrupt */
	    BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTEOI_WRITE(reg_eoi);
	}
#endif

    return BL_LILAC_SOC_OK;
}
EXPORT_SYMBOL(bl_lilac_ic_isr_ack);

/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_ic_interrupt_number                                     */
/***************************************************************************/
int32_t bl_lilac_ic_interrupt_number(uint32_t id) 
{
    uint32_t val = 0;

	if(id == 0)
    	BL_MIPS_BLOCKS_VPB_IC_0_INTHPAI_READ(val);
	else if(id == 1)
    	BL_MIPS_BLOCKS_VPB_IC_1_INTHPAI_READ(val);
#ifdef __MIPS_D
	else if(id == 2)
		BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTHPAI_READ(val);
#endif

    return val >> 2;
}
EXPORT_SYMBOL(bl_lilac_ic_interrupt_number);


/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_ic_set_ghost                                                 */
/*                                                                         */
/***************************************************************************/
void bl_lilac_ic_set_ghost(uint32_t xi_interrupt)
{
    MIPS_BLOCKS_VPB_IC_INTGHOST_DTE val;

    val.ghost = xi_interrupt;

    BL_MIPS_BLOCKS_VPB_IC_0_INTGHOST_WRITE(val);
    BL_MIPS_BLOCKS_VPB_IC_1_INTGHOST_WRITE(val);
#ifdef __MIPS_D
    BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTGHOST_WRITE(val);
#endif
}
EXPORT_SYMBOL(bl_lilac_ic_set_ghost);

#ifdef BL_LILAC_IC_DEBUG

/***************************************************************************/
/* Name                                                                    */
/*   bl_lilac_ic_status                                                    */
/*                                                                         */
/***************************************************************************/
BL_LILAC_IC_INTERRUPT_SATUS bl_lilac_ic_isr_status(int32_t xi_interrupt)
{
    MIPS_BLOCKS_VPB_IC_INTSTAT_INTACK_DTE	reg_status = {0};
    MIPS_BLOCKS_VPB_IC_INTIS_DTE			reg_inservice = {0};
   	int32_t 								pending;
   	uint32_t 								inservice;
	uint8_t 								id;

	BL_LILAC_IC_CHECK_ISR_NUM(xi_interrupt);

	BL_LILAC_IC_CRACK_NUMBER(id, xi_interrupt);
	
	if(id == 0)
	{
		BL_MIPS_BLOCKS_VPB_IC_0_INTSTAT_INTACK_READ(reg_status);
		BL_MIPS_BLOCKS_VPB_IC_0_INTIS_READ(reg_inservice);
	}
	else if(id == 1)
	{
		BL_MIPS_BLOCKS_VPB_IC_1_INTSTAT_INTACK_READ(reg_status);
		BL_MIPS_BLOCKS_VPB_IC_1_INTIS_READ(reg_inservice);
	}
#ifdef __MIPS_D
	else if(id == 2)
	{
		BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTSTAT_INTACK_READ(reg_status);
		BL_MIPS_BLOCKS_MIPSD_IH_REGS_INTIS_READ(reg_inservice);
	}
#endif

    pending		= (reg_status.state & (1 << xi_interrupt)) != 0;
    inservice	= (reg_inservice.is & (1 << xi_interrupt)) != 0;

    if(inservice)
    	return BL_LILAC_IC_INTERRUPT_STATUS_IN_SERVICE;
	
    if(pending)
    	return BL_LILAC_IC_INTERRUPT_STATUS_PENDING;
	
	return BL_LILAC_IC_INTERRUPT_STATUS_IDLE;

}
EXPORT_SYMBOL(bl_lilac_ic_isr_status);

#endif /*BL_LILAC_IC_DEBUG*/

