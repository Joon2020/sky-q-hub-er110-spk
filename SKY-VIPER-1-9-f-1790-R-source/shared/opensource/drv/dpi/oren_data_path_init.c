/*
   Copyright (c) 2013 Broadcom Corporation
   All Rights Reserved

    <:label-BRCM:2013:DUAL/GPL:standard
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License, version 2, as published by
    the Free Software Foundation (the "GPL").
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    
    A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
    writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
    
    :>
*/
/******************************************************************************/
/*                                                                            */
/* File Description:                                                          */
/*                                                                            */
/* This file contains the implementation for Broadcom's 6838 Data path		  */
/* initialization sequence											          */
/*                                                                            */
/******************************************************************************/

/*****************************************************************************/
/*                                                                           */
/* Include files                                                             */
/*                                                                           */
/*****************************************************************************/
//#define RDD_BASIC
#ifndef OREN
#define OREN
#endif


#include "oren_data_path_init.h"
#include "pmc_drv.h"
#include "clk_rst.h"
#include "rdpa_types.h"
#include "rdpa_config.h"
#include "rdp_drv_bpm.h"
#include "rdp_drv_sbpm.h"
#include "rdp_drv_ih.h"
#include "rdp_drv_bbh.h"
#include "rdp_dma.h"
#include "rdd_ih_defs.h"
#include "rdd.h"
#include "rdd_tm.h"
#include "phys_common_drv.h"
#include "hwapi_mac.h"
#include "boardparms.h"
#include "shared_utils.h"

#ifndef RDD_BASIC
#include "rdpa_platform.h"
void f_initialize_bbh_of_gpon_port(void);
void f_initialize_bbh_of_epon_port(void);
#endif


extern int BcmMemReserveGetByName(char *name, void **addr, uint32_t *size);
static  S_DPI_CFG	DpiBasicConfig = {WAN_TYPE_NONE,1536,0,0,0,0};
static	int			RdpBlockUnreset = 0;

#define TM_BASE_ADDR_STR						"tm"
#define TM_MC_BASE_ADDR_STR						"mc"
#define	OREN_TM_DEF_DDR_SIZE 					0x1400000
#define OREN_TM_MC_DEF_DDR_SIZE					0x0400000
#define CS_RDD_ETH_TX_QUEUE_PACKET_THRESHOLD	256
#define CS_RDD_CPU_RX_QUEUE_SIZE				32

#ifdef _CFE_
#define VIRT_TO_PHYS(_addr)					K0_TO_PHYS((uint32_t)_addr)
#else
#define VIRT_TO_PHYS(_addr)					CPHYSADDR(_addr)
#endif
#define SHIFTL(_a) ( 1 << _a)
/****************************************************************************
 *
 * Defines
 *
 * *************************************************************************/
#define PBCM_RESET 0
#define PBCM_UNRESET 1

#define BPCM_SRESET_CNTL_REG 			8 //0x20 in words offset
/*BPCM-RDP*/
#define RDP_S_RST_UBUS_DBR              0// (dma ubus bridge)
#define RDP_S_RST_UBUS_RABR             1//(rnrA ubus bridge)
#define RDP_S_RST_UBUS_RBBR             2// (rnrB ubus bridge)
#define RDP_S_RST_UBUS_VPBR             3//(vpb ubus bridge)
#define RDP_S_RST_RNR_0                 4//
#define RDP_S_RST_RNR_1                 5//
#define RDP_S_RST_IPSEC_RNR             6//
#define RDP_S_RST_IPSEC_RNG             7//
#define RDP_S_RST_IPSEC_MAIN            8
#define RDP_S_RST_RNR_SUB               9
#define RDP_S_RST_IH_RNR               10
#define RDP_S_RST_BB_RNR               11
#define RDP_S_RST_GEN_MAIN             12// (main tm)
#define RDP_S_RST_VDSL                 13
#define RDP_S_RST_E0_MAIN              14// (mac0)
#define RDP_S_RST_E0_RST_L             15// (mac0)
#define RDP_S_RST_E1_MAIN              16// (mac1)
#define RDP_S_RST_E1_RST_L             17// (mac1)
#define RDP_S_RST_E2_MAIN              18// (mac2)
#define RDP_S_RST_E2_RST_L             19// (mac2)
#define RDP_S_RST_E3_MAIN              20// (mac3)
#define RDP_S_RST_E3_RST_L             21// (mac3)
#define RDP_S_RST_E4_MAIN              22// (mac4)
#define RDP_S_RST_E4_RST_L             23// (mac4)
/*BPCM-WAN*/
#define WAN_S_REST_WAN_MAIN	 			0// WAN main
#define WAN_S_REST_UNIMAC_TOP 			1
#define WAN_S_REST_UNIMAC_CORE 			2
#define WAN_S_REST_PCS					3
#define WAN_S_REST_AE_GB_RBC_250MHz		4
#define WAN_S_REST_AE_GB_RBC_125MHz		5
#define WAN_S_REST_AE_GB_TBC_250MHz		6
#define WAN_S_REST_AE_GB_TX_125MHz		7
#define WAN_S_REST_EPON_MAIN			8
#define WAN_S_REST_EPON_RX				9
#define WAN_S_REST_EPON_TX				10
#define WAN_S_REST_EPON_CORE			11
#define WAN_S_REST_EPON_GB_RBC_250MHz	12
#define WAN_S_REST_EPON_GB_RBC_125MHz	13
#define WAN_S_REST_EPON_GB_TBC_250MHz	14
#define WAN_S_REST_EPON_GB_TX_125MHz	15
#define WAN_S_REST_GPON_RX_MAIN			16
#define WAN_S_REST_GPON_RX				17
#define WAN_S_REST_GPON_TX				18
#define WAN_S_REST_GPON_TX_MAIN			19
#define WAN_S_REST_SATA_SRDS_RSTB		20
#define WAN_S_REST_SATA_SRDS_RSTB_PLL	21
#define WAN_S_REST_SATA_SRDS_RSTB_MDIO	22
#define WAN_S_REST_SATA_SRDS_RSTB_OOB0	23
#define WAN_S_REST_SATA_SRDS_RSTB_RX0	24
#define WAN_S_REST_SATA_SRDS_RSTB_TX0	25
#define WAN_S_REST_GPON_NCO				26
#define WAN_S_REST_APM_CLK_GEN_LOGIC	27

#define DEFAULT_RUNNER_FREQ				800
/* multicast header size */
#define BBH_MULTICAST_HEADER_SIZE_FOR_LAN_PORT 32
#define BBH_MULTICAST_HEADER_SIZE_FOR_WAN_PORT 96

/* PD FIFO size of EMAC, when MDU mode is disabled */
#define BBH_TX_EMAC_PD_FIFO_SIZE_MDU_MODE_DISABLED 8
/* PD FIFO size of EMAC, when MDU mode is enabled */
#define BBH_TX_EMAC_PD_FIFO_SIZE_MDU_MODE_ENABLED 4
/* DMA */
#define BBH_RX_DMA_FIFOS_SIZE_WAN_DMA 19
#define BBH_RX_DMA_FIFOS_SIZE_WAN_GPON 19
#define BBH_RX_DMA_FIFOS_SIZE_LAN_DMA 11
#define BBH_RX_DMA_FIFOS_SIZE_LAN_BBH 11
#define BBH_RX_DMA_EXCLUSIVE_THRESHOLD_WAN_DMA 17
#define BBH_RX_DMA_EXCLUSIVE_THRESHOLD_WAN_BBH 18
#define BBH_RX_DMA_EXCLUSIVE_THRESHOLD_LAN_DMA 7
#define BBH_RX_DMA_EXCLUSIVE_THRESHOLD_LAN_BBH 11
#define BBH_RX_DMA_FIFOS_SIZE_WAN_DMA_B0 32
#define BBH_RX_DMA_FIFOS_SIZE_WAN_GPON_B0 32
#define BBH_RX_DMA_FIFOS_SIZE_LAN_DMA_B0 13
#define BBH_RX_DMA_FIFOS_SIZE_RGMII_LAN_DMA_B0 12
#define BBH_RX_DMA_FIFOS_SIZE_LAN_BBH_B0 13
#define BBH_RX_DMA_FIFOS_SIZE_RGMII_LAN_BBH_B0 12
#define BBH_RX_DMA_EXCLUSIVE_THRESHOLD_WAN_DMA_B0 32
#define BBH_RX_DMA_EXCLUSIVE_THRESHOLD_WAN_BBH_B0 32
#define BBH_RX_DMA_EXCLUSIVE_THRESHOLD_LAN_DMA_B0 13
#define BBH_RX_DMA_EXCLUSIVE_THRESHOLD_RGMII_LAN_DMA_B0 13
#define BBH_RX_DMA_EXCLUSIVE_THRESHOLD_LAN_BBH_B0 13
#define BBH_RX_DMA_EXCLUSIVE_THRESHOLD_RGMII_LAN_BBH_B0 12

#define BBH_RX_SDMA_FIFOS_SIZE_WAN 7
#define BBH_RX_SDMA_FIFOS_SIZE_LAN 5
#define BBH_RX_SDMA_EXCLUSIVE_THRESHOLD_WAN 6
#define BBH_RX_SDMA_EXCLUSIVE_THRESHOLD_LAN 4
#define BBH_RX_DMA_TOTAL_NUMBER_OF_CHUNK_DESCRIPTORS 64
#define BBH_RX_SDMA_TOTAL_NUMBER_OF_CHUNK_DESCRIPTORS 32

#define MIN_ETH_PKT_SIZE 64
#define MIN_GPON_PKT_SIZE 60
#define BBH_RX_FLOWS_32_255_GROUP_DIVIDER 255
#define BBH_RX_ETH_MIN_PKT_SIZE_SELECTION_INDEX 0
#define BBH_RX_OMCI_MIN_PKT_SIZE_SELECTION_INDEX 1
#define BBH_RX_MAX_PKT_SIZE_SELECTION_INDEX 0
/* minimal OMCI packet size */
#define MIN_OMCI_PKT_SIZE 14

#define IH_HEADER_LENGTH_MIN 60
/* high congestion threshold of runner A (DS), for GPON mode */
#define IH_GPON_MODE_DS_RUNNER_A_HIGH_CONGESTION_THRESHOLD    24
/* exclusive congestion threshold of runner A (DS), for GPON mode */
#define IH_GPON_MODE_DS_RUNNER_A_EXCLUSIVE_CONGESTION_THRESHOLD 28
#define IH_PARSER_EXCEPTION_STATUS_BITS 0x47
#define IH_PARSER_AH_DETECTION 0x18000
/* PPP protocol code for IPv4 is configured at index 0 */
#define IH_PARSER_PPP_PROTOCOL_CODE_0_IPV4 0x21
/* PPP protocol code for IPv6 is configured at index 1 */
#define IH_PARSER_PPP_PROTOCOL_CODE_1_IPV6 0x57

#define IH_ETH0_ROUTE_ADDRESS 0x1C
#define IH_ETH1_ROUTE_ADDRESS 0xC
#define IH_ETH2_ROUTE_ADDRESS 0x14
#define IH_ETH3_ROUTE_ADDRESS 0x8
#define IH_ETH4_ROUTE_ADDRESS 0x10
#define IH_GPON_ROUTE_ADDRESS 0
#define IH_RUNNER_A_ROUTE_ADDRESS 0x3
#define IH_RUNNER_B_ROUTE_ADDRESS 0x2
#define IH_DA_FILTER_IPTV_IPV4 0
#define IH_DA_FILTER_IPTV_IPV6 1
#define TCP_CTRL_FLAG_RST 0x04
#define TCP_CTRL_FLAG_SYN 0x02
#define TCP_CTRL_FLAG_FIN 0x01
/* size of ingress queue of each one of the EMACs which function as LAN */
#define IH_INGRESS_QUEUE_SIZE_LAN_EMACS 2
/* size of ingress queue of the WAN port */
#define IH_INGRESS_QUEUE_SIZE_WAN 4
/* size of ingress queue of each runner */
#define IH_INGRESS_QUEUE_SIZE_RUNNERS 1
/* priority of ingress queue of each one of the EMACs which function as LAN */
#define IH_INGRESS_QUEUE_PRIORITY_LAN_EMACS 1
/* priority of ingress queue of the WAN port */
#define IH_INGRESS_QUEUE_PRIORITY_WAN 2
/* priority of ingress queue of each runner */
#define IH_INGRESS_QUEUE_PRIORITY_RUNNERS 0
/* weight of ingress queue of each one of the EMACs which function as LAN */
#define IH_INGRESS_QUEUE_WEIGHT_LAN_EMACS 1
/* weight of ingress queue of the WAN port */
#define IH_INGRESS_QUEUE_WEIGHT_WAN 1
/* weight of ingress queue of each runner */
#define IH_INGRESS_QUEUE_WEIGHT_RUNNERS 1
/* congestion threshold of ingress queue of each one of the EMACs which function as LAN */
#define IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_LAN_EMACS 65
/* congestion threshold of ingress queue of the WAN port */
#define IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_WAN 65
/* congestion threshold of ingress queue of each runner */
#define IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_RUNNERS 65

#define IH_IP_L4_FILTER_USER_DEFINED_0			0
#define IH_IP_L4_FILTER_USER_DEFINED_1			1
#define IH_IP_L4_FILTER_USER_DEFINED_2			2
#define IH_IP_L4_FILTER_USER_DEFINED_3			3

#define IH_L4_FILTER_DEF						0xff
/* we use one basic class */
#define IH_BASIC_CLASS_INDEX	      0
/* IH classifier indices for broadcast and multicast traffic iptv destination */
#define IH_CLASSIFIER_BCAST_IPTV 0
#define IH_CLASSIFIER_IGMP_IPTV 1
#define IH_CLASSIFIER_ICMPV6 2
#define IH_CLASSIFIER_IPTV  (IH_CLASSIFIER_ICMPV6 + 1)

#define MASK_IH_CLASS_KEY_L4 0x3c0
/* IPTV DA filter mask in IH */
#define IPTV_FILTER_MASK_DA 0x3800
#define IPTV_FILTER_MASK_BCAST  0x8000

/* default value for SBPM */
#define SBPM_DEFAULT_THRESHOLD 800
#define SBPM_DEFAULT_HYSTERESIS 0
#define BPM_DEFAULT_HYSTERESIS 64
#define SBPM_BASE_ADDRESS 0
#define SBPM_LIST_SIZE 0x3FF
#define BPM_CPU_NUMBER_OF_BUFFERS 1536
#define BPM_DEFAULT_DS_THRESHOLD 7680


/*GPON DEFS*/
/* size of each one of FIFOs 0-7 */
#define BBH_TX_GPON_PD_FIFO_SIZE_0_7 4
/* size of each one of FIFOs 8-15 */
#define BBH_TX_GPON_PD_FIFO_SIZE_8_15 3
#define BBH_TX_GPON_PD_FIFO_SIZE_16_23 3
#define BBH_TX_GPON_PD_FIFO_SIZE_24_31 3
#define BBH_TX_GPON_PD_FIFO_SIZE_32_39 3
#define BBH_TX_GPON_NUMBER_OF_TCONTS_IN_PD_FIFO_GROUP 8
#define BBH_TX_GPON_TOTAL_NUMBER_OF_PDS 128
#define BBH_TX_GPON_PD_FIFO_BASE_0 0
#define BBH_TX_GPON_PD_FIFO_BASE_1 (BBH_TX_GPON_PD_FIFO_BASE_0 + BBH_TX_GPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_GPON_PD_FIFO_BASE_2 (BBH_TX_GPON_PD_FIFO_BASE_1 + BBH_TX_GPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_GPON_PD_FIFO_BASE_3 (BBH_TX_GPON_PD_FIFO_BASE_2 + BBH_TX_GPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_GPON_PD_FIFO_BASE_4 (BBH_TX_GPON_PD_FIFO_BASE_3 + BBH_TX_GPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_GPON_PD_FIFO_BASE_5 (BBH_TX_GPON_PD_FIFO_BASE_4 + BBH_TX_GPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_GPON_PD_FIFO_BASE_6 (BBH_TX_GPON_PD_FIFO_BASE_5 + BBH_TX_GPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_GPON_PD_FIFO_BASE_7 (BBH_TX_GPON_PD_FIFO_BASE_6 + BBH_TX_GPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_GPON_PD_FIFO_BASE_8_15 (BBH_TX_GPON_PD_FIFO_BASE_7 + BBH_TX_GPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_GPON_PD_FIFO_BASE_16_23 \
    (BBH_TX_GPON_PD_FIFO_BASE_8_15 + (BBH_TX_GPON_NUMBER_OF_TCONTS_IN_PD_FIFO_GROUP * BBH_TX_GPON_PD_FIFO_SIZE_8_15))
#define BBH_TX_GPON_PD_FIFO_BASE_24_31 (BBH_TX_GPON_PD_FIFO_BASE_16_23 + \
    (BBH_TX_GPON_NUMBER_OF_TCONTS_IN_PD_FIFO_GROUP * BBH_TX_GPON_PD_FIFO_SIZE_16_23))
#define BBH_TX_GPON_PD_FIFO_BASE_32_39 (BBH_TX_GPON_PD_FIFO_BASE_24_31 + \
    (BBH_TX_GPON_NUMBER_OF_TCONTS_IN_PD_FIFO_GROUP * BBH_TX_GPON_PD_FIFO_SIZE_24_31))

/*EPON DEFS*/
#define BBH_TX_EPON_PD_FIFO_SIZE_0_7 8
/* size of each one of FIFOs 8-15 */
#define BBH_TX_EPON_PD_FIFO_SIZE_8_15 8
#define BBH_TX_EPON_PD_FIFO_SIZE_16_23 8
#define BBH_TX_EPON_PD_FIFO_SIZE_24_31 8
#define BBH_TX_EPON_PD_FIFO_SIZE_32_39 8
#define BBH_TX_EPON_NUMBER_OF_TCONTS_IN_PD_FIFO_GROUP 8
#define BBH_TX_EPON_TOTAL_NUMBER_OF_PDS 128
#define BBH_TX_EPON_PD_FIFO_BASE_0 0
#define BBH_TX_EPON_PD_FIFO_BASE_1 (BBH_TX_EPON_PD_FIFO_BASE_0 + BBH_TX_EPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_EPON_PD_FIFO_BASE_2 (BBH_TX_EPON_PD_FIFO_BASE_1 + BBH_TX_EPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_EPON_PD_FIFO_BASE_3 (BBH_TX_EPON_PD_FIFO_BASE_2 + BBH_TX_EPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_EPON_PD_FIFO_BASE_4 (BBH_TX_EPON_PD_FIFO_BASE_3 + BBH_TX_EPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_EPON_PD_FIFO_BASE_5 (BBH_TX_EPON_PD_FIFO_BASE_4 + BBH_TX_EPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_EPON_PD_FIFO_BASE_6 (BBH_TX_EPON_PD_FIFO_BASE_5 + BBH_TX_EPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_EPON_PD_FIFO_BASE_7 (BBH_TX_EPON_PD_FIFO_BASE_6 + BBH_TX_EPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_EPON_PD_FIFO_BASE_8_15 (BBH_TX_EPON_PD_FIFO_BASE_7 + BBH_TX_EPON_PD_FIFO_SIZE_0_7)
#define BBH_TX_EPON_PD_FIFO_BASE_16_23 \
    (BBH_TX_EPON_PD_FIFO_BASE_8_15 + (BBH_TX_EPON_NUMBER_OF_TCONTS_IN_PD_FIFO_GROUP * BBH_TX_EPON_PD_FIFO_SIZE_8_15))
#define BBH_TX_EPON_PD_FIFO_BASE_24_31 (BBH_TX_EPON_PD_FIFO_BASE_16_23 + \
    (BBH_TX_EPON_NUMBER_OF_TCONTS_IN_PD_FIFO_GROUP * BBH_TX_EPON_PD_FIFO_SIZE_16_23))
#define BBH_TX_EPON_PD_FIFO_BASE_32_39 0
#define BBH_TX_EPON_RUNNER_STS_ROUTE 0xB


/******************************************************************************/
/* The enumeration values are compatible with the values used by the HW.      */
/******************************************************************************/
typedef enum
{
    BB_MODULE_DMA,
    BB_MODULE_SDMA,

    BB_MODULE_NUM
}E_BB_MODULE;
typedef enum
{
    DMA_PERIPHERAL_EMAC_0 ,
    DMA_PERIPHERAL_EMAC_1 ,
    DMA_PERIPHERAL_EMAC_2 ,
    DMA_PERIPHERAL_EMAC_3 ,
    DMA_PERIPHERAL_EMAC_4 ,
    DMA_PERIPHERAL_EMAC_AE_GPON_EPON ,

    DMA_NUMBER_OF_PERIPHERALS
}
E_DMA_PERIPHERAL;
/******************************************************************************/
/*                                                                            */
/* Macros definitions                                                         */
/*                                                                            */
/******************************************************************************/
/* sets bit #i of a given number to a given value */
#define SET_BIT_I( number , i , bit_value )   ( ( number ) &= ( ~ ( 1 << i ) )  , ( number ) |= ( bit_value << i ) )
#define MS_BYTE_TO_8_BYTE_RESOLUTION(address)  ((address) >> 3)
#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))
#define BBH_PORT_IS_WAN(_wanType,_portIndex) (( _wanType == WAN_TYPE_RGMII && _portIndex == DRV_BBH_EMAC_4) || \
											  ( _wanType == WAN_TYPE_AE && _portIndex == DRV_BBH_EMAC_5) 	|| \
											  ( _wanType == WAN_TYPE_GPON && _portIndex == DRV_BBH_GPON) 	|| \
											  ( _wanType == WAN_TYPE_EPON && _portIndex == DRV_BBH_EPON) )

/*****************************************************************************/
/*                                                                           */
/* Local Defines                                                             */
/*                                                                           */
/*****************************************************************************/

/*DDR address for RDP usage*/
static void * ddr_tm_base_address;
static void * ddr_tm_base_address_phys;
static void * ddr_multicast_base_address;
static void * ddr_multicast_base_address_phys;

static S_DPI_CFG *pDpiCfg = NULL;
static uint32_t	initDone = 0;

/* route addresses (for both TX & RX) */
static const uint8_t bbh_route_address_dma[DRV_BBH_NUMBER_OF_PORTS]=
{
    0xC, 0xD, 0xE, 0xD, 0xE, 0xF, 0xF,0xF
};

static const uint8_t bbh_route_address_bpm[DRV_BBH_NUMBER_OF_PORTS]=
{
    0x14, 0x15, 0x16, 0x15, 0x16, 0x17, 0x17,0x17
};

static const uint8_t bbh_route_address_sdma[DRV_BBH_NUMBER_OF_PORTS]=
{
    0x1C, 0x1D, 0x1E, 0x1D, 0x1E, 0x1F, 0x1F,0
};

static const uint8_t bbh_route_address_sbpm[DRV_BBH_NUMBER_OF_PORTS]=
{
    0x34, 0x35, 0x36, 0x35, 0x36, 0x37, 0x37,0
};

static const uint8_t bbh_route_address_runner_0[DRV_BBH_NUMBER_OF_PORTS]=
{
    0x0, 0x1, 0x2, 0x1, 0x2, 0x3, 0x3,0x3
};

static const uint8_t bbh_route_address_runner_1[DRV_BBH_NUMBER_OF_PORTS]=
{
    0x8, 0x9, 0xA, 0x9, 0xA, 0xB, 0xB,0xB
};

static const uint8_t bbh_route_address_ih[DRV_BBH_NUMBER_OF_PORTS]=
{
    0x18, 0x19, 0x1A, 0x11, 0x12, 0x13, 0x13,0x13
};

/* same values for DMA & SDMA */
static const uint8_t bbh_dma_and_sdma_read_requests_fifo_base_address[DRV_BBH_NUMBER_OF_PORTS]=
{
    0x0, 0x8, 0x10, 0x18, 0x20, 0x28, 0x28,0x28
};
/* IH Classes indexes & configurations */
typedef struct
{
    uint8_t class_index;

    DRV_IH_CLASS_CONFIG class_config;
}
ih_class_cfg;
ih_class_cfg gs_ih_classes[] =
{
    {
        /* Wan Control */
        DRV_RDD_IH_CLASS_WAN_CONTROL_INDEX,
        {
            DRV_RDD_IH_CLASS_0_CLASS_SEARCH_1, /*class_search_1*/
            DRV_RDD_IH_CLASS_0_CLASS_SEARCH_2, /*class_search_2*/
            DRV_RDD_IH_CLASS_0_CLASS_SEARCH_3, /*class_search_3*/
            DRV_RDD_IH_CLASS_0_CLASS_SEARCH_4, /*class_search_4*/
            DRV_RDD_IH_CLASS_0_DESTINATION_PORT_EXTRACTION,  /*destination_port_extraction*/
            DRV_RDD_IH_CLASS_0_DROP_ON_MISS, /*drop_on_miss*/
            DRV_RDD_IH_CLASS_0_DSCP_TO_PBITS_TABLE_INDEX, /*dscp_to_tci_table_index*/
            DRV_RDD_IH_CLASS_0_DIRECT_MODE_DEFAULT, /*direct_mode_default*/
            DRV_RDD_IH_CLASS_0_DIRECT_MODE_OVERRIDE, /*direct_mode_override*/
            DRV_RDD_IH_CLASS_0_TARGET_MEMORY_DEFAULT, /*target_memory_default*/
            DRV_RDD_IH_CLASS_0_TARGET_MEMORY_OVERRIDE, /*target_memory_override*/
            DRV_RDD_IH_CLASS_0_INGRESS_QOS_DEFAULT, /*ingress_qos_default*/
            DRV_RDD_IH_CLASS_0_INGRESS_QOS_OVERRIDE, /*ingress_qos_override*/
            DRV_RDD_IH_CLASS_0_TARGET_RUNNER_DEFAULT, /*target_runner_default*/
            DRV_RDD_IH_CLASS_0_TARGET_RUNNER_OVERRIDE_IN_DIRECT_MODE, /*target_runner_override_in_direct_mode*/
            DRV_RDD_IH_CLASS_0_TARGET_RUNNER_FOR_DIRECT_MODE, /*target_runner_for_direct_mode*/
            DRV_RDD_IH_CLASS_0_LOAD_BALANCING_ENABLE, /*load_balancing_enable*/
            DRV_RDD_IH_CLASS_0_PREFERENCE_LOAD_BALANCING_ENABLE /*preference_load_balancing_enable*/
        }
    },
    {
        /* IPTV */
        DRV_RDD_IH_CLASS_IPTV_INDEX,
        {
            DRV_RDD_IH_CLASS_1_CLASS_SEARCH_1,
            DRV_IH_CLASS_SEARCH_DISABLED,   /* iptv_lookup_method_mac */
            DRV_RDD_IH_CLASS_1_CLASS_SEARCH_3,
            DRV_RDD_IH_CLASS_1_CLASS_SEARCH_4,
            DRV_RDD_IH_CLASS_1_DESTINATION_PORT_EXTRACTION,
            DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_BASED_ON_SEARCH1,
            DRV_RDD_IH_CLASS_1_DSCP_TO_PBITS_TABLE_INDEX,
            DRV_RDD_IH_CLASS_1_DIRECT_MODE_DEFAULT,
            DRV_RDD_IH_CLASS_1_DIRECT_MODE_OVERRIDE,
            DRV_RDD_IH_CLASS_1_TARGET_MEMORY_DEFAULT,
            DRV_RDD_IH_CLASS_1_TARGET_MEMORY_OVERRIDE,
            DRV_RDD_IH_CLASS_1_INGRESS_QOS_DEFAULT,
            DRV_RDD_IH_CLASS_1_INGRESS_QOS_OVERRIDE,
            DRV_RDD_IH_CLASS_1_TARGET_RUNNER_DEFAULT,
            DRV_RDD_IH_CLASS_1_TARGET_RUNNER_OVERRIDE_IN_DIRECT_MODE,
            DRV_RDD_IH_CLASS_1_TARGET_RUNNER_FOR_DIRECT_MODE,
            DRV_RDD_IH_CLASS_1_LOAD_BALANCING_ENABLE,
            DRV_RDD_IH_CLASS_1_PREFERENCE_LOAD_BALANCING_ENABLE
        }
    },
    {
        /* PCIE */
        DRV_RDD_IH_CLASS_PCI_INDEX,
        {
            DRV_RDD_IH_LOOKUP_TABLE_MAC_DA_INDEX, /* da_lookup_required */
            DRV_RDD_IH_LOOKUP_TABLE_MAC_SA_INDEX, /* sa_lookup_required */
            DRV_RDD_IH_CLASS_2_CLASS_SEARCH_3,
            DRV_IH_CLASS_SEARCH_LOOKUP_TABLE_4,
            DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_BASED_ON_SEARCH1, /* da_lookup_required */
            DRV_RDD_IH_CLASS_2_DROP_ON_MISS,
            DRV_RDD_IH_CLASS_2_DSCP_TO_PBITS_TABLE_INDEX,
            DRV_RDD_IH_CLASS_2_DIRECT_MODE_DEFAULT,
            DRV_RDD_IH_CLASS_2_DIRECT_MODE_OVERRIDE,
            DRV_RDD_IH_CLASS_2_TARGET_MEMORY_DEFAULT,
            DRV_RDD_IH_CLASS_2_TARGET_MEMORY_OVERRIDE,
            DRV_RDD_IH_CLASS_2_INGRESS_QOS_DEFAULT,
            DRV_RDD_IH_CLASS_2_INGRESS_QOS_OVERRIDE,
            DRV_RDD_IH_CLASS_2_TARGET_RUNNER_DEFAULT,
            DRV_RDD_IH_CLASS_2_TARGET_RUNNER_OVERRIDE_IN_DIRECT_MODE,
            DRV_RDD_IH_CLASS_2_TARGET_RUNNER_FOR_DIRECT_MODE,
            DRV_RDD_IH_CLASS_2_LOAD_BALANCING_ENABLE,
            DRV_RDD_IH_CLASS_2_PREFERENCE_LOAD_BALANCING_ENABLE
        }
    },
    {
        /* WAN bridged high */
        DRV_RDD_IH_CLASS_WAN_BRIDGED_HIGH_INDEX,
        {
            DRV_RDD_IH_LOOKUP_TABLE_MAC_DA_INDEX, /* da_lookup_required */
            DRV_RDD_IH_LOOKUP_TABLE_MAC_SA_INDEX, /* sa_lookup_required */
            DRV_RDD_IH_CLASS_8_CLASS_SEARCH_3,
            DRV_RDD_IH_CLASS_8_CLASS_SEARCH_4,
            DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_BASED_ON_SEARCH1, /* da_lookup_required */
            DRV_RDD_IH_CLASS_8_DROP_ON_MISS,
            DRV_RDD_IH_CLASS_8_DSCP_TO_PBITS_TABLE_INDEX,
            DRV_RDD_IH_CLASS_8_DIRECT_MODE_DEFAULT,
            DRV_RDD_IH_CLASS_8_DIRECT_MODE_OVERRIDE,
            DRV_RDD_IH_CLASS_8_TARGET_MEMORY_DEFAULT,
            DRV_RDD_IH_CLASS_8_TARGET_MEMORY_OVERRIDE,
            DRV_RDD_IH_CLASS_8_INGRESS_QOS_DEFAULT,
            DRV_RDD_IH_CLASS_8_INGRESS_QOS_OVERRIDE,
            DRV_RDD_IH_CLASS_8_TARGET_RUNNER_DEFAULT,
            DRV_RDD_IH_CLASS_8_TARGET_RUNNER_OVERRIDE_IN_DIRECT_MODE,
            DRV_RDD_IH_CLASS_8_TARGET_RUNNER_FOR_DIRECT_MODE,
            DRV_RDD_IH_CLASS_8_LOAD_BALANCING_ENABLE,
            DRV_RDD_IH_CLASS_8_PREFERENCE_LOAD_BALANCING_ENABLE
        }
    },
    {
        /* WAN bridged low */
        DRV_RDD_IH_CLASS_WAN_BRIDGED_LOW_INDEX,
        {
            DRV_RDD_IH_LOOKUP_TABLE_MAC_DA_INDEX, /* da_lookup_required */
            DRV_RDD_IH_LOOKUP_TABLE_MAC_SA_INDEX, /* sa_lookup_required */
            DRV_RDD_IH_CLASS_9_CLASS_SEARCH_3,
            DRV_RDD_IH_CLASS_9_CLASS_SEARCH_4,
            DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_BASED_ON_SEARCH1, /* da_lookup_required */
            DRV_RDD_IH_CLASS_9_DROP_ON_MISS,
            DRV_RDD_IH_CLASS_9_DSCP_TO_PBITS_TABLE_INDEX,
            DRV_RDD_IH_CLASS_9_DIRECT_MODE_DEFAULT,
            DRV_RDD_IH_CLASS_9_DIRECT_MODE_OVERRIDE,
            DRV_RDD_IH_CLASS_9_TARGET_MEMORY_DEFAULT,
            DRV_RDD_IH_CLASS_9_TARGET_MEMORY_OVERRIDE,
            DRV_RDD_IH_CLASS_9_INGRESS_QOS_DEFAULT,
            DRV_RDD_IH_CLASS_9_INGRESS_QOS_OVERRIDE,
            DRV_RDD_IH_CLASS_9_TARGET_RUNNER_DEFAULT,
            DRV_RDD_IH_CLASS_9_TARGET_RUNNER_OVERRIDE_IN_DIRECT_MODE,
            DRV_RDD_IH_CLASS_9_TARGET_RUNNER_FOR_DIRECT_MODE,
            DRV_RDD_IH_CLASS_9_LOAD_BALANCING_ENABLE,
            DRV_RDD_IH_CLASS_9_PREFERENCE_LOAD_BALANCING_ENABLE
        }
    },
    {
        /* LAN bridged eth0 */
        DRV_RDD_IH_CLASS_LAN_BRIDGED_ETH0_INDEX,
        {
            DRV_RDD_IH_LOOKUP_TABLE_MAC_DA_INDEX, /* da_lookup_required */
            DRV_RDD_IH_LOOKUP_TABLE_MAC_SA_INDEX, /* sa_lookup_required */
            DRV_RDD_IH_CLASS_10_CLASS_SEARCH_3,
            DRV_IH_CLASS_SEARCH_LOOKUP_TABLE_4,
            DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_BASED_ON_SEARCH1, /* da_lookup_required */
            DRV_RDD_IH_CLASS_10_DROP_ON_MISS,
            DRV_RDD_IH_CLASS_10_DSCP_TO_PBITS_TABLE_INDEX,
            DRV_RDD_IH_CLASS_10_DIRECT_MODE_DEFAULT,
            DRV_RDD_IH_CLASS_10_DIRECT_MODE_OVERRIDE,
            DRV_RDD_IH_CLASS_10_TARGET_MEMORY_DEFAULT,
            DRV_RDD_IH_CLASS_10_TARGET_MEMORY_OVERRIDE,
            DRV_RDD_IH_CLASS_10_INGRESS_QOS_DEFAULT,
            DRV_RDD_IH_CLASS_10_INGRESS_QOS_OVERRIDE,
            DRV_RDD_IH_CLASS_10_TARGET_RUNNER_DEFAULT,
            DRV_RDD_IH_CLASS_10_TARGET_RUNNER_OVERRIDE_IN_DIRECT_MODE,
            DRV_RDD_IH_CLASS_10_TARGET_RUNNER_FOR_DIRECT_MODE,
            DRV_RDD_IH_CLASS_10_LOAD_BALANCING_ENABLE,
            DRV_RDD_IH_CLASS_10_PREFERENCE_LOAD_BALANCING_ENABLE
        }
    },
    {
        /* LAN bridged eth1 */
        DRV_RDD_IH_CLASS_LAN_BRIDGED_ETH1_INDEX,
        {
            DRV_RDD_IH_LOOKUP_TABLE_MAC_DA_INDEX, /* da_lookup_required */
            DRV_RDD_IH_LOOKUP_TABLE_MAC_SA_INDEX, /* sa_lookup_required */
            DRV_RDD_IH_CLASS_11_CLASS_SEARCH_3,
            DRV_IH_CLASS_SEARCH_LOOKUP_TABLE_4,
            DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_BASED_ON_SEARCH1, /* da_lookup_required */
            DRV_RDD_IH_CLASS_11_DROP_ON_MISS,
            DRV_RDD_IH_CLASS_11_DSCP_TO_PBITS_TABLE_INDEX,
            DRV_RDD_IH_CLASS_11_DIRECT_MODE_DEFAULT,
            DRV_RDD_IH_CLASS_11_DIRECT_MODE_OVERRIDE,
            DRV_RDD_IH_CLASS_11_TARGET_MEMORY_DEFAULT,
            DRV_RDD_IH_CLASS_11_TARGET_MEMORY_OVERRIDE,
            DRV_RDD_IH_CLASS_11_INGRESS_QOS_DEFAULT,
            DRV_RDD_IH_CLASS_11_INGRESS_QOS_OVERRIDE,
            DRV_RDD_IH_CLASS_11_TARGET_RUNNER_DEFAULT,
            DRV_RDD_IH_CLASS_11_TARGET_RUNNER_OVERRIDE_IN_DIRECT_MODE,
            DRV_RDD_IH_CLASS_11_TARGET_RUNNER_FOR_DIRECT_MODE,
            DRV_RDD_IH_CLASS_11_LOAD_BALANCING_ENABLE,
            DRV_RDD_IH_CLASS_11_PREFERENCE_LOAD_BALANCING_ENABLE
        }
    },
    {
        /* LAN bridged eth2 */
        DRV_RDD_IH_CLASS_LAN_BRIDGED_ETH2_INDEX,
        {
            DRV_RDD_IH_LOOKUP_TABLE_MAC_DA_INDEX, /* da_lookup_required */
            DRV_RDD_IH_LOOKUP_TABLE_MAC_SA_INDEX, /* sa_lookup_required */
            DRV_RDD_IH_CLASS_12_CLASS_SEARCH_3,
            DRV_IH_CLASS_SEARCH_LOOKUP_TABLE_4,
            DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_BASED_ON_SEARCH1, /* da_lookup_required */
            DRV_RDD_IH_CLASS_12_DROP_ON_MISS,
            DRV_RDD_IH_CLASS_12_DSCP_TO_PBITS_TABLE_INDEX,
            DRV_RDD_IH_CLASS_12_DIRECT_MODE_DEFAULT,
            DRV_RDD_IH_CLASS_12_DIRECT_MODE_OVERRIDE,
            DRV_RDD_IH_CLASS_12_TARGET_MEMORY_DEFAULT,
            DRV_RDD_IH_CLASS_12_TARGET_MEMORY_OVERRIDE,
            DRV_RDD_IH_CLASS_12_INGRESS_QOS_DEFAULT,
            DRV_RDD_IH_CLASS_12_INGRESS_QOS_OVERRIDE,
            DRV_RDD_IH_CLASS_12_TARGET_RUNNER_DEFAULT,
            DRV_RDD_IH_CLASS_12_TARGET_RUNNER_OVERRIDE_IN_DIRECT_MODE,
            DRV_RDD_IH_CLASS_12_TARGET_RUNNER_FOR_DIRECT_MODE,
            DRV_RDD_IH_CLASS_12_LOAD_BALANCING_ENABLE,
            DRV_RDD_IH_CLASS_12_PREFERENCE_LOAD_BALANCING_ENABLE
        }
    },
    {
        /* LAN bridged eth3 */
        DRV_RDD_IH_CLASS_LAN_BRIDGED_ETH3_INDEX,
        {
            DRV_RDD_IH_LOOKUP_TABLE_MAC_DA_INDEX, /* da_lookup_required */
            DRV_RDD_IH_LOOKUP_TABLE_MAC_SA_INDEX, /*sa_lookup_required*/
            DRV_RDD_IH_CLASS_13_CLASS_SEARCH_3,
            DRV_IH_CLASS_SEARCH_LOOKUP_TABLE_4,
            DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_BASED_ON_SEARCH1, /* da_lookup_required */
            DRV_RDD_IH_CLASS_13_DROP_ON_MISS,
            DRV_RDD_IH_CLASS_13_DSCP_TO_PBITS_TABLE_INDEX,
            DRV_RDD_IH_CLASS_13_DIRECT_MODE_DEFAULT,
            DRV_RDD_IH_CLASS_13_DIRECT_MODE_OVERRIDE,
            DRV_RDD_IH_CLASS_13_TARGET_MEMORY_DEFAULT,
            DRV_RDD_IH_CLASS_13_TARGET_MEMORY_OVERRIDE,
            DRV_RDD_IH_CLASS_13_INGRESS_QOS_DEFAULT,
            DRV_RDD_IH_CLASS_13_INGRESS_QOS_OVERRIDE,
            DRV_RDD_IH_CLASS_13_TARGET_RUNNER_DEFAULT,
            DRV_RDD_IH_CLASS_13_TARGET_RUNNER_OVERRIDE_IN_DIRECT_MODE,
            DRV_RDD_IH_CLASS_13_TARGET_RUNNER_FOR_DIRECT_MODE,
            DRV_RDD_IH_CLASS_13_LOAD_BALANCING_ENABLE,
            DRV_RDD_IH_CLASS_13_PREFERENCE_LOAD_BALANCING_ENABLE
        }
    },
    {
        /* LAN bridged eth4 */
        DRV_RDD_IH_CLASS_LAN_BRIDGED_ETH4_INDEX,
        {
            DRV_RDD_IH_LOOKUP_TABLE_MAC_DA_INDEX, /* da_lookup_required */
            DRV_RDD_IH_LOOKUP_TABLE_MAC_SA_INDEX, /* sa_lookup_required */
            DRV_RDD_IH_CLASS_14_CLASS_SEARCH_3,
            DRV_IH_CLASS_SEARCH_LOOKUP_TABLE_4,
            DRV_IH_OPERATION_BASED_ON_CLASS_SEARCH_BASED_ON_SEARCH1, /* da_lookup_required */
            DRV_RDD_IH_CLASS_14_DROP_ON_MISS,
            DRV_RDD_IH_CLASS_14_DSCP_TO_PBITS_TABLE_INDEX,
            DRV_RDD_IH_CLASS_14_DIRECT_MODE_DEFAULT,
            DRV_RDD_IH_CLASS_14_DIRECT_MODE_OVERRIDE,
            DRV_RDD_IH_CLASS_14_TARGET_MEMORY_DEFAULT,
            DRV_RDD_IH_CLASS_14_TARGET_MEMORY_OVERRIDE,
            DRV_RDD_IH_CLASS_14_INGRESS_QOS_DEFAULT,
            DRV_RDD_IH_CLASS_14_INGRESS_QOS_OVERRIDE,
            DRV_RDD_IH_CLASS_14_TARGET_RUNNER_DEFAULT,
            DRV_RDD_IH_CLASS_14_TARGET_RUNNER_OVERRIDE_IN_DIRECT_MODE,
            DRV_RDD_IH_CLASS_14_TARGET_RUNNER_FOR_DIRECT_MODE,
            DRV_RDD_IH_CLASS_14_LOAD_BALANCING_ENABLE,
            DRV_RDD_IH_CLASS_14_PREFERENCE_LOAD_BALANCING_ENABLE
        }
    }
};
/* following arrays are initialized in run-time. */
/* DMA related */
static uint8_t bbh_rx_dma_data_fifo_base_address[DRV_BBH_NUMBER_OF_PORTS];
static uint8_t bbh_rx_dma_chunk_descriptor_fifo_base_address[DRV_BBH_NUMBER_OF_PORTS];
static uint8_t bbh_rx_dma_data_and_chunk_descriptor_fifos_size[DRV_BBH_NUMBER_OF_PORTS];
static uint8_t bbh_rx_dma_exclusive_threshold[DRV_BBH_NUMBER_OF_PORTS];
/* SDMA related */
static uint8_t bbh_rx_sdma_data_fifo_base_address[DRV_BBH_NUMBER_OF_PORTS];
static uint8_t bbh_rx_sdma_chunk_descriptor_fifo_base_address[DRV_BBH_NUMBER_OF_PORTS];
static uint8_t bbh_rx_sdma_data_and_chunk_descriptor_fifos_size[DRV_BBH_NUMBER_OF_PORTS];
static uint8_t bbh_rx_sdma_exclusive_threshold[DRV_BBH_NUMBER_OF_PORTS];

static uint16_t bbh_ih_ingress_buffers_bitmask[DRV_BBH_NUMBER_OF_PORTS];
static const BL_LILAC_RDD_EMAC_ID_DTE bbh_to_rdd_emac_map[] =
{
		BL_LILAC_RDD_EMAC_ID_0 ,
		BL_LILAC_RDD_EMAC_ID_1 ,
		BL_LILAC_RDD_EMAC_ID_2 ,
		BL_LILAC_RDD_EMAC_ID_3 ,
		BL_LILAC_RDD_EMAC_ID_4 ,
};
static const uint32_t  bbh_to_rdd_eth_thread[] =
{
    ETH0_TX_THREAD_NUMBER,
    ETH1_TX_THREAD_NUMBER,
    ETH2_TX_THREAD_NUMBER,
    ETH3_TX_THREAD_NUMBER,
    ETH4_TX_THREAD_NUMBER,

};
/******************************************************************************/
/* There are 8 IH ingress queues. This enumeration defines, for each ingress  */
/* queue, which physical source port it belongs to.                           */
/******************************************************************************/
typedef enum
{
    IH_INGRESS_QUEUE_0_ETH0 ,
    IH_INGRESS_QUEUE_1_ETH1 ,
    IH_INGRESS_QUEUE_2_ETH2 ,
    IH_INGRESS_QUEUE_3_ETH3 ,
    IH_INGRESS_QUEUE_4_ETH4 ,
    IH_INGRESS_QUEUE_5_GPON_OR_AE ,
    IH_INGRESS_QUEUE_6_RUNNER_A ,
    IH_INGRESS_QUEUE_7_RUNNER_B ,

    NUMBER_OF_IH_INGRESS_QUEUES
}
IH_INGRESS_QUEUE_INDEX ;

/* FW binaries */
extern uint32_t firmware_binary_A[];
extern uint32_t firmware_binary_B[];
extern uint32_t firmware_binary_C[];
extern uint32_t firmware_binary_D[];
extern uint16_t firmware_predict_A[];
extern uint16_t firmware_predict_B[];
extern uint16_t firmware_predict_C[];
extern uint16_t firmware_predict_D[];

void f_basic_sbpm_sp_enable(void);
void f_basic_bpm_sp_enable(void);

int f_validate_ddr_address(uint32_t address);

static uint32_t f_numOfLanPorts(void)
{
	uint32_t ret;
	switch(pDpiCfg->wanType)
	{

		case WAN_TYPE_RGMII:
			ret = DRV_BBH_EMAC_3;
			break;
		case WAN_TYPE_NONE:
		case WAN_TYPE_AE:
		case WAN_TYPE_GPON:
		case WAN_TYPE_EPON:
			ret = UtilGetChipRev() > 1 ? DRV_BBH_EMAC_4:DRV_BBH_EMAC_3 ;
			break;
		default:
			ret = 0;
	}
	return ret;
}
static DRV_BBH_PORT_INDEX f_getWanEmac(void)
{
	DRV_BBH_PORT_INDEX ret;
	switch(pDpiCfg->wanType)
	{

		case WAN_TYPE_RGMII:
			ret = DRV_BBH_EMAC_4;
			break;
		case WAN_TYPE_AE:
			ret = DRV_BBH_EMAC_5;
			break;
		case WAN_TYPE_NONE:
		case WAN_TYPE_GPON:
			ret = DRV_BBH_GPON;
			break;
		case WAN_TYPE_EPON:
			ret = DRV_BBH_EPON;
			break;
		default:
			ret = 0;
	}
	return ret;
}
static void f_update_bbh_ih_ingress_buffers_bitmask(DRV_BBH_PORT_INDEX bbh_port_index, uint8_t base_location, uint8_t queue_size)
{
    uint16_t bitmask = 0;
    uint8_t i;

    /* set '1's according to queue_size  */
    for (i= 0; i < queue_size; i++)
    {
        SET_BIT_I(bitmask, i, 1);
    }
    /* do shifting according to xi_base_location */
    bitmask <<= base_location;

    /* update in database */
    bbh_ih_ingress_buffers_bitmask[bbh_port_index] = bitmask;
}
static void fi_dma_configure_memory_allocation ( E_BB_MODULE module_id ,
                                                 E_DMA_PERIPHERAL peripheral_id ,
                                                 uint32_t data_memory_offset_address ,
                                                 uint32_t cd_memory_offset_address ,
                                                 uint32_t number_of_buffers )
{
    DMA_REGS_CONFIG_MALLOC config_malloc ;

    DMA_REGS_CONFIG_MALLOC_READ( module_id , peripheral_id , config_malloc ) ;
    config_malloc.datatoffset = data_memory_offset_address ;
    config_malloc.cdoffset = cd_memory_offset_address ;
    config_malloc.numofbuff = number_of_buffers ;
    DMA_REGS_CONFIG_MALLOC_WRITE( module_id , peripheral_id , config_malloc ) ;
}
static void f_initialize_dma_sdma(void)
{
    E_DMA_PERIPHERAL peripheral_id ;
    DMA_REGS_CONFIG_U_THRESH dma_thresh;
    uint16_t lan_fifo_size,rgmii_fifo_size;

    lan_fifo_size   = (UtilGetChipRev() > 1 ? BBH_RX_DMA_FIFOS_SIZE_LAN_BBH_B0:BBH_RX_DMA_FIFOS_SIZE_LAN_BBH);
    rgmii_fifo_size = BBH_RX_DMA_FIFOS_SIZE_LAN_BBH_B0 * DMA_PERIPHERAL_EMAC_3;
    /* emac 0-4 */
    for ( peripheral_id = DMA_PERIPHERAL_EMAC_0 ; peripheral_id < DMA_PERIPHERAL_EMAC_4 ; ++peripheral_id )
    {
        fi_dma_configure_memory_allocation ( BB_MODULE_DMA ,
                                             peripheral_id ,
                                             lan_fifo_size * peripheral_id  ,
                                             lan_fifo_size * peripheral_id  ,
                                             lan_fifo_size ) ;
        /* SDMA */
        fi_dma_configure_memory_allocation ( BB_MODULE_SDMA ,
                                             peripheral_id ,
                                             bbh_rx_sdma_data_fifo_base_address [ peripheral_id ] ,
                                             bbh_rx_sdma_chunk_descriptor_fifo_base_address [ peripheral_id ] ,
                                             bbh_rx_sdma_data_and_chunk_descriptor_fifos_size [ peripheral_id ] ) ;
    }
    if (UtilGetChipRev() > 1  )
    {
    if(pDpiCfg->wanType!=WAN_TYPE_RGMII)
    {
      //handle the RGMII port
      fi_dma_configure_memory_allocation ( BB_MODULE_DMA ,
                                           DMA_PERIPHERAL_EMAC_4 ,
                                           rgmii_fifo_size+ BBH_RX_DMA_FIFOS_SIZE_RGMII_LAN_BBH_B0  ,
                                           rgmii_fifo_size+ BBH_RX_DMA_FIFOS_SIZE_RGMII_LAN_BBH_B0  ,
                                           BBH_RX_DMA_FIFOS_SIZE_RGMII_LAN_BBH_B0) ;
      /* SDMA */
      fi_dma_configure_memory_allocation ( BB_MODULE_SDMA ,
                                               DMA_PERIPHERAL_EMAC_4 ,
                                               bbh_rx_sdma_data_fifo_base_address [ peripheral_id ] ,
                                               bbh_rx_sdma_chunk_descriptor_fifo_base_address [ peripheral_id ] ,
                                               bbh_rx_sdma_data_and_chunk_descriptor_fifos_size [ peripheral_id ] ) ;
      }
    else
      {
          //handle the RGMII port
        fi_dma_configure_memory_allocation ( BB_MODULE_DMA ,
                                             DMA_PERIPHERAL_EMAC_4 ,
                                             bbh_rx_dma_data_fifo_base_address [ DRV_BBH_EMAC_4 ] ,
                                             bbh_rx_dma_chunk_descriptor_fifo_base_address [ DRV_BBH_EMAC_4 ] ,
                                             bbh_rx_dma_data_and_chunk_descriptor_fifos_size [ DRV_BBH_EMAC_4 ] ) ;
        /* SDMA */
        fi_dma_configure_memory_allocation ( BB_MODULE_SDMA ,
                                                 DMA_PERIPHERAL_EMAC_4 ,
                                                 bbh_rx_sdma_data_fifo_base_address [ DRV_BBH_EMAC_4 ] ,
                                                 bbh_rx_sdma_chunk_descriptor_fifo_base_address [ DRV_BBH_EMAC_4 ] ,
                                                 bbh_rx_sdma_data_and_chunk_descriptor_fifos_size [ DRV_BBH_EMAC_4 ] ) ;
        }
      }
    /* emac 5 or gpon if needed */
    if ( pDpiCfg->wanType==WAN_TYPE_AE )
    {
        /* DMA */
        fi_dma_configure_memory_allocation ( BB_MODULE_DMA ,
                                             DMA_PERIPHERAL_EMAC_AE_GPON_EPON ,
                                             bbh_rx_dma_data_fifo_base_address [ DRV_BBH_EMAC_5 ] ,
                                             bbh_rx_dma_chunk_descriptor_fifo_base_address [ DRV_BBH_EMAC_5 ] ,
                                             bbh_rx_dma_data_and_chunk_descriptor_fifos_size [ DRV_BBH_EMAC_5 ] ) ;
        /* SDMA */
        fi_dma_configure_memory_allocation ( BB_MODULE_SDMA ,
        		DMA_PERIPHERAL_EMAC_AE_GPON_EPON ,
                                             bbh_rx_sdma_data_fifo_base_address [ DRV_BBH_EMAC_5 ] ,
                                             bbh_rx_sdma_chunk_descriptor_fifo_base_address [ DRV_BBH_EMAC_5 ] ,
                                             bbh_rx_sdma_data_and_chunk_descriptor_fifos_size [ DRV_BBH_EMAC_5 ] ) ;
    }
    else if (pDpiCfg->wanType==WAN_TYPE_GPON)
    {
        /* DMA*/
        fi_dma_configure_memory_allocation ( BB_MODULE_DMA ,
        									 DMA_PERIPHERAL_EMAC_AE_GPON_EPON ,
                                             bbh_rx_dma_data_fifo_base_address [ DRV_BBH_GPON ] ,
                                             bbh_rx_dma_chunk_descriptor_fifo_base_address [ DRV_BBH_GPON ] ,
                                             bbh_rx_dma_data_and_chunk_descriptor_fifos_size [ DRV_BBH_GPON ]) ;
        /* SDMA */
        fi_dma_configure_memory_allocation ( BB_MODULE_SDMA ,
        									 DMA_PERIPHERAL_EMAC_AE_GPON_EPON ,
                                             bbh_rx_sdma_data_fifo_base_address [ DRV_BBH_GPON ] ,
                                             bbh_rx_sdma_chunk_descriptor_fifo_base_address [ DRV_BBH_GPON ] ,
                                             bbh_rx_sdma_data_and_chunk_descriptor_fifos_size [ DRV_BBH_GPON ] ) ;
    }
    //TODO:not operational yet
    else if (pDpiCfg->wanType==WAN_TYPE_EPON)
    {
        /* DMA*/
        fi_dma_configure_memory_allocation ( BB_MODULE_DMA ,
        									 DMA_PERIPHERAL_EMAC_AE_GPON_EPON ,
                                             bbh_rx_dma_data_fifo_base_address [ DRV_BBH_EPON ] ,
                                             bbh_rx_dma_chunk_descriptor_fifo_base_address [ DRV_BBH_EPON ] ,
                                             bbh_rx_dma_data_and_chunk_descriptor_fifos_size [ DRV_BBH_EPON ] ) ;
        /* SDMA */
        fi_dma_configure_memory_allocation ( BB_MODULE_SDMA ,
        									 DMA_PERIPHERAL_EMAC_AE_GPON_EPON ,
                                             bbh_rx_sdma_data_fifo_base_address [ DRV_BBH_EPON ] ,
                                             bbh_rx_sdma_chunk_descriptor_fifo_base_address [ DRV_BBH_EPON ] ,
                                             bbh_rx_sdma_data_and_chunk_descriptor_fifos_size [ DRV_BBH_EPON ] ) ;

    }
    dma_thresh.out_of_u = 1;
    dma_thresh.into_u = 5;
    DMA_REGS_CONFIG_U_THRESH_WRITE(BB_MODULE_DMA,DMA_PERIPHERAL_EMAC_0,dma_thresh);
	DMA_REGS_CONFIG_U_THRESH_WRITE(BB_MODULE_DMA,DMA_PERIPHERAL_EMAC_1,dma_thresh);
	DMA_REGS_CONFIG_U_THRESH_WRITE(BB_MODULE_DMA,DMA_PERIPHERAL_EMAC_2,dma_thresh);
	DMA_REGS_CONFIG_U_THRESH_WRITE(BB_MODULE_DMA,DMA_PERIPHERAL_EMAC_3,dma_thresh);
	DMA_REGS_CONFIG_U_THRESH_WRITE(BB_MODULE_DMA,DMA_PERIPHERAL_EMAC_4,dma_thresh);
	DMA_REGS_CONFIG_U_THRESH_WRITE(BB_MODULE_DMA,DMA_PERIPHERAL_EMAC_AE_GPON_EPON,dma_thresh);

}

static E_DPI_RC f_initialize_bbh_dma_sdma_related_arrays(void)
{
    int i;
    uint8_t dma_base_address = 0;
    uint8_t sdma_base_address = 0;

    uint32_t num_emac = f_numOfLanPorts();
    uint32_t wanEmac  = f_getWanEmac();
      /* handle emac ports */
    for (i = 0; i <= num_emac; i++)
    {

		bbh_rx_dma_data_and_chunk_descriptor_fifos_size[i] = (i == DRV_BBH_EMAC_4) ? BBH_RX_DMA_FIFOS_SIZE_RGMII_LAN_DMA_B0 :
		                                                      (UtilGetChipRev() > 1) ? BBH_RX_DMA_FIFOS_SIZE_LAN_DMA_B0 :BBH_RX_DMA_FIFOS_SIZE_LAN_DMA;
		bbh_rx_dma_exclusive_threshold[i] = (i == DRV_BBH_EMAC_4) ? BBH_RX_DMA_EXCLUSIVE_THRESHOLD_RGMII_LAN_DMA_B0:
		                                    (UtilGetChipRev() > 1) ?BBH_RX_DMA_EXCLUSIVE_THRESHOLD_LAN_DMA_B0:BBH_RX_DMA_EXCLUSIVE_THRESHOLD_LAN_DMA;
		bbh_rx_sdma_data_and_chunk_descriptor_fifos_size[i] = BBH_RX_SDMA_FIFOS_SIZE_LAN;
		bbh_rx_sdma_exclusive_threshold[i] = BBH_RX_SDMA_EXCLUSIVE_THRESHOLD_LAN;

        bbh_rx_dma_data_fifo_base_address[i] = dma_base_address;
        bbh_rx_dma_chunk_descriptor_fifo_base_address[i] = dma_base_address;
        bbh_rx_sdma_data_fifo_base_address[i] = sdma_base_address;
        bbh_rx_sdma_chunk_descriptor_fifo_base_address[i] = sdma_base_address;


        dma_base_address +=  bbh_rx_dma_data_and_chunk_descriptor_fifos_size[i] ;
        sdma_base_address += bbh_rx_sdma_data_and_chunk_descriptor_fifos_size[i];

    }

    /* cfg wan side */
    if ( pDpiCfg->wanType != WAN_TYPE_NONE )
    {
        bbh_rx_dma_data_and_chunk_descriptor_fifos_size[wanEmac] = UtilGetChipRev() > 1 ? BBH_RX_DMA_FIFOS_SIZE_WAN_DMA_B0:BBH_RX_DMA_FIFOS_SIZE_WAN_DMA;
        bbh_rx_dma_exclusive_threshold[wanEmac] = UtilGetChipRev() > 1 ? BBH_RX_DMA_EXCLUSIVE_THRESHOLD_WAN_DMA_B0:BBH_RX_DMA_EXCLUSIVE_THRESHOLD_WAN_DMA;
        bbh_rx_sdma_data_and_chunk_descriptor_fifos_size[wanEmac] = BBH_RX_SDMA_FIFOS_SIZE_WAN;
        bbh_rx_sdma_exclusive_threshold[wanEmac] = BBH_RX_SDMA_EXCLUSIVE_THRESHOLD_WAN;

        bbh_rx_dma_data_fifo_base_address[wanEmac] = (UtilGetChipRev() > 1) ? 0 :dma_base_address;
        bbh_rx_dma_chunk_descriptor_fifo_base_address[wanEmac] = (UtilGetChipRev() > 1) ? 0 :dma_base_address;
        bbh_rx_sdma_data_fifo_base_address[wanEmac] = sdma_base_address;
        bbh_rx_sdma_chunk_descriptor_fifo_base_address[wanEmac] = sdma_base_address;

        dma_base_address += bbh_rx_dma_data_and_chunk_descriptor_fifos_size[wanEmac];
        sdma_base_address += bbh_rx_sdma_data_and_chunk_descriptor_fifos_size[wanEmac];
    }

    /* check that we didn't overrun */
    if (dma_base_address > BBH_RX_DMA_TOTAL_NUMBER_OF_CHUNK_DESCRIPTORS ||
        sdma_base_address > BBH_RX_SDMA_TOTAL_NUMBER_OF_CHUNK_DESCRIPTORS)
    {
    	printk("%s:(%d) error",__FUNCTION__,__LINE__);
        return DPI_RC_ERROR;
    }
    return DPI_RC_OK;
}
static uint16_t f_get_bbh_rx_pd_fifo_base_address_normal_queue_in_8_byte(DRV_BBH_PORT_INDEX port_index)
{
    switch (port_index)
    {
    case DRV_BBH_EMAC_0:
        return MS_BYTE_TO_8_BYTE_RESOLUTION(ETH0_RX_DESCRIPTORS_ADDRESS);

    case DRV_BBH_EMAC_1:
        return MS_BYTE_TO_8_BYTE_RESOLUTION(ETH1_RX_DESCRIPTORS_ADDRESS);

    case DRV_BBH_EMAC_2:
        return MS_BYTE_TO_8_BYTE_RESOLUTION(ETH2_RX_DESCRIPTORS_ADDRESS);

    case DRV_BBH_EMAC_3:
        return MS_BYTE_TO_8_BYTE_RESOLUTION(ETH3_RX_DESCRIPTORS_ADDRESS);

    case DRV_BBH_EMAC_4:
        if ( pDpiCfg->wanType == WAN_TYPE_RGMII && port_index == DRV_BBH_EMAC_4 )
        {
            return MS_BYTE_TO_8_BYTE_RESOLUTION(GPON_RX_NORMAL_DESCRIPTORS_ADDRESS);
        }
        else
        {
            return MS_BYTE_TO_8_BYTE_RESOLUTION(ETH4_RX_DESCRIPTORS_ADDRESS);
        }

        /* emac 5 is supported as wan only */
    case DRV_BBH_EMAC_5:
    case DRV_BBH_GPON:
    case DRV_BBH_EPON:
        return MS_BYTE_TO_8_BYTE_RESOLUTION(GPON_RX_NORMAL_DESCRIPTORS_ADDRESS);


    default:
        return 0;
    }
}
static uint16_t f_get_bbh_rx_pd_fifo_base_address_direct_queue_in_8_byte(DRV_BBH_PORT_INDEX port_index)
{
    switch (port_index)
    {
    case DRV_BBH_EMAC_0:
        return MS_BYTE_TO_8_BYTE_RESOLUTION(ETH0_RX_DIRECT_DESCRIPTORS_ADDRESS);

    case DRV_BBH_EMAC_1:
        return MS_BYTE_TO_8_BYTE_RESOLUTION(ETH1_RX_DIRECT_DESCRIPTORS_ADDRESS);

    case DRV_BBH_EMAC_2:
        return MS_BYTE_TO_8_BYTE_RESOLUTION(ETH2_RX_DIRECT_DESCRIPTORS_ADDRESS);

    case DRV_BBH_EMAC_3:
        return MS_BYTE_TO_8_BYTE_RESOLUTION(ETH3_RX_DIRECT_DESCRIPTORS_ADDRESS);

    case DRV_BBH_EMAC_4:
    	if ( pDpiCfg->wanType == WAN_TYPE_RGMII && port_index == DRV_BBH_EMAC_4)
        {
            return MS_BYTE_TO_8_BYTE_RESOLUTION(GPON_RX_DIRECT_DESCRIPTORS_ADDRESS);
        }
        else
        {
            return MS_BYTE_TO_8_BYTE_RESOLUTION(ETH4_RX_DIRECT_DESCRIPTORS_ADDRESS);
        }

        /* emac 5 is supported as wan only */
    case DRV_BBH_EMAC_5:
    case DRV_BBH_GPON:
    case DRV_BBH_EPON:
        return MS_BYTE_TO_8_BYTE_RESOLUTION(GPON_RX_DIRECT_DESCRIPTORS_ADDRESS);


    default:
        return 0;
    }
}
static uint8_t f_bbh_rx_runner_task_normal_queue(DRV_IH_RUNNER_CLUSTER runner_cluster, DRV_BBH_PORT_INDEX port_index)
{
	uint8_t isWan = BBH_PORT_IS_WAN(pDpiCfg->wanType,port_index);

    if (runner_cluster == DRV_IH_RUNNER_CLUSTER_A)
    {
        if (isWan)
        {
            return WAN_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER;
        }
        /* the port is LAN */
        else
        {
            /* runner A doesn't handle the normal queue for LAN */
            return 0;
        }
    }
    /* runner B */
    else
    {
        if (isWan)
        {
            /* runner B doesn't handle WAN */
            return 0;
        }
        /* the port is LAN */
        else
        {
            switch (port_index)
            {
            case DRV_BBH_EMAC_0:
                return LAN0_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER;

            case DRV_BBH_EMAC_1:
                return LAN1_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER;

            case DRV_BBH_EMAC_2:
                return LAN2_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER;

            case DRV_BBH_EMAC_3:
                return LAN3_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER;

            case DRV_BBH_EMAC_4:
                return LAN4_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER;

                /* we are not supposed to get here */
            default:
                return 0;
            }
        }
    }
}

static uint8_t f_bbh_rx_runner_task_direct_queue(DRV_IH_RUNNER_CLUSTER runner_cluster, DRV_BBH_PORT_INDEX portIndex)
{
    if ( BBH_PORT_IS_WAN( pDpiCfg->wanType,portIndex ) )
    {
        return WAN_DIRECT_THREAD_NUMBER;
    }

    /* the port is LAN */
    if (runner_cluster == DRV_IH_RUNNER_CLUSTER_A)
    {
        switch (portIndex)
        {
        case DRV_BBH_EMAC_0:
             return ETH0_RX_DIRECT_RUNNER_A_TASK_NUMBER;

        case DRV_BBH_EMAC_1:
             return ETH1_RX_DIRECT_RUNNER_A_TASK_NUMBER;

        case DRV_BBH_EMAC_2:
             return ETH2_RX_DIRECT_RUNNER_A_TASK_NUMBER;

        case DRV_BBH_EMAC_3:
             return ETH3_RX_DIRECT_RUNNER_A_TASK_NUMBER;

        case DRV_BBH_EMAC_4:
             return ETH4_RX_DIRECT_RUNNER_A_TASK_NUMBER;

             /* we are not supposed to get here */
         default:
             return 0;
         }
    }/* runner B */
    else
    {
        switch (portIndex)
         {
        case DRV_BBH_EMAC_0:
             return ETH0_RX_DIRECT_RUNNER_B_TASK_NUMBER;

        case DRV_BBH_EMAC_1:
             return ETH1_RX_DIRECT_RUNNER_B_TASK_NUMBER;

        case DRV_BBH_EMAC_2:
             return ETH2_RX_DIRECT_RUNNER_B_TASK_NUMBER;

        case DRV_BBH_EMAC_3:
             return ETH3_RX_DIRECT_RUNNER_B_TASK_NUMBER;

        case DRV_BBH_EMAC_4:
             return ETH4_RX_DIRECT_RUNNER_B_TASK_NUMBER;

             /* we are not supposed to get here */
         default:
             return 0;
         }
    }
}

static void f_initialize_bbh_of_emac_port(DRV_BBH_PORT_INDEX port_index)
{
    uint16_t mdu_mode_read_pointer_address_in_byte;
    uint32_t mdu_mode_read_pointer_address_in_byte_uint32;
    DRV_BBH_TX_CONFIGURATION bbh_tx_configuration ;
    DRV_BBH_RX_CONFIGURATION bbh_rx_configuration ;
    DRV_BBH_PER_FLOW_CONFIGURATION per_flow_configuration ;
    uint16_t    lan_fifo_size;
    uint16_t    lan_exc_thresh;

    /*** BBH TX ***/
    bbh_tx_configuration.dma_route_address = bbh_route_address_dma[port_index];
    bbh_tx_configuration.bpm_route_address = bbh_route_address_bpm[port_index];
    bbh_tx_configuration.sdma_route_address = bbh_route_address_sdma[port_index];
    bbh_tx_configuration.sbpm_route_address = bbh_route_address_sbpm[port_index];
    /* runner 0 is the one which handles TX  except for wan mode emac4*/
    if (( pDpiCfg->wanType == WAN_TYPE_RGMII && port_index == DRV_BBH_EMAC_4) ||
        ( pDpiCfg->wanType == WAN_TYPE_AE && port_index == DRV_BBH_EMAC_5))
    {
        bbh_tx_configuration.runner_route_address = bbh_route_address_runner_1[port_index];
    }
    else
    {
        bbh_tx_configuration.runner_route_address = bbh_route_address_runner_0[port_index];
    }

    bbh_tx_configuration.ddr_buffer_size = DRV_BBH_DDR_BUFFER_SIZE_2_KB;
    bbh_tx_configuration.payload_offset_resolution = DRV_BBH_PAYLOAD_OFFSET_RESOLUTION_2_B;

    bbh_tx_configuration.multicast_headers_base_address_in_byte = (uint32_t)ddr_multicast_base_address_phys;//


    if (( pDpiCfg->wanType == WAN_TYPE_RGMII && port_index == DRV_BBH_EMAC_4) ||
    	( pDpiCfg->wanType == WAN_TYPE_AE && port_index == DRV_BBH_EMAC_5))
    {
        bbh_tx_configuration.multicast_header_size = BBH_MULTICAST_HEADER_SIZE_FOR_LAN_PORT;
        /* this is the task also in Gbe case */
        bbh_tx_configuration.task_0 = WAN_TX_THREAD_NUMBER;
        bbh_tx_configuration.skb_address = MS_BYTE_TO_8_BYTE_RESOLUTION(GPON_ABSOLUTE_TX_BBH_COUNTER_ADDRESS);
    }
    else
    	/*we will reach here in case the BBH port in lan ports 0 - 4*/
    {
        bbh_tx_configuration.multicast_header_size = BBH_MULTICAST_HEADER_SIZE_FOR_LAN_PORT;

        bbh_tx_configuration.skb_address = MS_BYTE_TO_8_BYTE_RESOLUTION(EMAC_ABSOLUTE_TX_BBH_COUNTER_ADDRESS) + bbh_to_rdd_emac_map[port_index];

        bbh_tx_configuration.task_0 = bbh_to_rdd_eth_thread[port_index];
    }

    /* other task numbers are irrelevant (relevant for GPON only) */
    bbh_tx_configuration.task_1 = 0;
    bbh_tx_configuration.task_2 = 0;
    bbh_tx_configuration.task_3 = 0;
    bbh_tx_configuration.task_4 = 0;
    bbh_tx_configuration.task_5 = 0;
    bbh_tx_configuration.task_6 = 0;
    bbh_tx_configuration.task_7 = 0;
    bbh_tx_configuration.task_8_39 = 0;

    if (( pDpiCfg->wanType == WAN_TYPE_RGMII && port_index == DRV_BBH_EMAC_4) ||
      	( pDpiCfg->wanType == WAN_TYPE_AE && port_index == DRV_BBH_EMAC_5))
    {
        bbh_tx_configuration.mdu_mode_enable = 0;
        /* irrelevant in this case */
        bbh_tx_configuration.mdu_mode_read_pointer_address_in_8_byte = 0;
    }
    else
    {
        bbh_tx_configuration.mdu_mode_enable = 1;

        rdd_mdu_mode_pointer_get(bbh_to_rdd_emac_map[port_index], &mdu_mode_read_pointer_address_in_byte);

        mdu_mode_read_pointer_address_in_byte_uint32 = mdu_mode_read_pointer_address_in_byte;

        /* after division, this will be back a 16 bit number */
        bbh_tx_configuration.mdu_mode_read_pointer_address_in_8_byte = (uint16_t)MS_BYTE_TO_8_BYTE_RESOLUTION(mdu_mode_read_pointer_address_in_byte_uint32);
    }

    /* For Ethernet port working in MDU mode, PD FIFO size should be configured to 4 (and not 8). */
    bbh_tx_configuration.pd_fifo_size_0 =  (bbh_tx_configuration.mdu_mode_enable == 1) ? BBH_TX_EMAC_PD_FIFO_SIZE_MDU_MODE_ENABLED : BBH_TX_EMAC_PD_FIFO_SIZE_MDU_MODE_DISABLED;

    /* other FIFOs are irrelevant (relevant for GPON only) */
    bbh_tx_configuration.pd_fifo_size_1 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    bbh_tx_configuration.pd_fifo_size_2 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    bbh_tx_configuration.pd_fifo_size_3 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    bbh_tx_configuration.pd_fifo_size_4 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    bbh_tx_configuration.pd_fifo_size_5 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    bbh_tx_configuration.pd_fifo_size_6 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    bbh_tx_configuration.pd_fifo_size_7 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    bbh_tx_configuration.pd_fifo_size_8_15 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    bbh_tx_configuration.pd_fifo_size_16_23 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    bbh_tx_configuration.pd_fifo_size_24_31 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    bbh_tx_configuration.pd_fifo_size_32_39 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;

    bbh_tx_configuration.pd_fifo_base_0 = 0;

    /* other FIFOs are irrelevant (relevant for GPON only) */
    bbh_tx_configuration.pd_fifo_base_1 = 0;
    bbh_tx_configuration.pd_fifo_base_2 = 0;
    bbh_tx_configuration.pd_fifo_base_3 = 0;
    bbh_tx_configuration.pd_fifo_base_4 = 0;
    bbh_tx_configuration.pd_fifo_base_5 = 0;
    bbh_tx_configuration.pd_fifo_base_6 = 0;
    bbh_tx_configuration.pd_fifo_base_7 = 0;
    bbh_tx_configuration.pd_fifo_base_8_15 = 0;
    bbh_tx_configuration.pd_fifo_base_16_23 = 0;
    bbh_tx_configuration.pd_fifo_base_24_31 = 0;
    bbh_tx_configuration.pd_fifo_base_32_39 = 0;

    /* pd_prefetch_byte_threshold feature is irrelevant in EMAC (since there is only one FIFO) */
    bbh_tx_configuration.pd_prefetch_byte_threshold_enable = 0;
    bbh_tx_configuration.pd_prefetch_byte_threshold_0_in_32_byte = 0;
    bbh_tx_configuration.pd_prefetch_byte_threshold_1_in_32_byte = 0;
    bbh_tx_configuration.pd_prefetch_byte_threshold_2_in_32_byte = 0;
    bbh_tx_configuration.pd_prefetch_byte_threshold_3_in_32_byte = 0;
    bbh_tx_configuration.pd_prefetch_byte_threshold_4_in_32_byte = 0;
    bbh_tx_configuration.pd_prefetch_byte_threshold_5_in_32_byte = 0;
    bbh_tx_configuration.pd_prefetch_byte_threshold_6_in_32_byte = 0;
    bbh_tx_configuration.pd_prefetch_byte_threshold_7_in_32_byte = 0;
    bbh_tx_configuration.pd_prefetch_byte_threshold_8_39_in_32_byte = 0;

    bbh_tx_configuration.dma_read_requests_fifo_base_address = bbh_dma_and_sdma_read_requests_fifo_base_address[port_index];
    bbh_tx_configuration.dma_read_requests_maximal_number = BBH_TX_CONFIGURATIONS_DMACFG_TX_MAXREQ_MAX_VALUE_VALUE_RESET_VALUE;
    bbh_tx_configuration.sdma_read_requests_fifo_base_address = bbh_dma_and_sdma_read_requests_fifo_base_address[port_index];
    bbh_tx_configuration.sdma_read_requests_maximal_number = BBH_TX_CONFIGURATIONS_SDMACFG_TX_MAXREQ_MAX_VALUE_VALUE_RESET_VALUE;

    /* irrelevant in EMAC case */
    bbh_tx_configuration.tcont_address_in_8_byte = 0;

    bbh_tx_configuration.ddr_tm_base_address = (uint32_t)ddr_tm_base_address_phys;

    bbh_tx_configuration.emac_1588_enable = 0;

    fi_bl_drv_bbh_tx_set_configuration(port_index, &bbh_tx_configuration);

    /*** BBH RX ***/
    /* bbh_rx_set_configuration */
    bbh_rx_configuration.dma_route_address = bbh_route_address_dma[port_index];
    bbh_rx_configuration.bpm_route_address = bbh_route_address_bpm[port_index];
    bbh_rx_configuration.sdma_route_address = bbh_route_address_sdma[port_index];
    bbh_rx_configuration.sbpm_route_address = bbh_route_address_sbpm[port_index];
    bbh_rx_configuration.runner_0_route_address = bbh_route_address_runner_0[port_index];
    bbh_rx_configuration.runner_1_route_address = bbh_route_address_runner_1[port_index];
    bbh_rx_configuration.ih_route_address = bbh_route_address_ih[port_index];

    bbh_rx_configuration.ddr_buffer_size = DRV_BBH_DDR_BUFFER_SIZE_2_KB;
    bbh_rx_configuration.ddr_tm_base_address = (uint32_t)ddr_tm_base_address_phys;
    bbh_rx_configuration.pd_fifo_base_address_normal_queue_in_8_byte = f_get_bbh_rx_pd_fifo_base_address_normal_queue_in_8_byte(port_index);
    bbh_rx_configuration.pd_fifo_base_address_direct_queue_in_8_byte = f_get_bbh_rx_pd_fifo_base_address_direct_queue_in_8_byte(port_index);
    bbh_rx_configuration.pd_fifo_size_normal_queue = DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    bbh_rx_configuration.pd_fifo_size_direct_queue = DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    bbh_rx_configuration.runner_0_task_normal_queue = f_bbh_rx_runner_task_normal_queue (DRV_IH_RUNNER_CLUSTER_A, port_index);
    bbh_rx_configuration.runner_0_task_direct_queue = f_bbh_rx_runner_task_direct_queue (DRV_IH_RUNNER_CLUSTER_A, port_index);
    bbh_rx_configuration.runner_1_task_normal_queue = f_bbh_rx_runner_task_normal_queue (DRV_IH_RUNNER_CLUSTER_B, port_index);
    bbh_rx_configuration.runner_1_task_direct_queue = f_bbh_rx_runner_task_direct_queue (DRV_IH_RUNNER_CLUSTER_B, port_index);
    lan_fifo_size = UtilGetChipRev() > 1 ? BBH_RX_DMA_FIFOS_SIZE_LAN_BBH_B0:BBH_RX_DMA_FIFOS_SIZE_LAN_BBH;
    lan_exc_thresh = UtilGetChipRev() > 1 ? BBH_RX_DMA_EXCLUSIVE_THRESHOLD_LAN_BBH_B0:BBH_RX_DMA_EXCLUSIVE_THRESHOLD_LAN_BBH;
    bbh_rx_configuration.dma_data_fifo_base_address = (port_index == DRV_BBH_EMAC_4) ? (lan_fifo_size * (port_index-1))+BBH_RX_DMA_FIFOS_SIZE_RGMII_LAN_BBH_B0:
                                                      lan_fifo_size * port_index;
    bbh_rx_configuration.dma_chunk_descriptor_fifo_base_address = bbh_rx_configuration.dma_data_fifo_base_address;
    bbh_rx_configuration.sdma_data_fifo_base_address = bbh_rx_sdma_data_fifo_base_address[port_index];
    bbh_rx_configuration.sdma_chunk_descriptor_fifo_base_address = bbh_rx_sdma_chunk_descriptor_fifo_base_address[port_index ];
    bbh_rx_configuration.dma_data_and_chunk_descriptor_fifos_size = (port_index == DRV_BBH_EMAC_4) ? BBH_RX_DMA_FIFOS_SIZE_RGMII_LAN_BBH_B0:lan_fifo_size;
    bbh_rx_configuration.dma_exclusive_threshold =  (port_index == DRV_BBH_EMAC_4) ? BBH_RX_DMA_EXCLUSIVE_THRESHOLD_RGMII_LAN_BBH_B0:lan_exc_thresh;;
    bbh_rx_configuration.sdma_data_and_chunk_descriptor_fifos_size = bbh_rx_sdma_data_and_chunk_descriptor_fifos_size[port_index ];
    bbh_rx_configuration.sdma_exclusive_threshold = bbh_rx_sdma_exclusive_threshold[port_index];


    bbh_rx_configuration.minimum_packet_size_0 = MIN_ETH_PKT_SIZE;


    /* minimum_packet_size 1-3 are not in use */
    bbh_rx_configuration.minimum_packet_size_1 = MIN_ETH_PKT_SIZE;
    bbh_rx_configuration.minimum_packet_size_2 = MIN_ETH_PKT_SIZE;
    bbh_rx_configuration.minimum_packet_size_3 = MIN_ETH_PKT_SIZE;


    bbh_rx_configuration.maximum_packet_size_0 = pDpiCfg->mtu_size;


    /* maximum_packet_size 1-3 are not in use */
    bbh_rx_configuration.maximum_packet_size_1 = pDpiCfg->mtu_size;
    bbh_rx_configuration.maximum_packet_size_2 = pDpiCfg->mtu_size;
    bbh_rx_configuration.maximum_packet_size_3 = pDpiCfg->mtu_size;

    bbh_rx_configuration.ih_ingress_buffers_bitmask = bbh_ih_ingress_buffers_bitmask[port_index];
    bbh_rx_configuration.packet_header_offset = DRV_RDD_IH_PACKET_HEADER_OFFSET;
    bbh_rx_configuration.reassembly_offset_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION(pDpiCfg->headroom_size);

    /* By default, the triggers for FC will be disabled and the triggers for drop enabled.
       If the user configures flow control for the port, the triggers for drop will be
       disabled and triggers for FC (including Runner request) will be enabled */
    bbh_rx_configuration.flow_control_triggers_bitmask = 0;
    bbh_rx_configuration.drop_triggers_bitmask = DRV_BBH_RX_DROP_TRIGGER_BPM_IS_IN_EXCLUSIVE_STATE | DRV_BBH_RX_DROP_TRIGGER_SBPM_IS_IN_EXCLUSIVE_STATE;

    /* following configuration is irrelevant in EMAC case */
    bbh_rx_configuration.flows_32_255_group_divider = BBH_RX_FLOWS_32_255_GROUP_DIVIDER;
    bbh_rx_configuration.ploam_default_ih_class = 0;
    bbh_rx_configuration.ploam_ih_class_override = 0;

    fi_bl_drv_bbh_rx_set_configuration(port_index, &bbh_rx_configuration);

    /* bbh_rx_set_per_flow_configuration */
    per_flow_configuration.minimum_packet_size_selection = BBH_RX_ETH_MIN_PKT_SIZE_SELECTION_INDEX;
    per_flow_configuration.maximum_packet_size_selection = BBH_RX_MAX_PKT_SIZE_SELECTION_INDEX;

    if (( pDpiCfg->wanType == WAN_TYPE_RGMII && port_index == DRV_BBH_EMAC_4) ||
    	(pDpiCfg->wanType == WAN_TYPE_AE && port_index == DRV_BBH_EMAC_5) )
    {
        per_flow_configuration.default_ih_class = DRV_RDD_IH_CLASS_WAN_BRIDGED_LOW_INDEX;
        per_flow_configuration.ih_class_override = 1;
    }
    else
    {
        per_flow_configuration.ih_class_override = 0;

        switch (port_index)
        {
        case DRV_BBH_EMAC_0:
            per_flow_configuration.default_ih_class = DRV_RDD_IH_CLASS_LAN_BRIDGED_ETH0_INDEX;
            break;
        case DRV_BBH_EMAC_1:
            per_flow_configuration.default_ih_class = DRV_RDD_IH_CLASS_LAN_BRIDGED_ETH1_INDEX;
            break;
        case DRV_BBH_EMAC_2:
            per_flow_configuration.default_ih_class = DRV_RDD_IH_CLASS_LAN_BRIDGED_ETH2_INDEX;
            break;
        case DRV_BBH_EMAC_3:
            per_flow_configuration.default_ih_class = DRV_RDD_IH_CLASS_LAN_BRIDGED_ETH3_INDEX;
            break;
        case DRV_BBH_EMAC_4:
            /* if we are here, emac 4 is not the WAN */
            per_flow_configuration.default_ih_class = DRV_RDD_IH_CLASS_LAN_BRIDGED_ETH4_INDEX;
            break;
            /*yoni itah: the next situation is impossible in Oren, AE port is never lan port*/
//        case DRV_BBH_EMAC_5:
//            /* if we are here, we are in GBE mode and emac 5 is not the WAN, meaning emac 4 is the WAN.
//               in this case emac 5 will have the class LAN_BRIDGED_ETH4 */
//            per_flow_configuration.default_ih_class = DRV_RDD_IH_CLASS_LAN_BRIDGED_ETH4_INDEX;
//            break;
        default:
            return;
            break;
        }
    }

    /* in EMAC, only flow 0 is relevant */
    fi_bl_drv_bbh_rx_set_per_flow_configuration(port_index, 0, &per_flow_configuration);

    return;
}
/* this function configures all 8 ingress queues in IH */
static int fi_ih_configure_ingress_queues(void)
{
    int i;
    DRV_IH_INGRESS_QUEUE_CONFIG ingress_queue_config;
    uint8_t base_location = 0;

    /* queues of EMACS 0-3 */
    for (i = IH_INGRESS_QUEUE_0_ETH0; i <= IH_INGRESS_QUEUE_3_ETH3; i++)
    {
        ingress_queue_config.base_location = base_location;
        ingress_queue_config.size = IH_INGRESS_QUEUE_SIZE_LAN_EMACS;
        ingress_queue_config.priority = IH_INGRESS_QUEUE_PRIORITY_LAN_EMACS;
        ingress_queue_config.weight = IH_INGRESS_QUEUE_WEIGHT_LAN_EMACS;
        ingress_queue_config.congestion_threshold = IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_LAN_EMACS;

        fi_bl_drv_ih_configure_ingress_queue(i, &ingress_queue_config);

        /* update the correspoding bitmask in database, to be configured in BBH */
        f_update_bbh_ih_ingress_buffers_bitmask((DRV_BBH_PORT_INDEX)i, ingress_queue_config.base_location, ingress_queue_config.size);

        base_location += ingress_queue_config.size;
    }

    /* queues of EMAC 4 and GPON/EMAC 5: */
    if (pDpiCfg->wanType == WAN_TYPE_RGMII)
    {
        /* queues of EMAC 4 */
        ingress_queue_config.base_location = base_location;
        ingress_queue_config.size = IH_INGRESS_QUEUE_SIZE_WAN;
        ingress_queue_config.priority = IH_INGRESS_QUEUE_PRIORITY_WAN;
        ingress_queue_config.weight = IH_INGRESS_QUEUE_WEIGHT_WAN;
        ingress_queue_config.congestion_threshold = IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_WAN;

        fi_bl_drv_ih_configure_ingress_queue(IH_INGRESS_QUEUE_4_ETH4, &ingress_queue_config);


        /* update the correspoding bitmask in database, to be configured in BBH */
        f_update_bbh_ih_ingress_buffers_bitmask(DRV_BBH_EMAC_4, ingress_queue_config.base_location, ingress_queue_config.size);

        base_location += ingress_queue_config.size;

        /* queues of EMAC 5 (it cannot be GPON in this case) */
        ingress_queue_config.base_location = base_location;
        ingress_queue_config.size = IH_INGRESS_QUEUE_SIZE_LAN_EMACS;
        ingress_queue_config.priority = IH_INGRESS_QUEUE_PRIORITY_LAN_EMACS;
        ingress_queue_config.weight = IH_INGRESS_QUEUE_WEIGHT_LAN_EMACS;
        ingress_queue_config.congestion_threshold = IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_LAN_EMACS;

        fi_bl_drv_ih_configure_ingress_queue(IH_INGRESS_QUEUE_5_GPON_OR_AE, &ingress_queue_config);

        /* update the correspoding bitmask in database, to be configured in BBH */
        f_update_bbh_ih_ingress_buffers_bitmask(DRV_BBH_EMAC_5, ingress_queue_config.base_location, ingress_queue_config.size);

        base_location += ingress_queue_config.size;
    }
    else
    {
        /* queues of EMAC 4 */
        ingress_queue_config.base_location = base_location;
        ingress_queue_config.size = IH_INGRESS_QUEUE_SIZE_LAN_EMACS;
        ingress_queue_config.priority = IH_INGRESS_QUEUE_PRIORITY_LAN_EMACS;
        ingress_queue_config.weight = IH_INGRESS_QUEUE_WEIGHT_LAN_EMACS;
        ingress_queue_config.congestion_threshold = IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_LAN_EMACS;

        fi_bl_drv_ih_configure_ingress_queue(IH_INGRESS_QUEUE_4_ETH4, &ingress_queue_config);

        /* update the correspoding bitmask in database, to be configured in BBH */
        f_update_bbh_ih_ingress_buffers_bitmask(DRV_BBH_EMAC_4, ingress_queue_config.base_location,
            ingress_queue_config.size);

        base_location += ingress_queue_config.size;

        /* queues of GPON/EMAC 5 */
        ingress_queue_config.base_location = base_location;
        ingress_queue_config.size = IH_INGRESS_QUEUE_SIZE_WAN;
        ingress_queue_config.priority = IH_INGRESS_QUEUE_PRIORITY_WAN;
        ingress_queue_config.weight = IH_INGRESS_QUEUE_WEIGHT_WAN;
        ingress_queue_config.congestion_threshold = IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_WAN;

        fi_bl_drv_ih_configure_ingress_queue(IH_INGRESS_QUEUE_5_GPON_OR_AE, &ingress_queue_config);

        /* TODO: add Active ethernet case when it will be supported */
        if ( pDpiCfg->wanType == WAN_TYPE_GPON)
        {
            f_update_bbh_ih_ingress_buffers_bitmask(DRV_BBH_GPON, ingress_queue_config.base_location,
                    ingress_queue_config.size);
        }
        else if ( pDpiCfg->wanType == WAN_TYPE_EPON)
        {
            f_update_bbh_ih_ingress_buffers_bitmask(DRV_BBH_EPON, ingress_queue_config.base_location,
                    ingress_queue_config.size);
        }

        base_location += ingress_queue_config.size;
    }

    /* queues of runners */
    for (i = IH_INGRESS_QUEUE_6_RUNNER_A; i <= IH_INGRESS_QUEUE_6_RUNNER_A; i++)
    {
        ingress_queue_config.base_location = base_location;
        ingress_queue_config.size = IH_INGRESS_QUEUE_SIZE_RUNNERS;
        ingress_queue_config.priority = IH_INGRESS_QUEUE_PRIORITY_RUNNERS;
        ingress_queue_config.weight = IH_INGRESS_QUEUE_WEIGHT_RUNNERS;
        ingress_queue_config.congestion_threshold = IH_INGRESS_QUEUE_CONGESTION_THRESHOLD_RUNNERS;

        fi_bl_drv_ih_configure_ingress_queue (i, &ingress_queue_config);

        base_location += ingress_queue_config.size;
    }

    /* check that we didn't overrun */
    if (base_location > DRV_IH_INGRESS_QUEUES_ARRAY_SIZE)
    {
        /* sum of sizes exceeded the total array size */
        return DPI_RC_ERROR;
    }
    return DPI_RC_OK;
}


/* this function configures the IH classes which are in use */
static void f_ih_configure_lookup_tables(void)
{
    DRV_IH_LOOKUP_TABLE_60_BIT_KEY_CONFIG lookup_table_60_bit_key_config ;

    /*** table 1: MAC DA ***/
    lookup_table_60_bit_key_config.table_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_LOOKUP_TABLE_1_BASE_ADDRESS);
    lookup_table_60_bit_key_config.table_size = DRV_RDD_IH_LOOKUP_TABLE_1_SIZE;
    lookup_table_60_bit_key_config.maximal_search_depth = DRV_RDD_IH_LOOKUP_TABLE_1_SEARCH_DEPTH;
    lookup_table_60_bit_key_config.hash_type = DRV_RDD_IH_LOOKUP_TABLE_1_HASH_TYPE;
    lookup_table_60_bit_key_config.sa_search_enable = DRV_RDD_IH_LOOKUP_TABLE_1_SA_ENABLE;
    lookup_table_60_bit_key_config.aging_enable = DRV_RDD_IH_LOOKUP_TABLE_1_AGING_ENABLE;
    lookup_table_60_bit_key_config.cam_enable = DRV_RDD_IH_LOOKUP_TABLE_1_CAM_ENABLE;
    lookup_table_60_bit_key_config.cam_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_LOOKUP_TABLE_1_CAM_BASE_ADDRESS);
    lookup_table_60_bit_key_config.context_table_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_CONTEXT_TABLE_1_BASE_ADDRESS);
    lookup_table_60_bit_key_config.context_table_entry_size = DRV_RDD_IH_CONTEXT_TABLE_1_ENTRY_SIZE;
    lookup_table_60_bit_key_config.cam_context_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_CONTEXT_TABLE_1_CAM_BASE_ADDRESS);
    lookup_table_60_bit_key_config.part_0_start_offset_in_4_byte = DRV_RDD_IH_LOOKUP_TABLE_1_DST_MAC_KEY_PART_0_START_OFFSET;
    lookup_table_60_bit_key_config.part_0_shift_offset_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_1_DST_MAC_KEY_PART_0_SHIFT;
    lookup_table_60_bit_key_config.part_1_start_offset_in_4_byte = DRV_RDD_IH_LOOKUP_TABLE_1_DST_MAC_KEY_PART_1_START_OFFSET;
    lookup_table_60_bit_key_config.part_1_shift_offset_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_1_DST_MAC_KEY_PART_1_SHIFT;
    lookup_table_60_bit_key_config.key_extension = DRV_RDD_IH_LOOKUP_TABLE_1_DST_MAC_KEY_KEY_EXTENSION;
    lookup_table_60_bit_key_config.part_0_mask_low = DRV_RDD_IH_LOOKUP_TABLE_1_DST_MAC_KEY_PART_0_MASK_LOW;
    lookup_table_60_bit_key_config.part_0_mask_high = DRV_RDD_IH_LOOKUP_TABLE_1_DST_MAC_KEY_PART_0_MASK_HIGH;
    lookup_table_60_bit_key_config.part_1_mask_low = DRV_RDD_IH_LOOKUP_TABLE_1_DST_MAC_KEY_PART_1_MASK_LOW;
    lookup_table_60_bit_key_config.part_1_mask_high = DRV_RDD_IH_LOOKUP_TABLE_1_DST_MAC_KEY_PART_1_MASK_HIGH;
    lookup_table_60_bit_key_config.global_mask_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_1_DST_MAC_KEY_GLOBAL_MASK;

    fi_bl_drv_ih_configure_lut_60_bit_key(DRV_RDD_IH_LOOKUP_TABLE_MAC_DA_INDEX, &lookup_table_60_bit_key_config);

    //TODO:move that function to RDPA_System
    /*** table 2: IPTV ***/
    //rdpa_ih_cfg_iptv_lookup_table(iptv_lookup_method_mac);

    /*** table 3: DS ingress classification ***/
    lookup_table_60_bit_key_config.table_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_LOOKUP_TABLE_3_BASE_ADDRESS);
    lookup_table_60_bit_key_config.table_size = DRV_RDD_IH_LOOKUP_TABLE_3_SIZE;
    lookup_table_60_bit_key_config.maximal_search_depth = DRV_RDD_IH_LOOKUP_TABLE_3_SEARCH_DEPTH;
    lookup_table_60_bit_key_config.hash_type = DRV_RDD_IH_LOOKUP_TABLE_3_HASH_TYPE;
    lookup_table_60_bit_key_config.sa_search_enable = DRV_RDD_IH_LOOKUP_TABLE_3_SA_ENABLE;
    lookup_table_60_bit_key_config.aging_enable = DRV_RDD_IH_LOOKUP_TABLE_3_AGING_ENABLE;
    lookup_table_60_bit_key_config.cam_enable = DRV_RDD_IH_LOOKUP_TABLE_3_CAM_ENABLE;
    lookup_table_60_bit_key_config.cam_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_LOOKUP_TABLE_3_CAM_BASE_ADDRESS);
    lookup_table_60_bit_key_config.context_table_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_CONTEXT_TABLE_3_BASE_ADDRESS);
    lookup_table_60_bit_key_config.context_table_entry_size = DRV_RDD_IH_CONTEXT_TABLE_3_ENTRY_SIZE;
    lookup_table_60_bit_key_config.cam_context_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_CONTEXT_TABLE_3_CAM_BASE_ADDRESS);
    lookup_table_60_bit_key_config.part_0_start_offset_in_4_byte = DRV_RDD_IH_LOOKUP_TABLE_3_VID_KEY_PART_0_START_OFFSET;
    lookup_table_60_bit_key_config.part_0_shift_offset_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_3_VID_KEY_PART_0_SHIFT;
    lookup_table_60_bit_key_config.part_1_start_offset_in_4_byte = DRV_RDD_IH_LOOKUP_TABLE_3_VID_KEY_PART_1_START_OFFSET;
    lookup_table_60_bit_key_config.part_1_shift_offset_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_3_VID_KEY_PART_1_SHIFT;
    lookup_table_60_bit_key_config.key_extension = DRV_RDD_IH_LOOKUP_TABLE_3_VID_KEY_KEY_EXTENSION;
    lookup_table_60_bit_key_config.part_0_mask_low = DRV_RDD_IH_LOOKUP_TABLE_3_VID_KEY_PART_0_MASK_LOW;
    lookup_table_60_bit_key_config.part_0_mask_high = DRV_RDD_IH_LOOKUP_TABLE_3_VID_KEY_PART_0_MASK_HIGH;
    lookup_table_60_bit_key_config.part_1_mask_low = DRV_RDD_IH_LOOKUP_TABLE_3_VID_KEY_PART_1_MASK_LOW;
    lookup_table_60_bit_key_config.part_1_mask_high = DRV_RDD_IH_LOOKUP_TABLE_3_VID_KEY_PART_1_MASK_HIGH;
    lookup_table_60_bit_key_config.global_mask_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_3_VID_KEY_GLOBAL_MASK;

    fi_bl_drv_ih_configure_lut_60_bit_key(DRV_RDD_IH_LOOKUP_TABLE_DS_INGRESS_CLASSIFICATION_INDEX, &lookup_table_60_bit_key_config);

    /*** table 4: US ingress classification 1 per LAN port ***/
    lookup_table_60_bit_key_config.table_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_LOOKUP_TABLE_4_BASE_ADDRESS);
    lookup_table_60_bit_key_config.table_size = DRV_RDD_IH_LOOKUP_TABLE_4_SIZE;
    lookup_table_60_bit_key_config.maximal_search_depth = DRV_RDD_IH_LOOKUP_TABLE_4_SEARCH_DEPTH;
    lookup_table_60_bit_key_config.hash_type = DRV_RDD_IH_LOOKUP_TABLE_4_HASH_TYPE;
    lookup_table_60_bit_key_config.sa_search_enable = DRV_RDD_IH_LOOKUP_TABLE_4_SA_ENABLE;
    lookup_table_60_bit_key_config.aging_enable = DRV_RDD_IH_LOOKUP_TABLE_4_AGING_ENABLE;
    lookup_table_60_bit_key_config.cam_enable = DRV_RDD_IH_LOOKUP_TABLE_4_CAM_ENABLE;
    lookup_table_60_bit_key_config.cam_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_LOOKUP_TABLE_4_CAM_BASE_ADDRESS);
    lookup_table_60_bit_key_config.context_table_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_CONTEXT_TABLE_4_BASE_ADDRESS);
    lookup_table_60_bit_key_config.context_table_entry_size = DRV_RDD_IH_CONTEXT_TABLE_4_ENTRY_SIZE;
    lookup_table_60_bit_key_config.cam_context_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_CONTEXT_TABLE_4_CAM_BASE_ADDRESS);
    lookup_table_60_bit_key_config.part_0_start_offset_in_4_byte = DRV_RDD_IH_LOOKUP_TABLE_4_VID_SRC_PORT_KEY_PART_0_START_OFFSET;
    lookup_table_60_bit_key_config.part_0_shift_offset_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_4_VID_SRC_PORT_KEY_PART_0_SHIFT;
    lookup_table_60_bit_key_config.part_1_start_offset_in_4_byte = DRV_RDD_IH_LOOKUP_TABLE_4_VID_SRC_PORT_KEY_PART_1_START_OFFSET;
    lookup_table_60_bit_key_config.part_1_shift_offset_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_4_VID_SRC_PORT_KEY_PART_1_SHIFT;
    lookup_table_60_bit_key_config.key_extension = DRV_RDD_IH_LOOKUP_TABLE_4_VID_SRC_PORT_KEY_KEY_EXTENSION;
    lookup_table_60_bit_key_config.part_0_mask_low = DRV_RDD_IH_LOOKUP_TABLE_4_VID_SRC_PORT_KEY_PART_0_MASK_LOW;
    lookup_table_60_bit_key_config.part_0_mask_high = DRV_RDD_IH_LOOKUP_TABLE_4_VID_SRC_PORT_KEY_PART_0_MASK_HIGH;
    lookup_table_60_bit_key_config.part_1_mask_low = DRV_RDD_IH_LOOKUP_TABLE_4_VID_SRC_PORT_KEY_PART_1_MASK_LOW;
    lookup_table_60_bit_key_config.part_1_mask_high = DRV_RDD_IH_LOOKUP_TABLE_4_VID_SRC_PORT_KEY_PART_1_MASK_HIGH;
    lookup_table_60_bit_key_config.global_mask_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_4_VID_SRC_PORT_KEY_GLOBAL_MASK;

    fi_bl_drv_ih_configure_lut_60_bit_key(DRV_RDD_IH_LOOKUP_TABLE_US_INGRESS_CLASSIFICATION_INDEX, &lookup_table_60_bit_key_config);

    /*** table 6: IPTV source IP ***/
    lookup_table_60_bit_key_config.table_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_LOOKUP_TABLE_6_BASE_ADDRESS);
    lookup_table_60_bit_key_config.table_size = DRV_RDD_IH_LOOKUP_TABLE_6_SIZE;
    lookup_table_60_bit_key_config.maximal_search_depth = DRV_RDD_IH_LOOKUP_TABLE_6_SEARCH_DEPTH;
    lookup_table_60_bit_key_config.hash_type = DRV_RDD_IH_LOOKUP_TABLE_6_HASH_TYPE;
    lookup_table_60_bit_key_config.sa_search_enable = DRV_RDD_IH_LOOKUP_TABLE_6_SA_ENABLE;
    lookup_table_60_bit_key_config.aging_enable = DRV_RDD_IH_LOOKUP_TABLE_6_AGING_ENABLE;
    lookup_table_60_bit_key_config.cam_enable = DRV_RDD_IH_LOOKUP_TABLE_6_CAM_ENABLE;
    lookup_table_60_bit_key_config.cam_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_LOOKUP_TABLE_6_CAM_BASE_ADDRESS);
    lookup_table_60_bit_key_config.context_table_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_CONTEXT_TABLE_6_BASE_ADDRESS);
    lookup_table_60_bit_key_config.context_table_entry_size = DRV_RDD_IH_CONTEXT_TABLE_6_ENTRY_SIZE;
    lookup_table_60_bit_key_config.cam_context_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_CONTEXT_TABLE_6_CAM_BASE_ADDRESS);
    lookup_table_60_bit_key_config.part_0_start_offset_in_4_byte = DRV_RDD_IH_LOOKUP_TABLE_6_KEY_PART_0_START_OFFSET;
    lookup_table_60_bit_key_config.part_0_shift_offset_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_6_KEY_PART_0_SHIFT;
    lookup_table_60_bit_key_config.part_1_start_offset_in_4_byte = DRV_RDD_IH_LOOKUP_TABLE_6_KEY_PART_1_START_OFFSET;
    lookup_table_60_bit_key_config.part_1_shift_offset_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_6_KEY_PART_1_SHIFT;
    lookup_table_60_bit_key_config.key_extension = DRV_RDD_IH_LOOKUP_TABLE_6_KEY_KEY_EXTENSION;
    lookup_table_60_bit_key_config.part_0_mask_low = DRV_RDD_IH_LOOKUP_TABLE_6_KEY_PART_0_MASK_LOW;
    lookup_table_60_bit_key_config.part_0_mask_high = DRV_RDD_IH_LOOKUP_TABLE_6_KEY_PART_0_MASK_HIGH;
    lookup_table_60_bit_key_config.part_1_mask_low = DRV_RDD_IH_LOOKUP_TABLE_6_KEY_PART_1_MASK_LOW;
    lookup_table_60_bit_key_config.part_1_mask_high = DRV_RDD_IH_LOOKUP_TABLE_6_KEY_PART_1_MASK_HIGH;
    lookup_table_60_bit_key_config.global_mask_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_6_KEY_GLOBAL_MASK;

    fi_bl_drv_ih_configure_lut_60_bit_key(DRV_RDD_IH_LOOKUP_TABLE_IPTV_SRC_IP_INDEX, &lookup_table_60_bit_key_config);

    /*** table 9: MAC SA ***/
    lookup_table_60_bit_key_config.table_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_LOOKUP_TABLE_9_BASE_ADDRESS);
    lookup_table_60_bit_key_config.table_size = DRV_RDD_IH_LOOKUP_TABLE_9_SIZE;
    lookup_table_60_bit_key_config.maximal_search_depth = DRV_RDD_IH_LOOKUP_TABLE_9_SEARCH_DEPTH;
    lookup_table_60_bit_key_config.hash_type = DRV_RDD_IH_LOOKUP_TABLE_9_HASH_TYPE;
    lookup_table_60_bit_key_config.sa_search_enable = DRV_RDD_IH_LOOKUP_TABLE_9_SA_ENABLE;
    lookup_table_60_bit_key_config.aging_enable = DRV_RDD_IH_LOOKUP_TABLE_9_AGING_ENABLE;
    lookup_table_60_bit_key_config.cam_enable = DRV_RDD_IH_LOOKUP_TABLE_9_CAM_ENABLE;
    lookup_table_60_bit_key_config.cam_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_LOOKUP_TABLE_9_CAM_BASE_ADDRESS);
    lookup_table_60_bit_key_config.context_table_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_CONTEXT_TABLE_9_BASE_ADDRESS);
    lookup_table_60_bit_key_config.context_table_entry_size = DRV_RDD_IH_CONTEXT_TABLE_9_ENTRY_SIZE;
    lookup_table_60_bit_key_config.cam_context_base_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION (DRV_RDD_IH_CONTEXT_TABLE_9_CAM_BASE_ADDRESS);
    lookup_table_60_bit_key_config.part_0_start_offset_in_4_byte = DRV_RDD_IH_LOOKUP_TABLE_9_SRC_MAC_KEY_PART_0_START_OFFSET;
    lookup_table_60_bit_key_config.part_0_shift_offset_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_9_SRC_MAC_KEY_PART_0_SHIFT;
    lookup_table_60_bit_key_config.part_1_start_offset_in_4_byte = DRV_RDD_IH_LOOKUP_TABLE_9_SRC_MAC_KEY_PART_1_START_OFFSET;
    lookup_table_60_bit_key_config.part_1_shift_offset_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_9_SRC_MAC_KEY_PART_1_SHIFT;
    lookup_table_60_bit_key_config.key_extension = DRV_RDD_IH_LOOKUP_TABLE_9_SRC_MAC_KEY_KEY_EXTENSION;
    lookup_table_60_bit_key_config.part_0_mask_low = DRV_RDD_IH_LOOKUP_TABLE_9_SRC_MAC_KEY_PART_0_MASK_LOW;
    lookup_table_60_bit_key_config.part_0_mask_high = DRV_RDD_IH_LOOKUP_TABLE_9_SRC_MAC_KEY_PART_0_MASK_HIGH;
    lookup_table_60_bit_key_config.part_1_mask_low = DRV_RDD_IH_LOOKUP_TABLE_9_SRC_MAC_KEY_PART_1_MASK_LOW;
    lookup_table_60_bit_key_config.part_1_mask_high = DRV_RDD_IH_LOOKUP_TABLE_9_SRC_MAC_KEY_PART_1_MASK_HIGH;
    lookup_table_60_bit_key_config.global_mask_in_4_bit = DRV_RDD_IH_LOOKUP_TABLE_9_SRC_MAC_KEY_GLOBAL_MASK;

    fi_bl_drv_ih_configure_lut_60_bit_key(DRV_RDD_IH_LOOKUP_TABLE_MAC_SA_INDEX,
        &lookup_table_60_bit_key_config);
}
static uint32_t is_matrix_source_lan(DRV_IH_TARGET_MATRIX_SOURCE_PORT source_port)
{

    if ( (pDpiCfg->wanType == WAN_TYPE_RGMII && source_port < DRV_IH_TARGET_MATRIX_SOURCE_PORT_ETH4 )||
       ( (pDpiCfg->wanType == WAN_TYPE_GPON ||pDpiCfg->wanType == WAN_TYPE_EPON || pDpiCfg->wanType == WAN_TYPE_AE)
    		   && source_port < DRV_IH_TARGET_MATRIX_SOURCE_PORT_GPON ))
    {
        return 1;
    }
    return 0;
}

static uint32_t is_matrix_dest_lan(DRV_IH_TARGET_MATRIX_SOURCE_PORT source_port)
{
    if (source_port <= DRV_IH_TARGET_MATRIX_SOURCE_PORT_ETH3 ||
        (source_port == DRV_IH_TARGET_MATRIX_SOURCE_PORT_ETH4 && !pDpiCfg->wanType==WAN_TYPE_RGMII ))
    {
        return 1;
    }
    return 0;
}

static uint32_t is_matrix_source_wan(DRV_IH_TARGET_MATRIX_SOURCE_PORT source_port)
{
    if ((pDpiCfg->wanType==WAN_TYPE_GPON && source_port == DRV_IH_TARGET_MATRIX_SOURCE_PORT_GPON) ||
        (pDpiCfg->wanType==WAN_TYPE_RGMII && source_port == DRV_IH_TARGET_MATRIX_SOURCE_PORT_ETH4))
    {
        return 1;
    }
    return 0;
}
/* calculates the "direct_mode" value for target matrix */
static uint32_t f_ih_calculate_direct_mode(DRV_IH_TARGET_MATRIX_SOURCE_PORT source_port,
									DRV_IH_TARGET_MATRIX_DESTINATION_PORT destination_port)
{
    /* lan -> lan/pci/multicast: direct mode should be set */
    if (is_matrix_source_lan(source_port) &&
        (is_matrix_dest_lan(destination_port) ||
        destination_port == DRV_IH_TARGET_MATRIX_DESTINATION_PORT_PCIE0 ||
        destination_port == DRV_IH_TARGET_MATRIX_DESTINATION_PORT_MULTICAST) &&
        (DRV_IH_TARGET_MATRIX_DESTINATION_PORT)source_port != destination_port)
    {
        return 1;
    }
    /* pci -> lan/pci: direct mode should be set */
    else if (source_port == DRV_IH_TARGET_MATRIX_SOURCE_PORT_PCIE0 &&
        (is_matrix_dest_lan(destination_port) || destination_port == DRV_IH_TARGET_MATRIX_DESTINATION_PORT_PCIE0))
    {
        return 1;
    }
    return 0;
}
static void sbpm_drv_init(void)
{
#define SBPM_WAN_LAN_GROUP(emac) ((BBH_PORT_IS_WAN(pDpiCfg->wanType, emac)) ? \
        DRV_SBPM_USER_GROUP_0 : DRV_SBPM_USER_GROUP_1)
    DRV_SBPM_GLOBAL_CONFIGURATION sbpm_global_configuration;
    DRV_SBPM_USER_GROUPS_THRESHOLDS sbpm_ug_configuration;
    int i;

    sbpm_global_configuration.hysteresis = SBPM_DEFAULT_HYSTERESIS;
    sbpm_global_configuration.threshold = SBPM_DEFAULT_THRESHOLD;

    for (i = 0; i < ARRAY_LENGTH(sbpm_ug_configuration.ug_arr); i++)
    {
        sbpm_ug_configuration.ug_arr[i].hysteresis = SBPM_DEFAULT_HYSTERESIS;
        sbpm_ug_configuration.ug_arr[i].threshold = SBPM_DEFAULT_THRESHOLD;
        sbpm_ug_configuration.ug_arr[i].exclusive_hysteresis = SBPM_DEFAULT_HYSTERESIS;
        sbpm_ug_configuration.ug_arr[i].exclusive_threshold = SBPM_DEFAULT_THRESHOLD;
    }
    fi_bl_drv_sbpm_init(SBPM_BASE_ADDRESS, SBPM_LIST_SIZE, SBPM_REPLY_ADDRESS, &sbpm_global_configuration,
        &sbpm_ug_configuration);

    fi_bl_drv_sbpm_set_user_group_mapping(DRV_SBPM_SP_GPON, DRV_SBPM_USER_GROUP_0);
    fi_bl_drv_sbpm_set_user_group_mapping(DRV_SBPM_SP_RNR_A, DRV_SBPM_USER_GROUP_0);
    fi_bl_drv_sbpm_set_user_group_mapping(DRV_SBPM_SP_EMAC0, SBPM_WAN_LAN_GROUP(DRV_BBH_EMAC_0));
    fi_bl_drv_sbpm_set_user_group_mapping(DRV_SBPM_SP_EMAC1, SBPM_WAN_LAN_GROUP(DRV_BBH_EMAC_1));
    fi_bl_drv_sbpm_set_user_group_mapping(DRV_SBPM_SP_EMAC2, SBPM_WAN_LAN_GROUP(DRV_BBH_EMAC_2));
    fi_bl_drv_sbpm_set_user_group_mapping(DRV_SBPM_SP_EMAC3, SBPM_WAN_LAN_GROUP(DRV_BBH_EMAC_3));
    fi_bl_drv_sbpm_set_user_group_mapping(DRV_SBPM_SP_EMAC4, SBPM_WAN_LAN_GROUP(DRV_BBH_EMAC_4));
    fi_bl_drv_sbpm_set_user_group_mapping(DRV_SBPM_SP_PCI0, DRV_SBPM_USER_GROUP_1);
    fi_bl_drv_sbpm_set_user_group_mapping(DRV_SBPM_SP_RNR_B, DRV_SBPM_USER_GROUP_1);
    fi_bl_drv_sbpm_set_user_group_mapping(DRV_SBPM_SP_SPARE_0, DRV_SBPM_USER_GROUP_6);
    fi_bl_drv_sbpm_set_user_group_mapping(DRV_SBPM_SP_MIPS_C, DRV_SBPM_USER_GROUP_7);
}

static void bpm_drv_init(void)
{
#define BPM_WAN_LAN_GROUP(emac) ((BBH_PORT_IS_WAN(pDpiCfg->wanType, emac)) ? \
        DRV_BPM_USER_GROUP_0 : DRV_BPM_USER_GROUP_1)
    DRV_BPM_GLOBAL_CONFIGURATION bpm_global_configuration;
    DRV_BPM_USER_GROUPS_THRESHOLDS bpm_ug_configuration;
    DRV_BPM_RUNNER_MSG_CTRL_PARAMS runner_msg_ctrl_params;
    int i;

    bpm_global_configuration.hysteresis = BPM_DEFAULT_HYSTERESIS;
    bpm_global_configuration.threshold = DRV_BPM_GLOBAL_THRESHOLD_7_5K;
    for (i = 0; i < ARRAY_LENGTH(bpm_ug_configuration.ug_arr); i++)
    {
        bpm_ug_configuration.ug_arr[i].hysteresis = BPM_DEFAULT_HYSTERESIS;
        bpm_ug_configuration.ug_arr[i].threshold = 64;
        bpm_ug_configuration.ug_arr[i].exclusive_hysteresis = BPM_DEFAULT_HYSTERESIS;
        bpm_ug_configuration.ug_arr[i].exclusive_threshold = 64;
    }
        
    bpm_ug_configuration.ug_arr[DRV_BPM_USER_GROUP_0].threshold = BPM_DEFAULT_DS_THRESHOLD;
    bpm_ug_configuration.ug_arr[DRV_BPM_USER_GROUP_0].exclusive_threshold = BPM_DEFAULT_DS_THRESHOLD;
    bpm_ug_configuration.ug_arr[DRV_BPM_USER_GROUP_1].hysteresis = 128;
    bpm_ug_configuration.ug_arr[DRV_BPM_USER_GROUP_1].exclusive_hysteresis= 128;
    bpm_ug_configuration.ug_arr[DRV_BPM_USER_GROUP_1].threshold = 3840;
    bpm_ug_configuration.ug_arr[DRV_BPM_USER_GROUP_1].exclusive_threshold = 4096;
    bpm_ug_configuration.ug_arr[DRV_BPM_USER_GROUP_6].threshold = BPM_DEFAULT_DS_THRESHOLD;
    bpm_ug_configuration.ug_arr[DRV_BPM_USER_GROUP_6].exclusive_threshold= BPM_DEFAULT_DS_THRESHOLD;
    bpm_ug_configuration.ug_arr[DRV_BPM_USER_GROUP_7].threshold = BPM_CPU_NUMBER_OF_BUFFERS;
    bpm_ug_configuration.ug_arr[DRV_BPM_USER_GROUP_7].exclusive_threshold= BPM_CPU_NUMBER_OF_BUFFERS - 32;

    fi_bl_drv_bpm_init(&bpm_global_configuration, &bpm_ug_configuration, BPM_REPLY_RUNNER_A_ADDRESS);

    fi_bl_drv_bpm_get_runner_msg_ctrl(&runner_msg_ctrl_params);

    runner_msg_ctrl_params.runner_a_reply_target_address = (BPM_REPLY_RUNNER_A_ADDRESS + 0x10000) >> 3;
    runner_msg_ctrl_params.runner_b_reply_target_address = (BPM_REPLY_RUNNER_B_ADDRESS + 0x10000) >> 3;

    fi_bl_drv_bpm_set_runner_msg_ctrl(&runner_msg_ctrl_params);

    fi_bl_drv_bpm_set_user_group_mapping(DRV_BPM_SP_GPON, DRV_BPM_USER_GROUP_0);
    fi_bl_drv_bpm_set_user_group_mapping(DRV_BPM_SP_RNR_A, DRV_BPM_USER_GROUP_0);
    fi_bl_drv_bpm_set_user_group_mapping(DRV_BPM_SP_EMAC0, BPM_WAN_LAN_GROUP(DRV_BBH_EMAC_0));
    fi_bl_drv_bpm_set_user_group_mapping(DRV_BPM_SP_EMAC1, BPM_WAN_LAN_GROUP(DRV_BBH_EMAC_1));
    fi_bl_drv_bpm_set_user_group_mapping(DRV_BPM_SP_EMAC2, BPM_WAN_LAN_GROUP(DRV_BBH_EMAC_2));
    fi_bl_drv_bpm_set_user_group_mapping(DRV_BPM_SP_EMAC3, BPM_WAN_LAN_GROUP(DRV_BBH_EMAC_3));
    fi_bl_drv_bpm_set_user_group_mapping(DRV_BPM_SP_EMAC4, BPM_WAN_LAN_GROUP(DRV_BBH_EMAC_4));
    fi_bl_drv_bpm_set_user_group_mapping(DRV_BPM_SP_PCI0, DRV_BPM_USER_GROUP_1);
    fi_bl_drv_bpm_set_user_group_mapping(DRV_BPM_SP_RNR_B, DRV_BPM_USER_GROUP_1);
    fi_bl_drv_bpm_set_user_group_mapping(DRV_BPM_SP_SPARE_0, DRV_BPM_USER_GROUP_6);
    fi_bl_drv_bpm_set_user_group_mapping(DRV_BPM_SP_MIPS_C, DRV_BPM_USER_GROUP_7);
}

/* calculates the "forward" value for target matrix */
static uint32_t f_ih_calculate_forward(DRV_IH_TARGET_MATRIX_SOURCE_PORT source_port, DRV_IH_TARGET_MATRIX_DESTINATION_PORT destination_port)
{

    /* no self forwarding except PCI */
    if (source_port == DRV_IH_TARGET_MATRIX_SOURCE_PORT_PCIE0 &&
        destination_port == DRV_IH_TARGET_MATRIX_DESTINATION_PORT_PCIE0)
    {
        return 1;
    }
    else if ((DRV_IH_TARGET_MATRIX_DESTINATION_PORT)source_port == destination_port)
    {
        return 0;
    }
    /* no self forwarding (if emac 4 is wan, then its, sp will be emac 4 and its dest port will be gpon */
    else if (is_matrix_source_wan(source_port) && destination_port == DRV_IH_TARGET_MATRIX_DESTINATION_PORT_GPON)
    {
        return 0;
    }
    /* enable forwarding to LANs/WAN/PCI0 (PCI1 is not supported currently). */
    else if (destination_port <= DRV_IH_TARGET_MATRIX_DESTINATION_PORT_PCIE0)
    {
        return 1;
    }
    /* forward to "multicast" from WAN/LAN port */
    else if ((is_matrix_source_wan(source_port) || is_matrix_source_lan(source_port)) &&
        (destination_port == DRV_IH_TARGET_MATRIX_DESTINATION_PORT_MULTICAST))
    {
        return 1;
    }
    return 0;
}
/* calculates the "target_memory" value for target matrix */
static DRV_IH_TARGET_MEMORY f_ih_calculate_target_memory(DRV_IH_TARGET_MATRIX_SOURCE_PORT source_port,
    DRV_IH_TARGET_MATRIX_DESTINATION_PORT destination_port)
{
    if (is_matrix_source_lan(source_port) &&
        ((DRV_IH_TARGET_MATRIX_DESTINATION_PORT)source_port != destination_port &&
        (is_matrix_dest_lan(destination_port) ||
        destination_port == DRV_IH_TARGET_MATRIX_DESTINATION_PORT_MULTICAST)))
    {
        /* lan -> lan: target memory is SRAM */
        return DRV_IH_TARGET_MEMORY_SRAM;
    }
    else if (destination_port == DRV_IH_TARGET_MATRIX_DESTINATION_PORT_ALWAYS_SRAM)
    {
        return DRV_IH_TARGET_MEMORY_SRAM;
    }

    return DRV_IH_TARGET_MEMORY_DDR;
}

static void f_ih_configure_target_matrix(void)
{
    int i, j;
    DRV_IH_TARGET_MATRIX_PER_SP_CONFIG per_sp_config;

    for (i = DRV_IH_TARGET_MATRIX_SOURCE_PORT_ETH0; i < DRV_IH_TARGET_MATRIX_NUMBER_OF_SOURCE_PORTS; i++)
    {
        /* the destination ports after 'multicast' are not in use currently */
        for (j = DRV_IH_TARGET_MATRIX_DESTINATION_PORT_ETH0; j < DRV_IH_TARGET_MATRIX_NUMBER_OF_DESTINATION_PORTS; j++)
        {
            per_sp_config.entry[j].target_memory = f_ih_calculate_target_memory(i, j);
            per_sp_config.entry[j].direct_mode = f_ih_calculate_direct_mode(i, j);

            fi_bl_drv_ih_set_forward(i, j, f_ih_calculate_forward(i, j));
        }
        fi_bl_drv_ih_set_target_matrix(i, &per_sp_config);
    }
}

static void f_ih_configure_parser(void)
{
    DRV_IH_PARSER_CONFIG parser_config ;
#ifndef _CFE_
    uint8_t i;
#endif
    /* parser configuration */
    parser_config.tcp_flags = TCP_CTRL_FLAG_FIN | TCP_CTRL_FLAG_RST | TCP_CTRL_FLAG_SYN;
    parser_config.exception_status_bits = IH_PARSER_EXCEPTION_STATUS_BITS;
    parser_config.ppp_code_1_ipv6 = 1;
    parser_config.ipv6_extension_header_bitmask = 0;
    parser_config.snap_user_defined_organization_code = IH_REGS_PARSER_CORE_CONFIGURATION_SNAP_ORG_CODE_CODE_RESET_VALUE_RESET_VALUE;
    parser_config.snap_rfc1042_encapsulation_enable = IH_REGS_PARSER_CORE_CONFIGURATION_SNAP_ORG_CODE_EN_RFC1042_DISABLED_VALUE_RESET_VALUE;
    parser_config.snap_802_1q_encapsulation_enable = IH_REGS_PARSER_CORE_CONFIGURATION_SNAP_ORG_CODE_EN_8021Q_DISABLED_VALUE_RESET_VALUE;
    parser_config.gre_protocol = IH_REGS_PARSER_CORE_CONFIGURATION_GRE_PROTOCOL_CFG_GRE_PROTOCOL_PROTOCOL_VALUE_RESET_VALUE;

    /*  eng[15] = eng[16] = 1 for AH header detection (IPv4 and IPv6) */
    parser_config.eng_cfg = IH_PARSER_AH_DETECTION;

    fi_bl_drv_ih_configure_parser(&parser_config);
#ifndef _CFE_
    /* set PPP protocol code for IPv4 */
    fi_bl_drv_ih_set_ppp_code(0,IH_PARSER_PPP_PROTOCOL_CODE_0_IPV4);

    /* set PPP protocol code for IPv6 */
    fi_bl_drv_ih_set_ppp_code (1, IH_PARSER_PPP_PROTOCOL_CODE_1_IPV6);

    for ( i = IH_IP_L4_FILTER_USER_DEFINED_0 ; i <= IH_IP_L4_FILTER_USER_DEFINED_3 ; i ++ )
    {
           /* set a user-defined L4 Protocol to 255 */
            fi_bl_drv_ih_set_user_ip_l4_protocol ( i - IH_IP_L4_FILTER_USER_DEFINED_0 ,
        		   	   	   	   	   	   	   	   	   	   	   	   IH_L4_FILTER_DEF ) ;

    }
#endif
}

/* this function configures the IH classes which are in use */
static void f_ih_configure_classes(void)
{
    uint8_t i;

    for (i = 0; i < ARRAY_LENGTH(gs_ih_classes); i++)
    {
        fi_bl_drv_ih_configure_class(gs_ih_classes[i].class_index, &gs_ih_classes[i].class_config);
    }
}
/* Enable IPTV prefix filter in IH. */
#ifndef RDD_BASIC
void f_ih_cfg_mcast_prefix_filter_enable(void)
{
    DRV_IH_CLASSIFIER_CONFIG ih_classifier_config;
    uint8_t ipv4_mac_da_prefix[] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x00};
    uint8_t ipv4_mac_da_prefix_mask[] = {0xFF, 0xFF, 0xFF, 0x80, 0x00, 0x00};
    uint8_t ipv6_mac_da_prefix[] = {0x33, 0x33, 0x00, 0x00, 0x00, 0x00};
    uint8_t ipv6_mac_da_prefix_mask[] = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00};

    /* Clear all attributes */
    memset(&ih_classifier_config, 0, sizeof(DRV_IH_CLASSIFIER_CONFIG));

    /* We're using 2 DA filters with mask, 1 for IPv4 and 1 for IPv6 */
    fi_bl_drv_ih_set_da_filter_with_mask(IH_DA_FILTER_IPTV_IPV4, ipv4_mac_da_prefix, ipv4_mac_da_prefix_mask);

    fi_bl_drv_ih_set_da_filter_with_mask (IH_DA_FILTER_IPTV_IPV6, ipv6_mac_da_prefix, ipv6_mac_da_prefix_mask);

    /* Enable the IH DA filter - IPv4: 0x01005E */
    fi_bl_drv_ih_enable_da_filter(IH_DA_FILTER_IPTV_IPV4, 1);

    /* Enable the IH DA filter - IPv6: 0x3333 */
    fi_bl_drv_ih_enable_da_filter (IH_DA_FILTER_IPTV_IPV6, 1);

    /* When IPTV prefix filter is enabled: We want to override the class to IPTV class  */
    ih_classifier_config.da_filter_any_hit = 1;
    /* da_filter_hit set to 1 and da_filter_number mask is[110] because we're after filters 0-1 */
    ih_classifier_config.mask = IPTV_FILTER_MASK_DA;
    ih_classifier_config.resulting_class = DRV_RDD_IH_CLASS_IPTV_INDEX;
    ih_classifier_config.matched_da_filter = 0;

    /* Configure a classifier for IPTV */
    fi_bl_drv_ih_configure_classifier(IH_CLASSIFIER_IPTV, &ih_classifier_config);
}
#endif


#ifndef RDD_BASIC
/* this function configures the IH IPTV classifier
 * For GPON mode: create a classifier to perform class override from IPTV to BRIDGE_LOW if and only if the bcast filter
 * for iptv is enabled. */
void f_ih_cfg_iptv_lookup_classifier(void)
{
    DRV_IH_CLASSIFIER_CONFIG ih_classifier_config;

	/* Configure the ih IGMP classifier */
	memset(&ih_classifier_config, 0, sizeof(DRV_IH_CLASSIFIER_CONFIG));

	ih_classifier_config.mask                    = MASK_IH_CLASS_KEY_L4;
	ih_classifier_config.resulting_class         = DRV_RDD_IH_CLASS_WAN_BRIDGED_LOW_INDEX;
	ih_classifier_config.l4_protocol             = DRV_IH_L4_PROTOCOL_IGMP;
	/* Configure a classifier for broadcast traffic arriving on IPTV flow */
	fi_bl_drv_ih_configure_classifier(IH_CLASSIFIER_IGMP_IPTV, &ih_classifier_config);

	/* Configure the ih ICMPV6 classifier */
	memset(&ih_classifier_config, 0, sizeof(DRV_IH_CLASSIFIER_CONFIG));

	ih_classifier_config.mask                    = MASK_IH_CLASS_KEY_L4;
	ih_classifier_config.resulting_class         = DRV_RDD_IH_CLASS_WAN_BRIDGED_LOW_INDEX;
	ih_classifier_config.l4_protocol             = DRV_IH_L4_PROTOCOL_ICMPV6;
	/* Configure a classifier for broadcast traffic arriving on IPTV flow */
	fi_bl_drv_ih_configure_classifier(IH_CLASSIFIER_ICMPV6, &ih_classifier_config);
}
#endif


static void f_ih_init(void)
{
    DRV_IH_GENERAL_CONFIG ih_general_config ;
    DRV_IH_PACKET_HEADER_OFFSETS packet_header_offsets ;
    DRV_IH_RUNNER_BUFFERS_CONFIG runner_buffers_config ;
    DRV_IH_RUNNERS_LOAD_THRESHOLDS runners_load_thresholds ;
    DRV_IH_ROUTE_ADDRESSES xi_route_addresses ;
    DRV_IH_LOGICAL_PORTS_CONFIG logical_ports_config ;
    DRV_IH_SOURCE_PORT_TO_INGRESS_QUEUE_MAPPING source_port_to_ingress_queue_mapping ;
    DRV_IH_WAN_PORTS_CONFIG wan_ports_config ;
#ifndef _CFE_
    int i;
#endif
    /* general configuration */
    ih_general_config.runner_a_ih_response_address 					= MS_BYTE_TO_8_BYTE_RESOLUTION(DRV_RDD_IH_RUNNER_0_IH_RESPONSE_ADDRESS);
    ih_general_config.runner_b_ih_response_address 					= MS_BYTE_TO_8_BYTE_RESOLUTION(DRV_RDD_IH_RUNNER_1_IH_RESPONSE_ADDRESS);
    ih_general_config.runner_a_ih_congestion_report_address 		= MS_BYTE_TO_8_BYTE_RESOLUTION(DRV_RDD_IH_RUNNER_0_IH_CONGESTION_REPORT_ADDRESS);
    ih_general_config.runner_b_ih_congestion_report_address 		= MS_BYTE_TO_8_BYTE_RESOLUTION(DRV_RDD_IH_RUNNER_1_IH_CONGESTION_REPORT_ADDRESS);
    ih_general_config.runner_a_ih_congestion_report_enable 			= DRV_RDD_IH_RUNNER_0_IH_CONGESTION_REPORT_ENABLE;
    ih_general_config.runner_b_ih_congestion_report_enable 			= DRV_RDD_IH_RUNNER_1_IH_CONGESTION_REPORT_ENABLE;
    ih_general_config.lut_searches_enable_in_direct_mode 			= 1;
    ih_general_config.sn_stamping_enable_in_direct_mode 			= 1;
    ih_general_config.header_length_minimum 						= IH_HEADER_LENGTH_MIN;
    ih_general_config.congestion_discard_disable 					= 0;
    ih_general_config.cam_search_enable_upon_invalid_lut_entry		= DRV_RDD_IH_CAM_SEARCH_ENABLE_UPON_INVALID_LUT_ENTRY;

    fi_bl_drv_ih_set_general_configuration(&ih_general_config);

    /* packet header offsets configuration */
    packet_header_offsets.eth0_packet_header_offset 				= DRV_RDD_IH_PACKET_HEADER_OFFSET;
    packet_header_offsets.eth1_packet_header_offset 				= DRV_RDD_IH_PACKET_HEADER_OFFSET;
    packet_header_offsets.eth2_packet_header_offset 				= DRV_RDD_IH_PACKET_HEADER_OFFSET;
    packet_header_offsets.eth3_packet_header_offset 				= DRV_RDD_IH_PACKET_HEADER_OFFSET;
    packet_header_offsets.eth4_packet_header_offset 				= DRV_RDD_IH_PACKET_HEADER_OFFSET;
    packet_header_offsets.gpon_packet_header_offset 				= DRV_RDD_IH_PACKET_HEADER_OFFSET;
    packet_header_offsets.runner_a_packet_header_offset 			= DRV_RDD_IH_PACKET_HEADER_OFFSET;
    packet_header_offsets.runner_b_packet_header_offset 			= DRV_RDD_IH_PACKET_HEADER_OFFSET;

    fi_bl_drv_ih_set_packet_header_offsets(& packet_header_offsets);

    /* Runner Buffers configuration */
    /* same ih_managed_rb_base_address should be used for both runners */
    runner_buffers_config.runner_a_ih_managed_rb_base_address 		= MS_BYTE_TO_8_BYTE_RESOLUTION(DRV_RDD_IH_RUNNER_0_IH_MANAGED_RB_BASE_ADDRESS);
    runner_buffers_config.runner_b_ih_managed_rb_base_address 		= MS_BYTE_TO_8_BYTE_RESOLUTION(DRV_RDD_IH_RUNNER_1_IH_MANAGED_RB_BASE_ADDRESS);
    runner_buffers_config.runner_a_runner_managed_rb_base_address 	= MS_BYTE_TO_8_BYTE_RESOLUTION(DRV_RDD_IH_RUNNER_0_RUNNER_MANAGED_RB_BASE_ADDRESS);
    runner_buffers_config.runner_b_runner_managed_rb_base_address 	= MS_BYTE_TO_8_BYTE_RESOLUTION(DRV_RDD_IH_RUNNER_1_RUNNER_MANAGED_RB_BASE_ADDRESS);
    runner_buffers_config.runner_a_maximal_number_of_buffers 		= DRV_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS_32;
    runner_buffers_config.runner_b_maximal_number_of_buffers 		= DRV_IH_RUNNER_MAXIMAL_NUMBER_OF_BUFFERS_32;

    fi_bl_drv_ih_set_runner_buffers_configuration(&runner_buffers_config);

    /* runners load thresholds configuration */
    runners_load_thresholds.runner_a_high_congestion_threshold 		= pDpiCfg->wanType == WAN_TYPE_GPON ? \
        IH_GPON_MODE_DS_RUNNER_A_HIGH_CONGESTION_THRESHOLD :  pDpiCfg->wanType == WAN_TYPE_EPON ? \
        IH_GPON_MODE_DS_RUNNER_A_EXCLUSIVE_CONGESTION_THRESHOLD : DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    runners_load_thresholds.runner_b_high_congestion_threshold 		= DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    runners_load_thresholds.runner_a_exclusive_congestion_threshold = pDpiCfg->wanType == WAN_TYPE_GPON ? IH_GPON_MODE_DS_RUNNER_A_EXCLUSIVE_CONGESTION_THRESHOLD : DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    runners_load_thresholds.runner_b_exclusive_congestion_threshold = DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    runners_load_thresholds.runner_a_load_balancing_threshold 		= DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    runners_load_thresholds.runner_b_load_balancing_threshold 		= DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    runners_load_thresholds.runner_a_load_balancing_hysteresis 		= DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    runners_load_thresholds.runner_b_load_balancing_hysteresis 		= DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;

    fi_bl_drv_ih_set_runners_load_thresholds (& runners_load_thresholds);

    /* route addresses configuration */
    xi_route_addresses.eth0_route_address     = IH_ETH0_ROUTE_ADDRESS;
    xi_route_addresses.eth1_route_address     = IH_ETH1_ROUTE_ADDRESS;
    xi_route_addresses.eth2_route_address     = IH_ETH2_ROUTE_ADDRESS;
    xi_route_addresses.eth3_route_address     = IH_ETH3_ROUTE_ADDRESS;
    xi_route_addresses.eth4_route_address     = IH_ETH4_ROUTE_ADDRESS;
    xi_route_addresses.gpon_route_address 	  = IH_GPON_ROUTE_ADDRESS;
    xi_route_addresses.runner_a_route_address = IH_RUNNER_A_ROUTE_ADDRESS;
    xi_route_addresses.runner_b_route_address = IH_RUNNER_B_ROUTE_ADDRESS;

    fi_bl_drv_ih_set_route_addresses(&xi_route_addresses);

    /* logical ports configuration */
    logical_ports_config.eth0_config.parsing_layer_depth 		= IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_ETH0_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE;
    logical_ports_config.eth0_config.proprietary_tag_size 		= DRV_IH_PROPRIETARY_TAG_SIZE_0;

    logical_ports_config.eth1_config.parsing_layer_depth 		= IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_ETH1_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE;
    logical_ports_config.eth1_config.proprietary_tag_size 		= DRV_IH_PROPRIETARY_TAG_SIZE_0;

    logical_ports_config.eth2_config.parsing_layer_depth 		= IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_ETH2_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE;
    logical_ports_config.eth2_config.proprietary_tag_size 		= DRV_IH_PROPRIETARY_TAG_SIZE_0;

    logical_ports_config.eth3_config.parsing_layer_depth 		= IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_ETH3_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE;
    logical_ports_config.eth3_config.proprietary_tag_size 		= DRV_IH_PROPRIETARY_TAG_SIZE_0;

    logical_ports_config.eth4_config.parsing_layer_depth 		= IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_ETH4_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE;
    logical_ports_config.eth4_config.proprietary_tag_size 		= DRV_IH_PROPRIETARY_TAG_SIZE_0;

    logical_ports_config.gpon_config.parsing_layer_depth 		= IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_GPON_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE;
    logical_ports_config.gpon_config.proprietary_tag_size 		= DRV_IH_PROPRIETARY_TAG_SIZE_0;

    logical_ports_config.runner_a_config.parsing_layer_depth 	= IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_RNRA_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE;
    logical_ports_config.runner_a_config.proprietary_tag_size 	= DRV_IH_PROPRIETARY_TAG_SIZE_0;

    logical_ports_config.runner_b_config.parsing_layer_depth 	= IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_RNRB_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE;
    logical_ports_config.runner_b_config.proprietary_tag_size 	= DRV_IH_PROPRIETARY_TAG_SIZE_0;

    logical_ports_config.pcie0_config.parsing_layer_depth 		= IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_PCIE0_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE;
    logical_ports_config.pcie0_config.proprietary_tag_size 		= DRV_IH_PROPRIETARY_TAG_SIZE_0;

    logical_ports_config.pcie1_config.parsing_layer_depth 		= IH_REGS_GENERAL_CONFIGURATION_PARSE_LAYER_PER_PORT_CFG_PCIE1_PARSE_LAYER_STG_PARSE_LAYER_VALUE_RESET_VALUE;
    logical_ports_config.pcie1_config.proprietary_tag_size 		= DRV_IH_PROPRIETARY_TAG_SIZE_0;

    fi_bl_drv_ih_set_logical_ports_configuration(&logical_ports_config);

    /* source port to ingress queue mapping */
    source_port_to_ingress_queue_mapping.eth0_ingress_queue 	= IH_INGRESS_QUEUE_0_ETH0;
    source_port_to_ingress_queue_mapping.eth1_ingress_queue 	= IH_INGRESS_QUEUE_1_ETH1;
    source_port_to_ingress_queue_mapping.eth2_ingress_queue 	= IH_INGRESS_QUEUE_2_ETH2;
    source_port_to_ingress_queue_mapping.eth3_ingress_queue 	= IH_INGRESS_QUEUE_3_ETH3;
    source_port_to_ingress_queue_mapping.eth4_ingress_queue 	= IH_INGRESS_QUEUE_4_ETH4;
    source_port_to_ingress_queue_mapping.gpon_ingress_queue 	= IH_INGRESS_QUEUE_5_GPON_OR_AE;
    source_port_to_ingress_queue_mapping.runner_a_ingress_queue = IH_INGRESS_QUEUE_6_RUNNER_A;
    source_port_to_ingress_queue_mapping.runner_b_ingress_queue = IH_INGRESS_QUEUE_7_RUNNER_B;

    fi_bl_drv_ih_set_source_port_to_ingress_queue_mapping(&source_port_to_ingress_queue_mapping);

    /* ingress queues configuration */
    fi_ih_configure_ingress_queues();
#ifndef _CFE_
    /* enabling the 2 dscp to pbit tables */
    for (i = 0; i < DRV_IH_NUMBER_OF_DSCP_TO_TCI_TABLES; i++)
        fi_bl_drv_ih_enable_dscp_to_tci_table(i, 1);
#endif
    /* configure wan ports */
    wan_ports_config.eth0 = 0;
    wan_ports_config.eth1 = 0;
    wan_ports_config.eth2 = 0;
    wan_ports_config.eth3 = 0;
    wan_ports_config.runner_a = 0;
    wan_ports_config.runner_b = 0;
    wan_ports_config.pcie0 = 0;
    wan_ports_config.pcie1 = 0;

    if ( pDpiCfg->wanType == WAN_TYPE_GPON ||
    	 pDpiCfg->wanType == WAN_TYPE_EPON ||
    	 pDpiCfg->wanType == WAN_TYPE_AE)
    {
        wan_ports_config.eth4 = 0;
        wan_ports_config.gpon = 1;
    }
    else
    {
        wan_ports_config.eth4 = 1;
        wan_ports_config.gpon = 0;
    }

    fi_bl_drv_ih_configure_wan_ports(&wan_ports_config);

    f_ih_configure_lookup_tables();

    f_ih_configure_classes();

    f_ih_configure_target_matrix();

    f_ih_configure_parser();
    /*do the following configuration only in wan mode*/
#ifndef RDD_BASIC
    if ( pDpiCfg->wanType != WAN_TYPE_NONE )
    {
    	f_ih_cfg_mcast_prefix_filter_enable();
    	f_ih_cfg_iptv_lookup_classifier();
    }
#endif
}


void f_configure_bridge_port_sa_da(void)
{
    /* SA/DA Lookup Configuration */
    int i;

    for(i = BL_LILAC_RDD_WAN_BRIDGE_PORT;  i <= BL_LILAC_RDD_PCI_BRIDGE_PORT; i++)
    {
        if (i == BL_LILAC_RDD_WAN_IPTV_BRIDGE_PORT)
        {
            rdd_da_mac_lookup_config(i, 0);
            rdd_sa_mac_lookup_config(i, 0);
        }
        else
        {
            rdd_da_mac_lookup_config(i, 1);
            rdd_unknown_da_mac_cmd_config(i, rdpa_forward_action_host);

            rdd_sa_mac_lookup_config(i, 1);
            rdd_unknown_sa_mac_cmd_config(i, rdpa_forward_action_host);
        }
    }
}
#ifdef RDD_BASIC
static void f_initialize_basic_runner_parameters(void)
{
	BL_LILAC_RDD_ERROR_DTE rdd_error;
	uint32_t	i,j;

	/* Default configuration of all RDD reasons to CPU_RX_QUEUE_ID_0 */
	for (i = 0; i < BL_LILAC_RDD_CPU_RX_REASON_MAX_REASON; i++)
	{
		rdd_error = rdd_cpu_reason_to_cpu_rx_queue(i, 0, rdpa_dir_ds);

		rdd_error |= rdd_cpu_reason_to_cpu_rx_queue(i, 0, rdpa_dir_us);
	}

	/* Initialize Ethernet priority queues */
	for (i = 0; i < 8; i++)
	{
		/* (-1) TBD : when runner will support EMAC_5 */
		for (j = BL_LILAC_RDD_EMAC_ID_0; j <= BL_LILAC_RDD_EMAC_ID_4; j++)
		{
			/* Configure queue size in RDD */
			rdd_error |= rdd_eth_tx_queue_config(j, i, CS_RDD_ETH_TX_QUEUE_PACKET_THRESHOLD,0);
		}
	}

	f_configure_bridge_port_sa_da();

}
#endif //RDD_BASIC

static void f_configure_runner(void)
{
    /* Local Variables */
    RDD_INIT_PARAMS rdd_init_params;

    memset(&rdd_init_params,0,sizeof(RDD_INIT_PARAMS));

    /* zero Runner memories (data, program and context) */
	rdd_init();

	rdd_load_microcode((uint8_t*) firmware_binary_A,
			(uint8_t*) firmware_binary_B, (uint8_t*) firmware_binary_C,
			(uint8_t*) firmware_binary_D);

	rdd_load_prediction((uint8_t*) firmware_predict_A,
			(uint8_t*) firmware_predict_B, (uint8_t*) firmware_predict_C,
			(uint8_t*) firmware_predict_D);

    switch (pDpiCfg->wanType)
    {
		case WAN_TYPE_GPON:
		  rdd_init_params.wan_physical_port  = BL_LILAC_RDD_WAN_PHYSICAL_PORT_GPON;
		  break;
		case WAN_TYPE_EPON:
		  rdd_init_params.wan_physical_port  = BL_LILAC_RDD_WAN_PHYSICAL_PORT_EPON;
		  break;
		case WAN_TYPE_RGMII:
		  rdd_init_params.wan_physical_port  = BL_LILAC_RDD_WAN_PHYSICAL_PORT_ETH4;
		  break;
		case WAN_TYPE_AE:
		case WAN_TYPE_NONE:
		  rdd_init_params.wan_physical_port  = BL_LILAC_RDD_WAN_PHYSICAL_PORT_ETH5;
		  break;
		default:
			  break;
    }

    /* Add basic offset when pass the addresses to RDD */
    rdd_init_params.ddr_pool_ptr       = (uint8_t*)DEVICE_ADDRESS((uint32_t)ddr_tm_base_address);
    rdd_init_params.extra_ddr_pool_ptr = (uint8_t*)DEVICE_ADDRESS((uint32_t)ddr_multicast_base_address);
    rdd_init_params.mac_table_size     = BL_LILAC_RDD_MAC_TABLE_SIZE_1024;
    rdd_init_params.iptv_table_size    = BL_LILAC_RDD_MAC_TABLE_SIZE_256;
    rdd_init_params.ddr_headroom_size  = pDpiCfg->headroom_size;
    /* XXX: Temporary, take this from global_system */
    rdd_init_params.broadcom_switch_mode = 0;
    rdd_init_params.broadcom_switch_physical_port = 0;

    /* ip class operational mode*/
    rdd_init_params.bridge_flow_cache_mode = pDpiCfg->bridge_fc_mode;

    rdd_init_params.chip_revision = UtilGetChipRev() > 1 ? RDD_CHIP_REVISION_B0 : RDD_CHIP_REVISION_A0;

    rdd_data_structures_init(&rdd_init_params);
}

/*pmcSetModuleResetState used to set the reset state of
 * each rdp block module, the state is (1) - in reset state, un-oprational.
 * (0) - out of reset state - operational.
 */
static void pmcSetModuleResetState(uint32_t rdpModule,uint32_t state)
{
	uint32_t	bpcmResReg;
	uint32_t	error;


	error = ReadBPCMRegister(PMB_ADDR_RDP, BPCM_SRESET_CNTL_REG, (uint32*)&bpcmResReg);
	if( error )
	{
		printk("Failed to ReadBPCMRegister RDP block BPCM_SRESET_CNTL_REG\n");
	}

	if ( state )
	{
		bpcmResReg |= SHIFTL(rdpModule);
	}
	else
	{
		bpcmResReg &= ~SHIFTL(rdpModule);
	}

	error = WriteBPCMRegister(PMB_ADDR_RDP, BPCM_SRESET_CNTL_REG, bpcmResReg);

	if( error )
	{
		printk("Failed to WriteBPCMRegister RDP block BPCM_SRESET_CNTL_REG\n");
	}
}
static void pmcPutAllRdpModulesInReset(void)
{
	uint32_t	bpcmResReg = 0;
	uint32_t	error;
	error = WriteBPCMRegister(PMB_ADDR_RDP, BPCM_SRESET_CNTL_REG, bpcmResReg);

	if( error )
	{
		printk("Failed to WriteBPCMRegister RDP block BPCM_SRESET_CNTL_REG\n");
	}

}

/* checks that the address is multipilcation of 2*(1024^2)*/
int f_validate_ddr_address(uint32_t address)
{
    if (address % (2 * 1024 * 1024) == 0)
        return 1;
    return 0;
}
void f_basic_bpm_sp_enable(void)
{
    BPM_MODULE_REGS_BPM_SP_EN bpm_sp_enable;

    BPM_MODULE_REGS_BPM_SP_EN_READ( bpm_sp_enable);

	bpm_sp_enable.rnra_en = 1;
	bpm_sp_enable.rnrb_en = 1;
	bpm_sp_enable.gpon_en = 0;
	bpm_sp_enable.emac0_en = 1;
	bpm_sp_enable.emac1_en = 1;
	bpm_sp_enable.emac2_en = 1;
	bpm_sp_enable.emac3_en = 1;
	bpm_sp_enable.emac4_en = 1;


    BPM_MODULE_REGS_BPM_SP_EN_WRITE( bpm_sp_enable);

}
static void f_enable_ubus_masters(void)
{
	uint32_t	reg;

	/*first Ubus Master*/
	READ_32(UBUS_MASTER_1_RDP_UBUS_MASTER_BRDG_REG_EN,reg);
	reg |= 1; // bit 0 is the enable bit
	WRITE_32(UBUS_MASTER_1_RDP_UBUS_MASTER_BRDG_REG_EN,reg);

	/*second Ubus Master*/
	READ_32(UBUS_MASTER_2_RDP_UBUS_MASTER_BRDG_REG_EN,reg);
	reg |= 1; // bit 0 is the enable bit
	WRITE_32(UBUS_MASTER_2_RDP_UBUS_MASTER_BRDG_REG_EN,reg);

	/*third Ubus Master*/
	READ_32(UBUS_MASTER_3_RDP_UBUS_MASTER_BRDG_REG_EN,reg);
	reg |= 1; // bit 0 is the enable bit
	WRITE_32(UBUS_MASTER_3_RDP_UBUS_MASTER_BRDG_REG_EN,reg);

	READ_32(UBUS_MASTER_3_RDP_UBUS_MASTER_BRDG_REG_HP,reg);
	reg |= (UtilGetChipRev() > 1) ? 0xf0e01 : 0xf0f01; // allow forwarding of Urgent to High priority on UBUS
	WRITE_32(UBUS_MASTER_3_RDP_UBUS_MASTER_BRDG_REG_HP,reg);

	/*Enable UBUS EGPHY clocks*/
	reg = 0x1b;
	WRITE_32(UBUS_MISC_UBUS_MISC_EGPHY_RGMII_OUT,reg);

}
void f_basic_sbpm_sp_enable(void)
{
	DRV_SBPM_SP_ENABLE sbpm_sp_enable = { 0 };

	sbpm_sp_enable.rnra_sp_enable = 1;
	sbpm_sp_enable.rnrb_sp_enable = 1;
	sbpm_sp_enable.eth0_sp_enable = 1;
	sbpm_sp_enable.eth1_sp_enable = 1;
	sbpm_sp_enable.eth2_sp_enable = 1;
	sbpm_sp_enable.eth3_sp_enable = 1;
	sbpm_sp_enable.eth4_sp_enable = 1;

	fi_bl_drv_sbpm_sp_enable(&sbpm_sp_enable);
}
void ResetUnresetRdpBlock(void)
{
	/*put all RDP modules in reset state*/
	pmcPutAllRdpModulesInReset();

	/*when Oren is out of reset, RDP block reset bits are all set to reset,
	 * first we have to take out of reset the Broadbus and VPB*/
	pmcSetModuleResetState(RDP_S_RST_UBUS_DBR,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_UBUS_RABR,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_UBUS_RBBR,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_UBUS_VPBR,PBCM_UNRESET);


	f_enable_ubus_masters();
}
uint32_t oren_data_path_init( S_DPI_CFG  *pCfg)
{
	uint32_t 			error		 = DPI_RC_OK;
	uint32_t			rnrFreq		 = DEFAULT_RUNNER_FREQ;
#ifndef _CFE_
    uint32_t 			size_dummy;
#endif

    DRV_BBH_PORT_INDEX 	macIter;

	/*set the DPI configuration structure point*/
    if ( pCfg == NULL )
    {
    	pDpiCfg = &DpiBasicConfig;
    }
    else
    {
      pDpiCfg = pCfg;
    }

	/*allocate memory for Runners*/
	//TODO:memory reserve
#ifdef _CFE_
	ddr_tm_base_address		   = (void*)K0_TO_K1((uint32_t)KMALLOC(0x1400000, 0x200000));
	ddr_multicast_base_address = (void*)K0_TO_K1((uint32_t)KMALLOC(0x400000, 0x200000));
	printk("ddr_tm_base_address = 0x%x\n",ddr_tm_base_address);
#else


	error = BcmMemReserveGetByName(TM_BASE_ADDR_STR, &ddr_tm_base_address, &size_dummy);
    if (error == -1 || f_validate_ddr_address((uint32_t)ddr_tm_base_address) != 1)
        printk( "Failed to get valid DDR TM address, rc = %d  validate_ddr_address: ddr_tm_base_address=%x\n", error,
           (int) ddr_tm_base_address);

    error = BcmMemReserveGetByName(TM_MC_BASE_ADDR_STR, &ddr_multicast_base_address, &size_dummy);
    if (error == -1 || f_validate_ddr_address((uint32_t)ddr_multicast_base_address) != 1)
    	printk("Failed to get valid DDR Multicast address, rc = %d\n", error);
#endif

    ddr_tm_base_address_phys = (void*)VIRT_TO_PHYS(ddr_tm_base_address);
    ddr_multicast_base_address_phys = (void*)VIRT_TO_PHYS(ddr_multicast_base_address);
    /*init dma arrays*/
	f_initialize_bbh_dma_sdma_related_arrays();
	if (!RdpBlockUnreset)
	{
		ResetUnresetRdpBlock();
		RdpBlockUnreset = 1;
	}
   	pmcSetModuleResetState(RDP_S_RST_BB_RNR,PBCM_UNRESET);

	udelay(100);

	/*take out of reset Runners 0 and 1*/
	pmcSetModuleResetState(RDP_S_RST_RNR_SUB,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_RNR_1,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_RNR_0,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_IPSEC_RNR,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_IPSEC_RNG,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_IPSEC_MAIN,PBCM_UNRESET);
    pmcSetModuleResetState(RDP_S_RST_GEN_MAIN,PBCM_UNRESET);

	/*init runner,load microcode and structures*/
    f_configure_runner();

    /*set runner frequency*/
    if ( get_rdp_freq((unsigned int *)&rnrFreq) != 0)
    {
    	printk("WARNNING:failed to get Runner Frequency from Clk&Rst, setting to default %d MHZ\n",DEFAULT_RUNNER_FREQ);
    	rnrFreq = DEFAULT_RUNNER_FREQ;
    }
    rdd_runner_frequency_set(rnrFreq);

	/*take IH out of*/
	pmcSetModuleResetState(RDP_S_RST_IH_RNR,PBCM_UNRESET);

	/*configure IH*/
    f_ih_init();

    /*take TM out of reset*/
    pmcSetModuleResetState(RDP_S_RST_BB_RNR,PBCM_UNRESET);
    
    /*init SBPM*/
    sbpm_drv_init();

    /*init BPM*/
    bpm_drv_init();

    /*take out of reset emacs*/
    pmcSetModuleResetState(RDP_S_RST_E0_MAIN,PBCM_UNRESET);
    pmcSetModuleResetState(RDP_S_RST_E0_RST_L,PBCM_UNRESET);
    pmcSetModuleResetState(RDP_S_RST_E1_MAIN,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_E1_RST_L,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_E2_MAIN,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_E2_RST_L,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_E3_MAIN,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_E3_RST_L,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_E4_MAIN,PBCM_UNRESET);
	pmcSetModuleResetState(RDP_S_RST_E4_RST_L,PBCM_UNRESET);

	if(WriteBPCMRegister(PMB_ADDR_CHIP_CLKRST, 0xE, 0x33))
    {
        printk("failed to configure PMB CLKRST 0xE\n");
    }
	if(WriteBPCMRegister(PMB_ADDR_CHIP_CLKRST, 0xF, 0xFF))
    {
        printk("failed to configure PMB CLKRST 0xF\n");
    }
	/*init BBH of emac ports*/
	for ( macIter =  DRV_BBH_EMAC_0; macIter <= DRV_BBH_EMAC_4; macIter++)
	{
		f_initialize_bbh_of_emac_port(macIter);
	}
#ifndef RDD_BASIC
	if (pDpiCfg->wanType == WAN_TYPE_GPON)
	{
		f_initialize_bbh_of_gpon_port();
	}
	else if (pDpiCfg->wanType == WAN_TYPE_EPON)
	{
		f_initialize_bbh_of_epon_port();
	}
#endif
	/*init DMA and SDMA*/
	f_initialize_dma_sdma();

#ifdef RDD_BASIC
	/*configure basic runner parameters*/
	f_initialize_basic_runner_parameters();
#endif
	/* XXX workaround for [JIRA SWBCACPE-14083]:
	 * Reducing the header hold trigger threshold (hold asserted when 3 transactions are in queue) */ 
	MWRITE_32(0xb200088c, 0x00000033);

	if ( error == DPI_RC_OK )
	{
		initDone = 1;
	}
	return error;

}
#ifndef RDD_BASIC

#endif
uint32_t oren_data_path_go(void)
{
	DRV_BBH_PORT_INDEX  macIter,maxPort;
	ETHERNET_MAC_INFO 	pE[BP_MAX_ENET_MACS];


	if ( BpGetEthernetMacInfo(pE, BP_MAX_ENET_MACS) == BP_BOARD_ID_NOT_SET )
	{
		printk("ERROR:BoardID not Set in BoardParams\n");
		return -1;
	}

	if ( initDone != 1 )
	{
		printk("Data Path init didn't finished \n");
		return DPI_RC_ERROR;
	}

	/*init EGPHY before accessing unimacs*/
	PhyReset();

	bitcount(maxPort,pE[0].sw.port_map) ;
	maxPort += DRV_BBH_EMAC_0;

	for ( macIter =  DRV_BBH_EMAC_0; macIter < maxPort; macIter++)
	{
		mac_hwapi_init_emac(macIter);
		mac_hwapi_set_unimac_cfg(macIter);
		mac_hwapi_set_rxtx_enable(macIter,1,1);
	}
	/*enable runner*/
	rdd_runner_enable();



	/*enable the source ports in bpm/sbpm in case of basic config*/
#ifdef 	RDD_BASIC
	f_basic_bpm_sp_enable();
	f_basic_sbpm_sp_enable();
#endif

	return DPI_RC_OK;
}
uint32_t oren_data_path_shutdown(void)
{
	/*for now we just put all RDP devices in reset state*/
	pmcPutAllRdpModulesInReset();
	return DPI_RC_OK;
}
#ifndef RDD_BASIC
/* initilaization of BBH of GPON port */
void f_initialize_bbh_of_gpon_port(void)
{
    DRV_BBH_TX_CONFIGURATION bbh_tx_configuration ;
    DRV_BBH_RX_CONFIGURATION bbh_rx_configuration ;
    /* need 16 bit variable (and not 8) for the break condition */
    DRV_BBH_PER_FLOW_CONFIGURATION per_flow_configuration ;
    uint16_t number_of_packets_dropped_by_ih;

    uint32_t i;


    /*** BBH TX ***/
    bbh_tx_configuration.skb_address = MS_BYTE_TO_8_BYTE_RESOLUTION(GPON_ABSOLUTE_TX_BBH_COUNTER_ADDRESS);

    bbh_tx_configuration.dma_route_address = bbh_route_address_dma[DRV_BBH_GPON];
    bbh_tx_configuration.bpm_route_address = bbh_route_address_bpm[DRV_BBH_GPON];
    bbh_tx_configuration.sdma_route_address = bbh_route_address_sdma[DRV_BBH_GPON];
    bbh_tx_configuration.sbpm_route_address = bbh_route_address_sbpm[DRV_BBH_GPON];
    /* runner 1 is the one which handles TX */
    bbh_tx_configuration.runner_route_address = bbh_route_address_runner_1[DRV_BBH_GPON];

    bbh_tx_configuration.ddr_buffer_size = DRV_BBH_DDR_BUFFER_SIZE_2_KB;
    bbh_tx_configuration.payload_offset_resolution = DRV_BBH_PAYLOAD_OFFSET_RESOLUTION_2_B;
    bbh_tx_configuration.multicast_header_size = BBH_MULTICAST_HEADER_SIZE_FOR_LAN_PORT;
    bbh_tx_configuration.multicast_headers_base_address_in_byte = (uint32_t)ddr_multicast_base_address_phys;

    /* one thread is used for all TCONTs */
    bbh_tx_configuration.task_0 = WAN_TX_THREAD_NUMBER;
    bbh_tx_configuration.task_1 = WAN_TX_THREAD_NUMBER;
    bbh_tx_configuration.task_2 = WAN_TX_THREAD_NUMBER;
    bbh_tx_configuration.task_3 = WAN_TX_THREAD_NUMBER;
    bbh_tx_configuration.task_4 = WAN_TX_THREAD_NUMBER;
    bbh_tx_configuration.task_5 = WAN_TX_THREAD_NUMBER;
    bbh_tx_configuration.task_6 = WAN_TX_THREAD_NUMBER;
    bbh_tx_configuration.task_7 = WAN_TX_THREAD_NUMBER;
    bbh_tx_configuration.task_8_39 = WAN_TX_THREAD_NUMBER;

    bbh_tx_configuration.pd_fifo_size_0 = BBH_TX_GPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_1 = BBH_TX_GPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_2 = BBH_TX_GPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_3 = BBH_TX_GPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_4 = BBH_TX_GPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_5 = BBH_TX_GPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_6 = BBH_TX_GPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_7 = BBH_TX_GPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_8_15 = BBH_TX_GPON_PD_FIFO_SIZE_8_15;
    bbh_tx_configuration.pd_fifo_size_16_23 = BBH_TX_GPON_PD_FIFO_SIZE_16_23;
    bbh_tx_configuration.pd_fifo_size_24_31 = BBH_TX_GPON_PD_FIFO_SIZE_24_31;
    bbh_tx_configuration.pd_fifo_size_32_39 = BBH_TX_GPON_PD_FIFO_SIZE_32_39;

    if (bbh_tx_configuration.pd_fifo_size_32_39 == 0)
    {
        /* in case we support 32 tconts only, we don't need PD FIFOs for tconts 32-39.
           however the bbh driver doesn't allow configuring their size to 0. therefore we set dummy value.
           anyway, the BBH will ingore this value, since FW will not activate these tconts. */
        bbh_tx_configuration.pd_fifo_size_32_39 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    }

    bbh_tx_configuration.pd_fifo_base_0 = BBH_TX_GPON_PD_FIFO_BASE_0;
    bbh_tx_configuration.pd_fifo_base_1 = BBH_TX_GPON_PD_FIFO_BASE_1;
    bbh_tx_configuration.pd_fifo_base_2 = BBH_TX_GPON_PD_FIFO_BASE_2;
    bbh_tx_configuration.pd_fifo_base_3 = BBH_TX_GPON_PD_FIFO_BASE_3;
    bbh_tx_configuration.pd_fifo_base_4 = BBH_TX_GPON_PD_FIFO_BASE_4;
    bbh_tx_configuration.pd_fifo_base_5 = BBH_TX_GPON_PD_FIFO_BASE_5;
    bbh_tx_configuration.pd_fifo_base_6 = BBH_TX_GPON_PD_FIFO_BASE_6;
    bbh_tx_configuration.pd_fifo_base_7 = BBH_TX_GPON_PD_FIFO_BASE_7;
    bbh_tx_configuration.pd_fifo_base_8_15 = BBH_TX_GPON_PD_FIFO_BASE_8_15;
    bbh_tx_configuration.pd_fifo_base_16_23 = BBH_TX_GPON_PD_FIFO_BASE_16_23;
    bbh_tx_configuration.pd_fifo_base_24_31 = BBH_TX_GPON_PD_FIFO_BASE_24_31;
    bbh_tx_configuration.pd_fifo_base_32_39 = BBH_TX_GPON_PD_FIFO_BASE_32_39;

    if (bbh_tx_configuration.pd_fifo_base_32_39 == BBH_TX_GPON_TOTAL_NUMBER_OF_PDS)
    {
        /* see comment of pd_fifo_size_32_39 above */
        bbh_tx_configuration.pd_fifo_base_32_39 = 0;
    }

    /* In GPON case, PD prefetch byte threshold will be enabled and configured to the maximal value (4095 * 32 bytes = 131040 bytes) */
    bbh_tx_configuration.pd_prefetch_byte_threshold_enable = 1;
    bbh_tx_configuration.pd_prefetch_byte_threshold_0_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_1_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_2_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_3_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_4_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_5_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_6_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_7_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_8_39_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;

    bbh_tx_configuration.dma_read_requests_fifo_base_address = bbh_dma_and_sdma_read_requests_fifo_base_address[DRV_BBH_GPON];
    bbh_tx_configuration.dma_read_requests_maximal_number = BBH_TX_CONFIGURATIONS_DMACFG_TX_MAXREQ_MAX_VALUE_VALUE_RESET_VALUE;
    bbh_tx_configuration.sdma_read_requests_fifo_base_address = bbh_dma_and_sdma_read_requests_fifo_base_address[DRV_BBH_GPON];
    bbh_tx_configuration.sdma_read_requests_maximal_number = BBH_TX_CONFIGURATIONS_SDMACFG_TX_MAXREQ_MAX_VALUE_VALUE_RESET_VALUE;

    /* The address is in 8 bytes resolution */
    bbh_tx_configuration.tcont_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION(BBH_TX_WAN_CHANNEL_INDEX_ADDRESS);

    /* MDU Mode feature is not supported for GPON */
    bbh_tx_configuration.mdu_mode_enable = 0;
    bbh_tx_configuration.mdu_mode_read_pointer_address_in_8_byte = 0;
    bbh_tx_configuration.ddr_tm_base_address = (uint32_t)ddr_tm_base_address_phys;

    bbh_tx_configuration.emac_1588_enable = 0;

    fi_bl_drv_bbh_tx_set_configuration(DRV_BBH_GPON, &bbh_tx_configuration);

    /*** BBH RX ***/
    /* bbh_rx_set_configuration */
    bbh_rx_configuration.dma_route_address = bbh_route_address_dma[DRV_BBH_GPON];
    bbh_rx_configuration.bpm_route_address = bbh_route_address_bpm[DRV_BBH_GPON];
    bbh_rx_configuration.sdma_route_address = bbh_route_address_sdma[DRV_BBH_GPON];
    bbh_rx_configuration.sbpm_route_address = bbh_route_address_sbpm[DRV_BBH_GPON];
    bbh_rx_configuration.runner_0_route_address = bbh_route_address_runner_0[DRV_BBH_GPON];
    bbh_rx_configuration.runner_1_route_address = bbh_route_address_runner_1[DRV_BBH_GPON];
    bbh_rx_configuration.ih_route_address = bbh_route_address_ih[DRV_BBH_GPON];

    bbh_rx_configuration.ddr_buffer_size = DRV_BBH_DDR_BUFFER_SIZE_2_KB;


    bbh_rx_configuration.ddr_tm_base_address = (uint32_t)ddr_tm_base_address_phys;
    bbh_rx_configuration.pd_fifo_base_address_normal_queue_in_8_byte = f_get_bbh_rx_pd_fifo_base_address_normal_queue_in_8_byte(DRV_BBH_GPON);
    bbh_rx_configuration.pd_fifo_base_address_direct_queue_in_8_byte = f_get_bbh_rx_pd_fifo_base_address_direct_queue_in_8_byte(DRV_BBH_GPON);
    bbh_rx_configuration.pd_fifo_size_normal_queue = DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    bbh_rx_configuration.pd_fifo_size_direct_queue = DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    bbh_rx_configuration.runner_0_task_normal_queue = f_bbh_rx_runner_task_normal_queue(DRV_IH_RUNNER_CLUSTER_A, DRV_BBH_GPON);
    bbh_rx_configuration.runner_0_task_direct_queue = f_bbh_rx_runner_task_direct_queue(DRV_IH_RUNNER_CLUSTER_A, DRV_BBH_GPON);
    bbh_rx_configuration.runner_1_task_normal_queue = f_bbh_rx_runner_task_normal_queue(DRV_IH_RUNNER_CLUSTER_B, DRV_BBH_GPON);
    bbh_rx_configuration.runner_1_task_direct_queue = f_bbh_rx_runner_task_direct_queue(DRV_IH_RUNNER_CLUSTER_B, DRV_BBH_GPON);
    bbh_rx_configuration.dma_data_fifo_base_address = bbh_rx_dma_data_fifo_base_address[DRV_BBH_GPON];
    bbh_rx_configuration.dma_chunk_descriptor_fifo_base_address = bbh_rx_dma_chunk_descriptor_fifo_base_address[DRV_BBH_GPON];
    bbh_rx_configuration.sdma_data_fifo_base_address = bbh_rx_sdma_data_fifo_base_address[DRV_BBH_GPON];
    bbh_rx_configuration.sdma_chunk_descriptor_fifo_base_address = bbh_rx_sdma_chunk_descriptor_fifo_base_address[DRV_BBH_GPON];
    bbh_rx_configuration.dma_data_and_chunk_descriptor_fifos_size = UtilGetChipRev() > 1 ? BBH_RX_DMA_FIFOS_SIZE_WAN_GPON_B0:BBH_RX_DMA_FIFOS_SIZE_WAN_GPON;
    bbh_rx_configuration.dma_exclusive_threshold = UtilGetChipRev() > 1 ? BBH_RX_DMA_EXCLUSIVE_THRESHOLD_WAN_BBH_B0:BBH_RX_DMA_EXCLUSIVE_THRESHOLD_WAN_BBH;
    bbh_rx_configuration.sdma_data_and_chunk_descriptor_fifos_size = bbh_rx_sdma_data_and_chunk_descriptor_fifos_size[DRV_BBH_GPON];
    bbh_rx_configuration.sdma_exclusive_threshold = bbh_rx_sdma_exclusive_threshold[DRV_BBH_GPON];

#if BBH_RX_ETH_MIN_PKT_SIZE_SELECTION_INDEX == 0
    bbh_rx_configuration.minimum_packet_size_0 = MIN_GPON_PKT_SIZE;
#else
#error problem with BBH_RX_ETH_MIN_PKT_SIZE_SELECTION_INDEX
#endif

#if BBH_RX_OMCI_MIN_PKT_SIZE_SELECTION_INDEX == 1
    bbh_rx_configuration.minimum_packet_size_1 = MIN_OMCI_PKT_SIZE;
#else
#error problem with BBH_RX_OMCI_MIN_PKT_SIZE_SELECTION_INDEX
#endif

    /* minimum_packet_size 2-3 are not in use */
    bbh_rx_configuration.minimum_packet_size_2 = MIN_GPON_PKT_SIZE;
    bbh_rx_configuration.minimum_packet_size_3 = MIN_GPON_PKT_SIZE;

#if BBH_RX_MAX_PKT_SIZE_SELECTION_INDEX == 0
    bbh_rx_configuration.maximum_packet_size_0 = pDpiCfg->mtu_size;
#else
#error problem with BBH_RX_MAX_PKT_SIZE_SELECTION_INDEX
#endif

    /* maximum_packet_size 1-3 are not in use */
    bbh_rx_configuration.maximum_packet_size_1 = pDpiCfg->mtu_size;
    bbh_rx_configuration.maximum_packet_size_2 = pDpiCfg->mtu_size;
    bbh_rx_configuration.maximum_packet_size_3 = pDpiCfg->mtu_size;

    bbh_rx_configuration.ih_ingress_buffers_bitmask = bbh_ih_ingress_buffers_bitmask[DRV_BBH_GPON];
    bbh_rx_configuration.packet_header_offset = DRV_RDD_IH_PACKET_HEADER_OFFSET;
    bbh_rx_configuration.reassembly_offset_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION(pDpiCfg->headroom_size);

    /* By default, the triggers for FC will be disabled and the triggers for drop enabled.
       If the user configures flow control for the port, the triggers for drop will be
       disabled and triggers for FC (including Runner request) will be enabled */
    bbh_rx_configuration.flow_control_triggers_bitmask = 0;
    bbh_rx_configuration.drop_triggers_bitmask = DRV_BBH_RX_DROP_TRIGGER_BPM_IS_IN_EXCLUSIVE_STATE | DRV_BBH_RX_DROP_TRIGGER_SBPM_IS_IN_EXCLUSIVE_STATE;

    bbh_rx_configuration.flows_32_255_group_divider = BBH_RX_FLOWS_32_255_GROUP_DIVIDER;
    bbh_rx_configuration.ploam_default_ih_class = DRV_RDD_IH_CLASS_WAN_CONTROL_INDEX;
    bbh_rx_configuration.ploam_ih_class_override = 0;

    /* Enable runner task for inter-wan option */
    bbh_rx_configuration.runner_1_task_direct_queue = WAN_TO_WAN_THREAD_NUMBER;

    fi_bl_drv_bbh_rx_set_configuration(DRV_BBH_GPON, &bbh_rx_configuration);

    /* bbh_rx_set_per_flow_configuration */
    per_flow_configuration.minimum_packet_size_selection = 0;
    per_flow_configuration.maximum_packet_size_selection = 0;
    per_flow_configuration.default_ih_class = 0;
    per_flow_configuration.ih_class_override = 0;

    /* initialize all flows (including the 2 groups) */
    for (i = 0; i <= DRV_BBH_RX_FLOW_INDEX_FOR_PER_FLOW_CONFIGURATION_GROUP_1; i++)
        fi_bl_drv_bbh_rx_set_per_flow_configuration(DRV_BBH_GPON, i, &per_flow_configuration);

    /* configure group 0 (we use one group only, of flows 32-255) */
    per_flow_configuration.minimum_packet_size_selection = BBH_RX_ETH_MIN_PKT_SIZE_SELECTION_INDEX;
    per_flow_configuration.maximum_packet_size_selection = BBH_RX_MAX_PKT_SIZE_SELECTION_INDEX;
    per_flow_configuration.default_ih_class = DRV_RDD_IH_CLASS_WAN_BRIDGED_LOW_INDEX;
    per_flow_configuration.ih_class_override = 1;

    fi_bl_drv_bbh_rx_set_per_flow_configuration(DRV_BBH_GPON, DRV_BBH_RX_FLOW_INDEX_FOR_PER_FLOW_CONFIGURATION_GROUP_0, &per_flow_configuration);

    /* bbh_rx_get_per_flow_counters */
    /* the per-flow counters must be read at initialization stage, in order to clear them */
     for (i = 0; i < DRV_BBH_RX_NUMBER_OF_FLOWS; i++)
         fi_bl_drv_bbh_rx_get_per_flow_counters(DRV_BBH_GPON, i, &number_of_packets_dropped_by_ih);
}
void f_initialize_bbh_of_epon_port(void)
{
    DRV_BBH_TX_CONFIGURATION 					bbh_tx_configuration ;
    DRV_BBH_RX_CONFIGURATION 					bbh_rx_configuration ;
    /* need 16 bit variable (and not 8) for the break condition */
    DRV_BBH_PER_FLOW_CONFIGURATION 				per_flow_configuration ;
    BBH_TX_EPON_CFG								epon_Cfg;
    BBH_TX_CONFIGURATIONS_PDWKUPH0_7			bbhTxPdWkup0_7;
    BBH_TX_CONFIGURATIONS_PDWKUPH8_31			bbhTxPdWkup8_31;
    uint32_t						eponIHclass0 = DRV_RDD_IH_CLASS_WAN_BRIDGED_LOW_INDEX;
    uint16_t number_of_packets_dropped_by_ih;

    uint32_t i;


    /*** BBH TX ***/
    bbh_tx_configuration.skb_address = MS_BYTE_TO_8_BYTE_RESOLUTION(GPON_ABSOLUTE_TX_BBH_COUNTER_ADDRESS);

    bbh_tx_configuration.dma_route_address = bbh_route_address_dma[DRV_BBH_EPON];
    bbh_tx_configuration.bpm_route_address = bbh_route_address_bpm[DRV_BBH_EPON];
    bbh_tx_configuration.sdma_route_address = bbh_route_address_sdma[DRV_BBH_EPON];
    bbh_tx_configuration.sbpm_route_address = bbh_route_address_sbpm[DRV_BBH_EPON];
    /* runner 1 is the one which handles TX */
    bbh_tx_configuration.runner_route_address = bbh_route_address_runner_1[DRV_BBH_EPON];
    bbh_tx_configuration.runner_sts_route	  = BBH_TX_EPON_RUNNER_STS_ROUTE;
    bbh_tx_configuration.ddr_buffer_size = DRV_BBH_DDR_BUFFER_SIZE_2_KB;
    bbh_tx_configuration.payload_offset_resolution = DRV_BBH_PAYLOAD_OFFSET_RESOLUTION_2_B;
    bbh_tx_configuration.multicast_header_size = BBH_MULTICAST_HEADER_SIZE_FOR_LAN_PORT;
    bbh_tx_configuration.multicast_headers_base_address_in_byte = (uint32_t)ddr_multicast_base_address_phys;

    /* one thread is used for all TCONTs */
    bbh_tx_configuration.task_0 = EPON_TX_REQUEST_THREAD_NUMBER;
    bbh_tx_configuration.task_1 = EPON_TX_REQUEST_THREAD_NUMBER;
    bbh_tx_configuration.task_2 = EPON_TX_REQUEST_THREAD_NUMBER;
    bbh_tx_configuration.task_3 = EPON_TX_REQUEST_THREAD_NUMBER;
    bbh_tx_configuration.task_4 = EPON_TX_REQUEST_THREAD_NUMBER;
    bbh_tx_configuration.task_5 = EPON_TX_REQUEST_THREAD_NUMBER;
    bbh_tx_configuration.task_6 = EPON_TX_REQUEST_THREAD_NUMBER;
    bbh_tx_configuration.task_7 = EPON_TX_REQUEST_THREAD_NUMBER;
    bbh_tx_configuration.task_8_39 = EPON_TX_REQUEST_THREAD_NUMBER;

    bbh_tx_configuration.pd_fifo_size_0 = BBH_TX_EPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_1 = BBH_TX_EPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_2 = BBH_TX_EPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_3 = BBH_TX_EPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_4 = BBH_TX_EPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_5 = BBH_TX_EPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_6 = BBH_TX_EPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_7 = BBH_TX_EPON_PD_FIFO_SIZE_0_7;
    bbh_tx_configuration.pd_fifo_size_8_15 = BBH_TX_EPON_PD_FIFO_SIZE_8_15;
    bbh_tx_configuration.pd_fifo_size_16_23 = BBH_TX_EPON_PD_FIFO_SIZE_16_23;
    bbh_tx_configuration.pd_fifo_size_24_31 = BBH_TX_EPON_PD_FIFO_SIZE_24_31;
    bbh_tx_configuration.pd_fifo_size_32_39 = BBH_TX_EPON_PD_FIFO_SIZE_32_39;

    if (bbh_tx_configuration.pd_fifo_size_32_39 == 0)
    {
        /* in case we support 32 tconts only, we don't need PD FIFOs for tconts 32-39.
           however the bbh driver doesn't allow configuring their size to 0. therefore we set dummy value.
           anyway, the BBH will ingore this value, since FW will not activate these tconts. */
        bbh_tx_configuration.pd_fifo_size_32_39 = DRV_BBH_TX_MINIMAL_PD_FIFO_SIZE;
    }

    bbh_tx_configuration.pd_fifo_base_0 = BBH_TX_EPON_PD_FIFO_BASE_0;
    bbh_tx_configuration.pd_fifo_base_1 = BBH_TX_EPON_PD_FIFO_BASE_1;
    bbh_tx_configuration.pd_fifo_base_2 = BBH_TX_EPON_PD_FIFO_BASE_2;
    bbh_tx_configuration.pd_fifo_base_3 = BBH_TX_EPON_PD_FIFO_BASE_3;
    bbh_tx_configuration.pd_fifo_base_4 = BBH_TX_EPON_PD_FIFO_BASE_4;
    bbh_tx_configuration.pd_fifo_base_5 = BBH_TX_EPON_PD_FIFO_BASE_5;
    bbh_tx_configuration.pd_fifo_base_6 = BBH_TX_EPON_PD_FIFO_BASE_6;
    bbh_tx_configuration.pd_fifo_base_7 = BBH_TX_EPON_PD_FIFO_BASE_7;
    bbh_tx_configuration.pd_fifo_base_8_15 = BBH_TX_EPON_PD_FIFO_BASE_8_15;
    bbh_tx_configuration.pd_fifo_base_16_23 = BBH_TX_EPON_PD_FIFO_BASE_16_23;
    bbh_tx_configuration.pd_fifo_base_24_31 = BBH_TX_EPON_PD_FIFO_BASE_24_31;
    bbh_tx_configuration.pd_fifo_base_32_39 = BBH_TX_EPON_PD_FIFO_BASE_32_39;

    if (bbh_tx_configuration.pd_fifo_base_32_39 == BBH_TX_EPON_TOTAL_NUMBER_OF_PDS)
    {
        /* see comment of pd_fifo_size_32_39 above */
        bbh_tx_configuration.pd_fifo_base_32_39 = 0;
    }


    /* In EPON case, PD prefetch byte threshold will be enabled and configured to the maximal value (4095 * 32 bytes = 131040 bytes) */
    bbh_tx_configuration.pd_prefetch_byte_threshold_enable = 1;
    bbh_tx_configuration.pd_prefetch_byte_threshold_0_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_1_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_2_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_3_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_4_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_5_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_6_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_7_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;
    bbh_tx_configuration.pd_prefetch_byte_threshold_8_39_in_32_byte = DRV_BBH_TX_MAXIMAL_PD_PREFETCH_BYTE_THRESHOLD_IN_32_BYTE;

    bbh_tx_configuration.dma_read_requests_fifo_base_address = bbh_dma_and_sdma_read_requests_fifo_base_address[DRV_BBH_EPON];
    bbh_tx_configuration.dma_read_requests_maximal_number = BBH_TX_CONFIGURATIONS_DMACFG_TX_MAXREQ_MAX_VALUE_VALUE_RESET_VALUE;
    bbh_tx_configuration.sdma_read_requests_fifo_base_address = bbh_dma_and_sdma_read_requests_fifo_base_address[DRV_BBH_EPON];
    bbh_tx_configuration.sdma_read_requests_maximal_number = BBH_TX_CONFIGURATIONS_SDMACFG_TX_MAXREQ_MAX_VALUE_VALUE_RESET_VALUE;


    /* The address is in 8 bytes resolution */
    bbh_tx_configuration.tcont_address_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION(BBH_TX_EPON_REQUEST_WAN_CHANNEL_INDEX_ADDRESS);

    /* MDU Mode feature is not supported for GPON */
    bbh_tx_configuration.mdu_mode_enable = 0;
    bbh_tx_configuration.mdu_mode_read_pointer_address_in_8_byte = 0;
    bbh_tx_configuration.ddr_tm_base_address = (uint32_t)ddr_tm_base_address_phys;

    bbh_tx_configuration.emac_1588_enable = 0;

    fi_bl_drv_bbh_tx_set_configuration(DRV_BBH_EPON, &bbh_tx_configuration);

    /*in EPON we must configure the wakeup threshold*/
    bbhTxPdWkup0_7.wkupthresh0	=	BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
    bbhTxPdWkup0_7.wkupthresh1	=	BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
    bbhTxPdWkup0_7.wkupthresh2	=	BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
    bbhTxPdWkup0_7.wkupthresh3	=	BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
    bbhTxPdWkup0_7.wkupthresh4	=	BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
    bbhTxPdWkup0_7.wkupthresh5	=	BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
    bbhTxPdWkup0_7.wkupthresh6	=	BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
    bbhTxPdWkup0_7.wkupthresh7	=	BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
    BBH_TX_CONFIGURATIONS_PDWKUPH0_7_WRITE(DRV_BBH_EPON,bbhTxPdWkup0_7);

    bbhTxPdWkup8_31.wkupthresh8_15	= BBH_TX_EPON_PD_FIFO_SIZE_8_15 - 1;
    bbhTxPdWkup8_31.wkupthresh16_23	= BBH_TX_EPON_PD_FIFO_SIZE_16_23 - 1;
    bbhTxPdWkup8_31.wkupthresh24_31	= BBH_TX_EPON_PD_FIFO_SIZE_24_31 - 1;
    BBH_TX_CONFIGURATIONS_PDWKUPH8_31_WRITE(DRV_BBH_EPON,bbhTxPdWkup8_31);

    /*** BBH RX ***/
    /* bbh_rx_set_configuration */
    bbh_rx_configuration.dma_route_address = bbh_route_address_dma[DRV_BBH_EPON];
    bbh_rx_configuration.bpm_route_address = bbh_route_address_bpm[DRV_BBH_EPON];
    bbh_rx_configuration.sdma_route_address = bbh_route_address_sdma[DRV_BBH_EPON];
    bbh_rx_configuration.sbpm_route_address = bbh_route_address_sbpm[DRV_BBH_EPON];
    bbh_rx_configuration.runner_0_route_address = bbh_route_address_runner_0[DRV_BBH_EPON];
    bbh_rx_configuration.runner_1_route_address = bbh_route_address_runner_1[DRV_BBH_EPON];
    bbh_rx_configuration.ih_route_address = bbh_route_address_ih[DRV_BBH_EPON];

    bbh_rx_configuration.ddr_buffer_size = DRV_BBH_DDR_BUFFER_SIZE_2_KB;


    bbh_rx_configuration.ddr_tm_base_address = (uint32_t)ddr_tm_base_address_phys;
    bbh_rx_configuration.pd_fifo_base_address_normal_queue_in_8_byte = f_get_bbh_rx_pd_fifo_base_address_normal_queue_in_8_byte(DRV_BBH_EPON);
    bbh_rx_configuration.pd_fifo_base_address_direct_queue_in_8_byte = f_get_bbh_rx_pd_fifo_base_address_direct_queue_in_8_byte(DRV_BBH_EPON);
    bbh_rx_configuration.pd_fifo_size_normal_queue = DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    bbh_rx_configuration.pd_fifo_size_direct_queue = DRV_RDD_IH_RUNNER_0_MAXIMAL_NUMBER_OF_BUFFERS;
    bbh_rx_configuration.runner_0_task_normal_queue = f_bbh_rx_runner_task_normal_queue(DRV_IH_RUNNER_CLUSTER_A, DRV_BBH_EPON);
    bbh_rx_configuration.runner_0_task_direct_queue = f_bbh_rx_runner_task_direct_queue(DRV_IH_RUNNER_CLUSTER_A, DRV_BBH_EPON);
    bbh_rx_configuration.runner_1_task_normal_queue = f_bbh_rx_runner_task_normal_queue(DRV_IH_RUNNER_CLUSTER_B, DRV_BBH_EPON);
    bbh_rx_configuration.runner_1_task_direct_queue = f_bbh_rx_runner_task_direct_queue(DRV_IH_RUNNER_CLUSTER_B, DRV_BBH_EPON);
    bbh_rx_configuration.dma_data_fifo_base_address = bbh_rx_dma_data_fifo_base_address[DRV_BBH_EPON];
    bbh_rx_configuration.dma_chunk_descriptor_fifo_base_address = bbh_rx_dma_chunk_descriptor_fifo_base_address[DRV_BBH_EPON];
    bbh_rx_configuration.dma_data_and_chunk_descriptor_fifos_size = UtilGetChipRev() > 1 ? BBH_RX_DMA_FIFOS_SIZE_WAN_GPON_B0:BBH_RX_DMA_FIFOS_SIZE_WAN_GPON;
    bbh_rx_configuration.dma_exclusive_threshold = UtilGetChipRev() > 1 ? BBH_RX_DMA_EXCLUSIVE_THRESHOLD_WAN_BBH_B0:BBH_RX_DMA_EXCLUSIVE_THRESHOLD_WAN_BBH;
    bbh_rx_configuration.sdma_data_fifo_base_address = bbh_rx_sdma_data_fifo_base_address[DRV_BBH_EPON];
    bbh_rx_configuration.sdma_chunk_descriptor_fifo_base_address = bbh_rx_sdma_chunk_descriptor_fifo_base_address[DRV_BBH_EPON];
    bbh_rx_configuration.sdma_data_and_chunk_descriptor_fifos_size = bbh_rx_sdma_data_and_chunk_descriptor_fifos_size[DRV_BBH_EPON];
    bbh_rx_configuration.sdma_exclusive_threshold = bbh_rx_sdma_exclusive_threshold[DRV_BBH_EPON];


    bbh_rx_configuration.minimum_packet_size_0 = MIN_ETH_PKT_SIZE;
    bbh_rx_configuration.minimum_packet_size_1 = MIN_ETH_PKT_SIZE;
    bbh_rx_configuration.minimum_packet_size_2 = MIN_ETH_PKT_SIZE;
    bbh_rx_configuration.minimum_packet_size_3 = MIN_ETH_PKT_SIZE;
    bbh_rx_configuration.maximum_packet_size_0 = pDpiCfg->mtu_size;
    bbh_rx_configuration.maximum_packet_size_1 = pDpiCfg->mtu_size;
    bbh_rx_configuration.maximum_packet_size_2 = pDpiCfg->mtu_size;
    bbh_rx_configuration.maximum_packet_size_3 = pDpiCfg->mtu_size;

    bbh_rx_configuration.ih_ingress_buffers_bitmask = bbh_ih_ingress_buffers_bitmask[DRV_BBH_EPON];

    bbh_rx_configuration.packet_header_offset = DRV_RDD_IH_PACKET_HEADER_OFFSET;
    bbh_rx_configuration.reassembly_offset_in_8_byte = MS_BYTE_TO_8_BYTE_RESOLUTION(pDpiCfg->headroom_size);

    /* By default, the triggers for FC will be disabled and the triggers for drop enabled.
       If the user configures flow control for the port, the triggers for drop will be
       disabled and triggers for FC (including Runner request) will be enabled */
    bbh_rx_configuration.flow_control_triggers_bitmask = 0;
    bbh_rx_configuration.drop_triggers_bitmask = DRV_BBH_RX_DROP_TRIGGER_BPM_IS_IN_EXCLUSIVE_STATE | DRV_BBH_RX_DROP_TRIGGER_SBPM_IS_IN_EXCLUSIVE_STATE;

    bbh_rx_configuration.flows_32_255_group_divider = BBH_RX_FLOWS_32_255_GROUP_DIVIDER;
    bbh_rx_configuration.ploam_default_ih_class = DRV_RDD_IH_CLASS_WAN_CONTROL_INDEX;
    bbh_rx_configuration.ploam_ih_class_override = 0;

    /* Enable runner task for inter-wan option */
    bbh_rx_configuration.runner_1_task_direct_queue = WAN_TO_WAN_THREAD_NUMBER;
    fi_bl_drv_bbh_rx_set_configuration(DRV_BBH_EPON, &bbh_rx_configuration);

    /* bbh_rx_set_per_flow_configuration */
    per_flow_configuration.minimum_packet_size_selection = BBH_RX_ETH_MIN_PKT_SIZE_SELECTION_INDEX;
    per_flow_configuration.maximum_packet_size_selection = BBH_RX_MAX_PKT_SIZE_SELECTION_INDEX;
    per_flow_configuration.default_ih_class = DRV_RDD_IH_CLASS_WAN_BRIDGED_LOW_INDEX;
    per_flow_configuration.ih_class_override = 1;

    /* initialize all flows (including the 2 groups) */
    for (i = 0; i <= DRV_BBH_RX_FLOW_INDEX_FOR_PER_FLOW_CONFIGURATION_GROUP_1; i++)
        fi_bl_drv_bbh_rx_set_per_flow_configuration(DRV_BBH_EPON, i, &per_flow_configuration);

    /* configure group 0 (we use one group only, of flows 32-255) */
    per_flow_configuration.minimum_packet_size_selection = BBH_RX_ETH_MIN_PKT_SIZE_SELECTION_INDEX;
    per_flow_configuration.maximum_packet_size_selection = BBH_RX_MAX_PKT_SIZE_SELECTION_INDEX;
    per_flow_configuration.default_ih_class = DRV_RDD_IH_CLASS_WAN_BRIDGED_LOW_INDEX;
    per_flow_configuration.ih_class_override = 1;

    fi_bl_drv_bbh_rx_set_per_flow_configuration(DRV_BBH_EPON, DRV_BBH_RX_FLOW_INDEX_FOR_PER_FLOW_CONFIGURATION_GROUP_0, &per_flow_configuration);

    /* bbh_rx_get_per_flow_counters */
    /* the per-flow counters must be read at initialization stage, in order to clear them */
     for (i = 0; i < DRV_BBH_RX_NUMBER_OF_FLOWS; i++)
         fi_bl_drv_bbh_rx_get_per_flow_counters(DRV_BBH_EPON, i, &number_of_packets_dropped_by_ih);

     /*configure the extras for epon cfg*/
     BBH_TX_EPON_CFG_TASKLSB_READ(DRV_BBH_EPON,epon_Cfg.tasklsb);
     epon_Cfg.tasklsb.task0 = WAN_TX_THREAD_NUMBER;
     epon_Cfg.tasklsb.task1 = WAN_TX_THREAD_NUMBER;
     epon_Cfg.tasklsb.task2 = WAN_TX_THREAD_NUMBER;
     epon_Cfg.tasklsb.task3 = WAN_TX_THREAD_NUMBER;
     BBH_TX_EPON_CFG_TASKLSB_WRITE(DRV_BBH_EPON,epon_Cfg.tasklsb);

     BBH_TX_EPON_CFG_TASKMSB_READ(DRV_BBH_EPON,epon_Cfg.taskmsb);
     epon_Cfg.taskmsb.task4 = WAN_TX_THREAD_NUMBER;
     epon_Cfg.taskmsb.task5 = WAN_TX_THREAD_NUMBER;
     epon_Cfg.taskmsb.task6 = WAN_TX_THREAD_NUMBER;
     epon_Cfg.taskmsb.task7 = WAN_TX_THREAD_NUMBER;
     BBH_TX_EPON_CFG_TASKMSB_WRITE(DRV_BBH_EPON,epon_Cfg.taskmsb);


     BBH_TX_EPON_CFG_TASK8_39_READ(DRV_BBH_EPON, epon_Cfg.task8_39);
     epon_Cfg.task8_39.task8_39	= WAN_TX_THREAD_NUMBER;
     BBH_TX_EPON_CFG_TASK8_39_WRITE(DRV_BBH_EPON, epon_Cfg.task8_39);


     BBH_TX_EPON_CFG_PDSIZE0_3_READ(DRV_BBH_EPON, epon_Cfg.pdsize0_3);
     epon_Cfg.pdsize0_3.fifosize0 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     epon_Cfg.pdsize0_3.fifosize1 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     epon_Cfg.pdsize0_3.fifosize2 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     epon_Cfg.pdsize0_3.fifosize3 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     BBH_TX_EPON_CFG_PDSIZE0_3_WRITE(DRV_BBH_EPON, epon_Cfg.pdsize0_3);

     BBH_TX_EPON_CFG_PDSIZE4_7_READ(DRV_BBH_EPON, epon_Cfg.pdsize4_7);
     epon_Cfg.pdsize4_7.fifosize4 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     epon_Cfg.pdsize4_7.fifosize5 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     epon_Cfg.pdsize4_7.fifosize6 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     epon_Cfg.pdsize4_7.fifosize7 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     BBH_TX_EPON_CFG_PDSIZE4_7_WRITE(DRV_BBH_EPON, epon_Cfg.pdsize4_7);

     BBH_TX_EPON_CFG_PDSIZE8_31_READ(DRV_BBH_EPON, epon_Cfg.pdsize8_31);
     epon_Cfg.pdsize8_31.fifosize8_15 = BBH_TX_EPON_PD_FIFO_SIZE_8_15 - 1;
     epon_Cfg.pdsize8_31.fifosize16_23 = BBH_TX_EPON_PD_FIFO_SIZE_16_23 - 1;
     epon_Cfg.pdsize8_31.fifosize24_31 = BBH_TX_EPON_PD_FIFO_SIZE_24_31 - 1;
     BBH_TX_EPON_CFG_PDSIZE8_31_WRITE(DRV_BBH_EPON, epon_Cfg.pdsize8_31);

     BBH_TX_EPON_CFG_PDBASE0_3_READ(DRV_BBH_EPON, epon_Cfg.pdbase0_3);
     epon_Cfg.pdbase0_3.fifobase0 =  BBH_TX_EPON_PD_FIFO_BASE_0;
     epon_Cfg.pdbase0_3.fifobase1 =  BBH_TX_EPON_PD_FIFO_BASE_1;
     epon_Cfg.pdbase0_3.fifobase2 =  BBH_TX_EPON_PD_FIFO_BASE_2;
     epon_Cfg.pdbase0_3.fifobase3 =  BBH_TX_EPON_PD_FIFO_BASE_3;
     BBH_TX_EPON_CFG_PDBASE0_3_WRITE(DRV_BBH_EPON, epon_Cfg.pdbase0_3);

     BBH_TX_EPON_CFG_PDBASE4_7_READ(DRV_BBH_EPON, epon_Cfg.pdbase4_7);
     epon_Cfg.pdbase4_7.fifobase4 =  BBH_TX_EPON_PD_FIFO_BASE_4;
     epon_Cfg.pdbase4_7.fifobase5 =  BBH_TX_EPON_PD_FIFO_BASE_5;
     epon_Cfg.pdbase4_7.fifobase6 =  BBH_TX_EPON_PD_FIFO_BASE_6;
     epon_Cfg.pdbase4_7.fifobase7 =  BBH_TX_EPON_PD_FIFO_BASE_7;
     BBH_TX_EPON_CFG_PDBASE4_7_WRITE(DRV_BBH_EPON, epon_Cfg.pdbase4_7);

     BBH_TX_EPON_CFG_PDBASE8_39_WRITE(DRV_BBH_EPON, epon_Cfg.pdbase8_39);
     epon_Cfg.pdbase8_39.fifobase8_15 =  BBH_TX_EPON_PD_FIFO_BASE_8_15;
     epon_Cfg.pdbase8_39.fifobase16_23 =  BBH_TX_EPON_PD_FIFO_BASE_16_23;
     epon_Cfg.pdbase8_39.fifobase24_31 =  BBH_TX_EPON_PD_FIFO_BASE_24_31;
     epon_Cfg.pdbase8_39.fifobase32_39 =  BBH_TX_EPON_PD_FIFO_BASE_32_39;
     BBH_TX_EPON_CFG_PDBASE8_39_WRITE(DRV_BBH_EPON, epon_Cfg.pdbase8_39);

     BBH_TX_EPON_CFG_RUNNERCFG_READ(DRV_BBH_EPON, epon_Cfg.runnercfg);
     epon_Cfg.runnercfg.tcontaddr = MS_BYTE_TO_8_BYTE_RESOLUTION(BBH_TX_WAN_CHANNEL_INDEX_ADDRESS);
     BBH_TX_EPON_CFG_RUNNERCFG_WRITE(DRV_BBH_EPON, epon_Cfg.runnercfg);


     /*In EPON the IH_CLASS0 shall be 9*/
     BBH_RX_GENERAL_CONFIGURATION_IHCLASS0_WRITE(DRV_BBH_EPON,eponIHclass0);

     epon_Cfg.pdwkuph0_3.wkupthresh0 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     epon_Cfg.pdwkuph0_3.wkupthresh1 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
	 epon_Cfg.pdwkuph0_3.wkupthresh2 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
	 epon_Cfg.pdwkuph0_3.wkupthresh3 = BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
	 BBH_TX_EPON_CFG_PDWKUPH0_3_WRITE(DRV_BBH_EPON,epon_Cfg.pdwkuph0_3);

     epon_Cfg.pdwkuph4_7.wkupthresh4 =  BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     epon_Cfg.pdwkuph4_7.wkupthresh5 =  BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     epon_Cfg.pdwkuph4_7.wkupthresh6 =  BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     epon_Cfg.pdwkuph4_7.wkupthresh7 =  BBH_TX_EPON_PD_FIFO_SIZE_0_7 - 1;
     BBH_TX_EPON_CFG_PDWKUPH4_7_WRITE(DRV_BBH_EPON,epon_Cfg.pdwkuph4_7);

     epon_Cfg.pdwkuph8_31.wkupthresh8_15  =  BBH_TX_EPON_PD_FIFO_SIZE_8_15 - 1;
     epon_Cfg.pdwkuph8_31.wkupthresh16_23 =  BBH_TX_EPON_PD_FIFO_SIZE_16_23 - 1;
     epon_Cfg.pdwkuph8_31.wkupthresh24_31 =  BBH_TX_EPON_PD_FIFO_SIZE_24_31 - 1;

     BBH_TX_EPON_CFG_PDWKUPH8_31_WRITE(DRV_BBH_EPON,epon_Cfg.pdwkuph8_31);




}
#endif
