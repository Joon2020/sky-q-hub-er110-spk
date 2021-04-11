#ifndef __RAWSOCK_H__
#define __RAWSOCK_H__

#include<sys/socket.h>
#include<sys/ioctl.h>

#include<net/if.h>
#include<net/ethernet.h>
#include<linux/if_packet.h>

#include<string.h>
#include<errno.h>

#include "common.h"

int CreateRawSocket(int protocol_to_sniff);
int BindRawSocketToInterface(char *device, int rawsock, int protocol);
int CreateAndBindRawSocketToInterface(char *device);
int SendRawPacket(int rawsock, unsigned char *pkt, int pkt_len);
void GetInterfaceHwAddr(const char *ifName, char *hwAddr);

#endif /* __RAWSOCK_H__ */
