/* dhcpd.c
 *
 * Moreton Bay DHCP Server
 * Copyright (C) 1999 Matthew Ramsay <matthewr@moreton.com.au>
 *			Chris Trew <ctrew@moreton.com.au>
 *
 * Rewrite by Russ Dill <Russ.Dill@asu.edu> July 2001
 *
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

#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/time.h>
#include <malloc.h>
#include "debug.h"
#include "dhcpd.h"
#include "arpping.h"
#include "socket.h"
#include "options.h"
#include "files.h"
#include "leases.h"
#include "packet.h"
#include "serverpacket.h"
#include "pidfile.h"
#include "static_leases.h"
#include "cms_mem.h"
#include "cms_msg.h"
#ifdef DHCP_RELAY
#include "relay.h"
#endif

/* globals */
struct server_config_t server_config;
struct iface_config_t *iface_config;
struct iface_config_t *cur_iface;
#ifdef DHCP_RELAY
struct relay_config_t *relay_config;
struct relay_config_t *cur_relay;
#endif
// BRCM_begin
struct dhcpOfferedAddr *declines;
pVI_INFO_LIST viList;
void *msgHandle=NULL;
extern char deviceOui[];
extern char deviceSerialNum[];
extern char deviceProductClass[];

extern void bcmQosDhcp(int optionnum, char *cmd);
extern void bcmDelObsoleteRules(void);
extern void bcmExecOptCmd(void);

// BRCM_end

#ifdef SKY_DIAGNOSTICS
#define SKY_DIAGNOSTICS_DHCP_MSG_STATS_FILE "/tmp/skyDiagDhcpStats.txt"

UBOOL8 gLogDhcpMsgStats=FALSE;

struct dlist_node dhcpMsgInfoHead[DHCPINFORM+1];

typedef struct {
    DlistNode dlist;
    char mac[BUFLEN_18];
    char timeStamp[BUFLEN_32];
}dhcpMsgInfo;

uint32_t dhcpMsgCount[DHCPINFORM+1];

void init_dhcp_msg_stats() 
{
    int msg;
    for(msg=DHCPDISCOVER;msg<=DHCPINFORM;msg++) {
        dhcpMsgInfoHead[msg].prev = &dhcpMsgInfoHead[msg];
        dhcpMsgInfoHead[msg].next = &dhcpMsgInfoHead[msg];
    }
}


void skyDiagLogDhcpMsg(int msg, u_int8_t *mac)
{
    dhcpMsgInfo  *msgInfo;
    time_t cur_time;
    char macAddr[BUFLEN_18]="";
    struct tm gtime;
    char timestamp[BUFLEN_32]="";

    cmsUtl_macNumToStr(mac, macAddr);
    memset(&gtime,0,sizeof(struct tm));

    if(!cmsUtl_isValidMacAddress(macAddr)) {
        cmsLog_debug("Invalid MAC, Return");
        return;
    }

    if((msg>=DHCPDISCOVER) && (msg<=DHCPINFORM)) {
        cmsLog_debug("Msg Received - %d",msg);
        dhcpMsgCount[msg]++;

        if(gLogDhcpMsgStats) {
            dlist_for_each_entry(msgInfo, &dhcpMsgInfoHead[msg], dlist) {
                if (!cmsUtl_strcmp(msgInfo->mac,macAddr)) {
                    cur_time = time(NULL);
                    localtime_r(&cur_time, &gtime);
                    strftime(msgInfo->timeStamp,BUFLEN_32,"%d:%m:%Y::%H:%M:%S",&gtime);
                    cmsLog_debug("New Msg Added: msg-%d, mac-%s,ts-%s",msg,macAddr,msgInfo->timeStamp);
                    return;
                }
            }

            if ((msgInfo = cmsMem_alloc(sizeof(dhcpMsgInfo), ALLOC_ZEROIZE)) == NULL){
                cmsLog_error("dhcpMsgInfo allocation failed");
                return;
            }
            cmsUtl_strncpy(msgInfo->mac,macAddr,BUFLEN_18);
            cur_time = time(NULL);
            localtime_r(&cur_time, &gtime);
            strftime(msgInfo->timeStamp,BUFLEN_32,"%d:%m:%Y::%H:%M:%S",&gtime);
            cmsLog_debug("New Msg Added: msg-%d, mac-%s,ts-%s",msg,macAddr,msgInfo->timeStamp);
            dlist_prepend((DlistNode *) msgInfo, &dhcpMsgInfoHead[msg]);
        }
	}
}


void write_dhcp_msg_stats ()
{
    FILE *fp;
    dhcpMsgInfo  *msgInfo;
    char logMsg[BUFLEN_1024];

    cmsLog_debug("Entry");

    if (!(fp = fopen(SKY_DIAGNOSTICS_DHCP_MSG_STATS_FILE, "w"))) {
        cmsLog_error("Unable to open %s for writing", SKY_DIAGNOSTICS_DHCP_MSG_STATS_FILE);
        return;
    }

    dlist_for_each_entry(msgInfo, &dhcpMsgInfoHead[DHCPDISCOVER], dlist) {
        snprintf(logMsg,sizeof(logMsg),"DIS|%s|%s|\n",msgInfo->mac,msgInfo->timeStamp);
        cmsLog_debug("dhcpLogMsg:DHCPDISCOVER  - %s",logMsg);
        fwrite(logMsg, strlen(logMsg), 1, fp);
    }

    dlist_for_each_entry(msgInfo, &dhcpMsgInfoHead[DHCPOFFER], dlist) {
        snprintf(logMsg,sizeof(logMsg),"OFF|%s|%s|\n", msgInfo->mac,msgInfo->timeStamp);
        cmsLog_debug("dhcpLogMsg:DHCPOFFER  - %s",logMsg);
        fwrite(logMsg, strlen(logMsg), 1, fp);
    }

    dlist_for_each_entry(msgInfo, &dhcpMsgInfoHead[DHCPREQUEST], dlist) {
        snprintf(logMsg,sizeof(logMsg),"REQ|%s|%s|\n", msgInfo->mac,msgInfo->timeStamp);
        cmsLog_debug("dhcpLogMsg:DHCPREQUEST  - %s",logMsg);
        fwrite(logMsg, strlen(logMsg), 1, fp);
    }
    
    dlist_for_each_entry(msgInfo, &dhcpMsgInfoHead[DHCPDECLINE], dlist) {
        snprintf(logMsg,sizeof(logMsg),"DEC|%s|%s|\n", msgInfo->mac,msgInfo->timeStamp);
        cmsLog_debug("dhcpLogMsg:DHCPDECLINE  - %s",logMsg);
        fwrite(logMsg, strlen(logMsg), 1, fp);
    }

    dlist_for_each_entry(msgInfo, &dhcpMsgInfoHead[DHCPACK], dlist) {
        snprintf(logMsg,sizeof(logMsg),"ACK|%s|%s|\n", msgInfo->mac,msgInfo->timeStamp);
        cmsLog_debug("dhcpLogMsg:DHCPACK  - %s",logMsg);
        fwrite(logMsg, strlen(logMsg), 1, fp);
    }
    
    dlist_for_each_entry(msgInfo, &dhcpMsgInfoHead[DHCPNAK], dlist) {
        snprintf(logMsg,sizeof(logMsg),"NAK|%s|%s|\n", msgInfo->mac,msgInfo->timeStamp);
        cmsLog_debug("dhcpLogMsg:DHCPNAK  - %s",logMsg);
        fwrite(logMsg, strlen(logMsg), 1, fp);
    }

    dlist_for_each_entry(msgInfo, &dhcpMsgInfoHead[DHCPRELEASE], dlist) {
        snprintf(logMsg,sizeof(logMsg),"REL|%s|%s|\n", msgInfo->mac,msgInfo->timeStamp);
        cmsLog_debug("dhcpLogMsg:DHCPRELEASE  - %s",logMsg);
        fwrite(logMsg, strlen(logMsg), 1, fp);
    }

    dlist_for_each_entry(msgInfo, &dhcpMsgInfoHead[DHCPINFORM], dlist) {
        snprintf(logMsg,sizeof(logMsg),"INF|%s|%s|\n", msgInfo->mac,msgInfo->timeStamp);
        cmsLog_debug("dhcpLogMsg:DHCPINFORM  - %s",logMsg);
        fwrite(logMsg, strlen(logMsg), 1, fp);
    }
    fclose(fp);
    cmsLog_debug("Done: write_dhcp_msg_stats");
}

void clear_dhcp_msg_stats(void)
{
    dhcpMsgInfo *tmp = NULL;
    int msg;
    cmsLog_debug("Entry");
    for(msg=DHCPDISCOVER;msg<=DHCPINFORM;msg++) {
        while (dlist_empty(&dhcpMsgInfoHead[msg]) == 0){
            tmp = (dhcpMsgInfo *) dhcpMsgInfoHead[msg].next;
            dlist_unlink((DlistNode *) tmp);
            cmsMem_free(tmp);
        }
    }
    cmsLog_debug("Done: clear_dhcp_msg_stats");
}


#endif //SKY_DIAGNOSTICS
/* Exit and cleanup */
void exit_server(int retval)
{
    cmsMsg_cleanup(&msgHandle);
    pidfile_delete(server_config.pidfile);
    CLOSE_LOG();
    exit(retval);
}


/* SIGTERM handler */
static void udhcpd_killed(int sig)
{
	(void)sig;
	cmsLog_debug(LOG_INFO, "Received SIGTERM");
	exit_server(0);
}


// BRCM_BEGIN
static void test_vendorid(struct dhcpMessage *packet, char *vid, int *declined)
{
	*declined = 0;

	/*
	 * If both internal vendor_ids list is not NULL and client
	 * request packet contains a vendor id, then check for match.
	 * Otherwise, leave declined at 0.
	 */
	if (cur_iface->vendor_ids != NULL && vid != NULL) {
		int len = (int)(*((unsigned char*)vid - 1));
		vendor_id_t * id = cur_iface->vendor_ids;
		//char tmpbuf[257] = {0};
		//memcpy(tmpbuf, vid, len);
		do {
			//printf("dhcpd:test_vendorid: id=%s(%d) vid=%s(%d)\n", id->id, id->len, tmpbuf, len);
			if (id->len == len && memcmp(id->id, vid, len) == 0) {
				/* vid matched something in our list, decline */
				memcpy(declines->chaddr,  packet->chaddr, sizeof(declines->chaddr));

				/*
				 * vid does not contain a terminating null, so we have to make sure
				 * the declines->vendorid is null terminated.  And use memcpy 
				 * instead of strcpy because vid is not null terminated.
				 */
				memset(declines->vendorid, 0, sizeof(declines->vendorid));
				memcpy(declines->vendorid, vid, len);

				*declined = 1;
			}
		} while((*declined == 0) && (id = id->next));
	}

	if (*declined) {
		write_decline();
		sendNAK(packet);
	}
}

// BRCM_END

#ifdef COMBINED_BINARY	
int udhcpd(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
	CmsRet ret;
	int msg_fd;
	fd_set rfds;
	struct timeval tv;
        //BRCM --initialize server_socket to -1
	int bytes, retval;
	struct dhcpMessage packet;
	unsigned char *state;
        // BRCM added vendorid
	char *server_id, *requested, *hostname = NULL, *vendorid = NULL, *clientid;
    unsigned short length=0;	
	u_int32_t server_id_align, requested_align;
	unsigned long timeout_end;
	struct dhcpOfferedAddr *lease;
	//For static IP lease
	struct dhcpOfferedAddr static_lease;
	uint32_t static_lease_ip;

	int pid_fd;
	/* Optimize malloc */
	mallopt(M_TRIM_THRESHOLD, 8192);
	mallopt(M_MMAP_THRESHOLD, 16384);

	//argc = argv[0][0]; /* get rid of some warnings */

	cmsLog_init(EID_DHCPD);
	cmsLog_debug( "udhcp server (v%s) started", VERSION);
	
	pid_fd = pidfile_acquire(server_config.pidfile);
	pidfile_write_release(pid_fd);

	if ((ret = cmsMsg_initWithFlags(EID_DHCPD, 0, &msgHandle)) != CMSRET_SUCCESS) {
		cmsLog_error( "cmsMsg_init failed, ret=%d", ret);
		pidfile_delete(server_config.pidfile);
		CLOSE_LOG();
		exit(1);
	}

	//read_config(DHCPD_CONF_FILE);
	read_config(argc < 2 ? DHCPD_CONF_FILE : argv[1]);
	
	read_leases(server_config.lease_file);

        // BRCM begin
	declines = malloc(sizeof(struct dhcpOfferedAddr));
	/* vendor identifying info list */
	viList = malloc(sizeof(VI_INFO_LIST));
	memset(viList, 0, sizeof(VI_INFO_LIST));
        deviceOui[0] = '\0';
        deviceSerialNum[0] = '\0';
        deviceProductClass[0] = '\0';
        // BRCM end


#ifndef DEBUGGING
	pid_fd = pidfile_acquire(server_config.pidfile); /* hold lock during fork. */

#ifdef BRCM_CMS_BUILD
   /*
    * BRCM_BEGIN: mwang: In CMS architecture, we don't want dhcpd to
    * daemonize itself.  It creates an extra zombie process that
    * we don't want to deal with.
    * However, we do want to do a setid to detach from console.
    */
   if (setsid() < 0)
   {
      cmsLog_error("could not detach from terminal");
      exit(-1);
   }
#else
	switch(fork()) {
	case -1:
		perror("fork");
		exit_server(1);
		/*NOTREACHED*/
	case 0:
		break; /* child continues */
	default:
		exit(0); /* parent exits */
		/*NOTREACHED*/
		}
	close(0);
	setsid();
#endif /* BRCM_CMS_BUILD */

	pidfile_write_release(pid_fd);
#endif
	signal(SIGUSR1, write_leases);
	signal(SIGTERM, udhcpd_killed);
	signal(SIGUSR2, write_viTable);

#ifdef BRCM_CMS_BUILD
	/* ignore some common, problematic signals */
	signal(SIGINT, SIG_IGN);
	signal(SIGPIPE, SIG_IGN);
#endif

	timeout_end = time(0) + server_config.auto_time;

	cmsMsg_getEventHandle(msgHandle, &msg_fd);
#ifdef SKY_DIAGNOSTICS
	init_dhcp_msg_stats();
#endif //SKY_DIAGNOSTICS

	while(1) { /* loop until universe collapses */
                //BRCM_begin
                int declined = 0;
		int max_skt = msg_fd;
		FD_ZERO(&rfds);
		FD_SET(msg_fd, &rfds);
#ifdef DHCP_RELAY
                for(cur_relay = relay_config; cur_relay;
			cur_relay = cur_relay->next ) {
			if (cur_relay->skt < 0) {
				cur_relay->skt = listen_socket(INADDR_ANY,
					SERVER_PORT, cur_relay->interface);
				if(cur_relay->skt == -1) {
					cmsLog_error( "couldn't create relay "
						"socket");
					exit_server(0);
				}
			}

			FD_SET(cur_relay->skt, &rfds);
			if (cur_relay->skt > max_skt)
				max_skt = cur_relay->skt;
		}
#endif
                for(cur_iface = iface_config; cur_iface;
			cur_iface = cur_iface->next ) {
			if (cur_iface->skt < 0) {
				cur_iface->skt = listen_socket(INADDR_ANY,
					SERVER_PORT, cur_iface->interface);
				if(cur_iface->skt == -1) {
					cmsLog_error( "couldn't create server "
						"socket on %s -- au revoir",
						cur_iface->interface);
					exit_server(0);
				}
			}

			FD_SET(cur_iface->skt, &rfds);
			if (cur_iface->skt > max_skt)
				max_skt = cur_iface->skt;
                }  //BRCM_end
		if (server_config.auto_time) {
			tv.tv_sec = timeout_end - time(0);
			if (tv.tv_sec <= 0) {
				tv.tv_sec = server_config.auto_time;
				timeout_end = time(0) + server_config.auto_time;
				write_leases(0);
			}
			tv.tv_usec = 0;
		}
		retval = select(max_skt + 1, &rfds, NULL, NULL,
			server_config.auto_time ? &tv : NULL);
		if (retval == 0) {
			write_leases(0);
			timeout_end = time(0) + server_config.auto_time;
			continue;
		} else if (retval < 0) {
			if (errno != EINTR)
				perror("select()");
			continue;
		}

		/* Is there a CMS message received? */
		if (FD_ISSET(msg_fd, &rfds)) {
			CmsMsgHeader *msg;
			CmsRet ret;
			ret = cmsMsg_receiveWithTimeout(msgHandle, &msg, 0);
			if (ret == CMSRET_SUCCESS) {
            switch (msg->type) {
            case CMS_MSG_DHCPD_RELOAD:
					/* Reload config file */
					write_leases(0);
					read_config(argc < 2 ? DHCPD_CONF_FILE : argv[1]);
					read_leases(server_config.lease_file);
               break;
            case CMS_MSG_WAN_CONNECTION_UP:
#ifdef DHCP_RELAY
					/* Refind local addresses for relays */
					set_relays();
#endif
               break;
            case CMS_MSG_GET_LEASE_TIME_REMAINING:
               if (msg->dataLength == sizeof(GetLeaseTimeRemainingMsgBody)) {
					   GetLeaseTimeRemainingMsgBody *body = (GetLeaseTimeRemainingMsgBody *) (msg + 1);
					   u_int8_t chaddr[16]={0};

					   cur_iface = find_iface_by_ifname(body->ifName);
					   if (cur_iface != NULL) {
							   msg->dst = msg->src;
							   msg->src = EID_DHCPD;
							   msg->flags_request = 0;
							   msg->flags_response = 1;
						   cmsUtl_macStrToNum(body->macAddr, chaddr);
						   lease = find_lease_by_chaddr(chaddr);
						   if (lease != NULL) {
							   msg->wordData = lease_time_remaining(lease);
						   }else{
							   msg->wordData = 0;
						   }
							   msg->dataLength = 0;
							   cmsMsg_send(msgHandle, msg);
					   }
						else
						{
						 	msg->dst = msg->src;
						   	msg->src = EID_DHCPD;
							msg->flags_request = 0;
							msg->flags_response = 1;
							msg->wordData = 0;/* No lease since DHCP is not aware of this*/
							msg->dataLength = 0;
							cmsMsg_send(msgHandle, msg);/* Send reply since LAN HOST ENTRY stl function will wait for a reply */
						}
               }
               else {
                  cmsLog_error( "invalid msg, type=0x%x dataLength=%d", msg->type, msg->dataLength);
               }
               break;
            case CMS_MSG_EVENT_SNTP_SYNC:               
               if (msg->dataLength == sizeof(long)) 
               {
                  long *delta = (long *) (msg + 1); 
                  adjust_lease_time(*delta);
               }
               else
               {
                  cmsLog_error( "Invalid CMS_MSG_EVENT_SNTP_SYNC msg, dataLength=%d", msg->dataLength);
               }
               break;
            case CMS_MSG_QOS_DHCP_OPT60_COMMAND:
               bcmQosDhcp(DHCP_VENDOR, (char *)(msg+1));
               break;
            case CMS_MSG_QOS_DHCP_OPT77_COMMAND:
               bcmQosDhcp(DHCP_USER_CLASS_ID, (char *)(msg+1));
               break;
#ifdef SKY_DIAGNOSTICS			   
			case CMS_MSG_GET_DHCP_MSG_COUNT:
				cmsLog_debug("DHCPStats - %d:%d:%d:%d:%d:%d:%d:%d",dhcpMsgCount[1],dhcpMsgCount[2],
					dhcpMsgCount[3],dhcpMsgCount[4],dhcpMsgCount[5],dhcpMsgCount[6],dhcpMsgCount[7],dhcpMsgCount[8]);
				if(msg->dataLength == sizeof(dhcpMsgCount)) {
					uint32_t *body = (uint32_t *)(msg+1);
			   		//uint32_t *body;
				   	memcpy(body,dhcpMsgCount,(DHCPINFORM+1)*sizeof(UINT32));
				   	msg->dst = msg->src;
				   	msg->src = EID_DHCPD;
				   	msg->flags_request = 0;
				   	msg->flags_response = 1;
				   	msg->wordData = 0;/* No lease since DHCP is not aware of this*/
				   	msg->dataLength = sizeof(dhcpMsgCount);
				   	cmsMsg_send(msgHandle, msg);
				   	//reset the counts
				   	memset(dhcpMsgCount,0,sizeof(dhcpMsgCount));
				}
				break;
				
			case CMS_MSG_START_DHCP_MSG_STATS:
				clear_dhcp_msg_stats();
				gLogDhcpMsgStats = TRUE;
				break;

			case CMS_MSG_STOP_DHCP_MSG_STATS:
				clear_dhcp_msg_stats();
				gLogDhcpMsgStats = FALSE;
				break;
			   
			case CMS_MSG_DUMP_DHCP_MSG_STATS:
				if(gLogDhcpMsgStats) {
					write_dhcp_msg_stats();
					msg->dst = msg->src;
					msg->src = EID_DHCPD;
					msg->flags_request = 0;
					msg->flags_response = 1;
					msg->wordData = CMSRET_SUCCESS;
					cmsMsg_send(msgHandle, msg);
					clear_dhcp_msg_stats();
				}
				break;
#endif //SKY_DIAGNOSTICS			   
            default:
               cmsLog_error( "unrecognized msg, type=0x%x dataLength=%d", msg->type, msg->dataLength);
               break;
            }
				cmsMem_free(msg);
			} else if (ret == CMSRET_DISCONNECTED) {
				if (!cmsFil_isFilePresent(SMD_SHUTDOWN_IN_PROGRESS)) {
					cmsLog_error( "lost connection to smd, exiting now.");
				}
				exit_server(0);
			}
			continue;
		}
#ifdef DHCP_RELAY
		/* Check packets from upper level DHCP servers */
		for (cur_relay = relay_config; cur_relay;
			cur_relay = cur_relay->next) {
			if (FD_ISSET(cur_relay->skt, &rfds))
				break;
		}
		if (cur_relay) {
			process_relay_response();
			continue;
		}
#endif

		/* Find out which interface is selected */
		for(cur_iface = iface_config; cur_iface;
			cur_iface = cur_iface->next ) {
			if (FD_ISSET(cur_iface->skt, &rfds))
				break;
		}
		if (cur_iface == NULL)
			continue;

		bytes = get_packet(&packet, cur_iface->skt); /* this waits for a packet - idle */
		if(bytes < 0)
			continue;

		/* Decline requests */
		if (cur_iface->decline)
			continue;

		if ( (packet.chaddr[0] == cur_iface->arp[0]) &&
				(packet.chaddr[1] == cur_iface->arp[1]) &&
				(packet.chaddr[2] == cur_iface->arp[2]) &&
				(packet.chaddr[3] == cur_iface->arp[3]) &&
				(packet.chaddr[4] == cur_iface->arp[4]) &&
				(packet.chaddr[5] == cur_iface->arp[5]) ) {
			// DHCP REQUEST came from the same HW address
			// as ours - looks like a loop - ignore that request

			cmsLog_error("Received DHCP request with the same HW address as the ours. IGNORING...");
			continue;
		}

#ifdef DHCP_RELAY
		/* Relay request? */
		if (cur_iface->relay_interface[0]) {
			process_relay_request((char*)&packet, bytes);
			continue;
		}
#endif

		if((state = get_option(&packet, DHCP_MESSAGE_TYPE, NULL)) == NULL) {
			DEBUG(LOG_ERR, "couldnt get option from packet -- ignoring");
			continue;
		}
		
		/* For static IP lease */
		/* Look for a static lease */
		static_lease_ip = getIpByMac(cur_iface->static_leases, &packet.chaddr);

		if(static_lease_ip)
		{
			memcpy(&static_lease.chaddr, &packet.chaddr, 16);
			static_lease.yiaddr = static_lease_ip;
			static_lease.expires = -1; /* infinite lease time */
			lease = &static_lease;
		} /* For static IP lease end */
		else
		{
			lease = find_lease_by_chaddr(packet.chaddr);
		}

		switch (state[0]) {
		case DHCPDISCOVER:
			DEBUG(LOG_INFO,"received DISCOVER");
			//BRCM_begin
			vendorid = (char *)get_option(&packet, DHCP_VENDOR,NULL);
			test_vendorid(&packet, vendorid, &declined);
			// BRCM_end
#ifdef SKY_DIAGNOSTICS
			skyDiagLogDhcpMsg(DHCPDISCOVER, &packet.chaddr);
#endif

         if (!declined) {
				if (sendOffer(&packet) < 0) {
					LOG(LOG_DEBUG, "send OFFER failed -- ignoring");
				}
//brcm begin
            /* delete any obsoleted QoS rules because sendOffer() may have
             * cleaned up the lease data.
             */
            bcmDelObsoleteRules(); 
//brcm end
			}
			break;			
 		case DHCPREQUEST:
			DEBUG(LOG_INFO,"received REQUEST");
#ifdef SKY_DIAGNOSTICS
			skyDiagLogDhcpMsg(DHCPREQUEST, &packet.chaddr);
#endif

			requested = (char *)get_option(&packet, DHCP_REQUESTED_IP, NULL);
			server_id = (char *)get_option(&packet, DHCP_SERVER_ID, NULL);
			hostname  = (char *)get_option(&packet, DHCP_HOST_NAME, NULL);
			clientid = (char *)get_option(&packet, DHCP_CLIENT_ID,NULL);
			//printf("\n=== DHCP Sky Debug hostname [%s]=====\n",hostname);
			if (requested) memcpy(&requested_align, requested, 4);
			if (server_id) memcpy(&server_id_align, server_id, 4);
		
			//BRCM_begin
			vendorid = (char *)get_option(&packet, DHCP_VENDOR,NULL);
			test_vendorid(&packet, vendorid, &declined);
			// BRCM_end

			if (!declined) {
				if (lease) {
					if (hostname) {
						bytes = hostname[-1];
						if (bytes >= (int) sizeof(lease->hostname))
							bytes = sizeof(lease->hostname) - 1;
						strncpy(lease->hostname, hostname, bytes);
						lease->hostname[bytes] = '\0';
					/* This is where it notify to lan host if we have will send now and also after sending ACK 
					 This may not have much impact as in requested id or renew it will check but we don't give 
					 chance to server to change hence this is the first packet received by the server so this should
					 definetley has correct one*/	

					if(clientid != NULL){
	  						length = *(clientid -1);
          						memcpy(lease->clientid, clientid, (size_t)length);
         						 lease->clientid[(int)length] = '\0';
						}
						send_lease_info(FALSE, lease);
					}  else
					{
						lease->hostname[0] = '\0';             
#ifdef SKY
						send_lease_info(FALSE, lease); /*Let device monitor try to get from NetBios */
#endif //SKY
					}
					if (server_id) {
						/* SELECTING State */
						DEBUG(LOG_INFO, "server_id = %08x", ntohl(server_id_align));
						if (server_id_align == cur_iface->server && requested && 
						    requested_align == lease->yiaddr) {
							sendACK(&packet, lease);
						/* 03/04/2013: SR-837: 
							Attached devices are shown as UNKNOWN, this is b'cas second send_lease_info 
							message as hostname missing in lease message, this causes lanhostinfo to has
							UNKNOWN as hostname

							Fix was deliberately removed during 4.12.08 merge and has been taken back 
							*/
							if (hostname) {
								bytes = hostname[-1];
								if (bytes >= (int) sizeof(lease->hostname))
									bytes = sizeof(lease->hostname) - 1;
								strncpy(lease->hostname, hostname, bytes);
								lease->hostname[bytes] = '\0';
							}  else
								lease->hostname[0] = '\0';       
							send_lease_info(FALSE, lease);
//brcm begin
                     /* delete any obsoleted QoS rules because sendACK() may have
                      * cleaned up the lease data.
                      */
                     bcmDelObsoleteRules(); 
                     bcmExecOptCmd(); 
//brcm end
						}
					} else {
						if (requested) {
							/* INIT-REBOOT State */
							if (lease->yiaddr == requested_align) {
								sendACK(&packet, lease);
							/* 03/04/2013: SR-837: 
							Attached devices are shown as UNKNOWN, this is b'cas second send_lease_info 
							message as hostname missing in lease message, this causes lanhostinfo to has
							UNKNOWN as hostname

							Fix was deliberately removed during 4.12.08 merge and has been taken back 
							*/
								if (hostname) {
									bytes = hostname[-1];
									if (bytes >= (int) sizeof(lease->hostname))
										bytes = sizeof(lease->hostname) - 1;
									strncpy(lease->hostname, hostname, bytes);
									lease->hostname[bytes] = '\0';
								}  else
									lease->hostname[0] = '\0';       
								send_lease_info(FALSE, lease);
//brcm begin
                        /* delete any obsoleted QoS rules because sendACK() may have
                         * cleaned up the lease data.
                         */
                        bcmDelObsoleteRules(); 
                        bcmExecOptCmd(); 
//brcm end
							}
							else sendNAK(&packet);
						} else {
							/* RENEWING or REBINDING State */
							if (lease->yiaddr == packet.ciaddr) {
								sendACK(&packet, lease);
							/* 03/04/2013: SR-837: 
							Attached devices are shown as UNKNOWN, this is b'cas second send_lease_info 
							message as hostname missing in lease message, this causes lanhostinfo to has
							UNKNOWN as hostname

							Fix was deliberately removed during 4.12.08 merge and has been taken back 
							*/
								if (hostname) {
									bytes = hostname[-1];
									if (bytes >= (int) sizeof(lease->hostname))
										bytes = sizeof(lease->hostname) - 1;
									strncpy(lease->hostname, hostname, bytes);
									lease->hostname[bytes] = '\0';
								}  else
									lease->hostname[0] = '\0';       
								send_lease_info(FALSE, lease);
//brcm begin
                        /* delete any obsoleted QoS rules because sendACK() may have
                         * cleaned up the lease data.
                         */
                        bcmDelObsoleteRules(); 
                        bcmExecOptCmd(); 
//brcm end
							}
							else {
								/* don't know what to do!!!! */
								sendNAK(&packet);
							}
						}						
					}
				} else { /* else remain silent */				
    					sendNAK(&packet);
            			}
			}
			break;
		case DHCPDECLINE:
			DEBUG(LOG_INFO,"received DECLINE");
#ifdef SKY_DIAGNOSTICS
			skyDiagLogDhcpMsg(DHCPDECLINE, &packet.chaddr);
#endif
			
			if (lease) {
				memset(lease->chaddr, 0, 16);
				lease->expires = time(0) + server_config.decline_time;
			}			
			break;
		case DHCPRELEASE:
			DEBUG(LOG_INFO,"received RELEASE");
#ifdef SKY_DIAGNOSTICS
		skyDiagLogDhcpMsg(DHCPRELEASE, &packet.chaddr);
#endif
			
			if (lease) {
				lease->expires = time(0);
				send_lease_info(TRUE, lease);
//brcm begin
            sleep(1);
            /* delete the obsoleted QoS rules */
            bcmDelObsoleteRules(); 
//brcm end
			}
			break;
		case DHCPINFORM:
			DEBUG(LOG_INFO,"received INFORM");
#ifdef SKY_DIAGNOSTICS
			skyDiagLogDhcpMsg(DHCPINFORM, &packet.chaddr);
#endif
			
			send_inform(&packet);
			break;	
		default:
			LOG(LOG_WARNING, "unsupported DHCP message (%02x) -- ignoring", state[0]);
		}
	}
	return 0;
}

