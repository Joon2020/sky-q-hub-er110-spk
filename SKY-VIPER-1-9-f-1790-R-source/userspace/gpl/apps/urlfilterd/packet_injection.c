#include<stdio.h>
#include<stdlib.h>

#include <netinet/ip.h>
#include <netinet/tcp.h>

#ifdef SKY_IPV6
#include <netinet/ip6.h>
#endif

#include<sys/time.h>

#include "common.h"
#include "rawsock.h"
#include "pseudo_header.h"

#ifndef ETHERTYPE_IPV6
	// uclibc delivered with the broadcom toolchain is too old and does not contain this definition in net/ethernet.h
	#warning manually defining ETHERTYPE_IPV6
	#define ETHERTYPE_IPV6	0x86dd
#endif

#define UFD_DEBUG
extern char routerIPAddress[ROUTER_IP_ADDR_SIZE];

#ifdef SKY_IPV6
extern char routerIPv6Address[ROUTER_IPV6_ADDR_SIZE];
#endif


void CreateEthernetHeader(struct ethhdr *ethernet_header, char *src_mac, char *dst_mac, int protocol)
{
	if (!ethernet_header || !src_mac || !dst_mac) {
		return;
	}

	memcpy(ethernet_header->h_source, (void *)ether_aton(src_mac), 6);

	/* copy the Dst mac addr */

	memcpy(ethernet_header->h_dest, (void *)ether_aton(dst_mac), 6);

	/* copy the protocol */

	ethernet_header->h_proto = htons(protocol);

	/* done ...send the header back */
}


/** 
 * Ripped from Richard Stevans Book 
 * [tw] Tue May 28 17:12:35 BST 2013 the implementation was wrong, making untrustful assumptions 
 * about data type size and unnecesary data folding in 16 bit word summing loop
 */
uint16_t ComputeChecksum(uint8_t *data, int len)
{
	uint32_t sum = 0;  
	uint16_t *temp = (uint16_t *)data;

	while(len > 1){
		sum += *temp++;
		len -= 2;
	}

	if (len) {
		/* take care of left over byte */
		uint16_t temp2;
		*(uint16_t *)(&temp2) = *(uint8_t *)temp;
		sum += temp2;
	}

	sum = (sum & 0xFFFF) + (sum >> 16);
	sum += (sum >> 16);

	return ~sum;
}


#ifdef SKY_IPV6
void CreateIPv6Header(struct ip6_hdr* ip6_header, struct ip6_hdr *iph, int len)
{
	if (!ip6_header || !iph) {
		return;
	}
	
	memset(ip6_header, 0x00, sizeof(struct ip6_hdr));

	// version 6, flow label 0, default class of service
	ip6_header->ip6_flow = htonl((unsigned int)0x60000000);

	ip6_header->ip6_src = iph->ip6_dst;
	ip6_header->ip6_dst = iph->ip6_src;
	ip6_header->ip6_plen = htons(len + sizeof(struct tcphdr));
	ip6_header->ip6_nxt = IPPROTO_TCP;

	// random value chosen (1 would do since its intended for LAN only)
	ip6_header->ip6_hlim = 16;	
}
#endif


//struct iphdr *CreateIPHeader(struct iphdr *iph, int len)
void CreateIPHeader(struct iphdr* ip_header, struct iphdr *iph, int len)
{
	if (!ip_header || !iph) {
		return;
	}

	ip_header->version = 4;
	ip_header->ihl = (sizeof(struct iphdr)) >> 2 ;
	ip_header->tos = 0;
	ip_header->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr) + len);
	ip_header->id = htons(242);
	ip_header->frag_off = 0;
	ip_header->ttl = 111;
	ip_header->protocol = IPPROTO_TCP;
	ip_header->check = 0; /* We will calculate the checksum later */
	ip_header->saddr = iph->daddr;
	ip_header->daddr = iph->saddr;

	/* Calculate the IP checksum now : 
	   The IP Checksum is only over the IP header */
	ip_header->check = ComputeChecksum((unsigned char *)ip_header, ip_header->ihl*4);
}


void CreateTcpHeader(struct tcphdr* tcp_header, struct tcphdr *tcph, int len)
{
	if (!tcp_header || !tcph) {
		return;
	}
	
	tcp_header->source = tcph->dest;
	tcp_header->dest = tcph->source;
	tcp_header->seq = tcph->ack_seq;
	tcp_header->ack_seq = tcph->seq + len;
	tcp_header->res1 = 0;
	tcp_header->doff = (sizeof(struct tcphdr))/4;
	tcp_header->fin = 1;
	tcp_header->ack = 1;
	tcp_header->psh = tcph->psh;
	tcp_header->window = 0;
	tcp_header->check = 0; /* Will calculate the checksum with pseudo-header later */
	tcp_header->urg_ptr = 0;
}

#ifdef SKY_IPV6
void CreatePseudoHeaderv6AndComputeTcpChecksum(struct tcphdr *tcp_header, struct ip6_hdr *ip_header, unsigned char *data, int dataSize)
{
	unsigned char buff[DATA_MAX_PDU] = {0};	

	int segment_len = ntohs(ip_header->ip6_plen);
	int header_len = segment_len + sizeof(PseudoHeaderv6);

	if (header_len > DATA_MAX_PDU) {
		// very unlikely but must be considered
		return;
	}

	PseudoHeaderv6 *ps = (PseudoHeaderv6*)buff;
	memset(ps, 0x00, sizeof(PseudoHeaderv6));

	ps->source_ip = ip_header->ip6_src;
	ps->dest_ip = ip_header->ip6_dst;
	ps->tcp_len = ip_header->ip6_plen;
	ps->next_header = ip_header->ip6_nxt;

	/* Now copy TCP */
	memcpy((void *)(buff + sizeof(PseudoHeaderv6)), (void *)tcp_header, GET_HEADER_LEN_TCP(tcp_header));

	/* Now copy the Data */
	memcpy((void *)(buff + sizeof(PseudoHeaderv6) + GET_HEADER_LEN_TCP(tcp_header)), data, dataSize);

	/* Calculate the Checksum */
	tcp_header->check = ComputeChecksum(buff, header_len);
}
#endif


void CreatePseudoHeaderAndComputeTcpChecksum(struct tcphdr *tcp_header, struct iphdr *ip_header, unsigned char *data, int dataSize)
{
	/*The TCP Checksum is calculated over the PseudoHeader + TCP header +Data*/
	unsigned char hdr[DATA_MAX_PDU] = {0};	

	/* Find the size of the TCP Header + Data */
	int segment_len = ntohs(ip_header->tot_len) - GET_HEADER_LEN_IP(ip_header); 

	/* Total length over which TCP checksum will be computed */
	int header_len = sizeof(PseudoHeader) + segment_len;

	if (header_len > DATA_MAX_PDU) {
		// very unlikely but must be considered
		return;
	}

	memset(hdr, 0x00, sizeof(hdr));

	/* Fill in the pseudo header first */
	PseudoHeader *pseudo_header = (PseudoHeader *)hdr;
	pseudo_header->source_ip = ip_header->saddr;
	pseudo_header->dest_ip = ip_header->daddr;
	pseudo_header->reserved = 0;
	pseudo_header->protocol = ip_header->protocol;
	pseudo_header->tcp_length = htons(segment_len);

	
	/* Now copy TCP */
	memcpy((hdr + sizeof(PseudoHeader)), (void *)tcp_header, GET_HEADER_LEN_TCP(tcp_header));

	/* Now copy the Data */
	memcpy((hdr + sizeof(PseudoHeader) + GET_HEADER_LEN_TCP(tcp_header)), data, dataSize);

	/* Calculate the Checksum */
	tcp_header->check = ComputeChecksum(hdr, header_len);
}

 

static char getSkyHijack()
{
        FILE *fs=NULL;
	char hijackBitmap = 0;

        if((fs = fopen("/var/redirectAll", "r")))
        {
                fread(&hijackBitmap,1,1,fs);
                fclose(fs);
        }
        else
        {
                perror("could not open file  /var/redirectAll");
        }

	return hijackBitmap;
}


#ifdef SKY_IPV6

void inject_http_ipv6_hijack_reply(char *dest_mac, char *datagram)
{
	char hijackBitmap = 0;
	int raw = 0;

	int s_tcp_payload_len = 0;
	int d_tcp_payload_len = 0;
	int d_eth_tot_len = 0;
	
	char hwAddr[MAC_ADDR_LEN] = {0};
	unsigned char packet[DATA_MAX_PDU] = {0};

	int paddingBytes = 0;
	char padStr[DATA_PADDING_BOUNDARY] = {0};
	unsigned char data[DATA_BUFFER_LENGTH] = {0};

	// ingress packet
	struct ip6_hdr *iph;	
	struct tcphdr *tcph;

	// egress packet
	struct ethhdr *eth_header = (struct ethhdr *)packet;
	struct ip6_hdr *ip6_header = (struct ip6_hdr *)(packet + sizeof(struct ethhdr));
	struct tcphdr *tcp_header = (struct tcphdr *)((unsigned char *)ip6_header + sizeof(struct ip6_hdr));

	hijackBitmap = getSkyHijack(); 	
	sprintf(data,
		"HTTP/1.1 303 See Other \r\nCache-Control: no-cache\r\nConnection: close\r\nLocation: http://[%s]/%s\r\n",
		routerIPv6Address,
		hijackBitmap & 0x08 ? "sky_self_heal.html" : "sky_blocked_site.html");

	// payload length for egress tcp header
	d_tcp_payload_len = strlen(data);

	paddingBytes = d_tcp_payload_len % DATA_PADDING_BOUNDARY;
	if(paddingBytes != 0)
	{
		snprintf(padStr, paddingBytes+1, "\n\n\n\n");
		strcat(data, padStr);
	}

	// correct the length taking padding into account
	d_tcp_payload_len = strlen(data);

	// cast ingress data to ip6 header
	iph = (struct ip6_hdr *)datagram;

	if (IPPROTO_TCP != iph->ip6_nxt) {
		printf ("Error: not a tcp segment, aborting");
		return;
	}

	// cast ingress ip6 payload to tcp header
	tcph = (struct tcphdr *)(datagram + (sizeof(struct ip6_hdr)));	// 40 bytes ipv6 constant offset

	// calculate data length in ingress packet	
	s_tcp_payload_len = (unsigned int)ntohs(iph->ip6_plen) - GET_HEADER_LEN_TCP(tcph);

	if((raw = CreateAndBindRawSocketToInterface(HIJACK_INTERFACE)) == -1)
	{
#ifdef UFD_DEBUG
		printf(" Error Creating And Binding RAW socket, Hijacking Functionality will not be used,  all packets will be dropped without notification \n");
#endif
		return;
	}

	/* create Ethernet header */
	GetInterfaceHwAddr(HIJACK_INTERFACE, hwAddr);
	CreateEthernetHeader(eth_header, hwAddr, dest_mac, ETHERTYPE_IPV6);
	
	/* Create IP Header */
	CreateIPv6Header(ip6_header, iph, d_tcp_payload_len);

	/* Create TCP Header */
	CreateTcpHeader(tcp_header, tcph, s_tcp_payload_len);

	/* Create PseudoHeader and compute TCP Checksum  */
	CreatePseudoHeaderv6AndComputeTcpChecksum(tcp_header, ip6_header, data, d_tcp_payload_len);

	/* Packet length = ETH + IP header + TCP header + Data*/
	d_eth_tot_len = sizeof(struct ethhdr) + ntohs(ip6_header->ip6_plen) + 40;

	if (d_eth_tot_len > DATA_MAX_PDU) {
		// very unlikely
#ifdef UFD_DEBUG
		fprintf(stderr, "no memory to assemble redirection packet");
#endif
		return;
	}

	/* Copy the Data after the TCP header */
	memcpy(((unsigned char *)tcp_header + sizeof(struct tcphdr)), data, d_tcp_payload_len);

	/* send the packet on the wire */
	if(!SendRawPacket(raw, packet, d_eth_tot_len))
	{
#ifdef UFD_DEBUG
		perror("Error sending packet");
#endif
	}

	close(raw);

}

#endif

#undef DATA_BUFFER_LENGTH
#define DATA_BUFFER_LENGTH 128

void inject_http_hijack_reply(char *dest_mac, char *datagram)
{
	char hijackBitmap = 0;
	int raw;

	unsigned int s_ip_tot_len = 0;
	unsigned int s_ip_hdr_len = 0;
	unsigned int s_tcp_hdr_len = 0;
	unsigned int d_eth_tot_len = 0;

	unsigned int d_ip_tot_len = 0; // pkt_len
	unsigned int d_tcp_payload_len = 0;
	unsigned int s_tcp_payload_len = 0;

	int paddingBytes = 0;
	char padStr[DATA_PADDING_BOUNDARY] = {0};
	unsigned char data[DATA_BUFFER_LENGTH] = {0};

	char hwAddr[MAC_ADDR_LEN] = {0};
	unsigned char packet[DATA_MAX_PDU] = {0};

	// ingress packet
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;

	// egress packet
	struct ethhdr *eth_header = (struct ethhdr *)packet;
	struct iphdr *ip_header = (struct iphdr *)(packet + sizeof(struct ethhdr));

	// it's safe to do that since in our case ip header will have constant length
	struct tcphdr *tcp_header = (struct tcphdr *)((unsigned char *)ip_header + sizeof(struct iphdr));

	/* update hijack bit map */
	hijackBitmap = getSkyHijack(); 	

	/* If self heal hijack page is active we will send sel heal page */
	snprintf(data, 
		DATA_BUFFER_LENGTH, 
		"HTTP/1.1 303 See Other \r\nCache-Control: no-cache\r\nConnection: close\r\nLocation: http://%s/%s\r\n",
		routerIPAddress, 
		hijackBitmap & 0x08 ? "sky_self_heal.html" : "sky_blocked_site.html");

	// payload length for egress tcp header
	d_tcp_payload_len = strlen(data);

	paddingBytes = d_tcp_payload_len % DATA_PADDING_BOUNDARY;
	if(paddingBytes != 0)
	{
		snprintf(padStr, paddingBytes+1, "\n\n\n\n");
		strcat(data, padStr);
	}
	
	// correct the length taking padding into account
	d_tcp_payload_len = strlen(data);

	// cast ingress data to ip header
	iph = (struct iphdr *) datagram;
	s_ip_tot_len = (unsigned int)ntohs(iph->tot_len);
	s_ip_hdr_len = (unsigned int)GET_HEADER_LEN_IP(iph);

	if (IPPROTO_TCP != iph->protocol) {
#ifdef UFD_DEBUG
		fprintf(stderr, "Error: not a tcp segment, aborting");
#endif
		return;
	}

	// cast ingress ip payload to tcp header
	tcph = (struct tcphdr *)(datagram + s_ip_hdr_len);
	s_tcp_hdr_len = (unsigned int)GET_HEADER_LEN_TCP(tcph);

	// calculate data length in ingress packet	
	s_tcp_payload_len =  s_ip_tot_len - s_ip_hdr_len - s_tcp_hdr_len;
	
	if((raw = CreateAndBindRawSocketToInterface(HIJACK_INTERFACE)) == -1)
	{
#ifdef UFD_DEBUG
		fprintf(stderr, "Error Creating And Binding RAW socket, Hijacking Functionality will not be used,  all packets will be dropped without notification \n");
#endif
		return;
	}

	// clear the memory
	memset((void *)packet, 0x00, sizeof(packet));
	
	/* create Ethernet header */
	GetInterfaceHwAddr(HIJACK_INTERFACE, hwAddr);
	CreateEthernetHeader(eth_header, hwAddr, dest_mac, ETHERTYPE_IP);

	/* Create IP Header */
	CreateIPHeader(ip_header, iph, d_tcp_payload_len);

	/* Create TCP Header */
	CreateTcpHeader(tcp_header, tcph, s_tcp_payload_len);

	/* Create PseudoHeader and compute TCP Checksum  */
	CreatePseudoHeaderAndComputeTcpChecksum(tcp_header, ip_header, data, d_tcp_payload_len);

	/* Packet length = ETH + IP header + TCP header + Data*/
	d_ip_tot_len = ntohs(ip_header->tot_len);
	d_eth_tot_len = sizeof(struct ethhdr) + d_ip_tot_len;

	if (d_eth_tot_len > DATA_MAX_PDU) {
#ifdef UFD_DEBUG
		fprintf(stderr, "No memory to assemble the redirection packet");
#endif
		return;

	}

	/* Copy the Data after the TCP header */
	memcpy(((unsigned char *)tcp_header + sizeof(struct tcphdr)), data, d_tcp_payload_len);

	/* send the packet on the wire */
	if(!SendRawPacket(raw, packet, d_eth_tot_len))
	{
		perror("Error sending packet");
	}

	close(raw);

}
