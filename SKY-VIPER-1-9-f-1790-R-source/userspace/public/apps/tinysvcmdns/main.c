 /*
 * main.c - an entry point for tinysvcmdns developed by SKY.
 * tinysvcmdns is a tiny MDNS implementation for publishing services.
 * Copyright (C) 2011 Darell Tan
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "mdns.h"
#include "mdnsd.h"

 
int main(int argc, char *argv[]) 
{
    static struct mdnsd *svr = NULL;
    struct mdns_service *svc = NULL;
    int fd;
    int hostname_ip_set=0;
    uint32_t br_ip=0;
    struct ifreq intf;
    char hostname[256]={0}; 
    const char *txt[] = { "path=/", NULL };
    char cmd[256]={0};
    /*
     * Add route for mdns service
     */
    sprintf(cmd,"route add -net 224.0.0.0 netmask 224.0.0.0 br0");
    system(cmd);

    /*start mdns thread*/
    svr = mdnsd_start();
    if (svr == NULL) 
    {
        printf("tinysvcmdns: mdnsd_start() failed");
        return 1;
    }

    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) 
    {
        printf("tinysvcmdns: couldn't open socket to obtain bridge ip address");
	    mdnsd_stop(svr);
        return 1;
    }

    strcpy(intf.ifr_name, "br0");

    if (ioctl(fd, SIOCGIFADDR, &intf) != -1)
    {
        if(intf.ifr_addr.sa_family == AF_INET)
        {
            /*Get IP address of bridge*/
            br_ip =((struct sockaddr_in *)&intf.ifr_addr)->sin_addr.s_addr;

            /* Get HW Addr */
            memset(&intf.ifr_hwaddr, 0, sizeof(struct sockaddr));
            if (ioctl(fd, SIOCGIFHWADDR, &intf) != -1)
            {
                sprintf(hostname,"%02x%02x%02x%02x%02x%02x.local",(int)((unsigned char*)&intf.ifr_hwaddr.sa_data)[0],
                        (int)((unsigned char*)&intf.ifr_hwaddr.sa_data)[1],
                        (int)((unsigned char*)&intf.ifr_hwaddr.sa_data)[2],
                        (int)((unsigned char*)&intf.ifr_hwaddr.sa_data)[3],
                        (int)((unsigned char*)&intf.ifr_hwaddr.sa_data)[4],
                        (int)((unsigned char*)&intf.ifr_hwaddr.sa_data)[5]);
                mdnsd_set_hostname(svr, hostname, br_ip);
                hostname_ip_set=1;
            }
        }
    }
    close(fd);

    if(hostname_ip_set==0)
    {
        printf("tinysvcmdns: mdnsd_set_hostname was not done\n");
	    mdnsd_stop(svr); 
	    return 1;
    }

    /*Regiter the service*/
    svc = mdnsd_register_svc(svr,
            "tinysvcmdns responder",
            "_http._tcp.local",
            80,
            NULL,
            txt); 
    printf("tinysvcmdns: hostname %s ip %x set for mdns service\n",hostname,br_ip); 

    while(1) usleep(500*1000);
    
    mdns_service_destroy(svc);

	mdnsd_stop(svr);

    return 0; 
} 
