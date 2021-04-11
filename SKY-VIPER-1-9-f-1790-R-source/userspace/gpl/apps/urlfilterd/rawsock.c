#include <stdio.h>
#include "rawsock.h"
#include <unistd.h>

int CreateRawSocket(int protocol_to_sniff)
{
	int rawsock;

	if((rawsock = socket(PF_PACKET, SOCK_RAW, htons(protocol_to_sniff)))== -1)
	{
#ifdef UFD_DEBUG
		perror("Error creating raw socket: ");
#endif
		return -1;
	}

	return rawsock;
}

int BindRawSocketToInterface(char *device, int rawsock, int protocol)
{
	
	struct sockaddr_ll sll;
	struct ifreq ifr;

	bzero(&sll, sizeof(sll));
	bzero(&ifr, sizeof(ifr));
	
	/* First Get the Interface Index  */


	strncpy((char *)ifr.ifr_name, device, IFNAMSIZ);
	if((ioctl(rawsock, SIOCGIFINDEX, &ifr)) == -1)
	{
#ifdef UFD_DEBUG
		fprintf(stderr,"Error getting Interface index !\n");
#endif
		return -1;
	}

	/* Bind our raw socket to this interface */

	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifr.ifr_ifindex;
	sll.sll_protocol = htons(protocol); 


	if((bind(rawsock, (struct sockaddr *)&sll, sizeof(sll)))== -1)
	{
#ifdef UFD_DEBUG
		perror("Error binding raw socket to interface\n");
#endif
		return -1;
	}

	return 1;
	
}

int CreateAndBindRawSocketToInterface(char *device)
{
	int rawsock, SockFD;
	struct sockaddr_ll sll;
	struct ifreq ifr, ifhwaddr;

	bzero(&sll, sizeof(sll));
	bzero(&ifr, sizeof(ifr));
	bzero(&ifhwaddr, sizeof(ifhwaddr));
	
	if((rawsock = socket(PF_PACKET, SOCK_RAW, ETH_P_ALL))== -1)
	{
#ifdef UFD_DEBUG
		perror("Error creating raw socket: ");
#endif
		return -1;
	}
	
	strncpy((char *)ifr.ifr_name, device, IFNAMSIZ);
	if((ioctl(rawsock, SIOCGIFINDEX, &ifr)) == -1)
	{
#ifdef UFD_DEBUG
		fprintf(stderr,"Error getting Interface index !\n");
#endif
		return -1;
	}
	
	/* Bind our raw socket to this interface */

	sll.sll_family = AF_PACKET;
	sll.sll_ifindex = ifr.ifr_ifindex;
	sll.sll_protocol = htons(ETH_P_ALL); 

	
	if((bind(rawsock, (struct sockaddr *)&sll, sizeof(sll)))== -1)
	{
#ifdef UFD_DEBUG
		perror("Error binding raw socket to interface\n");
#endif
		return -1;
	}

	return rawsock;
}


void GetInterfaceHwAddr(const char *ifName, char *hwAddr)
{
	int  socketFd;
	struct ifreq intf;

	if (NULL == hwAddr) 
	{
#ifdef UFD_DEBUG
		fprintf(stderr, "Output pointer is null. No memory to place the result in\n");
#endif
		return;
	}
	
	if (ifName == NULL)
	{
		return;
	}

	memset(hwAddr, 0, MAC_ADDR_LEN);
	if ((socketFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
#ifdef UFD_DEBUG
		fprintf(stderr,"could not open socket");
#endif
		return;
	}

	strcpy(intf.ifr_name, ifName);
	if (ioctl(socketFd, SIOCGIFHWADDR, &intf) != -1)
	{
		sprintf(hwAddr, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
			(unsigned char)intf.ifr_hwaddr.sa_data[0],
			(unsigned char)intf.ifr_hwaddr.sa_data[1],
			(unsigned char)intf.ifr_hwaddr.sa_data[2],
			(unsigned char)intf.ifr_hwaddr.sa_data[3],
			(unsigned char)intf.ifr_hwaddr.sa_data[4],
			(unsigned char)intf.ifr_hwaddr.sa_data[5]);
	}
	else
	{
#ifdef UFD_DEBUG
		fprintf(stderr,"Error getting Interface index !, cant get our MAC \n");
#endif
	}

#ifdef UFD_DEBUG
	printf("OUR MAC Address = %s \n", hwAddr);
#endif
	close(socketFd);
}



int SendRawPacket(int rawsock, unsigned char *pkt, int pkt_len)
{
	int sent= 0;

	/* A simple write on the socket ..thats all it takes ! */

	if((sent = write(rawsock, pkt, pkt_len)) != pkt_len)
	{
		/* Error */
#ifdef UFD_DEBUG
		fprintf(stderr,"Could only send %d bytes of packet of length %d\n", sent, pkt_len);
#endif
		return 0;
	}

	return 1;
}
