/*****************************************************************************
* Copyright,
*
* FILE:sky_streamDns.c
* PROJECT:
* MODULE:
* Date Created:26/03/2013
* Description:TCP DNS support as a fall back to UDP.
*
* Notes:
*
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <linux/sockios.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdarg.h>
#include <signal.h>
#include <syslog.h>
#include <linux/if.h>
#include "dproxy.h"
#include "dns_decode.h"
#include "conf.h"
#include "dns_list.h"
#include "dns_construct.h"
#include "dns_io.h"
#include "dns_dyn_cache.h"
#include "dns_mapping.h"
#include "prctl.h"
#include <pthread.h>
#include <sky_streamDns.h>


extern pthread_mutex_t  streamMutexProtect;  
int dns_querysock_client[2]={-1, -1};
int dns_querysock_stream[2]={-1, -1};
pthread_t streamThread;
char streamdns1[INET6_ADDRSTRLEN];
int clientFd;

/* Only interface functions added prefix as Sky rest are internal functions */
/************************ Internal functions *************************************************/
static void * handleStreamConnections (void *param);
static int createDNSStreamSocket(int proto);
static int dns_write_stream_packet(int sock, struct sockaddr_storage *sa, dns_request_t *m);
static int writeErrorReplyToStream(int socket, dns_request_t *ptr);
static int dns_read_stream_packet(int sock, dns_request_t *m);

/**********************************************************************************************
* NAME: skyHandleStreamSocket()
*
* Description: This function will create the TCp sockets for handling DNS TCP requests.
*
* INPUT:None
* OUTPUT:None
*
* RETURN:
*	0 on success/ -1 on failure
*
* ADDITIONAL NOTES:
*
**********************************************************************************************/

int sky_handleStreamSocket()
{
	struct addrinfo hintsStream, *res, *p;
	int errcode;
	int ret, i, on=1;


	/* This is for LAN side STREAM socket creation as a fallback for UDP DNS
	   dns_querysock_client[0] for IPV4
	   dns_querysock_client[1] for IPV6
	 */
	memset(&hintsStream, 0, sizeof (hintsStream));
	hintsStream.ai_family = AF_UNSPEC;
	hintsStream.ai_socktype = SOCK_STREAM;
	hintsStream.ai_flags = AI_PASSIVE;
	errcode = getaddrinfo(NULL, PORT_STR, &hintsStream, &res);/* Get address info for port 53 */
	if (errcode != 0)
	{
		debug("gai err %d %s", errcode, gai_strerror(errcode));
		return -1;
	}

	p = res;
	while (p)
	{
		if ( p->ai_family == AF_INET )   i = 0;
		else if ( p->ai_family == AF_INET6 )   i = 1;
		else
		{
			debug_perror("Unknown protocol!!\n");
			goto next_stream_lan;
		}

		dns_querysock_client[i] = socket(p->ai_family, p->ai_socktype, 
				p->ai_protocol);

		if (dns_querysock_client[i] < 0)
		{
			debug("Could not create dns_querysock_client[%d]", i);
			goto next_stream_lan;
		}

#ifdef IPV6_V6ONLY
		if ( (p->ai_family == AF_INET6) && 
				(setsockopt(dns_querysock_client[i], IPPROTO_IPV6, IPV6_V6ONLY, 
					    &on, sizeof(on)) < 0) )
		{
			debug_perror("Could not set IPv6 only option for WAN");
			close(dns_querysock_client[i]);
			goto next_stream_lan;
		}
#endif

		/* bind() the socket to the interface */
		if (bind(dns_querysock_client[i], p->ai_addr, p->ai_addrlen) < 0){
			debug("dns_init: bind: dns_querysock_client[%d] can't bind to port", i);
			close(dns_querysock_client[i]);
		}

next_stream_lan:
		p = p->ai_next;
	}

	freeaddrinfo(res);

	if ( (dns_querysock_client[0] < 0) && (dns_querysock_client[1] < 0) )
	{
		debug_perror("Cannot create sockets for LAN");
		return -1;
	}

	if( (ret = pthread_create(&streamThread,NULL,handleStreamConnections, NULL)) != 0)
	{
		perror("DNSPROXY: Stream thread cannot be created");
		return -1;
	}

	return 0;
}

/**********************************************************************************************
* NAME: handleStreamConnections (void *param)
*
* Description: TCP DNS handler thread function.
*
* INPUT:Thread parameter (Not passing anything now)
* OUTPUT:None
*
* RETURN:
*
* ADDITIONAL NOTES:
*
**********************************************************************************************/

static void * handleStreamConnections (void *param)
{  

	fd_set streamrfds;
	fd_set read_fd_set;
	int selectRetVal = 0;
	dns_request_t m;
	struct sockaddr_in client_address;
	socklen_t client_len;
	int streamClientFD = -1;
	struct timeval tv;
	int proto;
	int query_type = 0;

	if(dns_querysock_client[0] > 0)
	{
		if (listen (dns_querysock_client[0], 1) < 0) /* Listen client side DNS proxy socket*/
		{
			perror ("listen");
			exit (1);
		}
	}
	if(dns_querysock_client[1] > 0)
	{
		if (listen (dns_querysock_client[1], 1) < 0) /* Listen client side DNS proxy socket*/
		{
			perror ("listen");
			exit (1);
		}
	}

	FD_ZERO( &streamrfds );
	if(dns_querysock_client[0] > 0)
		FD_SET( dns_querysock_client[0], &streamrfds );/* Add client side socket to select to avoid polling */
	if(dns_querysock_client[1] > 0)
		FD_SET( dns_querysock_client[1], &streamrfds );/* Add client side socket to select to avoid polling */
	read_fd_set = streamrfds;
	
/* 
TODO: for IPV6 we need to relook into this implementation since client can request IPV6 address(Name resolution) over IPV4 which can result in
opening IPV6 connection to WAN side DNS server.
*/
	while(TRUE)
	{

		selectRetVal = select( FD_SETSIZE, &read_fd_set, NULL, NULL, NULL ); /* Wait in select to read the data */
		if(selectRetVal)
		{
			if ((dns_querysock_client[0] > 0) && FD_ISSET(dns_querysock_client[0], &read_fd_set)) 
			{
				memset(&m,0,sizeof(m));
				memset(&client_address,0,sizeof(client_address));
				client_len = sizeof(client_address);
				streamClientFD =  accept(dns_querysock_client[0],(struct sockaddr *) &client_address, &client_len);
				if( dns_read_stream_packet(streamClientFD, &m)  > 0)
				{
					//debug("received stream DNS message (LAN side)");
					query_type = m.message.question[0].type;
					pthread_mutex_lock(&streamMutexProtect);
					if (dns_mapping_find_dns_ip(&(m.src_info), query_type, streamdns1, &proto) == TRUE)
					{
						pthread_mutex_unlock(&streamMutexProtect);
						if(!createDNSStreamSocket(AF_INET))
						{
							if(dns_write_stream_packet(dns_querysock_stream[0], &m.src_info, &m) > 0)
							{
								tv.tv_sec = 5;  /* 30 Secs Timeout */
								tv.tv_usec = 0;  // Not init'ing this can cause strange errors
								if( setsockopt(dns_querysock_stream[0], SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval)) < 0) /* set the socket recieve timeout to 5 seconds */
								{
									perror("DNSPROXY: setsockopt failed for wan socket");
								}
								if (dns_read_stream_packet(dns_querysock_stream[0], &m) > 0 )
								{
									dns_write_stream_packet(streamClientFD, &m.src_info, &m);
								}
								else
								{ /* Time out,Some thing happened at WAN side ..don't know write error to client,Let him request again. */
									printf("DNSPROXY:WAN data arrival timed out\n");
									writeErrorReplyToStream(streamClientFD, &m); /* This will not happen normally ,UDP to TCP switching happening in milliseconds.*/
								}
							}
							close(dns_querysock_stream[0]);/* close stream socket to WAN side DNS server */
							dns_querysock_stream[0] = -1;
						}
						else
						{
							printf("DNSPROXY:WAN side socket creation failed\n");
							writeErrorReplyToStream(streamClientFD,&m); /* This will not happen normally ,UDP to TCP switching happening in milliseconds.*/
						}
					}
					else
					{
						pthread_mutex_unlock(&streamMutexProtect);
					}
				}

				close(streamClientFD);/* Close connected client from LAN, at any cost this has to be done*/
				streamClientFD = -1;
			} 
			if ((dns_querysock_client[1] > 0) && FD_ISSET(dns_querysock_client[1], &streamrfds)) 
			{
				memset(&m,0,sizeof(m));
				memset(&client_address,0,sizeof(client_address));
				client_len = sizeof(client_address);
				streamClientFD =  accept(dns_querysock_client[1],(struct sockaddr *) &client_address, &client_len);
				if( dns_read_stream_packet(streamClientFD, &m)  > 0)
				{
					//debug("received stream DNS message (LAN side)");
					query_type = m.message.question[0].type;
					pthread_mutex_lock(&streamMutexProtect);
					if (dns_mapping_find_dns_ip(&(m.src_info), query_type, streamdns1, &proto) == TRUE)
					{
						pthread_mutex_unlock(&streamMutexProtect);
						if(!createDNSStreamSocket(AF_INET6))
						{
							if(dns_write_stream_packet(dns_querysock_stream[1], &m.src_info, &m) > 0)
							{
								tv.tv_sec = 5;  /* 5 Secs Timeout */
								tv.tv_usec = 0;  // Not init'ing this can cause strange errors
								if( setsockopt(dns_querysock_stream[1], SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval)) < 0) /* set the socket recieve timeout to 5 seconds */
								{
									perror("DNSPROXY: setsockopt failed for wan socket");
								}
								if (dns_read_stream_packet(dns_querysock_stream[1], &m) > 0 )
								{
									dns_write_stream_packet(streamClientFD, &m.src_info, &m);
								}
								else
								{ /* Time out,Some thing happened at WAN side ..don't know write error to client,Let him request again. */
									printf("DNSPROXY:WAN data arrival timed out\n");
									writeErrorReplyToStream(streamClientFD, &m); /* This will not happen normally ,UDP to TCP switching happening in milliseconds.*/
								}
							}
							close(dns_querysock_stream[1]);/* close stream socket to WAN side DNS server */
							dns_querysock_stream[1] = -1;
						}
						else
						{
							printf("DNSPROXY:WAN side socket creation failed\n");
							writeErrorReplyToStream(streamClientFD,&m); /* This will not happen normally ,UDP to TCP switching happening in milliseconds.*/
						}
					}
					else
					{
						pthread_mutex_unlock(&streamMutexProtect);
					}
				}

				close(streamClientFD);/* Close connected client from LAN, at any cost this has to be done*/
				streamClientFD = -1;

			} 

		} /* indefenite wait no else */
	}

	/*How to reach here */
	if(dns_querysock_client[0] > 0)
	{
		close(dns_querysock_client[0]);
	}

	if(dns_querysock_client[1] > 0)
	{
		close(dns_querysock_client[1]);
	}
	pthread_exit(NULL);
}
/******************************************************************************************
* NAME:  createDNSStreamSocket(int proto)
*
* Description: Create stream socket to WAN DNS server.
*
* INPUT: int proto , protocol ipv4/ipv6
* OUTPUT:None
*
* RETURN:
*		0 on success/ -1 on failure
*
* ADDITIONAL NOTES:
*
*******************************************************************************************/

static int createDNSStreamSocket(int proto)
{

	struct sockaddr_storage serveraddr;
	int sock;

	if(strcmp(streamdns1,"0.0.0.0") == 0)
	{
		return -1; /* it's loop back no need to create WAN socket , normal use case UDP will handle this */
	}

	if (proto == AF_INET)
	{
		dns_querysock_stream[0] = socket(AF_INET, SOCK_STREAM, 0);
		sock = dns_querysock_stream[0];

		((struct sockaddr_in *)&serveraddr)->sin_port = PORT;
		inet_aton(streamdns1,  &(((struct sockaddr_in *)&serveraddr)->sin_addr) );
		serveraddr.ss_family = AF_INET;
	}
	else
	{
		dns_querysock_stream[1] = socket(AF_INET6, SOCK_STREAM, 0);
		sock = dns_querysock_stream[1];

		((struct sockaddr_in6 *)&serveraddr)->sin6_port = PORT;
		inet_pton(AF_INET6,streamdns1, &(((struct sockaddr_in6 *)&serveraddr)->sin6_addr) );
		serveraddr.ss_family = AF_INET6;
	}


	if(sock < 0)
	{
		perror("\nError in opening socket to WAN\n");
		return -1;
	}
	serveraddr.ss_family = proto;


	/*  connect to remote DNS server  */
	if ( connect(sock, (struct sockaddr *) &serveraddr, sizeof(serveraddr) ) < 0 )
	{
		perror("Error calling connect()\n");
		close(sock);
		return -1;
	}

	return 0;
}

/**********************************************************************************************
* NAME: writeErrorReplyToStream()
*
* Description: This function will write back error to client.
*
* INPUT: int socket(Client socket), dns_request_t *ptr
* OUTPUT:None
*
* RETURN:
*	0 on success/ -1 on failure
*
* ADDITIONAL NOTES:
*
**********************************************************************************************/


static int writeErrorReplyToStream(int socket, dns_request_t *ptr)
{

	dns_construct_error_reply(ptr);

	printf("\nDNSPROXY: Writing Error reply to client \n");
	dns_write_stream_packet( socket, &ptr->src_info, ptr );
	return 0;
}

/**********************************************************************************************
* NAME: dns_read_stream_packet(int sock, dns_request_t *m)
*
* Description: This function will read the packet from client socket.
*
* INPUT: int sock(Client socket), dns_request_t *m (DNS request linked list pointer)
* OUTPUT:None
*
* RETURN:
*	0 on success/ -1 on failure
*
* ADDITIONAL NOTES:
*
**********************************************************************************************/


static int dns_read_stream_packet(int sock, dns_request_t *m)
{
  
  m->numread = read(sock, m->original_buf, sizeof(m->original_buf));
  
  if ( m->numread < 0) {
    debug_perror("dns_read_packet: recvfrom\n");
    return -1;
  }
  
  return m->numread;
}

/**********************************************************************************************
* NAME: dns_write_stream_packet()
*
* Description: This function will write back the reply to the client.
*
* INPUT:int sock (Client socket),  struct sockaddr_storage *sa (Client socket address details)
*					dns_request_t *m (linked list pointer of DNS request)
* OUTPUT:None
*
* RETURN:
*	0 on success/ -1 on failure
*
* ADDITIONAL NOTES:
*
**********************************************************************************************/


static int dns_write_stream_packet(int sock, struct sockaddr_storage *sa, dns_request_t *m)
{
  int retval;
  char dstIp[INET6_ADDRSTRLEN];

  inet_ntop(sa->ss_family, get_in_addr(sa), dstIp, INET6_ADDRSTRLEN);

  if (sa->ss_family == AF_INET)
  {
    retval = write(sock, m->original_buf, m->numread); 
  }
  else
  {
    retval = write(sock, m->original_buf, m->numread);
  }
  
  if( retval < 0 )
  {
    perror("dns_write_packet: sendto");
  }
	
  return retval;
}

/**********************************************************************************************
* NAME: skyCancelStreamDNSHandle()
*
* Description: This function can be used from outside to cancel the currently running thread.
*
* INPUT:None
* OUTPUT:None
*
* RETURN:
*	0 on success/ -1 on failure
*
* ADDITIONAL NOTES:
*
**********************************************************************************************/


int sky_cancelStreamDNSHandle()
{
	void *threadRetVal;
	int ret = -1;

	if( pthread_cancel(streamThread) != 0)
	{
		perror("Stream thread cancel failed");
		ret = -1;
	}
	else
	{
		pthread_join(streamThread, &threadRetVal); /* Wait to join the stream handle connection thread, default thread creation is joinable mode*/
		ret = 0;
	}
	return ret;	
}
