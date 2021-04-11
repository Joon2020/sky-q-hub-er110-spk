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
#include "bl_lilac_cr.h"
#include "bl_lilac_serdes.h"
#include "bl_lilac_drv_serdes.h"
#include "bl_lilac_rdd.h"
#include "bl_lilac_pin_muxing.h"
#include "bl_lilac_gpio.h"
#include "boardparms.h"

LILAC_BDPI_ERRORS fi_basic_runner_data_path_init ( DRV_MAC_NUMBER xi_emac_id,ETH_MODE xi_emacs_group_mode,uint8_t* xi_ddr_pool_ptr,unsigned long xi_ddr_pool_size,uint8_t* xi_extra_ddr_pool_ptr);
void bl_lilac_put_all_data_devs_to_reset(int emac_id);

static	uint8_t* 	 extra_ddr_pool_ptr = NULL;
static	uint8_t* 	 ddr_pool_ptr; 
static	unsigned long	 ddr_pool_size; 

static LILAC_BDPI_ERRORS f_serdes_start_lan ( ETH_MODE emacs_group_mode, DRV_MAC_NUMBER xi_emac_num )
{
    BL_DRV_SERDES_GENERAL_CONFIG_DTE serdes;
	BL_DRV_SERDES_EMAC_ID_DTE xi_emac_id;

    fi_bl_drv_serdes_get_general_configuration(&serdes);
    serdes.emacs_group_mode = emacs_group_mode;
    fi_bl_drv_serdes_set_general_configuration ( &serdes );

    /* NETIF SERDES configure and enable */
	switch(xi_emac_num)
	{
		case DRV_MAC_EMAC_0:
			xi_emac_id = CE_BL_DRV_SERDES_EMAC_ID_0;
			break;
		case DRV_MAC_EMAC_1:
			xi_emac_id = CE_BL_DRV_SERDES_EMAC_ID_1;
			break;
		case DRV_MAC_EMAC_2:
			xi_emac_id = CE_BL_DRV_SERDES_EMAC_ID_2;
			break;
		case DRV_MAC_EMAC_3:
			xi_emac_id = CE_BL_DRV_SERDES_EMAC_ID_3;
			break;
		default:
			xi_emac_id = xi_emac_num;
			break;
	}
    pi_bl_drv_serdes_netif_configure_and_enable (xi_emac_id);

    return ( BDPI_NO_ERROR);
}

static LILAC_BDPI_ERRORS f_vpb_set_emacs_mode( BL_LILAC_VPB_MODESEL_FIELD_VALUE xi_emacs_group_mode ,
                                                ETH_MODE xi_emac4_mode )
{
    BL_LILAC_VPB_STATUS status ;
	
    status = bl_lilac_vpb_modesel_field_set(BL_LILAC_VPB_ETH_MODE , xi_emacs_group_mode);
    if( status != BL_LILAC_VPB_OK )
        return ( BDPI_ERROR_VPB_INITIALIZATION_ERROR);

    /* when emacs_group_mode is SS_SMII, emac4_mode is irrelevant */
    if( (xi_emac4_mode == E4_MODE_RGMII) || (xi_emac4_mode == E4_MODE_TMII))
    {
        status = bl_lilac_vpb_modesel_field_set ( BL_LILAC_VPB_E4_MODE ,
                                                  E4_MODE_TOV_PB_E4_MODE(xi_emac4_mode));
        if ( status != BL_LILAC_VPB_OK )
            return ( BDPI_ERROR_VPB_INITIALIZATION_ERROR);
    }

    return ( BDPI_NO_ERROR);
}

static void setPinMuxing(DRV_MAC_NUMBER xi_emac_id,ETH_MODE xi_emacs_group_mode,ETH_MODE xi_emac4_mode)
{
	if((xi_emac_id == DRV_MAC_EMAC_4) && (xi_emac4_mode == E4_MODE_TMII)) 
		bl_lilac_start_25M_clock();
    if ((xi_emacs_group_mode == ETH_MODE_SMII) && (xi_emac_id != DRV_MAC_EMAC_0))
        bl_lilac_mux_connect_ss_mii();
}

static void bl_lilac_HW_reset_phy(int port,int reset)
{
	unsigned short pin;

	if (BpGetPhyResetGpio(port,&pin) == BP_SUCCESS)
	{
		pin = pin & ~BP_ACTIVE_LOW;
		bl_lilac_gpio_set_mode(pin, BL_LILAC_GPIO_MODE_OUTPUT);
		bl_lilac_gpio_write_pin(pin, reset);
	}
}

LILAC_BDPI_ERRORS fi_basic_data_path_init ( DRV_MAC_NUMBER xi_emac_id,
                                            ETH_MODE xi_emacs_group_mode,
                                            ETH_MODE xi_emac4_mode,
											int do_rst)
{
    LILAC_BDPI_ERRORS	error;

	if (do_rst)
		bl_lilac_put_all_data_devs_to_reset((int) xi_emac_id);

	bl_lilac_cr_device_reset ( BL_LILAC_CR_DLL_SEL_RX , BL_LILAC_CR_ON);
	bl_lilac_cr_device_reset ( BL_LILAC_CR_DLL_SEL_TX , BL_LILAC_CR_ON);
    /* set EMACs mode in VPB bridge */
    if (xi_emacs_group_mode == ETH_MODE_NONE)
		f_vpb_set_emacs_mode(BL_LILAC_VPB_ETH_MODE_QSGMII, xi_emac4_mode);
    else
    	f_vpb_set_emacs_mode ( xi_emacs_group_mode , xi_emac4_mode);
    /* GPON/UBB selection in VPB bridge */
    bl_lilac_vpb_modesel_field_set( BL_LILAC_VPB_WAN_UBB_SEL ,
                                    BL_LILAC_VPB_WAN_UBB_SEL_UBB);
	/* enable clocks and resets for emac0 and required emac */
	enableEmacs(xi_emac_id,xi_emacs_group_mode,xi_emac4_mode);
	/* reset phy */

	bl_lilac_HW_reset_phy(xi_emac_id,1);

	/* make all the required pin muxing connection */
	setPinMuxing(xi_emac_id,xi_emacs_group_mode,xi_emac4_mode);
	/* configure emac according to its mode */
	error = configureEmacs(xi_emac_id,xi_emacs_group_mode,xi_emac4_mode);
	if (error != BDPI_NO_ERROR)
		return error;
	/* enable serdes lane for emac */
	if (xi_emac_id < DRV_MAC_EMAC_4)
	{
	    /* start LAN of NETIF SERDES, only SGMII, QSGMII or HiSGMII */
	    error = f_serdes_start_lan (xi_emacs_group_mode, xi_emac_id);
	    if ( error != BDPI_NO_ERROR )
	        return ( error);
	}
	/* reset phy */
	if((xi_emacs_group_mode != ETH_MODE_QSGMII) && (xi_emacs_group_mode != ETH_MODE_NONE))
		bl_lilac_HW_reset_phy(xi_emac_id,1);
	/*RNR_BASE_ADDR_STR*/
#if defined _CFE_
    ddr_pool_size = 0x1400000;
    ddr_pool_ptr = (uint8_t*)K0_TO_K1((uint32_t)KMALLOC(ddr_pool_size, 0x200000));
	if(ddr_pool_ptr == NULL)
        return BDPI_ERROR_MEMORY_ERROR;
#else
	if(bl_lilac_mem_rsrv_get_by_name(TM_BASE_ADDR_STR, (void **)&ddr_pool_ptr, &ddr_pool_size) != BL_LILAC_SOC_OK)
        return BDPI_ERROR_MEMORY_ERROR;
#endif
	/* initialize Runner data path */
	return fi_basic_runner_data_path_init(xi_emac_id,xi_emacs_group_mode,ddr_pool_ptr,ddr_pool_size,extra_ddr_pool_ptr);
}
EXPORT_SYMBOL(fi_basic_data_path_init);

static void bl_lilac_disable_active_emacs(DRV_MAC_NUMBER mac_number)
{
	BL_LILAC_CR_STATE reset_state;
	
	if(bl_lilac_cr_get_emac_reset_state(bl_lilac_map_emac_id_to_cr_device(mac_number), &reset_state) != BL_LILAC_SOC_OK)
		return ;

	if(reset_state == BL_LILAC_CR_ON)
	{
		pi_bl_drv_mac_enable(mac_number, DRV_MAC_DISABLE);
	}

}

void bl_lilac_put_all_data_devs_to_reset(int emac_id)
{
	int i;
	/* disable active MACs */
	bl_lilac_disable_active_emacs(DRV_MAC_EMAC_0);
	bl_lilac_disable_active_emacs(emac_id);
	
	/*  wait 1 second */ 
	for(i=0; i< 10; i++)
		mdelay(100);
	
	bl_rdd_runner_disable();
	
    /* reset TM path */
    bl_lilac_cr_device_reset(BL_LILAC_CR_PKT_SRAM_OCP_C,       BL_LILAC_CR_OFF);
    bl_lilac_cr_device_reset(BL_LILAC_CR_PKT_SRAM_OCP_D,       BL_LILAC_CR_OFF);
    bl_lilac_cr_device_reset(BL_LILAC_CR_GENERAL_MAIN_BB_RNR_BB_MAIN,	BL_LILAC_CR_OFF);
	
    /* reset IH path */
    bl_lilac_cr_device_reset(BL_LILAC_CR_IH_RNR,               BL_LILAC_CR_OFF);

	/* reset emac */
	bl_lilac_cr_device_reset(BL_LILAC_CR_NIF_0,                BL_LILAC_CR_OFF);
	bl_lilac_cr_device_reset(bl_lilac_map_emac_id_to_cr_device(emac_id), BL_LILAC_CR_OFF );

    /* Runner path */
    bl_lilac_cr_device_reset(BL_LILAC_CR_RNR_SUB,              BL_LILAC_CR_OFF);
    bl_lilac_cr_device_reset(BL_LILAC_CR_RNR_1,                BL_LILAC_CR_OFF);
    bl_lilac_cr_device_reset(BL_LILAC_CR_RNR_0,                BL_LILAC_CR_OFF);
		
	bl_lilac_cr_device_reset (BL_LILAC_CR_DLL_SEL_RX, 		   BL_LILAC_CR_OFF);
    bl_lilac_cr_device_reset (BL_LILAC_CR_DLL_SEL_TX, 		   BL_LILAC_CR_OFF);

    /* disable clocks */
    bl_lilac_cr_device_enable_clk(BL_LILAC_CR_TM,              BL_LILAC_CR_OFF);
    bl_lilac_cr_device_enable_clk(bl_lilac_map_emac_id_to_cr_device(emac_id), BL_LILAC_CR_OFF);
    bl_lilac_cr_device_enable_clk(BL_LILAC_CR_NIF_0,           BL_LILAC_CR_OFF);

}
EXPORT_SYMBOL(bl_lilac_put_all_data_devs_to_reset);

