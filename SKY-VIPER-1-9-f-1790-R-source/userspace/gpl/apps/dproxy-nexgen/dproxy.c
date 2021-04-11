/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

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

/*****************************************************************************/
/*****************************************************************************/
int dns_main_quit;
int dns_sock[2]={-1, -1};
fd_set rfds;

//BRCM
int dns_querysock[2]={-1, -1};
#ifdef DMP_X_ITU_ORG_GPON_1
static unsigned int dns_query_error = 0;
#endif

/* CMS message handle */
void *msgHandle=NULL;
int msg_fd;

#ifdef SKY
pthread_mutex_t  streamMutexProtect = PTHREAD_MUTEX_INITIALIZER;  
char streamdns1[INET6_ADDRSTRLEN];
struct sockaddr_in sa_stream;
int clientFd;
char hijackBitmap = 0;
int compare = 0;
char routerIPAddress[16]="192.168.0.1"; //Assign default value for safety
const char* LOOPBACK_ADDRESS = "127.0.0.1"; //loopback address for IPv4
const char* LOOPBACK_ADDRESS_IPV6 = "::1"; //loopback address for IPv6
pthread_t streatThreadId;


void * handleStreamConnections (void *);
int createDNSStreamSocket(int proto);
int writeErrorReplyToStream(int socket, dns_request_t *ptr);
static void dns_get_skyhijack();
static void getRouterIPAddress();

#ifdef SKY_IPV6
#define ROUTER_IPV6_ADDR_SIZE 48
char routerIPv6Address[ROUTER_IPV6_ADDR_SIZE] = {0};
char routerIPv6LinkAddr[ROUTER_IPV6_ADDR_SIZE] = {0};

static void getRouterIPv6ULA();
static void getRouterIPv6LinkAddr();
#endif

static UBOOL8 isRouterLocalIpAddress(char *srcAddr);

#endif /* SKY */

/*****************************************************************************/
int dns_init()
{
  struct addrinfo hints, *res, *p;
  int errcode;
  int ret, i, on=1;

  memset(&hints, 0, sizeof (hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  /*
   * BRCM:
   * Open sockets to receive DNS queries from LAN.
   * dns_sock[0] is used for DNS queries over IPv4
   * dns_sock[1] is used for DNS queries over IPv6
   */
  errcode = getaddrinfo(NULL, PORT_STR, &hints, &res);
  if (errcode != 0)
  {
     debug("gai err %d %s", errcode, gai_strerror(errcode));
     exit(1) ;
  }

  p = res;
  while (p)
  {
     if ( p->ai_family == AF_INET )   i = 0;
     else if ( p->ai_family == AF_INET6 )   i = 1;
     else {
        debug_perror("Unknown protocol!!\n");
        goto next_lan;
     }

     dns_sock[i] = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

     if (dns_sock[i] < 0)
     {
        debug("Could not create dns_sock[%d]", i);
        goto next_lan;
     }

#ifdef IPV6_V6ONLY
     if ( (p->ai_family == AF_INET6) && 
          (setsockopt(dns_sock[i], IPPROTO_IPV6, IPV6_V6ONLY, 
                      &on, sizeof(on)) < 0) )
     {
        debug_perror("Could not set IPv6 only option for LAN");
        close(dns_sock[i]);
        goto next_lan;
     }
#endif

     /* bind() the socket to the interface */
     if (bind(dns_sock[i], p->ai_addr, p->ai_addrlen) < 0){
        debug("dns_init: bind: dns_sock[%d] can't bind to port", i);
        close(dns_sock[i]);
     }

next_lan:
     p = p->ai_next;
  }

  freeaddrinfo(res);

  if ( (dns_sock[0] < 0) && (dns_sock[1] < 0) )
  {
     debug_perror("Cannot create sockets for LAN");
     exit(1) ;
  }

  /*
   * BRCM:
   * use different sockets to send queries to WAN so we can use ephemeral port
   * dns_querysock[0] is used for DNS queries sent over IPv4
   * dns_querysock[1] is used for DNS queries sent over IPv6
   */
  errcode = getaddrinfo(NULL, "0", &hints, &res);
  if (errcode != 0)
  {
     debug("gai err %d %s", errcode, gai_strerror(errcode));
     exit(1) ;
  }

  p = res;
  while (p)
  {
     if ( p->ai_family == AF_INET )   i = 0;
     else if ( p->ai_family == AF_INET6 )   i = 1;
     else
     {
        debug_perror("Unknown protocol!!\n");
        goto next_wan;
     }

     dns_querysock[i] = socket(p->ai_family, p->ai_socktype, 
                               p->ai_protocol);

     if (dns_querysock[i] < 0)
     {
        debug("Could not create dns_querysock[%d]", i);
        goto next_wan;
     }

#ifdef IPV6_V6ONLY
     if ( (p->ai_family == AF_INET6) && 
          (setsockopt(dns_querysock[i], IPPROTO_IPV6, IPV6_V6ONLY, 
                     &on, sizeof(on)) < 0) )
     {
        debug_perror("Could not set IPv6 only option for WAN");
        close(dns_querysock[i]);
        goto next_wan;
     }
#endif

     /* bind() the socket to the interface */
     if (bind(dns_querysock[i], p->ai_addr, p->ai_addrlen) < 0){
        debug("dns_init: bind: dns_querysock[%d] can't bind to port", i);
        close(dns_querysock[i]);
     }

next_wan:
     p = p->ai_next;
  }

  freeaddrinfo(res);

  if ( (dns_querysock[0] < 0) && (dns_querysock[1] < 0) )
  {
     debug_perror("Cannot create sockets for WAN");
     exit(1) ;
  }

  // BRCM: Connect to ssk
  if ((ret = cmsMsg_initWithFlags(EID_DNSPROXY, 0, &msgHandle)) != CMSRET_SUCCESS) {
  	debug_perror("dns_init: cmsMsg_init() failed");
	exit(1);
  }
  cmsMsg_getEventHandle(msgHandle, &msg_fd);

  dns_main_quit = 0;

  FD_ZERO( &rfds );
  if (dns_sock[0] > 0)
     FD_SET( dns_sock[0], &rfds );
  if (dns_sock[1] > 0)  
     FD_SET( dns_sock[1], &rfds );
  if (dns_querysock[0] > 0)  
     FD_SET( dns_querysock[0], &rfds );
  if (dns_querysock[1] > 0)  
     FD_SET( dns_querysock[1], &rfds );

  FD_SET( msg_fd, &rfds );

#ifdef DNS_DYN_CACHE
  dns_dyn_hosts_add();
#endif

#ifdef SKY 
  dns_get_skyhijack();
  //printf(" DNS hijack bitmap = %d \n", hijackBitmap);
   getRouterIPAddress();  

#ifdef SKY_IPV6
	getRouterIPv6ULA();
#endif

   /*Check the router ip so that the no connection hijack logic can return
    an ip in a different private subnet range. If the router doesn't use the 
    range starting with 10, we can use that.*/
  compare = strncmp(routerIPAddress, "10", 2 );
#endif   /* SKY */
  return 1;
}


/*****************************************************************************/
void dns_handle_new_query(dns_request_t *m)
{
   int retval = -1;
   dns_dyn_list_t *dns_entry;
   char srcIP[INET6_ADDRSTRLEN];
   int query_type = m->message.question[0].type;

   if( query_type == A )
   {
      retval = 0;         
#ifdef DNS_DYN_CACHE
      if ((dns_entry = dns_dyn_find(m))) 
      {
         strcpy(m->ip, inet_ntoa(dns_entry->addr));
         m->ttl = abs(dns_entry->expiry - time(NULL));
         retval = 1;
         debug(".... %s, srcPort=%d, retval=%d\n", m->cname, ((struct sockaddr_in *)&m->src_info)->sin_port, retval); 
      }
#endif      
   }
   else if( query_type == AAA)
   { /* IPv6 todo */
		retval = 0;
#ifdef SKY_IPV6
	  	// check if the query name is for sky router 
		if(m->cname && ((strcasecmp(m->cname, "SkyRouter") == 0 ) || (strcasecmp(m->cname, "SkyRouter.Home") == 0)))
		{
			if(routerIPv6Address[0] != '\0')
			{
				strcpy(m->ip, routerIPv6Address);
		   	    m->ttl = 10000; // give large value
    	    	retval = 1;
			}
			else
			{
				printf("DNS query is for %s ,  rotuer ula is null \n",m->cname);
				retval = 2; //ignore the request
			}
		}
#endif // SKY_IPV6	
   }
   else if( query_type == PTR ) 
   {
      retval = 0;   
      /* reverse lookup */
#ifdef DNS_DYN_CACHE
      if ((dns_entry = dns_dyn_find(m))) 
      {
         strcpy(m->cname, dns_entry->cname);
         m->ttl = abs(dns_entry->expiry - time(NULL));
         retval = 1;
         debug("TYPE==PTR.... %s, srcPort=%d, retval=%d\n", m->cname,  ((struct sockaddr_in *)&m->src_info)->sin_port, retval); 
         
      }
#endif
   }  
   else /* BRCM:  For all other query_type including TXT, SPF... */
   {
       retval = 0;
   }
   
   inet_ntop( m->src_info.ss_family, 
              get_in_addr(&m->src_info), 
              srcIP, INET6_ADDRSTRLEN );
   cmsLog_notice("dns_handle_new_query: %s %s, srcPort=%d, retval=%d\n", 
                  m->cname, srcIP, 
                  ((struct sockaddr_in *)&m->src_info)->sin_port, 
                  retval);
   
  switch( retval )
  {
     case 0:
     {
        char dns1[INET6_ADDRSTRLEN];
        int proto;

		//			printf(" DNS hijack bitmap set to %d \n", hijackBitmap);
				 
#ifdef SKY  
		if(((hijackBitmap & 0x01)!= 0)  && (!isRouterLocalIpAddress(srcIP))) /* Murthy Hijack only when No Connection Hijack is set */
		{
			if (m->src_info.ss_family == AF_INET)
			{
				compare ?  strcpy(m->ip, "10.10.10.10"):  strcpy(m->ip, "172.16.10.10");
				m->ttl = 1;
				dns_construct_reply( m );
				dns_write_packet( dns_sock[0], &m->src_info, m );
				break;
			}		
			if (m->src_info.ss_family == AF_INET6)
			{
				// use any dummy IPV6 address
				/*
					SV-615: 8.3.3.3CPEs shall act as a DNS server for IPv6-capable LAN devices by supporting IPv6 (AAAA) records in 
					its DNS server (RFC3596) and allowing these records to be queried using either IPv4 or IPv6 transport (RFC3901).
					IPV6 will try to send solicit message to IP address before using it for HTTP.
				*/
#ifdef SKY_IPV6
				if(query_type == A) /* We have seen some devices still requesting A record through IPV6 connection , if you reply
							with an IPV6 address browser rejects it, in some cases it will then fall back to AAAA record.*/
				{
					compare ?  strcpy(m->ip, "10.10.10.10"):  strcpy(m->ip, "172.16.10.10");
				}
				else //AAAA
				{
					getRouterIPv6LinkAddr(); /* return link local address*/
					//strcpy(m->ip, "fc00::1");
					strcpy(m->ip, routerIPv6LinkAddr);
				}
#endif
				m->ttl = 100;
				dns_construct_reply( m );
				dns_write_packet( dns_sock[1], &m->src_info, m );
				break;
			}
		}
#endif /* SKY */
        if (dns_mapping_find_dns_ip(&(m->src_info), query_type, 
                                    dns1, &proto) == TRUE)
        {
           struct sockaddr_storage sa;
           int sock;

           cmsLog_notice("Found dns %s for subnet %s", dns1, srcIP);
           dns_list_add(m);

           if (proto == AF_INET)
           {
              sock = dns_querysock[0];
              ((struct sockaddr_in *)&sa)->sin_port = htons(PORT);
              inet_pton(proto, dns1, &(((struct sockaddr_in *)&sa)->sin_addr));
           }
           else
           {
              sock = dns_querysock[1];
              ((struct sockaddr_in6 *)&sa)->sin6_port = htons(PORT);
              inet_pton( proto, dns1, 
                         &(((struct sockaddr_in6 *)&sa)->sin6_addr) );
           }
           sa.ss_family = proto;
           dns_write_packet( sock, &sa, m );
        }
        else
        {
            cmsLog_debug("No dns service.");
        }   
        break;
     }

     case 1:
        dns_construct_reply( m );

		/*  Murthy: 12/06/2013
			differentiate between IPV6 and IPV4 soource and use the corresponding socket
		*/
		if (m->src_info.ss_family == AF_INET6)
		{
			debug("address family is AF_INET6");
	        dns_write_packet(dns_sock[1], &m->src_info, m);
		}
	 	else if (m->src_info.ss_family == AF_INET)
	 	{
			debug("address family is AF_INET");	 		
	        dns_write_packet(dns_sock[0], &m->src_info, m);
	 	}
		else
			debug("address family is UNKNOWN");
        break;

     default:
        debug("Unknown query type: %d\n", query_type);
   }
}
/*****************************************************************************/
void dns_handle_request(dns_request_t *m)
{
  dns_request_t *ptr = NULL;

  /* request may be a new query or a answer from the upstream server */
  ptr = dns_list_find_by_id(m);

  if( ptr != NULL ){
      debug("Found query in list; id 0x%04x flags 0x%04x\n ip %s - cname %s\n", 
             m->message.header.id, m->message.header.flags.flags, 
             m->ip, m->cname);

      /* message may be a response */
      if( m->message.header.flags.flags & 0x8000)
      {
          int sock;
          char srcIP[INET6_ADDRSTRLEN];

          if (ptr->src_info.ss_family == AF_INET)
             sock = dns_sock[0];
          else
             sock = dns_sock[1];

          inet_ntop( m->src_info.ss_family, 
                     get_in_addr(&m->src_info), 
                     srcIP, INET6_ADDRSTRLEN );
   
          debug("Replying with answer from %s, id 0x%04x\n", 
                 srcIP, m->message.header.id);
          dns_write_packet( sock, &ptr->src_info, m );
          dns_list_remove( ptr );         
          
#ifdef DMP_X_ITU_ORG_GPON_1
          if( m->message.header.flags.f.rcode != 0 ){ // lookup failed
              dns_query_error++;
              debug("dns_query_error = %u\n", dns_query_error);
          }
#endif

#ifdef DNS_DYN_CACHE
          if( m->message.question[0].type != AAA) /* No cache for IPv6 */
          {
#ifndef SKY
          	/* Sky:  we will not cache any DNS responses . 
          	we use DNS_DYN_CACHE only to hold  lists of hosts in the LAN network
          	which is populated using /var/hosts file
          	 */
             dns_dyn_cache_add(m);
#endif /* SKY */
          }
#endif
      }
      else
      {
         debug( "Duplicate query to %s (retx_count=%d)", 
                m->cname, ptr->retx_count );

         if (ptr->retx_count < MAX_RETX_COUNT)
         {
            char dns1[INET6_ADDRSTRLEN];
            int proto;

            debug("=>send again\n");
            ptr->retx_count++;
            cmsLog_debug("retx_count=%d", ptr->retx_count);

            if (dns_mapping_find_dns_ip(&(m->src_info), 
                                        m->message.question[0].type, 
                                        dns1, &proto) == TRUE)
            {
               struct sockaddr_storage sa;
               int sock;

               if (proto == AF_INET)
               {
                  sock = dns_querysock[0];
                  ((struct sockaddr_in *)&sa)->sin_port = PORT;
                  inet_pton( proto, dns1, 
                             &(((struct sockaddr_in *)&sa)->sin_addr) );
               }
               else
               {
                  sock = dns_querysock[1];
                  ((struct sockaddr_in6 *)&sa)->sin6_port = PORT;
                  inet_pton( proto, dns1, 
                             &(((struct sockaddr_in6 *)&sa)->sin6_addr) );
               }
               sa.ss_family = proto;
               dns_write_packet( sock, &sa, m );
            }
            else
            {
               cmsLog_debug("No dns service for duplicate query??");
            }   
         }
         else
         {
            debug("=>drop! retx limit reached.\n");
         }
      }
  }
  else
  {
      dns_handle_new_query( m );
  }

  debug("dns_handle_request: done\n\n");
}

/*****************************************************************************/
static void processCmsMsg(void)
{
  CmsMsgHeader *msg;
  CmsRet ret;

  while( (ret = cmsMsg_receiveWithTimeout(msgHandle, &msg, 0)) == CMSRET_SUCCESS) {
    switch(msg->type) {
    case CMS_MSG_SET_LOG_LEVEL:
      cmsLog_debug("got set log level to %d", msg->wordData);
      cmsLog_setLevel(msg->wordData);
      if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS) {
        cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
      }
      break;
    case CMS_MSG_SET_LOG_DESTINATION:
      cmsLog_debug("got set log destination to %d", msg->wordData);
      cmsLog_setDestination(msg->wordData);
      if ((ret = cmsMsg_sendReply(msgHandle, msg, CMSRET_SUCCESS)) != CMSRET_SUCCESS) {
        cmsLog_error("send response for msg 0x%x failed, ret=%d", msg->type, ret);
      }
      break;

    case CMS_MSG_DNSPROXY_RELOAD:
      cmsLog_debug("received CMS_MSG_DNSPROXY_RELOAD\n");
#ifdef SKY	  
 	pthread_mutex_lock(&streamMutexProtect);
#endif /* SKY */	
      /* Reload config file */
#ifdef DNS_DYN_CACHE
      dns_dyn_hosts_add();
#endif

      /* load the /var/dnsinfo.conf into the linked list for determining
      * which dns ip to use for the dns query.
      */
      dns_mapping_conifg_init();
     
#ifdef SKY	  
 	pthread_mutex_unlock(&streamMutexProtect);
#endif /* SKY */     
      /*
       * During CDRouter dhcp scaling tests, this message is sent a lot to dnsproxy.
       * To make things more efficient/light weight, the sender of the message does
       * not expect a reply.
       */
#ifdef SKY
	  dns_get_skyhijack();
	 // printf(" DNS hijack bitmap = %d \n", hijackBitmap);
#endif /* SKY */	 
      break;

#ifdef SUPPORT_DEBUG_TOOLS

    case CMS_MSG_DNSPROXY_DUMP_STATUS:
       printf("\n============= Dump dnsproxy status=====\n\n");
       printf("WAN interface; LAN IP/MASK; Primary DNS IP,Secondary DNS IP;\n");
       prctl_runCommandInShellBlocking("cat /var/dnsinfo.conf");
       dns_list_print();
       dns_dyn_print();
       break;

    case CMS_MSG_DNSPROXY_DUMP_STATS:
       printf("stats dump not implemented yet\n");
       break;

#endif /* SUPPORT_DEBUG_TOOLS */

#ifdef DMP_X_ITU_ORG_GPON_1
    case CMS_MSG_DNSPROXY_GET_STATS:
    {
        char buf[sizeof(CmsMsgHeader) + sizeof(DnsGetStatsMsgBody)]={0};
        CmsMsgHeader *replyMsgPtr = (CmsMsgHeader *) buf;
        DnsGetStatsMsgBody *dnsStats = (DnsGetStatsMsgBody *) (replyMsgPtr+1);


        // Setup response message.
        replyMsgPtr->type = msg->type;
        replyMsgPtr->dst = msg->src;
        replyMsgPtr->src = EID_DNSPROXY;
        replyMsgPtr->flags.all = 0;
        replyMsgPtr->flags_response = 1;
        //set dns query error counter 
        replyMsgPtr->dataLength = sizeof(DnsGetStatsMsgBody);
        dnsStats->dnsErrors = dns_query_error;

        cmsLog_notice("dns query error is %d", dns_query_error);
        // Attempt to send CMS response message & test result.
        ret = cmsMsg_send(msgHandle, replyMsgPtr);
        if (ret != CMSRET_SUCCESS)
        {
           // Log error.
           cmsLog_error("Send message failure, cmsReturn: %d", ret);
        }
    }
    break;
#endif

    default:
      cmsLog_error("unrecognized msg 0x%x", msg->type);
      break;
    }
    CMSMEM_FREE_BUF_AND_NULL_PTR(msg);
  }
  
  if (ret == CMSRET_DISCONNECTED) {
    if (!cmsFil_isFilePresent(SMD_SHUTDOWN_IN_PROGRESS)) {
      cmsLog_error("lost connection to smd, exiting now.");
    }
    dns_main_quit = 1;
  }
}

/*****************************************************************************/
int dns_main_loop()
{
    struct timeval tv, *ptv;
    fd_set active_rfds;
    int retval;
    dns_request_t m;
    dns_request_t *ptr, *next;
#ifdef SKY_TBD	
    fd_set dns_server_fds;
    struct timeval dnsTime;
    struct sockaddr_in clientname;
    size_t size;
    int is_stream_request = 0;
#endif /* SKY */

    while( !dns_main_quit )
    {
#ifdef SKY_TBD	
    	is_stream_request = 0;
#endif /* SKY */		

      int next_request_time = dns_list_next_time();
      
      if (next_request_time == 1) {
          tv.tv_sec = next_request_time;
          tv.tv_usec = 0;
          ptv = &tv;
          cmsLog_notice("select timeout = %lu seconds", tv.tv_sec);          
       } else {
         ptv = NULL;
         cmsLog_debug("\n\n =============select will wait indefinitely============");          
       }


      /* copy the main rfds in the active one as it gets modified by select*/
      active_rfds = rfds;
      retval = select( FD_SETSIZE, &active_rfds, NULL, NULL, ptv );

      if (retval){
         debug("received something");

         if (FD_ISSET(msg_fd, &active_rfds)) { /* message from ssk */
            debug("received cms message");
            processCmsMsg();
         } else if ((dns_sock[0] > 0) && FD_ISSET(dns_sock[0], &active_rfds)) {
            debug("received DNS message (LAN side IPv4)");
            /* data is now available */
            bzero(&m, sizeof(dns_request_t));
            //BRCM
            if (dns_read_packet(dns_sock[0], &m) == 0) {
               dns_handle_request( &m );
            }
         } else if ((dns_sock[1] > 0) && FD_ISSET(dns_sock[1], &active_rfds)) {
            debug("received DNS message (LAN side IPv6)");
            /* data is now available */
            bzero(&m, sizeof(dns_request_t));
            //BRCM
            if (dns_read_packet(dns_sock[1], &m) == 0) {
               dns_handle_request( &m );
            }
         } else if ((dns_querysock[0] > 0) && 
                    FD_ISSET(dns_querysock[0], &active_rfds)) {
            debug("received DNS response (WAN side IPv4)");
            bzero(&m, sizeof(dns_request_t));
            if (dns_read_packet(dns_querysock[0], &m) == 0)
               dns_handle_request( &m );
         } else if ((dns_querysock[1] > 0) && 
                    FD_ISSET(dns_querysock[1], &active_rfds)) {
            debug("received DNS response (WAN side IPv6)");
            bzero(&m, sizeof(dns_request_t));
            if (dns_read_packet(dns_querysock[1], &m) == 0)
               dns_handle_request( &m );
         }
      } else { /* select time out */
         time_t now = time(NULL);
         ptr = dns_request_list;
         while (ptr) {
            next = ptr->next;
            if (ptr->expiry <= now) {
               char date_time_buf[DATE_TIME_BUFLEN];
               get_date_time_str(date_time_buf, sizeof(date_time_buf));

               debug("removing expired request %p\n", ptr);
               cmsLog_notice("%s dnsproxy: query for %s timed out after %d secs (type=%d retx_count=%d)",  
                  date_time_buf, ptr->cname, DNS_TIMEOUT, (unsigned int) ptr->message.question[0].type, ptr->retx_count);

               /*  dns1 and dns2 will be swapped if possible in dns_list_remove_related_requests_and_swap call */
               if (dns_list_remove_related_requests_and_swap(ptr)) {
                  /* reset to the header since dns_list_remove_related_requests_and_swap may free the dns requests with 
                  * the using the same dns ip
                  */
                  ptr = dns_request_list;
                  continue;
               }
               
            }

            ptr = next;
         }

      } /* if (retval) */

    }  /* while (!dns_main_quit) */
   return 0;
}


/*
 * Return a buffer which contains the current date/time.
 */
void get_date_time_str(char *buf, unsigned int buflen)
{
	time_t t;
	struct tm *tmp;

	memset(buf, 0, buflen);

	t = time(NULL);
	tmp = localtime(&t);
	if (tmp == NULL) {
		debug_perror("could not get localtime");
		return;
	}

	strftime(buf, buflen, "[%F %k:%M:%S]", tmp);

}


/*****************************************************************************/
void debug_perror( char * msg ) {
	debug( "%s : %s\n" , msg , strerror(errno) );
}

#if 0 //BRCM: Mask the debug() function. It's redefined by cmsLog_debug()
#ifdef DPROXY_DEBUG
/*****************************************************************************/
void debug(char *fmt, ...)
{
#define MAX_MESG_LEN 1024
  
  va_list args;
  char text[ MAX_MESG_LEN ];
  
  sprintf( text, "[ %d ]: ", getpid());
  va_start (args, fmt);
  vsnprintf( &text[strlen(text)], MAX_MESG_LEN - strlen(text), fmt, args);	 
  va_end (args);
  
  printf(text);
#if 0 //BRCM 
  if( config.debug_file[0] ){
	 FILE *fp;
	 fp = fopen( config.debug_file, "a");
	 if(!fp){
		syslog( LOG_ERR, "could not open log file %m" );
		return;
	 }
	 fprintf( fp, "%s", text);
	 fclose(fp);
  }
  

  /** if not in daemon-mode stderr was not closed, use it. */
  if( ! config.daemon_mode ) {
	 fprintf( stderr, "%s", text);
  }
#endif
}

#endif
#endif
/*****************************************************************************
 * print usage informations to stderr.
 * 
 *****************************************************************************/
void usage(char * program , char * message ) {
  fprintf(stderr,"%s\n" , message );
  fprintf(stderr,"usage : %s [-c <config-file>] [-d] [-h] [-P]\n", program );
  fprintf(stderr,"\t-c <config-file>\tread configuration from <config-file>\n");
  fprintf(stderr,"\t-d \t\trun in debug (=non-daemon) mode.\n");
  fprintf(stderr,"\t-D \t\tDomain Name\n");
  fprintf(stderr,"\t-P \t\tprint configuration on stdout and exit.\n");
  fprintf(stderr,"\t-v \t\tset verbosity, where num==0 is LOG_ERROR, 1 is LOG_NOTICE, all others is LOG_DEBUG\n");
  fprintf(stderr,"\t-h \t\tthis message.\n");
}
/*****************************************************************************
 * get commandline options.
 * 
 * @return 0 on success, < 0 on error.
 *****************************************************************************/
int get_options( int argc, char ** argv ) 
{
  char c = 0;
  char * progname = argv[0];
  SINT32 logLevelNum;
  CmsLogLevel logLevel=DEFAULT_LOG_LEVEL;
  //UBOOL8 useConfiguredLogLevel=TRUE;

  cmsLog_initWithName(EID_DNSPROXY, argv[0]);

  conf_defaults();
#if 1 
  while( (c = getopt( argc, argv, "cD:dhPv:")) != EOF ) {
    switch(c) {
	 case 'c':
  		conf_load(optarg);
		break;
	 case 'd':
		break;
     case 'D':
        strcpy(config.domain_name, optarg);
        break;
	 case 'h':
		usage(progname,"");
		return -1;
	 case 'P':
		break;
         case 'v':
         	logLevelNum = atoi(optarg);
         	if (logLevelNum == 0)
         	{
            		logLevel = LOG_LEVEL_ERR;
         	}
         	else if (logLevelNum == 1)
         	{
            		logLevel = LOG_LEVEL_NOTICE;
         	}
         	else
         	{
            		logLevel = LOG_LEVEL_DEBUG;
         	}
         	cmsLog_setLevel(logLevel);
         	//useConfiguredLogLevel = FALSE;
         	break;
	 default:
		usage(progname,"");
		return -1;
    }
  }
#endif

#if 0  
  /** unset daemon-mode if -d was given. */
  if( not_daemon ) {
	 config.daemon_mode = 0;
  }
  
  if( want_printout ) {
	 conf_print();
	 exit(0);
  }
#endif 

  return 0;
}
/*****************************************************************************/
void sig_hup (int signo)
{
  signal(SIGHUP, sig_hup); /* set this for the next sighup */
  conf_load (config.config_file);
}
/*****************************************************************************/
int main(int argc, char **argv)
{

  /* get commandline options, load config if needed. */
  if(get_options( argc, argv ) < 0 ) {
	  exit(1);
  }

  /* detach from terminal and detach from smd session group. */
  if (setsid() < 0)
  {
    cmsLog_error("could not detach from terminal");
    exit(-1);
  }

  /* ignore some common, problematic signals */
  signal(SIGINT, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);

  signal(SIGHUP, sig_hup);
  dns_init();

//BRCM: Don't fork a task again!
#if 0 
  if (config.daemon_mode) {
    /* Standard fork and background code */
    switch (fork()) {
	 case -1:	/* Oh shit, something went wrong */
		debug_perror("fork");
		exit(-1);
	 case 0:	/* Child: close off stdout, stdin and stderr */
		close(0);
		close(1);
		close(2);
		break;
	 default:	/* Parent: Just exit */
		exit(0);
    }
  }
#endif
#ifdef SKY
	if(sky_handleStreamSocket() < 0)
	{
		printf("\n %s: DNS Stream Socket creation failed\n",__FILE__);
		/* Do I need to really exit here*/
		return -1;
	}
#endif /*SKY*/
  dns_main_loop();

#ifdef SKY
	if(sky_cancelStreamDNSHandle() < 0)
	{
		printf("\n%s : DNS stream handling cancellation failed\n",__FILE__);
		return -1;
	}
#endif
  return 0;
}

#ifdef SKY
static void dns_get_skyhijack()
{
	FILE *fs=NULL;
	
	if((fs = fopen("/var/redirectAll", "r")))
	{
		fread(&hijackBitmap,1,1,fs);
		fclose(fs);
	}
	else
	{
		hijackBitmap=0;
		debug_perror("could not open file  /var/redirectAll");
	}
	

}

static void getRouterIPAddress()
{
	FILE *fp=NULL;
  	char line[256];
	char *ip, *name;
	int found = 0;
	if (!(fp = fopen("/var/hosts", "r"))) {
         	fprintf(stderr,"can not open file %s", "/var/hosts");
		return;
	}

	while(fgets(line, 256, fp)){
		line[strlen(line) - 1] = 0; /* get rid of '\n' */
		ip = strtok( line, " \t");

		if(ip == NULL) /* ignore blank lines */
			continue;
		if(ip[0] == '#') /* ignore comments */
			continue;

		while((name = strtok( NULL, " \t" )))
		{
			if(name[0] == '#')
				break;

			if (!strcasecmp("skyrouter",name))
			{
				strncpy(routerIPAddress,ip, sizeof(routerIPAddress));
				found = 1;
				break;
			}
		}

		if (found) {
			break;
		}
	}

	fclose(fp);	
}

#ifdef SKY_IPV6

static void getRouterIPv6ULA()
{
	FILE *fp=NULL;

	if (!(fp = fopen("/var/router.ula", "r"))) {
       	fprintf(stderr,"can not open file %s", "/var/router.ula");
		return;
	}

	fgets(routerIPv6Address, ROUTER_IPV6_ADDR_SIZE, fp);
	/* ula address is in the format   fdb9:f7ab:ec22:0000:0219:fbff:feff:ffea/64
	we have to remove trailing  /64
	*/	
	routerIPv6Address[strcspn(routerIPv6Address, "/")] = 0;
	
	fclose(fp);	
}


static void getRouterIPv6LinkAddr()
{
	FILE *fh = NULL;
	int addr_len = 0;

	system("ip -6 addr show dev br0 scope link | "
		"sed -n -e 's/^[[:space:]]*inet6[[:space:]]\\([0-9a-fA-F:]*\\).*$/\\1/pg' 2> /dev/null > /var/rouer.lla");

	// open data file
	if (NULL == (fh = fopen("/var/rouer.lla", "r"))) {
		fprintf(stderr, "Unable to open LLA intermediate file");
		return;
	}

	memset (routerIPv6LinkAddr, 0x00, sizeof(char) * ROUTER_IPV6_ADDR_SIZE);
	if (NULL == fgets(routerIPv6LinkAddr, ROUTER_IPV6_ADDR_SIZE, fh)) {
		fprintf(stderr, "LLA file empty or not readable");
		goto exit_cleanup;
	}

	if (1 < (addr_len = strlen(routerIPv6LinkAddr))) {
		routerIPv6LinkAddr[--addr_len] = '\0';
	}

    cmsLog_debug("routerIPv6LinkAddr - %s",routerIPv6LinkAddr);
exit_cleanup:
	fclose(fh);
	unlink("/var/rouer.lla");
}


#endif // SKY_IPV6



static UBOOL8 isRouterLocalIpAddress(char *srcAddr)
{
	cmsLog_debug("isRouterLocalIpAddress:: srcAddr - %s ",srcAddr);

#ifndef SKY_IPV6	
	if ((!strcmp(srcAddr, LOOPBACK_ADDRESS)) || (!strcmp(srcAddr, routerIPAddress)))
	{
		cmsLog_debug("DNS Request from local router IPV4 address");
		return  TRUE;
	}
#else
	if ((!strcmp(srcAddr, LOOPBACK_ADDRESS)) || (!strcmp(srcAddr, routerIPAddress)) || (!strcmp(srcAddr, LOOPBACK_ADDRESS_IPV6)) )
	{
		cmsLog_debug("DNS Request from local router IPV4 address/ IPV6 address - %s", srcAddr);
		return  TRUE;
	}
	if(routerIPv6LinkAddr[0] == '\0')
		getRouterIPv6LinkAddr();

	if(routerIPv6Address[0] == '\0')
		getRouterIPv6ULA();
	
	if ((!strcasecmp(srcAddr, routerIPv6LinkAddr)) || (!strcasecmp(srcAddr, routerIPv6Address)))
	{
		cmsLog_debug("DNS Request from local router IPV6 address");
		return	TRUE;
	}
#endif // SKY_IPV6	
	return FALSE;
}
#endif /* SKY */
