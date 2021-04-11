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
#include "bl_lilac_rdd.h"
#include "rdp_ih.h"
#include "rdp_bbh.h"
#include "rdp_drv_bpm.h"
#include "rdp_drv_sbpm.h"
#include "rdp_drv_bbh.h"
#include "rdp_drv_ih.h"
#include "bl_rnr_sw_fw_defs.h"
#include "bl_rnr_ih_defs.h"

#define RDD_ERROR uint32_t

#define CS_MIN_ETH_PACKET_SIZE					64
#define CS_MAX_ETH_PACKET_SIZE					1536
#define CS_EMAC_PAUSE_TIMER_INTERVAL_GMII		0x1FFFFF
#define CS_RDD_ETH_TX_QUEUE_PACKET_THRESHOLD	256
#define CS_RDD_CPU_RX_QUEUE_SIZE				32

/* this delay is used in TM init sequence */
#define CS_TM_INIT_SEQUENCE_DELAY_IN_MILLISECONDS	1000
/* runner frequency: 800MHz */

/******************************************************************************/
/* Definitions of IH configuration                                            */
/******************************************************************************/
#define CS_IH_HEADER_LENGTH_MINIMUM                              64
/* same for both runners */
#define CS_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS                   32 
/* the following definitions are valid only when CS_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS is 32 */
#define CS_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS_IHD_ENUM          (DRV_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS_32)
#define CS_IH_RUNNERS_HIGH_CONGESTION_THRESHOLD                  22
#define CS_IH_RUNNERS_EXCLUSIVE_CONGESTION_THRESHOLD             28
/* load balancing threshold is set to maximal number of buffers, i.e. load balancing is disabled. */
#define CS_IH_RUNNERS_LOAD_BALANCING_THRESHOLD                   (CS_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS)
/* load balancing hysteresis is set to maximal number of buffers, i.e. load balancing is disabled. */
#define CS_IH_RUNNERS_LOAD_BALANCING_HYSTERESIS                  (CS_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS)
/* size of ingress queue of each one of the EMACs which function as LAN */
#define CS_IH_INGRESS_QUEUE_SIZE_LAN_EMACS                        2
/* size of ingress queue of the WAN port */
#define CS_IH_INGRESS_QUEUE_SIZE_WAN                              4
/* size of ingress queue of each runner */                                
#define CS_IH_INGRESS_QUEUE_SIZE_RUNNERS                          1
/* priority of ingress queue of each one of the EMACs which function as LAN */
#define CS_IH_INGRESS_QUEUE_PRIORITY_LAN_EMACS                    1
/* priority of ingress queue of the WAN port */
#define CS_IH_INGRESS_QUEUE_PRIORITY_WAN                          2
/* priority of ingress queue of each runner */                        
#define CS_IH_INGRESS_QUEUE_PRIORITY_RUNNERS                      0
/* weight of ingress queue of each one of the EMACs which function as LAN */
#define CS_IH_INGRESS_QUEUE_WEIGHT_LAN_EMACS                      1
/* weight of ingress queue of the WAN port */
#define CS_IH_INGRESS_QUEUE_WEIGHT_WAN                            1
/* weight of ingress queue of each runner */                       
#define CS_IH_INGRESS_QUEUE_WEIGHT_RUNNERS                        1
/* congestion threshold of ingress queue of each one of the EMACs which function as LAN */
#define CS_IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_LAN_EMACS        64
/* congestion threshold of ingress queue of the WAN port */
#define CS_IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_WAN              64
/* congestion threshold of ingress queue of each runner */              
#define CS_IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_RUNNERS          64


/******************************************************************************/
/* There are 8 IH ingress queues. This enumeration defines, for each ingress  */
/* queue, which physical source port it belongs to.                           */
/******************************************************************************/
typedef enum
{
    CS_IH_INGRESS_QUEUE_0_ETH0 ,
    CS_IH_INGRESS_QUEUE_1_ETH1 ,
    CS_IH_INGRESS_QUEUE_2_ETH2 ,
    CS_IH_INGRESS_QUEUE_3_ETH3 ,
    CS_IH_INGRESS_QUEUE_4_ETH4 ,
    CS_IH_INGRESS_QUEUE_5_GPON_OR_ETH5 ,
    CS_IH_INGRESS_QUEUE_6_RUNNER_A ,
    CS_IH_INGRESS_QUEUE_7_RUNNER_B ,

    CS_NUMBER_OF_IH_INGRESS_QUEUES
}
IH_INGRESS_QUEUE_INDEX_DTS ;

/* Modules which we configure via CR (Clocks&Reset) */
typedef enum
{
    /* common bit for 4 modules */
    CS_CR_MODULE_SBPM_BPM_SDMA_DMA ,
    CS_CR_MODULE_IH ,
    CS_CR_MODULE_EMAC ,
    CS_CR_MODULE_GPON ,
    CS_CR_MODULE_RUNNER ,
    CS_CR_MODULE_TM,
    CS_CR_MODULE_NIF_SERDES ,
    CS_CR_MODULE_GPON_SERDES ,
    CS_CR_MODULE_DLL_SEL
}
CR_MODULE_DTS ;

/* route addresses (we should NOT use the reggae defaults) */
#define CS_IH_ETH0_ROUTE_ADDRESS      0x1C
#define CS_IH_ETH1_ROUTE_ADDRESS      0xC
#define CS_IH_ETH2_ROUTE_ADDRESS      0x14
#define CS_IH_ETH3_ROUTE_ADDRESS      0x8
#define CS_IH_ETH4_ROUTE_ADDRESS      0x10
#define CS_IH_GPON_ROUTE_ADDRESS      0
#define CS_IH_RUNNER_A_ROUTE_ADDRESS  0x3
#define CS_IH_RUNNER_B_ROUTE_ADDRESS  0x2
/* we use one basic class */
#define CS_IH_BASIC_CLASS_INDEX	      0


/******************************************************************************/
/* Definitions of BBH configuration values                                    */
/******************************************************************************/

/* route addresses (for both TX & RX) */
static const uint8_t gs_bl_bbh_route_address_dma [ DRV_BBH_NUMBER_OF_PORTS ] =
{ 0xC , 0xD , 0xE , 0xD , 0xE , 0xF , 0xF} ;
static const uint8_t gs_bl_bbh_route_address_bpm [ DRV_BBH_NUMBER_OF_PORTS ] =
{ 0x14 , 0x15 , 0x16 , 0x15 , 0x16 , 0x17 , 0x17} ;
static const uint8_t gs_bl_bbh_route_address_sdma [ DRV_BBH_NUMBER_OF_PORTS ] =
{ 0x1C , 0x1D , 0x1E , 0x1D , 0x1E , 0x1F , 0x1F} ;
static const uint8_t gs_bl_bbh_route_address_sbpm [ DRV_BBH_NUMBER_OF_PORTS ] =
{ 0x34 , 0x35 , 0x36 , 0x35 , 0x36 , 0x37 , 0x37} ;
static const uint8_t gs_bl_bbh_route_address_runner_0 [ DRV_BBH_NUMBER_OF_PORTS ] =
{ 0x0 , 0x1 , 0x2 , 0x1 , 0x2 , 0x3 , 0x3} ;
static const uint8_t gs_bl_bbh_route_address_runner_1 [ DRV_BBH_NUMBER_OF_PORTS ] =
{0x8 , 0x9 , 0xA , 0x9 , 0xA , 0xB , 0xB} ;
static const uint8_t gs_bl_bbh_route_address_ih [ DRV_BBH_NUMBER_OF_PORTS ] =
{ 0x18 , 0x19 , 0x1A , 0x11 , 0x12 , 0x13 , 0x13} ;
/* same values for DMA & SDMA */
static const uint8_t gs_bl_bbh_dma_and_sdma_read_requests_fifo_base_address [ DRV_BBH_NUMBER_OF_PORTS ] =
{ 0x0 , 0x8 , 0x10 , 0x18 , 0x20 , 0x28 , 0x28} ;

#define MS_BYTE_TO_8_BYTE_RESOLUTION( address )  ( ( address ) >> 3 )

static const uint16_t gs_bl_bbh_rx_pd_fifo_base_address_normal_queue_in_8_byte [ DRV_BBH_NUMBER_OF_PORTS ] =
{
    MS_BYTE_TO_8_BYTE_RESOLUTION( ETH0_RX_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( ETH1_RX_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( ETH2_RX_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( ETH3_RX_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( ETH4_RX_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( GPON_RX_NORMAL_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( GPON_RX_NORMAL_DESCRIPTORS_ADDRESS ) 
} ;

static const uint16_t gs_bl_bbh_rx_pd_fifo_base_address_direct_queue_in_8_byte [ DRV_BBH_NUMBER_OF_PORTS ] =
{
    MS_BYTE_TO_8_BYTE_RESOLUTION( ETH0_RX_DIRECT_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( ETH1_RX_DIRECT_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( ETH2_RX_DIRECT_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( ETH3_RX_DIRECT_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( ETH4_RX_DIRECT_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( GPON_RX_DIRECT_DESCRIPTORS_ADDRESS ) ,
    MS_BYTE_TO_8_BYTE_RESOLUTION( GPON_RX_DIRECT_DESCRIPTORS_ADDRESS )
} ;

/* same values for both runners */
static const uint8_t gs_bl_bbh_rx_runner_task_normal_queue [ DRV_BBH_NUMBER_OF_PORTS ] =
{
    LAN0_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ,
    LAN1_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ,
    LAN2_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ,
    LAN3_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ,
    LAN4_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ,
    WAN_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ,
    WAN_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER
} ;

static const uint8_t gs_bl_bbh_rx_runner_task_direct_queue [ DRV_BBH_NUMBER_OF_PORTS ] =
{
    LOCAL_SWITCHING_ETH0_THREAD_NUMBER ,
    LOCAL_SWITCHING_ETH1_THREAD_NUMBER ,
    LOCAL_SWITCHING_ETH2_THREAD_NUMBER ,
    LOCAL_SWITCHING_ETH3_THREAD_NUMBER ,
    LOCAL_SWITCHING_ETH4_THREAD_NUMBER ,
    WAN_DIRECT_THREAD_NUMBER ,
    WAN_DIRECT_THREAD_NUMBER
} ;



static const uint8_t gs_bl_bbh_rx_dma_data_fifo_base_address [ DRV_BBH_NUMBER_OF_PORTS ] =
{ 0x0 , 0x9 , 0x12 , 0x1B , 0x24 , 0x2D , 0x2D} ;
static const uint8_t gs_bl_bbh_rx_sdma_data_fifo_base_address [ DRV_BBH_NUMBER_OF_PORTS ] =
{0x0 , 0x5 , 0xA , 0xF , 0x14 , 0x19 , 0x19} ;
static const uint8_t gs_bl_bbh_rx_dma_chunk_descriptor_fifo_base_address [ DRV_BBH_NUMBER_OF_PORTS ] =
{0x0 , 0x9 , 0x12 , 0x1B , 0x24 , 0x2D , 0x2D} ;
static const uint8_t gs_bl_bbh_rx_sdma_chunk_descriptor_fifo_base_address [ DRV_BBH_NUMBER_OF_PORTS ] =
{ 0x0 , 0x5 , 0xA , 0xF , 0x14 , 0x19 , 0x19} ;
/* multicast header size */
#define CS_BBH_MULTICAST_HEADER_SIZE                      64
/* PD FIFO size of EMAC, when MDU mode is disabled */
#define CS_BBH_TX_EMAC_PD_FIFO_SIZE_MDU_MODE_DISABLED     8
/* PD FIFO size of EMAC, when MDU mode is enabled */
#define CS_BBH_TX_EMAC_PD_FIFO_SIZE_MDU_MODE_ENABLED      4
/* PD FIFOs sizes */
#define CS_BBH_RX_PD_FIFO_SIZE_NORMAL_QUEUE               (CS_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS)
#define CS_BBH_RX_PD_FIFO_SIZE_DIRECT_QUEUE               (CS_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS)
/* DMA data and chunk descriptor FIFOs size, for LAN port */
#define CS_BBH_RX_DMA_FIFOS_SIZE_LAN                      9
/* DMA exclusive threshold, for LAN port */         
#define CS_BBH_RX_DMA_EXCLUSIVE_THRESHOLD_LAN             7
/* SDMA data and chunk descriptor FIFOs size, for LAN port */
#define CS_BBH_RX_SDMA_FIFOS_SIZE_LAN                     5
/* SDMA exclusive threshold, for LAN port */         
#define CS_BBH_RX_SDMA_EXCLUSIVE_THRESHOLD_LAN            4
/* this feature is not supported currently */
#define CS_BBH_RX_FLOWS_32_255_GROUP_DIVIDER              32


/******************************************************************************/
/* Definitions of BPM configuration values                                    */
/******************************************************************************/
#define CS_BPM_GLOBAL_HYSTERESIS                       0
#define CS_BPM_GLOBAL_THRESHOLD                        ( DRV_BPM_GLOBAL_THRESHOLD_7_5K )
#define CS_BPM_USER_GROUP_THRESHOLD                    7680
#define CS_BPM_USER_GROUP_HYSTERESIS                   0
#define CS_BPM_USER_GROUP_EXCLUSIVE_THRESHOLD          7680
#define CS_BPM_USER_GROUP_EXCLUSIVE_HYSTERESIS         0


/******************************************************************************/
/* Definitions of SBPM configuration values                                   */
/******************************************************************************/
#define CS_SBPM_BASE_ADDRESS                               0
#define CS_SBPM_LIST_SIZE                                  800
#define CS_SBPM_GLOBAL_HYSTERESIS                       0
#define CS_SBPM_GLOBAL_THRESHOLD                        800
#define CS_SBPM_USER_GROUP_THRESHOLD                    800
#define CS_SBPM_USER_GROUP_HYSTERESIS                   0
#define CS_SBPM_USER_GROUP_EXCLUSIVE_THRESHOLD          800
#define CS_SBPM_USER_GROUP_EXCLUSIVE_HYSTERESIS         0

/******************************************************************************/
/*                                                                            */
/* Macros definitions                                                         */
/*                                                                            */
/******************************************************************************/
/* sets bit #i of a given number to a given value */
#define MS_SET_BIT_I( number , i , bit_value )   ( ( number ) &= ( ~ ( 1 << i ) )  , ( number ) |= ( bit_value << i ) )

/******************************************************************************/
/*                                                                            */
/* Global variables definitions                                               */
/*                                                                            */
/******************************************************************************/
/* this array holds, per BBH port, the ingress buffers bitmask of this port
   (totally there are 16 ingress buffers) */
static uint16_t gs_bbh_ih_ingress_buffers_bitmask [ DRV_BBH_NUMBER_OF_PORTS ] ;

static	uint8_t* 	 extra_ddr_pool_ptr = NULL;
static	uint8_t* 	 ddr_pool_ptr; 
static	unsigned long	 ddr_pool_size; 

/* FW binaries */
extern uint32_t firmware_uboot_binary_A[] ;
extern uint32_t firmware_uboot_binary_B[] ;
extern uint32_t firmware_uboot_binary_C[] ;
extern uint32_t firmware_uboot_binary_D[] ;

static void initRunnerDataPath(void)
{
	bl_lilac_cr_device_enable_clk ( BL_LILAC_CR_CLUSTER_1, 				BL_LILAC_CR_ON);
	bl_lilac_cr_device_enable_clk ( BL_LILAC_CR_TM , 					BL_LILAC_CR_ON);

	bl_lilac_cr_device_reset(BL_LILAC_CR_RNR_SUB , 						BL_LILAC_CR_ON);
	bl_lilac_cr_device_reset(BL_LILAC_CR_RNR_1_DDR , 					BL_LILAC_CR_ON);
	bl_lilac_cr_device_reset(BL_LILAC_CR_RNR_1 , 						BL_LILAC_CR_ON);
	bl_lilac_cr_device_reset(BL_LILAC_CR_RNR_0_DDR , 					BL_LILAC_CR_ON);
	bl_lilac_cr_device_reset(BL_LILAC_CR_RNR_0 , 						BL_LILAC_CR_ON);
	bl_lilac_cr_device_reset(BL_LILAC_CR_PKT_SRAM_OCP_C, 				BL_LILAC_CR_ON);
	bl_lilac_cr_device_reset(BL_LILAC_CR_PKT_SRAM_OCP_D, 				BL_LILAC_CR_ON);
	bl_lilac_cr_device_reset(BL_LILAC_CR_DMA_DDR , 						BL_LILAC_CR_ON);
	bl_lilac_cr_device_reset(BL_LILAC_CR_GENERAL_MAIN_BB_RNR_BB_MAIN, 	BL_LILAC_CR_ON);
	bl_lilac_cr_device_reset(BL_LILAC_CR_IH_RNR , 						BL_LILAC_CR_ON);
	udelay(100);
}
static LILAC_BDPI_ERRORS f_map_emac_id_to_rdd ( DRV_MAC_NUMBER xi_emac_id , RDD_EMAC_ID * xo_rdd_emac_id )
{
    switch ( xi_emac_id )
    {
    case DRV_MAC_EMAC_0:
        *xo_rdd_emac_id = RDD_EMAC_ID_0 ;
        return ( BDPI_NO_ERROR);
    case DRV_MAC_EMAC_1:
        *xo_rdd_emac_id = RDD_EMAC_ID_1 ;
        return ( BDPI_NO_ERROR);
    case DRV_MAC_EMAC_2:
        *xo_rdd_emac_id = RDD_EMAC_ID_2 ;
        return ( BDPI_NO_ERROR);
    case DRV_MAC_EMAC_3:
        *xo_rdd_emac_id = RDD_EMAC_ID_3 ;
        return ( BDPI_NO_ERROR);
    case DRV_MAC_EMAC_4:
        *xo_rdd_emac_id = RDD_EMAC_ID_4 ;
        return ( BDPI_NO_ERROR);
    default:
        return BDPI_ERROR_EMAC_ID_OUT_OF_RANGE;
    }
}
static LILAC_BDPI_ERRORS f_configure_runner ( DRV_MAC_NUMBER xi_emac_id )
{
    LILAC_BDPI_ERRORS error ;
    RDD_ERROR rdd_error ;
    RDD_EMAC_ID rdd_emac_id ;
    /* init the firmware data structures */
   	rdd_error = bl_rdd_data_structures_init ( ( uint8_t * ) ddr_pool_ptr, 
                                              ( uint8_t * ) extra_ddr_pool_ptr,
                                              RDD_MAC_TABLE_SIZE_256,
                                              RDD_MAC_TABLE_SIZE_1024,
                                              RDD_MAC_TABLE_SIZE_1024);
   	if ( rdd_error != RDD_OK )
        return ( BDPI_ERROR_RDD_ERROR); 

    error = f_map_emac_id_to_rdd ( xi_emac_id , & rdd_emac_id);
    if ( error != BDPI_NO_ERROR )
        return ( error);

    /* init Eth TX queue */
    rdd_error = bl_rdd_eth_tx_queue_config ( rdd_emac_id ,
                                             RDD_QUEUE_0 ,
                                             CS_RDD_ETH_TX_QUEUE_PACKET_THRESHOLD);
    if ( rdd_error != RDD_OK )
        return( BDPI_ERROR_RDD_ERROR);
   
    /* configure CPU RX queue.
       queue id and interrupt id must be the same (RDD demand) */
    rdd_error = bl_rdd_cpu_rx_queue_config ( RDD_CPU_RX_QUEUE_0 ,
                                             CS_RDD_CPU_RX_QUEUE_SIZE ,
                                             RDD_CPU_RX_QUEUE_0);
    if ( rdd_error != RDD_OK )
        return( BDPI_ERROR_RDD_ERROR);

    /* enable MAC lookup for the relevant bridge port */
    rdd_error = bl_rdd_sa_mac_lookup_config ( RDD_LAN0_BRIDGE_PORT + xi_emac_id ,
                                              RDD_DA_MAC_LOOKUP_ENABLE);
    if ( rdd_error != RDD_OK )
        return( BDPI_ERROR_RDD_ERROR);

    /* configure unknown DA command to trap to cpu */
    rdd_error = bl_rdd_unknown_sa_mac_cmd_config ( RDD_LAN0_BRIDGE_PORT + xi_emac_id ,
                                                   RDD_UNKNOWN_MAC_CMD_CPU_TRAP);
    if ( rdd_error != RDD_OK )
        return( BDPI_ERROR_RDD_ERROR);

    /* configure CPU_RX_REASON_UNKNOWN_DA to cpu queue 0 */
    rdd_error = bl_rdd_cpu_reason_to_cpu_rx_queue ( RDD_CPU_RX_REASON_UNKNOWN_SA ,
                                                    RDD_CPU_RX_QUEUE_0,
                                                    RDD_UPSTREAM);
    if ( rdd_error != RDD_OK )
        return( BDPI_ERROR_RDD_ERROR);

    return ( BDPI_NO_ERROR);
}


/* this function configures, per BBH port, the ingress buffers bitmask of this port (to be configured in BBH) */
static void f_update_bbh_ih_ingress_buffers_bitmask( DRV_BBH_PORT_INDEX xi_bbh_port_index ,
                                                     uint8_t xi_base_location ,
                                                     uint8_t xi_queue_size )
{
    uint16_t bitmask = 0 ;
    uint8_t index ;

    /* set '1's according to xi_queue_size  */
    for ( index = 0 ; index < xi_queue_size ; ++ index )
    {
        MS_SET_BIT_I( bitmask , index , 1);
    }

    /* do shifting according to xi_base_location */
    bitmask <<= xi_base_location ;

    /* update in database */
    gs_bbh_ih_ingress_buffers_bitmask [ xi_bbh_port_index ] = bitmask ;
}


/* this function configures all 8 ingress queues in IH */
static DRV_IH_ERROR f_ih_configure_ingress_queues ( void )
{
    DRV_IH_ERROR ih_error ; 
    IH_INGRESS_QUEUE_INDEX_DTS ingress_queue_index ;
    DRV_IH_INGRESS_QUEUE_CONFIG ingress_queue_config ;
    uint8_t base_location = 0 ;


    /* queues of EMACS 0-4 */
    for ( ingress_queue_index = CS_IH_INGRESS_QUEUE_0_ETH0 ; ingress_queue_index <= CS_IH_INGRESS_QUEUE_4_ETH4 ; ++ ingress_queue_index )
    {
        ingress_queue_config.base_location = base_location ;
        ingress_queue_config.size = CS_IH_INGRESS_QUEUE_SIZE_LAN_EMACS ;
        ingress_queue_config.priority = CS_IH_INGRESS_QUEUE_PRIORITY_LAN_EMACS ;
        ingress_queue_config.weight = CS_IH_INGRESS_QUEUE_WEIGHT_LAN_EMACS ;
        ingress_queue_config.congestion_threshold = CS_IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_LAN_EMACS ;

        ih_error = fi_bl_drv_ih_configure_ingress_queue ( ingress_queue_index ,
                                                          & ingress_queue_config);
        if ( ih_error != DRV_IH_NO_ERROR )
            return ( ih_error);

        /* update the correspoding bitmask in database, to be configured in BBH */
        f_update_bbh_ih_ingress_buffers_bitmask( ( DRV_BBH_PORT_INDEX ) ingress_queue_index ,
                                                 ingress_queue_config.base_location ,
                                                 ingress_queue_config.size);

        base_location += ingress_queue_config.size ;
    }

    /* queues of EMAC 5 */
    ingress_queue_config.base_location = base_location ;
    ingress_queue_config.size = CS_IH_INGRESS_QUEUE_SIZE_WAN ;
    ingress_queue_config.priority = CS_IH_INGRESS_QUEUE_PRIORITY_WAN ;
    ingress_queue_config.weight = CS_IH_INGRESS_QUEUE_WEIGHT_WAN ;
    ingress_queue_config.congestion_threshold = CS_IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_WAN ;

    ih_error = fi_bl_drv_ih_configure_ingress_queue ( CS_IH_INGRESS_QUEUE_5_GPON_OR_ETH5 ,
                                                      & ingress_queue_config);
    if ( ih_error != DRV_IH_NO_ERROR )
        return ( ih_error);

    /* update the correspoding bitmask in database, to be configured in BBH */
    f_update_bbh_ih_ingress_buffers_bitmask( DRV_BBH_EMAC_5 ,
                                             ingress_queue_config.base_location ,
                                             ingress_queue_config.size);

    base_location += ingress_queue_config.size ;


    /* queues of runners */
    for ( ingress_queue_index = CS_IH_INGRESS_QUEUE_6_RUNNER_A ; ingress_queue_index <= CS_IH_INGRESS_QUEUE_7_RUNNER_B ; ++ ingress_queue_index )
    {
        ingress_queue_config.base_location = base_location ;
        ingress_queue_config.size = CS_IH_INGRESS_QUEUE_SIZE_RUNNERS ;
        ingress_queue_config.priority = CS_IH_INGRESS_QUEUE_PRIORITY_RUNNERS ;
        ingress_queue_config.weight = CS_IH_INGRESS_QUEUE_WEIGHT_RUNNERS ;
        ingress_queue_config.congestion_threshold = CS_IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_RUNNERS ;

        ih_error = fi_bl_drv_ih_configure_ingress_queue ( ingress_queue_index ,
                                                                & ingress_queue_config);
        if ( ih_error != DRV_IH_NO_ERROR )
            return ( ih_error);

        base_location += ingress_queue_config.size ;
    }

    /* check that we didn't overrun */
    if ( base_location > DRV_IH_INGRESS_QUEUES_ARRAY_SIZE )
    {
        /* sum of sizes exceeded the total array size */
        return ( DRV_IH_ERROR_INVALID_INGRESS_QUEUE_SIZE);
    }
    else
    {
        return ( DRV_IH_NO_ERROR);
    }
}


/* this function configures a basic IH class */
static DRV_IH_ERROR f_ih_configure_basic_class ( void )
{
    DRV_IH_ERROR ih_error ;
    DRV_IH_CLASS_CONFIG class_config ;

    class_config.class_search_1 = DRV_IH_CLASS_SEARCH_DISABLED ;
    class_config.class_search_2 = DRV_IH_CLASS_SEARCH_DISABLED ;
    class_config.class_search_3 = DRV_IH_CLASS_SEARCH_DISABLED ;
    class_config.class_search_4 = DRV_IH_CLASS_SEARCH_DISABLED ;
    class_config.destination_port_extraction = DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_OPERATION_DISABLED ;
    class_config.drop_on_miss = DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_OPERATION_DISABLED ;
    /* just an initialization. no DSCP to TCI table is used */
    class_config.dscp_to_tci_table_index = 0;
    class_config.direct_mode_default = 0 ;
    class_config.direct_mode_override = 0 ;
    class_config.target_memory_default = DRV_IH_TARGET_MEMORY_DDR ;
    class_config.target_memory_override = 0 ;
    class_config.ingress_qos_default = DRV_IH_INGRESS_QOS_LOW ;
    class_config.ingress_qos_override = DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_OPERATION_DISABLED ;
    class_config.target_runner_default = LILAC_DRV_RDD_IH_US_DEFAULT_RUNNER ;
    class_config.target_runner_override_in_direct_mode = 0 ;
    class_config.target_runner_for_direct_mode = LILAC_DRV_RDD_IH_US_DEFAULT_RUNNER ;
    class_config.load_balancing_enable = 0 ;
    class_config.preference_load_balancing_enable = 0 ;

    ih_error = fi_bl_drv_ih_configure_class ( CS_IH_BASIC_CLASS_INDEX ,
                                              & class_config);
    if ( ih_error != DRV_IH_NO_ERROR )
        return ( ih_error);

    return ( DRV_IH_NO_ERROR);
}

/* initialization of IH block */
static DRV_IH_ERROR f_ih_init ( void )
{
    DRV_IH_ERROR ih_error ; 
    DRV_IH_GENERAL_CONFIG ih_general_config ;
    DRV_IH_PACKET_HEADER_OFFSETS packet_header_offsets ;
    DRV_IH_RUNNER_BUFFERS_CONFIG runner_buffers_config ;
    DRV_IH_ROUTE_ADDRESSES xi_route_addresses ;
    DRV_IH_LOGICAL_PORTS_CONFIG logical_ports_config ;
    DRV_IH_SOURCE_PORT_TO_INGRESS_QUEUE_MAPPING source_port_to_ingress_queue_mapping ;

    ih_general_config.runner_a_ih_response_address = MS_BYTE_TO_8_BYTE_RESOLUTION( LILAC_DRV_RDD_IH_RUNNER_0_IH_RESPONSE_ADDRESS);
    ih_general_config.runner_b_ih_response_address = MS_BYTE_TO_8_BYTE_RESOLUTION( LILAC_DRV_RDD_IH_RUNNER_1_IH_RESPONSE_ADDRESS);
    ih_general_config.runner_a_ih_congestion_report_address = MS_BYTE_TO_8_BYTE_RESOLUTION( LILAC_DRV_RDD_IH_RUNNER_0_IH_CONGESTION_REPORT_ADDRESS);
    ih_general_config.runner_b_ih_congestion_report_address = MS_BYTE_TO_8_BYTE_RESOLUTION( LILAC_DRV_RDD_IH_RUNNER_1_IH_CONGESTION_REPORT_ADDRESS);

    ih_general_config.runner_a_ih_congestion_report_enable = LILAC_DRV_RDD_IH_RUNNER_0_IH_CONGESTION_REPORT_ENABLE ;
    ih_general_config.runner_b_ih_congestion_report_enable = LILAC_DRV_RDD_IH_RUNNER_1_IH_CONGESTION_REPORT_ENABLE ;
    ih_general_config.lut_searches_enable_in_direct_mode = 1 ;
    ih_general_config.sn_stamping_enable_in_direct_mode = 1 ;
    ih_general_config.header_length_minimum = CS_IH_HEADER_LENGTH_MINIMUM ;
    ih_general_config.congestion_discard_disable = 0 ;
    ih_general_config.cam_search_enable_upon_invalid_lut_entry = LILAC_DRV_RDD_IH_CAM_SEARCH_ENABLE_UPON_INVALID_LUT_ENTRY ;
    
    ih_error = fi_bl_drv_ih_set_general_configuration ( & ih_general_config);
    if ( ih_error != DRV_IH_NO_ERROR )
        return ( ih_error);

    /* packet header offsets configuration */
    packet_header_offsets.eth0_packet_header_offset = LILAC_DRV_RDD_IH_PACKET_HEADER_OFFSET ;
    packet_header_offsets.eth1_packet_header_offset = LILAC_DRV_RDD_IH_PACKET_HEADER_OFFSET ;
    packet_header_offsets.eth2_packet_header_offset = LILAC_DRV_RDD_IH_PACKET_HEADER_OFFSET ;
    packet_header_offsets.eth3_packet_header_offset = LILAC_DRV_RDD_IH_PACKET_HEADER_OFFSET ;
    packet_header_offsets.eth4_packet_header_offset = LILAC_DRV_RDD_IH_PACKET_HEADER_OFFSET ;
    packet_header_offsets.gpon_packet_header_offset = LILAC_DRV_RDD_IH_PACKET_HEADER_OFFSET ;
    packet_header_offsets.runner_a_packet_header_offset = LILAC_DRV_RDD_IH_PACKET_HEADER_OFFSET ;
    packet_header_offsets.runner_b_packet_header_offset = LILAC_DRV_RDD_IH_PACKET_HEADER_OFFSET ;

    ih_error = fi_bl_drv_ih_set_packet_header_offsets ( & packet_header_offsets);
    if ( ih_error != DRV_IH_NO_ERROR )
        return ( ih_error);

    /* Runner Buffers configuration */
    runner_buffers_config.runner_a_ih_managed_rb_base_address = MS_BYTE_TO_8_BYTE_RESOLUTION( LILAC_DRV_RDD_IH_RUNNER_0_IH_MANAGED_RB_BASE_ADDRESS);
    runner_buffers_config.runner_b_ih_managed_rb_base_address = MS_BYTE_TO_8_BYTE_RESOLUTION( LILAC_DRV_RDD_IH_RUNNER_1_IH_MANAGED_RB_BASE_ADDRESS);
    runner_buffers_config.runner_a_runner_managed_rb_base_address = MS_BYTE_TO_8_BYTE_RESOLUTION( LILAC_DRV_RDD_IH_RUNNER_0_RUNNER_MANAGED_RB_BASE_ADDRESS);
    runner_buffers_config.runner_b_runner_managed_rb_base_address = MS_BYTE_TO_8_BYTE_RESOLUTION( LILAC_DRV_RDD_IH_RUNNER_1_RUNNER_MANAGED_RB_BASE_ADDRESS);
    
    runner_buffers_config.runner_a_maximal_number_of_buffers = CS_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS_IHD_ENUM ;
    runner_buffers_config.runner_b_maximal_number_of_buffers = CS_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS_IHD_ENUM ;

    ih_error = fi_bl_drv_ih_set_runner_buffers_configuration ( & runner_buffers_config);
    if ( ih_error != DRV_IH_NO_ERROR )
        return ( ih_error);

    xi_route_addresses.eth0_route_address = CS_IH_ETH0_ROUTE_ADDRESS ;
    xi_route_addresses.eth1_route_address = CS_IH_ETH1_ROUTE_ADDRESS ;
    xi_route_addresses.eth2_route_address = CS_IH_ETH2_ROUTE_ADDRESS ;
    xi_route_addresses.eth3_route_address = CS_IH_ETH3_ROUTE_ADDRESS ;
    xi_route_addresses.eth4_route_address = CS_IH_ETH4_ROUTE_ADDRESS ;
    xi_route_addresses.gpon_route_address = CS_IH_GPON_ROUTE_ADDRESS ;
    xi_route_addresses.runner_a_route_address = CS_IH_RUNNER_A_ROUTE_ADDRESS ;
    xi_route_addresses.runner_b_route_address = CS_IH_RUNNER_B_ROUTE_ADDRESS ;

    ih_error = fi_bl_drv_ih_set_route_addresses ( & xi_route_addresses);
    if ( ih_error != DRV_IH_NO_ERROR )
        return ( ih_error);

    /* logical ports configuration */
    logical_ports_config.eth0_config.parsing_layer_depth = IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_ETH0_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE ;
    logical_ports_config.eth0_config.proprietary_tag_size = DRV_IH_PROPRIETARY_TAG_SIZE_0 ;

    logical_ports_config.eth1_config.parsing_layer_depth = IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_ETH1_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE ;
    logical_ports_config.eth1_config.proprietary_tag_size = DRV_IH_PROPRIETARY_TAG_SIZE_0 ;

    logical_ports_config.eth2_config.parsing_layer_depth = IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_ETH2_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE ;
    logical_ports_config.eth2_config.proprietary_tag_size = DRV_IH_PROPRIETARY_TAG_SIZE_0 ;

    logical_ports_config.eth3_config.parsing_layer_depth = IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_ETH3_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE ;
    logical_ports_config.eth3_config.proprietary_tag_size = DRV_IH_PROPRIETARY_TAG_SIZE_0 ;

    logical_ports_config.eth4_config.parsing_layer_depth = IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_ETH4_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE ;
    logical_ports_config.eth4_config.proprietary_tag_size = DRV_IH_PROPRIETARY_TAG_SIZE_0 ;

    logical_ports_config.gpon_config.parsing_layer_depth = IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_GPON_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE ;
    logical_ports_config.gpon_config.proprietary_tag_size = DRV_IH_PROPRIETARY_TAG_SIZE_0 ;

    logical_ports_config.runner_a_config.parsing_layer_depth = IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_RNRA_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE ;
    logical_ports_config.runner_a_config.proprietary_tag_size = DRV_IH_PROPRIETARY_TAG_SIZE_0 ;

    logical_ports_config.runner_b_config.parsing_layer_depth = IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_RNRB_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE ;
    logical_ports_config.runner_b_config.proprietary_tag_size = DRV_IH_PROPRIETARY_TAG_SIZE_0 ;

    logical_ports_config.pcie0_config.parsing_layer_depth = IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_PCIE0_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE ;
    logical_ports_config.pcie0_config.proprietary_tag_size = DRV_IH_PROPRIETARY_TAG_SIZE_0 ;

    logical_ports_config.pcie1_config.parsing_layer_depth = IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_PCIE1_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE ;
    logical_ports_config.pcie1_config.proprietary_tag_size = DRV_IH_PROPRIETARY_TAG_SIZE_0 ;

    ih_error = fi_bl_drv_ih_set_logical_ports_configuration ( & logical_ports_config);
    if ( ih_error != DRV_IH_NO_ERROR )
        return ( ih_error);

    /* source port to ingress queue mapping */
    source_port_to_ingress_queue_mapping.eth0_ingress_queue = CS_IH_INGRESS_QUEUE_0_ETH0 ;
    source_port_to_ingress_queue_mapping.eth1_ingress_queue = CS_IH_INGRESS_QUEUE_1_ETH1 ;
    source_port_to_ingress_queue_mapping.eth2_ingress_queue = CS_IH_INGRESS_QUEUE_2_ETH2 ;
    source_port_to_ingress_queue_mapping.eth3_ingress_queue = CS_IH_INGRESS_QUEUE_3_ETH3 ;
    source_port_to_ingress_queue_mapping.eth4_ingress_queue = CS_IH_INGRESS_QUEUE_4_ETH4 ;
    source_port_to_ingress_queue_mapping.gpon_ingress_queue = CS_IH_INGRESS_QUEUE_5_GPON_OR_ETH5 ;
    source_port_to_ingress_queue_mapping.runner_a_ingress_queue = CS_IH_INGRESS_QUEUE_6_RUNNER_A ;
    source_port_to_ingress_queue_mapping.runner_b_ingress_queue = CS_IH_INGRESS_QUEUE_7_RUNNER_B ;

    ih_error = fi_bl_drv_ih_set_source_port_to_ingress_queue_mapping ( & source_port_to_ingress_queue_mapping);
    if ( ih_error != DRV_IH_NO_ERROR )
        return ( ih_error);

    /* ingress queues configuration */

    ih_error = f_ih_configure_ingress_queues ();
    if ( ih_error != DRV_IH_NO_ERROR )
        return ( ih_error);

    /* configure classes */
    ih_error = f_ih_configure_basic_class ();
    if ( ih_error != DRV_IH_NO_ERROR )
        return ( ih_error);

    return ( DRV_IH_NO_ERROR);
}


static DRV_SBPM_ERROR f_sbpm_init ( void )
{
    DRV_SBPM_ERROR sbpm_error ;
    DRV_SBPM_GLOBAL_CONFIGURATION sbpm_global_configuration ;
    DRV_SBPM_USER_GROUPS_THRESHOLDS sbpm_ug_configuration ;
    uint32_t index ;

    sbpm_global_configuration.hysteresis = CS_SBPM_GLOBAL_HYSTERESIS ;
    sbpm_global_configuration.threshold = CS_SBPM_GLOBAL_THRESHOLD ;


    for ( index = 0 ; index < DRV_SBPM_NUMBER_OF_USER_GROUPS ; ++ index )
    {
        sbpm_ug_configuration.ug_arr [ index ].threshold = CS_SBPM_USER_GROUP_THRESHOLD ;
        sbpm_ug_configuration.ug_arr [ index ].hysteresis = CS_SBPM_USER_GROUP_HYSTERESIS ;
        sbpm_ug_configuration.ug_arr [ index ].exclusive_threshold = CS_SBPM_USER_GROUP_EXCLUSIVE_THRESHOLD ;
        sbpm_ug_configuration.ug_arr [ index ].exclusive_hysteresis = CS_SBPM_USER_GROUP_EXCLUSIVE_HYSTERESIS ;
    }

    sbpm_error = fi_bl_drv_sbpm_init ( CS_SBPM_BASE_ADDRESS ,
                                       CS_SBPM_LIST_SIZE ,
									   SBPM_REPLY_ADDRESS ,
                                       & sbpm_global_configuration ,
                                       & sbpm_ug_configuration);

    return ( sbpm_error);
}


static DRV_BPM_ERROR f_bpm_init ( void )
{
    DRV_BPM_ERROR bpm_error ;
    DRV_BPM_GLOBAL_CONFIGURATION bpm_global_configuration ;
    DRV_BPM_USER_GROUPS_THRESHOLDS bpm_ug_configuration ;
    uint32_t index ;

    bpm_global_configuration.hysteresis = CS_BPM_GLOBAL_HYSTERESIS ;
    bpm_global_configuration.threshold = CS_BPM_GLOBAL_THRESHOLD ;


    for ( index = 0 ; index < DRV_BPM_NUMBER_OF_USER_GROUPS ; ++ index )
    {
        bpm_ug_configuration.ug_arr [ index ].threshold = CS_BPM_USER_GROUP_THRESHOLD ;
        bpm_ug_configuration.ug_arr [ index ].hysteresis = CS_BPM_USER_GROUP_HYSTERESIS ;
        bpm_ug_configuration.ug_arr [ index ].exclusive_threshold = CS_BPM_USER_GROUP_EXCLUSIVE_THRESHOLD ;
        bpm_ug_configuration.ug_arr [ index ].exclusive_hysteresis = CS_BPM_USER_GROUP_EXCLUSIVE_HYSTERESIS ;
    }

    bpm_error = fi_bl_drv_bpm_init ( & bpm_global_configuration ,
                                     & bpm_ug_configuration , 
									 BPM_REPLY_ADDRESS );

    return ( bpm_error);
}

/* this function returns, for each EMAC port, the corresponding runner TX task number.
   the EMAC port should be a LAN EMAC. 
   in case of error, a negative number is returned. */
static int8_t f_get_runner_tx_task_number_of_lan_emac ( DRV_MAC_NUMBER xi_emac_id )
{
    switch ( xi_emac_id )
    {
    case DRV_MAC_EMAC_0:
        return ( ETH0_TX_THREAD_NUMBER);
    case DRV_MAC_EMAC_1:
        return ( ETH1_TX_THREAD_NUMBER);
    case DRV_MAC_EMAC_2:
        return ( ETH2_TX_THREAD_NUMBER);
    case DRV_MAC_EMAC_3:
        return ( ETH3_TX_THREAD_NUMBER);
    case DRV_MAC_EMAC_4:
        return ( ETH4_TX_THREAD_NUMBER);
    /* TBD!!! not yet supported */
    case DRV_MAC_EMAC_5:
        return ( -1);
    /* we are not supposed to get here */
    default:
        return ( -1);
    }
}


/* initialize the BBH of EMAC port */
static LILAC_BDPI_ERRORS f_initialize_bbh_of_emac_port ( DRV_BBH_PORT_INDEX xi_port_index )
{
    LILAC_BDPI_ERRORS error ;
    DRV_BBH_ERROR bbh_error ;
    RDD_ERROR rdd_error ;
    DRV_BBH_TX_CONFIGURATION bbh_tx_configuration ;
    DRV_BBH_RX_CONFIGURATION bbh_rx_configuration ;
    DRV_BBH_PER_FLOW_CONFIGURATION per_flow_configuration ;
    int8_t tx_task_number ;
    DRV_MAC_NUMBER emac_id = ( DRV_MAC_NUMBER ) xi_port_index ;
    RDD_EMAC_ID rdd_emac_id ;
    uint16_t mdu_mode_read_pointer_address_in_byte ;
    uint32_t temp;
#if !defined _CFE_
    unsigned long tmp;
#endif

    /*** BBH TX ***/
    bbh_tx_configuration.dma_route_address = gs_bl_bbh_route_address_dma [ xi_port_index ] ;
    bbh_tx_configuration.bpm_route_address = gs_bl_bbh_route_address_bpm [ xi_port_index ] ;
    bbh_tx_configuration.sdma_route_address = gs_bl_bbh_route_address_sdma [ xi_port_index ] ;
    bbh_tx_configuration.sbpm_route_address = gs_bl_bbh_route_address_sbpm [ xi_port_index ] ;
    /* runner 1 is the one which handles TX */
    bbh_tx_configuration.runner_route_address = gs_bl_bbh_route_address_runner_1 [ xi_port_index ] ;
    bbh_tx_configuration.ddr_buffer_size = DRV_BBH_DDR_BUFFER_SIZE_2_KB ;
    bbh_tx_configuration.payload_offset_resolution = DRV_BBH_PAYLOAD_OFFSET_RESOLUTION_2_B ;
    bbh_tx_configuration.multicast_header_size = CS_BBH_MULTICAST_HEADER_SIZE ;

#if defined _CFE_
    bbh_tx_configuration.multicast_headers_base_address_in_byte = K0_TO_K1((uint32_t)KMALLOC(0x400000, 0x100000));
#else
    bl_lilac_mem_rsrv_get_by_name(TM_MC_BASE_ADDR_STR,(void**)&(bbh_tx_configuration.multicast_headers_base_address_in_byte) ,&tmp );
#endif
    bbh_tx_configuration.multicast_headers_base_address_in_byte &= 0x1FFFFFFF;
    tx_task_number = f_get_runner_tx_task_number_of_lan_emac ( emac_id);
    if ( tx_task_number < 0 )
        return ( BDPI_ERROR_BBH_INITIALIZATION_ERROR); 

    bbh_tx_configuration.task_0 = tx_task_number ;

    /* other task numbers are irrelevant (relevant for GPON only) */
    bbh_tx_configuration.task_1 = 0 ;
    bbh_tx_configuration.task_2 = 0 ;
    bbh_tx_configuration.task_3 = 0 ;
    bbh_tx_configuration.task_4 = 0 ;
    bbh_tx_configuration.task_5 = 0 ;
    bbh_tx_configuration.task_6 = 0 ;
    bbh_tx_configuration.task_7 = 0 ;
    bbh_tx_configuration.task_8_39 = 0 ;

    bbh_tx_configuration.mdu_mode_enable = 1 ;

    error = f_map_emac_id_to_rdd ( emac_id , & rdd_emac_id);
    if ( error != BDPI_NO_ERROR )
        return ( error);

    rdd_error = bl_rdd_get_mdu_mode_pointer ( rdd_emac_id ,
                                                    & mdu_mode_read_pointer_address_in_byte);
    if ( rdd_error != RDD_OK )
        return ( BDPI_ERROR_RDD_ERROR);

    temp = mdu_mode_read_pointer_address_in_byte;
    if (mdu_mode_read_pointer_address_in_byte >= 0x8000)
    {
        temp = mdu_mode_read_pointer_address_in_byte;
        temp |= 0x10000;
    }
    bbh_tx_configuration.mdu_mode_read_pointer_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION( temp);
    /* For Ethernet port working in MDU mode, PD FIFO size should be configured to 4 (and not 8). */
    if ( bbh_tx_configuration.mdu_mode_enable == 1 )
    {
        bbh_tx_configuration.pd_fifo_size_0 = CS_BBH_TX_EMAC_PD_FIFO_SIZE_MDU_MODE_ENABLED ;
    }
    else
    {
        bbh_tx_configuration.pd_fifo_size_0 = CS_BBH_TX_EMAC_PD_FIFO_SIZE_MDU_MODE_DISABLED ;
    }

    /* other FIFOs are irrelevant (relevant for GPON only) */
    bbh_tx_configuration.pd_fifo_size_1 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE ;
    bbh_tx_configuration.pd_fifo_size_2 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE ;
    bbh_tx_configuration.pd_fifo_size_3 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE ;
    bbh_tx_configuration.pd_fifo_size_4 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE ;
    bbh_tx_configuration.pd_fifo_size_5 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE ;
    bbh_tx_configuration.pd_fifo_size_6 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE ;
    bbh_tx_configuration.pd_fifo_size_7 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE ;
    bbh_tx_configuration.pd_fifo_size_8_15 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE ;
    bbh_tx_configuration.pd_fifo_size_16_23 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE ;
    bbh_tx_configuration.pd_fifo_size_24_31 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE ;
    bbh_tx_configuration.pd_fifo_size_32_39 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE ;

    bbh_tx_configuration.pd_fifo_base_0 = 0 ;

    /* other FIFOs are irrelevant (relevant for GPON only) */
    bbh_tx_configuration.pd_fifo_base_1 = 0 ;
    bbh_tx_configuration.pd_fifo_base_2 = 0 ;
    bbh_tx_configuration.pd_fifo_base_3 = 0 ;
    bbh_tx_configuration.pd_fifo_base_4 = 0 ;
    bbh_tx_configuration.pd_fifo_base_5 = 0 ;
    bbh_tx_configuration.pd_fifo_base_6 = 0 ;
    bbh_tx_configuration.pd_fifo_base_7 = 0 ;
    bbh_tx_configuration.pd_fifo_base_8_15 = 0 ;
    bbh_tx_configuration.pd_fifo_base_16_23 = 0 ;
    bbh_tx_configuration.pd_fifo_base_24_31 = 0 ;
    bbh_tx_configuration.pd_fifo_base_32_39 = 0 ;

    /* pd_prefetch_byte_threshold feature is irrelevant in EMAC (since there is only one FIFO) */
    bbh_tx_configuration.pd_prefetch_byte_threshold_enable = 0 ;
    bbh_tx_configuration.pd_prefetch_byte_threshold_0_in_32_byte = 0 ;
    bbh_tx_configuration.pd_prefetch_byte_threshold_1_in_32_byte = 0 ;
    bbh_tx_configuration.pd_prefetch_byte_threshold_2_in_32_byte = 0 ;
    bbh_tx_configuration.pd_prefetch_byte_threshold_3_in_32_byte = 0 ;
    bbh_tx_configuration.pd_prefetch_byte_threshold_4_in_32_byte = 0 ;
    bbh_tx_configuration.pd_prefetch_byte_threshold_5_in_32_byte = 0 ;
    bbh_tx_configuration.pd_prefetch_byte_threshold_6_in_32_byte = 0 ;
    bbh_tx_configuration.pd_prefetch_byte_threshold_7_in_32_byte = 0 ;
    bbh_tx_configuration.pd_prefetch_byte_threshold_8_39_in_32_byte = 0 ;

    bbh_tx_configuration.dma_read_requests_fifo_base_address = gs_bl_bbh_dma_and_sdma_read_requests_fifo_base_address [ xi_port_index ] ;
    bbh_tx_configuration.dma_read_requests_maximal_number = BBH_TX_CONFIGURATIONS_DMACFG_TX_MAXREQ_MAX_VALUE_VALUE_RESET_VALUE ;
    bbh_tx_configuration.sdma_read_requests_fifo_base_address = gs_bl_bbh_dma_and_sdma_read_requests_fifo_base_address [ xi_port_index ] ;
    bbh_tx_configuration.sdma_read_requests_maximal_number = BBH_TX_CONFIGURATIONS_SDMACFG_TX_MAXREQ_MAX_VALUE_VALUE_RESET_VALUE ;

    /* irrelevant in EMAC case */
    bbh_tx_configuration.tcont_address_in_8_byte = 0 ;

    /* TBD!!! - not supported yet */
    bbh_tx_configuration.skb_address = 0 ;
    bbh_tx_configuration.ddr_tm_base_address = (uint32_t)ddr_pool_ptr;
    bbh_tx_configuration.ddr_tm_base_address &= 0xFFFFFFF;
    bbh_tx_configuration.emac_1588_enable = 0 ;

    bbh_error = fi_bl_drv_bbh_tx_set_configuration ( xi_port_index ,
                                                     & bbh_tx_configuration);
    if ( bbh_error != DRV_BBH_NO_ERROR )
        return ( BDPI_ERROR_BBH_INITIALIZATION_ERROR);

    /*** BBH RX ***/

    bbh_rx_configuration.dma_route_address = gs_bl_bbh_route_address_dma [ xi_port_index ] ;
    bbh_rx_configuration.bpm_route_address = gs_bl_bbh_route_address_bpm [ xi_port_index ] ;
    bbh_rx_configuration.sdma_route_address = gs_bl_bbh_route_address_sdma [ xi_port_index ] ;
    bbh_rx_configuration.sbpm_route_address = gs_bl_bbh_route_address_sbpm [ xi_port_index ] ;
    bbh_rx_configuration.runner_0_route_address = gs_bl_bbh_route_address_runner_0 [ xi_port_index ] ;
    bbh_rx_configuration.runner_1_route_address = gs_bl_bbh_route_address_runner_1 [ xi_port_index ] ;
    bbh_rx_configuration.ih_route_address = gs_bl_bbh_route_address_ih [ xi_port_index ] ;

    bbh_rx_configuration.ddr_buffer_size = DRV_BBH_DDR_BUFFER_SIZE_2_KB ;
    bbh_rx_configuration.ddr_tm_base_address = (uint32_t)ddr_pool_ptr;
    bbh_rx_configuration.ddr_tm_base_address &= 0xFFFFFFF;
    bbh_rx_configuration.pd_fifo_base_address_normal_queue_in_8_byte = gs_bl_bbh_rx_pd_fifo_base_address_normal_queue_in_8_byte [ xi_port_index ] ;
    bbh_rx_configuration.pd_fifo_base_address_direct_queue_in_8_byte = gs_bl_bbh_rx_pd_fifo_base_address_direct_queue_in_8_byte [ xi_port_index ] ;
    bbh_rx_configuration.pd_fifo_size_normal_queue = CS_BBH_RX_PD_FIFO_SIZE_NORMAL_QUEUE ;
    bbh_rx_configuration.pd_fifo_size_direct_queue = CS_BBH_RX_PD_FIFO_SIZE_DIRECT_QUEUE ;
    bbh_rx_configuration.runner_0_task_normal_queue = gs_bl_bbh_rx_runner_task_normal_queue [ xi_port_index ] ;
    bbh_rx_configuration.runner_0_task_direct_queue = gs_bl_bbh_rx_runner_task_direct_queue [ xi_port_index ] ;
    bbh_rx_configuration.runner_1_task_normal_queue = gs_bl_bbh_rx_runner_task_normal_queue [ xi_port_index ] ;
    bbh_rx_configuration.runner_1_task_direct_queue = gs_bl_bbh_rx_runner_task_direct_queue [ xi_port_index ] ;
    bbh_rx_configuration.dma_data_fifo_base_address = gs_bl_bbh_rx_dma_data_fifo_base_address [ xi_port_index ] ;
    bbh_rx_configuration.dma_chunk_descriptor_fifo_base_address = gs_bl_bbh_rx_dma_chunk_descriptor_fifo_base_address [ xi_port_index ] ;
    bbh_rx_configuration.sdma_data_fifo_base_address = gs_bl_bbh_rx_sdma_data_fifo_base_address [ xi_port_index ] ;
    bbh_rx_configuration.sdma_chunk_descriptor_fifo_base_address = gs_bl_bbh_rx_sdma_chunk_descriptor_fifo_base_address [ xi_port_index ] ;

    bbh_rx_configuration.dma_data_and_chunk_descriptor_fifos_size = CS_BBH_RX_DMA_FIFOS_SIZE_LAN ;
    bbh_rx_configuration.dma_exclusive_threshold = CS_BBH_RX_DMA_EXCLUSIVE_THRESHOLD_LAN ;
    bbh_rx_configuration.sdma_data_and_chunk_descriptor_fifos_size = CS_BBH_RX_SDMA_FIFOS_SIZE_LAN ;
    bbh_rx_configuration.sdma_exclusive_threshold = CS_BBH_RX_SDMA_EXCLUSIVE_THRESHOLD_LAN ;

    bbh_rx_configuration.minimum_packet_size_0 = CS_MIN_ETH_PACKET_SIZE ;
    /* minimum_packet_size 1-3 are not in use */
    bbh_rx_configuration.minimum_packet_size_1 = CS_MIN_ETH_PACKET_SIZE ;
    bbh_rx_configuration.minimum_packet_size_2 = CS_MIN_ETH_PACKET_SIZE ;
    bbh_rx_configuration.minimum_packet_size_3 = CS_MIN_ETH_PACKET_SIZE ;

    bbh_rx_configuration.maximum_packet_size_0 = CS_MAX_ETH_PACKET_SIZE ;
    /* maximum_packet_size 1-3 are not in use */
    bbh_rx_configuration.maximum_packet_size_1 = CS_MAX_ETH_PACKET_SIZE ;
    bbh_rx_configuration.maximum_packet_size_2 = CS_MAX_ETH_PACKET_SIZE ;
    bbh_rx_configuration.maximum_packet_size_3 = CS_MAX_ETH_PACKET_SIZE ;

    bbh_rx_configuration.ih_ingress_buffers_bitmask = gs_bbh_ih_ingress_buffers_bitmask [ xi_port_index ] ;
    bbh_rx_configuration.packet_header_offset = LILAC_DRV_RDD_IH_PACKET_HEADER_OFFSET ;
    bbh_rx_configuration.reassembly_offset_in_8_byte = BBH_RX_GENERAL_CONFIGURATION_REASSEMBLYOFFSET_OFFSET_DEFAULT_VALUE ;

    bbh_rx_configuration.flow_control_triggers_bitmask = 0 ;
    bbh_rx_configuration.drop_triggers_bitmask = DRV_BBH_RX_DROP_TRIGGER_BPM_IS_IN_EXCLUSIVE_STATE | DRV_BBH_RX_DROP_TRIGGER_SBPM_IS_IN_EXCLUSIVE_STATE ;

    /* following configuration is irrelevant in EMAC case */
    bbh_rx_configuration.flows_32_255_group_divider = CS_BBH_RX_FLOWS_32_255_GROUP_DIVIDER ;
    bbh_rx_configuration.ploam_default_ih_class = 0 ;
    bbh_rx_configuration.ploam_ih_class_override = 0 ;


    bbh_error = fi_bl_drv_bbh_rx_set_configuration ( xi_port_index ,
                                                     & bbh_rx_configuration);
    if ( bbh_error != DRV_BBH_NO_ERROR )
        return ( BDPI_ERROR_BBH_INITIALIZATION_ERROR);

    /* bbh_rx_set_per_flow_configuration */
    per_flow_configuration.minimum_packet_size_selection = 0 ;
    per_flow_configuration.maximum_packet_size_selection = 0 ;
    per_flow_configuration.ih_class_override = 0 ;
    per_flow_configuration.default_ih_class = CS_IH_BASIC_CLASS_INDEX ;

    /* in EMAC, only flow 0 is relevant */
    bbh_error = fi_bl_drv_bbh_rx_set_per_flow_configuration ( xi_port_index ,
                                                              0 ,
                                                              & per_flow_configuration);
    if ( bbh_error != DRV_BBH_NO_ERROR )
        return ( BDPI_ERROR_BBH_INITIALIZATION_ERROR);

    return ( BDPI_NO_ERROR);
}


static LILAC_BDPI_ERRORS f_map_emac_to_bpm_sbpm_sp ( DRV_MAC_NUMBER xi_emac_id , 
                                               DRV_BPM_SP_USR  * const xo_bpm_sp ) 
{
    switch ( xi_emac_id )
    {
    case DRV_MAC_EMAC_0:
        * xo_bpm_sp = DRV_BPM_SP_EMAC0 ;
        break;
    case DRV_MAC_EMAC_1:
        * xo_bpm_sp = DRV_BPM_SP_EMAC1 ;
        break;
    case DRV_MAC_EMAC_2:
        * xo_bpm_sp = DRV_BPM_SP_EMAC2 ;
        break;
    case DRV_MAC_EMAC_3:
        * xo_bpm_sp = DRV_BPM_SP_EMAC3 ;
        break;
    case DRV_MAC_EMAC_4:
        * xo_bpm_sp = DRV_BPM_SP_EMAC4 ;
        break;
    case DRV_MAC_EMAC_5: 
        * xo_bpm_sp = DRV_BPM_SP_GPON;
        break;
    default: 
        return ( BDPI_ERROR_EMAC_ID_OUT_OF_RANGE);
    }

    return ( BDPI_NO_ERROR );
}

/* This function enable or disable the EMAC interface */
static LILAC_BDPI_ERRORS bl_ctrl_emac(DRV_MAC_NUMBER xi_emac_id, int32_t xi_enable, ETH_MODE xi_emacs_group_mode)
{
    LILAC_BDPI_ERRORS error ;
    DRV_BPM_ERROR bpm_error ;
    DRV_BPM_SP_USR drv_bpm_sp = DRV_BPM_SP_EMAC0 ;

    /* Check if refering to a non-functional EMAC ID */
    if ( ( xi_emacs_group_mode == ETH_MODE_HISGMII ) && 
       ( ( xi_emac_id == DRV_MAC_EMAC_2 ) || ( xi_emac_id == DRV_MAC_EMAC_3 ) ) )
         return ( BDPI_ERROR_EMAC_IS_NOT_FUNCTIONAL);

    /* BPM and SBPM configuration */
    error = f_map_emac_to_bpm_sbpm_sp ( xi_emac_id , & drv_bpm_sp );
    if ( error != BDPI_NO_ERROR )
        return ( error );

    bpm_error = fi_bl_drv_bpm_sp_enable ( drv_bpm_sp , xi_enable );
    if ( bpm_error != DRV_BPM_ERROR_NO_ERROR ) 
        return ( BDPI_ERROR_BPM_INITIALIZATION_ERROR);

    pi_bl_drv_mac_enable ( xi_emac_id , xi_enable);

    return ( BDPI_NO_ERROR);
}

LILAC_BDPI_ERRORS fi_basic_runner_data_path_init ( DRV_MAC_NUMBER 	xi_emac_id,
												   ETH_MODE 		xi_emacs_group_mode,
												   uint8_t* 	 	xi_ddr_pool_ptr,
												   unsigned long	xi_ddr_pool_size,
												   uint8_t* 		xi_extra_ddr_pool_ptr)
{
    LILAC_BDPI_ERRORS	error;
    DRV_IH_ERROR		ih_error;
    DRV_BPM_ERROR		bpm_error;
    DRV_SBPM_ERROR		sbpm_error;
    RDD_ERROR			rdd_error;
    int					rdd_version;

	extra_ddr_pool_ptr = xi_extra_ddr_pool_ptr;
	ddr_pool_ptr = xi_ddr_pool_ptr; 
	ddr_pool_size = xi_ddr_pool_size; 
    /* enable clock of TM (including runners) */
	initRunnerDataPath();

    rdd_error = bl_rdd_runner_frequency_set(get_lilac_rnr_clock_rate());
    if( rdd_error != RDD_OK )
       return ( BDPI_ERROR_RDD_ERROR); 

    /* zero Runner memories (data, program and context) */
    rdd_error = bl_rdd_init ();
    if( rdd_error != RDD_OK )
		return ( BDPI_ERROR_RDD_ERROR); 

    if(ddr_pool_size < 0x1400000) // ddr_pool_size should be at least 20M 
    {
    	printk("BDPI ERROR: TM Reserved DDR size (0x%X) smaller than min needed (0x%X)\n", (unsigned int)ddr_pool_size, 0x1400000);   
        return BDPI_ERROR_MEMORY_ERROR;
    }

    if((unsigned int)ddr_pool_ptr & 0x1FFFFF) // ddr_pool_ptr should be aligned to 2MByte 
    {
      	printk("BDPI ERROR: TM Reserved DDR not aligned to 2MByte\n");   
      	return BDPI_ERROR_MEMORY_ERROR;
    }
	
    /* load runner firmware */
    rdd_error = bl_rdd_load_microcode( (uint8_t*)firmware_uboot_binary_A ,
                                       (uint8_t*)firmware_uboot_binary_B ,
                                       (uint8_t*)firmware_uboot_binary_C ,
                                       (uint8_t*)firmware_uboot_binary_D
                                     );
    if(rdd_error != RDD_OK)
        return BDPI_ERROR_RDD_ERROR; 

    /* Configure Runner */
    error = f_configure_runner ( xi_emac_id);
    if ( error != BDPI_NO_ERROR )
        return ( error);

    /* Configure IH  */
    ih_error = f_ih_init ();
    if ( ih_error != DRV_IH_NO_ERROR )
        return ( BDPI_ERROR_IH_INITIALIZATION_ERROR);

    /* Initialize SBPM */
    sbpm_error = f_sbpm_init ();
    if ( sbpm_error != DRV_SBPM_ERROR_NO_ERROR )
        return ( BDPI_ERROR_SBPM_INITIALIZATION_ERROR);
    
    /* Initialize BPM */
    bpm_error = f_bpm_init ();
    if ( bpm_error != DRV_BPM_ERROR_NO_ERROR )
        return ( BDPI_ERROR_BPM_INITIALIZATION_ERROR);

    /* initialize BBH of relevant port */
    error = f_initialize_bbh_of_emac_port ( ( DRV_BBH_PORT_INDEX ) xi_emac_id);
    if ( error != BDPI_NO_ERROR )
        return ( error);

    /* Enable all runner processors */    
    rdd_error = bl_rdd_runner_enable () ;
    if ( rdd_error != RDD_OK )
        return ( BDPI_ERROR_RDD_ERROR);

    /* configure EMACs (including BPM & SBPM Source port enable) and EMAC enable */
    error = bl_ctrl_emac(xi_emac_id, 1, xi_emacs_group_mode);
    if ( error != BDPI_NO_ERROR )
        return ( error);
    
	bl_rdd_get_rdd_version((uint32_t*)&rdd_version);
	
    printk("NET: RDD version: %d.%d.%d.%d\n",
	            (rdd_version >> 24) & 0xff,
	            (rdd_version >> 16) & 0xff,
	            (rdd_version >> 8 ) & 0xff,
	                    rdd_version & 0xff );

    return ( BDPI_NO_ERROR);
}
EXPORT_SYMBOL(fi_basic_runner_data_path_init);

