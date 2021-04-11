#define MAX_HIJACK_PAGES 8
#define FILTER_BUFFER_LEN 512
#define MAC_ADDR_LEN 20

#define ROUTER_IP_ADDR_SIZE 16
#define ROUTER_IPV6_ADDR_SIZE 48

// standard ethernet pdu
#define DATA_MAX_PDU	1500
#define HIJACK_INTERFACE "br0"

/**  
 * definitions used by packet injection routines
 */
#define DATA_PADDING_BOUNDARY 4
#define DATA_BUFFER_LENGTH 256


#define GET_HEADER_LEN_IP(__ip_ptr) (__ip_ptr->ihl << 2)
#define GET_HEADER_LEN_TCP(__tcp_ptr)	(__tcp_ptr->doff << 2)
