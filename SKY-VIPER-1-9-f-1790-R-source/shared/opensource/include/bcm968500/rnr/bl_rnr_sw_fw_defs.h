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

#ifndef _BL_LILAC_RNR_DEFS_H
#define _BL_LILAC_RNR_DEFS_H

#define DOWNSTREAM_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS             0x04000
#define UPSTREAM_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS               0x02000
#define FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_ADDRESS             0x03B30
#define FREE_PACKET_DESCRIPTORS_POOL_THRESHOLD_ADDRESS              0x0B772
#define BPM_DDR_BUFFERS_BASE_ADDRESS                                0x0BCB4
#define BPM_EXTRA_DDR_BUFFERS_BASE_ADDRESS                          0x0B760
#define BPM_REPLY_ADDRESS                                           0x0AA04
#define SBPM_REPLY_ADDRESS                                          0x08980

#define GPON_RX_DIRECT_DESCRIPTORS_ADDRESS                          0x0B400
#define GPON_RX_NORMAL_DESCRIPTORS_ADDRESS                          0x0B800
#define ETH0_RX_DESCRIPTORS_ADDRESS                                 0x0B000
#define ETH1_RX_DESCRIPTORS_ADDRESS                                 0x0B200
#define ETH2_RX_DESCRIPTORS_ADDRESS                                 0x0B800
#define ETH3_RX_DESCRIPTORS_ADDRESS                                 0x0BA00
#define ETH4_RX_DESCRIPTORS_ADDRESS                                 0x0BE00
#define ETH0_RX_DIRECT_DESCRIPTORS_ADDRESS                          0x09800
#define ETH1_RX_DIRECT_DESCRIPTORS_ADDRESS                          0x0AC00
#define ETH2_RX_DIRECT_DESCRIPTORS_ADDRESS                          0x0AE00
#define ETH3_RX_DIRECT_DESCRIPTORS_ADDRESS                          0x0B000
#define ETH4_RX_DIRECT_DESCRIPTORS_ADDRESS                          0x0BE00
#define INGRESS_HANDLER_BUFFER_BASE_ADDRESS                         0x00000

#define BBH_TX_TCONT_INDEX_ADDRESS                                  0x0BCB8
#define TCONTS_0_7_TABLE_ADDRESS                                    0x09400
#define TCONTS_8_39_TABLE_ADDRESS                                   0x09800
#define GPON_TX_QUEUES_TABLE_ADDRESS                                0x0D800

#define ETH_TX_MACS_DYNAMIC_TABLE_ADDRESS                           0x0C980
#define ETH_TX_MACS_STATIC_TABLE_ADDRESS                            0x03DC0
#define ETH_TX_QUEUES_DYNAMIC_DESCRIPTORS_ADDRESS                   0x0B900
#define ETH_TX_QUEUES_STATIC_DESCRIPTORS_ADDRESS                    0x03900
#define ETH_TX_QUEUES_D_POINTERS_TABLE_RUNNER_A_ADDRESS             0x03E00
#define ETH_TX_QUEUES_S_POINTERS_TABLE_RUNNER_A_ADDRESS             0x03C00
#define ETH_TX_QUEUES_POINTERS_TABLE_RUNNER_B_ADDRESS               0x0B800
#define ETH_TX_RS_QUEUES_DESCRIPTORS_ADDRESS                        0x08000
#define ETH_TX_QUEUE_DUMMY_DESCRIPTOR_ADDRESS                       0x0BF70

#define INGRESS_FILTERS_CONFIGURATION_TABLE_ADDRESS                 0x0BCE0
#define LOCAL_SWITCH_INGRESS_FILTERS_CONFIGURATION_TABLE_ADDRESS    0x0B7E0

#define BRIDGE_CONFIGURATION_REGISTER_ADDRESS                       0x0BD00
#define BCR_UNKNOWN_SA_MAC_COMMAND_ADDRESS                          ( BRIDGE_CONFIGURATION_REGISTER_ADDRESS + 8  )
#define BCR_UNKNOWN_DA_MAC_COMMAND_ADDRESS                          ( BRIDGE_CONFIGURATION_REGISTER_ADDRESS + 18 )
#define BCR_DS_CONNECTION_MISS_ACTION_FILTER_ADDRESS                ( BRIDGE_CONFIGURATION_REGISTER_ADDRESS + 48 )

#define CPU_RX_QUEUES_TABLE_ADDRESS                                 0x03F80
#define CPU_FREE_PACKET_DESCRIPTORS_POOL_ADDRESS                    0x0D000
#define CPU_FREE_PACKET_DESCRIPTORS_POOL_DESCRIPTOR_ADDRESS         0x03B20
#define CPU_RX_REASON_TO_QUEUE_TABLE_ADDRESS                        0x0B140
#define CPU_RX_REASON_TO_METER_TABLE_ADDRESS                        0x0B540
#define CPU_RX_METERS_TABLE_ADDRESS                                 0x0B180
#define CPU_RX_FAST_INGRESS_QUEUE_ADDRESS                           0x0B700
#define CPU_RX_PICO_INGRESS_QUEUE_ADDRESS                           0x0AB80
#define CPU_RX_FAST_INGRESS_QUEUE_PTR_ADDRESS                       0x0BCAE
#define CPU_RX_PICO_INGRESS_QUEUE_PTR_ADDRESS                       0x0B77C

#define CPU_TX_PICO_QUEUE_ADDRESS                                   0x0AD00
#define CPU_TX_FAST_QUEUE_ADDRESS                                   0x0AC00
#define CPU_TX_BBH_DESCRIPTORS_ADDRESS                              0x0A800
#define CPU_TX_MESSAGE_DATA_BUFFER_ADDRESS                          0x0ABC0

#define RUNNER_FLOW_HEADER_BUFFER_ADDRESS                           0x08F80
#define RUNNER_FLOW_HEADER_DESCRIPTOR_ADDRESS                       0x0B6F8
#define RUNNER_FLOW_IH_RESPONSE_ADDRESS                             0x0B6F0

#define PACKET_SRAM_TO_DDR_COPY_BUFFER_ADDRESS                      0x0AA80

#define VLAN_ACTION_INGRESS_QUEUE_ADDRESS                           0x0B980
#define ROUTER_INGRESS_QUEUE_ADDRESS                                0x0B680
#define BRIDGE_INGRESS_QUEUE_ADDRESS                                0x0B780
#define WAN_INTERWORKING_INGRESS_QUEUE_ADDRESS                      0x0B780
#define DOWNSTREAM_MULTICAST_INGRESS_QUEUE_ADDRESS                  0x0B600
#define RUNNER_A_LAN_ENQUEUE_INGRESS_QUEUE_ADDRESS                  0x09600
#define RUNNER_B_LAN_ENQUEUE_INGRESS_QUEUE_ADDRESS                  0x03600
#define LAN_ENQUEUE_INGRESS_QUEUE_PTR_ADDRESS                       0x0B776

#define SC_BUFFER_ADDRESS                                           0x0BCA0


#define FAST_RUNNER_GLOBAL_REGISTERS_INIT_ADDRESS                   0x0BC80
#define PICO_RUNNER_GLOBAL_REGISTERS_INIT_ADDRESS                   0x0BCC0
#define DOWNSTREAM_DMA_PIPE_BUFFER_ADDRESS                          0x0BCB8
#define MICROCODE_VERSION_ADDRESS                                   0x0BCB0

#define CPU_RX_NUMBER_OF_QUEUES                                     8

#define CPU_TX_FAST_THREAD_NUMBER                                   0
#define CPU_RX_THREAD_NUMBER                                        1
#define CPU_TX_PICO_THREAD_NUMBER                                   33

#define WAN_DIRECT_THREAD_NUMBER                                    8
#define WAN_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER                9
#define LOCAL_SWITCHING_ETH0_THREAD_NUMBER                          16
#define LOCAL_SWITCHING_ETH1_THREAD_NUMBER                          17
#define LOCAL_SWITCHING_ETH2_THREAD_NUMBER                          18
#define LOCAL_SWITCHING_ETH3_THREAD_NUMBER                          19
#define LOCAL_SWITCHING_ETH4_THREAD_NUMBER                          20
#define LOCAL_SWITCHING_LAN_ENQUEUE_THREAD_NUMBER                   44

#define LAN0_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER               8
#define LAN1_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER               9
#define LAN2_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER               10
#define LAN3_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER               11
#define LAN4_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER               12
#define CPU_US_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER             13

#define UPSTREAM_VLAN_THREAD_NUMBER                                 15
#define UPSTREAM_ROUTER_THREAD_NUMBER                               20
#define ETH0_TX_THREAD_NUMBER                                       40
#define ETH1_TX_THREAD_NUMBER                                       41
#define ETH2_TX_THREAD_NUMBER                                       42
#define ETH3_TX_THREAD_NUMBER                                       43
#define ETH4_TX_THREAD_NUMBER                                       44

#define CPU_RX_THREAD_WAKEUP_REQUEST_VALUE                          ( ( CPU_RX_THREAD_NUMBER << 4 ) + 1 )
#define WAN_INTERWORKING_THREAD_WAKEUP_REQUEST_VALUE                ( ( WAN_INTERWORKING_THREAD_NUMBER << 4 ) + 1 )
#define CPU_DS_CLASSIFICATION_THREAD_WAKEUP_REQUEST_VALUE           ( ( CPU_DS_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER << 4 ) + 1 )
#define CPU_US_CLASSIFICATION_THREAD_WAKEUP_REQUEST_VALUE           ( ( CPU_US_FILTERS_AND_CLASSIFICATION_THREAD_NUMBER << 4 ) + 1 )

#define BBH_PERIPHERAL_ETH0_TX                                  60
#define BBH_PERIPHERAL_ETH0_RX                                  28
#define BBH_PERIPHERAL_ETH1_TX                                  44
#define BBH_PERIPHERAL_ETH1_RX                                  12
#define BBH_PERIPHERAL_ETH2_TX                                  52
#define BBH_PERIPHERAL_ETH2_RX                                  20
#define BBH_PERIPHERAL_ETH3_TX                                  40
#define BBH_PERIPHERAL_ETH3_RX                                  8
#define BBH_PERIPHERAL_ETH4_TX                                  48
#define BBH_PERIPHERAL_ETH4_RX                                  16
#define BBH_PERIPHERAL_IH                                       6

#define IH_RUNNER_B_BUFFER_MASK                                 0x40

#define GPON_SRC_PORT                                           0
#define MIPS_C_SRC_PORT                                         6
#define FAST_RUNNER_A_SRC_PORT                                  14
#define FAST_RUNNER_B_SRC_PORT                                  15

#endif /* _BL_LILAC_RNR_DEFS_H */

