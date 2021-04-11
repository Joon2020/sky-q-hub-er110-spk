/*
* <:copyright-BRCM:2013:DUAL/GPL:standard
* 
*    Copyright (c) 2013 Broadcom Corporation
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


#ifndef RDPA_CPU_H_
#define RDPA_CPU_H_

/** \defgroup cpu CPU Interface
 * Functions in this module:
 * - Send packets from the host to any egress port or to virtual "bridge" port
 * - Receive packets via one of host termination (CPU_RX) or Wi-Fi acceleration (WLAN_TX) queues
 * - Connect per-port and/or per-receive queue interrupt handlers
 * - Configure CPU trap reason --> queue mapping
 *
 * Runner to host traffic management parameters can be configured similarly to any other egress queue.
 * Initial port level (CPU, WLAN0, WLAN1) configuration is done using the appropriate port configuration
 * that fixes parameters such as scheduling type, number of queues, etc.\n
 * @{
 */

/** CPU port */
typedef enum
{
    rdpa_cpu_host,      /**< Host Rx */
    rdpa_cpu_wlan0,     /**< WLAN 0 TX */
    rdpa_cpu_wlan1,     /* Reserved for future use */

    rdpa_cpu_port__num_of
} rdpa_cpu_port;


#define RDPA_CPU_MAX_QUEUES         8   /**< Max number of queues on host port */
#define RDPA_WLAN_MAX_QUEUES        4   /**< Max number of queues on WLAN port */
#define RDPA_TOTAL_RING_QUEUES      (RDPA_CPU_MAX_QUEUES) + (RDPA_WLAN_MAX_QUEUES)

/** \defgroup isr Interrupt Control
 * \ingroup cpu
 * Functions in this group allow to
 * - Register / unregister per-queue interrupt handler
 * - Enable / disable / clear per-queue interrupt
 * @{
 */

/** Enable CPU queue interrupt
 * \param[in]   port        port. rdpa_cpu, rdpa_wlan0, rdpa_wlan1
 * \param[in]   queue       Queue index < num_queues in port tm configuration
 */
void rdpa_cpu_int_enable(rdpa_cpu_port port, int queue);

/** Disable CPU queue interrupt
 * \param[in]   port        port. rdpa_cpu, rdpa_wlan0, rdpa_wlan1
 * \param[in]   queue       Queue index < num_queues in port tm configuration
 */
void rdpa_cpu_int_disable(rdpa_cpu_port port, int queue);

/** Clear CPU queue interrupt
 * \param[in]   port        port. rdpa_cpu, rdpa_wlan0, rdpa_wlan1
 * \param[in]   queue       Queue index < num_queues in port tm configuration
 */
void rdpa_cpu_int_clear(rdpa_cpu_port port, int queue);

/** @} end of isr Doxygen group */

/** \defgroup cpu_rx Receive
 * \ingroup cpu
 * Functions in this group allow to
 * - Register/unregister per-port and per-queue interrupt handlers
 * - Register/unregister packet handler
 * - Pull received packets from port (scheduling) or queue
 * @{
 */

/** Max number of CPU interface meters per direction */
#define RDPA_CPU_MAX_DS_METERS   4      /**< Number of meters for packets received from WAN port(s) */
#define RDPA_CPU_MAX_US_METERS   16     /**< Number of meters for packets received from LAN port(s) */

/** CPU meter SR validation constants */
#define RDPA_CPU_METER_MIN_SR       100         /**< Min CPU meter SR value in pps */
#define RDPA_CPU_METER_MAX_SR       40000       /**< Max CPU meter SR value in pps */
#define RDPA_CPU_METER_SR_QUANTA    100         /**< CPU meter SR must be a multiple of quanta */

/** CPU trap reasons */
typedef enum
{
    rdpa_cpu_reason_min = 0,

    rdpa_cpu_rx_reason_oam                 = 0, /**< OAM packet */
    rdpa_cpu_rx_reason_omci                = 1, /**< OMCI packet */
    rdpa_cpu_rx_reason_flow                = 2,
    rdpa_cpu_rx_reason_mcast               = 3, /**< Multicat packet */
    rdpa_cpu_rx_reason_bcast               = 4, /**< Broadcast packet */
    rdpa_cpu_rx_reason_igmp                = 5, /**< Igmp packet */
    rdpa_cpu_rx_reason_icmpv6              = 6, /**< Icmpv6 packet */
    rdpa_cpu_rx_reason_mac_trap_0          = 7,
    rdpa_cpu_rx_reason_mac_trap_1          = 8,
    rdpa_cpu_rx_reason_mac_trap_2          = 9,
    rdpa_cpu_rx_reason_mac_trap_3          = 10,
    rdpa_cpu_rx_reason_dhcp                = 11, /**< DHCP packet */
    rdpa_cpu_rx_reason_local_ip            = 13,
    rdpa_cpu_rx_reason_hdr_err             = 14, /**< Packet with IP header error */
    rdpa_cpu_rx_reason_sa_moved            = 15, /**< SA move indication*/
    rdpa_cpu_rx_reason_unknown_sa          = 16, /**< Unknown SA indication */
    rdpa_cpu_rx_reason_unknown_da          = 17, /**< Unknown DA indication */
    rdpa_cpu_rx_reason_ip_frag             = 18, /**< Packet is fragmented */
    rdpa_cpu_rx_reason_direct_queue_0      = 20, /* */
    rdpa_cpu_rx_reason_direct_queue_1      = 21, /* */
    rdpa_cpu_rx_reason_direct_queue_2      = 22, /* */
    rdpa_cpu_rx_reason_direct_queue_3      = 23, /* */
    rdpa_cpu_rx_reason_direct_queue_4      = 24, /* */
    rdpa_cpu_rx_reason_direct_queue_5      = 25, /* */
    rdpa_cpu_rx_reason_direct_queue_6      = 26, /* */
    rdpa_cpu_rx_reason_direct_queue_7      = 27, /* */
    rdpa_cpu_rx_reason_etype_udef_0        = 28, /**< User defined ethertype 1 */
    rdpa_cpu_rx_reason_etype_udef_1        = 29, /**< User defined ethertype 2 */
    rdpa_cpu_rx_reason_etype_udef_2        = 30, /**< User defined ethertype 3 */
    rdpa_cpu_rx_reason_etype_udef_3        = 31, /**< User defined ethertype 4 */
    rdpa_cpu_rx_reason_etype_pppoe_d       = 32, /**< PPPoE Discovery */
    rdpa_cpu_rx_reason_etype_pppoe_s       = 33, /**< PPPoE Source */
    rdpa_cpu_rx_reason_etype_arp           = 34, /**< Packet with ethertype Arp */
    rdpa_cpu_rx_reason_etype_ptp_1588      = 35, /**< Packet with ethertype 1588 */
    rdpa_cpu_rx_reason_etype_802_1x        = 36, /**< Packet with ethertype 802_1x */
    rdpa_cpu_rx_reason_etype_801_1ag_cfm   = 37, /**< Packet with ethertype v801 Lag CFG*/
    rdpa_cpu_rx_reason_non_tcp_udp         = 40, /**< Packet is non TCP or UDP */
    rdpa_cpu_rx_reason_ip_flow_miss        = 41, /**< Flow miss indication */
    rdpa_cpu_rx_reason_tcp_flags           = 42, /**< TCP flag indication */
    rdpa_cpu_rx_reason_ttl_expired         = 43, /**< TTL expired indication */
    rdpa_cpu_rx_reason_arp_table_miss      = 44, /* */
    rdpa_cpu_rx_reason_l4_icmp             = 45, /**< layer-4 ICMP protocol */
    rdpa_cpu_rx_reason_l4_esp              = 46, /**< layer-4 ESP protocol */
    rdpa_cpu_rx_reason_l4_gre              = 47, /**< layer-4 GRE protocol */
    rdpa_cpu_rx_reason_l4_ah               = 48, /**< layer-4 AH protocol */
    rdpa_cpu_rx_reason_l4_ipv6             = 50, /**< layer-4 IPV6 protocol */
    rdpa_cpu_rx_reason_l4_udef_0           = 51, /**< User defined layer-4 1 */
    rdpa_cpu_rx_reason_l4_udef_1           = 52, /**< User defined layer-4 2 */
    rdpa_cpu_rx_reason_l4_udef_2           = 53, /**< User defined layer-4 3 */
    rdpa_cpu_rx_reason_l4_udef_3           = 54, /**< User defined layer-4 4 */
    rdpa_cpu_rx_reason_firewall_match      = 55, /* */
    rdpa_cpu_rx_reason_connection_trap_0   = 56, /**< User defined connection 1 */
    rdpa_cpu_rx_reason_connection_trap_1   = 57, /**< User defined connection 2 */
    rdpa_cpu_rx_reason_connection_trap_2   = 58, /**< User defined connection 3 */
    rdpa_cpu_rx_reason_connection_trap_3   = 59, /**< User defined connection 4 */
    rdpa_cpu_rx_reason_connection_trap_4   = 60, /**< User defined connection 5 */
    rdpa_cpu_rx_reason_connection_trap_5   = 61, /**< User defined connection 6 */
    rdpa_cpu_rx_reason_connection_trap_6   = 62, /**< User defined connection 7 */
    rdpa_cpu_rx_reason_connection_trap_7   = 63, /**< User defined connection 8 */

    rdpa_cpu_reason__num_of
} rdpa_cpu_reason;

/** Reason index. Underlying type of cpu_reason_index aggregate */
typedef struct
{
    rdpa_traffic_dir dir;       /** Traffic direction */
    rdpa_cpu_reason reason;     /** CPU reason */
    int entry_index;            /* Reserved for internal use. */
} rdpa_cpu_reason_index_t;

/** Reason configuration structure.
 * Underlying structure for cpu_reason_cfg aggregate
 */
typedef struct
{
    bdmf_index queue;           /**< reason --> queue mapping */
    bdmf_index meter;           /**< CPU interface meter index < RDPA_CPU_MAX_METERS or BDMF_INDEX_UNDEFINED */
    rdpa_ports meter_ports;     /**< Mask of ports for which the policer is active */
} rdpa_cpu_reason_cfg_t;

/** CPU meter configuration
 * Underlying structure for cpu_meter_cfg aggregate
 */
typedef struct
{
    uint32_t sir;               /**< SIR */
} rdpa_cpu_meter_cfg_t;

/** Receive packet info */
typedef struct
{
    rdpa_cpu_reason reason;     /**< trap reason */
    rdpa_if src_port;           /**< source port */
    uint32_t reason_data;       /**< Reason-specific data.
                                     For CPU port it usually contains src_flow
                                     For Wi-Fi port it contains source SSID index in 16 MSB and destination SSID vector in 16 LSB
                                     It can contain other reason-specific info (such as OAM info, etc.).
                                */
    uint16_t  wl_chain_priority; /** wlan chainid with priority  (chainid|priority)*/
    /* ToDo: we can consider adding a union here */
} rdpa_cpu_rx_info_t;

typedef void (*rdpa_cpu_rxq_rx_isr_cb_t)(long isr_priv);
typedef void (*rdpa_cpu_rxq_rx_cb_t)(long priv, bdmf_sysb sysb, rdpa_cpu_rx_info_t *info);

/** Host receive configuration.
 * Underlying structure of cpu_rx_cfg aggregate.
 */
typedef struct {

    /** ISR callback
     * If set ISR is connected to the appropriate port+queue
     * \param[in]   isr_priv
     */
    rdpa_cpu_rxq_rx_isr_cb_t rx_isr;

    /** System buffer type */
    bdmf_sysb_type sysb_type;

    uint32_t size;              /**< Queue size */
#define RDPA_CPU_QUEUE_MAX_SIZE  1024
#define RDPA_CPU_WLAN_QUEUE_MAX_SIZE  1024
    bdmf_boolean dump;          /**< Dump rx data */
    long isr_priv;              /**< Parameter to be passed to isr callback */
} rdpa_cpu_rxq_cfg_t;

/** Receive statistics.
 * Underlying structure for cpu_rx_stat aggregate
 */
typedef struct
{
    uint32_t received;      /**< Packets passed to rx callback */
    uint32_t queued;        /**< Packets in the queue */
    uint32_t dropped;       /**< Packets dropped by RDD / FW */
    uint32_t interrupts;    /**< interrupts */
} rdpa_cpu_rx_stat_t;


/** Pull a single received packet from host queue.
 * This function pulls a single packet from port or queue.
 * If queue==UNASSIGNED, the function invokes queue scheduler,
 * otherwise, it tries to pull the packet from the specified queue.
 * port/queue-level packet handler is called for the packet.
 * \param[in]	port			CPU port (CPU, PCI1, PCI2)
 * \param[in]	queue			Queue index < num_queues in rdpa_port_tm_cfg_t or UNASSIGNED
 * \param[out]  sysb                    Packet data (buffer)
 * \param[out]  info                    Packet info
 * \return =1-packet pulled ok, ==0-no packet pulled, <0-int error code\n
 */
int rdpa_cpu_packet_get(rdpa_cpu_port port, bdmf_index queue,
    bdmf_sysb *sysb, rdpa_cpu_rx_info_t *info);


/** @} end of cpu_rx Doxygen group */

/** \defgroup cpu_tx Transmit
 * \ingroup cpu
 * Packets can be transmitted
 * - via specific port/queue
 * - using FW bridging & classification logic
 *
 * All rdpa_cpu_send_to_xx functions return int error code
 * - If rc==0, buffer ownership is passed to GMP. The buffer will be released when safe.
 * - If rc!=0, buffer ownership stays with the caller
 * @{
 */

/** CPU tx packet insertion point */
typedef enum
{
    rdpa_cpu_tx_port,           /**< Egress port and priority are specified explicitly. This is the most common mode */
    rdpa_cpu_tx_bridge,         /**< Before bridge forwarding decision, before classification */

    rdpa_cpu_tx_entry__num_of   /**< Number of CPU TX entries */
} rdpa_cpu_tx_method;

/** Extra data that can be passed along with the packet to be transmitted */
typedef struct
{
    rdpa_cpu_tx_method method;  /**< Packet transmit method */
    rdpa_if port;               /**< Destination port for method=port, source port for method=bridge*/

    union {
        /* queue_id in the following substructures must overlap */
        struct {
            uint32_t queue_id;          /**< Egress queue id */
        } lan;

        struct {
            uint32_t queue_id;          /**< Egress queue id. method=port only */
            rdpa_flow flow;             /**< Destination flow for method=port, Source flow for method=bridge,port=wan */
        } wan;

        uint32_t oam_data;              /**< Extra data entry-specific */
    } x;
} rdpa_cpu_tx_info_t;

/** Transmit statistics */
typedef struct
{
    uint32_t tx_ok;                     /**< Successfully passed to RDD for transmission */
    uint32_t tx_invalid_queue;          /**< Discarded because transmit queue is not configured */
    uint32_t tx_no_buf;                 /**< Discarded because couldn't allocate BPM buffer */
    uint32_t tx_too_long;               /**< Discarded packets with length > max MTU size */
    uint32_t tx_rdd_error;              /**< Discarded because RDD returned error */
} rdpa_cpu_tx_stat_t;


/** Send system buffer
 *
 * \param[in]   sysb        System buffer. Released regardless on the function outcome
 * \param[in]   info        Tx info
 * \return 0=OK or int error code\n
 */
int rdpa_cpu_send_sysb(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info);

/** Send raw packet
 *
 * \param[in]   data        Packet data
 * \param[in]   length      Packet length
 * \param[in]   info        Info
 * \return 0=OK or int error code\n
 */
int rdpa_cpu_send_raw(void *data, uint32_t length, const rdpa_cpu_tx_info_t *info);

/** Get cpu queue emptiness status
 *
 * \param[in]	port			CPU port (CPU, PCI1, PCI2)
 * \param[in]	queue			Queue index < num_queues in rdpa_port_tm_cfg_t or UNASSIGNED
 * \return 0=no packets are waiting or at least one packet in queue\n
 */
int rdpa_cpu_queue_not_empty(rdpa_cpu_port port, bdmf_index queue);

/** Send system buffer - Special function to send EPON Dying
 *  Gasp:
 *
 * \param[in]   sysb        System buffer. Released regardless on the function outcome
 * \param[in]   info        Tx info
 * \return 0=OK or int error code\n
 *  
 *  */ 
int rdpa_cpu_send_epon_dying_gasp(bdmf_sysb sysb, const rdpa_cpu_tx_info_t *info);

/** @} end of cpu_tx Doxygen group */
/** @} end of cpu Doxygen group */

#endif /* RDPA_CPU_H_ */
