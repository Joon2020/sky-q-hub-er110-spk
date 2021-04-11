#ifndef __PSEUDO_HEADER_H__
#define __PSEUDO_HEADER_H__

typedef struct _PseudoHeader{

	unsigned long int source_ip;
	unsigned long int dest_ip;
	unsigned char reserved;
	unsigned char protocol;
	unsigned short int tcp_length;

} PseudoHeader;

#ifdef SKY_IPV6

typedef struct _PseudoHeaderv6 {
	struct in6_addr source_ip;
	struct in6_addr dest_ip;
	uint32_t tcp_len;
	uint8_t reserved[3];
	uint8_t next_header;
} PseudoHeaderv6;

#endif


#endif /* __PSEUDO_HEADER_H__ */
