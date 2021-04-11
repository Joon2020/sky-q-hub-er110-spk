/* tinyproxy - A fast light-weight HTTP proxy
 * Copyright (C) 1999 George Talusan <gstalusan@uwaterloo.ca>
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

/* See 'filter.c' for detailed information. */

#ifndef _TINYPROXY_FILTER_H_
#define _TINYPROXY_FILTER_H_

#define ROUTER_IP_ADDR_SIZE	16
#define ROUTER_IPV6_ADDR_SIZE	48 

typedef enum {
        FILTER_DEFAULT_ALLOW,
        FILTER_DEFAULT_DENY
} filter_policy_t;

extern void filter_init (void);
extern void filter_destroy (void);
extern void filter_reload (void);
extern int filter_domain (const char *host);
extern int filter_url (const char *url);
// extern int filter_url_host(const char *url, const char *host);
extern int redirectALL();

extern void filter_set_default_policy (filter_policy_t policy);

#define FILTER_BUFFER_LEN 512
#define MAX_HIJACK_PAGES 8

#endif
