#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <syslog.h>
#include "filter.h"
#ifdef SKY
#include "common.h"
#include <unistd.h>


char routerIPAddress[ROUTER_IP_ADDR_SIZE] = {0};

#ifdef SKY_IPV6
char routerIPv6Address[ROUTER_IPV6_ADDR_SIZE] = {0};
#endif


#ifdef SKY
static char *sky_supportedBrowsers[] = {
	"Chrome",
	"firefox",
	"msie",
	"safari",
	"opera",
	NULL,
};

int sky_match_browser(char *agent)
{
	int matched = 0;
	int cnt = 0;

	if (NULL == agent) {
		return matched;
	}

	while (sky_supportedBrowsers[cnt]) {
		if (strcasestr(agent, sky_supportedBrowsers[cnt])) {
			matched = 1;
			break;
		}
		cnt++;
	}

	return matched;
}
#endif



int blocklogstatus = 0;
#endif /* SKY */

#define BUFSIZE 2048

// turn on the urlfilterd debug message.
// #define UFD_DEBUG 1

typedef enum
{
	PKT_ACCEPT,
	PKT_DROP
}pkt_decision_enum;

struct nfq_handle *h;
struct nfq_q_handle *qh;
char listtype[8];

#ifndef SKY
void add_entry(char *website, char *folder)
#else /* SKY */
void add_entry(char *keyword)
#endif /* SKY */
{
	PURL new_entry, current, prev;
	new_entry = (PURL) malloc (sizeof(URL));
#ifndef SKY
	strcpy(new_entry->website, website);
	strcpy(new_entry->folder, folder);
#else /* SKY */
	strcpy(new_entry->keyword, keyword);
#endif /* SKY */	
	new_entry->next = NULL;

	if (purl == NULL)
	{
		purl = new_entry;
	}
	else 
	{
		current = purl;
		while (current) 
		{
			prev = current;
			current = current->next;
		}
		prev->next = new_entry;
	}
}

int get_url_info()
{
	char temp[MAX_WEB_LEN + MAX_FOLDER_LEN], *temp1, *temp2, web[MAX_WEB_LEN], folder[MAX_FOLDER_LEN];
			
	FILE *f = fopen("/var/url_list", "r");
	if (f != NULL){
	   while (fgets(temp,96, f) != '\0')
	   {
		if (temp[0]=='h' && temp[1]=='t' && temp[2]=='t' && 
			temp[3]=='p' && temp[4]==':' && temp[5]=='/' && temp[6]=='/')
		{
			temp1 = temp + 7;	
		}
		else
		{
			temp1 = temp;	
		}

		if ((*temp1=='w') && (*(temp1+1)=='w') && (*(temp1+2)=='w') && (*(temp1+3)=='.'))
		{
			temp1 = temp1 + 4;
		}

		if ((temp2 = strchr(temp1, '\n')))
		{
			*temp2 = '\0';
		}
#ifndef SKY
		       
		sscanf(temp1, "%[^/]", web);		
		temp1 = strchr(temp1, '/');
		if (temp1 == NULL)
		{
			strcpy(folder, "\0");
		}
		else
		{
			strcpy(folder, ++temp1);		
		}
		add_entry(web, folder);
#else /* SKY */		
		add_entry(temp1);
#endif /* SKY */		
		list_count ++;
	   }
	   fclose(f);
	}
#ifdef UFD_DEBUG
	else {
	   printf("/var/url_list isn't presented.\n");
	   return 1;
	}
#endif


	return 0;
}

static int updateOffset(uint8_t isIPv4, const char *data_p)
{
	const struct tcphdr *tcp;
	int payload_offset;

#ifdef UFD_DEBUG
	printf("isIPv4<%d>\n", isIPv4);
#endif

	if (isIPv4)
	{
		const struct iphdr *iph;

		iph = (const struct iphdr *)data_p;
		tcp = (const struct tcphdr *)(data_p + (iph->ihl<<2));
		payload_offset = ((iph->ihl)<<2) + (tcp->doff<<2);

#ifdef UFD_DEBUG
	printf("offset<%d>\n", payload_offset);
#endif
		return payload_offset;
	}
	else
	{
		const struct ip6_hdr *ip6h;
		const struct ip6_ext *ip_ext_p;
		uint8_t nextHdr;
		int count = 8;

		ip6h = (const struct ip6_hdr *)data_p;
		nextHdr = ip6h->ip6_nxt;
		ip_ext_p = (const struct ip6_ext *)(ip6h + 1);
		payload_offset = sizeof(struct ip6_hdr);

		do
		{
			if ( nextHdr == IPPROTO_TCP )
			{
					tcp = (struct tcphdr *)ip_ext_p;
					payload_offset += tcp->doff << 2;

#ifdef UFD_DEBUG
					printf("offset<%d>\n", payload_offset);
#endif
					return payload_offset;
			}

			payload_offset += (ip_ext_p->ip6e_len + 1) << 3;
			nextHdr = ip_ext_p->ip6e_nxt;
			ip_ext_p = (struct ip6_ext *)(data_p + payload_offset);
			count--; /* at most 8 extension headers */
		} while(count);
	}

	return -1;
}

#ifdef SKY


#ifdef SKY_IPV6


#ifdef SKY_SYSTEM_IPC_VIA_TMP_FILE

void getRouterIPv6ULA()
{
	FILE *fh = NULL;
	int addr_len = 0;

	// get the ULA out of the if
	//system("ip -6 addr show dev br0 scope global to fd00::/8 | sed -n -e 's/^\s*inet6\s\([^\/]*\).*$/\1/pg' 2> /dev/null > /var/tinyproxy.ula");
	// different syntax due to limitations of busybox
	system("ip -6 addr show dev br0 scope global to fd00::/8 | "
		"sed -n -e 's/^[[:space:]]*inet6[[:space:]]\\([0-9a-fA-F:]*\\).*$/\\1/pg' 2> /dev/null > /var/urlfilterd.ula");

	// open data file
	if (NULL == (fh = fopen("/var/urlfilterd.ula", "r"))) {
		fprintf(stderr, "Unable to open ULA intermediate file");
		return;
	}

	memset (routerIPv6Address, 0x00, sizeof(char) * ROUTER_IPV6_ADDR_SIZE);
	if (NULL == fgets(routerIPv6Address, ROUTER_IPV6_ADDR_SIZE, fh)) {
		fprintf(stderr, "Ula file empty or not readable");
		goto exit_cleanup;
	}

	if (1 < (addr_len = strlen(routerIPv6Address))) {
		routerIPv6Address[--addr_len] = '\0';
	}

exit_cleanup:
	fclose(fh);
	unlink("/var/urlfilterd.ula");
}

#else

void getRouterIPv6ULA()
{
	FILE *fh = NULL;
	int addr_len = 0;
	unsigned char buf[sizeof(struct in6_addr)];

	/**
	 * check if the address has been already obtained
	 * - prevents threads of performing the same job twice
	 */
	if (inet_pton(AF_INET6, routerIPv6Address , buf)) {
		fprintf(stderr, "[P] We already have a valid IPv6 address available. Aborting !");
		goto exit_cleanup;
	}

	// get the ULA out of the if
	//system("ip -6 addr show dev br0 scope global to fd00::/8 | sed -n -e 's/^\s*inet6\s\([^\/]*\).*$/\1/pg' 2> /dev/null > /var/tinyproxy.ula");
	// different syntax due to limitations of busybox
	if (NULL == (fh = popen("ip -6 addr show dev br0 scope global to fd00::/8 | "
					"sed -n -e 's/^[[:space:]]*inet6[[:space:]]\\([0-9a-fA-F:]*\\).*$/\\1/pg' 2> /dev/null", "r"))) {
		
		fprintf(stderr, "[P] Unable to execute external command");
		goto exit_cleanup;
	}

	memset (routerIPv6Address, 0x00, sizeof(char) * ROUTER_IPV6_ADDR_SIZE);
	if (NULL == fgets(routerIPv6Address, ROUTER_IPV6_ADDR_SIZE, fh)) {
		fprintf(stderr, "Ula not available");
		goto exit_cleanup;
	}

	if (1 < (addr_len = strlen(routerIPv6Address))) {
		routerIPv6Address[--addr_len] = '\0';
	}

exit_cleanup:
	if (fh) {
		pclose(fh);
	}
}


#endif

#endif


void getRouterIPAddress()
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

			if (strcasestr(name, "skyrouter"))
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


void getBlockSiteLoggingStatus()
{
	char temp[5];
			
	FILE *f = fopen("/var/blocksitelogstatus", "r");
	if(f == NULL)
	{
		blocklogstatus = 0;
#ifdef URLFILTER_DEBUG
		fprintf(stdout, "%s():: /var/blocksitelogstatus  doesn't exist blocklogstatus = %d \n", __FUNCTION__, blocklogstatus);
#endif
		return;
	}
	if (fgets(temp, 5, f) != NULL)
	{
		blocklogstatus = atoi(temp);
#ifdef URLFILTER_DEBUG
		fprintf(stdout, "%s():: blocklogstatus = %d \n", blocklogstatus);
#endif
	}
	return;
}



#endif /* SKY */
#ifndef SKY
static int pkt_decision(struct nfq_data * payload)
#else /* SKY */
static int pkt_decision(struct nfq_data * payload, char* source_mac)
#endif /* SKY */
{
	char *data;
	char *match, *folder, *url;
	PURL current;
	int payload_offset, data_len;
	struct iphdr *iph;
	uint8_t isIPv4;
	match = folder = url = NULL;
#ifdef SKY	
	char hostString[255] = {0};
	char urlString[4 * 1024] = {0}; /* 4k buffer to copy complete url string */
	int i;
	char *keywordHost = NULL;
	char *keywordUrl = NULL;
#endif /* SKY */

	data_len = nfq_get_payload(payload, &data);
	if( data_len == -1 )
	{
#ifdef UFD_DEBUG
	printf("data_len == -1!!!!!!!!!!!!!!!, EXIT\n");
#endif
		exit(1);
	}
#ifdef UFD_DEBUG
	printf("data_len=%d ", data_len);
#endif

	iph = (struct iphdr *)data;
	isIPv4 = (iph->version == 4)?1:0;
	payload_offset = updateOffset(isIPv4, data);

	if (payload_offset < 0)
	{
		/* always accept the packet if error happens */
		return PKT_ACCEPT;
	}

	match = (char *)(data + payload_offset);

	if(strstr(match, "GET ") == NULL && strstr(match, "POST ") == NULL && strstr(match, "HEAD ") == NULL)
	{
#ifdef UFD_DEBUG
	printf("****NO HTTP INFORMATION!!!\n");
#endif
		return PKT_ACCEPT;
	}

#ifdef UFD_DEBUG
	printf("####payload = %s\n\n", match);
#endif

#ifdef SKY
	if( access("/var/matchbrowser",F_OK) >= 0 ) /* Match browsers only when there is self heal hijack */
	{
		/* For optimisation we need to look for this pattern only when self heal hijack is enabled */
		/* if the traffic is not from browsers allow them to pass */
		if (!sky_match_browser(match))
		{
			return PKT_ACCEPT;
		}	
		else
		{
			if (isIPv4) {
				inject_http_hijack_reply(source_mac, data);
			}
#ifdef SKY_IPV6
			else {
				inject_http_ipv6_hijack_reply(source_mac, data);
			}
#endif
			return PKT_DROP;
		}
	}
	
	// here check for Hijacking (no connection or Firmware upgrade)
//	printf("Recevied MAC = %s \n", source_mac);
	if(strcasestr(match,"Host:")) /* Get Host Name */
	{
		if(strcasestr(strcasestr(match,"Host:"),"\n"))
		{			
			strncpy(hostString,strcasestr(match,"Host:"), (strcasestr(strcasestr(match,"Host:"),"\n") - strcasestr(match,"Host:"))); /* Only Host string */
		}
		else
		{
			printf("\n=== urlfilterd: Corrupted string don't know what is happening [%s]\n",match);
		}
	}
	if(strcasestr(match,"HTTP/")) /* Get actual url*/
	{
		int n = snprintf(urlString,(strstr(match,"HTTP/") - match),"%s",match);
		urlString[n]='\0';
	}
	for (current = purl; current != NULL; current = current->next)
	{
		if (current->keyword[0] != '\0')
		{
			keywordHost = strcasestr(hostString, current->keyword);

			if( (keywordHost == NULL))/* Only if Host pattern is null, then only check url to avoid performance issues */
				keywordUrl = strcasestr(urlString, current->keyword);

			if( (keywordHost) || (keywordUrl) ) /* Either host or url pattern matching is not null then got to hijack.*/
			{
				if (isIPv4){
					inject_http_hijack_reply(source_mac, data);
				}
#ifdef SKY_IPV6
				else {
					inject_http_ipv6_hijack_reply(source_mac, data);
				}
#endif

				if(blocklogstatus)
				{
					syslog(LOG_CRIT, "Access blocked to url/keyword \"%s\", request from %s",current->keyword, inet_ntoa(iph->saddr));
				}
				//printf("Packet blocked by accept list \n");
				return PKT_DROP;
			}
		}
	}

	// accept the Packet
	return PKT_ACCEPT;
#else /* SKY */	

	for (current = purl; current != NULL; current = current->next)
	{
		if (current->folder[0] != '\0')
		{
			folder = strstr(match, current->folder);
		}

		if ( (url = strstr(match, current->website)) != NULL ) 
		{
			if (strcmp(listtype, "Exclude") == 0) 
			{
				if ( (folder != NULL) || (current->folder[0] == '\0') )
				{
#ifdef UFD_DEBUG
					printf("####This page is blocked by Exclude list!");
#endif
					return PKT_DROP;
				}
				else 
				{
#ifdef UFD_DEBUG
					printf("###Website hits but folder no hit in Exclude list! packets pass\n");
#endif
					return PKT_ACCEPT;
				}
			}
			else 
			{
				if ( (folder != NULL) || (current->folder[0] == '\0') )
				{
#ifdef UFD_DEBUG
					printf("####This page is accepted by Include list!");
#endif
					return PKT_ACCEPT;
				}
				else 
				{
#ifdef UFD_DEBUG
					printf("####Website hits but folder no hit in Include list!, packets drop\n");
#endif
					return PKT_DROP;
				}
			}
		}
	}

	if (url == NULL) 
	{
		if (strcmp(listtype, "Exclude") == 0) 
		{
#ifdef UFD_DEBUG
			printf("~~~~No Url hits!! This page is accepted by Exclude list!\n");
#endif
			return PKT_ACCEPT;
		}
		else 
		{
#ifdef UFD_DEBUG
			printf("~~~~No Url hits!! This page is blocked by Include list!\n");
#endif
			return PKT_DROP;
		}
	}

#ifdef UFD_DEBUG
	printf("~~~None of rules can be applied!! Traffic is allowed!!\n");
#endif
	return PKT_ACCEPT;
#endif /* SKY */

}


/*
 * callback function for handling packets
 */
static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg,
	      struct nfq_data *nfa, void *data)
{
	struct nfqnl_msg_packet_hdr *ph;
	int decision, id=0;
#ifdef SKY	
	char source_mac[MAC_ADDR_LEN];
	struct nfqnl_msg_packet_hw *hwhdr;
	int i;
#endif /* SKY */	

	ph = nfq_get_msg_packet_hdr(nfa);
	if (ph)
	{
		id = ntohl(ph->packet_id);
	}
#ifndef SKY
	/* check if we should block this packet */
	decision = pkt_decision(nfa);
#else /* SKY */
	hwhdr = nfq_get_packet_hw(nfa);
	for(i=0;i < hwhdr->hw_addrlen;i++)
	{
		source_mac[i] = hwhdr->hw_addr[i];
	}

	sprintf(source_mac, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x",
    (unsigned char)hwhdr->hw_addr[0],
        (unsigned char)hwhdr->hw_addr[1],
        (unsigned char)hwhdr->hw_addr[2],
        (unsigned char)hwhdr->hw_addr[3],
        (unsigned char)hwhdr->hw_addr[4],
        (unsigned char)hwhdr->hw_addr[5]);
	
   /* check if we should block this packet */
	decision = pkt_decision(nfa, source_mac);
#endif /* SKY */	
	if( decision == PKT_ACCEPT)
	{
		return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
	}
	else
	{
		return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
	}
}


/*
 * Open a netlink connection and returns file descriptor
 */
int netlink_open_connection(void *data)
{
	struct nfnl_handle *nh;
	int v4_ok = 1, v6_ok = 1;
 
#ifdef UFD_DEBUG
	printf("opening library handle\n");
#endif
	h = nfq_open();
	if (!h) 
	{
		fprintf(stderr, "error during nfq_open()\n");
		exit(1);
	}

#ifdef UFD_DEBUG
	printf("unbinding existing nf_queue handler(if any)\n");
#endif
	if (nfq_unbind_pf(h, AF_INET) < 0) 
	{
		fprintf(stderr, "error during nfq_unbind_pf() for IPv4\n");
		v4_ok = 0;
	}

	if (nfq_unbind_pf(h, AF_INET6) < 0) 
	{
		fprintf(stderr, "error during nfq_unbind_pf() for IPv6\n");
		v6_ok = 0;
	}

	if ( !(v4_ok || v6_ok) )
	{
		fprintf(stderr, "error during nfq_unbind_pf()\n");
		exit(1);
	}

	v4_ok = v6_ok = 1;
#ifdef UFD_DEBUG
	printf("binding nfnetlink_queue as nf_queue handler\n");
#endif
	if (nfq_bind_pf(h, AF_INET) < 0) 
	{
		fprintf(stderr, "error during nfq_bind_pf() for IPv4\n");
		v4_ok = 0;
	}

	if (nfq_bind_pf(h, AF_INET6) < 0) 
	{
		fprintf(stderr, "error during nfq_bind_pf() for IPv6\n");
		v6_ok = 0;
	}

	if ( !(v4_ok || v6_ok) )
	{
		fprintf(stderr, "error during nfq_bind_pf()\n");
		exit(1);
	}

#ifdef UFD_DEBUG
	printf("binding this socket to queue '0'\n");
#endif
	qh = nfq_create_queue(h,  0, &cb, NULL);
	if (!qh) 
	{
		fprintf(stderr, "error during nfq_create_queue()\n");
		exit(1);
	}

#ifdef UFD_DEBUG
	printf("setting copy_packet mode\n");
#endif
	if (nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff) < 0) 
	{
		fprintf(stderr, "can't set packet_copy mode\n");
		exit(1);
	}

	nh = nfq_nfnlh(h);
	return nfnl_fd(nh);
}


int main(int argc, char **argv)
{
	int fd, rv;
	char buf[BUFSIZE]; 

	strcpy(listtype, argv[1]);
	if (get_url_info())
	{
	   printf("error during get_url_info()\n");
	   return 0;
	}
#ifdef SKY	
	getRouterIPAddress();

#ifdef SKY_IPV6
	getRouterIPv6ULA();
#endif

	getBlockSiteLoggingStatus();
#endif /* SKY */	
	memset(buf, 0, sizeof(buf));

	/* open a netlink connection to get packet from kernel */
	fd = netlink_open_connection(NULL);

	while (1)
	{
		rv = recv(fd, buf, sizeof(buf), 0);
		if ( rv >= 0) 
		{
#ifdef UFD_DEBUG
		   printf("pkt received\n");
#endif
		   nfq_handle_packet(h, buf, rv);
		   memset(buf, 0, sizeof(buf));
		}
		else
		{
		   nfq_close(h);
#ifdef UFD_DEBUG
		   printf("nfq close done\n");
#endif
		   fd = netlink_open_connection(NULL);
#ifdef UFD_DEBUG
		   printf("need to rebind to netfilter queue 0\n");
#endif
		}
	}
#ifdef UFD_DEBUG
        printf("unbinding from queue 0\n");
#endif
	nfq_destroy_queue(qh);
	nfq_close(h);

	return 0;
}
