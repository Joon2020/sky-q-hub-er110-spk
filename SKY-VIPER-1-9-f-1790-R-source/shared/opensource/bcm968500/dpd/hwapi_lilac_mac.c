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

/******************************************************************************/
/*                                                                            */
/* File Description:                                                          */
/*                                                                            */
/* This file implementation for the hwapi_mac interface for Lilac Chip        */
/* 					                                                          */
/*                                                                            */
/******************************************************************************/
#include "access_macros.h"
#include "packing.h"
#include "hwapi_mac.h"
#include "bl_os_wraper.h"
#include "bl_lilac_drv_mac.h"
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_configuration                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - get configuration                                           */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   get the configuratin of a mac port	,note that current status of emac	  */
/*   might be different than configuration when working in autoneg			  */
/*	to get the current status use mac_hwapi_get_mac_status API				  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                                                            */
/*                                                                            */
/* Output:                                                                    */
/*    emac_cfg -  structure holds the current configuration of the mac  port  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_get_configuration(rdpa_emac emacNum,rdpa_emac_cfg_t *rdpa_emac_cfg_t)
{
	DRV_MAC_CONF  mac_conf = {};

	pi_bl_drv_mac_get_configuration(emacNum,&mac_conf);
	/*fill the rdpa structure*/
	rdpa_emac_cfg_t->allow_too_long		=	mac_conf.allow_huge_frame;
	rdpa_emac_cfg_t->back2back_gap		=	mac_conf.back2back_gap;
	rdpa_emac_cfg_t->check_length		=	mac_conf.disable_check_frame_length ? 0 : 1;
	rdpa_emac_cfg_t->full_duplex		=	mac_conf.full_duplex;
	rdpa_emac_cfg_t->generate_crc		=	mac_conf.disable_crc ? 0 : 1;
	rdpa_emac_cfg_t->loopback			=   0;
	rdpa_emac_cfg_t->min_interframe_gap	=	mac_conf.min_interframe_gap;
	rdpa_emac_cfg_t->non_back2back_gap	=	mac_conf.non_back2back_gap;
	rdpa_emac_cfg_t->pad_short			=	mac_conf.disable_pading ? 0 : 1;
	rdpa_emac_cfg_t->preamble_length	=	mac_conf.preamble_length;
	rdpa_emac_cfg_t->rx_flow_control	=	mac_conf.flow_control_rx;
	rdpa_emac_cfg_t->tx_flow_control	=	mac_conf.flow_control_tx;

	switch (mac_conf.rate)
	{
	case  DRV_MAC_RATE_10:
		rdpa_emac_cfg_t->rate = rdpa_emac_rate_10m;
		break ;
	case  DRV_MAC_RATE_100:
		rdpa_emac_cfg_t->rate = rdpa_emac_rate_100m ;
		break ;
	case DRV_MAC_RATE_1000:
		rdpa_emac_cfg_t->rate =  rdpa_emac_rate_1g;
		break ;
	default:
		printk("mac_hwapi_get_configuration wrong rdpa emac rate = %d\n ",rdpa_emac_cfg_t->rate);

	}

}
EXPORT_SYMBOL (mac_hwapi_get_configuration);
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_set_configuration                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set configuration                                           */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   set the configuratin of a mac port                                       */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*   emac_cfg -  structure holds the current configuration of the mac  port   */
/*                                                                            */
/* Output:                                                                    */
/*      																	  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_set_configuration(rdpa_emac emacNum,rdpa_emac_cfg_t *emac_cfg)
{
	DRV_MAC_CONF  mac_conf = {};

	/*read configuration*/
	pi_bl_drv_mac_get_configuration(emacNum,&mac_conf);

	/*fill the mac structure*/
	mac_conf.allow_huge_frame	 		= 	emac_cfg->allow_too_long;
	mac_conf.back2back_gap				=	emac_cfg->back2back_gap;
	mac_conf.disable_check_frame_length = 	!emac_cfg->check_length;
	mac_conf.full_duplex				= 	emac_cfg->full_duplex ? DRV_MAC_DUPLEX_MODE_FULL:DRV_MAC_DUPLEX_MODE_HALF;
	mac_conf.disable_crc				=	!emac_cfg->generate_crc;
	mac_conf.min_interframe_gap			=	emac_cfg->min_interframe_gap;
	mac_conf.non_back2back_gap			=	emac_cfg->non_back2back_gap;
	mac_conf.disable_pading				=	!emac_cfg->pad_short;
	mac_conf.preamble_length			=	emac_cfg->preamble_length;
	mac_conf.flow_control_rx			=	emac_cfg->rx_flow_control;
	mac_conf.flow_control_tx			=	emac_cfg->tx_flow_control;
	mac_conf.enable_preamble_length		= 	DRV_MAC_ENABLE;
	mac_conf.set_back2back_gap 			= 	DRV_MAC_ENABLE;
	mac_conf.set_min_interframe_gap 	= 	DRV_MAC_ENABLE;
	mac_conf.set_non_back2back_gap 		= 	DRV_MAC_ENABLE;

	switch (emac_cfg->rate)
	    {
	    case rdpa_emac_rate_10m:
	        mac_conf.rate = DRV_MAC_RATE_10 ;
	        break ;
	    case rdpa_emac_rate_100m:
	        mac_conf.rate = DRV_MAC_RATE_100 ;
	        break ;
	    case rdpa_emac_rate_1g:
	        mac_conf.rate = DRV_MAC_RATE_1000 ;
	        break ;
	    default:
	    	printk("mac_hwapi_set_configuration wrong rdpa emac rate = %d\n ",emac_cfg->rate);
	    	mac_conf.rate = DRV_MAC_RATE_1000 ;
	    }

	pi_bl_drv_mac_set_configuration(emacNum,&mac_conf);

}
EXPORT_SYMBOL (mac_hwapi_set_configuration);
/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_duplex		                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - get port duplex                                         	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	get the dulplex of a port												  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                                                            */
/*                                                                            */
/* Output:                                                                    */
/*    full_duples  :  1 = full dulplex,0 = half_duplex						  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_get_duplex(rdpa_emac emacNum,int32_t *full_duplex)
{
	DRV_MAC_DUPLEX_MODE 	duplex;
	pi_bl_drv_mac_get_duplex(emacNum,&duplex);
	*full_duplex = (duplex==DRV_MAC_DUPLEX_MODE_FULL) ? 1 : 0;
}
EXPORT_SYMBOL (mac_hwapi_get_duplex);


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_set_duplex		                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set port duplex                                         	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	set the dulplex of a port												  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                                                            */
/*                                                                            */
/* Output:                                                                    */
/*    full_duples  :  1 = full dulplex,0 = half_duplex						  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_set_duplex(rdpa_emac emacNum,int32_t full_duplex)
{
	DRV_MAC_DUPLEX_MODE 	duplex;
	duplex = full_duplex ? DRV_MAC_DUPLEX_MODE_FULL:DRV_MAC_DUPLEX_MODE_HALF;
	pi_bl_drv_mac_set_duplex(emacNum,duplex);

}
EXPORT_SYMBOL (mac_hwapi_set_duplex);


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_speed		                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - get port speed rate                                      	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	get the speed of a port													  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                              							                  */
/*                                                                            */
/* Output:                                                                    */
/*      rate -   enum of the speed                                            */
/******************************************************************************/
void mac_hwapi_get_speed(rdpa_emac emacNum,rdpa_emac_rate *rate)
{
	DRV_MAC_RATE	macRate;
	pi_bl_drv_mac_get_rate(emacNum,&macRate);
	switch(macRate)
	{
	case DRV_MAC_RATE_1000:
		*rate = rdpa_emac_rate_1g;
		break;
	case DRV_MAC_RATE_100:
		*rate = rdpa_emac_rate_100m;
		break;
	case DRV_MAC_RATE_10:
		*rate = rdpa_emac_rate_10m;
		break;
	default:
		break;
	}

}
EXPORT_SYMBOL (mac_hwapi_get_speed);



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_set_speed		                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set port speed rate                                      	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	set the speed of a port													  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*      rate -   enum of the speed   						                  */
/*                                                                            */
/* Output:                                                                    */
/*                                   							              */
/******************************************************************************/

void mac_hwapi_set_speed(rdpa_emac emacNum,rdpa_emac_rate rate)
{
	DRV_MAC_RATE	macRate = DRV_MAC_RATE_1000;

	switch(rate)
	{
	case rdpa_emac_rate_1g:
		macRate = DRV_MAC_RATE_1000;
		break;
	case rdpa_emac_rate_100m:
		macRate = DRV_MAC_RATE_100;
		break;
	case rdpa_emac_rate_10m:
		macRate = DRV_MAC_RATE_10 ;
		break;
	default:
		break;
	}

	pi_bl_drv_mac_set_rate(emacNum,macRate);
}
EXPORT_SYMBOL ( mac_hwapi_set_speed );



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_rxtx_enable                                                */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set tx and rx enable                                  	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	get tx and rx enable													  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                             								  */
/*                                                                            */
/* Output:                                                                    */
/*       rxtxEnable  - boolean enable   	                                  */
/******************************************************************************/
void mac_hwapi_get_rxtx_enable(rdpa_emac emacNum,int32_t *rxEnable,int32_t *txEnable)
{
	E_DRV_MAC_ENABLE tx_enable;
	E_DRV_MAC_ENABLE rx_enable;
	pi_bl_drv_mac_get_status(emacNum,&tx_enable,&rx_enable);
	*rxEnable	=	 (rx_enable == DRV_MAC_ENABLE) ? 1 : 0;
	*txEnable	=	 (tx_enable == DRV_MAC_ENABLE) ? 1 : 0;

}
EXPORT_SYMBOL (mac_hwapi_get_rxtx_enable);



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_set_rxtx_enable                                                */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set tx and rx enable                                  	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	set tx and rx enable													  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*    rxtxEnable  - boolean enable                                            */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_set_rxtx_enable(rdpa_emac emacNum,int32_t rxEnable,int32_t txEnable)
{
	pi_bl_drv_mac_enable(emacNum,txEnable);
}
EXPORT_SYMBOL (mac_hwapi_set_rxtx_enable);



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_sw_reset		                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - get port software reset                                  	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	get the sw reset bit of emac port										  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                                                            */
/*                                                                            */
/* Output:                                                                    */
/*    swReset  :  1 = reset,0 = not reset									  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_get_sw_reset(rdpa_emac emacNum,int32_t *swReset)
{
	pi_bl_drv_mac_get_reset_bit(emacNum,(E_DRV_MAC_ENABLE * )swReset);
}
EXPORT_SYMBOL (mac_hwapi_get_sw_reset);



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_set_sw_reset		                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set port software reset                                  	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	set the sw reset bit of emac port										  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*   swReset  :  1 = reset,0 = not reset	                                  */
/*                                                                            */
/* Output:                                                                    */
/*    								  										  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_set_sw_reset(rdpa_emac emacNum,int32_t swReset)
{
	pi_bl_drv_mac_sw_reset(emacNum,swReset);
}
EXPORT_SYMBOL (mac_hwapi_set_sw_reset);



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_tx_max_frame_len		                                      */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - get port TX MTU                                         	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	get the port maximum transmit unit size in bytes						  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                              										      */
/*                                                                            */
/* Output:                                                                    */
/*    maxTxFrameLen - size of frame in bytes 								  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_get_tx_max_frame_len(rdpa_emac emacNum,uint32_t *maxTxFrameLen )
{
	pi_bl_drv_mac_get_max_frame_length(emacNum,maxTxFrameLen);
}
EXPORT_SYMBOL (mac_hwapi_get_tx_max_frame_len);



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_set_tx_max_frame_len		                                      */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set port TX MTU                                         	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	set the port maximum transmit unit size in bytes						  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*    maxTxFrameLen - size of frame in bytes                         	      */
/*                                                                            */
/* Output:                                                                    */
/*   																		  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_set_tx_max_frame_len(rdpa_emac emacNum,uint32_t maxTxFrameLen )
{
	pi_bl_drv_mac_set_max_frame_length(emacNum,maxTxFrameLen);
}
EXPORT_SYMBOL ( mac_hwapi_set_tx_max_frame_len );



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_rx_max_frame_len		                                      */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - get port RX MTU                                         	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	get the port maximum receive unit size in bytes							  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                           	     										  */
/*                                                                            */
/* Output:                                                                    */
/*   maxRxFrameLen - size of current MRU									  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_get_rx_max_frame_len(rdpa_emac emacNum,uint32_t *maxRxFrameLen )
{
	/*do nothing in lilac*/
}




/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_set_rx_max_frame_len		                                      */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set port RX MTU                                         	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	set the port maximum receive unit size in bytes							  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*    maxRxFrameLen - size of current MRU        							  */
/*                                                                            */
/* Output:                                                                    */
/*   									  									  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_set_rx_max_frame_len(rdpa_emac emacNum,uint32_t maxRxFrameLen )
{
	/*do nothing in lilac*/
}




/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_set_tx_igp_len	                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set port inter frame gap                                 	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	set the inter frame gap	size in bytes									  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*   txIpgLen - length in bytes                                               */
/*                                                                            */
/* Output:                                                                    */
/*    																		  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_set_tx_igp_len(rdpa_emac emacNum,uint32_t txIpgLen )
{
	/*do nothing in lilac*/
}



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_tx_igp_len	                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - get port inter frame gap                                 	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	get the inter frame gap	size in bytes									  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                      							          */
/*                                                                            */
/* Output:                                                                    */
/*    txIpgLen - length in bytes    										  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_get_tx_igp_len(rdpa_emac emacNum,uint32_t *txIpgLen )
{
	/*do nothing in lilac*/
}




/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_mac_status	                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set port status                                         	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	set the status of mac													  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                                                            */
/*                                                                            */
/* Output:                                                                    */
/*    macStatus  :  														  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_get_mac_status(rdpa_emac emacNum,S_HWAPI_MAC_STATUS *macStatus)
{
	/*do nothing in lilac*/
}





/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_flow_control                                               */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - get port flow control                                    	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	get the flow control of a port											  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*           																  */
/*                                                                            */
/* Output:                                                                    */
/*   flowControl - structure with parameters of tx and rx flow control		  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_get_flow_control(rdpa_emac emacNum,S_MAC_HWAPI_FLOW_CTRL *flowControl)
{
	E_DRV_MAC_ENABLE tx_flow_control;
	E_DRV_MAC_ENABLE rx_flow_control;

	pi_bl_drv_mac_get_flow_control(emacNum,&tx_flow_control,&rx_flow_control);
	flowControl->rxFlowEnable = (int32_t)rx_flow_control;
	flowControl->txFlowEnable = (int32_t)tx_flow_control;
}
EXPORT_SYMBOL (mac_hwapi_get_flow_control);



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_set_flow_control                                               */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set port flow control                                    	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	set the flow control of a port											  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*   flowControl - structure with parameters of tx and rx flow control        */
/*                                                                            */
/* Output:                                                                    */
/*   						  												  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_set_flow_control(rdpa_emac emacNum,S_MAC_HWAPI_FLOW_CTRL *flowControl)
{
	pi_bl_drv_mac_set_flow_control(emacNum,flowControl->txFlowEnable,flowControl->rxFlowEnable);
}
EXPORT_SYMBOL ( mac_hwapi_set_flow_control );

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_set_pause_params                                               */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - set port flow control                                    	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	set the flow control of a port											  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*   flowControl - structure with parameters of tx and rx flow control        */
/*                                                                            */
/* Output:                                                                    */
/*   						  												  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_set_pause_params(rdpa_emac emacNum,int32_t pauseCtrlEnable,uint32_t pauseTimer,uint32_t pauseQuanta)
{
	pi_bl_drv_mac_set_pause_packets_transmision_parameters(emacNum,pauseCtrlEnable,pauseTimer);
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_rx_counters	                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - 	get the rx counters of port                           	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	get the rx counters of port				  								  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                                                            */
/*                                                                            */
/* Output:                                                                    */
/*    rxCounters  :  structure filled with counters							  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_get_rx_counters(rdpa_emac emacNum,rdpa_emac_rx_stat_t *rxCounters)
{
	/* Read EMAC counters from mac driver */
	DRV_MAC_RX_COUNTERS	emac_rx_counters;
	pi_bl_drv_mac_read_rx_counters(emacNum, &emac_rx_counters);
	rxCounters->fcs_error             = emac_rx_counters.fcs_errors ;
	rxCounters->oversize_packet       = ( emac_rx_counters.oversize_frames - emac_rx_counters.jabber_frames ) ;
	rxCounters->undersize_packet      = emac_rx_counters.undersize_frames ;
	rxCounters->frame_64              = emac_rx_counters.frame_64_bytes;
	rxCounters->frame_65_127          = emac_rx_counters.frame_65_127_bytes;
	rxCounters->frame_128_255         = emac_rx_counters.frame_128_255_bytes;
	rxCounters->frame_256_511         = emac_rx_counters.frame_256_511_bytes;
	rxCounters->frame_512_1023        = emac_rx_counters.frame_512_1023_bytes;
	rxCounters->frame_1024_1518       = emac_rx_counters.frame_1024_1518_bytes;
	rxCounters->frame_1519_mtu        = emac_rx_counters.frame_1519_mtu_bytes;
	rxCounters->byte                  = emac_rx_counters.bytes;
	rxCounters->packet                = emac_rx_counters.packets;
	rxCounters->multicast_packet      = emac_rx_counters.multicast_packets;
	rxCounters->broadcast_packet      = emac_rx_counters.broadcat_packets;
	rxCounters->unicast_packet	      = emac_rx_counters.packets
			- (emac_rx_counters.broadcat_packets + emac_rx_counters.multicast_packets);
	rxCounters->alignment_error       = emac_rx_counters.alignment_error;
	rxCounters->frame_length_error    = emac_rx_counters.length_error;
	rxCounters->code_error            = emac_rx_counters.code_error;
	rxCounters->carrier_sense_error   = emac_rx_counters.carrier_sense_error;
	rxCounters->control_frame         = emac_rx_counters.control_packets ;
	rxCounters->pause_control_frame   = emac_rx_counters.pause_packets ;
	rxCounters->unknown_opcode        = emac_rx_counters.unknown_op ;
	rxCounters->fragments             = emac_rx_counters.fragmets ;
	rxCounters->jabber                = emac_rx_counters.jabber_frames ;
	rxCounters->overflow              = emac_rx_counters.overflow ;
}
EXPORT_SYMBOL (mac_hwapi_get_rx_counters);



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_tx_counters	                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - 	get the tx counters of port                           	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	get the tx counters of port				  								  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                                                            */
/*                                                                            */
/* Output:                                                                    */
/*    txCounters  :  structure filled with counters							  */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_get_tx_counters(rdpa_emac emacNum,rdpa_emac_tx_stat_t *txCounters)
{
	DRV_MAC_TX_COUNTERS emac_tx_counters ;
	 /* Read EMAC counters from mac driver */
	pi_bl_drv_mac_read_tx_counters(emacNum, &emac_tx_counters);

	txCounters->packet                      = emac_tx_counters.packets ;
	txCounters->fcs_error                   = emac_tx_counters.fcs_errors ;
	txCounters->frame_64                    = emac_tx_counters.frame_64_bytes;
	txCounters->frame_65_127                = emac_tx_counters.frame_65_127_bytes;
	txCounters->frame_128_255               = emac_tx_counters.frame_128_255_bytes;
	txCounters->frame_256_511               = emac_tx_counters.frame_256_511_bytes;
	txCounters->frame_512_1023              = emac_tx_counters.frame_512_1023_bytes;
	txCounters->frame_1024_1518             = emac_tx_counters.frame_1024_1518_bytes;
	txCounters->frame_1519_mtu              = emac_tx_counters.frame_1519_mtu_bytes;
	txCounters->byte                        = emac_tx_counters.bytes;
	txCounters->multicast_packet            = emac_tx_counters.multicast_packets;
	txCounters->broadcast_packet            = emac_tx_counters.broadcat_packets;
	txCounters->unicast_packet	            = emac_tx_counters.broadcat_packets
			- (emac_tx_counters.broadcat_packets + emac_tx_counters.multicast_packets);
	txCounters->excessive_collision         = emac_tx_counters.excessive_collision;
	txCounters->late_collision              = emac_tx_counters.late_collision;
	txCounters->single_collision            = emac_tx_counters.single_collision;
	txCounters->multiple_collision          = emac_tx_counters.multiple_collision;
	txCounters->total_collision             = emac_tx_counters.total_collision;
	txCounters->pause_control_frame         = emac_tx_counters.pause_packets ;
	txCounters->deferral_packet             = emac_tx_counters.deferral_packets ;
	txCounters->excessive_deferral_packet   = emac_tx_counters.excessive_packets ;
	txCounters->jabber_frame                = emac_tx_counters.jabber_frames ;
	txCounters->control_frame               = emac_tx_counters.control_frames ;
	txCounters->oversize_frame              = emac_tx_counters.oversize_frames - emac_tx_counters.jabber_frames;
	txCounters->undersize_frame             = emac_tx_counters.undersize_frames ;
	txCounters->fragments_frame             = emac_tx_counters.fragmets ;
	txCounters->underrun                    = emac_tx_counters.underrun ;
	txCounters->error                       = emac_tx_counters.transmission_errors ;
}
EXPORT_SYMBOL (mac_hwapi_get_tx_counters);

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_init_emac	                         	                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - 	init the emac to well known state                      	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	initialized the emac port				  								  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                                                            */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_init_emac(rdpa_emac emacNum)
{
	/*do nothing in lilac*/
}

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_loopback	                         	                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - 	init the emac to well known state                      	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	initialized the emac port				  								  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                                                            */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_get_loopback(rdpa_emac emacNum,MAC_LPBK *loopback)
{
	E_DRV_MAC_ENABLE macloopback;
	pi_bl_drv_mac_get_loopback(emacNum,&macloopback);
	*loopback = (macloopback == DRV_MAC_ENABLE) ? MAC_LPBK_LOCAL : MAC_LPBK_NONE;
}

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_get_loopback	                         	                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - 	init the emac to well known state                      	  */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* 	initialized the emac port				  								  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - emac Port index           	                                  */
/*                                                                            */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_set_loopback(rdpa_emac emacNum,MAC_LPBK loopback)
{
	E_DRV_MAC_ENABLE macLoopback;
	macLoopback =  (loopback == MAC_LPBK_LOCAL) ? DRV_MAC_ENABLE : DRV_MAC_DISABLE;
	pi_bl_drv_mac_set_loopback(emacNum,macLoopback);

}

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   mac_hwapi_modify_flow_control_pause_pkt_addr                             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   MAC Driver - Modify the Flow Control pause pkt source address            */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function modifies the flow control pause pkt source address         */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   emacNum - EMAC id (0-5)                                                  */
/*                                                                            */
/*   mac - Flow Control mac address                                           */
/*                                                                            */
/* Output:   N/A                                                              */
/*                                                                            */
/******************************************************************************/
void mac_hwapi_modify_flow_control_pause_pkt_addr ( rdpa_emac emacNum,
    bdmf_mac_t mac)
{
    DRV_MAC_ADDRESS drv_mac;

    memcpy(&drv_mac, &mac, sizeof(drv_mac));
    pi_bl_drv_mac_modify_flow_control_pause_pkt_addr(emacNum, drv_mac);
}
EXPORT_SYMBOL (mac_hwapi_modify_flow_control_pause_pkt_addr);
