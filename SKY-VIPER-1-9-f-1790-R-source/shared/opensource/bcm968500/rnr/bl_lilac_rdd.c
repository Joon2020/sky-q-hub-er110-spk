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

#include "bl_os_wraper.h"
#include "access_macros.h"
#include "rdp_runner.h"
#include "bl_rnr_sw_fw_defs.h"
#include "bl_lilac_rdd.h"
#include "rdp_drv_bpm.h"

/********************************** Defines ***********************************/

/* Runner Device Driver version */
#define RDD_RELEASE                                ( 0x09 )
#define RDD_VERSION                                ( 0x04 )
#define RDD_PATCH                                  ( 0x00 )
#define RDD_REVISION                               ( 0x04 )

#define RDD_RUNNER_PACKET_DESCRIPTOR_SIZE          8
#define RDD_RUNNER_PACKET_BUFFER_SIZE              2048
#define RDD_PACKET_DDR_OFFSET                      18
#define RDD_RUNNER_PSRAM_BUFFER_SIZE               128

#define RDD_DOWNSTREAM_FREE_PACKET_DESCRIPTORS_POOL_SIZE   1536
#define RDD_UPSTREAM_FREE_PACKET_DESCRIPTORS_POOL_SIZE     3072

/* RX - TX */
#define RDD_NUMBER_OF_EMACS                        5
#define RDD_EMAC_NUMBER_OF_QUEUES                  8
#define RDD_EMAC_EGRESS_COUNTER_OFFSET             8
#define RDD_EMAC_RATE_SHAPER_GROUPS_STATUS_OFFSET  16
#define RDD_RATE_SHAPERS_POOL_SIZE                 128

/* CPU-RX table & queues */
#define RDD_CPU_FREE_PACKET_DESCRIPTORS_POOL_SIZE  256
#define RDD_CPU_RX_NUMBER_OF_REASONS               64
#define RDD_INTERRUPT_IO_ADDRESS                   0x160

/* CPU-TX table */
#define RDD_CPU_TX_DESCRIPTOR_SIZE                 8
#define RDD_CPU_TX_QUEUE_SIZE                      16
#define RDD_CPU_TX_QUEUE_SIZE_MASK                 0xFF7F
#define RDD_CPU_TX_COMMAND_ETH_PACKET              0
#define RDD_CPU_TX_COMMAND_ETH_FORWARD_PACKET      1
#define RDD_CPU_TX_COMMAND_MESSAGE                 7

/* Ingress Handler */
#define RDD_IH_HEADER_LENGTH                       110
#define RDD_IH_BUFFER_BBH_ADDRESS                  0x8000
#define RDD_RUNNER_A_IH_BUFFER                     14
#define RDD_RUNNER_B_IH_BUFFER                     15
#define RDD_RUNNER_A_IH_BUFFER_BBH_OFFSET          ( ( RDD_RUNNER_A_IH_BUFFER * 128 + RDD_PACKET_DDR_OFFSET ) / 8 )
#define RDD_RUNNER_B_IH_BUFFER_BBH_OFFSET          ( ( RDD_RUNNER_B_IH_BUFFER * 128 + RDD_PACKET_DDR_OFFSET ) / 8 )
#define RDD_IH_HEADER_DESCRIPTOR_BBH_ADDRESS       0x0

#define RDD_IH_WAN_BRIDGE_LOW_CLASS                9
#define RDD_IH_LAN_EMAC0_CLASS                     10
#define RDD_IH_LAN_EMAC1_CLASS                     11
#define RDD_IH_LAN_EMAC2_CLASS                     12
#define RDD_IH_LAN_EMAC3_CLASS                     13
#define RDD_IH_LAN_EMAC4_CLASS                     14

/* MAC Table */
#define RDD_HASH_TABLE_CAM_SIZE                    32
#define RDD_MAC_CONTEXT_MULTICAST                  0x1

/* Router Tables */
#define RDD_NUMBER_OF_SUBNETS                      6
#define RDD_SUBNET_BRIDGE                          4

#define RDD_DMA_LOOKUP_RESULT_SLOT0                0
#define RDD_DMA_LOOKUP_RESULT_SLOT1                1
#define RDD_DMA_LOOKUP_RESULT_SLOT2                2
#define RDD_DMA_LOOKUP_RESULT_SLOT3                3
#define RDD_DMA_LOOKUP_RESULT_SLOT4                4
#define RDD_DMA_LOOKUP_RESULT_SLOT5                5
#define RDD_DMA_LOOKUP_RESULT_SLOT0_IO_ADDRESS     0x40
#define RDD_DMA_LOOKUP_RESULT_SLOT1_IO_ADDRESS     0x44
#define RDD_DMA_LOOKUP_RESULT_SLOT2_IO_ADDRESS     0x48
#define RDD_DMA_LOOKUP_RESULT_SLOT3_IO_ADDRESS     0x4C
#define RDD_DMA_LOOKUP_RESULT_SLOT4_IO_ADDRESS     0x50
#define RDD_DMA_LOOKUP_RESULT_SLOT5_IO_ADDRESS     0x54
#define RDD_DMA_LOOKUP_4_STEPS                     4

#define RDD_CAM_RESULT_SLOT0                       0
#define RDD_CAM_RESULT_SLOT1                       1
#define RDD_CAM_RESULT_SLOT2                       2
#define RDD_CAM_RESULT_SLOT3                       3
#define RDD_CAM_RESULT_SLOT4                       4
#define RDD_CAM_RESULT_SLOT5                       5
#define RDD_CAM_RESULT_SLOT6                       6
#define RDD_CAM_RESULT_SLOT7                       7
#define RDD_CAM_RESULT_SLOT0_IO_ADDRESS            0x60
#define RDD_CAM_RESULT_SLOT1_IO_ADDRESS            0x64
#define RDD_CAM_RESULT_SLOT2_IO_ADDRESS            0x68
#define RDD_CAM_RESULT_SLOT3_IO_ADDRESS            0x6C
#define RDD_CAM_RESULT_SLOT4_IO_ADDRESS            0x70
#define RDD_CAM_RESULT_SLOT5_IO_ADDRESS            0x74
#define RDD_CAM_RESULT_SLOT6_IO_ADDRESS            0x78
#define RDD_CAM_RESULT_SLOT7_IO_ADDRESS            0x7C

/* -Etc- */
#define RDD_TRUE                                   1
#define RDD_FALSE                                  0
#define RDD_ON                                     1
#define RDD_OFF                                    0



/******************************* Data structures ******************************/

/* Packet Descriptor structure */
typedef struct
{
    uint32_t  reserved0                        :32 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved1                        :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  next_packet_descriptor_address   :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_PACKET_DESCRIPTOR_DTS;


/* Free Buffer Pool is an array of packet descriptors which maintain as a stack */
typedef struct
{
    RDD_PACKET_DESCRIPTOR_DTS  packet_descriptor[ RDD_UPSTREAM_FREE_PACKET_DESCRIPTORS_POOL_SIZE ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_FREE_PACKET_DESCRIPTORS_POOL_DTS;


#define RDD_PACKET_DESCRIPTOR_NEXT_PTR_READ( r, p )    FIELD_MREAD_32( ( (uint8_t *)p + 4), 0, 16, r )
#define RDD_PACKET_DESCRIPTOR_NEXT_PTR_WRITE( v, p )   FIELD_MWRITE_32( ( (uint8_t *)p + 4), 0, 16, v )



/* Free Buffers Pool Descriptor structure */
typedef struct
{
    uint32_t  head_pointer                 :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  tail_pointer                 :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  egress_packet_descriptors    :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  ingress_packet_descriptors   :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_DTS;


#define RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_HEAD_PTR_READ( r, p )    MREAD_16( ( (uint8_t *)p + 0), r )
#define RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_TAIL_PTR_READ( r, p )    MREAD_16( ( (uint8_t *)p + 2), r )
#define RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_EGRESS_READ( r, p )      MREAD_16( ( (uint8_t *)p + 4), r )
#define RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_INGRESS_READ( r, p )     MREAD_16( ( (uint8_t *)p + 6), r )
#define RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_HEAD_PTR_WRITE( v, p )   MWRITE_16( ( (uint8_t *)p + 0), v )
#define RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_TAIL_PTR_WRITE( v, p )   MWRITE_16( ( (uint8_t *)p + 2), v )
#define RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_EGRESS_WRITE( v, p )     MWRITE_16( ( (uint8_t *)p + 4), v )
#define RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_INGRESS_WRITE( v, p )    MWRITE_16( ( (uint8_t *)p + 6), v )



typedef struct
{
    uint32_t  head_ptr                :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  tail_ptr                :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  ingress_packet_counter  :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  egress_packet_counter   :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  packet_threshold        :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved0               :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  status_offset           :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rs_status_offset        :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rs_group_status_offset  :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved1               :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_ETH_TX_QUEUE_DYNAMIC_DESCRIPTOR_DTS;


typedef struct
{
    RDD_ETH_TX_QUEUE_DYNAMIC_DESCRIPTOR_DTS  entry[ RDD_EMAC_NUMBER_OF_QUEUES ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_ETH_TX_QUEUES_DYNAMIC_TABLE_DTS;


#define RDD_ETH_TX_DYNAMIC_QUEUE_HEAD_PTR_READ( r, p )                  MREAD_16( ( (uint8_t *)p + 0), r )
#define RDD_ETH_TX_DYNAMIC_QUEUE_TAIL_PTR_READ( r, p )                  MREAD_16( ( (uint8_t *)p + 2), r )
#define RDD_ETH_TX_DYNAMIC_QUEUE_INGRESS_PC_READ( r, p )                MREAD_16( ( (uint8_t *)p + 4), r )
#define RDD_ETH_TX_DYNAMIC_QUEUE_EGRESS_PC_READ( r, p )                 MREAD_16( ( (uint8_t *)p + 6), r )
#define RDD_ETH_TX_DYNAMIC_QUEUE_STATUS_OFFSET_READ( r, p )             MREAD_8( ( (uint8_t *)p + 11), r )
#define RDD_ETH_TX_DYNAMIC_QUEUE_HEAD_PTR_WRITE( v, p )                 MWRITE_16( ( (uint8_t *)p + 0), v )
#define RDD_ETH_TX_DYNAMIC_QUEUE_TAIL_PTR_WRITE( v, p )                 MWRITE_16( ( (uint8_t *)p + 2), v )
#define RDD_ETH_TX_DYNAMIC_QUEUE_PACKET_THRSHLD_WRITE( v, p )           MWRITE_16( ( (uint8_t *)p + 8), v )
#define RDD_ETH_TX_DYNAMIC_QUEUE_STATUS_OFFSET_WRITE( v, p )            MWRITE_8( ( (uint8_t *)p + 11), v )
#define RDD_ETH_TX_DYNAMIC_QUEUE_RS_STATUS_OFFSET_WRITE( v, p )         MWRITE_8( ( (uint8_t *)p + 12), v )
#define RDD_ETH_TX_DYNAMIC_QUEUE_RS_GROUP_STATUS_OFFSET_WRITE( v, p )   MWRITE_8( ( (uint8_t *)p + 13), v )



typedef struct
{
    uint32_t  packet_threshold        :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved0               :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  status_offset           :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved1               :32 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_ETH_TX_QUEUE_STATIC_DESCRIPTOR_DTS;


typedef struct
{
    RDD_ETH_TX_QUEUE_STATIC_DESCRIPTOR_DTS  entry[ RDD_EMAC_NUMBER_OF_QUEUES ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_ETH_TX_QUEUES_STATIC_TABLE_DTS;


#define RDD_ETH_TX_STATIC_QUEUE_PACKET_THRSHLD_READ( r, p )    MREAD_16( ( (uint8_t *)p + 0), r )
#define RDD_ETH_TX_STATIC_QUEUE_STATUS_OFFSET_READ( r, p )     MREAD_8( ( (uint8_t *)p + 3), r )
#define RDD_ETH_TX_STATIC_QUEUE_PACKET_THRSHLD_WRITE( v, p )   MWRITE_16( ( (uint8_t *)p + 0), v )
#define RDD_ETH_TX_STATIC_QUEUE_STATUS_OFFSET_WRITE( v, p )    MWRITE_8( ( (uint8_t *)p + 3), v )



typedef struct
{
    uint32_t  tx_queue_6_status             :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  tx_queue_4_status             :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  tx_queue_2_status             :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  tx_queue_0_status             :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  tx_queue_7_status             :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  tx_queue_5_status             :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  tx_queue_3_status             :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  tx_queue_1_status             :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  egress_counter                :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved0                     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_max_burst         :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  ingress_counter               :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_limiter_id               :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  tx_task_number                :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shapers_groups_vector    :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group6_status     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group4_status     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group2_status     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group0_status     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group7_status     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group5_status     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group3_status     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group1_status     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group0_vector     :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group1_vector     :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group2_vector     :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group3_vector     :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group4_vector     :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group5_vector     :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group6_vector     :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_shaper_group7_vector     :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved1                     :32 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved2                     :32 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved3                     :32 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved4                     :32 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved5                     :32 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved6                     :32 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_ETH_TX_MAC_DYNAMIC_DESCRIPTOR_DTS;


typedef struct
{
    RDD_ETH_TX_MAC_DYNAMIC_DESCRIPTOR_DTS  entry[ RDD_NUMBER_OF_EMACS ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_ETH_TX_MAC_DYNAMIC_TABLE_DTS;


#define RDD_ETH_TX_MAC_DYNAMIC_RATE_SHAPER_BURST_WRITE( v, p )       MWRITE_16( (uint8_t *)p + 10, v )
#define RDD_ETH_TX_MAC_DYNAMIC_RATE_LIMITER_ID_WRITE( v, p )         MWRITE_8( (uint8_t *)p + 13, v )
#define RDD_ETH_TX_MAC_DYNAMIC_TX_TASK_NUMBER_WRITE( v, p )          MWRITE_8( (uint8_t *)p + 14, v )



typedef struct
{
    uint32_t  tx_task_number                :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved0                     :24 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved1                     :32 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_ETH_TX_MAC_STATIC_DESCRIPTOR_DTS;


typedef struct
{
    RDD_ETH_TX_MAC_STATIC_DESCRIPTOR_DTS  entry[ RDD_NUMBER_OF_EMACS ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_ETH_TX_MAC_STATIC_TABLE_DTS;


#define RDD_ETH_TX_MAC_STATIC_TX_TASK_NUMBER_WRITE( v, p )   MWRITE_8( ( (uint8_t *)p + 0), v )



typedef struct
{
    uint32_t  eth_mac_pointer   :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  tx_queue_pointer  :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_ETH_TX_QUEUE_POINTERS_ENTRY_DTS;


typedef struct
{
    RDD_ETH_TX_QUEUE_POINTERS_ENTRY_DTS  entry[ RDD_EMAC_NUMBER_OF_QUEUES ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_ETH_TX_QUEUES_POINTERS_TABLE_DTS;


#define RDD_ETH_TX_QUEUE_POINTERS_QUEUE_PTR_READ( r, p )    MREAD_16( ( (uint8_t *)p + 2), r )
#define RDD_ETH_TX_QUEUE_POINTERS_MAC_PTR_WRITE( v, p )     MWRITE_16( ( (uint8_t *)p + 0), v )
#define RDD_ETH_TX_QUEUE_POINTERS_QUEUE_PTR_WRITE( v, p )   MWRITE_16( ( (uint8_t *)p + 2), v )


/* CPU-RX Table Entry holds the maintenance data for each CPU-RX queue */
typedef struct
{
    uint16_t  head_pointer                __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint16_t  tail_pointer                __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint16_t  egress_packet_counter       __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint16_t  ingress_packet_counter      __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint16_t  interrupt                   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint16_t  max_size                    __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint16_t  drop_counter                __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint8_t   zero_copy_flag              __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint8_t   reserved                    __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_CPU_RX_QUEUE_DESCRIPTOR_DTS;


/* CPU-RX Table for 8 CPU-RX queues */
typedef struct
{
    RDD_CPU_RX_QUEUE_DESCRIPTOR_DTS  entry[ CPU_RX_NUMBER_OF_QUEUES ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_CPU_RX_TABLE_DTS;


#define RDD_CPU_RX_QUEUE_HEAD_POINTER_READ( r, p )      MREAD_16( ( (uint8_t *)p + 0),  r )
#define RDD_CPU_RX_QUEUE_TAIL_POINTER_READ( r, p )      MREAD_16( ( (uint8_t *)p + 2 ), r )
#define RDD_CPU_RX_QUEUE_EGRESS_COUNTER_READ( r, p )    MREAD_16( ( (uint8_t *)p + 4),  r )
#define RDD_CPU_RX_QUEUE_INGRESS_COUNTER_READ( r, p )   MREAD_16( ( (uint8_t *)p + 6),  r )
#define RDD_CPU_RX_QUEUE_DROP_COUNTER_READ( r, p )      MREAD_16( ( (uint8_t *)p + 12), r )
#define RDD_CPU_RX_QUEUE_HEAD_POINTER_WRITE( v, p )     MWRITE_16( ( (uint8_t *)p + 0 ), v )
#define RDD_CPU_RX_QUEUE_TAIL_POINTER_WRITE( v, p )     MWRITE_16( ( (uint8_t *)p + 2 ), v )
#define RDD_CPU_RX_QUEUE_EGRESS_COUNTER_WRITE( v, p )   MWRITE_16( ( (uint8_t *)p + 4 ), v )
#define RDD_CPU_RX_QUEUE_INTERRUPT_WRITE( v, p )        MWRITE_16( ( (uint8_t *)p + 8 ), v )
#define RDD_CPU_RX_QUEUE_MAX_SIZE_WRITE( v, p )         MWRITE_16( ( (uint8_t *)p + 10), v )



/* CPU-RX Descriptor structure */
typedef struct
{
    uint32_t  valid                :1  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reason               :6  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved1            :3  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  flow_id              :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  packet_length        :14 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  src_port             :4  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  next_pointer         :13 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved0            :1  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  buffer_number        :14 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_CPU_RX_DESCRIPTOR_DTS;


#define RDD_CPU_RX_DESCRIPTOR_REASON_READ( r, p )           FIELD_MREAD_32( ( (uint8_t *)p + 0), 25,  6, r )
#define RDD_CPU_RX_DESCRIPTOR_FLOW_ID_READ( r, p )          FIELD_MREAD_32( ( (uint8_t *)p + 0), 14,  8, r )
#define RDD_CPU_RX_DESCRIPTOR_PACKET_LENGTH_READ( r, p )    FIELD_MREAD_32( ( (uint8_t *)p + 0),  0, 14, r )
#define RDD_CPU_RX_DESCRIPTOR_SRC_PORT_READ( r, p )         FIELD_MREAD_32( ( (uint8_t *)p + 4), 28,  4, r )
#define RDD_CPU_RX_DESCRIPTOR_NEXT_PTR_READ( r, p )         FIELD_MREAD_32( ( (uint8_t *)p + 4), 15, 13, r )
#define RDD_CPU_RX_DESCRIPTOR_BUFFER_NUMBER_READ( r, p )    FIELD_MREAD_32( ( (uint8_t *)p + 4),  0, 14, r )



/* CPU reason to CPU-RX queue */
typedef struct
{
      uint8_t cpu_rx_queue;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_CPU_REASON_TO_CPU_RX_QUEUE_ENTRY_DTS;


typedef struct
{
    RDD_CPU_REASON_TO_CPU_RX_QUEUE_ENTRY_DTS  entry[ RDD_CPU_RX_NUMBER_OF_REASONS ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_CPU_REASON_TO_CPU_RX_QUEUE_TABLE_DTS;


#define RDD_CPU_REASON_TO_QUEUE_ENTRY_WRITE( v, p )   MWRITE_8( p, v )


/* CPU reason to CPU traffic meter */
typedef struct
{
      uint8_t  cpu_meter;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_CPU_REASON_TO_METER_ENTRY_DTS;


typedef struct
{
    RDD_CPU_REASON_TO_METER_ENTRY_DTS  entry[ RDD_CPU_RX_NUMBER_OF_REASONS ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_CPU_REASON_TO_METER_TABLE_DTS;


#define RDD_CPU_REASON_TO_METER_ENTRY_WRITE( v, p )   MWRITE_8( p, v )


/* CPU-TX Descriptor structure */
typedef struct
{
    uint32_t  valid                :1   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  command              :3   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved0            :3   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  emac                 :3   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  queue                :3   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  src_bridge_port      :5   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  packet_length        :14  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved1            :9   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  payload_offset       :7   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  indication_1588      :1   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  target_memory        :1   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  buffer_number        :14  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_CPU_TX_ETH_DESCRIPTOR_DTS;


typedef struct
{
    uint32_t  valid                :1   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  command              :3   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved0            :9   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  downstream_gem_flow  :5   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  packet_length        :14  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved1            :5   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  ih_class             :4   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  payload_offset       :7   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  indication_1588      :1   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  target_memory        :1   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  buffer_number        :14  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_CPU_TX_ETH_FORWARD_DESCRIPTOR_DTS;


typedef struct
{
    uint32_t  valid              :1   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  command            :3   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved1          :28  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  message_type       :4   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved2          :28  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_CPU_TX_MESSAGE_DESCRIPTOR_DTS;


typedef union
{
    RDD_CPU_TX_ETH_DESCRIPTOR_DTS           eth;
    RDD_CPU_TX_ETH_FORWARD_DESCRIPTOR_DTS   eth_forward;
    RDD_CPU_TX_MESSAGE_DESCRIPTOR_DTS       message;
}
RDD_CPU_TX_DESCRIPTOR_DTS;



/* CPU-TX Queue is an array of CPU-TX descriptors */
typedef struct
{
    RDD_CPU_TX_DESCRIPTOR_DTS  cpu_tx_descriptor[ RDD_CPU_TX_QUEUE_SIZE ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_CPU_TX_QUEUE_DTS;


#define RDD_CPU_TX_DESCRIPTOR_VALID_READ( r, p )              FIELD_MREAD_32( ( (uint8_t *)p + 0), 31,  1, r )
#define RDD_CPU_TX_DESCRIPTOR_VALID_WRITE( v, p )             FIELD_MWRITE_8( p, 7, 1, v )
#define RDD_CPU_TX_DESCRIPTOR_COMMAND_WRITE( v, p )           FIELD_MWRITE_32( ( (uint8_t *)p + 0), 28,  3, v )
#define RDD_CPU_TX_DESCRIPTOR_EMAC_WRITE( v, p )              FIELD_MWRITE_32( ( (uint8_t *)p + 0), 22,  3, v )
#define RDD_CPU_TX_DESCRIPTOR_US_GEM_FLOW_WRITE( v, p )       FIELD_MWRITE_32( ( (uint8_t *)p + 0), 20,  8, v )
#define RDD_CPU_TX_DESCRIPTOR_QUEUE_WRITE( v, p )             FIELD_MWRITE_32( ( (uint8_t *)p + 0), 19,  3, v )
#define RDD_CPU_TX_DESCRIPTOR_SUBNET_ID_WRITE( v, p )         FIELD_MWRITE_32( ( (uint8_t *)p + 0), 19,  3, v )
#define RDD_CPU_TX_DESCRIPTOR_DS_GEM_FLOW_WRITE( v, p)        FIELD_MWRITE_32( ( (uint8_t *)p + 0), 14,  8, v )
#define RDD_CPU_TX_DESCRIPTOR_SRC_BRIDGE_PORT_WRITE( v, p )   FIELD_MWRITE_32( ( (uint8_t *)p + 0), 14,  5, v )
#define RDD_CPU_TX_DESCRIPTOR_PACKET_LENGTH_WRITE( v, p )     FIELD_MWRITE_32( ( (uint8_t *)p + 0), 0,  14, v )
#define RDD_CPU_TX_DESCRIPTOR_TX_QUEUE_PTR_WRITE( v, p )      FIELD_MWRITE_32( ( (uint8_t *)p + 4), 23,  9, v )
#define RDD_CPU_TX_DESCRIPTOR_IH_CLASS_WRITE( v, p )          FIELD_MWRITE_32( ( (uint8_t *)p + 4), 23,  4, v )
#define RDD_CPU_TX_DESCRIPTOR_PAYLOAD_OFFSET_WRITE( v, p )    FIELD_MWRITE_32( ( (uint8_t *)p + 4), 16,  7, v )
#define RDD_CPU_TX_DESCRIPTOR_BUFFER_NUMBER_WRITE( v, p )     FIELD_MWRITE_32( ( (uint8_t *)p + 4),  0, 14, v )

#define RDD_CPU_TX_DESCRIPTOR_FLOW_WRITE( v, p )              FIELD_MWRITE_32( ( (uint8_t *)p + 0), 19,  9, v )
#define RDD_CPU_TX_DESCRIPTOR_GROUP_WRITE( v, p )             FIELD_MWRITE_32( ( (uint8_t *)p + 0), 19,  7, v )
#define RDD_CPU_TX_DESCRIPTOR_COUNTER_WRITE( v, p )           FIELD_MWRITE_32( ( (uint8_t *)p + 0),  0,  5, v )
#define RDD_CPU_TX_DESCRIPTOR_COUNTER_4_BYTES_WRITE( v, p )   FIELD_MWRITE_32( ( (uint8_t *)p + 0),  5,  1, v )
#define RDD_CPU_TX_DESCRIPTOR_MSG_TYPE_WRITE( v, p )          FIELD_MWRITE_32( ( (uint8_t *)p + 4), 0,  4, v )


/* Filter Configuration */
typedef struct
{
    uint16_t  entry;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_FILTERS_CONFIGURATION_ENTRY_DTS;


typedef struct
{
    RDD_FILTERS_CONFIGURATION_ENTRY_DTS  entry[ RDD_NUMBER_OF_SUBNETS ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_FILTERS_CONFIGURATION_TABLE_DTS;


#define RDD_FILTERS_CFG_ENTRY_READ( r, p )    MREAD_16( p, r )
#define RDD_FILTERS_CFG_ENTRY_WRITE( v, p )   MWRITE_16( p, v )


/* MAC Table Entry structure */
typedef struct
{
    uint32_t  user_defined    :12  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  mac_addr0       :8   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  mac_addr1       :8   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  mac_addr2       :8   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  mac_addr3       :8   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  mac_addr4       :8   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  mac_addr5       :8   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved        :1   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  aging           :1   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  skip            :1   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  valid           :1   __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_MAC_ENTRY_DTS;


/* MAC Table is an array of MAC entries */
typedef struct
{
    RDD_MAC_ENTRY_DTS  entry[ 256 ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_MAC_TABLE_DTS;


#define RDD_MAC_ENTRY_SRC_PORT_READ( r, p )    FIELD_MREAD_32( ( (uint8_t *)p + 0), 24,  5, r )
#define RDD_MAC_ENTRY_VLAN_ID_READ( r, p )     FIELD_MREAD_32( ( (uint8_t *)p + 0), 20, 12, r )
#define RDD_MAC_ENTRY_MAC_0_READ( r, p )       FIELD_MREAD_32( ( (uint8_t *)p + 0), 12,  8, r )
#define RDD_MAC_ENTRY_MAC_1_READ( r, p )       FIELD_MREAD_32( ( (uint8_t *)p + 0),  4,  8, r )
#define RDD_MAC_ENTRY_MAC_2_READ( r, p )       { uint32_t temp; FIELD_MREAD_32( ( (uint8_t *)p + 0), 0, 4, temp ); r = temp << 4; FIELD_MREAD_32( ( (uint8_t *)p + 4), 28, 4, temp ); r = r | temp; }
#define RDD_MAC_ENTRY_MAC_3_READ( r, p )       FIELD_MREAD_32( ( (uint8_t *)p + 4), 20,  8, r )
#define RDD_MAC_ENTRY_MAC_4_READ( r, p )       FIELD_MREAD_32( ( (uint8_t *)p + 4), 12,  8, r )
#define RDD_MAC_ENTRY_MAC_5_READ( r, p )       FIELD_MREAD_32( ( (uint8_t *)p + 4),  4,  8, r )
#define RDD_MAC_ENTRY_AGING_READ( r, p )       FIELD_MREAD_32( ( (uint8_t *)p + 4),  2,  1, r )
#define RDD_MAC_ENTRY_SKIP_READ( r, p )        FIELD_MREAD_32( ( (uint8_t *)p + 4),  1,  1, r )
#define RDD_MAC_ENTRY_VALID_READ( r, p )       FIELD_MREAD_32( ( (uint8_t *)p + 4),  0,  1, r )
#define RDD_MAC_ENTRY_SRC_PORT_WRITE( v, p )   FIELD_MWRITE_32( ( (uint8_t *)p + 0 ), 24, 5,  v )
#define RDD_MAC_ENTRY_VLAN_ID_WRITE( v, p )    FIELD_MWRITE_32( ( (uint8_t *)p + 0 ), 20, 12, v )
#define RDD_MAC_ENTRY_MAC_0_WRITE( v, p )      FIELD_MWRITE_32( ( (uint8_t *)p + 0 ), 12, 8,  v )
#define RDD_MAC_ENTRY_MAC_1_WRITE( v, p )      FIELD_MWRITE_32( ( (uint8_t *)p + 0 ), 4,  8,  v )
#define RDD_MAC_ENTRY_MAC_2_WRITE( v, p )      { FIELD_MWRITE_32( ( (uint8_t *)p + 0 ), 0, 4, ( v >> 4 ) ); FIELD_MWRITE_32( ( (uint8_t *)p + 4 ), 28, 4, v ); }
#define RDD_MAC_ENTRY_MAC_3_WRITE( v, p )      FIELD_MWRITE_32( ( (uint8_t *)p + 4 ), 20, 8, v )
#define RDD_MAC_ENTRY_MAC_4_WRITE( v, p )      FIELD_MWRITE_32( ( (uint8_t *)p + 4 ), 12, 8, v )
#define RDD_MAC_ENTRY_MAC_5_WRITE( v, p )      FIELD_MWRITE_32( ( (uint8_t *)p + 4 ), 4,  8, v )
#define RDD_MAC_ENTRY_AGING_WRITE( v, p )      FIELD_MWRITE_32( ( (uint8_t *)p + 4 ), 2,  1, v )
#define RDD_MAC_ENTRY_SKIP_WRITE( v, p )       FIELD_MWRITE_32( ( (uint8_t *)p + 4 ), 1,  1, v )
#define RDD_MAC_ENTRY_VALID_WRITE( v, p )      FIELD_MWRITE_32( ( (uint8_t *)p + 4 ), 0,  1, v )


/* MAC forwarding Entry structure */
typedef struct
{
    uint32_t  multicast          :1  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  multicast_vector   :2  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  egress_port        :5  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  ingress_cos        :2  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  da_action          :3  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  sa_action          :3  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_FORWARD_ENTRY_DTS;


/* Forwarding Table is an array of forwarding entries */
typedef struct
{
    RDD_FORWARD_ENTRY_DTS  entry[ 256 ];
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_FORWARD_TABLE_DTS;


#define RDD_FORWARD_ENTRY_READ( r, p )                     MREAD_16( p, r )
#define RDD_FORWARD_ENTRY_MULTICAST_READ( r, p )           FIELD_MREAD_16( p, 15, 1, r )
#define RDD_FORWARD_ENTRY_MULTICAST_VECTOR_READ( r, p )    FIELD_MREAD_16( p,  8, 7, r )
#define RDD_FORWARD_ENTRY_DA_ACTION_READ( r, p )           FIELD_MREAD_16( p,  3, 3, r )
#define RDD_FORWARD_ENTRY_SA_ACTION_READ( r, p )           FIELD_MREAD_16( p,  0, 3, r )
#define RDD_FORWARD_ENTRY_WRITE( v, p )                    MWRITE_16( p, v )
#define RDD_FORWARD_ENTRY_MULTICAST_WRITE( v, p )          FIELD_MWRITE_16( p, 15, 1, v )
#define RDD_FORWARD_ENTRY_MULTICAST_VECTOR_WRITE( v, p )   FIELD_MWRITE_16( p,  8, 7, v )
#define RDD_FORWARD_ENTRY_EGRESS_PORT_WRITE( v, p )        FIELD_MWRITE_16( p,  8, 5, v )
#define RDD_FORWARD_ENTRY_DA_ACTION_WRITE( v, p )          FIELD_MWRITE_16( p,  3, 3, v )
#define RDD_FORWARD_ENTRY_SA_ACTION_WRITE( v, p )          FIELD_MWRITE_16( p,  0, 3, v )


/* BCR - Bridge Configuration Register */
typedef struct
{
    uint32_t  gpon_miss_eth_flow          :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  gpon_untagged_eth_flow      :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  wan_to_wan_gem_flow         :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved0                   :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  subnet_classification_mode  :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rgw_upstream_gem_mapping    :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved1                   :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  gpon_unknown_sa_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth0_unknown_sa_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth1_unknown_sa_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth2_unknown_sa_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth3_unknown_sa_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth4_unknown_sa_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved2                   :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  pci_unknown_sa_command      :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved3                   :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  gpon_unknown_da_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth0_unknown_da_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth1_unknown_da_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth2_unknown_da_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth3_unknown_da_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth4_unknown_da_command     :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved4                   :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  pci_unknown_da_command      :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved5                   :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  egress_ether_type_1         :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  egress_ether_type_2         :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  egress_ether_type_3         :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  rate_controller_timer       :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved6                   :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth0_acl_layer3_mode        :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth1_acl_layer3_mode        :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth2_acl_layer3_mode        :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth3_acl_layer3_mode        :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  eth4_acl_layer3_mode        :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved7                   :16 __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  pci_acl_layer3_mode         :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  reserved8                   :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  ip_sync_1588_mode           :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  iptv_classification_method  :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  ds_connection_miss_action   :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  dscp_to_gem_control         :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
    uint32_t  ipv6_enable                 :8  __PACKING_ATTRIBUTE_FIELD_LEVEL__;
}
__PACKING_ATTRIBUTE_STRUCT_END__ RDD_BRIDGE_CONFIGURATION_REGISTER_DTS;


#define RDD_BCR_MISS_ETH_FLOW_WRITE( v, p )               MWRITE_8( (uint8_t *)p + 0, v )
#define RDD_BCR_UNTAGGED_ETH_FLOW_WRITE( v, p )           MWRITE_8( (uint8_t *)p + 1, v )
#define RDD_BCR_WAN_TO_WAN_GEM_FLOW_WRITE( v, p )         MWRITE_8( (uint8_t *)p + 2, v )
#define RDD_BCR_SUBNET_CLASSIFY_MODE_WRITE( v, p )        MWRITE_8( (uint8_t *)p + 4, v )
#define RDD_BCR_RGW_US_GEM_MAPPING_WRITE( v, p )          MWRITE_8( (uint8_t *)p + 5, v )
#define RDD_BCR_UNKNOWN_SA_COMMAND_WRITE( v, p, i )       MWRITE_I_8( p, i + 8,  v )
#define RDD_BCR_UNKNOWN_DA_COMMAND_WRITE( v, p, i )       MWRITE_I_8( p, i + 18, v )
#define RDD_BCR_1ST_EGRESS_ETHER_TYPE_WRITE( v, p )       MWRITE_16( (uint8_t *)p + 28, v )
#define RDD_BCR_2ND_EGRESS_ETHER_TYPE_WRITE( v, p )       MWRITE_16( (uint8_t *)p + 30, v )
#define RDD_BCR_3RD_EGRESS_ETHER_TYPE_WRITE( v, p )       MWRITE_16( (uint8_t *)p + 32, v )
#define RDD_BCR_RATE_CONTROLLER_TIMER_WRITE( v, p )       MWRITE_16( (uint8_t *)p + 34, v )
#define RDD_BCR_DS_CONNECTION_MISS_ACTION_WRITE( v, p )   MWRITE_8( (uint8_t *)p + 48, v )
#define RDD_BCR_DSCP_TO_GEM_CONTROL_WRITE( v, p )         MWRITE_8( (uint8_t *)p + 49, v )

/* software version */
typedef struct
{
    /* code */
    uint8_t   code;

    /* version */
    uint8_t   version;

    /* patch */
    uint8_t   patch;

    /* revision */
    uint8_t   revision;
}
RDD_VERSION_DTS;




/****************************** Enumeration ***********************************/

typedef enum
{
    RDD_CPU_TX_MESSAGE_RX_FLOW_PM_COUNTERS_GET     = 1,
    RDD_CPU_TX_MESSAGE_TX_FLOW_PM_COUNTERS_GET     = 2,
    RDD_CPU_TX_MESSAGE_FLOW_PM_COUNTERS_GET        = 3,
    RDD_CPU_TX_MESSAGE_BRIDGE_PORT_PM_COUNTERS_GET = 4,
    RDD_CPU_TX_MESSAGE_FLUSH_ETH_QUEUE             = 5,
    RDD_CPU_TX_MESSAGE_GLOBAL_REGISTERS_GET        = 6,
    RDD_CPU_TX_MESSAGE_PM_COUNTER_GET              = 11,
}
RDD_CPU_TX_MESSAGE_TYPE;


typedef enum
{
    RDD_MAC_HASH_TYPE_INCREMENTAL = 0,
    RDD_MAC_HASH_TYPE_CRC16       = 1,
}
RDD_MAC_TABLE_HASH_TYPE;


typedef enum
{
    RDD_PARSER_LAYER2_PROTOCOL_1_HOT_UNKNOWN         = 0,
    RDD_PARSER_LAYER2_PROTOCOL_1_HOT_PPPOE_DISCOVERY = 1,
    RDD_PARSER_LAYER2_PROTOCOL_1_HOT_PPPOE_SESSION   = 2,
    RDD_PARSER_LAYER2_PROTOCOL_1_HOT_IPOE            = 4,
    RDD_PARSER_LAYER2_PROTOCOL_1_HOT_USER_DEFINED_0  = 8,
    RDD_PARSER_LAYER2_PROTOCOL_1_HOT_USER_DEFINED_1  = 16,
    RDD_PARSER_LAYER2_PROTOCOL_1_HOT_USER_DEFINED_2  = 32,
    RDD_PARSER_LAYER2_PROTOCOL_1_HOT_USER_DEFINED_3  = 64,
}
RDD_PARSER_LAYER2_PROTOCOL_1_HOT;


typedef enum
{
    RDD_FARWARDING_DIRECTION_UPSTREAM =   0,
    RDD_FARWARDING_DIRECTION_DOWNSTREAM = 1,
}
RDD_BRIDGE_FORWARDING_DIRECTION_DTS;



typedef enum
{
    CS_R8  = 0,
    CS_R9  = 1,
    CS_R10 = 2,
    CS_R11 = 4,
    CS_R12 = 5,
    CS_R13 = 6,
    CS_R14 = 8,
    CS_R15 = 9,
    CS_R16 = 10,
    CS_R17 = 12,
    CS_R18 = 13,
    CS_R19 = 14,
    CS_R20 = 16,
    CS_R21 = 17,
    CS_R22 = 18,
    CS_R23 = 20,
    CS_R24 = 21,
    CS_R25 = 22,
    CS_R26 = 24,
    CS_R27 = 25,
    CS_R28 = 26,
    CS_R29 = 28,
    CS_R30 = 29,
    CS_R31 = 30,
}
RDD_LOCAL_REGISTER_INDEX_DTS;


typedef enum
{
    FAST_RUNNER_A = 0,
    FAST_RUNNER_B = 1,
    PICO_RUNNER_A = 2,
    PICO_RUNNER_B = 3,
}
RDD_RUNNER_INDEX_DTS;



/************************** Internal variables ********************************/

static uint32_t g_runner_enable;
static uint32_t g_runner_ddr_base_addr;
static uint32_t g_runner_extra_ddr_base_addr;
static uint32_t g_cpu_tx_queue_write_ptr[ 4 ];
static RDD_WAN_PHYSICAL_PORT                  g_wan_physical_port;
static RDD_VERSION_DTS  gs_rdd_version =
{
    RDD_RELEASE,
    RDD_VERSION,
    RDD_PATCH,
    RDD_REVISION
};
static RDD_LOCK_CRITICAL_SECTION_FP    f_rdd_lock;
static RDD_UNLOCK_CRITICAL_SECTION_FP  f_rdd_unlock;



/******************* Internal functions declaration ***************************/

static RDD_ERROR f_rdd_bpm_initialize ( uint32_t, uint32_t );
static RDD_ERROR f_rdd_ddr_initialize ( uint32_t );
static RDD_ERROR f_rdd_free_packet_descriptors_pool_initialize ( void );
static RDD_ERROR f_rdd_cpu_rx_initialize ( void );
static RDD_ERROR f_rdd_cpu_tx_initialize ( void );
static RDD_ERROR f_rdd_global_registers_initialize ( void );
static RDD_ERROR f_rdd_local_registers_initialize ( void );
static RDD_ERROR f_rdd_eth_tx_initialize ( void );
static RDD_ERROR f_rdd_local_switching_initialize ( void );
//static RDD_ERROR f_rdd_make_shell_commands ( void );

static void f_dummy_lock ( uint32_t * );
static void f_dummy_unlock ( uint32_t );

extern uint32_t crcbitbybit ( uint8_t *, uint32_t, uint32_t, uint32_t, uint32_t );
extern uint32_t get_crc_init_value ( uint32_t );



/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_init                                                        */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - initialize the firmware driver                         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   resets Runner memories (data, program and context), should be called     */
/*   after reset and before any other API                                     */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_init ( void )
{
    RUNNER_INST_MAIN   *sram_fast_program_ptr;
    RUNNER_INST_PICO   *sram_pico_program_ptr;
    RUNNER_COMMON      *sram_common_data_ptr;
    RUNNER_PRIVATE     *sram_private_data_ptr;
    RUNNER_CNTXT_MAIN  *sram_fast_context_ptr;
    RUNNER_CNTXT_PICO  *sram_pico_context_ptr;
    RUNNER_PRED_MAIN   *sram_fast_prediction_ptr;
    RUNNER_PRED_PICO   *sram_pico_prediction_ptr;

    /* reset SRAM program memory of both Runners */
    sram_fast_program_ptr = ( RUNNER_INST_MAIN * )DEVICE_ADDRESS( RUNNER_INST_MAIN_0_OFFSET );
    MEMSET ( sram_fast_program_ptr, 0, sizeof ( RUNNER_INST_MAIN ) );

    sram_fast_program_ptr = ( RUNNER_INST_MAIN * )DEVICE_ADDRESS( RUNNER_INST_MAIN_1_OFFSET );
    MEMSET ( sram_fast_program_ptr, 0, sizeof ( RUNNER_INST_MAIN ) );

    sram_pico_program_ptr = ( RUNNER_INST_PICO * )DEVICE_ADDRESS( RUNNER_INST_PICO_0_OFFSET );
    MEMSET ( sram_pico_program_ptr, 0, sizeof ( RUNNER_INST_PICO ) );

    sram_pico_program_ptr = ( RUNNER_INST_PICO * )DEVICE_ADDRESS( RUNNER_INST_PICO_1_OFFSET );
    MEMSET ( sram_fast_program_ptr, 0, sizeof ( RUNNER_INST_PICO ) );

    /* reset SRAM common data memory of both Runners */
    sram_common_data_ptr = ( RUNNER_COMMON * )DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET );
    MEMSET ( sram_common_data_ptr, 0, sizeof ( RUNNER_COMMON ) );

    sram_common_data_ptr = ( RUNNER_COMMON * )DEVICE_ADDRESS( RUNNER_COMMON_1_OFFSET );
    MEMSET ( sram_common_data_ptr, 0, sizeof ( RUNNER_COMMON ) );

    /* reset SRAM private data memory of both Runners */
    sram_private_data_ptr = ( RUNNER_PRIVATE * )DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET );
    MEMSET ( sram_private_data_ptr, 0, sizeof ( RUNNER_PRIVATE ) );

    sram_private_data_ptr = ( RUNNER_PRIVATE * )DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET );
    MEMSET ( sram_private_data_ptr, 0, sizeof ( RUNNER_PRIVATE ) );

    /* reset SRAM context memory of both Runners */
    sram_fast_context_ptr = ( RUNNER_CNTXT_MAIN * )DEVICE_ADDRESS( RUNNER_CNTXT_MAIN_0_OFFSET );
    MEMSET ( sram_fast_context_ptr, 0, sizeof ( RUNNER_CNTXT_MAIN ) );

    sram_fast_context_ptr = ( RUNNER_CNTXT_MAIN * )DEVICE_ADDRESS( RUNNER_CNTXT_MAIN_1_OFFSET );
    MEMSET ( sram_fast_context_ptr, 0, sizeof ( RUNNER_CNTXT_MAIN ) );

    sram_pico_context_ptr = ( RUNNER_CNTXT_PICO * )DEVICE_ADDRESS( RUNNER_CNTXT_PICO_0_OFFSET );
    MEMSET ( sram_pico_context_ptr, 0, sizeof ( RUNNER_CNTXT_PICO ) );

    sram_pico_context_ptr = ( RUNNER_CNTXT_PICO * )DEVICE_ADDRESS( RUNNER_CNTXT_PICO_1_OFFSET );
    MEMSET ( sram_pico_context_ptr, 0, sizeof ( RUNNER_CNTXT_PICO ) );

    /* reset SRAM prediction memory of both Runners */
    sram_fast_prediction_ptr = ( RUNNER_PRED_MAIN * )DEVICE_ADDRESS( RUNNER_PRED_MAIN_0_OFFSET );
    MEMSET ( sram_fast_prediction_ptr, 0, sizeof ( RUNNER_PRED_MAIN ) * 2 );

    sram_fast_prediction_ptr = ( RUNNER_PRED_MAIN * )DEVICE_ADDRESS( RUNNER_PRED_MAIN_1_OFFSET );
    MEMSET ( sram_fast_prediction_ptr, 0, sizeof ( RUNNER_PRED_MAIN ) * 2 );

    sram_pico_prediction_ptr = ( RUNNER_PRED_PICO * )DEVICE_ADDRESS( RUNNER_PRED_PICO_0_OFFSET );
    MEMSET ( sram_pico_prediction_ptr, 0, sizeof ( RUNNER_PRED_PICO ) );

    sram_pico_prediction_ptr = ( RUNNER_PRED_PICO * )DEVICE_ADDRESS( RUNNER_PRED_PICO_1_OFFSET );
    MEMSET ( sram_pico_prediction_ptr, 0, sizeof ( RUNNER_PRED_PICO ) );

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_load_microcode                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - load the microcode into the SRAM program memory        */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   Four Runners are loaded (the code is imported as a C code array)         */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_runer_A_microcode_ptr - Fast Runner 0 microcode                       */
/*   xi_runer_B_microcode_ptr - Fast Runner 1 microcode                       */
/*   xi_runer_C_microcode_ptr - Pico Runner 0 microcode                       */
/*   xi_runer_D_microcode_ptr - Pico Runner 1 microcode                       */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_load_microcode ( uint8_t  *xi_runer_A_microcode_ptr,
                                                     uint8_t  *xi_runer_B_microcode_ptr,
                                                     uint8_t  *xi_runer_C_microcode_ptr,
                                                     uint8_t  *xi_runer_D_microcode_ptr )
{
    RUNNER_INST_MAIN  *sram_fast_program_ptr;
    RUNNER_INST_PICO  *sram_pico_program_ptr;

    /* load the code segment into the SRAM program memory of fast Runner A */
    sram_fast_program_ptr = ( RUNNER_INST_MAIN * )DEVICE_ADDRESS( RUNNER_INST_MAIN_0_OFFSET );
    MWRITE_BLK_32 ( sram_fast_program_ptr, xi_runer_A_microcode_ptr, sizeof ( RUNNER_INST_MAIN ) );

    /* load the code segment into the SRAM program memory of fast Runner B */
    sram_fast_program_ptr = ( RUNNER_INST_MAIN * )DEVICE_ADDRESS( RUNNER_INST_MAIN_1_OFFSET );
    MWRITE_BLK_32 ( sram_fast_program_ptr, xi_runer_B_microcode_ptr, sizeof ( RUNNER_INST_MAIN ) );

    /* load the code segment into the SRAM program memory of pico Runner A */
    sram_pico_program_ptr = ( RUNNER_INST_PICO * )DEVICE_ADDRESS( RUNNER_INST_PICO_0_OFFSET );
    MWRITE_BLK_32 ( sram_pico_program_ptr, xi_runer_C_microcode_ptr, sizeof ( RUNNER_INST_PICO ) );

    /* load the code segment into the SRAM program memory of pico Runner B */
    sram_pico_program_ptr = ( RUNNER_INST_PICO * )DEVICE_ADDRESS( RUNNER_INST_PICO_1_OFFSET );
    MWRITE_BLK_32 ( sram_pico_program_ptr, xi_runer_D_microcode_ptr, sizeof ( RUNNER_INST_PICO ) );

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_runner_enable                                               */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - Enables the Runner                                     */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This API move the Runner from halt mode to running mode, firmware starts */
/*   execute.                                                                 */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*   Runner_Global_Control register                                           */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_runner_enable ( void )
{
    RUNNER_REGS_CFG_GLOBAL_CTRL  runner_global_control_register;

    /* enable Runner A through the global control register */
    RUNNER_REGS_0_CFG_GLOBAL_CTRL_READ ( runner_global_control_register );
    runner_global_control_register.main_en = RDD_TRUE;
    runner_global_control_register.pico_en = RDD_TRUE;
    RUNNER_REGS_0_CFG_GLOBAL_CTRL_WRITE ( runner_global_control_register );

    /* enable Runner B through the global control register */
    RUNNER_REGS_1_CFG_GLOBAL_CTRL_READ ( runner_global_control_register );
    runner_global_control_register.main_en = RDD_TRUE;
    runner_global_control_register.pico_en = RDD_TRUE;
    RUNNER_REGS_1_CFG_GLOBAL_CTRL_WRITE ( runner_global_control_register );

    g_runner_enable = RDD_TRUE;

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_runner_disable                                              */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - Disables the Runner                                    */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This API move the Runner from running mode to halt mode, firmware is     */
/*   stoped.                                                                  */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*   Runner_Global_Control register                                           */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_runner_disable ( void )
{
    RUNNER_REGS_CFG_GLOBAL_CTRL  runner_global_control_register;

	if (g_runner_enable == RDD_TRUE)
	{
		/* enable the Runner through the global control register */
		RUNNER_REGS_0_CFG_GLOBAL_CTRL_READ ( runner_global_control_register );
		runner_global_control_register.main_en = RDD_FALSE;
		runner_global_control_register.pico_en = RDD_FALSE;
		RUNNER_REGS_0_CFG_GLOBAL_CTRL_WRITE ( runner_global_control_register );


		RUNNER_REGS_1_CFG_GLOBAL_CTRL_READ ( runner_global_control_register );
		runner_global_control_register.main_en = RDD_FALSE;
		runner_global_control_register.pico_en = RDD_FALSE;
		RUNNER_REGS_1_CFG_GLOBAL_CTRL_WRITE ( runner_global_control_register );

		g_runner_enable = RDD_FALSE;
	}

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_runner_frequency_set                                        */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - set the frequency of the Runner timers                 */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This API set the frequency of the Runner timers                          */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*   Runner_Global_Control register                                           */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_runner_frequency - Runner frequency in MHZ                            */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_runner_frequency_set ( uint16_t  xi_runner_frequency )
{
    RUNNER_REGS_CFG_GLOBAL_CTRL  runner_global_control_register;

    /* set the frequency of the Runner through the global control register */
    RUNNER_REGS_0_CFG_GLOBAL_CTRL_READ ( runner_global_control_register );
    runner_global_control_register.micro_sec_val = xi_runner_frequency - 1;
    RUNNER_REGS_0_CFG_GLOBAL_CTRL_WRITE ( runner_global_control_register );

    RUNNER_REGS_1_CFG_GLOBAL_CTRL_READ ( runner_global_control_register );
    runner_global_control_register.micro_sec_val = xi_runner_frequency - 1;
    RUNNER_REGS_1_CFG_GLOBAL_CTRL_WRITE ( runner_global_control_register );

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_data_structures_init                                        */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - initialize all firmware data structures to a default   */
/*                     state, should be called after reset and                */
/*                     bl_rdd_init() API and before any other API.      */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function configures the schedule mechansim of a single TCONT.       */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_ddr_pool_ptr - packet DDR buffer base address                         */
/*   xi_extra_ddr_pool_ptr - packet DDR buffer base address (Multicast)       */
/*   xi_mac_table_size - Mac lookup table size                                */
/*   xi_mac_table_search_depth - Mac lookup table maximum search depth        */
/*   xi_wan_physical_port - GPON EMAC5 or EMAC4                               */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_data_structures_init ( uint8_t                           *xi_ddr_pool_ptr,
                                                           uint8_t                           *xi_extra_ddr_pool_ptr,
                                                           E_RDD_MAC_TABLE_SIZE     xi_mac_table_size,
                                                           E_RDD_MAC_TABLE_SIZE     xi_iptv_table_size,
                                                           RDD_WAN_PHYSICAL_PORT  xi_wan_physical_port )
{
    /* initialize the base address of the packets in the ddr */
    g_runner_ddr_base_addr = ( uint32_t )xi_ddr_pool_ptr;
    g_runner_extra_ddr_base_addr = ( uint32_t )xi_extra_ddr_pool_ptr;

    g_wan_physical_port = xi_wan_physical_port;

    /* initialize the RDD lock/Unlock mechanism */
    f_rdd_lock = f_dummy_lock;
    f_rdd_unlock = f_dummy_unlock;

    /* initialize the base address of the BPM base address */
    f_rdd_bpm_initialize ( g_runner_ddr_base_addr, g_runner_extra_ddr_base_addr );

    /* initialize runner dma base address */
    f_rdd_ddr_initialize ( g_runner_ddr_base_addr );

    /* create the Runner's free packet descriptors pool */
    f_rdd_free_packet_descriptors_pool_initialize ();

    /* initialize the CPU-RX mechanism */
    f_rdd_cpu_rx_initialize ();

    /* initialize the CPU-TX queue */
    f_rdd_cpu_tx_initialize ();

    /* initialize global registers */
    f_rdd_global_registers_initialize ();

    /* initialize the local registers through the Context memory */
    f_rdd_local_registers_initialize ();

    /* initialize ethernet tx queues and ports */
    f_rdd_eth_tx_initialize ();

    /* initialize local switching */
    f_rdd_local_switching_initialize ();

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   f_rdd_bpm_initialize                                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Runner Initialization - initialize BPM                                   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function returns the status of the operation                        */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_runner_ddr_pool_ptr - Packet DDR buffer base address                  */
/*   xi_extra_ddr_pool_ptr - Packet DDR buffer base address (Multicast)       */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   none                                                                     */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
static RDD_ERROR f_rdd_bpm_initialize ( uint32_t  xi_runner_ddr_pool_ptr,
                                                     uint32_t  xi_runner_extra_ddr_pool_ptr )
{
    uint32_t  *bpm_ddr_base_ptr;
    uint32_t  *bpm_extra_ddr_base_ptr;

    bpm_ddr_base_ptr = ( uint32_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) + BPM_DDR_BUFFERS_BASE_ADDRESS );
    bpm_extra_ddr_base_ptr = ( uint32_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) + BPM_EXTRA_DDR_BUFFERS_BASE_ADDRESS );

    MWRITE_32( bpm_ddr_base_ptr, xi_runner_ddr_pool_ptr & 0x1FFFFFFF );
    MWRITE_32( bpm_extra_ddr_base_ptr, xi_runner_extra_ddr_pool_ptr & 0x1FFFFFFF );

    bpm_ddr_base_ptr = ( uint32_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) + BPM_DDR_BUFFERS_BASE_ADDRESS );
    bpm_extra_ddr_base_ptr = ( uint32_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) + BPM_EXTRA_DDR_BUFFERS_BASE_ADDRESS );

    MWRITE_32( bpm_ddr_base_ptr, xi_runner_ddr_pool_ptr & 0x1FFFFFFF );
    MWRITE_32( bpm_extra_ddr_base_ptr, xi_runner_extra_ddr_pool_ptr & 0x1FFFFFFF );

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   f_rdd_ddr_initialize                                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Runner Initialization - initialize the runner ddr config register        */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function returns the status of the operation                        */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*   DDR_config Register                                                      */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_runner_ddr_pool_ptr - Packet DDR buffer base address                  */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   none                                                                     */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
static RDD_ERROR f_rdd_ddr_initialize ( uint32_t  xi_runner_ddr_pool_ptr )
{
#if !defined(FIRMWARE_INIT)
    RUNNER_REGS_CFG_DDR_CFG         runner_ddr_config_register;
    RUNNER_REGS_CFG_DDR_LKUP_MASK0  runner_ddr_lkup_mask0_register;

    runner_ddr_config_register.buffer_offset = RDD_PACKET_DDR_OFFSET;
    runner_ddr_config_register.rserved1 = 0;
    runner_ddr_config_register.buffer_size = ( RDD_RUNNER_PACKET_BUFFER_SIZE >> 12 );
    runner_ddr_config_register.dma_base = ( xi_runner_ddr_pool_ptr & 0x07E00000 ) >> 21;
    runner_ddr_config_register.rserved2 = 0;

    RUNNER_REGS_0_CFG_DDR_CFG_WRITE ( runner_ddr_config_register );
    RUNNER_REGS_1_CFG_DDR_CFG_WRITE ( runner_ddr_config_register );


    runner_ddr_lkup_mask0_register.global_mask = 0x000001FF;

    RUNNER_REGS_0_CFG_DDR_LKUP_MASK0_WRITE ( runner_ddr_lkup_mask0_register );
    RUNNER_REGS_1_CFG_DDR_LKUP_MASK0_WRITE ( runner_ddr_lkup_mask0_register );
#endif

    return ( RDD_OK );
}

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   f_rdd_free_packet_descriptors_pool_initialize                            */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Runner Initialization - initialize the list of the free buffers pool     */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   Upstream pool is implemented as a stack of 2048 packet descriptors       */
/*   Downstream pool is implemented as a list of 1024 packet descriptors      */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   none                                                                     */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
static RDD_ERROR f_rdd_free_packet_descriptors_pool_initialize ( void )
{
    RDD_FREE_PACKET_DESCRIPTORS_POOL_DTS             *free_packet_descriptors_pool_ptr;
    RDD_PACKET_DESCRIPTOR_DTS                        *packet_descriptor_ptr;
    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_DTS  *free_packet_descriptors_pool_descriptor_ptr;
    uint32_t                                             next_packet_descriptor_address;
    uint32_t                                             i;

    free_packet_descriptors_pool_ptr = ( RDD_FREE_PACKET_DESCRIPTORS_POOL_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) +
                                                                                        DOWNSTREAM_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS );

    /* create the free packet descriptors pool as a list of packet descriptors */
    for ( i = 0; i < RDD_DOWNSTREAM_FREE_PACKET_DESCRIPTORS_POOL_SIZE; i++ )
    {
        packet_descriptor_ptr = &( free_packet_descriptors_pool_ptr->packet_descriptor[ i ] );

        /* the last packet descriptor should point to NULL, the others points to the next packet descriptor */
        if ( i == ( RDD_DOWNSTREAM_FREE_PACKET_DESCRIPTORS_POOL_SIZE - 1 ) )
        {
            next_packet_descriptor_address = 0;
        }
        else
        {
            next_packet_descriptor_address = DOWNSTREAM_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS + ( i + 1 ) * RDD_RUNNER_PACKET_DESCRIPTOR_SIZE;
        }

        RDD_PACKET_DESCRIPTOR_NEXT_PTR_WRITE ( next_packet_descriptor_address, packet_descriptor_ptr );
    }

    free_packet_descriptors_pool_descriptor_ptr =
        ( RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) +
                                                                    FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_ADDRESS );

    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_HEAD_PTR_WRITE ( DOWNSTREAM_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS,
                                                                       free_packet_descriptors_pool_descriptor_ptr );

    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_TAIL_PTR_WRITE ( DOWNSTREAM_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS +
                                                                       ( RDD_DOWNSTREAM_FREE_PACKET_DESCRIPTORS_POOL_SIZE - 1 ) *
                                                                       RDD_RUNNER_PACKET_DESCRIPTOR_SIZE,
                                                                       free_packet_descriptors_pool_descriptor_ptr );

    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_INGRESS_WRITE ( RDD_DOWNSTREAM_FREE_PACKET_DESCRIPTORS_POOL_SIZE - 1,
                                                                      free_packet_descriptors_pool_descriptor_ptr );

    free_packet_descriptors_pool_ptr = ( RDD_FREE_PACKET_DESCRIPTORS_POOL_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) +
                                                                                        UPSTREAM_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS );

    /* create the free packet descriptors pool as a stack of packet descriptors */
    for ( i = 0; i < RDD_UPSTREAM_FREE_PACKET_DESCRIPTORS_POOL_SIZE; i++ )
    {
        packet_descriptor_ptr = &( free_packet_descriptors_pool_ptr->packet_descriptor[ i ] );

        /* the last packet descriptor should point to NULL, the others points to the next packet descriptor */
        if ( i == ( RDD_UPSTREAM_FREE_PACKET_DESCRIPTORS_POOL_SIZE - 1 ) )
        {
            next_packet_descriptor_address = 0;
        }
        else
        {
            next_packet_descriptor_address = UPSTREAM_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS + ( i + 1 ) * RDD_RUNNER_PACKET_DESCRIPTOR_SIZE;
        }

        RDD_PACKET_DESCRIPTOR_NEXT_PTR_WRITE ( next_packet_descriptor_address, packet_descriptor_ptr );
    }

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   f_rdd_cpu_rx_initialize                                                  */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Runner Initialization - initialize CPU-RX mechanism                      */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   CPU pool is implemented as a list of 256 packet descriptors, each queue  */
/*   is initilaize with a dummy packet descriptors for race condition         */
/*   prevention                                                               */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   none                                                                     */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
static RDD_ERROR f_rdd_cpu_rx_initialize ( void )
{
    RDD_FREE_PACKET_DESCRIPTORS_POOL_DTS             *cpu_free_packet_descriptors_pool_ptr;
    RDD_PACKET_DESCRIPTOR_DTS                        *packet_descriptor_ptr;
    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_DTS  *cpu_free_packet_descriptors_pool_descriptor_ptr;
    RDD_CPU_RX_TABLE_DTS                             *cpu_rx_table_ptr;
    RDD_CPU_RX_QUEUE_DESCRIPTOR_DTS                  *cpu_rx_queue_descriptor_ptr;
    RDD_CPU_REASON_TO_CPU_RX_QUEUE_TABLE_DTS         *cpu_reason_to_cpu_rx_queue_table_ptr;
    RDD_CPU_REASON_TO_CPU_RX_QUEUE_ENTRY_DTS         *cpu_reason_to_cpu_rx_queue_entry_ptr;
    RDD_CPU_REASON_TO_METER_TABLE_DTS                *cpu_reason_to_meter_table_ptr;
    RDD_CPU_REASON_TO_METER_ENTRY_DTS                *cpu_reason_to_meter_entry_ptr;
    uint16_t                                             *cpu_rx_ingress_queue_ptr;
    uint16_t                                             packet_descriptor_address;
    uint16_t                                             next_packet_descriptor_address;
    uint16_t                                             packet_descriptors_counter;
    uint8_t                                              cpu_reason;
    uint8_t                                              cpu_queue;
    uint32_t                                             i;

    cpu_free_packet_descriptors_pool_ptr =
        ( RDD_FREE_PACKET_DESCRIPTORS_POOL_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_1_OFFSET ) +
                                                         CPU_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS - sizeof ( RUNNER_COMMON ) );

    /* create the CPU free packet descriptors pool as a list of packet descriptors */
    for ( i = 0; i < RDD_CPU_FREE_PACKET_DESCRIPTORS_POOL_SIZE; i++ )
    {
        packet_descriptor_ptr = &( cpu_free_packet_descriptors_pool_ptr->packet_descriptor[ i ] );

        /* the last packet descriptor should point to NULL, the others points to the next packet descriptor */
        if ( i == ( RDD_CPU_FREE_PACKET_DESCRIPTORS_POOL_SIZE - 1 ) )
        {
            next_packet_descriptor_address = 0;
        }
        else
        {
            next_packet_descriptor_address = CPU_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS + ( i + 1 ) * RDD_RUNNER_PACKET_DESCRIPTOR_SIZE;
        }

        RDD_PACKET_DESCRIPTOR_NEXT_PTR_WRITE ( next_packet_descriptor_address, packet_descriptor_ptr );
    }

    cpu_free_packet_descriptors_pool_descriptor_ptr =
        ( RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) +
                                                                    CPU_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_ADDRESS );

    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_HEAD_PTR_WRITE ( CPU_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS,
                                                                       cpu_free_packet_descriptors_pool_descriptor_ptr );
    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_TAIL_PTR_WRITE ( CPU_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS +
                                                                       ( RDD_CPU_FREE_PACKET_DESCRIPTORS_POOL_SIZE - 1 ) *
                                                                       RDD_RUNNER_PACKET_DESCRIPTOR_SIZE,
                                                                       cpu_free_packet_descriptors_pool_descriptor_ptr );
    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_INGRESS_WRITE ( RDD_CPU_FREE_PACKET_DESCRIPTORS_POOL_SIZE - 1,
                                                                      cpu_free_packet_descriptors_pool_descriptor_ptr );


    /* allocate packet descriptor for each CPU-RX queue */
    cpu_rx_table_ptr = ( RDD_CPU_RX_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) + CPU_RX_QUEUES_TABLE_ADDRESS );

    packet_descriptors_counter = 0;
	cpu_queue = RDD_CPU_RX_QUEUE_0;
    //for ( cpu_queue = RDD_CPU_RX_QUEUE_0; cpu_queue <= RDD_CPU_RX_QUEUE_7; cpu_queue++ )
    {
        cpu_rx_queue_descriptor_ptr = &( cpu_rx_table_ptr->entry[ cpu_queue ] );

        RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_HEAD_PTR_READ ( packet_descriptor_address, cpu_free_packet_descriptors_pool_descriptor_ptr );

        RDD_CPU_RX_QUEUE_HEAD_POINTER_WRITE ( packet_descriptor_address, cpu_rx_queue_descriptor_ptr );
        RDD_CPU_RX_QUEUE_TAIL_POINTER_WRITE ( packet_descriptor_address, cpu_rx_queue_descriptor_ptr );

        packet_descriptor_ptr = ( RDD_PACKET_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_1_OFFSET ) +
                                                                      packet_descriptor_address - sizeof ( RUNNER_COMMON ) );

        RDD_PACKET_DESCRIPTOR_NEXT_PTR_READ ( next_packet_descriptor_address, packet_descriptor_ptr );
        RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_HEAD_PTR_WRITE ( next_packet_descriptor_address,
                                                                           cpu_free_packet_descriptors_pool_descriptor_ptr );

        next_packet_descriptor_address = 0;
        RDD_PACKET_DESCRIPTOR_NEXT_PTR_WRITE ( next_packet_descriptor_address, packet_descriptor_ptr );

        packet_descriptors_counter++;
    }

    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_EGRESS_WRITE ( packet_descriptors_counter, cpu_free_packet_descriptors_pool_descriptor_ptr );


    cpu_rx_ingress_queue_ptr = ( uint16_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) + CPU_RX_FAST_INGRESS_QUEUE_PTR_ADDRESS );

    MWRITE_16 ( cpu_rx_ingress_queue_ptr, CPU_RX_FAST_INGRESS_QUEUE_ADDRESS );

    cpu_rx_ingress_queue_ptr = ( uint16_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) + CPU_RX_FAST_INGRESS_QUEUE_PTR_ADDRESS );

    MWRITE_16 ( cpu_rx_ingress_queue_ptr, CPU_RX_FAST_INGRESS_QUEUE_ADDRESS );


    cpu_rx_ingress_queue_ptr = ( uint16_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) + CPU_RX_PICO_INGRESS_QUEUE_PTR_ADDRESS );

    MWRITE_16 ( cpu_rx_ingress_queue_ptr, CPU_RX_PICO_INGRESS_QUEUE_ADDRESS );

    cpu_rx_ingress_queue_ptr = ( uint16_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) + CPU_RX_PICO_INGRESS_QUEUE_PTR_ADDRESS );

    MWRITE_16 ( cpu_rx_ingress_queue_ptr, CPU_RX_PICO_INGRESS_QUEUE_ADDRESS );

    cpu_reason_to_cpu_rx_queue_table_ptr = ( RDD_CPU_REASON_TO_CPU_RX_QUEUE_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) +
                                                                                                CPU_RX_REASON_TO_QUEUE_TABLE_ADDRESS );

    for ( cpu_reason = RDD_CPU_RX_REASON_DIRECT_QUEUE_0, cpu_queue = RDD_CPU_RX_QUEUE_0;
          cpu_reason <= RDD_CPU_RX_REASON_DIRECT_QUEUE_7;
          cpu_reason++, cpu_queue++)
    {
        cpu_reason_to_cpu_rx_queue_entry_ptr = &( cpu_reason_to_cpu_rx_queue_table_ptr->entry[ cpu_reason ] );
        RDD_CPU_REASON_TO_QUEUE_ENTRY_WRITE ( cpu_queue, cpu_reason_to_cpu_rx_queue_entry_ptr );
    }


    for ( cpu_reason= RDD_CPU_RX_REASON_PLOAM; cpu_reason <= RDD_CPU_RX_REASON_MAX_REASON; cpu_reason++ )
    {
        cpu_reason_to_meter_table_ptr = ( RDD_CPU_REASON_TO_METER_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) +
                                                                                      CPU_RX_REASON_TO_METER_TABLE_ADDRESS );

        cpu_reason_to_meter_entry_ptr = &( cpu_reason_to_meter_table_ptr->entry[ cpu_reason ] );

        RDD_CPU_REASON_TO_METER_ENTRY_WRITE ( RDD_METER_DISABLE, cpu_reason_to_meter_entry_ptr );

        cpu_reason_to_meter_table_ptr = ( RDD_CPU_REASON_TO_METER_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) +
                                                                                      CPU_RX_REASON_TO_METER_TABLE_ADDRESS );

        cpu_reason_to_meter_entry_ptr = &( cpu_reason_to_meter_table_ptr->entry[ cpu_reason ] );

        RDD_CPU_REASON_TO_METER_ENTRY_WRITE ( RDD_METER_DISABLE, cpu_reason_to_meter_entry_ptr );
    }

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   f_rdd_cpu_tx_initialize                                                  */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Runner Initialization - initialize CPU-TX queues pointers                */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   none                                                                     */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
static RDD_ERROR f_rdd_cpu_tx_initialize ( void )
{
    uint32_t  *ih_header_descriptor_ptr;
    uint32_t  ih_header_descriptor[ 2 ];

    ih_header_descriptor_ptr = ( uint32_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) + RUNNER_FLOW_HEADER_DESCRIPTOR_ADDRESS );

    ih_header_descriptor[ 0 ] = ( RDD_ON << 5 ) + ( CPU_TX_FAST_THREAD_NUMBER << 6 );

    ih_header_descriptor[ 1 ] = GPON_SRC_PORT + ( RDD_IH_HEADER_LENGTH << 5 ) + ( RDD_RUNNER_A_IH_BUFFER << 20 );

    MWRITE_32( ( ( uint8_t * )ih_header_descriptor_ptr + 0 ), ih_header_descriptor[ 0 ] );
    MWRITE_32( ( ( uint8_t * )ih_header_descriptor_ptr + 4 ), ih_header_descriptor[ 1 ] );


    ih_header_descriptor_ptr = ( uint32_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) + RUNNER_FLOW_HEADER_DESCRIPTOR_ADDRESS );

    ih_header_descriptor[ 0 ] = ( RDD_ON << 5 ) + ( CPU_TX_FAST_THREAD_NUMBER << 6 );

    ih_header_descriptor[ 1 ] = ( RDD_IH_HEADER_LENGTH << 5 ) + ( RDD_RUNNER_B_IH_BUFFER << 20 );

    MWRITE_32( ( ( uint8_t * )ih_header_descriptor_ptr + 0 ), ih_header_descriptor[ 0 ] );
    MWRITE_32( ( ( uint8_t * )ih_header_descriptor_ptr + 4 ), ih_header_descriptor[ 1 ] );

    g_cpu_tx_queue_write_ptr[ 0 ] = CPU_TX_FAST_QUEUE_ADDRESS;
    g_cpu_tx_queue_write_ptr[ 1 ] = CPU_TX_FAST_QUEUE_ADDRESS;
    g_cpu_tx_queue_write_ptr[ 2 ] = CPU_TX_PICO_QUEUE_ADDRESS;
    g_cpu_tx_queue_write_ptr[ 3 ] = CPU_TX_PICO_QUEUE_ADDRESS;

    return ( RDD_OK );
}

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   f_rdd_global_registers_initialize                                        */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Runner Initialization - initialize the global registers (R1-R7)          */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*   Runners global registers (R1-R7)                                         */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   none                                                                     */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
static RDD_ERROR f_rdd_global_registers_initialize ( void )
{
    uint32_t  *global_register_init_ptr;
    uint32_t  global_register[ 8 ];


    /********** Fast Runner A **********/

    /* zero all global registers */
    memset ( global_register, 0, sizeof ( global_register ) );

    /* vlan action cyclic ingress queue pointer */
    global_register[ 5 ] = ( VLAN_ACTION_INGRESS_QUEUE_ADDRESS << 16 ) | DOWNSTREAM_MULTICAST_INGRESS_QUEUE_ADDRESS;

    global_register_init_ptr = ( uint32_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) + FAST_RUNNER_GLOBAL_REGISTERS_INIT_ADDRESS );

    /* copy the global regsiters to the data SRAM, the firmware will load it from the SRAM at task -1 (initialization task) */
    MWRITE_BLK_32 ( global_register_init_ptr, global_register, sizeof ( global_register ) );


    /********** Fast Runner B **********/

    /* zero all global registers */
    memset ( global_register, 0, sizeof ( global_register ) );

    /* vlan action cyclic ingress queue pointer */
    global_register[ 5 ] = ( VLAN_ACTION_INGRESS_QUEUE_ADDRESS << 16 ) + VLAN_ACTION_INGRESS_QUEUE_ADDRESS;
    global_register[ 6 ] =  ROUTER_INGRESS_QUEUE_ADDRESS;

    global_register_init_ptr = ( uint32_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) + FAST_RUNNER_GLOBAL_REGISTERS_INIT_ADDRESS );

    MWRITE_BLK_32 ( global_register_init_ptr, global_register, sizeof ( global_register ) );


    /********** Pico Runner A **********/

    /* zero all global registers */
    memset ( global_register, 0, sizeof ( global_register ) );

    global_register[ 3 ] = 64 - 1;

    /* vlan action cyclic ingress queue pointer */
    global_register[ 5 ] = ( VLAN_ACTION_INGRESS_QUEUE_ADDRESS << 16 ) | DOWNSTREAM_MULTICAST_INGRESS_QUEUE_ADDRESS;

    global_register_init_ptr = ( uint32_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) + PICO_RUNNER_GLOBAL_REGISTERS_INIT_ADDRESS );

    /* copy the global regsiters to the data SRAM, the firmware will load it from the SRAM at task -1 (initialization task) */
    MWRITE_BLK_32 ( global_register_init_ptr, global_register, sizeof ( global_register ) );


    /********** Pico Runner B **********/

    /* zero all global registers */
    memset ( global_register, 0, sizeof ( global_register ) );

    /* R2 - head pointer of the free buffers pool stack */
    global_register[ 2 ] = UPSTREAM_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS;

    global_register[ 3 ] = 64;

    /* R4 - free buffers pool counter */
    global_register[ 4 ] = RDD_UPSTREAM_FREE_PACKET_DESCRIPTORS_POOL_SIZE;

    /* vlan action cyclic ingress queue pointer */
    global_register[ 5 ] = VLAN_ACTION_INGRESS_QUEUE_ADDRESS << 16;

    global_register_init_ptr = ( uint32_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) + PICO_RUNNER_GLOBAL_REGISTERS_INIT_ADDRESS );

    /* copy the global regsiters to the data SRAM, the firmware will load it from the SRAM at task -1 (initialization task) */
    MWRITE_BLK_32 ( global_register_init_ptr, global_register, sizeof ( global_register ) );

    return ( RDD_OK );
}


#if !defined (FIRMWARE_INIT) && !defined (FSSIM)

static void memcpyl_context (void *__to, void *__from, unsigned int __n)
{
    unsigned int src = (unsigned int )__from;
    unsigned int dst = (unsigned int )__to;
    int          i;

    for ( i = 0; i < ( __n / 4 ); i++, src+=4, dst+=4 )
    {
        if ( ( i & 0x3 ) == 3 )
            continue;

        *( volatile unsigned int * )dst = *( volatile unsigned int * )src;
    }
}
#endif


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   f_rdd_local_registers_initialize                                         */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Runner Initialization - initialize context memeories of 4 Rnners         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   initialize the local registers (R8-R31), 32 threads for fast Runners     */
/*   and 16 threads for Pico Runners                                          */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*   Runners local registers (R8-R31)                                         */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   none                                                                     */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
static RDD_ERROR f_rdd_local_registers_initialize ( void )
{
    RUNNER_CNTXT_MAIN  *sram_fast_context_ptr;
    RUNNER_CNTXT_PICO  *sram_pico_context_ptr;
    static uint32_t      local_register[ 32 ][ 32 ];

    /********** Fast Runner A **********/

    sram_fast_context_ptr = ( RUNNER_CNTXT_MAIN * )DEVICE_ADDRESS( RUNNER_CNTXT_MAIN_0_OFFSET );

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32 ( local_register, sram_fast_context_ptr, sizeof ( RUNNER_CNTXT_MAIN ) );

    /* CPU TX fast: thread 0 */
    local_register[ CPU_TX_FAST_THREAD_NUMBER ][ CS_R16 ] = 0xB00 << 16;
    local_register[ CPU_TX_FAST_THREAD_NUMBER ][ CS_R8  ] = CPU_TX_FAST_QUEUE_ADDRESS;
    local_register[ CPU_TX_FAST_THREAD_NUMBER ][ CS_R9  ] = CPU_TX_BBH_DESCRIPTORS_ADDRESS | ( RUNNER_A_LAN_ENQUEUE_INGRESS_QUEUE_ADDRESS << 16 );
    local_register[ CPU_TX_FAST_THREAD_NUMBER ][ CS_R10 ] = ( BBH_PERIPHERAL_IH << 16 ) | ( RDD_IH_BUFFER_BBH_ADDRESS +
                                                                                            RDD_RUNNER_A_IH_BUFFER_BBH_OFFSET );
    local_register[ CPU_TX_FAST_THREAD_NUMBER ][ CS_R11 ] = ( BBH_PERIPHERAL_IH << 16 ) | RDD_IH_HEADER_DESCRIPTOR_BBH_ADDRESS;

    /* CPU-RX: thread 1 */
    local_register[ CPU_RX_THREAD_NUMBER ][ CS_R16 ] = 0x300 << 16;
    local_register[ CPU_RX_THREAD_NUMBER ][ CS_R9  ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ CPU_RX_THREAD_NUMBER ][ CS_R10 ] = CPU_RX_FAST_INGRESS_QUEUE_ADDRESS + ( CPU_RX_PICO_INGRESS_QUEUE_ADDRESS << 16 );
    local_register[ CPU_RX_THREAD_NUMBER ][ CS_R11 ] = CPU_RX_REASON_TO_QUEUE_TABLE_ADDRESS + ( ( CPU_RX_QUEUES_TABLE_ADDRESS ) << 16 );
    local_register[ CPU_RX_THREAD_NUMBER ][ CS_R12 ] = CPU_RX_REASON_TO_METER_TABLE_ADDRESS;

    /* Local Switching ETH0: thread 16 */
    local_register[ LOCAL_SWITCHING_ETH0_THREAD_NUMBER ][ CS_R16 ] = 0x3000 << 16;
    local_register[ LOCAL_SWITCHING_ETH0_THREAD_NUMBER ][ CS_R8  ] = ETH0_RX_DIRECT_DESCRIPTORS_ADDRESS;
    local_register[ LOCAL_SWITCHING_ETH0_THREAD_NUMBER ][ CS_R9  ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ LOCAL_SWITCHING_ETH0_THREAD_NUMBER ][ CS_R12 ] = ( BBH_PERIPHERAL_ETH0_RX << 16 ) | RDD_LAN0_BRIDGE_PORT;

    /* Local Switching ETH1: thread 17 */
    local_register[ LOCAL_SWITCHING_ETH1_THREAD_NUMBER ][ CS_R16 ] = 0x3000 << 16;
    local_register[ LOCAL_SWITCHING_ETH1_THREAD_NUMBER ][ CS_R8  ] = ETH1_RX_DIRECT_DESCRIPTORS_ADDRESS;
    local_register[ LOCAL_SWITCHING_ETH1_THREAD_NUMBER ][ CS_R9  ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ LOCAL_SWITCHING_ETH1_THREAD_NUMBER ][ CS_R12 ] = ( BBH_PERIPHERAL_ETH1_RX << 16 ) | RDD_LAN1_BRIDGE_PORT;

    /* Local Switching ETH2: thread 18 */
    local_register[ LOCAL_SWITCHING_ETH2_THREAD_NUMBER ][ CS_R16 ] = 0x3000 << 16;
    local_register[ LOCAL_SWITCHING_ETH2_THREAD_NUMBER ][ CS_R8  ] = ETH2_RX_DIRECT_DESCRIPTORS_ADDRESS;
    local_register[ LOCAL_SWITCHING_ETH2_THREAD_NUMBER ][ CS_R9  ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ LOCAL_SWITCHING_ETH2_THREAD_NUMBER ][ CS_R12 ] = ( BBH_PERIPHERAL_ETH2_RX << 16 ) | RDD_LAN2_BRIDGE_PORT;

    /* Local Switching ETH3: thread 19 */
    local_register[ LOCAL_SWITCHING_ETH3_THREAD_NUMBER ][ CS_R16 ] = 0x3000 << 16;
    local_register[ LOCAL_SWITCHING_ETH3_THREAD_NUMBER ][ CS_R8  ] = ETH3_RX_DIRECT_DESCRIPTORS_ADDRESS;
    local_register[ LOCAL_SWITCHING_ETH3_THREAD_NUMBER ][ CS_R9  ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ LOCAL_SWITCHING_ETH3_THREAD_NUMBER ][ CS_R12 ] = ( BBH_PERIPHERAL_ETH3_RX << 16 ) | RDD_LAN3_BRIDGE_PORT;

    /* Local Switching ETH4: thread 20 */
    local_register[ LOCAL_SWITCHING_ETH4_THREAD_NUMBER ][ CS_R16 ] = 0x3000 << 16;
    local_register[ LOCAL_SWITCHING_ETH4_THREAD_NUMBER ][ CS_R8  ] = ETH4_RX_DIRECT_DESCRIPTORS_ADDRESS;
    local_register[ LOCAL_SWITCHING_ETH4_THREAD_NUMBER ][ CS_R9  ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ LOCAL_SWITCHING_ETH4_THREAD_NUMBER ][ CS_R12 ] = ( BBH_PERIPHERAL_ETH4_RX << 16 ) | RDD_LAN4_BRIDGE_PORT;

    memcpyl_context ( sram_fast_context_ptr, local_register, sizeof ( RUNNER_CNTXT_MAIN ) );


    /********** Fast Runner B **********/

    sram_fast_context_ptr = ( RUNNER_CNTXT_MAIN * )DEVICE_ADDRESS( RUNNER_CNTXT_MAIN_1_OFFSET );

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32 ( local_register, sram_fast_context_ptr, sizeof ( RUNNER_CNTXT_MAIN ) );

    /* CPU TX fast: thread 0 */
    local_register[ CPU_TX_FAST_THREAD_NUMBER ][ CS_R16 ] = 0x900 << 16;
    local_register[ CPU_TX_FAST_THREAD_NUMBER ][ CS_R8  ] = CPU_TX_FAST_QUEUE_ADDRESS;
    local_register[ CPU_TX_FAST_THREAD_NUMBER ][ CS_R9  ] = CPU_TX_BBH_DESCRIPTORS_ADDRESS | ( RUNNER_B_LAN_ENQUEUE_INGRESS_QUEUE_ADDRESS << 16 );
    local_register[ CPU_TX_FAST_THREAD_NUMBER ][ CS_R10 ] = ( BBH_PERIPHERAL_IH << 16 ) | ( RDD_IH_BUFFER_BBH_ADDRESS +
                                                                                            RDD_RUNNER_B_IH_BUFFER_BBH_OFFSET );
    local_register[ CPU_TX_FAST_THREAD_NUMBER ][ CS_R11 ] = ( BBH_PERIPHERAL_IH << 16 ) | RDD_IH_HEADER_DESCRIPTOR_BBH_ADDRESS;

    /* CPU-RX: thread 2 */
    local_register[ CPU_RX_THREAD_NUMBER ][ CS_R16 ] = 0x300 << 16;
    local_register[ CPU_RX_THREAD_NUMBER ][ CS_R9  ] = ( IH_RUNNER_B_BUFFER_MASK << 16 ) | INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ CPU_RX_THREAD_NUMBER ][ CS_R10 ] = CPU_RX_FAST_INGRESS_QUEUE_ADDRESS + ( CPU_RX_PICO_INGRESS_QUEUE_ADDRESS << 16 );
    local_register[ CPU_RX_THREAD_NUMBER ][ CS_R11 ] = CPU_RX_REASON_TO_QUEUE_TABLE_ADDRESS + ( ( CPU_RX_QUEUES_TABLE_ADDRESS ) << 16 );
    local_register[ CPU_RX_THREAD_NUMBER ][ CS_R12 ] = CPU_RX_REASON_TO_METER_TABLE_ADDRESS;

    /* LAN-0 Filters and Classification : thread 8 */
    local_register[ LAN0_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R16 ] = ( 0x1900 << 16 );
    local_register[ LAN0_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R8  ] = ETH0_RX_DESCRIPTORS_ADDRESS;
    local_register[ LAN0_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R9  ] = ( ROUTER_INGRESS_QUEUE_ADDRESS << 16 ) |
                                                                                  INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ LAN0_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R12 ] = RDD_LAN0_BRIDGE_PORT | ( BBH_PERIPHERAL_ETH0_RX << 16 );
    local_register[ LAN0_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R13 ] = RDD_CAM_RESULT_SLOT1 |
                                                                                ( RDD_CAM_RESULT_SLOT1_IO_ADDRESS << 16 );
    local_register[ LAN0_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R15 ] = ( RDD_DMA_LOOKUP_RESULT_SLOT0 << 5 ) |
                                                                                RDD_DMA_LOOKUP_4_STEPS |
                                                                                ( RDD_DMA_LOOKUP_RESULT_SLOT0_IO_ADDRESS << 16 );

    /* LAN-1 Filters and Classification : thread 9 */
    local_register[ LAN1_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R16 ] = 0x1900 << 16;
    local_register[ LAN1_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R8  ] = ETH1_RX_DESCRIPTORS_ADDRESS;
    local_register[ LAN1_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R9  ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ LAN1_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R12 ] = RDD_LAN1_BRIDGE_PORT | ( BBH_PERIPHERAL_ETH1_RX << 16 );
    local_register[ LAN1_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R13 ] = RDD_CAM_RESULT_SLOT2 |
                                                                                ( RDD_CAM_RESULT_SLOT2_IO_ADDRESS << 16 );
    local_register[ LAN1_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R15 ] = ( RDD_DMA_LOOKUP_RESULT_SLOT1 << 5 ) |
                                                                                RDD_DMA_LOOKUP_4_STEPS |
                                                                                ( RDD_DMA_LOOKUP_RESULT_SLOT1_IO_ADDRESS << 16 );

    /* LAN-2 Filters and Classification : thread 10 */
    local_register[ LAN2_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R16 ] = 0x1900 << 16;
    local_register[ LAN2_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R8  ] = ETH2_RX_DESCRIPTORS_ADDRESS;
    local_register[ LAN2_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R9  ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ LAN2_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R12 ] = RDD_LAN2_BRIDGE_PORT | ( BBH_PERIPHERAL_ETH2_RX << 16 );
    local_register[ LAN2_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R13 ] = RDD_CAM_RESULT_SLOT3 |
                                                                                ( RDD_CAM_RESULT_SLOT3_IO_ADDRESS << 16 );
    local_register[ LAN2_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R15 ] = ( RDD_DMA_LOOKUP_RESULT_SLOT2 << 5 ) |
                                                                                RDD_DMA_LOOKUP_4_STEPS |
                                                                                ( RDD_DMA_LOOKUP_RESULT_SLOT2_IO_ADDRESS << 16 );

    /* LAN-3 Filters and Classification : thread 11 */
    local_register[ LAN3_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R16 ] = 0x1900 << 16;
    local_register[ LAN3_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R8  ] = ETH3_RX_DESCRIPTORS_ADDRESS;
    local_register[ LAN3_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R9  ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ LAN3_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R12 ] = RDD_LAN3_BRIDGE_PORT | ( BBH_PERIPHERAL_ETH3_RX << 16 );
    local_register[ LAN3_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R13 ] = RDD_CAM_RESULT_SLOT4 |
                                                                                ( RDD_CAM_RESULT_SLOT4_IO_ADDRESS << 16 );
    local_register[ LAN3_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R15 ] = ( RDD_DMA_LOOKUP_RESULT_SLOT3 << 5 ) |
                                                                                RDD_DMA_LOOKUP_4_STEPS |
                                                                                ( RDD_DMA_LOOKUP_RESULT_SLOT3_IO_ADDRESS << 16 );

    /* LAN-4 Filters and Classification : thread 12 */
    local_register[ LAN4_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R16 ] = 0x1900 << 16;
    local_register[ LAN4_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R8  ] = ETH4_RX_DESCRIPTORS_ADDRESS;
    local_register[ LAN4_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R9  ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ LAN4_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R12 ] = RDD_LAN4_BRIDGE_PORT | ( BBH_PERIPHERAL_ETH4_RX << 16 );
    local_register[ LAN4_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R13 ] = RDD_CAM_RESULT_SLOT5 |
                                                                                ( RDD_CAM_RESULT_SLOT5_IO_ADDRESS << 16 );
    local_register[ LAN4_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R15 ] = ( RDD_DMA_LOOKUP_RESULT_SLOT4 << 5 ) |
                                                                                RDD_DMA_LOOKUP_4_STEPS |
                                                                                ( RDD_DMA_LOOKUP_RESULT_SLOT4_IO_ADDRESS << 16 );

    /* CPU upstream Filters and Classification : thread 13 */
    local_register[ CPU_US_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R16 ] = 0x1880 << 16;
    local_register[ CPU_US_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R8  ] = CPU_TX_BBH_DESCRIPTORS_ADDRESS | ( 1 << 31 );
    local_register[ CPU_US_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R9  ] = ( ROUTER_INGRESS_QUEUE_ADDRESS << 16 ) |
                                                                                    INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ CPU_US_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R12 ] = RDD_WAN_ROUTER_PORT;
    local_register[ CPU_US_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R13 ] = RDD_CAM_RESULT_SLOT6 |
                                                                                  ( RDD_CAM_RESULT_SLOT6_IO_ADDRESS << 16 );
    local_register[ CPU_US_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER ][ CS_R15 ] = ( RDD_DMA_LOOKUP_RESULT_SLOT5 << 5 ) |
                                                                                    RDD_DMA_LOOKUP_4_STEPS |
                                                                                    ( RDD_DMA_LOOKUP_RESULT_SLOT5_IO_ADDRESS << 16 );

    /* Upstream VLAN: thread 15 */
    local_register[ UPSTREAM_VLAN_THREAD_NUMBER ][ CS_R16 ] = 0x2300 << 16;
    local_register[ UPSTREAM_VLAN_THREAD_NUMBER ][ CS_R10 ] = ( WAN_INTERWORKING_INGRESS_QUEUE_ADDRESS << 16) | INGRESS_HANDLER_BUFFER_BASE_ADDRESS;

    /* Router : thread 20 */
    local_register[ UPSTREAM_ROUTER_THREAD_NUMBER ][ CS_R16 ] = 0x2900 << 16;
    local_register[ UPSTREAM_ROUTER_THREAD_NUMBER ][ CS_R9  ] = ROUTER_INGRESS_QUEUE_ADDRESS << 16;
    local_register[ UPSTREAM_ROUTER_THREAD_NUMBER ][ CS_R10 ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;

    memcpyl_context ( sram_fast_context_ptr, local_register, sizeof ( RUNNER_CNTXT_MAIN ) );


    /********** Pico Runner A **********/

    sram_pico_context_ptr = ( RUNNER_CNTXT_PICO * )DEVICE_ADDRESS( RUNNER_CNTXT_PICO_0_OFFSET );

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32 ( local_register, sram_pico_context_ptr, sizeof ( RUNNER_CNTXT_PICO ) );

    /* CPU-TX: thread 33 */
    local_register[ CPU_TX_PICO_THREAD_NUMBER - 32 ][ CS_R16 ] = 0x300 << 16;
    local_register[ CPU_TX_PICO_THREAD_NUMBER - 32 ][ CS_R9  ] = CPU_TX_PICO_QUEUE_ADDRESS;

    /* Local switching LAN enqueue: thread 44 */
    local_register[ LOCAL_SWITCHING_LAN_ENQUEUE_THREAD_NUMBER - 32 ][ CS_R16 ] = ( 0x1000 << 16 ) | 0x1000;
    local_register[ LOCAL_SWITCHING_LAN_ENQUEUE_THREAD_NUMBER - 32 ][ CS_R9  ] = INGRESS_HANDLER_BUFFER_BASE_ADDRESS;
    local_register[ LOCAL_SWITCHING_LAN_ENQUEUE_THREAD_NUMBER - 32 ][ CS_R10 ] = RUNNER_A_LAN_ENQUEUE_INGRESS_QUEUE_ADDRESS;

    memcpyl_context ( sram_pico_context_ptr, local_register, sizeof ( RUNNER_CNTXT_PICO ) );


    /********** Pico Runner B **********/

    sram_pico_context_ptr = ( RUNNER_CNTXT_PICO * )DEVICE_ADDRESS( RUNNER_CNTXT_PICO_1_OFFSET );

    /* read the local registers from the Context memory - maybe it was initialized by the ACE compiler */
    MREAD_BLK_32 ( local_register, sram_pico_context_ptr, sizeof ( RUNNER_CNTXT_PICO ) );

    /* CPU-TX: thread 33 */
    local_register[ CPU_TX_PICO_THREAD_NUMBER - 32 ][ CS_R16 ] = 0x500 << 16;
    local_register[ CPU_TX_PICO_THREAD_NUMBER - 32 ][ CS_R8  ] = CPU_TX_PICO_QUEUE_ADDRESS;

	/* ETH0-TX: thread 40 */
    local_register[ ETH0_TX_THREAD_NUMBER - 32 ][ CS_R16 ] = 0x1500 << 16;
    local_register[ ETH0_TX_THREAD_NUMBER - 32 ][ CS_R9  ] = ( ETH_TX_MACS_DYNAMIC_TABLE_ADDRESS +
                                                               1 * sizeof (RDD_ETH_TX_MAC_DYNAMIC_DESCRIPTOR_DTS) ) << 16;
    local_register[ ETH0_TX_THREAD_NUMBER - 32 ][ CS_R11 ] = ( ( ETH_TX_QUEUES_POINTERS_TABLE_RUNNER_B_ADDRESS + 1 * RDD_EMAC_NUMBER_OF_QUEUES *
                                                                 sizeof (RDD_ETH_TX_QUEUE_POINTERS_ENTRY_DTS) ) << 16 ) | BBH_PERIPHERAL_ETH0_TX;

    /* ETH1-TX: thread 41 */
    local_register[ ETH1_TX_THREAD_NUMBER - 32 ][ CS_R16 ] = 0x1500 << 16;
    local_register[ ETH1_TX_THREAD_NUMBER - 32 ][ CS_R9  ] = ( ETH_TX_MACS_DYNAMIC_TABLE_ADDRESS +
                                                               2 * sizeof (RDD_ETH_TX_MAC_DYNAMIC_DESCRIPTOR_DTS) ) << 16;
    local_register[ ETH1_TX_THREAD_NUMBER - 32 ][ CS_R11 ] = ( ( ETH_TX_QUEUES_POINTERS_TABLE_RUNNER_B_ADDRESS + 2 * RDD_EMAC_NUMBER_OF_QUEUES *
                                                                 sizeof (RDD_ETH_TX_QUEUE_POINTERS_ENTRY_DTS) ) << 16) | BBH_PERIPHERAL_ETH1_TX;

    /* ETH2-TX: thread 42 */
    local_register[ ETH2_TX_THREAD_NUMBER - 32 ][ CS_R16 ] = 0x1500 << 16;
    local_register[ ETH2_TX_THREAD_NUMBER - 32 ][ CS_R9  ] = ( ETH_TX_MACS_DYNAMIC_TABLE_ADDRESS +
                                                               3 * sizeof (RDD_ETH_TX_MAC_DYNAMIC_DESCRIPTOR_DTS) ) << 16;
    local_register[ ETH2_TX_THREAD_NUMBER - 32 ][ CS_R11 ] = ( ( ETH_TX_QUEUES_POINTERS_TABLE_RUNNER_B_ADDRESS + 3 * RDD_EMAC_NUMBER_OF_QUEUES *
                                                                 sizeof (RDD_ETH_TX_QUEUE_POINTERS_ENTRY_DTS) ) << 16) | BBH_PERIPHERAL_ETH2_TX;

    /* ETH3-TX: thread 43 */
    local_register[ ETH3_TX_THREAD_NUMBER - 32 ][ CS_R16 ] = 0x1500 << 16;
    local_register[ ETH3_TX_THREAD_NUMBER - 32 ][ CS_R9  ] = ( ETH_TX_MACS_DYNAMIC_TABLE_ADDRESS +
                                                               4 * sizeof (RDD_ETH_TX_MAC_DYNAMIC_DESCRIPTOR_DTS) ) << 16;
    local_register[ ETH3_TX_THREAD_NUMBER - 32 ][ CS_R11 ] = ( ( ETH_TX_QUEUES_POINTERS_TABLE_RUNNER_B_ADDRESS + 4 * RDD_EMAC_NUMBER_OF_QUEUES *
                                                                 sizeof (RDD_ETH_TX_QUEUE_POINTERS_ENTRY_DTS) ) << 16) | BBH_PERIPHERAL_ETH3_TX;

    /* ETH4-TX: thread 44 */
    local_register[ ETH4_TX_THREAD_NUMBER - 32 ][ CS_R16 ] = 0x1500 << 16;
    local_register[ ETH4_TX_THREAD_NUMBER - 32 ][ CS_R9  ] = ( ETH_TX_MACS_DYNAMIC_TABLE_ADDRESS +
                                                               5 * sizeof (RDD_ETH_TX_MAC_DYNAMIC_DESCRIPTOR_DTS) ) << 16;
    local_register[ ETH4_TX_THREAD_NUMBER - 32 ][ CS_R11 ] = ( ( ETH_TX_QUEUES_POINTERS_TABLE_RUNNER_B_ADDRESS + 5 * RDD_EMAC_NUMBER_OF_QUEUES *
                                                                 sizeof (RDD_ETH_TX_QUEUE_POINTERS_ENTRY_DTS) ) << 16) | BBH_PERIPHERAL_ETH4_TX;

    memcpyl_context ( sram_pico_context_ptr, local_register, sizeof ( RUNNER_CNTXT_PICO ) );

    return ( RDD_OK );
}

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   f_rdd_eth_tx_initialize                                                  */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Runner Initialization - initialize the ethernet tx mechanism             */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   none                                                                     */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
static RDD_ERROR f_rdd_eth_tx_initialize ( void )
{
    RDD_ETH_TX_MAC_DYNAMIC_TABLE_DTS                 *eth_tx_mac_dynamic_table_ptr;
    RDD_ETH_TX_MAC_STATIC_TABLE_DTS                  *eth_tx_mac_static_table_ptr;
    RDD_ETH_TX_MAC_DYNAMIC_DESCRIPTOR_DTS            *eth_tx_mac_dynamic_descriptor_ptr;
    RDD_ETH_TX_MAC_STATIC_DESCRIPTOR_DTS             *eth_tx_mac_static_descriptor_ptr;
    RDD_ETH_TX_QUEUE_DYNAMIC_DESCRIPTOR_DTS          *eth_tx_queue_dynamic_descriptor_ptr;
    RDD_ETH_TX_QUEUE_STATIC_DESCRIPTOR_DTS           *eth_tx_queue_static_descriptor_ptr;
    RDD_ETH_TX_QUEUES_POINTERS_TABLE_DTS             *eth_tx_queues_d_pointers_runner_a_table_ptr;
    RDD_ETH_TX_QUEUES_POINTERS_TABLE_DTS             *eth_tx_queues_s_pointers_runner_a_table_ptr;
    RDD_ETH_TX_QUEUES_POINTERS_TABLE_DTS             *eth_tx_queues_pointers_runner_b_table_ptr;
    RDD_ETH_TX_QUEUE_POINTERS_ENTRY_DTS              *eth_tx_queue_pointers_entry_ptr;
    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_DTS  *free_packet_descriptors_pool_descriptor_ptr;
    RDD_PACKET_DESCRIPTOR_DTS                        *packet_descriptor_ptr;
    uint16_t                                             eth_tx_queue_address;
    uint16_t                                             mac_descriptor_address;
    uint16_t                                             packet_descriptor_address;
    uint16_t                                             next_packet_descriptor_address;
    uint16_t                                             packet_descriptors_counter;
    uint32_t                                             tx_queue;
    uint32_t                                             emac_id;
    uint16_t                                             queue_status_offset[ 8 ] = { 3, 7, 2, 6, 1, 5, 0, 4 };

    eth_tx_mac_dynamic_table_ptr = ( RDD_ETH_TX_MAC_DYNAMIC_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_1_OFFSET ) +
                                                                                ETH_TX_MACS_DYNAMIC_TABLE_ADDRESS - sizeof ( RUNNER_COMMON ) );

    eth_tx_mac_static_table_ptr = ( RDD_ETH_TX_MAC_STATIC_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) +
                                                                              ETH_TX_MACS_STATIC_TABLE_ADDRESS );

    eth_tx_queues_d_pointers_runner_a_table_ptr = ( RDD_ETH_TX_QUEUES_POINTERS_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) +
                                                                                                   ETH_TX_QUEUES_D_POINTERS_TABLE_RUNNER_A_ADDRESS );

    eth_tx_queues_s_pointers_runner_a_table_ptr = ( RDD_ETH_TX_QUEUES_POINTERS_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) +
                                                                                                   ETH_TX_QUEUES_S_POINTERS_TABLE_RUNNER_A_ADDRESS );

    eth_tx_queues_pointers_runner_b_table_ptr =
        ( RDD_ETH_TX_QUEUES_POINTERS_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_1_OFFSET ) +
                                                         ETH_TX_QUEUES_POINTERS_TABLE_RUNNER_B_ADDRESS - sizeof ( RUNNER_COMMON ) );

    free_packet_descriptors_pool_descriptor_ptr =
        ( RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) +
                                                                    FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_ADDRESS );

    packet_descriptors_counter = 0;

    for ( emac_id = RDD_EMAC_ID_0; emac_id <= RDD_EMAC_ID_4; emac_id++ )
    {
        eth_tx_mac_dynamic_descriptor_ptr = &( eth_tx_mac_dynamic_table_ptr->entry[ emac_id ] );
        eth_tx_mac_static_descriptor_ptr = &( eth_tx_mac_static_table_ptr->entry[ emac_id ] );

        RDD_ETH_TX_MAC_STATIC_TX_TASK_NUMBER_WRITE ( ( ETH0_TX_THREAD_NUMBER + ( emac_id - RDD_EMAC_ID_0 ) ),
                                                           eth_tx_mac_static_descriptor_ptr );
        RDD_ETH_TX_MAC_DYNAMIC_RATE_LIMITER_ID_WRITE ( RDD_RATE_LIMITER_IDLE, eth_tx_mac_dynamic_descriptor_ptr );
        RDD_ETH_TX_MAC_DYNAMIC_TX_TASK_NUMBER_WRITE ( ( ETH0_TX_THREAD_NUMBER + ( emac_id - RDD_EMAC_ID_0 ) ),
                                                            eth_tx_mac_dynamic_descriptor_ptr );

        for ( tx_queue = 0; tx_queue < RDD_EMAC_NUMBER_OF_QUEUES; tx_queue++ )
        {
            eth_tx_queue_address = ETH_TX_QUEUES_DYNAMIC_DESCRIPTORS_ADDRESS +
                                   ( ( emac_id - RDD_EMAC_ID_0 ) * RDD_EMAC_NUMBER_OF_QUEUES + tx_queue ) *
                                    sizeof ( RDD_ETH_TX_QUEUE_DYNAMIC_DESCRIPTOR_DTS );

            mac_descriptor_address = ETH_TX_MACS_DYNAMIC_TABLE_ADDRESS + emac_id * sizeof ( RDD_ETH_TX_MAC_DYNAMIC_DESCRIPTOR_DTS );

            eth_tx_queue_pointers_entry_ptr =
                &( eth_tx_queues_d_pointers_runner_a_table_ptr->entry[ emac_id * RDD_EMAC_NUMBER_OF_QUEUES + tx_queue ] );

            RDD_ETH_TX_QUEUE_POINTERS_MAC_PTR_WRITE ( mac_descriptor_address, eth_tx_queue_pointers_entry_ptr );
            RDD_ETH_TX_QUEUE_POINTERS_QUEUE_PTR_WRITE ( eth_tx_queue_address , eth_tx_queue_pointers_entry_ptr );

            eth_tx_queue_pointers_entry_ptr =
                &( eth_tx_queues_pointers_runner_b_table_ptr->entry[ emac_id * RDD_EMAC_NUMBER_OF_QUEUES + tx_queue ] );

            RDD_ETH_TX_QUEUE_POINTERS_MAC_PTR_WRITE ( mac_descriptor_address, eth_tx_queue_pointers_entry_ptr );
            RDD_ETH_TX_QUEUE_POINTERS_QUEUE_PTR_WRITE ( eth_tx_queue_address , eth_tx_queue_pointers_entry_ptr );

            RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_HEAD_PTR_READ ( packet_descriptor_address, free_packet_descriptors_pool_descriptor_ptr );

            packet_descriptor_ptr = ( RDD_PACKET_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) + packet_descriptor_address );

            RDD_PACKET_DESCRIPTOR_NEXT_PTR_READ ( next_packet_descriptor_address, packet_descriptor_ptr );
            RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_HEAD_PTR_WRITE ( next_packet_descriptor_address,
                                                                               free_packet_descriptors_pool_descriptor_ptr );

            packet_descriptors_counter++;

            eth_tx_queue_dynamic_descriptor_ptr =
                ( RDD_ETH_TX_QUEUE_DYNAMIC_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_1_OFFSET ) +
                                                                    eth_tx_queue_address - sizeof ( RUNNER_COMMON ) );

            RDD_ETH_TX_DYNAMIC_QUEUE_HEAD_PTR_WRITE ( packet_descriptor_address, eth_tx_queue_dynamic_descriptor_ptr );
            RDD_ETH_TX_DYNAMIC_QUEUE_TAIL_PTR_WRITE ( packet_descriptor_address, eth_tx_queue_dynamic_descriptor_ptr );
            RDD_ETH_TX_DYNAMIC_QUEUE_STATUS_OFFSET_WRITE ( queue_status_offset[ tx_queue ], eth_tx_queue_dynamic_descriptor_ptr );

            eth_tx_queue_address = ETH_TX_QUEUES_STATIC_DESCRIPTORS_ADDRESS +
                                   ( ( emac_id - RDD_EMAC_ID_0 ) * RDD_EMAC_NUMBER_OF_QUEUES + tx_queue ) *
                                    sizeof ( RDD_ETH_TX_QUEUE_STATIC_DESCRIPTOR_DTS );

            mac_descriptor_address = ETH_TX_MACS_STATIC_TABLE_ADDRESS + emac_id * sizeof ( RDD_ETH_TX_MAC_STATIC_DESCRIPTOR_DTS );

            eth_tx_queue_pointers_entry_ptr =
                &( eth_tx_queues_s_pointers_runner_a_table_ptr->entry[ emac_id * RDD_EMAC_NUMBER_OF_QUEUES + tx_queue ] );


            RDD_ETH_TX_QUEUE_POINTERS_MAC_PTR_WRITE ( mac_descriptor_address, eth_tx_queue_pointers_entry_ptr );
            RDD_ETH_TX_QUEUE_POINTERS_QUEUE_PTR_WRITE ( eth_tx_queue_address , eth_tx_queue_pointers_entry_ptr );

            eth_tx_queue_static_descriptor_ptr =
                ( RDD_ETH_TX_QUEUE_STATIC_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) + eth_tx_queue_address );

            RDD_ETH_TX_STATIC_QUEUE_STATUS_OFFSET_WRITE ( queue_status_offset[ tx_queue ], eth_tx_queue_static_descriptor_ptr );
        }
    }

    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_EGRESS_WRITE ( packet_descriptors_counter, free_packet_descriptors_pool_descriptor_ptr );


    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   f_rdd_local_switching_initialize                                         */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Runner Initialization - initialize the local switching mechanism         */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   none                                                                     */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
static RDD_ERROR f_rdd_local_switching_initialize ( void )
{
    uint16_t  *lan_enqueue_ingress_queue_ptr;

    lan_enqueue_ingress_queue_ptr = ( uint16_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) +
                                                     LAN_ENQUEUE_INGRESS_QUEUE_PTR_ADDRESS );

    MWRITE_16 ( lan_enqueue_ingress_queue_ptr, RUNNER_A_LAN_ENQUEUE_INGRESS_QUEUE_ADDRESS );

    return ( RDD_OK );
}


static int32_t f_rdd_bridge_port_to_port_index ( RDD_BRIDGE_PORT  xi_bridge_port,
                                                   uint32_t    xi_subnet_id )
{
    if ( ( xi_bridge_port >= RDD_LAN0_BRIDGE_PORT ) && ( xi_bridge_port <= RDD_LAN4_BRIDGE_PORT ) )
    {
        if ( xi_subnet_id == 0 )
        {
            return ( xi_bridge_port );
        }
    }
    else if ( xi_bridge_port == RDD_WAN_BRIDGE_PORT )
    {
        if ( xi_subnet_id == 0 )
        {
            return ( RDD_SUBNET_BRIDGE );
        }
    }
    else if ( xi_bridge_port == RDD_WAN_ROUTER_PORT )
    {
        if ( xi_subnet_id < 4 )
        {
            return ( xi_subnet_id );
        }
    }

    return ( -1 );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_eth_tx_queue_config                                         */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - configure downstream Ethernet TX queue behind an EMAC. */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   configures EMAC TX queue:                                                */
/*   - the packet threshold indicates how many packets can be TX pending      */
/*     behind a TX queue.                                                     */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_emac_id - EMAC port index (ETH0 - ETH4)                               */
/*   xi_queue_id - ETH TX queue index                                         */
/*   xi_packet_threshold - ETH TX queue packet threshold                      */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*     RDD_ERROR_ILLEGAL_EMAC_ID - EMAC index is illegal.            */
/*     RDD_ERROR_ILLEGAL_QUEUE_ID - queue index is greater then 7    */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_eth_tx_queue_config ( RDD_EMAC_ID   xi_emac_id,
                                                          RDD_QUEUE_ID  xi_queue_id,
                                                          uint16_t                 xi_packet_threshold )
{
    RDD_ETH_TX_QUEUE_STATIC_DESCRIPTOR_DTS   *eth_tx_queue_static_descriptor_ptr;
    RDD_ETH_TX_QUEUES_POINTERS_TABLE_DTS     *eth_tx_queues_pointers_table_ptr;
    RDD_ETH_TX_QUEUE_POINTERS_ENTRY_DTS      *eth_tx_queue_pointers_entry_ptr;
    uint16_t                                     eth_tx_queue_descriptor_offset;

    /* check the validity of the input parameters - emac id */
    if ( ( xi_emac_id < RDD_EMAC_ID_0 ) || ( xi_emac_id > RDD_EMAC_ID_4 ) )
    {
        return ( RDD_ERROR_ILLEGAL_EMAC_ID );
    }

    /* check the validity of the input parameters - emac tx queue id */
    if ( xi_queue_id > RDD_QUEUE_0 )
    {
        return ( RDD_ERROR_ILLEGAL_QUEUE_ID );
    }

    eth_tx_queues_pointers_table_ptr =
        ( RDD_ETH_TX_QUEUES_POINTERS_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) +
                                                         ETH_TX_QUEUES_S_POINTERS_TABLE_RUNNER_A_ADDRESS );

    eth_tx_queue_pointers_entry_ptr = &( eth_tx_queues_pointers_table_ptr->entry[ xi_emac_id * RDD_EMAC_NUMBER_OF_QUEUES + xi_queue_id ] );

    RDD_ETH_TX_QUEUE_POINTERS_QUEUE_PTR_READ ( eth_tx_queue_descriptor_offset, eth_tx_queue_pointers_entry_ptr );


    eth_tx_queue_static_descriptor_ptr = ( RDD_ETH_TX_QUEUE_STATIC_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) +
                                                                                            eth_tx_queue_descriptor_offset );

    /* initialize the configured parameters of the eth tx descriptor */
    RDD_ETH_TX_STATIC_QUEUE_PACKET_THRSHLD_WRITE ( xi_packet_threshold, eth_tx_queue_static_descriptor_ptr );

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_get_mdu_mode_pointer                                        */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - return an EMAC egress counter pointer.                 */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   return an EMAC egress counter pointer to the BBH driver in order to      */
/*   how many packets were taken by the EMAC for transmission, the firmware   */
/*   is responsible to maintain the ingress counter, the difference between   */
/*   the two counter cannot exceed 8 packets. Both counters are 6 bits wrap   */
/*   around.                                                                  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_emac_id - EMAC port index (ETH0 - ETH4)                               */
/*   xo_mdu_mode_pointer - pointer to the egress counter of that EMAC         */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*     RDD_ERROR_ILLEGAL_EMAC_ID - EMAC index is illegal.            */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_get_mdu_mode_pointer ( RDD_EMAC_ID  xi_emac_id,
                                                           uint16_t                *xo_mdu_mode_pointer )
{
    /* check the validity of the input parameters - emac id */
    if ( ( xi_emac_id < RDD_EMAC_ID_0 ) || ( xi_emac_id > RDD_EMAC_ID_4 ) )
    {
        return ( RDD_ERROR_ILLEGAL_EMAC_ID );
    }

    *xo_mdu_mode_pointer = ETH_TX_MACS_DYNAMIC_TABLE_ADDRESS + xi_emac_id * sizeof ( RDD_ETH_TX_MAC_DYNAMIC_DESCRIPTOR_DTS ) +
                           RDD_EMAC_EGRESS_COUNTER_OFFSET;

    return ( RDD_OK ) ;
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_cpu_rx_queue_config                                         */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - CPU interface.                                         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   configures CPU-RX queue, set the number packets that are pending to be   */
/*   read by the CPU for that queue, and the interrupt that will be set       */
/*   during packet enterance to the queue, the default interrupt is the queue */
/*   number.                                                                  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_queue_id - CPU-RX queue index (0-7)                                   */
/*   xi_queue_size - queue maximum size in packets.                           */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*     RDD_ERROR_CPU_RX_QUEUE_ILLEGAL - CPU-RX queue is illegal.     */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_cpu_rx_queue_config ( RDD_CPU_RX_QUEUE  xi_queue_id,
                                                          uint16_t                     xi_queue_size,
                                                          uint16_t                     xi_interrupt_id )
{
    RDD_CPU_RX_TABLE_DTS             *cpu_rx_table_ptr;
    RDD_CPU_RX_QUEUE_DESCRIPTOR_DTS  *cpu_rx_queue_descriptor_ptr;
    uint32_t                             int_lock;

    /* check the validity of the input parameters - CPU-RX queue index */
    if ( xi_queue_id >= CPU_RX_NUMBER_OF_QUEUES )
    {
        return ( RDD_ERROR_CPU_RX_QUEUE_ILLEGAL );
    }

    f_rdd_lock ( &int_lock );


    cpu_rx_table_ptr = ( RDD_CPU_RX_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) + CPU_RX_QUEUES_TABLE_ADDRESS );

    cpu_rx_queue_descriptor_ptr = &( cpu_rx_table_ptr->entry[ xi_queue_id ] );

    RDD_CPU_RX_QUEUE_MAX_SIZE_WRITE ( xi_queue_size, cpu_rx_queue_descriptor_ptr );

    RDD_CPU_RX_QUEUE_INTERRUPT_WRITE ( xi_interrupt_id + RDD_INTERRUPT_IO_ADDRESS, cpu_rx_queue_descriptor_ptr );

    f_rdd_unlock ( int_lock );
    return ( RDD_OK );
}

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_cpu_reason_to_cpu_rx_queue                                  */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - CPU interface.                                         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   cpu trap reason to cpu rx queue conversion.                              */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_cpu_reason - the reason for sending the packet to the CPU             */
/*   xi_queue_id - CPU-RX queue index (0-7)                                   */
/*   xi_direction - upstream or downstream                                    */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*     RDD_ERROR_CPU_RX_QUEUE_ILLEGAL - CPU-RX queue is illegal.     */
/*     RDD_ERROR_CPU_RX_REASON_ILLEGAL - CPU-RX reason is illegal.   */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_cpu_reason_to_cpu_rx_queue ( RDD_CPU_RX_REASON  xi_cpu_reason,
                                                                 RDD_CPU_RX_QUEUE   xi_queue_id,
                                                                 RDD_DIRECTION      xi_direction )
{
    RDD_CPU_REASON_TO_CPU_RX_QUEUE_TABLE_DTS  *cpu_reason_to_cpu_rx_queue_table_ptr;
    RDD_CPU_REASON_TO_CPU_RX_QUEUE_ENTRY_DTS  *cpu_reason_to_cpu_rx_queue_entry_ptr;
    uint8_t                                       cpu_queue;

    /* check the validity of the input parameters - CPU-RX reason */
    if ( xi_cpu_reason >= RDD_CPU_RX_NUMBER_OF_REASONS )
    {
        return ( RDD_ERROR_CPU_RX_REASON_ILLEGAL );
    }

    /* check the validity of the input parameters - CPU-RX queue-id */
    if ( xi_queue_id >= CPU_RX_NUMBER_OF_QUEUES )
    {
        return ( RDD_ERROR_CPU_RX_QUEUE_ILLEGAL );
    }

    if ( xi_direction == RDD_DOWNSTREAM )
    {
        cpu_reason_to_cpu_rx_queue_table_ptr =
            ( RDD_CPU_REASON_TO_CPU_RX_QUEUE_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) +
                                                                 CPU_RX_REASON_TO_QUEUE_TABLE_ADDRESS );
    }
    else
    {
        cpu_reason_to_cpu_rx_queue_table_ptr =
            ( RDD_CPU_REASON_TO_CPU_RX_QUEUE_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) +
                                                                 CPU_RX_REASON_TO_QUEUE_TABLE_ADDRESS );
    }

    cpu_reason_to_cpu_rx_queue_entry_ptr = &( cpu_reason_to_cpu_rx_queue_table_ptr->entry[ xi_cpu_reason ] );

    cpu_queue = xi_queue_id;

    RDD_CPU_REASON_TO_QUEUE_ENTRY_WRITE ( cpu_queue, cpu_reason_to_cpu_rx_queue_entry_ptr );

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_cpu_rx_read_packet                                          */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - CPU interface.                                         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   check if queue is empty, if there is packet pending copy it's data to    */
/*   the input buffer pointer, free the DDR buffer to the BPM and return      */
/*   associated information like source port, packet length and reason - why  */
/*   the packet was trapped to the CPU.                                       */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_queue_id - CPU-RX queue number (0-7)                                  */
/*   xi_packet_ptr - pointer to a DDR buffer to fill in the packet data       */
/*   xo_packet_size -  the size of the packet                                 */
/*   xo_src_bridge_port - the source bridge port of the packet                */
/*   xo_flow_id - the ingress flow id of the packet (GPON)                    */
/*   xo_reason - the reason that the packet was trapped to the CPU            */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*     RDD_ERROR_CPU_RX_QUEUE_ILLEGAL - illegal CPU-RX queue id.     */
/*     RDD_ERROR_CPU_RX_QUEUE_EMPTY - the CPU RX has no pending      */
/*                                             packets.                       */
/*     RDD_ERROR_BPM_FREE_FAIL - unable to free the DDR buffer to    */
/*                                        the BPM.                            */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_cpu_rx_read_packet ( uint32_t                      xi_queue_id,
                                                         uint8_t                       *xi_packet_ptr,
                                                         uint32_t                      *xo_packet_size,
                                                         RDD_BRIDGE_PORT    *xo_src_bridge_port,
                                                         uint32_t                      *xo_flow_id,
                                                         RDD_CPU_RX_REASON  *xo_reason )
{
    RDD_CPU_RX_TABLE_DTS                             *cpu_rx_table_ptr;
    RDD_CPU_RX_QUEUE_DESCRIPTOR_DTS                  *cpu_rx_queue_descriptor_ptr;
    RDD_CPU_RX_DESCRIPTOR_DTS                        *cpu_rx_dummy_descriptor_ptr;
    RDD_CPU_RX_DESCRIPTOR_DTS                        *cpu_rx_descriptor_ptr;
    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_DTS  *cpu_free_packet_descriptors_pool_descriptor_ptr;
    RDD_PACKET_DESCRIPTOR_DTS                        *pool_last_packet_descriptor_ptr;
    RDD_BRIDGE_PORT                           source_bridge_port;
    uint16_t                                             cpu_rx_queue_head_ptr;
    uint16_t                                             cpu_rx_descriptor_packet_length;
    uint16_t                                             cpu_rx_descriptor_next_ptr;
    uint16_t                                             cpu_rx_descriptor_buffer_number;
    uint32_t                                             packet_ddr_ptr;
    uint16_t                                             cpu_free_packet_descriptor_tail_ptr;
    uint16_t                                             ingress_packet_descriptors_counter;
    uint16_t                                             cpu_rx_queue_egress_packet_counter;
    uint32_t                                             int_lock;

    /* check the validity of the input parameters - CPU-RX queue-id */
    if ( xi_queue_id >= CPU_RX_NUMBER_OF_QUEUES )
    {
        return ( RDD_ERROR_CPU_RX_QUEUE_ILLEGAL );
    }

    f_rdd_lock ( &int_lock );


    /* read CPU-RX table entry according to the input queue-id */
    cpu_rx_table_ptr = ( RDD_CPU_RX_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) + CPU_RX_QUEUES_TABLE_ADDRESS );

    cpu_rx_queue_descriptor_ptr = &( cpu_rx_table_ptr->entry[ xi_queue_id ] );

    RDD_CPU_RX_QUEUE_HEAD_POINTER_READ ( cpu_rx_queue_head_ptr, cpu_rx_queue_descriptor_ptr );

    cpu_rx_dummy_descriptor_ptr = ( RDD_CPU_RX_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_1_OFFSET ) +
                                                                        cpu_rx_queue_head_ptr - sizeof ( RUNNER_COMMON ) );

    RDD_CPU_RX_DESCRIPTOR_NEXT_PTR_READ ( cpu_rx_descriptor_next_ptr, cpu_rx_dummy_descriptor_ptr );

    if ( cpu_rx_descriptor_next_ptr == 0 )
    {
        f_rdd_unlock ( int_lock );
        return ( RDD_ERROR_CPU_RX_QUEUE_EMPTY );
    }

    cpu_rx_descriptor_next_ptr = cpu_rx_descriptor_next_ptr << 3;

    cpu_rx_descriptor_ptr = ( RDD_CPU_RX_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_1_OFFSET ) +
                                                                  cpu_rx_descriptor_next_ptr - sizeof ( RUNNER_COMMON ) );

    RDD_CPU_RX_DESCRIPTOR_BUFFER_NUMBER_READ ( cpu_rx_descriptor_buffer_number, cpu_rx_descriptor_ptr );

    packet_ddr_ptr = g_runner_ddr_base_addr + cpu_rx_descriptor_buffer_number * RDD_RUNNER_PACKET_BUFFER_SIZE + RDD_PACKET_DDR_OFFSET;

    RDD_CPU_RX_DESCRIPTOR_PACKET_LENGTH_READ ( cpu_rx_descriptor_packet_length, cpu_rx_descriptor_ptr );

    /* copy the packet into the supplied DDR buffer */
    MREAD_BLK_8 ( xi_packet_ptr, (uint8_t *)packet_ddr_ptr, cpu_rx_descriptor_packet_length );

    /* set as function output the packet length, ingress flow id and the cpu reason */
    *xo_packet_size = cpu_rx_descriptor_packet_length;
    RDD_CPU_RX_DESCRIPTOR_REASON_READ ( *xo_reason, cpu_rx_descriptor_ptr );
    RDD_CPU_RX_DESCRIPTOR_SRC_PORT_READ ( *xo_src_bridge_port, cpu_rx_descriptor_ptr );
    RDD_CPU_RX_DESCRIPTOR_FLOW_ID_READ ( *xo_flow_id, cpu_rx_descriptor_ptr );

#if !defined(FIRMWARE_INIT)
    RDD_CPU_RX_DESCRIPTOR_SRC_PORT_READ ( source_bridge_port, cpu_rx_descriptor_ptr );

    if ( fi_bl_drv_bpm_free_buffer ( source_bridge_port, cpu_rx_descriptor_buffer_number ) != DRV_BPM_ERROR_NO_ERROR )
    {
        f_rdd_unlock ( int_lock );
        return ( RDD_ERROR_BPM_FREE_FAIL );
    }
#endif

    RDD_CPU_RX_QUEUE_HEAD_POINTER_WRITE ( cpu_rx_descriptor_next_ptr, cpu_rx_queue_descriptor_ptr );

    MWRITE_32( ( (uint8_t *)cpu_rx_dummy_descriptor_ptr + 0 ), 0 );
    MWRITE_32( ( (uint8_t *)cpu_rx_dummy_descriptor_ptr + 4 ), 0 );

    cpu_free_packet_descriptors_pool_descriptor_ptr =
        ( RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_0_OFFSET ) +
                                                                    CPU_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_ADDRESS );

    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_TAIL_PTR_READ ( cpu_free_packet_descriptor_tail_ptr,
                                                                      cpu_free_packet_descriptors_pool_descriptor_ptr );

    pool_last_packet_descriptor_ptr = ( RDD_PACKET_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_COMMON_1_OFFSET ) +
                                                                            cpu_free_packet_descriptor_tail_ptr - sizeof ( RUNNER_COMMON ) );

    RDD_PACKET_DESCRIPTOR_NEXT_PTR_WRITE ( cpu_rx_queue_head_ptr, pool_last_packet_descriptor_ptr );

    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_TAIL_PTR_WRITE ( cpu_rx_queue_head_ptr, cpu_free_packet_descriptors_pool_descriptor_ptr );

    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_INGRESS_READ ( ingress_packet_descriptors_counter,
                                                                     cpu_free_packet_descriptors_pool_descriptor_ptr );

    ingress_packet_descriptors_counter++;
    ingress_packet_descriptors_counter = ingress_packet_descriptors_counter % RDD_CPU_FREE_PACKET_DESCRIPTORS_POOL_SIZE;

    RDD_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_INGRESS_WRITE ( ingress_packet_descriptors_counter,
                                                                      cpu_free_packet_descriptors_pool_descriptor_ptr );

    RDD_CPU_RX_QUEUE_EGRESS_COUNTER_READ ( cpu_rx_queue_egress_packet_counter, cpu_rx_queue_descriptor_ptr );

    cpu_rx_queue_egress_packet_counter++;
    cpu_rx_queue_egress_packet_counter = cpu_rx_queue_egress_packet_counter % RDD_CPU_FREE_PACKET_DESCRIPTORS_POOL_SIZE;

    RDD_CPU_RX_QUEUE_EGRESS_COUNTER_WRITE ( cpu_rx_queue_egress_packet_counter, cpu_rx_queue_descriptor_ptr );

    f_rdd_unlock ( int_lock );
    return ( RDD_OK );
}
EXPORT_SYMBOL(bl_rdd_cpu_rx_read_packet);

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_cpu_tx_write_eth_packet                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - CPU interface.                                         */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This packet is directed to the EMAC TX queue.                            */
/*   check if the CPU TX queue is not full, if not then a DDR buffer is       */
/*   allocated from the BPM, then the packet data is copied to the allocated  */
/*   buffer and a new packet descriptor is written to the CPU TX queue.       */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_packet_ptr - pointer to a DDR buffer that hold the packet data        */
/*   xi_packet_size - packet size in bytes                                    */
/*   xi_emac_id - EMAC port index (ETH0 - ETH4)                               */
/*   xi_queue_id - ETH TX queue index                                         */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*     RDD_ERROR_CPU_TX_NOT_ALLOWED - the runner is not enabled.     */
/*     RDD_ERROR_CPU_TX_QUEUE_FULL - the CPU TX has no place for new */
/*                                            packets.                        */
/*     RDD_ERROR_BPM_ALLOC_FAIL - unable to allocate a DDR buffer to */
/*                                         from the BPM.                      */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_cpu_tx_write_eth_packet ( uint8_t                  *xi_packet_ptr,
                                                              uint32_t                 xi_packet_size,
                                                              RDD_EMAC_ID   xi_emac_id,
                                                              RDD_QUEUE_ID  xi_queue_id )
{
    RUNNER_REGS_CFG_CPU_WAKEUP   runner_cpu_wakeup_register;
    RDD_CPU_TX_DESCRIPTOR_DTS  *cpu_tx_descriptor_ptr;
    uint32_t                       packet_ddr_ptr;
    uint32_t                       bpm_buffer_number;
    uint8_t                        cpu_tx_descriptor_valid;
    uint32_t                       int_lock;

    if ( g_runner_enable == RDD_FALSE )
    {
        return ( RDD_ERROR_CPU_TX_NOT_ALLOWED );
    }

    f_rdd_lock ( &int_lock );

    cpu_tx_descriptor_ptr = ( RDD_CPU_TX_DESCRIPTOR_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) +
                                                                  g_cpu_tx_queue_write_ptr[ 2 ] );

    /* if the descriptor is valid then the CPU-TX queue is full and the packet should be dropped */
    RDD_CPU_TX_DESCRIPTOR_VALID_READ ( cpu_tx_descriptor_valid, cpu_tx_descriptor_ptr );

    if ( cpu_tx_descriptor_valid )
    {
        f_rdd_unlock ( int_lock );
        return ( RDD_ERROR_CPU_TX_QUEUE_FULL );
    }

#if !defined(FIRMWARE_INIT)
    if ( fi_bl_drv_bpm_req_buffer ( DRV_BPM_SP_MIPS_C, &bpm_buffer_number ) != DRV_BPM_ERROR_NO_ERROR )
    {
        f_rdd_unlock ( int_lock );
        return ( RDD_ERROR_BPM_ALLOC_FAIL );
    }
#endif

    packet_ddr_ptr = g_runner_ddr_base_addr + bpm_buffer_number * RDD_RUNNER_PACKET_BUFFER_SIZE + RDD_PACKET_DDR_OFFSET;

    /* copy the packet from the supplied DDR buffer */
    MWRITE_BLK_8 ( (uint8_t *)packet_ddr_ptr, xi_packet_ptr, xi_packet_size );

    /* write CPU-TX descriptor and validate it */
    RDD_CPU_TX_DESCRIPTOR_COMMAND_WRITE ( RDD_CPU_TX_COMMAND_ETH_PACKET, cpu_tx_descriptor_ptr );
    RDD_CPU_TX_DESCRIPTOR_EMAC_WRITE ( xi_emac_id, cpu_tx_descriptor_ptr );
    RDD_CPU_TX_DESCRIPTOR_QUEUE_WRITE ( xi_queue_id, cpu_tx_descriptor_ptr );
    RDD_CPU_TX_DESCRIPTOR_SRC_BRIDGE_PORT_WRITE ( MIPS_C_SRC_PORT, cpu_tx_descriptor_ptr );
    RDD_CPU_TX_DESCRIPTOR_PACKET_LENGTH_WRITE ( xi_packet_size, cpu_tx_descriptor_ptr );

    RDD_CPU_TX_DESCRIPTOR_PAYLOAD_OFFSET_WRITE ( RDD_PACKET_DDR_OFFSET / 2, cpu_tx_descriptor_ptr );
    RDD_CPU_TX_DESCRIPTOR_BUFFER_NUMBER_WRITE ( bpm_buffer_number, cpu_tx_descriptor_ptr );
    RDD_CPU_TX_DESCRIPTOR_VALID_WRITE ( RDD_TRUE, cpu_tx_descriptor_ptr );

    /* increment and wrap around if needed the write pointer of the CPU-TX queue */
    g_cpu_tx_queue_write_ptr[ 2 ] += RDD_CPU_TX_DESCRIPTOR_SIZE;
    g_cpu_tx_queue_write_ptr[ 2 ] &= RDD_CPU_TX_QUEUE_SIZE_MASK;

    /* send asynchronous wakeup command to the CPU-TX thread in the Runner */
    runner_cpu_wakeup_register.req_trgt = CPU_TX_PICO_THREAD_NUMBER / 32;
    runner_cpu_wakeup_register.thread_num = CPU_TX_PICO_THREAD_NUMBER % 32;
    runner_cpu_wakeup_register.urgent_req = RDD_FALSE;

    RUNNER_REGS_0_CFG_CPU_WAKEUP_WRITE ( runner_cpu_wakeup_register );

    f_rdd_unlock ( int_lock );
    return ( RDD_OK );
}
EXPORT_SYMBOL(bl_rdd_cpu_tx_write_eth_packet);

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_sa_mac_lookup_config                                        */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - bridge interface.                                      */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   configures whether SA MAC lookup is required for that port or not.       */
/*                                                                            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_bridge_port - bridge port index.                                      */
/*   xi_src_mac_lookup_mode - enables/disables the SA MAC lookup on this port */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*     RDD_ERROR_ILLEGAL_BRIDGE_PORT_ID - illegal bridge port.       */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_sa_mac_lookup_config ( RDD_BRIDGE_PORT    xi_bridge_port,
                                                           RDD_SA_MAC_LOOKUP  xi_src_mac_lookup_mode )
{
    RDD_FILTERS_CONFIGURATION_TABLE_DTS  *filters_cfg_table_ptr;
    RDD_FILTERS_CONFIGURATION_ENTRY_DTS  *filters_cfg_entry_ptr;
    int32_t                                  bridge_port_index;
    uint16_t                                 filters_cfg_entry;

    if ( xi_bridge_port == RDD_WAN_BRIDGE_PORT )
    {
        filters_cfg_table_ptr = ( RDD_FILTERS_CONFIGURATION_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) +
                                                                                INGRESS_FILTERS_CONFIGURATION_TABLE_ADDRESS );
    }
    else if ( ( xi_bridge_port >= RDD_LAN0_BRIDGE_PORT ) && ( xi_bridge_port <= RDD_LAN4_BRIDGE_PORT ) )
    {
        filters_cfg_table_ptr = ( RDD_FILTERS_CONFIGURATION_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) +
                                                                                INGRESS_FILTERS_CONFIGURATION_TABLE_ADDRESS );
    }
    else
    {
        return ( RDD_ERROR_ILLEGAL_BRIDGE_PORT_ID );
    }

    if ( ( bridge_port_index = f_rdd_bridge_port_to_port_index ( xi_bridge_port, 0 ) ) < 0 )
    {
        return ( RDD_ERROR_ILLEGAL_BRIDGE_PORT_ID );
    }

    filters_cfg_entry_ptr = &( filters_cfg_table_ptr->entry[ bridge_port_index ] );

    RDD_FILTERS_CFG_ENTRY_READ ( filters_cfg_entry, filters_cfg_entry_ptr );

    if ( xi_src_mac_lookup_mode == RDD_SA_MAC_LOOKUP_ENABLE )
    {
        filters_cfg_entry |= ( RDD_SA_MAC_LOOKUP_ENABLE << 8 );
    }
    else
    {
        filters_cfg_entry &= ~( RDD_SA_MAC_LOOKUP_ENABLE << 8 );
    }

    RDD_FILTERS_CFG_ENTRY_WRITE ( filters_cfg_entry, filters_cfg_entry_ptr );

    if ( ( xi_bridge_port >= RDD_LAN0_BRIDGE_PORT ) && ( xi_bridge_port <= RDD_LAN4_BRIDGE_PORT ) ) 
    {
        filters_cfg_table_ptr = ( RDD_FILTERS_CONFIGURATION_TABLE_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) +
                                                                                LOCAL_SWITCH_INGRESS_FILTERS_CONFIGURATION_TABLE_ADDRESS );

        filters_cfg_entry_ptr = &( filters_cfg_table_ptr->entry[ bridge_port_index ] );

        RDD_FILTERS_CFG_ENTRY_WRITE ( filters_cfg_entry, filters_cfg_entry_ptr );
    }

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_unknown_sa_mac_cmd_config                                   */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - bridge interface.                                      */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   configures SA MAC lookup failure action (forward, CPU trap, or drop      */
/*   packet.                                                                  */
/*                                                                            */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xi_bridge_port - bridge port index.                                      */
/*   xi_slf_cmd - Forward / Drop / CPU Trap                                   */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_unknown_sa_mac_cmd_config ( RDD_BRIDGE_PORT          xi_bridge_port,
                                                                RDD_UNKNOWN_MAC_COMMAND  xi_slf_cmd )
{
    RDD_BRIDGE_CONFIGURATION_REGISTER_DTS  *bridge_cfg_register;

    if ( xi_bridge_port == RDD_WAN_BRIDGE_PORT )
    {
        bridge_cfg_register = ( RDD_BRIDGE_CONFIGURATION_REGISTER_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) +
                                                                                BRIDGE_CONFIGURATION_REGISTER_ADDRESS );
    }
    else
    {
        bridge_cfg_register = ( RDD_BRIDGE_CONFIGURATION_REGISTER_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_1_OFFSET ) +
                                                                                BRIDGE_CONFIGURATION_REGISTER_ADDRESS );
    }

    RDD_BCR_UNKNOWN_SA_COMMAND_WRITE ( xi_slf_cmd, bridge_cfg_register, xi_bridge_port );

    /* local switching support */
    if ( xi_bridge_port != RDD_WAN_BRIDGE_PORT )
    {
        bridge_cfg_register = ( RDD_BRIDGE_CONFIGURATION_REGISTER_DTS * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) +
                                                                                BRIDGE_CONFIGURATION_REGISTER_ADDRESS );

        RDD_BCR_UNKNOWN_SA_COMMAND_WRITE ( xi_slf_cmd, bridge_cfg_register, xi_bridge_port );
    }

    return ( RDD_OK );
}

/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_get_rdd_version                                             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - software information                                   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function returns the RDD version in the form of                     */
/*   release.version.patch.revision                                           */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xo_version - Runner Device Driver code version                           */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_get_rdd_version ( uint32_t * const xo_version )
{
    /* return RDD version */
    *xo_version = *( uint32_t * )&gs_rdd_version ;

    return ( RDD_OK ) ;
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_get_microcode_version                                       */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - firmware information                                   */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   This function returns the firmware version in the form of                */
/*   release.version.patch.revision                                           */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   xo_microcode_version - firmware code version                             */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_get_microcode_version ( uint32_t  *xo_microcode_version )
{
    /* read the firmware version from the sram data */
    *xo_microcode_version = *( uint32_t * )(DEVICE_ADDRESS( RUNNER_PRIVATE_0_OFFSET ) + MICROCODE_VERSION_ADDRESS );

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   bl_rdd_critical_section_config                                     */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   Firmware Driver - RDD lock mechanism                                     */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/* This function initializes pointers to critical section callback functions  */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*     xi_lock_function - pointer to critical section begin function          */
/*     xi_unlock_function - pointer to critical section exit function         */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   RDD_ERROR - Return status                                   */
/*     RDD_OK - No error                                             */
/*                                                                            */
/******************************************************************************/
RDD_ERROR bl_rdd_critical_section_config ( RDD_LOCK_CRITICAL_SECTION_FP    xi_lock_function,
                                                              RDD_UNLOCK_CRITICAL_SECTION_FP  xi_unlock_function )
{
    f_rdd_lock = xi_lock_function;
    f_rdd_unlock = xi_unlock_function;

    return ( RDD_OK );
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   f_dummy_lock                                                             */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   lock critical section ( dummy function )                                 */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   used to setup default callback                                           */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*   none.                                                                    */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   none                                                                     */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
static void f_dummy_lock ( uint32_t *xo_int_lock )
{
    return;
}


/******************************************************************************/
/*                                                                            */
/* Name:                                                                      */
/*                                                                            */
/*   f_dummy_unlock                                                           */
/*                                                                            */
/* Title:                                                                     */
/*                                                                            */
/*   unlock critical section ( dummy function )                               */
/*                                                                            */
/* Abstract:                                                                  */
/*                                                                            */
/*   used to setup default callback                                           */
/*                                                                            */
/* Registers :                                                                */
/*                                                                            */
/*   none.                                                                    */
/*                                                                            */
/* Input:                                                                     */
/*                                                                            */
/*   none                                                                     */
/*                                                                            */
/* Output:                                                                    */
/*                                                                            */
/*   none                                                                     */
/*                                      .                                     */
/*                                                                            */
/******************************************************************************/
static void f_dummy_unlock ( uint32_t xi_int_lock )
{
    return;
}

