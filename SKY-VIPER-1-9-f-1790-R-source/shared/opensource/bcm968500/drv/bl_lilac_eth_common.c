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

/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/

#include "bl_os_wraper.h"
#include "bl_lilac_eth_common.h"
#include "bl_lilac_gpio.h"
#include "bl_lilac_cr.h"
#include "bl_lilac_vpb_bridge.h"
#include "bl_lilac_pin_muxing.h"
#include "bl_lilac_net_if.h"
#include "phy_definitions.h"
#include "boardparms.h"

/******************************************************************************/
/*                                                                            */
/* Types and values definitions                                               */
/*                                                                            */
/******************************************************************************/
#define CS_EMAC_PREAMBLE_LENGTH					7
#define CS_EMAC_BACK2BACK_GAP					0x60
#define CS_EMAC_NON_BACK2BACK_GAP				0x60
#define CS_EMAC_MIN_INTER_FRAME_GAP 			0x80
#define CS_MAX_ETH_PACKET_SIZE					1536

const char* MAC_PHY_RATE_NAME[DRV_MAC_PHY_RATE_LAST] =
{
	"10M Full",
	"10M Half",
	"100M Full",
	"100M Half",
	"1000M Full",
	"1000M Half",
	"Down",
	"Error",
};
EXPORT_SYMBOL(MAC_PHY_RATE_NAME);

const char* EMAC_MODE_NAME[ETH_MODE_LAST] = 
{
	"SGMII",      
	"HISGMII",    
    "QSGMII" ,    
	"SS-SMII",       
	"RGMII" ,		
	"TMII" 	,	
	"NONE"
};
EXPORT_SYMBOL(EMAC_MODE_NAME);

extern uint8_t tbi_phy_addr []; 

#define LILAC_SGMII_BMCR_VALUE         ( 0x1040 )
#define LILAC_QSGMII_BMCR_VALUE        ( 0x0040 )

//#define LILAC_BMCR_VALUE        0x0040
#define LILAC_ADVERTISE_VALUE   0x7fff

/******************************************************************************/
/*                                                                            */
/* Global variables definitions                                               */
/*                                                                            */
/******************************************************************************/
void config_tbi(DRV_MAC_NUMBER xi_emac_id, ETH_MODE xi_emacs_group_mode)
{
    NET_IF_CORE_GENERAL_TBI_CFG_DTE reg;
	uint32_t bmcr; 
    uint32_t phyTBI;  

    phyTBI = tbi_phy_addr[xi_emac_id];
    if ((xi_emacs_group_mode == ETH_MODE_SGMII) || (xi_emacs_group_mode == ETH_MODE_SMII))
        bmcr = LILAC_SGMII_BMCR_VALUE;
    else
        bmcr = LILAC_QSGMII_BMCR_VALUE;
    reg.phyaddr = phyTBI;
    reg.disp_en = 1;
    reg.lnk_tmr_val = 6;
    BL_NET_IF_CORE_GENERAL_TBI_CFG_WRITE(xi_emac_id, reg);
	
    if (xi_emac_id == DRV_MAC_EMAC_4)
        return;

    fi_bl_drv_mac_mgmt_write(0, phyTBI, MII_ADVERTISE, LILAC_ADVERTISE_VALUE);
    fi_bl_drv_mac_mgmt_write(0, phyTBI, MII_BMCR, bmcr);
	
    mdelay(400);
}
EXPORT_SYMBOL(config_tbi);

BL_LILAC_CR_DEVICE bl_lilac_map_emac_id_to_cr_device(DRV_MAC_NUMBER xi_emac_id)
{
    switch ( xi_emac_id )
    {
	    case DRV_MAC_EMAC_0:
	        return BL_LILAC_CR_NIF_0;
	    case DRV_MAC_EMAC_1:
	        return BL_LILAC_CR_NIF_1;
	    case DRV_MAC_EMAC_2:
	        return BL_LILAC_CR_NIF_2;
	    case DRV_MAC_EMAC_3:
	        return BL_LILAC_CR_NIF_3;
	    case DRV_MAC_EMAC_4:
	        return BL_LILAC_CR_NIF_4;
	    case DRV_MAC_EMAC_5:
	        return BL_LILAC_CR_NIF_5;
	    default:
	        return -1;
    }
}
EXPORT_SYMBOL(bl_lilac_map_emac_id_to_cr_device);

static LILAC_BDPI_ERRORS f_reset_emac(DRV_MAC_NUMBER emac_id, BL_LILAC_CR_STATE xi_cr_state)
{
    BL_LILAC_CR_DEVICE cr_device ;

    cr_device = bl_lilac_map_emac_id_to_cr_device ( emac_id);
    bl_lilac_cr_device_reset ( cr_device , xi_cr_state);

	/* in case of un-reset, we also clear the sw_reset bit in the EMAC core */
	if(xi_cr_state == BL_LILAC_CR_ON)
		pi_bl_drv_mac_sw_reset(emac_id , DRV_MAC_DISABLE) ;

	return BDPI_NO_ERROR;
}

/* enables/disables clock of all EMACs */
static void f_enable_clock_emac ( DRV_MAC_NUMBER emac_id, BL_LILAC_CR_STATE xi_cr_state )
{
    BL_LILAC_CR_DEVICE cr_device ;

    if (emac_id != DRV_MAC_EMAC_0)
    {
		cr_device = bl_lilac_map_emac_id_to_cr_device (DRV_MAC_EMAC_0);
		bl_lilac_cr_device_enable_clk ( cr_device , xi_cr_state);
    }
	cr_device = bl_lilac_map_emac_id_to_cr_device (emac_id);
	bl_lilac_cr_device_enable_clk ( cr_device , xi_cr_state);
}

/*
perform alignment of the signals of RGMII
gives the point X (tap) where the out is 1
gives the point Y (tap) where the out is 0
returns N = Y - X + 1 to configure TX and RX DLLs
*/
static LILAC_BDPI_ERRORS f_rgmii_alignment(uint32_t * const xo_x, uint32_t * const xo_y, uint32_t * const xo_result)
{
    const uint32_t last_tap = 114 ;
    uint32_t index;
    volatile CR_CONTROL_REGS_CR_RGMII_MASTER_DLL_DTE master_in;
    volatile CR_CONTROL_REGS_CR_RGMII_MASTER_DLL_OUT_DTE master_out;
    volatile CR_CONTROL_REGS_RGMII_DLL_CTRL_DTE control;

    *xo_x = 0;
    *xo_y = 0;

    BL_CR_CONTROL_REGS_RGMII_DLL_CTRL_READ(control);
    control.master_dll_enable = 1;
    control.tx_dll_enable = 1;
    control.rx_dll_enable = 1;
    BL_CR_CONTROL_REGS_RGMII_DLL_CTRL_WRITE(control);
    master_in.reserved = 0;

    for(index = 0; index <= last_tap; index ++)
    {
        master_in.cr_rgmii_master_dll_sel = index;
        BL_CR_CONTROL_REGS_CR_RGMII_MASTER_DLL_WRITE(master_in);
        BL_CR_CONTROL_REGS_CR_RGMII_MASTER_DLL_OUT_READ(master_out);

        if(master_out.cr_rgmii_master_dll_out)
        {
            *xo_x = index;
            break;
        }
    }

    for(index = index + 1 ; index <= last_tap; index ++)
    {
        master_in.cr_rgmii_master_dll_sel = index;
        BL_CR_CONTROL_REGS_CR_RGMII_MASTER_DLL_WRITE(master_in);
        BL_CR_CONTROL_REGS_CR_RGMII_MASTER_DLL_OUT_READ(master_out);

        if(!master_out.cr_rgmii_master_dll_out)
        {
            *xo_y = index;
            break;
        }
    }

    if(index > last_tap)
    {
        return ( BDPI_ERROR_EMAC_IS_NOT_FUNCTIONAL );
    }


    if (*xo_y - *xo_x + 1 < 2)
    {
        if (*xo_y > *xo_x)
        {
            *xo_result = *xo_y;
        }
        else
        {
            *xo_result = *xo_x;
        }
    }
    else
    {
        *xo_result = ( *xo_x + 1 >= *xo_y - *xo_x + 1 ? *xo_x + 1 : *xo_y - *xo_x + 1 );
    }


    return ( BDPI_NO_ERROR );
}
EXPORT_SYMBOL(f_rgmii_alignment);

static void p_set_rgmii_dll(uint32_t xi_n, uint32_t master)
{
    volatile CR_CONTROL_REGS_CR_RGMII_RX_DLL_DTE rx;
    volatile CR_CONTROL_REGS_RGMII_DLL_CTRL_DTE control;

    rx.cr_rgmii_rx_dll_sel = xi_n; // works with 7
    BL_CR_CONTROL_REGS_CR_RGMII_RX_DLL_WRITE(rx);
    BL_CR_CONTROL_REGS_CR_RGMII_TX_DLL_WRITE(rx);
    BL_CR_CONTROL_REGS_RGMII_DLL_CTRL_READ(control);
    control.tx_dll_enable = 1;
    control.rx_dll_enable = 1;
    control.master_dll_enable = master;
    BL_CR_CONTROL_REGS_RGMII_DLL_CTRL_WRITE(control);
}

LILAC_BDPI_ERRORS f_align_rgmii ( void )
{
    uint32_t x , y , result ;
    LILAC_BDPI_ERRORS error ;
    static uint32_t last_run ;
    static uint32_t first_run = 1 ;

	error = f_rgmii_alignment ( & x , & y , & result ) ;
	 if ( error != BDPI_NO_ERROR )
	 {
	     return ( error ) ;
	 }

	 result /= 2 ;

	  if ( first_run != 1 )
	  {
		  if (result < last_run)
		  {
			  result = last_run - 1;
		  } 
		  else if (result > last_run)
		  {
			  result = last_run + 1;
		  }
	  }

	  /* result/2 should be passed to p_set_rgmii_dll */
	  p_set_rgmii_dll ( result , 1 ) ;

	  last_run = result ;
	  first_run = 0 ;

    return ( BDPI_NO_ERROR);
}
EXPORT_SYMBOL(f_align_rgmii);

static LILAC_BDPI_ERRORS config_mdc_mdio(DRV_MAC_NUMBER xi_emac_id, ETH_MODE xi_emacs_group_mode, ETH_MODE xi_emac4_mode)
{
    unsigned int mgmt=0;
    LILAC_BDPI_ERRORS error ;

	fi_bl_lilac_mux_mdc_mdio();
   
	((NET_IF_CORE_MENTOR_MII_MGMT_DTE *)&mgmt)->prmbl_supp	= 1;
	((NET_IF_CORE_MENTOR_MII_MGMT_DTE *)&mgmt)->mgmt_clk_sel= 11;
	BL_NET_IF_CORE_MENTOR_MII_MGMT_WRITE(xi_emac_id,mgmt);

	if(xi_emacs_group_mode == ETH_MODE_QSGMII)
	{
		bl_lilac_init_qsgmii_phy();
	}

	if((xi_emacs_group_mode == ETH_MODE_SGMII)||
       (xi_emacs_group_mode == ETH_MODE_QSGMII)||
       (xi_emacs_group_mode == ETH_MODE_SMII)||
       (xi_emacs_group_mode == ETH_MODE_NONE))
	{
		config_tbi(xi_emac_id, xi_emacs_group_mode);
	}
   
	if(xi_emac_id == DRV_MAC_EMAC_4)
	{
		/* RGMII/MII pin muxing configuration (according to EMAC 4 mode) */
		if(xi_emac4_mode  == E4_MODE_RGMII)
		{
			fi_bl_lilac_mux_rgmii();
			error = f_align_rgmii();

			if(error != BDPI_NO_ERROR)
			{
				return error;
			}
			
			bl_lilac_plat_rgmii_phy_init(BpGetPhyAddr(0,xi_emac_id));

			return BDPI_NO_ERROR;
		}
		/* mode is MII */
		else
		{
            p_set_rgmii_dll (0xd, 1);
			return fi_bl_lilac_mux_mii();
		}
	}

	return BDPI_NO_ERROR;
}

static void bl_modify_emac_cfg ( DRV_MAC_NUMBER xi_emac_id )
{
    DRV_MAC_CONF mac_conf;

    /* Call emac driver */
    pi_bl_drv_mac_get_configuration ( xi_emac_id , & mac_conf);
    
    /* crc */
    mac_conf.disable_crc = DRV_MAC_DISABLE ;

    /* rate */
    mac_conf.rate = DRV_MAC_RATE_1000 ;

    /* duplex mode */
    mac_conf.full_duplex = DRV_MAC_DUPLEX_MODE_FULL ;

    /* huge frames */
    mac_conf.allow_huge_frame = DRV_MAC_DISABLE ;

    /* enable padding of short packets */
    mac_conf.disable_pading = DRV_MAC_DISABLE ;

    /* frame length check */
    mac_conf.disable_check_frame_length = DRV_MAC_ENABLE ;
    
    /* preamble length */
    mac_conf.enable_preamble_length = DRV_MAC_ENABLE;
    mac_conf.preamble_length = CS_EMAC_PREAMBLE_LENGTH ;

    /* gaps */
    mac_conf.set_back2back_gap = DRV_MAC_ENABLE;
    mac_conf.back2back_gap = CS_EMAC_BACK2BACK_GAP ;

    mac_conf.set_non_back2back_gap = DRV_MAC_ENABLE;
    mac_conf.non_back2back_gap = CS_EMAC_NON_BACK2BACK_GAP ;

    mac_conf.set_min_interframe_gap = DRV_MAC_ENABLE;
    mac_conf.min_interframe_gap = CS_EMAC_MIN_INTER_FRAME_GAP ;

    /* Init emac */
    pi_bl_drv_mac_set_configuration ( xi_emac_id , & mac_conf );

    /* Update EMAC Frame length */
    pi_bl_drv_mac_set_max_frame_length ( xi_emac_id , CS_MAX_ETH_PACKET_SIZE );
}

void enableEmacs(DRV_MAC_NUMBER xi_emac_id,ETH_MODE xi_emacs_group_mode,ETH_MODE xi_emac4_mode)
{
	/* We need MAC0 for MDC/MDIO - always enable it */ 
    f_enable_clock_emac ( DRV_MAC_EMAC_0, BL_LILAC_CR_ON);
	f_reset_emac(DRV_MAC_EMAC_0, BL_LILAC_CR_ON);

    if ((xi_emacs_group_mode == ETH_MODE_SMII) 
        && (xi_emac_id != DRV_MAC_EMAC_0))
    {
		int i;

        for (i=DRV_MAC_EMAC_1; i <=xi_emac_id; i++)
        {
            f_enable_clock_emac ( i, BL_LILAC_CR_ON);
            f_reset_emac(i, BL_LILAC_CR_ON);
        }
    }
    else
    {
        if(xi_emac_id != DRV_MAC_EMAC_0)
        {
            f_enable_clock_emac ( xi_emac_id, BL_LILAC_CR_ON);
            f_reset_emac(xi_emac_id, BL_LILAC_CR_ON);
        }
    }

}
EXPORT_SYMBOL(enableEmacs);

LILAC_BDPI_ERRORS configureEmacs(DRV_MAC_NUMBER xi_emac_id,ETH_MODE xi_emacs_group_mode,ETH_MODE xi_emac4_mode)
{
    LILAC_BDPI_ERRORS	error;

    if ((xi_emacs_group_mode == ETH_MODE_SMII) 
        && (xi_emac_id != DRV_MAC_EMAC_0))
    {
		int i;

        for (i=DRV_MAC_EMAC_1; i <=xi_emac_id; i++)
        {
			NET_IF_CORE_MENTOR_IFC_CONTROL_DTE ifc_control;
			BL_NET_IF_CORE_MENTOR_IFC_CONTROL_READ( i, ifc_control );
			ifc_control.phy_mode = 1;
			BL_NET_IF_CORE_MENTOR_IFC_CONTROL_WRITE( i, ifc_control );
            /* un-reset runner */
            bl_modify_emac_cfg(i );
            error = config_mdc_mdio(i, xi_emacs_group_mode, xi_emac4_mode);
            if (error != BDPI_NO_ERROR)
                return error;
        }
    }
    else
    {
        /* un-reset runner */
        bl_modify_emac_cfg(xi_emac_id );
        error = config_mdc_mdio(xi_emac_id, xi_emacs_group_mode, xi_emac4_mode);
        if (error != BDPI_NO_ERROR)
            return error;
    }
	return BDPI_NO_ERROR;
}
EXPORT_SYMBOL(configureEmacs);

int bl_lilac_check_mgm_port_cfg(int mac, ETH_MODE emac_group_mode, ETH_MODE emac_mode)
{
    switch (mac)
    {
    case DRV_MAC_EMAC_0:
		if(emac_group_mode == ETH_MODE_SMII)
		{
			if(emac_mode != ETH_MODE_SGMII)
				return -1;
			return emac_mode;
		}
		if((emac_group_mode == ETH_MODE_QSGMII) || (emac_mode == ETH_MODE_SGMII ||
		        emac_group_mode == ETH_MODE_SGMII))
			return emac_group_mode;
			
		return -1;
				
	case DRV_MAC_EMAC_1:
	case DRV_MAC_EMAC_2:
	case DRV_MAC_EMAC_3:
		if(emac_group_mode != ETH_MODE_NONE)
			return emac_group_mode;
		return -1;

	case DRV_MAC_EMAC_4:
		if(emac_group_mode == ETH_MODE_SMII)
			return emac_group_mode;	
		if((emac_mode == E4_MODE_RGMII) || (emac_mode == E4_MODE_TMII)) 
			return emac_mode;
		if(emac_mode == ETH_MODE_SGMII)
			return emac_mode;
		return -1;
	case DRV_MAC_EMAC_5:
	        if(emac_group_mode == ETH_MODE_SGMII && emac_mode == ETH_MODE_SGMII)
	            return emac_group_mode;
	        else
	            return -1;
    default:
        return -1;
    }
}
EXPORT_SYMBOL(bl_lilac_check_mgm_port_cfg);


/*=======================================================================*/
void bl_lilac_phy_auto_enable(int phyId)
{
	fi_bl_drv_mac_mgmt_write(0, phyId, 0, BMCR_RESET);
	mdelay(300);
    fi_bl_drv_mac_mgmt_write(0, phyId, MII_ADVERTISE, ADVERTISE_SGMII_FULL);
	fi_bl_drv_mac_mgmt_write(0, phyId, 0, (BMCR_SPEED1000|BMCR_FULLDPLX|BMCR_ANENABLE));
}
EXPORT_SYMBOL(bl_lilac_phy_auto_enable);


int bl_lilac_ReadPHYReg(int phyAddr, int reg)
{
    uint32_t valInt = 0;

    if (fi_bl_drv_mac_mgmt_read(0, phyAddr, reg, &valInt) != DRV_MAC_NO_ERROR)
    {
        return -1;
    }

    return valInt;
}
EXPORT_SYMBOL(bl_lilac_ReadPHYReg);

int bl_lilac_WritePHYReg(int phyAddr, int reg, int val)
{
    if (fi_bl_drv_mac_mgmt_write(0, phyAddr, reg, val) != DRV_MAC_NO_ERROR)
    {
        return -1;
    }

    return 0;
}
EXPORT_SYMBOL(bl_lilac_WritePHYReg);


/*=======================================================================*/
/*  description: read current link value.  In order to do this, we need  */
/*  to read the status register twice, keeping the second value          */
/*=======================================================================*/
int bl_lilac_get_link_status(int phyAddr)
{
	int status;

	/* Do a fake read */
	status = bl_lilac_ReadPHYReg(phyAddr, MII_BMSR);

	/* Read link and autonegotiation status */
	status = bl_lilac_ReadPHYReg(phyAddr, MII_BMSR);
	if (status <= 0)
		return 0;

	if ((status & BMSR_LSTATUS) == 0)
		return 0;
	else
		return 1;

}
EXPORT_SYMBOL(bl_lilac_get_link_status);

/*=======================================================================*/
/*  Check the link, then figure out the current state                    */
/*	by comparing what we advertise with what the link partner            */
/*	advertises.  Start by checking the gigabit possibilities,            */
/*	then move on to 10/100.                                              */
/*=======================================================================*/
DRV_MAC_PHY_RATE bl_lilac_read_phy_line_status(int phyAddr)
{
	DRV_MAC_RATE 		speed  = DRV_MAC_RATE_10;
	DRV_MAC_DUPLEX_MODE  duplex = DRV_MAC_DUPLEX_MODE_HALF;
	int linkStatus;   /* link up -> 1 ,link down -> 0 */
	int autonegEnable;
	int lpa;
	int lpagb;
	int adv;

	linkStatus  = bl_lilac_get_link_status(phyAddr);

	if( linkStatus < 0)
		return DRV_MAC_PHY_RATE_ERR;
	else  if( linkStatus == 0) 
		return DRV_MAC_PHY_RATE_LINK_DOWN;

	autonegEnable = ((bl_lilac_ReadPHYReg(phyAddr,MII_BMCR)& BMCR_ANENABLE) == BMCR_ANENABLE);

	
	if(autonegEnable)
	{
        autonegEnable = bl_lilac_ReadPHYReg(phyAddr,MII_BMSR);
        if (autonegEnable & BMSR_ANEGCOMPLETE)
        {
		    lpagb = bl_lilac_ReadPHYReg(phyAddr, MII_STAT1000);
            
		    if (lpagb < 0)
		    	return DRV_MAC_PHY_RATE_ERR;
            
		    adv = bl_lilac_ReadPHYReg(phyAddr, MII_CTRL1000);
            
		    if (adv < 0)
		    	return DRV_MAC_PHY_RATE_ERR;
            
		    lpagb &= adv << 2;
            
		    lpa = bl_lilac_ReadPHYReg(phyAddr, MII_LPA);
            
		    if (lpa < 0)
		    	return DRV_MAC_PHY_RATE_ERR;
            
		    adv = bl_lilac_ReadPHYReg(phyAddr, MII_ADVERTISE);
            
		    if (adv < 0)
		    	return DRV_MAC_PHY_RATE_ERR;
            
		    lpa &= adv;
            
		    if (lpagb & (LPA_1000FULL | LPA_1000HALF)) 
		    {
		    	speed = DRV_MAC_RATE_1000;
            
		    	if (lpagb & LPA_1000FULL)
		    		duplex = DRV_MAC_DUPLEX_MODE_FULL;
		    	
		    } 
		    else if (lpa & (LPA_100FULL | LPA_100HALF)) 
		    {
		    	speed = DRV_MAC_RATE_100;
		    	
		    	if (lpa & LPA_100FULL)
		    		duplex = DRV_MAC_DUPLEX_MODE_FULL;
		    }
		    else if (lpa & LPA_10FULL)
		    {
		    		duplex = DRV_MAC_DUPLEX_MODE_FULL;
		    }
        }
		else
            return DRV_MAC_PHY_RATE_LINK_DOWN;
	}
	else
	{
		int bmcr = bl_lilac_ReadPHYReg(phyAddr, MII_BMCR);
		if (bmcr < 0)
		{
			return DRV_MAC_PHY_RATE_ERR;
		}

		if (bmcr & BMCR_FULLDPLX)
		{
			duplex = DRV_MAC_DUPLEX_MODE_FULL;
		}

		if (bmcr & BMCR_SPEED1000)
		{
			speed = DRV_MAC_RATE_1000;
		}
		else if (bmcr & BMCR_SPEED100)
		{
			speed = DRV_MAC_RATE_100;
		}
	}
	return speed + duplex;
} 
EXPORT_SYMBOL(bl_lilac_read_phy_line_status);


static void bl_lilac_mac_autoneg(int macId, ETH_MODE etch_mode, DRV_MAC_RATE rate, DRV_MAC_DUPLEX_MODE mode)
{  
	pi_bl_drv_mac_set_rate(macId,   rate);
	pi_bl_drv_mac_set_duplex(macId, mode);
}


/*=======================================================================*/
/* function   switch MAC  between the modes 10Mb, 100Mb and 1G           */
/* according to link status                                              */ 
/*=======================================================================*/
DRV_MAC_PHY_RATE bl_lilac_auto_mac(int phyId, int macId, ETH_MODE mode, DRV_MAC_PHY_RATE cur_rate)
{
    DRV_MAC_PHY_RATE  phyRate;
	ETHERNET_MAC_INFO enetMacInfo;
	int               retVal,rate;

	retVal = BpGetEthernetMacInfo( &enetMacInfo, 1 );
	if ( BP_SUCCESS == retVal ) 
	{
		int linkStatus;
		unsigned long mac_type;
		if (!IsPortConnectedToExternalSwitch(enetMacInfo.sw.phy_id[phyId]))
			phyRate = bl_lilac_read_phy_line_status(phyId);
		else
		{
			mac_type = enetMacInfo.sw.phy_id[macId] & MAC_IFACE;
			if ((mac_type == MAC_IF_TMII) || (mac_type == MAC_IF_SS_SMII) || (mac_type == MAC_IF_MII))
				rate = DRV_MAC_PHY_RATE_100_FULL;
			else  
				rate = DRV_MAC_PHY_RATE_1000_FULL;
			linkStatus = bl_lilac_get_link_status(0);
			if (linkStatus <= 0)
				phyRate = DRV_MAC_PHY_RATE_LINK_DOWN;
			else
				phyRate = rate;
		}
	}
	else
		phyRate = cur_rate;

	if(phyRate == cur_rate)
		return phyRate;

    switch( phyRate )
    {
    case DRV_MAC_PHY_RATE_1000_FULL:
        bl_lilac_mac_autoneg(macId, mode, DRV_MAC_RATE_1000, DRV_MAC_DUPLEX_MODE_FULL);
        break;
    case DRV_MAC_PHY_RATE_1000_HALF:
        bl_lilac_mac_autoneg(macId, mode, DRV_MAC_RATE_1000, DRV_MAC_DUPLEX_MODE_HALF);
        break;
    case DRV_MAC_PHY_RATE_100_FULL:
        bl_lilac_mac_autoneg(macId, mode, DRV_MAC_RATE_100, DRV_MAC_DUPLEX_MODE_FULL);
        break;
    case DRV_MAC_PHY_RATE_100_HALF:
        bl_lilac_mac_autoneg(macId, mode, DRV_MAC_RATE_100, DRV_MAC_DUPLEX_MODE_HALF);
        break;
    case DRV_MAC_PHY_RATE_10_FULL:
        bl_lilac_mac_autoneg(macId, mode, DRV_MAC_RATE_10, DRV_MAC_DUPLEX_MODE_FULL);
        break;
    case DRV_MAC_PHY_RATE_10_HALF:
        bl_lilac_mac_autoneg(macId, mode, DRV_MAC_RATE_10, DRV_MAC_DUPLEX_MODE_HALF);
        break;
    default:
        break;
    }

	printk("PHY %d on MAC %d : link state = %s\n\r", phyId, macId, MAC_PHY_RATE_NAME[phyRate]);
	
	return phyRate;
}
EXPORT_SYMBOL(bl_lilac_auto_mac);

void bl_lilac_start_25M_clock(void)
{
        bl_lilac_set_synce_tdm_divider(BL_PLL_0, 0, 0xc8, 1, 1, 0);
        bl_lilac_mux_connect_25M_clock(GPIO_4);
        bl_lilac_enable_25M_clock();
        mdelay(500);
}
EXPORT_SYMBOL(bl_lilac_start_25M_clock);





