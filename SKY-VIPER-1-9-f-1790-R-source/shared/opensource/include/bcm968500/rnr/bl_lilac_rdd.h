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

#ifndef _LILAC_RDD_H
#define _LILAC_RDD_H


/* Runner Device Driver Errors */
typedef enum
{
    RDD_OK                                   = 0,
    RDD_ERROR_MALLOC_FAILED                  = 10,
    RDD_ERROR_ILLEGAL_BRIDGE_PORT_ID         = 11,
    RDD_ERROR_ILLEGAL_EMAC_ID                = 12,
    RDD_ERROR_ILLEGAL_QUEUE_ID               = 13,
    RDD_ERROR_CPU_RX_QUEUE_ILLEGAL           = 100,
    RDD_ERROR_CPU_RX_REASON_ILLEGAL          = 101,
    RDD_ERROR_CPU_RX_QUEUE_EMPTY             = 102,
    RDD_ERROR_CPU_RX_QUEUE_INVALID           = 103,
    RDD_ERROR_CPU_TX_QUEUE_FULL              = 120,
    RDD_ERROR_CPU_TX_NOT_ALLOWED             = 122,
    RDD_ERROR_BPM_ALLOC_FAIL                 = 230,
    RDD_ERROR_BPM_FREE_FAIL                  = 231,
}
RDD_ERROR;


typedef enum
{
    RDD_WAN_BRIDGE_PORT            = 0,
    RDD_LAN0_BRIDGE_PORT           = 1,
    RDD_LAN1_BRIDGE_PORT           = 2,
    RDD_LAN2_BRIDGE_PORT           = 3,
    RDD_LAN3_BRIDGE_PORT           = 4,
    RDD_LAN4_BRIDGE_PORT           = 5,
    RDD_WAN_ROUTER_PORT            = 6,
}
RDD_BRIDGE_PORT;



typedef enum
{
    RDD_WAN_PHYSICAL_PORT_ETH4 = 1,
}
RDD_WAN_PHYSICAL_PORT;


typedef enum
{
    RDD_EMAC_ID_0 = 1,
    RDD_EMAC_ID_1 = 2,
    RDD_EMAC_ID_2 = 3,
    RDD_EMAC_ID_3 = 4,
    RDD_EMAC_ID_4 = 5,
}
RDD_EMAC_ID;


typedef enum
{
    RDD_QUEUE_0 = 0,
}
RDD_QUEUE_ID;


typedef enum
{
    RDD_CPU_RX_REASON_PLOAM             = 0,
    RDD_CPU_RX_REASON_OMCI              = 1,
    RDD_CPU_RX_REASON_FLOW              = 2,
    RDD_CPU_RX_REASON_MULTICAST         = 3,
    RDD_CPU_RX_REASON_BROADCAST         = 4,
    RDD_CPU_RX_REASON_IGMP              = 5,
    RDD_CPU_RX_REASON_ICMPV6            = 6,
    RDD_CPU_RX_REASON_MAC_TRAP_0        = 7,
    RDD_CPU_RX_REASON_MAC_TRAP_1        = 8,
    RDD_CPU_RX_REASON_MAC_TRAP_2        = 9,
    RDD_CPU_RX_REASON_MAC_TRAP_3        = 10,
    RDD_CPU_RX_REASON_DHCP              = 11,
    RDD_CPU_RX_REASON_MLD               = 12,
    RDD_CPU_RX_REASON_LOCAL_IP          = 13,
    RDD_CPU_RX_REASON_IP_HEADER_ERROR   = 14,
    RDD_CPU_RX_REASON_SA_MOVED          = 15,
    RDD_CPU_RX_REASON_UNKNOWN_SA        = 16,
    RDD_CPU_RX_REASON_UNKNOWN_DA        = 17,
    RDD_CPU_RX_REASON_IP_FRAGMENT       = 18,
    RDD_CPU_RX_REASON_PROPRIETARY       = 19,
    RDD_CPU_RX_REASON_DIRECT_QUEUE_0    = 20,
    RDD_CPU_RX_REASON_DIRECT_QUEUE_1    = 21,
    RDD_CPU_RX_REASON_DIRECT_QUEUE_2    = 22,
    RDD_CPU_RX_REASON_DIRECT_QUEUE_3    = 23,
    RDD_CPU_RX_REASON_DIRECT_QUEUE_4    = 24,
    RDD_CPU_RX_REASON_DIRECT_QUEUE_5    = 25,
    RDD_CPU_RX_REASON_DIRECT_QUEUE_6    = 26,
    RDD_CPU_RX_REASON_DIRECT_QUEUE_7    = 27,
    RDD_CPU_RX_REASON_ETHER_TYPE_0      = 28,
    RDD_CPU_RX_REASON_ETHER_TYPE_1      = 29,
    RDD_CPU_RX_REASON_ETHER_TYPE_2      = 30,
    RDD_CPU_RX_REASON_ETHER_TYPE_3      = 31,
    RDD_CPU_RX_REASON_ETHER_TYPE_4      = 32,
    RDD_CPU_RX_REASON_ETHER_TYPE_5      = 33,
    RDD_CPU_RX_REASON_ETHER_TYPE_6      = 34,
    RDD_CPU_RX_REASON_ETHER_TYPE_7      = 35,
    RDD_CPU_RX_REASON_ETHER_TYPE_8      = 36,
    RDD_CPU_RX_REASON_ETHER_TYPE_9      = 37,
    RDD_CPU_RX_REASON_ETHER_TYPE_10     = 38,
    RDD_CPU_RX_REASON_ETHER_TYPE_11     = 39,
    RDD_CPU_RX_REASON_NON_TCP_UDP       = 42,
    RDD_CPU_RX_REASON_CONNECTION_MISS   = 43,
    RDD_CPU_RX_REASON_TCP_FLAGS         = 44,
    RDD_CPU_RX_REASON_TTL_EXPIRED       = 45,
    RDD_CPU_RX_REASON_ARP_TABLE_MISS    = 46,
    RDD_CPU_RX_REASON_LAYER4_PROTOCOL_0 = 47,
    RDD_CPU_RX_REASON_LAYER4_PROTOCOL_1 = 48,
    RDD_CPU_RX_REASON_LAYER4_PROTOCOL_2 = 49,
    RDD_CPU_RX_REASON_LAYER4_PROTOCOL_3 = 50,
    RDD_CPU_RX_REASON_LAYER4_ESP        = 51,
    RDD_CPU_RX_REASON_LAYER4_GRE        = 52,
    RDD_CPU_RX_REASON_LAYER4_AH         = 53,
    RDD_CPU_RX_REASON_LAYER4_ICMP       = 54,
    RDD_CPU_RX_REASON_CONNECTION_TRAP_0 = 56,
    RDD_CPU_RX_REASON_CONNECTION_TRAP_1 = 57,
    RDD_CPU_RX_REASON_CONNECTION_TRAP_2 = 58,
    RDD_CPU_RX_REASON_CONNECTION_TRAP_3 = 59,
    RDD_CPU_RX_REASON_CONNECTION_TRAP_4 = 60,
    RDD_CPU_RX_REASON_CONNECTION_TRAP_5 = 61,
    RDD_CPU_RX_REASON_CONNECTION_TRAP_6 = 62,
    RDD_CPU_RX_REASON_CONNECTION_TRAP_7 = 63,
    RDD_CPU_RX_REASON_MAX_REASON        = 63,
}
RDD_CPU_RX_REASON;

#define RDD_RATE_LIMITER_IDLE  16

typedef enum
{
    RDD_CPU_RX_QUEUE_0 = 0,
}
RDD_CPU_RX_QUEUE;


typedef enum
{
    RDD_METER_DISABLE = 4
}
RDD_CPU_METER;

typedef enum
{
    RDD_DOWNSTREAM = 0,
    RDD_UPSTREAM   = 1,
}
RDD_DIRECTION;

typedef enum
{
    RDD_SA_MAC_LOOKUP_DISABLE = 0,
    RDD_SA_MAC_LOOKUP_ENABLE  = 1,
}
RDD_SA_MAC_LOOKUP;

typedef enum
{
    RDD_DA_MAC_LOOKUP_DISABLE = 0,
    RDD_DA_MAC_LOOKUP_ENABLE  = 1,
}
RDD_DA_MAC_LOOKUP;


typedef enum
{
    RDD_UNKNOWN_MAC_CMD_FORWARD  = 1,
    RDD_UNKNOWN_MAC_CMD_CPU_TRAP = 2,
    RDD_UNKNOWN_MAC_CMD_DROP     = 4,
}
RDD_UNKNOWN_MAC_COMMAND;


typedef enum
{
    RDD_MAC_TABLE_SIZE_32   = 0,
    RDD_MAC_TABLE_SIZE_64   = 1,
    RDD_MAC_TABLE_SIZE_128  = 2,
    RDD_MAC_TABLE_SIZE_256  = 3,
    RDD_MAC_TABLE_SIZE_512  = 4,
    RDD_MAC_TABLE_SIZE_1024 = 5,
    RDD_MAC_TABLE_SIZE_2048 = 6,
    RDD_MAC_TABLE_SIZE_4096 = 7,
}E_RDD_MAC_TABLE_SIZE;

#define  RDD_MAC_TABLE_MAX_HOP_32 5

typedef enum
{
    RDD_NO_WAIT = 0,
    RDD_WAIT    = 1,
}
RDD_CPU_WAIT;


typedef void ( * RDD_LOCK_CRITICAL_SECTION_FP )( uint32_t * );
typedef void ( * RDD_UNLOCK_CRITICAL_SECTION_FP )( uint32_t );



/* API declaration */

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
RDD_ERROR bl_rdd_init (void);

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
RDD_ERROR bl_rdd_load_microcode ( uint8_t  *xi_runner_A_microcode_ptr,
                                                     uint8_t  *xi_runner_B_microcode_ptr,
                                                     uint8_t  *xi_runner_C_microcode_ptr,
                                                     uint8_t  *xi_runner_D_microcode_ptr );

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
RDD_ERROR bl_rdd_runner_enable ( void );

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
RDD_ERROR bl_rdd_runner_disable ( void );

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
RDD_ERROR bl_rdd_runner_frequency_set ( uint16_t  xi_runner_frequency );

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
                                                           RDD_WAN_PHYSICAL_PORT  xi_wan_physical_port );

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
                                                          uint16_t                 xi_packet_threshold );

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
											   uint16_t                *xo_mdu_mode_pointer );

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
                                                          uint16_t                     xi_interrupt_id );

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
                                                                 RDD_DIRECTION      xi_direction );

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
                                                         RDD_CPU_RX_REASON  *xo_reason );

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
                                                              RDD_QUEUE_ID  xi_queue_id );

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
                                                           RDD_SA_MAC_LOOKUP  xi_src_mac_lookup_mode );

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
                                                                RDD_UNKNOWN_MAC_COMMAND  xi_slf_cmd );

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
RDD_ERROR bl_rdd_get_rdd_version ( uint32_t * const xo_version );

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
RDD_ERROR bl_rdd_get_microcode_version ( uint32_t  *xo_microcode_version );

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
                                                              RDD_UNLOCK_CRITICAL_SECTION_FP  xi_unlock_function );

#endif /* _LILAC_RDD_H */

