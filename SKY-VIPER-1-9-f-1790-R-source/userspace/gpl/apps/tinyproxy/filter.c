/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1999 George Talusan <gstalusan@uwaterloo.ca>
 * Copyright (C) 2002 James E. Flemer <jflemer@acm.jhu.edu>
 * Copyright (C) 2002 Robert James Kaes <rjkaes@users.sourceforge.net>
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
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* A substring of the domain to be filtered goes into the file
 * pointed at by DEFAULT_FILTER.
 */

#include "main.h"

#include "filter.h"
#include "heap.h"
#include "log.h"
#include "reqs.h"
#include "conf.h"

//#define FILTER_BUFFER_LEN (512)
#define REDIRECT_ALL "redirect_all"

static int err;

struct filter_list {
        struct filter_list *next;
        char *pat;
        regex_t *cpat;
};

static struct filter_list *fl = NULL;
static int already_init = 0;
static filter_policy_t default_policy = FILTER_DEFAULT_ALLOW;
/* char routerIP[20]; */
static char redirectAllTraffic[256];

char routerIPAddress[ROUTER_IP_ADDR_SIZE];
char routerIPv6Address[ROUTER_IPV6_ADDR_SIZE];

char redirectURL[256];
char urlFilter[10]="";
char hijackBitmap = 0;

//#define MAX_HIJACK_PAGES 8
char skyHijackPages[MAX_HIJACK_PAGES][FILTER_BUFFER_LEN]= {"NULL"};
char skyHijackExceptions[MAX_HIJACK_PAGES][FILTER_BUFFER_LEN]= {"NULL"};

FILE *fsRedirect = NULL;

void getRouterIPAddress();
void getRouterIPv6ULA();


/*
 * Initializes a linked list of strings containing hosts/urls to be filtered
 */
void filter_init (void)
{
        FILE *fd;
        struct filter_list *p;
        char buf[FILTER_BUFFER_LEN];
        char *s;
        int cflags;
        FILE *fs;

/*
  	// fprintf(stderr,"tinyproxy:: filter_init:  pid = %d \n", getpid());
        if(fs = fopen("/var/allowedHosts", "r"))
        {
         	    fgets(routerIP,20,fs);
		 //  fprintf(stderr,"Allowed Host = %s \n",routerIP);
          	   fclose(fs);
        }
        */

        if(fs = fopen("/var/redirectAll", "r"))
        {
		fread(&hijackBitmap,1,1,fs);
		fclose(fs);
		
	}

/*
	if(fs = fopen("/var/urlfilter", "r"))
	{
		fgets(urlFilter,256,fs);
		fclose(fs);
	}
*/
	getRouterIPAddress();
	getRouterIPv6ULA();
	//fprintf(stderr,"Router Ip Address = %s \n",routerIPAddress);
		
	if(fs = fopen("/var/skyhijackpages", "r"))
	{
		int i=0;
      		while (fgets (buf, FILTER_BUFFER_LEN, fs) && (i < MAX_HIJACK_PAGES)) {
			//fprintf(stderr,"populating hijack page array, page = %s \n",buf);
			strcpy(skyHijackPages[i], buf);
			i++;
		}
		fclose(fs);
	}
	/*

	if(fs = fopen("/var/skyhijackexceptions", "r"))
	{
		int i=0;
      		while (fgets (buf, FILTER_BUFFER_LEN, fs) && (i < MAX_HIJACK_PAGES)) {
			//fprintf(stderr,"populating hijack page array, page = %s \n",buf);
			strcpy(skyHijackExceptions[i], buf);
			i++;
		}
		fclose(fs);
	}
	
	*/

        if (fl || already_init) {
                return;
        }


        fd = fopen (config.filter, "a+");
        if (!fd) {
            fprintf (stderr,"could not open filter %s\n",config.filter);
                return;
        }

        p = NULL;

        cflags = REG_NEWLINE | REG_NOSUB;
        if (config.filter_extended)
                cflags |= REG_EXTENDED;
        if (!config.filter_casesensitive)
                cflags |= REG_ICASE;

        while (fgets (buf, FILTER_BUFFER_LEN, fd)) {
                /*
                 * Remove any trailing white space and
                 * comments.
                 */
                s = buf;
                while (*s) {
                        if (isspace ((unsigned char) *s))
                                break;
                        if (*s == '#') {
                                /*
                                 * If the '#' char is preceeded by
                                 * an escape, it's not a comment
                                 * string.
                                 */
                                if (s == buf || *(s - 1) != '\\')
                                        break;
                        }
                        ++s;
                }
                *s = '\0';

                /* skip leading whitespace */
                s = buf;
                while (*s && isspace ((unsigned char) *s))
                        s++;

                /* skip blank lines and comments */
                if (*s == '\0')
                        continue;

                if (!p) /* head of list */
                        fl = p =
                            (struct filter_list *)
                            safecalloc (1, sizeof (struct filter_list));
                else {  /* next entry */
                        p->next =
                            (struct filter_list *)
                            safecalloc (1, sizeof (struct filter_list));
                        p = p->next;
                }

                if ((strncasecmp (s, "http://", 7) == 0 )|| (strncasecmp (s, "ftp://", 6) == 0))
                {
                    char *skipped_type = strstr (s, "//") + 2;
                    s = skipped_type;
                }
                p->pat = safestrdup (s);
                p->cpat = (regex_t *) safemalloc (sizeof (regex_t));
                err = regcomp (p->cpat, p->pat, cflags);
                if (err != 0) {
                        fprintf (stderr,
                                 "Bad regex in %s: %s\n",
                                 config.filter, p->pat);
                        exit (EX_DATAERR);
                }
        }
        if (ferror (fd)) {
                perror ("fgets");
                exit (EX_DATAERR);
        }
        fclose (fd);

        already_init = 1;
}

/* unlink the list */
void filter_destroy (void)
{
        struct filter_list *p, *q;

        if (already_init) {
                for (p = q = fl; p; p = q) {
                        regfree (p->cpat);
                        safefree (p->cpat);
                        safefree (p->pat);
                        q = p->next;
                        safefree (p);
                }
                fl = NULL;
                already_init = 0;
                /* memset(routerIP, 0, sizeof(routerIP)); */
				memset(redirectAllTraffic, 0, sizeof(redirectAllTraffic));
				memset(routerIPAddress, 0, sizeof(routerIPAddress));
				memset(redirectURL, 0, sizeof(redirectURL));
        }
}

/**
 * reload the filter file if filtering is enabled
 */
void filter_reload (void)
{
        if (config.filter) {
                log_message (LOG_NOTICE, "Re-reading filter file.");
                filter_destroy ();
                filter_init ();
        }
}

/* Return 0 to allow, non-zero to block */
int filter_domain (const char *host)
{
    fprintf (stderr,"IN filter domain\n");
        struct filter_list *p;
        int result;

        if (!fl || !already_init)
                goto COMMON_EXIT;

        for (p = fl; p; p = p->next) {
                result =
                    regexec (p->cpat, host, (size_t) 0, (regmatch_t *) 0, 0);

                if (result == 0) {
                        if (default_policy == FILTER_DEFAULT_ALLOW)
                                return 1;
                        else
                                return 0;
                }
        }

COMMON_EXIT:
        if (default_policy == FILTER_DEFAULT_ALLOW)
                return 0;
        else
                return 1;
}

/* returns 0 to allow, non-zero to block */
int filter_url (const char *url)
{
        struct filter_list *p;
        int result;

        if (!fl || !already_init)
                goto COMMON_EXIT;

        for (p = fl; p; p = p->next)
        {

            result =  regexec (p->cpat, url, (size_t) 0, (regmatch_t *) 0, 0);


            if (result == 0) {
                if (default_policy == FILTER_DEFAULT_ALLOW)
                    return 1;
                else
                    return 0;
            }

        }

COMMON_EXIT:
        if (default_policy == FILTER_DEFAULT_ALLOW)
                return 0;
        else
                return 1;
}

int redirectALL()
{
    if(strncmp(redirectAllTraffic, REDIRECT_ALL, strlen(REDIRECT_ALL))!=0)
    {
        return 0;
    }
    else
    {
        return 1;
    }
}

#if 0
int filter_url_host(const char *url, const char *host)
{
    struct filter_list *p;
    int result;
   // fprintf (stderr,"COMP: host %s %d, routerIP %s %d \n",host, strlen(host), routerIP, strlen(routerIP));

    if (!fl || !already_init)
        goto COMMON_EXIT;
    // fprintf (stderr,"Doing strncmp\n");
    if(strncmp(host, routerIP, strlen(host))!= 0)
    {
        for (p = fl; p; p = p->next)
        {
            //fprintf (stderr,"URL is %s\n, cpat is %s\n, pat is %s\n",url,p->cpat,p->pat);
            if (config.filter_casesensitive)
            {
				result = strstr(url,p->pat);
			}
			else
			{
				result = strcasestr(url,p->pat);
			}
            if(result != NULL)
            {
                if (default_policy == FILTER_DEFAULT_ALLOW)
                    return 1;
                else
                    return 0;
            }
            
            if (config.filter_casesensitive)
            {
				result = strstr(host, p->pat);
			}
			else
			{
				result = strcasestr(host, p->pat);
			}
				
            if(result != NULL)
            {
                if (default_policy == FILTER_DEFAULT_ALLOW)
                    return 1;
                else
                    return 0;
            }
        }
    }
    else
    {
        return 0;
    }

    COMMON_EXIT:
    if (default_policy == FILTER_DEFAULT_ALLOW)
        return 0;
    else
        return 1;
}
#endif

/*
 * Set the default filtering policy
 */
void filter_set_default_policy (filter_policy_t policy)
{
        default_policy = policy;
}


#ifdef SKY_SYSTEM_IPC_VIA_TMP_FILE
/**
 * retrieve ipv6 ula
 */
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
		log_message(LOG_NOTICE, "We already have a valid IPv6 address available. Aborting !");
		return;
	}

	// get the ULA out of the if
	//system("ip -6 addr show dev br0 scope global to fd00::/8 | sed -n -e 's/^\s*inet6\s\([^\/]*\).*$/\1/pg' 2> /dev/null > /var/tinyproxy.ula");
	// different syntax due to limitations of busybox
	system("ip -6 addr show dev br0 scope global to fd00::/8 | "
		"sed -n -e 's/^[[:space:]]*inet6[[:space:]]\\([0-9a-fA-F:]*\\).*$/\\1/pg' 2> /dev/null > /var/tinyproxy.ula");

	// open data file
	if (NULL == (fh = fopen("/var/tinyproxy.ula", "r"))) {
		log_message(LOG_ERR, "Unable to open ULA intermediate file");
		return;
	}

	memset (routerIPv6Address, 0x00, sizeof(char) * ROUTER_IPV6_ADDR_SIZE);
	if (NULL == fgets(routerIPv6Address, ROUTER_IPV6_ADDR_SIZE, fh)) {
		log_message(LOG_ERR, "Ula file empty or not readable");
		goto exit_cleanup;
	}

	if (1 < (addr_len = strlen(routerIPv6Address))) {
		routerIPv6Address[--addr_len] = '\0';
	}

exit_cleanup:
	fclose(fh);
	unlink("/var/tinyproxy.ula");
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
		log_message(LOG_NOTICE, "We already have a valid IPv6 address available. Aborting !");
		goto exit_cleanup;
	}

	// get the ULA out of the if
	//system("ip -6 addr show dev br0 scope global to fd00::/8 | sed -n -e 's/^\s*inet6\s\([^\/]*\).*$/\1/pg' 2> /dev/null > /var/tinyproxy.ula");
	// different syntax due to limitations of busybox
	if ( NULL == (fh = popen("ip -6 addr show dev br0 scope global to fd00::/8 | "
			"sed -n -e 's/^[[:space:]]*inet6[[:space:]]\\([0-9a-fA-F:]*\\).*$/\\1/pg' 2> /dev/null", "r"))) {
		log_message(LOG_ERR, "Ula not available");
		goto exit_cleanup;
	}

	memset (routerIPv6Address, 0x00, sizeof(char) * ROUTER_IPV6_ADDR_SIZE);
	if (NULL == fgets(routerIPv6Address, ROUTER_IPV6_ADDR_SIZE, fh)) {
		log_message(LOG_ERR, "Ula not available");
		goto exit_cleanup;
	}

	if (1 < (addr_len = strlen(routerIPv6Address))) {
		routerIPv6Address[--addr_len] = '\0';
	}

	log_message(LOG_NOTICE, "[P] %s", routerIPv6Address);

exit_cleanup:
	if (fh) {
		pclose(fh);
	}
}

#endif



void getRouterIPAddress()
{
	FILE *fp=NULL;
  	char line[256];
	char *ip, *name;
	int found = 0;
	
	if (!(fp = fopen("/var/hosts", "r"))) {
		log_message(LOG_ERR, "Cannot open file %s", "/var/hosts");
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