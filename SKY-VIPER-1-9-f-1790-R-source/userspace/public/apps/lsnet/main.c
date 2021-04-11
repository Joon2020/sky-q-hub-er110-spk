/* includes */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <wait.h>
#include "cms.h"
#include "cms_eid.h"
#include "cms_util.h"
//#include "cms_core.h"
#include "cms_msg.h"

#include "include/utils.h"
//#include "include/ip_common.h"
#include "include/libnetlink.h"
#include "include/rt_names.h"
#include <sys/time.h>

#ifdef S_SPLINT_S
#include "cms_api_override.h"
#endif // S_SPLINT_S

#ifndef SKY_LSNET_SUPPORT

#error "Sky lsnet is not supported"

#else

void  *msgHandle = NULL;
static int logLevel=DEFAULT_LOG_LEVEL;
//int lsnet_state=-1;
typedef enum {
	LSNET_STATE_IMMD=0,
	LSNET_STATE_1M,
	LSNET_STATE_2M,
	LSNET_STATE_4M,
} LSNET_STATE;

typedef enum {
	DEVICE_ENTRY_CREATED=0,
	DEVICE_ENTRY_MODIFIED,
	DEVICE_ENTRY_REFRESHED,
	DEVICE_ENTRY_TEST_REACHABILITY,
	DEVICE_ENTRY_REACHABILITY_CONFIRMED,
	DEVICE_ENTRY_NOT_ACTIVE,
} DeviceState;


char *BridgeInterfaces[] = {"none", "eth0", "eth1", "eth2", "eth3", "eth4", "eth5", "wl0"};

/* definitions */
#define LSNET_LAN_INTERFACE_NAME 	"br0"
#define WHITESPACE " \t\r\n"
#define HOSTNAME_LEN 64
#define IFNAME_SIZE 8
#define IP_ADDR_LEN 16
#define MACADDR_LEN 18
#define MAX_HOST_ENTRIES 255
#define DEFAULT_MAC "00:00:00:00:00:00"
#define DEFAULT_HOSTNAME "UNKNOWN"
#define LSNET_REFRESH_INTERVAL 60 // 60 seconds
#define TWENTY_FOUR_HOURS_IN_SEC 60*60*24
#define LATEST_ACTIVE_TIME_DIFF_IN_SEC 55 // just less than  LSNET_REFRESH_INTERVAL

/* types */
struct LanDevice
{
    char IP[IP_ADDR_LEN];
    char HostName[HOSTNAME_LEN];
    char MAC[MACADDR_LEN];
	DeviceState state;
	char ifName[IFNAME_SIZE];
    struct LanDevice *Next;
	char addressSource[8];
	UINT32 lastActiveTime;
	UINT32 hostNameResolvedTime;
} LanDevice;

typedef enum {
	REMOVE_ON_IFNAME=0,
	REMOVE_ON_MAC,
	REMOVE_ON_IP
} DeviceRemoveFilter;

typedef struct LanDevice *PLANDevice;
typedef struct Mac_Host_map *P_Mac_Host_map;


PLANDevice lanDeviceListStart = NULL;
char lan_address[16];

pthread_mutex_t  deviceListMutex = PTHREAD_MUTEX_INITIALIZER;  
pthread_t neighbourThreadId;


struct rtnl_handle rth = { .fd = -1 };
int show_stats = 1;
int timestamp = 0;
static struct
{
	int family;
    int index;
	int state;
	int unused_only;
	inet_prefix pfx;
	int flushed;
	char *flushb;
	int flushp;
	int flushe;
} filter;


/* prototypes */
static void start_lsnet(int startup);
static void registerCMSmessageOfInterest (CmsMsgType msgType, char * name);
static void addModifyLanDevice(char *IP, char *hostName, char *MAC, int create, UBOOL8 hostsFromOther, char *addressSource);
static void printLanDevices(char *fileName);
static void getLanIPAddress(void);
static UBOOL8 processNetbiosResults(char*);
//static void signalAnyTime();
static int processArpScanResults();
static void inActivateAttachedDevices(char *key, DeviceRemoveFilter inInterface);
static UBOOL8  resolveHostNameUsingNbtScan(char *ipAddr, char *hostName);
static int  getAttachedDeviceInterface(char *macAddr);
static void  mapBridgePortToIfName(int PortNo, char* ifName);
static void updateAttachedDevice(PLANDevice deviceEntry,  UBOOL8 isDelete);
static void inActivateNonReachableDevices();
static int createArpScanList();
static void testReachabilityUsingWirelessAssocList();
static void testReachabilityUsingArpScan();
static int handleNeighbourMessages(const struct sockaddr_nl *who,
	       struct nlmsghdr *n, void *arg);
/*@null@*/ static void*  listen_for_netlink_messages(void *);
static void processDhcpMsg(CmsMsgHeader *msg);
static void prepareLanDevicesForReachabilityTest();
static UBOOL8 searchDeviceMacFromAssocList(char *MAC);
static void mapDeviceInterface(PLANDevice deviceEntry);




void threadCleanupHandler(void  *arg)
{
	/* To be safer exit the complete process if neighbour message handling thread is terminated
	lsnet process will be restarted by SMD on receiving any event
	*/
	cmsLog_debug(" thread cleanup handler called ");
	exit(1);
	
}

static void create_neighbour_thread()
{
	int ret = 0;
	if( (ret = pthread_create(&neighbourThreadId,NULL,listen_for_netlink_messages, NULL)) != 0)
	{
		cmsLog_error(" Stream thread cannot be created");
		exit(1);
	}
	else
		cmsLog_debug(" new thraed createde for listening neighbour messages ");
}

static void readHostsFileAndCreateList()
{
		FILE *brOutput = NULL;
		char line[256];
		char *pch;
		int count = 0;
		int noDevices=0;
		char ipAddr[IP_ADDR_LEN];
		char hostname[HOSTNAME_LEN];
		char macAddr[MACADDR_LEN];
	
		cmsLog_debug("enter");

		memset(line,0,sizeof(line));
	
		brOutput = fopen("/var/lsnethosts", "r");
		if (NULL == brOutput)
		{
			cmsLog_error("file open error  file = /var/lsnethosts \n");
			return;
		}
	
			
		/* read a line */
		while (NULL != fgets(line, sizeof(line), brOutput))
		{
//			cmsLog_error("Line = %s",line);
			count = 0;
			pch = strtok (line,",");

			noDevices++;			
			while (pch != NULL)
			{
				switch(count)
				{
					case 0 :
						snprintf(ipAddr,sizeof(ipAddr),"%s",pch);
						break;
					case 1 :
						snprintf(hostname,sizeof(hostname),"%s",pch);
						break;
					case 2 :
						snprintf(macAddr,sizeof(macAddr),"%s",pch);
						macAddr[strlen(macAddr)-1]='\0';
						break;
					default:
						/* don't care */
						break;
				}
				count ++;			
				pch = strtok (NULL, ",");
			}
			addModifyLanDevice(ipAddr,hostname,macAddr, 1, TRUE, "DHCP");
		}
	
		fclose(brOutput);	
		cmsLog_debug("/var/lsnethosts contains %d ", noDevices);

}



/* lsnet entry point */
int main(int argc, char *argv[])
{	
	UINT32 cmsSocketFD= -1;
	fd_set readfd;
	CmsMsgHeader *cmsMsg=NULL;
	CmsRet cmsRet;

	if (cmsMsg_init(EID_LSNET,&msgHandle) != CMSRET_SUCCESS) {
		cmsLog_error ("\n lsnet cmsMsg_init FAILED!\n");
		exit (1);
	}
    cmsLog_init(EID_LSNET); 
	if (cmsMsg_getEventHandle(msgHandle, &cmsSocketFD) != CMSRET_SUCCESS) {
		printf ("Lsnet message handle failed!\n");
		exit (1);
	}

	cmsLog_setLevel(LOG_LEVEL_ERR);  
    registerCMSmessageOfInterest (CMS_MSG_ETH_LINK_DOWN, "START LS NET");
    registerCMSmessageOfInterest (CMS_MSG_WLAN_ASSOC, "START LS NET");
    registerCMSmessageOfInterest (CMS_MSG_WLAN_DISASSOC, "START LS NET");

	/* We need to be aware when dhcp adds a new host entry to ssk, 
	so that we will monitor that device 

	Initially i thought it is not necessary as we will see the new device in the arp cache, 
	but its not 100%.   we did observe with Linux laptops ,device gets the IP address via
	dhcp but no ARP or IP packets transferred and we dont see a new ARP cache entry.

	do overcome this we also register for messages from DHCP when a new DHCP host is added, 
	we either add a new host or will modify the existing entry
	*/
	
    registerCMSmessageOfInterest (CMS_MSG_DHCPD_HOST_INFO, "START LS NET");
	

    printf("\n STATRTING LSNET \n");
	signal(SIGTERM, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	struct timeval timeVal;
	int nReady = 0 ;
	
	
	/* read hosts from /var/hosts file  and use them as the base list
	This is necessary in case if lsnet is terminated due to exception on 
	any other reason.

	to synchronise lsnet with already connected devices read the hosts from hosts file
	and populate to devices list.
	
	*/
	readHostsFileAndCreateList();

	/*  Try to find if the devices in /var/hosts are still conected, and get the MAC address */	
	start_lsnet(1);

	/* create tread for listening netlink messages from kernel neighbour modeul */
	create_neighbour_thread();
	
	linkUpDownMsgBody * linkUpDownMsg=NULL;
    struct timeval currTime, prevTime;

	gettimeofday(&prevTime,NULL);
	timeVal.tv_sec = LSNET_REFRESH_INTERVAL;
	
    for (;;)
	{
	
		FD_ZERO (&readfd);
		/*@i@*/FD_SET(cmsSocketFD, &readfd); //add cms message socket to read set mask

		nReady = select(cmsSocketFD+1, &readfd, NULL, NULL, &timeVal); 

		if(nReady > 0)
		{
	        if (FD_ISSET(cmsSocketFD, &readfd))
			{
				if ((cmsRet = cmsMsg_receive(msgHandle, &cmsMsg)) != CMSRET_SUCCESS)
				{
					if (cmsRet == CMSRET_DISCONNECTED)
					{
						cmsLog_error("detected exit of smd, ssk will also exit");
						exit (1);
					}
					continue;
				}
				if(cmsMsg == NULL)
					continue;
				
				cmsLog_debug ("Message received: type:%X length%d",cmsMsg->type, cmsMsg->dataLength);
				char *macAddr=NULL;
			    switch (cmsMsg->type)
				{
					case CMS_MSG_SET_LOG_LEVEL:
						logLevel=cmsMsg->wordData;
						cmsLog_setLevel(logLevel);
					    printLanDevices("/var/lsnet.out");
						if ((cmsRet = cmsMsg_sendReply(msgHandle, cmsMsg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
						{
							cmsLog_error("send response for msg 0x%x failed, ret=%d", cmsMsg->type, cmsRet);
						}
	            		 break;	
					case CMS_MSG_SET_LOG_DESTINATION:
					     cmsLog_debug("got set log destination to %d", cmsMsg->wordData);
					     cmsLog_setDestination(cmsMsg->wordData);
					     if ((cmsRet = cmsMsg_sendReply(msgHandle, cmsMsg, CMSRET_SUCCESS)) != CMSRET_SUCCESS)
					     {
					          cmsLog_error("send response for msg 0x%x failed, ret=%d", cmsMsg->type, cmsRet);
						 }	
						 break;					 
	 				case CMS_MSG_ETH_LINK_DOWN:
						 if (cmsMsg->dataLength)
						 {
						 	 linkUpDownMsg =  (linkUpDownMsgBody *) (cmsMsg + 1);
							 char interface[IFNAME_SIZE];
							 snprintf(interface, sizeof(interface), "eth%d",(int)(linkUpDownMsg->layer2InterfaceName[3]-'0'));
							 cmsLog_debug("Received CMS_MSG_ETH_LINK_DOWN for interface %s",interface);
							 inActivateAttachedDevices(interface, REMOVE_ON_IFNAME);
							 //sky_DeviceMonitorRemoveIPv6AddressesonIf(interface); TODO
						 }
	 					 break;
						 
					case CMS_MSG_ETH_LINK_UP:
						break;
#if 0
					case CMS_MSG_WLAN_ASSOC_DISACCOC:		
						cmsLog_debug ("  Link UP/DOWN received  process attached devices ");
						iteration = 1;
	//					start_lsnet(iteration);
						break;
					case CMS_MSG_REFRESH_ATTACHED_DEVICES:	
						cmsLog_debug ("  CMS_MSG_REFRESH_ATTACHED_DEVICES  process attached devices once");
						/* This message is received only when DHCP message is received
						No need to run multiple times, just run once
						*/
						iteration = 1;
	//					start_lsnet(iteration);
						break;
#endif	
					case CMS_MSG_WLAN_ASSOC:	
						macAddr = (char *)(cmsMsg + 1);
						cmsLog_debug("CMS_MSG_WLAN_ASSOC  Message received for MAC=%s", macAddr);
						break;
						
					case CMS_MSG_WLAN_DISASSOC:	
						macAddr = (char *)(cmsMsg + 1);
						cmsLog_debug("CMS_MSG_WLAN_DISASSOC  Message received for MAC=%s", macAddr);
						inActivateAttachedDevices(macAddr, REMOVE_ON_MAC);
						break;
					case CMS_MSG_DHCPD_HOST_INFO:
						processDhcpMsg(cmsMsg);
						break;
					default :
						break;

		    	}
				/* Leak fix for mes header in receive message */	
				CMSMEM_FREE_BUF_AND_NULL_PTR(cmsMsg);
	   		}
		}
		gettimeofday(&currTime,NULL);
		if(currTime.tv_sec - prevTime.tv_sec >= LSNET_REFRESH_INTERVAL )
		{
			//cmsLog_error("Calling start_lsnet() time - %ld",currTime.tv_sec);
			start_lsnet(0);
			prevTime.tv_sec = currTime.tv_sec;
		}
		else
		{
			//cmsLog_error("updating remaining time = %ld for refresh ", (currTime.tv_sec - prevTime.tv_sec));
			timeVal.tv_sec = (currTime.tv_sec - prevTime.tv_sec);
			if(timeVal.tv_sec == 0)
				timeVal.tv_sec = 1;
	   		timeVal.tv_usec = 0;
		}

   	}
 return 0;
}

static void registerCMSmessageOfInterest (CmsMsgType msgType, char * name)
{
	CmsRet ret;
	CmsMsgHeader msg = EMPTY_MSG_HEADER;
   cmsLog_debug("registering interest for %s event msg (id %X)", name?name:"", msgType);
   
   memset(&msg, 0, sizeof(CmsMsgHeader));
   msg.type = CMS_MSG_REGISTER_EVENT_INTEREST;
   msg.src = EID_LSNET;
   msg.dst = EID_SMD;
   msg.flags_request = 1;
   msg.wordData = msgType;

   
   if ((ret = cmsMsg_sendAndGetReply(msgHandle, &msg)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not register for event message %s (id %X), returned: %d", name?name:"", msgType,ret);
   }
}

static void processDhcpMsg(CmsMsgHeader *msg)
{
   DhcpdHostInfoMsgBody *body = (DhcpdHostInfoMsgBody *) (msg + 1);

   if (msg->dataLength != sizeof(DhcpdHostInfoMsgBody))
   {
      cmsLog_error("bad data length, got %d expected %d, drop msg",
                   msg->dataLength, sizeof(DhcpdHostInfoMsgBody));
      return;
   }

   cmsLog_debug("Entered: delete=%d ifName=%s ipAddr=%s macAddr=%s hostName=%s",
                body->deleteHost, body->ifName, body->ipAddr, body->macAddr, body->hostName);
   if (body->deleteHost)
   {
		if((body->macAddr != NULL) && (body->macAddr[0] != '\0'))
		{
			cmsLog_error("removing  Device as it is removed by DHCP "); 
			inActivateAttachedDevices(body->macAddr, REMOVE_ON_MAC);
		}
   } /* deleteHost */
   else
   {
		cmsLog_debug("adding  Device as it is added by DHCP "); 
		addModifyLanDevice(body->ipAddr,body->hostName,body->macAddr, 1, TRUE, "DHCP");
   } /* add or edit */


}


static void start_lsnet(int startup)
{
    struct timeval currTime, prevTime;
	gettimeofday(&prevTime,NULL);

	cmsLog_debug("Starting lsnet Processing, time=%ld",prevTime.tv_sec);
	
	getLanIPAddress();

	// prepare the devices and identify the devices which needs reachability test
    cmsLog_debug("prepare the devices for reachability test");	
	prepareLanDevicesForReachabilityTest();

	/* We assume that devices present in wlan assoclist will be still connected.
	   the devices might go away from the list  in approximately 1 minute,
	   if they are disconnected  without sending DISSASSOC event.
	   so dont test reachabality for those devices
	*/
	testReachabilityUsingWirelessAssocList();

	/* For the remaining devices, test the reachabality using arp scan 
 	    This is topic for further optimization, can we stop doing arpscan every 1 sec, 

 	    one solution is to, test the reachability only when the attached devices are accessed from TR69, 
 	    or when attached devices table is refreshed from WebGUI.

 	    or we have to think of better alternate solution
	
	*/
	if(createArpScanList() == 0)
	{		
		cmsLog_debug("No hosts for refresh, returning");	
		return;
	}
	else
	{
		cmsLog_debug("Starting arp_scan processing");	
		testReachabilityUsingArpScan();
		if(processArpScanResults() > 0)
		{
		    cmsLog_debug("Completed arp_scan processing");			
		}
		else
		    cmsLog_debug("Completed arp_scan processing, with no devices verified");	

	}

	inActivateNonReachableDevices();
	gettimeofday(&currTime,NULL);
	cmsLog_debug("lsnet took %d seconds to refresh connected devices ", (currTime.tv_sec - prevTime.tv_sec));	

    cmsLog_debug(" Completed lsnet Processing");
  //sendLsnetCompletedEventToSSK();
}

static void inActivateAttachedDevices(char *key, DeviceRemoveFilter filter)
{

	PLANDevice curLD = NULL;
	PLANDevice prevLD = NULL;
	int noInactiveDevices=0;
	char delString[64];
//	char cmdLine[256];

 	pthread_mutex_lock(&deviceListMutex);
	curLD = lanDeviceListStart;
	while(curLD != NULL)
	{
		if(filter == REMOVE_ON_IFNAME)
		{
			strcpy(delString, curLD->ifName);
		    cmsLog_debug(" Delete devices connected to Interface %s",key);
		}
		else if(filter == REMOVE_ON_MAC)
		{
			strcpy(delString, curLD->MAC);
		    cmsLog_debug(" Delete device with MAC %s",key);
		}
		else if(filter == REMOVE_ON_IP)
		{
			strcpy(delString, curLD->IP);
		    cmsLog_debug(" Delete device with IP %s",key);
		}
		else
		{
		    cmsLog_error(" Invalid Remove Filter");
			break;
		}
		
		if(cmsUtl_strcasecmp(delString, key) == 0)
		{

			cmsLog_debug(" Inactivating device wth MAC = %s, IP - %s ",curLD->MAC, curLD->IP);
			curLD->state = DEVICE_ENTRY_NOT_ACTIVE;
			updateAttachedDevice(curLD, TRUE);
			strcpy(curLD->ifName, "NULL");
			// to be safer and not to impact on FAP or flowcache flow
			// we dont want to flush the ARP entry 
			//sprintf(cmdLine, "arp -d %s", curLD->IP);
			//system(cmdLine);
			noInactiveDevices++;		
		}
		prevLD = curLD;
		curLD = curLD->Next;
	}

	cmsLog_debug(" Inactivaed %d devices key= %s",noInactiveDevices, key);
 	pthread_mutex_unlock(&deviceListMutex);
	return;
}

#if 0
static void removeNonReachableDevices()
{

	PLANDevice curLD = NULL;
	PLANDevice prevLD = NULL;
	int noDeletedDevices=0;

 	pthread_mutex_lock(&deviceListMutex);
	curLD = lanDeviceListStart;
	while(curLD != NULL)
	{
		if(curLD->state == DEVICE_ENTRY_TEST_REACHABILITY)
		{
			if(curLD == lanDeviceListStart)
			{
				lanDeviceListStart = lanDeviceListStart->Next;
	  		    cmsLog_debug(" Deleted device wth IP = %s ",curLD->IP);
				curLD->state = DEVICE_ENTRY_NOT_ACTIVE;
				updateAttachedDevice(curLD, TRUE);
				strcpy(curLD->ifName, "NULL");
				//cmsMem_free(curLD);
				curLD = lanDeviceListStart;
			}
			else
			{
				if(prevLD)
					/*@i@*/prevLD->Next = curLD->Next;
	  		    cmsLog_debug(" Deleted device wth IP = %s ",curLD->IP);
				curLD->state = DEVICE_ENTRY_NOT_ACTIVE;
				updateAttachedDevice(curLD, TRUE);
				strcpy(curLD->ifName, "NULL");
				cmsMem_free(curLD);
				if(prevLD)
					curLD = prevLD->Next;			
				
			}
			noDeletedDevices++;		
		}
		else
		{
			prevLD = curLD;
			curLD = curLD->Next;
		}
	}

	cmsLog_debug(" Deleted %d devices",noDeletedDevices);
 	pthread_mutex_unlock(&deviceListMutex);
	return;
}

#endif

static void inActivateNonReachableDevices()
{

	PLANDevice curLD = NULL;
	PLANDevice prevLD = NULL;
	int noInActivatedDevices=0;

 	pthread_mutex_lock(&deviceListMutex);
	curLD = lanDeviceListStart;
	while(curLD != NULL)
	{
		if(curLD->state == DEVICE_ENTRY_TEST_REACHABILITY)
		{

			cmsLog_debug(" Inactivate  device wth MAC - %s, IP = %s ",curLD->MAC, curLD->IP);
			curLD->state = DEVICE_ENTRY_NOT_ACTIVE;
			updateAttachedDevice(curLD, TRUE);		
			strcpy(curLD->ifName, "NULL");
			noInActivatedDevices++;		
		}
		else
		{
			prevLD = curLD;
			curLD = curLD->Next;
		}
	}

	cmsLog_debug(" InActivated  %d devices",noInActivatedDevices);
 	pthread_mutex_unlock(&deviceListMutex);
	return;
}

#if 0

/* 

   This function is invoked when  when arp cache entry is deleted, flag the entry as removed from cache. 
   and do arping to check if the device is still connected. 
   
   a. Device Still connected :
      If device is still connected new ARP entry is added, then change the 
      device entry state from DEVICE_ENTRY_REMOVED_FROM_CACHE to DEVICE_ENTRY_MODIFIED.
      
   b. Device is not connected :
   	If the device is actually disconncted, we will have a new arp cache entry added with state as NUD_FAILED
   	then completely remove the device from the list

*/
   
static void removeModifyLanDevice(char *IP, char *MAC, UBOOL8 deletePermanently)
{
	PLANDevice currLD = NULL;
	PLANDevice prevLD = currLD;
	char cmdLine[256];

 	pthread_mutex_lock(&deviceListMutex);
	currLD = lanDeviceListStart;

	while(currLD != NULL)
	{
		if((deletePermanently)  && (!strcmp(currLD->IP, IP)))
		{
			cmsLog_error("Device Entry Deleted Permanently Cache Entry Removed for IP - %s, MAC - %s, flag it as Removed from Cache", IP, MAC);
//			removeAttachedDevices(currLD->IP, REMOVE_ON_IP);
		 	pthread_mutex_unlock(&deviceListMutex);
			return;
		}
		else if(!strcmp(currLD->IP, IP))
		{
			cmsLog_error("ARP Cache Entry Removed for IP - %s, MAC - %s, flag it as Removed from Cache", IP, MAC);
			currLD->state = DEVICE_ENTRY_REMOVED_FROM_CACHE;
			sprintf(cmdLine, "arping  -c 2 -w 1 -f -I br0 %s",currLD->IP);
			system(cmdLine);
		 	pthread_mutex_unlock(&deviceListMutex);
			return;
		}
		currLD = currLD->Next;
	}

 	pthread_mutex_unlock(&deviceListMutex);

	return;		
}

#endif

static void modifyLanDeviceState(char *IP, char *MAC, DeviceState state)
{
	PLANDevice currLD = NULL;
    struct timeval currTime;

	gettimeofday(&currTime, NULL);

	pthread_mutex_lock(&deviceListMutex);
	currLD = lanDeviceListStart;

	while(currLD != NULL)
	{
		if((cmsUtl_strcasecmp(currLD->IP, IP) == 0) && (cmsUtl_strcasecmp(currLD->MAC, MAC) == 0))
		{
			cmsLog_debug("Lan device Entry Reachability Confirmed from ARPSCAN  IP - %s, MAC - %s", IP, MAC);
			currLD->state = state;
			currLD->lastActiveTime = currTime.tv_sec;
		 	pthread_mutex_unlock(&deviceListMutex);
			return;
		}
		currLD = currLD->Next;
	}	
 	pthread_mutex_unlock(&deviceListMutex);
	return; 	
}


static void modifyLanDeviceStateByMAC(char *MAC, DeviceState state)
{
	PLANDevice currLD = NULL;
    struct timeval currTime;

	gettimeofday(&currTime, NULL);
 	pthread_mutex_lock(&deviceListMutex);
	currLD = lanDeviceListStart;

	while(currLD != NULL)
	{
		if((cmsUtl_strcasecmp(currLD->MAC, MAC) == 0) )
		{
			if(currLD->state != DEVICE_ENTRY_NOT_ACTIVE)
			{
				cmsLog_debug("Lan device Entry(Wireless) Reachability Confirmed from ASSOCLIST  MAC - %s", MAC);
				currLD->state = state;
				currLD->lastActiveTime = currTime.tv_sec;
			}
		 	pthread_mutex_unlock(&deviceListMutex);
			return;
		}
		currLD = currLD->Next;
	}	
 	pthread_mutex_unlock(&deviceListMutex);
	return; 	
}


static void prepareLanDevicesForReachabilityTest()
{
	PLANDevice currLD = NULL;
	struct timeval currTime;
	int noTestDevices=0;

	pthread_mutex_lock(&deviceListMutex);
	currLD = lanDeviceListStart;

	gettimeofday(&currTime,NULL);	
	while(currLD != NULL)
	{
		if((currLD->state != DEVICE_ENTRY_NOT_ACTIVE) && 
			((cmsUtl_strcmp(currLD->ifName, "wl0") == 0) || ((currTime.tv_sec - currLD->lastActiveTime ) > LATEST_ACTIVE_TIME_DIFF_IN_SEC)))
		{
			cmsLog_debug("timeDiff = %d",(currTime.tv_sec - currLD->lastActiveTime ));
			currLD->state = DEVICE_ENTRY_TEST_REACHABILITY;
			noTestDevices++;
		}
		currLD = currLD->Next;
	}	
	pthread_mutex_unlock(&deviceListMutex);
	cmsLog_debug("noTestDevices - %d",noTestDevices);	
	return; 
}

/* create a new LAN device and append it to the list */
static void addModifyLanDevice(char *IP, char *hostName, char *MAC, int create,
		UBOOL8 hostsFromOtherModules, char *addressSource)
{
	PLANDevice newLD = NULL;
	PLANDevice currLD = NULL;
	PLANDevice prevLD = currLD;
	int	updated = 0;
	char hName[HOSTNAME_LEN]=DEFAULT_HOSTNAME;
	struct timeval currTime;
//	char cmdLine[256];
	char oldIP[IP_ADDR_LEN];
	

	gettimeofday(&currTime,NULL);
	pthread_mutex_lock(&deviceListMutex);
	currLD = lanDeviceListStart;

	/* walk the list looking for duplicate IPs */
	while(currLD != NULL)
	{
		if ((cmsUtl_strcasecmp(MAC, DEFAULT_MAC) != 0) && (cmsUtl_strcasecmp(currLD->MAC, MAC) == 0))
		{
			if (cmsUtl_strcasecmp(IP, currLD->IP) != 0) 
			{
     	         cmsLog_debug("IP Address changed  for MAC - %s, OLD  IP - %s is replaced with NEW IP - %s ",currLD->MAC, currLD->IP, IP);
				 /* copy across fields */
				 
 				 cmsUtl_strncpy(oldIP, currLD->IP, IP_ADDR_LEN);
				 cmsUtl_strncpy(currLD->IP, IP, IP_ADDR_LEN);

				 if(hostsFromOtherModules)
			 	 {
			 	 	/* this is host update from DHCP, if we already have an entry, give priority to DHCP over static, 
			 	 	hence update the fileds */

					strncpy(currLD->HostName, hostName, HOSTNAME_LEN);
					currLD->hostNameResolvedTime = currTime.tv_sec;
					
			 	 }
				 else
				 {
				 	/* as we look for ARP cache stale events, we should delete ARP cache entry for Old IP
				 	otherwise we might get  neighbour event with old IP and MAC mapping

					Ex: Connect Set Top Box, configure to use static IP and change the IP address, 
					you will see the old IP addess stays and you will see new IP address and Old Ip Adrress, 
					flapping in attached devices list.				 	
				 	*/
				 	// to be safer and not to impact on FAP or flowcache flow
					// we dont want to flush the ARP entry 
	 				//sprintf(cmdLine, "arp -d %s", oldIP);
					//system(cmdLine);
					
				 	 if((currTime.tv_sec - currLD->hostNameResolvedTime) > TWENTY_FOUR_HOURS_IN_SEC )
				 	 {
				 		 if(resolveHostNameUsingNbtScan(currLD->IP, hName))
					  	 {
							cmsLog_debug("Host Name Resolved for IP=%s, HOSTNAME=%s",currLD->IP, hName);
							strncpy(currLD->HostName, hName, HOSTNAME_LEN);
							currLD->hostNameResolvedTime = currTime.tv_sec;
					     }
				 		 else
				 		 {
							cmsLog_debug("Could Not resolve Host Name  for IP=%s",currLD->IP);
							/* To make sure we wont resolve hostname too many times, when IP address is changing, lets
							set the time, this will help when the hosts doesnt have hostname at all. Its good to display the name as
							unknown instead of making too many requests */
							currLD->hostNameResolvedTime = currTime.tv_sec;
				 		 }
				 	 }
				 }

				 strcpy(currLD->addressSource, addressSource);

				 // MAP the device interface
				 mapDeviceInterface(currLD);
				 
				 if(currLD->state == DEVICE_ENTRY_NOT_ACTIVE)
				 {
					 cmsLog_debug("Device Entry State changed from DEVICE_ENTRY_NOT_ACTIVE to DEVICE_ENTRY_MODIFIED, IP - %s, MAC - %s, HOSTNAME - %s", 
							currLD->IP, currLD->MAC, currLD->HostName);
				 }
				 currLD->state = DEVICE_ENTRY_MODIFIED; 	
			 	 if(!hostsFromOtherModules) 
					 updateAttachedDevice(currLD, FALSE);

			}
			else
			{
					if(hostsFromOtherModules)
				 	{
			 	 		/* this is host update from DHCP, if we already have an entry, give priority to DHCP over static, 
				 	 	hence update the fileds */

						strncpy(currLD->HostName, hostName, HOSTNAME_LEN);
						strcpy(currLD->addressSource, addressSource);
						currLD->hostNameResolvedTime = currTime.tv_sec;

						// MAP the device interface
						mapDeviceInterface(currLD);
						
			 		}
					else  if(currLD->state == DEVICE_ENTRY_NOT_ACTIVE)
					{
						currLD->state = DEVICE_ENTRY_REFRESHED;

						/* even if the device has  the same IP address and MAC, its possible that its host name has
						been changed to try to resolve the hostname again if the resolved time is more than 24 hours
						*/
						
						if((currTime.tv_sec - currLD->hostNameResolvedTime) > TWENTY_FOUR_HOURS_IN_SEC )
						{
				 			if(resolveHostNameUsingNbtScan(currLD->IP, hName))
					  	 	{
								cmsLog_debug("Host Name Resolved for IP=%s, HOSTNAME=%s",currLD->IP, hName);
								strncpy(currLD->HostName, hName, HOSTNAME_LEN);
								currLD->hostNameResolvedTime = currTime.tv_sec;
						     }
					 		 else
								cmsLog_debug("Could Not resolve Host Name  for IP=%s",currLD->IP);
				 		}

						// MAP the device interface
						mapDeviceInterface(currLD);

						updateAttachedDevice(currLD, FALSE);
						cmsLog_debug("Device Entry State changed from DEVICE_ENTRY_NOT_ACTIVE to DEVICE_ENTRY_MODIFIED, IP - %s, MAC - %s, HOSTNAME - %s", 
							currLD->IP, currLD->MAC, currLD->HostName);
					}
					else
					{
						cmsLog_debug("Entry Already exists and ACTIVE  IP - %s,  OLD MAC - %s , HOSTNAME - %s",currLD->IP, currLD->MAC, currLD->HostName);
						// try to map the device  interface again if it is NULL
						//its possible at the first instance we might not able to map the interface again
						//so try agiin
						if(cmsUtl_strcmp(currLD->ifName,"NULL") == 0)
							mapDeviceInterface(currLD);
					}
					
					currLD->state = DEVICE_ENTRY_REFRESHED;
					strcpy(currLD->addressSource, addressSource);
					
			}
			currLD->lastActiveTime = currTime.tv_sec;
 			updated = 1;
			break;
		}
		else
		{
			prevLD = currLD;
			currLD = currLD->Next;
		}
	}
	
	/* if we've been through the list and not updated, this is a new entry */
	if ((!updated) && create)
	{

		/* malloc a new LanDevice */
		newLD = cmsMem_alloc(sizeof(LanDevice), ALLOC_ZEROIZE);
		
		if (newLD == NULL)
		{
			cmsLog_error("addModifyLanDevice() - cmsMem_alloc failed!");
		 	pthread_mutex_unlock(&deviceListMutex);
			return ;
		}		
		
		
		/* initialise LanDevice */
		memset(newLD, 0, sizeof(LanDevice));

		/* copy across fields */
		strncpy(newLD->IP,
				IP,
				sizeof(newLD->IP));
		newLD->IP[sizeof(newLD->IP) - 1] = '\0';
		
		if (hostName != NULL)
		{
			strncpy(newLD->HostName,
					hostName,
					sizeof(newLD->HostName));
		}
		else
		{
			strncpy(newLD->HostName,
					DEFAULT_HOSTNAME,
					sizeof(newLD->HostName));
		}
		newLD->HostName[sizeof(newLD->HostName) - 1] = '\0';

		if (MAC != NULL  && cmsUtl_strncasecmp(MAC,"00:00:00:00:00:00",17) && (strlen(MAC)>=17))
		{
			strncpy(newLD->MAC,
					MAC,
					sizeof(newLD->MAC));
		}
		else
		{
			strncpy(newLD->MAC,
					DEFAULT_HOSTNAME,
					sizeof(newLD->MAC));
		}
		newLD->MAC[sizeof(newLD->MAC) - 1] = '\0';
		strcpy(newLD->ifName, "NULL");
		newLD->state = DEVICE_ENTRY_CREATED;
		strcpy(newLD->addressSource, addressSource);


		if (MAC != NULL  && cmsUtl_strcasecmp(MAC,"00:00:00:00:00:00"))
		{
			// MAP the device interface
			mapDeviceInterface(newLD);
		}
#if 0		
		else
		{
			/*   this is the case when  
				the interface goes down, we remove the entry from our list,
				btu the entry is still present in ARP cache, and we might get Neighbour add message with 
				STALE state. 

				So check if the MAC is actually present in Bridge forwarding entry, 
				if not then dont add it to list 
			*/

			cmsLog_error("Device Entry will not be added as the MAC - %s is not present in Bridge ", MAC);
			cmsMem_free(newLD);
		 	pthread_mutex_unlock(&deviceListMutex);
			return NULL;
		}
#endif		
		if(!hostsFromOtherModules)
		{
			if(resolveHostNameUsingNbtScan(newLD->IP, hName))
			{
				cmsLog_debug("Host Name Resolved for IP=%s, HOSTNAME=%s",newLD->IP, hName);
				strncpy(newLD->HostName, hName, HOSTNAME_LEN);
				newLD->hostNameResolvedTime = currTime.tv_sec;
			}
			else
				cmsLog_debug("Could Not resolve Host Name  for IP=%s",newLD->IP);

		}
		
		/* is this the first entry? */		
		if (lanDeviceListStart == NULL)
		{
			lanDeviceListStart = newLD;
		}
		else
		{
			/* add to end of list */
			/*@i@*/prevLD->Next = newLD;
		}
		
		if(!hostsFromOtherModules) 
		{
			/*@i@*/updateAttachedDevice(newLD, FALSE);
			cmsLog_debug("New Attached Device Entry  Added IP=%s, MAC=%s, HOSTNAME=%s",newLD->IP, newLD->MAC, newLD->HostName);
		}

		/*@i@*/newLD->lastActiveTime= currTime.tv_sec;
	}
	
 	pthread_mutex_unlock(&deviceListMutex);
	return;
}

/* walks the list of LanDevices, printing as it goes */
static void printLanDevices(char* fileName)
{
	char *DeviceStateString[] = {"DEVICE_ENTRY_CREATED",
								"DEVICE_ENTRY_MODIFIED",
								"DEVICE_ENTRY_REFRESHED",
								"DEVICE_ENTRY_TEST_REACHABILITY",
								"DEVICE_ENTRY_REACHABILITY_CONFIRMED",
								"DEVICE_ENTRY_NOT_ACTIVE"};

	PLANDevice curLD=NULL;
	FILE *fp=NULL;
	if ((fp = fopen(fileName, "w+")) == NULL )
       {
           cmsLog_error("could not open %s", fileName);
           return;
       }
	cmsLog_notice(" Printing attached Devices");
	cmsLog_notice(" =====================================");
	pthread_mutex_lock(&deviceListMutex);	
	curLD = lanDeviceListStart;
	while(curLD != NULL)
	{
		if (cmsUtl_strcasecmp(curLD->IP, lan_address))
		{
			//printf("%s,%s,%s\n",
			//		curLD->IP,
			//		curLD->HostName,
			//		curLD->MAC);
			fprintf(fp,"%s,%s,%s,%s,%s,%s,%d,%d\n",
					curLD->IP,
					curLD->HostName,
					curLD->MAC,
					DeviceStateString[curLD->state],
					curLD->ifName,
					curLD->addressSource, 
					curLD->lastActiveTime,
					curLD->hostNameResolvedTime);
			
			cmsLog_notice("Device = %s,%s,%s,%s,%s,%s,%d,%d",
					curLD->IP,
					curLD->HostName,
					curLD->MAC,
					DeviceStateString[curLD->state],
					curLD->ifName,
					curLD->addressSource,
					curLD->lastActiveTime,
					curLD->hostNameResolvedTime);
		}
		
		curLD = curLD->Next;
	}
	pthread_mutex_unlock(&deviceListMutex);	
	cmsLog_notice(" =====================================");
	fclose(fp);
}

#if 0
/* free the LanDevices, one by one */
static int readAndFlushAttachedDevices()
{
	PLANDevice curLD = lanDeviceListStart;
	FILE *devList = NULL;
	int noDevices=0, noArpCacheEntries=0, i;
	char arpEntries[255][16];
	UBOOL8 found = FALSE;
	
	if ((devList = fopen("/tmp/arpscan.input", "w")) == NULL )
    {
           cmsLog_error("could not open /tmp/arpscan.input");
           return 0;
    }
	else
	{
		noDevices = readArpCache(devList,arpEntries);
		noArpCacheEntries = noDevices;
	}
	
	while(curLD != NULL)
	{
		PLANDevice tmpLD = curLD->Next;
		if(devList)
		{
			found = FALSE;
			for(i=0;i<noArpCacheEntries;i++)
			{
				if(strcmp(curLD->IP, arpEntries[i]) == 0)
				{
					found = TRUE;
					break;
				}
			}
			if(!found)
			{
				cmsLog_debug(" adding IP addr : %s to arpscan.input file", curLD->IP);
				fprintf(devList, "%s\n",curLD->IP);
				noDevices++;
			}
		}
		cmsMem_free(curLD);
		curLD = tmpLD;
	}
	lanDeviceListStart=NULL;
	
	if(devList)
		fclose(devList);

	return noDevices;
}
#endif
/* get the IP address of LAN interface */
static void getLanIPAddress(void)
{
	FILE *ifcOutput = NULL;
	int found_ip = 0;
	char line[256];
	
	system("ifconfig br0 > /tmp/ifcbr0.txt");
	memset(line, 0, sizeof(line));
	
	ifcOutput = fopen("/tmp/ifcbr0.txt", "r");
	
	if (NULL == ifcOutput)
	{
		cmsLog_error("file open error");
		return ;
	}
	
	while(!found_ip)
	{
		char *tmp;
		char *tmp2;
		
		/* read a line */
		if (NULL == fgets(line, sizeof(line), ifcOutput))
		{
			printf("could not get LAN interface IP address!\n");
			fclose(ifcOutput);
			return ;
		}
		
		/* look for 'inet addr' section */
		tmp = strstr ( line, "inet addr:" );
		if (NULL != tmp)
		{
			/* found the 'inet addr' line, look for 'Bcast' */
			tmp2 = strstr( line, " Bcast:");
			
			if (NULL != tmp2)
			{
				int i;
				found_ip = 1;
				strncpy(lan_address, tmp + 10, tmp2 - (tmp + 10));
				lan_address[sizeof(lan_address) - 1] = '\0';
				
				for (i=0; i<sizeof(lan_address); ++i)
				{
					if ((lan_address[i] == '\n') ||
						(lan_address[i] == '\r') ||
						(lan_address[i] == '\t') ||
						(lan_address[i] == ' '))
					{
						lan_address[i] = '\0';
					}
				}
			}
			else
			{
				printf("could not read LAN interface IP address!\n");
				fclose(ifcOutput);
				return ;
			}
		}
	}
	
	fclose(ifcOutput);
	
    return;
}


static UBOOL8  resolveHostNameUsingNbtScan(char *ipAddr, char *hostName)
{
	char sysLine[256];
	memset(sysLine, 0, sizeof(sysLine));

	snprintf(sysLine,sizeof(sysLine),"nbtscan -m 1 %s > /tmp/nbstat.txt", ipAddr);
	cmsLog_debug("Running nbtcan with command %s",sysLine);
	system(sysLine);

	return (processNetbiosResults(hostName));
}
#if 0
/* makes a system call to nbtscan */
static void scanNetbios(void)
{
	char sysLine[256] = {0,};
	char inputFile[BUFLEN_128] = "/tmp/netbios.input";
#if 0
	char ip[16] = {0,};
	char *tmp=NULL;
	strncpy(ip,lan_address,sizeof(ip));
     tmp=strrchr(ip,'.');
     tmp++;
	tmp[0]='1';  
	tmp++;
	tmp[0]='\0';  
	/* build the system string */
	sprintf(sysLine,"nbtscan %s-254 -s , > /tmp/nbstat.txt",ip);
#endif	

	if(crateNetBiosInputFile(inputFile) > 0)
	{
		sprintf(sysLine,"nbtscan -f %s -s ,> /tmp/nbstat.txt", inputFile);
		system(sysLine);
	}
	else
		cmsLog_debug(" Not Scanning Netbios as no hosts doung in arpscan ");
}
#endif

/* add or modify LAN device objects based on results from NetBios scan */
static UBOOL8 processNetbiosResults(char *hostName)
{
	FILE *nbOutput = NULL;
	char line[256];
	char *pch;
	int count = 0;
	UBOOL8 hostResolved=FALSE;
	int lineCount=0;

	memset(line, 0, sizeof(line));

	nbOutput = fopen("/tmp/nbstat.txt", "r");
	if (NULL == nbOutput)
	{
		printf("file open error  file = /tmp/nbstat.txt \n");
		return FALSE;
	}

		
	/* read a line */
	while (NULL != fgets(line, sizeof(line), nbOutput))
	{
		lineCount++;
		if(lineCount<5)
			continue;
			
		count = 0;
		pch = strtok (line," ");
		
		while (pch != NULL)
		{
			switch(count)
			{
				case 1 :
					/* this is the hostname */
					strncpy(hostName, pch, HOSTNAME_LEN);
					hostName[HOSTNAME_LEN-1] = '\0';
					if(strcmp(hostName, "<unknown>") == 0)
						strcpy(hostName, DEFAULT_HOSTNAME);
					hostResolved = TRUE;
					break;
				default:
					/* don't care */
					break;
			}
			count ++;			
			pch = strtok (NULL, " ");
		}
	}

	fclose(nbOutput);	
    return hostResolved;
}


#if 0

/* add or modify LAN device objects based on results from NetBios scan */
static void processNetbiosResults(void)
{
	FILE *nbOutput = NULL;
	char line[256] = {0,};
	char *pch;
	int count = 0;
	char ip[16] = {0,};
	char host[17] = {0,};
	char mac[18] = {0,};
	char *c;

	nbOutput = fopen("/tmp/nbstat.txt", "r");
	
	if (NULL == nbOutput)
	{
		printf("file open error\n");
		return ;
	}

		
	/* read a line */
	while (NULL != fgets(line, sizeof(line), nbOutput))
	{
		memset(ip, 0, sizeof(ip));
		memset(host, 0, sizeof(host));
		memset(mac, 0, sizeof(mac));
		
		count = 0;
		pch = strtok (line,",");
		
		while (pch != NULL)
		{
			switch(count)
			{
				case 0 :
					/* this is the ip address */
					strncpy(ip, pch, sizeof(ip));
					ip[sizeof(ip)-1] = '\0';
					break;
				case 1 :
					/* this is the hostname */
					strncpy(host, pch, sizeof(host));
					host[sizeof(host)-1] = '\0';
					break;
				case 4 :
					/* this is the MAC */
					strncpy(mac, pch, sizeof(mac));
					mac[sizeof(mac)-1] = '\0';

					/* replace '-' with ':' */
					c = &mac[0];
					while(*c)
					{		
						if (*c == '-')
						{
							*c = ':';
						}
						c++;
					}
					break;
				default:
					/* don't care */
					break;
			}
			
			count ++;
			
			pch = strtok (NULL, ",");
		}
		
		/* add details to the list */
		(void)addModifyLanDevice(ip, host, mac, 0, 0);
	}

	fclose(nbOutput);
	
    return;
}

#endif

/* add or modify LAN device objects based on results from fping scan */
static int  processArpScanResults(void)
{
		FILE *arpOutput = NULL;
		char line[256];
		char *pch;
		int count = 0;
		char ip[16];
		char mac[18];
		int hostCount=0;
		
		arpOutput = fopen("/tmp/arpscan.txt", "r");
		
		if (NULL == arpOutput)
		{
			printf("file open error\n");
			return 0;
		}
		memset(line, 0, sizeof(line));
		memset(ip, 0, sizeof(ip));
		memset(mac, 0, sizeof(mac));

		
		/* read a line */
		while (NULL != fgets(line, sizeof(line), arpOutput))
		{
			memset(ip, 0, sizeof(ip));
			memset(mac, 0, sizeof(mac));
			
			count = 0;
			pch = strtok (line,WHITESPACE);
			
			while (pch != NULL)
			{
				switch(count)
				{
					case 0 :
						/* this is the ip address */
						strncpy(ip, pch, sizeof(ip));
						ip[sizeof(ip)-1] = '\0';
						break;
					case 1 :
						/* this is the MAC */
						strncpy(mac, pch, sizeof(mac));
						mac[sizeof(mac)-1] = '\0';
						break;
					default:
						/* don't care */
						break;
				}
				
				count ++;
				
				pch = strtok (NULL, WHITESPACE);
			}
			modifyLanDeviceState(ip, mac, DEVICE_ENTRY_REACHABILITY_CONFIRMED);
			hostCount++;		
		}
		
		fclose(arpOutput);
		
		return hostCount;
	}

#if 0
static void clearArpCache()
{
	FILE *arpCache = NULL;
	char line[256] = {0,};
	char cmd[256];
	arpCache =  fopen("/tmp/arpscan.txt", "r");
	int count=0;
	char *pch;
	
	if (NULL == arpCache) {
		printf("file open error\n");
		return ;
	}
	while( fgets( line, sizeof(line), arpCache ) != NULL ) {	

	    pch = strtok (line,WHITESPACE);
	    if(pch)
		{
			sprintf(cmd, "arp -i br0 -d %s",pch);
			printf("Clearing arp cache for IP Address - %s \n", pch);
			system(cmd);
		}
	}
	
}

static void signalAnyTime()
{
    int max_pid_length = 10;
    char buf[max_pid_length];
    char cmd[256];

    FILE *fs = fopen("/var/anytimeupnp.pid", "r");
    //printf("signalling anytimeupnp \n");
    if(fs)
    {
        while (fgets (buf, max_pid_length, fs)!=NULL)
        {
            sprintf(cmd,"kill -SIGUSR1 %s",buf);
            system(cmd);
        }
		fclose(fs);
    }
    else
    {
        printf("/var/anytimeupnp.pid doesn't exist\n");
    }
   
}
#endif

static void testReachabilityUsingArpScan()
{
	//printf(" \n Performing arpscan \n");
	//system("/bin/arpscan -I br0 -r 1 -i 50 -l -q -z /tmp/arpscan.txt");
	cmsLog_debug(" performing arpscan for hosts in the file");
//    system("/bin/arpscan -I br0 -r 3 -i 20 -t 100 -R -f /tmp/arpscan.input -q -z /tmp/arpscan.txt");
	system("/bin/arpscan -I br0 -r 2 -i 100 -t 1000 -R -f /tmp/arpscan.input -q -z /tmp/arpscan.txt");

}
#if 0
static int crateNetBiosInputFile(char *netbiosInput)
{
	PLANDevice curLD = lanDeviceListStart;
	int count=0;
	FILE *fp=NULL;
	cmsLog_debug(" Opening Betbios Input File %s ", netbiosInput);
	if ((fp = fopen(netbiosInput, "w+")) == NULL )
    {
           cmsLog_error("could not open %s", netbiosInput);
           return 0;
    }
	while(curLD != NULL)
	{
		cmsLog_debug(" adding IP address %s to netbios input file ", curLD->IP);
		
		fprintf(fp,"%s\n",curLD->IP);
		curLD = curLD->Next;
		count++;
	}
	fclose(fp);
	return count;

}

static int readArpCache( FILE * arp, char  arpEntries[][16])
{
	FILE *arpCache=NULL;
	char line[1024];
	char *pch=NULL;
	char ip[16];
	char device[32];
	int noHosts=0;
	if(arp == NULL)
		return 0;
	
	cmsLog_debug(" Reading Entries from arp cache");

	system("cat /proc/net/arp > /tmp/arpcache.out");
	if ((arpCache = fopen("/tmp/arpcache.out", "r")) == NULL )
    {
           cmsLog_error("could not open /tmp/arpcache.out");
           return 0;
    }
	else
	{
		int lineCount = 0;
		while (NULL != fgets(line, sizeof(line), arpCache))
		{
			//cmsLog_error(" line = %s ", line);
			memset(ip, 0, sizeof(ip));
			memset(device, 0, sizeof(device));
			lineCount++;
			if(lineCount == 1)
				continue;
			
		    int count=0;	
			pch = strtok (line," "); // space
			while (pch != NULL)
			{
				//cmsLog_error(" pch string = %s ", pch);
				switch(count)
				{
					case 0 :
						/* this is the ip address */
						strncpy(ip, pch, sizeof(ip));
						ip[sizeof(ip)-1] = '\0';
						break;
					
					case 5 :						
						strncpy(device, pch, sizeof(device));
						break;
					default:
						/* don't care */
						break;
				}
				
				count ++;
				
				pch = strtok (NULL, " ");
			}
			
			cmsLog_debug("device Name = %s, ip=%s", device, ip);

			if(cmsUtl_strncmp(device, "br0", strlen("br0")) == 0)
			{
				//cmsLog_error(" adding IP address = %s to arpscan.input", ip);
				fprintf(arp, "%s\n",ip);
				strcpy(arpEntries[noHosts], ip);
				cmsLog_debug("Adding IP to arpscan.input = %s", ip);
				noHosts++;
			}
		}
		fclose(arpCache);
		return noHosts;
	}

	
}

static CmsRet sendLsnetCompletedEventToSSK()
{
	char buf[sizeof(CmsMsgHeader)];
	CmsMsgHeader *msg=(CmsMsgHeader *) buf;
	CmsRet ret;


	cmsLog_debug("Writing Attached Devices to output file");	
	printLanDevices("/tmp/lsnet.out");
	

	memset (buf,0,sizeof(buf));
	msg->type = CMS_MSG_LSNET_COMPLETED;
	msg->src = EID_LSNET;
	msg->dst = EID_SSK;
	msg->flags_event = 1;
	msg->dataLength = 0; /* No data at the moment */

	if ((ret = cmsMsg_send(msgHandle, msg)) != CMSRET_SUCCESS)
	{
		cmsLog_error("could not send out CMS_MSG_LSNET_COMPLETED, ret=%d", ret);
	}
	else
	{
		cmsLog_notice("sent out CMS_MSG_LSNET_COMPLETED");
	}
	return ret;
}
#endif
int prefix_banner = 0;


static void reset_filter()
{
	memset(&filter, 0, sizeof(filter));
	filter.state = ~0;
}

int print_neigh(const struct sockaddr_nl *who, struct nlmsghdr *n, void *arg)
{
	FILE *fp = (FILE*)arg;
	struct ndmsg *r = NLMSG_DATA(n);
	UINT32 len = n->nlmsg_len;
	struct rtattr * tb[NDA_MAX+1]={NULL};
	char abuf[256];
	char ipAddr[16];
	char macAddr[18];

	memset(ipAddr,0,sizeof(ipAddr));
	memset(macAddr,0,sizeof(macAddr));

/*
	if (n->nlmsg_type != RTM_NEWNEIGH && n->nlmsg_type != RTM_DELNEIGH) {
		fprintf(stderr, "Not RTM_NEWNEIGH or RTM_DELNEIGH: %08x %08x %08x\n",
			n->nlmsg_len, n->nlmsg_type, n->nlmsg_flags);

		return 0;
	}

*/
	
	if (n->nlmsg_type != RTM_NEWNEIGH) {
//		fprintf(stderr, "Not RTM_NEWNEIGH: %08x %08x %08x\n",
	//		n->nlmsg_len, n->nlmsg_type, n->nlmsg_flags);

		return 0;
	}
	
	len -= NLMSG_LENGTH(sizeof(*r));
	if (len < 0) {
		fprintf(stderr, "BUG: wrong nlmsg len %d\n", len);
		return -1;
	}

	// we are only interested in these message, ignore rest of the states to optimize.
	if (r->ndm_state) {
//		if((r->ndm_state != NUD_REACHABLE) && (r->ndm_state != NUD_FAILED ) && (r->ndm_state != NUD_STALE))
		if((r->ndm_state != NUD_REACHABLE) && (r->ndm_state != NUD_STALE))

		{
			cmsLog_debug("STATE = %d is not one of  NUD_REACHABLE/NUD_FAILED/NUD_STALE , ignore the message",r->ndm_state );
			return 0;
		}
	}

/*
	
	if (filter.family && filter.family != r->ndm_family)
		return 0;
	
	if (!(filter.state&r->ndm_state) &&
	    (r->ndm_state || !(filter.state&0x100)) &&
             (r->ndm_family != AF_DECnet))
		return 0;

*/
	parse_rtattr(tb, NDA_MAX, NDA_RTA(r), (int)n->nlmsg_len - NLMSG_LENGTH(sizeof(*r)));
#if 0
	if (tb[NDA_DST]) {
		if (filter.pfx.family) {
			inet_prefix dst;
			memset(&dst, 0, sizeof(dst));
			dst.family = r->ndm_family;
			memcpy(&dst.data, RTA_DATA(tb[NDA_DST]), RTA_PAYLOAD(tb[NDA_DST]));
			if (inet_addr_match(&dst, &filter.pfx, filter.pfx.bitlen))
				return 0;
		}
	}
	if (filter.unused_only && tb[NDA_CACHEINFO]) {
		struct nda_cacheinfo *ci = RTA_DATA(tb[NDA_CACHEINFO]);
		if (ci->ndm_refcnt)
			return 0;
	}

	if (n->nlmsg_type == RTM_NEWNEIGH)
		fprintf(fp, "%s ","RTM_NEWNEIGH");
	else if(n->nlmsg_type == RTM_DELNEIGH)
		fprintf(fp, "%s ","RTM_DELNEIGH");

	

	if (!filter.index && r->ndm_ifindex)
		fprintf(fp, "dev %s ", ll_index_to_name(r->ndm_ifindex));
#endif

/*@i@ */	if(strcmp(ll_index_to_name((UINT32)r->ndm_ifindex), "br0") == 0)
	{
		// we are interested in only br0 interface
//		fprintf(fp, "dev %s ", ll_index_to_name(r->ndm_ifindex));

		if (tb[NDA_DST]) {
			/*
				fprintf(fp, "%s ",
					format_host(r->ndm_family,
							RTA_PAYLOAD(tb[NDA_DST]),
							RTA_DATA(tb[NDA_DST]),
							abuf, sizeof(abuf)));
				*/
				snprintf(ipAddr, sizeof(ipAddr), "%s",
				/*@i@*/	format_host(r->ndm_family,
							RTA_PAYLOAD(tb[NDA_DST]),
							RTA_DATA(tb[NDA_DST]),
							abuf, sizeof(abuf)));
				
		}

		if (tb[NDA_LLADDR]) {
			SPRINT_BUF(b1);
			/*
			fprintf(fp, "lladdr %s", ll_addr_n2a(RTA_DATA(tb[NDA_LLADDR]),
						      RTA_PAYLOAD(tb[NDA_LLADDR]),
						      ll_index_to_type(r->ndm_ifindex),
						      b1, sizeof(b1)));
						     */
			/*@ignore@*/
			snprintf(macAddr, sizeof(macAddr),"%s", ll_addr_n2a(RTA_DATA(tb[NDA_LLADDR]),
						      RTA_PAYLOAD(tb[NDA_LLADDR]),
						      ll_index_to_type((UINT32)r->ndm_ifindex),
						      b1, sizeof(b1)));			
			/*@end@*/
		}

		if (n->nlmsg_type == RTM_NEWNEIGH) {
			if ( (r->ndm_state) && (r->ndm_state == NUD_FAILED))
			{
				/*  THis is the case when we try to arping a device which is not connected
				this means we can safely delete this from our list */
				cmsLog_debug("Removing device IP=%s, MAC=%s", ipAddr, macAddr);
			//	removeModifyLanDevice(ipAddr, macAddr,TRUE);
				inActivateAttachedDevices(ipAddr, REMOVE_ON_IP);

			}
			else
			{
				cmsLog_debug("RTM_NEWNEIGH  IP=%s, MAC=%s", ipAddr, macAddr);
				addModifyLanDevice(ipAddr, DEFAULT_HOSTNAME, macAddr, 1, FALSE, "STATIC");
			}
		}
#if 0
		else if(n->nlmsg_type == RTM_DELNEIGH) {
			/* when entry is removed from cache just flag it for removal */
			cmsLog_error("Removing device IP=%s, MAC=%s", ipAddr, macAddr);
			removeModifyLanDevice(ipAddr, macAddr,FALSE);
		}
#endif
		
	}

#if 0	
	
	if (r->ndm_flags & NTF_ROUTER) {
		fprintf(fp, " router");
	}
	
	if (tb[NDA_CACHEINFO] && show_stats) {
		struct nda_cacheinfo *ci = RTA_DATA(tb[NDA_CACHEINFO]);
		int hz = get_user_hz();

		if (ci->ndm_refcnt)
			printf(" ref %d", ci->ndm_refcnt);
		fprintf(fp, " used %d/%d/%d", ci->ndm_used/hz,
		       ci->ndm_confirmed/hz, ci->ndm_updated/hz);
	}

#ifdef NDA_PROBES
	if (tb[NDA_PROBES] && show_stats) {
		__u32 p = *(__u32 *) RTA_DATA(tb[NDA_PROBES]);
		fprintf(fp, " probes %u", p);
	}
#endif

	if (r->ndm_state) {
		int nud = r->ndm_state;
		fprintf(fp, " ");

#define PRINT_FLAG(f) if (nud & NUD_##f) { \
	nud &= ~NUD_##f; fprintf(fp, #f "%s", nud ? "," : ""); }
		PRINT_FLAG(INCOMPLETE);
		PRINT_FLAG(REACHABLE);
		PRINT_FLAG(STALE);
		PRINT_FLAG(DELAY);
		PRINT_FLAG(PROBE);
		PRINT_FLAG(FAILED);
		PRINT_FLAG(NOARP);
		PRINT_FLAG(PERMANENT);
#undef PRINT_FLAG

	}

	fprintf(fp, "\n");
#endif

	fflush(fp);
	return 0;
}



int handleNeighbourMessages(const struct sockaddr_nl *who,
	       struct nlmsghdr *n, void *arg)
{
	FILE *fp = (FILE*)arg;

	cmsLog_debug(" nlmsg_type=%d",n->nlmsg_type);

	if (timestamp)
		print_timestamp(fp);

	if (n->nlmsg_type == RTM_NEWNEIGH || n->nlmsg_type == RTM_DELNEIGH) {
		print_neigh(who, n, arg);
		return 0;
	}
/*	
	if (n->nlmsg_type != NLMSG_ERROR && n->nlmsg_type != NLMSG_NOOP &&
	    n->nlmsg_type != NLMSG_DONE) {
		fprintf(fp, "Unknown message: %08x %08x %08x\n",
			n->nlmsg_len, n->nlmsg_type, n->nlmsg_flags);
	}
*/
	return 0;
}

void*  listen_for_netlink_messages(void *param)
{
	unsigned groups = (unsigned)~RTMGRP_TC;
	cmsLog_debug(" listen_for_netlink_messages  in new thread ");
	pthread_cleanup_push(threadCleanupHandler, NULL);
	rtnl_close(&rth);
	reset_filter();

	groups |= nl_mgrp(RTNLGRP_NEIGH);

	if (rtnl_open(&rth, groups) < 0)
		exit(1);
	ll_init_map(&rth);

	if (rtnl_listen(&rth, handleNeighbourMessages, stdout) < 0)
		exit(2);
	
	pthread_cleanup_pop(0);
	return NULL;
}




static void  mapBridgePortToIfName(int PortNo, char* ifName)
{
		char sysLine[256];
	
		FILE *brOutput = NULL;
		char line[256];
		char *pch;
		int count = 0;
		UBOOL8 ifFound=FALSE;
		int portNo=0;
		int lineCount=0;
	
		snprintf(sysLine,sizeof(sysLine), "brctl show > /tmp/showbridge.txt");
		system(sysLine);
	
		brOutput = fopen("/tmp/showbridge.txt", "r");
		if (NULL == brOutput)
		{
			printf("file open error  file = /tmp/showbridge.txt \n");
			return ;
		}
	
		memset(	line, 0, sizeof(line));
		/* read a line */
		while (NULL != fgets(line, sizeof(line), brOutput))
		{
			lineCount++;
			if(lineCount <= PortNo)
				continue;
			
			count = 0;
			pch = strtok (line,"\t");
			
			while (pch != NULL)
			{
				switch(count)
				{
				
					case 0 :
						if(lineCount!=2)
						{
							/* this is the hostname */
							strncpy(ifName, pch, IFNAME_SIZE);							
							ifName[strlen(ifName)-1] = '\0';
							ifFound = 1;
							}
						break;
					case 3 :
						if(lineCount==2)
						{
							/* this is the hostname */
							strncpy(ifName, pch, IFNAME_SIZE);
							ifName[strlen(ifName)-1] = '\0';
							ifFound = TRUE;
						}
						break;
					default:
						/* don't care */
						break;
				}
				if(ifFound)
				{
					cmsLog_debug("Interface found Interface=%s, portNo=%d",ifName, portNo);
					goto end;
				}
				count ++;			
				pch = strtok (NULL, "\t");
			}
		}
	
	end:	fclose(brOutput);	
		return ;

}


static int  getAttachedDeviceInterface(char *macAddr)
{
	char sysLine[256];

	FILE *brOutput = NULL;
	char line[256];
	char *pch;
	int count = 0;
	UBOOL8 macFound=FALSE;
	int portNo=0;

	snprintf(sysLine,sizeof(sysLine), "brctl showmacs br0 > /tmp/showmacs.txt");
	system(sysLine);

	brOutput = fopen("/tmp/showmacs.txt", "r");
	if (NULL == brOutput)
	{
		printf("file open error  file = /tmp/showmacs.txt \n");
		return 0;
	}

	memset( line, 0, sizeof(line));
		
	/* read a line */
	while (NULL != fgets(line, sizeof(line), brOutput))
	{
//		cmsLog_error("Line = %s",line)
		count = 0;
		pch = strtok (line,"\t");
		
		while (pch != NULL)
		{
			switch(count)
			{
				case 0 :
					portNo = atoi(pch);
					break;
				case 1 :
					/* this is the hostname */
					if(cmsUtl_strcasecmp(macAddr, pch) == 0)
					{
						macFound = TRUE;
					}
					break;
				default:
					/* don't care */
					break;
			}
			if(macFound)
			{
				cmsLog_debug("MAC Address found MAC=%s, portNo=%d",pch, portNo);
				goto end;
			}
			count ++;			
			pch = strtok (NULL, "\t");
		}
	}

	portNo=0;

end:	fclose(brOutput);	
    return portNo;

	
}


void updateAttachedDevice(PLANDevice deviceEntry,  UBOOL8 isDelete)
{
/*@i@*/   char buf[sizeof(CmsMsgHeader) + sizeof(DhcpdHostInfoMsgBody)] = {0};
   CmsMsgHeader *hdr = (CmsMsgHeader *) buf;
   DhcpdHostInfoMsgBody *body = (DhcpdHostInfoMsgBody *) (hdr+1);
   CmsRet ret;

   if((!isDelete) && (deviceEntry->state ==  DEVICE_ENTRY_NOT_ACTIVE ))
   {
   		cmsLog_error("Should not have come here, Updating/Adding Device which is not active, IP - %s, MAC-%s",deviceEntry->IP, deviceEntry->MAC);
		return;
   }
   else if((isDelete) && (deviceEntry->state !=  DEVICE_ENTRY_NOT_ACTIVE ))
   {
   		cmsLog_error("Should not have come here, Updating/deleting Device which is active, IP - %s, MAC-%s",deviceEntry->IP, deviceEntry->MAC);
		return;
   }

 	hdr->type = CMS_MSG_LSNET_HOST_INFO;
	hdr->src = EID_LSNET;
	hdr->dst = EID_SSK;
	hdr->flags_event = 1;
    hdr->dataLength = sizeof(DhcpdHostInfoMsgBody);
    body->deleteHost = isDelete;

	cmsLog_debug("Updating Device Entry, IP=%s, MAC=%s, HostName=%s, ifName=%s, isDelete=%d",
		deviceEntry->IP, deviceEntry->MAC, deviceEntry->HostName, deviceEntry->ifName, isDelete);

    snprintf(body->ifName, sizeof(body->ifName), "br0");
    snprintf(body->ipAddr, sizeof(body->ipAddr), "%s", deviceEntry->IP);
    snprintf(body->hostName, sizeof(body->hostName), "%s", deviceEntry->HostName);
	snprintf(body->macAddr, sizeof(body->hostName), "%s", deviceEntry->MAC);
	body->leaseTimeRemaining = 0;

  	
    /* does DHCP include the statically assigned addresses?  Or should that be STATIC? */
	snprintf(body->addressSource, sizeof(body->addressSource), "%s", deviceEntry->addressSource);

	
	if(cmsUtl_strstr(deviceEntry->ifName, "eth"))
	    /* is there a way we can tell if we assigned this address to a host on WLAN? */
    	snprintf(body->interfaceType, sizeof(body->interfaceType), MDMVS_ETHERNET);
	else if(cmsUtl_strstr(deviceEntry->ifName, "wl"))
	    /* is there a way we can tell if we assigned this address to a host on WLAN? */
    	snprintf(body->interfaceType, sizeof(body->interfaceType), MDMVS_802_11);
	else
	    snprintf(body->interfaceType, sizeof(body->interfaceType), "Other");
	
   if ((ret = cmsMsg_send(msgHandle, hdr)) != CMSRET_SUCCESS)
   {
      cmsLog_error("could not update device");
   }
   else
   {
      cmsLog_debug("Device update send to SSK");
   }
}


static int createArpScanList()
{
	PLANDevice curLD = lanDeviceListStart;
	FILE *devList = NULL;
	int noDevices=0;

	if ((devList = fopen("/tmp/arpscan.input", "w")) == NULL )
    {
           cmsLog_error("could not open /tmp/arpscan.input");
           return 0;
    }
 	pthread_mutex_lock(&deviceListMutex);
	while(curLD != NULL)
	{
		/* do not do apring for wireless devices, 
		  its observed that wireless devices will not respond to 
		  arping consistantly
		 */
		if(curLD->state ==  DEVICE_ENTRY_TEST_REACHABILITY)
		{
			cmsLog_debug(" adding IP addr : %s to arpscan.input file", curLD->IP);
			if(cmsUtl_strcasecmp(curLD->MAC,DEFAULT_MAC) != 0)
				fprintf(devList, "%s %s\n",curLD->IP,curLD->MAC);
			else
				fprintf(devList, "%s %s\n",curLD->IP,"ff:ff:ff:ff:ff:ff");
			
			noDevices++;
		}
		curLD = curLD->Next;
	}
 	pthread_mutex_unlock(&deviceListMutex);
	
	if(devList)
		fclose(devList);

	cmsLog_debug("Exit, noDevices = %d",noDevices);
	return noDevices;
}


static void testReachabilityUsingWirelessAssocList()
{
	FILE *brOutput = NULL;
	char line[256];
	char *pch;
	int count = 0;
	int noDevices=0;
	char macAddr[MACADDR_LEN];

	cmsLog_debug("enter");
	system("wlctl -i wl0 assoclist > /tmp/lsnetassoclist24Ghz.txt");
	system("wlctl -i wl1 assoclist > /tmp/lsnetassoclist5Ghz.txt");
	
	system("cat /tmp/lsnetassoclist24Ghz.txt  /tmp/lsnetassoclist5Ghz.txt > /tmp/lsnetassoclist.txt");

	brOutput = fopen("/tmp/lsnetassoclist.txt", "r");
	if (NULL == brOutput)
	{
		cmsLog_error("file open error  file = /tmp/lsnetassoclist.txt \n");
		return;
	}

	memset(line, 0, sizeof(line));		
	/* read a line */
	while (NULL != fgets(line, sizeof(line), brOutput))
	{
//		cmsLog_error("Line = %s",line)
		count = 0;
		pch = strtok (line," ");
		
		while (pch != NULL)
		{
			switch(count)
			{
				case 1 :
					noDevices++;
					snprintf(macAddr,sizeof(macAddr),"%s",pch);
					macAddr[strlen(macAddr)-1] = '\0';
					modifyLanDeviceStateByMAC(macAddr,DEVICE_ENTRY_REACHABILITY_CONFIRMED);
					break;
				default:
					/* don't care */
					break;
			}
			count ++;			
			pch = strtok (NULL, " ");
		}
	}

	fclose(brOutput);	
	cmsLog_debug("wlctl assoclist contains %d ", noDevices);
		
}

/***********************************************************************
* 	static UBOOL8 searchDeviceMacFromAssocList(char *MAC)
*  
* 	This function will search for mac address in wireless associated list
*
*  	INPUT
*  		char * MAC  : MAC address to search
*
* 	RETURN
* 		UBOOL8  : TRUE if MAC address is found
* 	 		          FALSE if MAC address is not found
 ************************************************************************/

static UBOOL8 searchDeviceMacFromAssocList(char *MAC)
{
	FILE *brOutput = NULL;
	char line[256];
	char *pch;
	int count = 0;
	int noDevices=0;
	char macAddr[MACADDR_LEN];

	cmsLog_debug("enter");
	system("wlctl -i wl0 assoclist > /tmp/lsnetassoclist24Ghz.txt");
	system("wlctl -i wl1 assoclist > /tmp/lsnetassoclist5Ghz.txt");
	
	system("cat /tmp/lsnetassoclist24Ghz.txt  /tmp/lsnetassoclist5Ghz.txt > /tmp/lsnetassoclist.txt");

	brOutput = fopen("/tmp/lsnetassoclist.txt", "r");
	if (NULL == brOutput)
	{
		cmsLog_error("file open error  file = /tmp/lsnetassoclist.txt \n");
		return FALSE;
	}
	memset(line, 0, sizeof(line));
	
	while (NULL != fgets(line, sizeof(line), brOutput))
	{
		count = 0;
		pch = strtok (line," ");
		
		while (pch != NULL)
		{
			switch(count)
			{
				case 1 :
					noDevices++;
					snprintf(macAddr,sizeof(macAddr),"%s",pch);
					macAddr[strlen(macAddr)-1] = '\0';
					if(cmsUtl_strcasecmp(MAC,macAddr) == 0)
					{
						cmsLog_debug("MAC address - %s found in wireless assoclist ",MAC);
						fclose(brOutput);
						return TRUE;
					}
					break;
				default:
					/* don't care */
					break;
			}
			count ++;			
			pch = strtok (NULL, " ");
		}
	}

	fclose(brOutput);	
	cmsLog_debug("MAC address - %s not found in wireless assoclist ",MAC);
	return FALSE;
	
		
}

/***********************************************************************
*	static void mapDeviceInterface(PLANDevice deviceEntry)
*
* 	This function will find out the interface where the device is connected to
* 	It will do so my looking at bridge mac learning table, if  find the MAC address 
* 	is found, it will get the corresponding  interace name by looking at brodge port table
* 	If the MAC is not found at bridge table, look at the MAC in wireless association list.
*
*  	INPUT:
* 		PLANDevice deviceEntry  : Device entry to search 
*
*	RETURN:
*		None
*
************************************************************************/

static void mapDeviceInterface(PLANDevice deviceEntry)
{
	int bridgePortNo=0;

	bridgePortNo = getAttachedDeviceInterface(deviceEntry->MAC);
	if(bridgePortNo != 0)
	{
		mapBridgePortToIfName(bridgePortNo, deviceEntry->ifName);
		cmsLog_debug("Resovled Host interface to ifName=%s",deviceEntry->ifName);
	}
	else if(searchDeviceMacFromAssocList(deviceEntry->MAC))
	{
		cmsLog_debug("Device is connected via wl0 interface");
		snprintf(deviceEntry->ifName,sizeof(deviceEntry->ifName),"wl0");		
	}
	else
	{
		cmsLog_debug("Device interface cannot be identified");
		snprintf(deviceEntry->ifName,sizeof(deviceEntry->ifName),"NULL");						
	}	
}

#if 0

/***********************************************************************
* 	static UBOOL8 isDevicePresentInARPCache(char *inputMAC)
*  
* 	This function will search for mac address in arp cache
*
*  	INPUT
*  		char * MAC  : MAC address to search
*
* 	RETURN
* 		UBOOL8  : TRUE if MAC address is found
* 	 		          FALSE if MAC address is not found
 ************************************************************************/

static UBOOL8 isDevicePresentInARPCache(char *inputMAC)
{
	FILE *arpCache=NULL;
	char line[1024];
	char *pch=NULL;
	char mac[MACADDR_LEN];

	cmsLog_debug(" Reading Entries from arp cache");

	system("cat /proc/net/arp > /tmp/arpcache.out");
	if ((arpCache = fopen("/tmp/arpcache.out", "r")) == NULL )
    {
           cmsLog_error("could not open /tmp/arpcache.out");
           return FALSE;
    }
	else
	{
		int lineCount = 0;
		while (NULL != fgets(line, sizeof(line), arpCache))
		{
			memset(mac, 0, sizeof(mac));
			lineCount++;
			if(lineCount == 1)
				continue;
			
		    int count=0;	
			pch = strtok (line," "); // space
			while (pch != NULL)
			{
				switch(count)
				{
					case 3 :						
						strncpy(mac, pch, sizeof(mac));
						mac[sizeof(mac)-1] = '\0';
						break;
					default:
						/* don't care */
						break;
				}
				count ++;
				pch = strtok (NULL, " ");
			}
			
			cmsLog_debug("mac = %s", mac);
			if(cmsUtl_strcasecmp(mac,inputMAC) == 0)
			{
				cmsLog_debug("MAC %s present in ARP cache ", inputMAC);
				fclose(arpCache);
				return TRUE;
			}
		}
		fclose(arpCache);
		cmsLog_debug("MAC %s Not  present in ARP cache ", inputMAC);
		return FALSE;
	}
}
#endif

#endif // SKY_LSNET_SUPPORT
