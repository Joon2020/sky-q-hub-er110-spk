/*
 *   $Id: send.c,v 1.48 2011/05/06 07:51:54 reubenhwk Exp $
 *
 *   Authors:
 *    Pedro Roque		<roque@di.fc.ul.pt>
 *    Lars Fenneberg		<lf@elemental.net>
 *
 *   This software is Copyright 1996,1997 by the above mentioned author(s),
 *   All Rights Reserved.
 *
 *   The license which is distributed with this software in the file COPYRIGHT
 *   applies to this software. If your distribution is missing this file, you
 *   may request it from <pekkas@netcore.fi>.
 *
 */

#include "config.h"
#include "includes.h"
#include "radvd.h"

#ifdef SKY
extern int get_IfEntry (const char *pIfName, struct Interface **pIfEntry);
#endif /* SKY */

/*
 * Sends an advertisement for all specified clients of this interface
 * (or via broadcast, if there are no restrictions configured).
 *
 * If a destination address is given, the RA will be sent to the destination
 * address only, but only if it was configured.
 *
 */
int
send_ra_forall(struct Interface *iface, struct in6_addr *dest)
{
	struct Clients *current;

	/* If no list of clients was specified for this interface, we broadcast */
	if (iface->ClientList == NULL)
#ifdef SKY
		return send_ra(iface, dest, 0);
#else
		return send_ra(iface, dest);
#endif /* SKY */

	/* If clients are configured, send the advertisement to all of them via unicast */
	for (current = iface->ClientList; current; current = current->next)
	{
		char address_text[INET6_ADDRSTRLEN];
		memset(address_text, 0, sizeof(address_text));
		if (get_debuglevel() >= 5)
			inet_ntop(AF_INET6, &current->Address, address_text, INET6_ADDRSTRLEN);

                /* If a non-authorized client sent a solicitation, ignore it (logging later) */
		if (dest != NULL && memcmp(dest, &current->Address, sizeof(struct in6_addr)) != 0)
			continue;
		dlog(LOG_DEBUG, 5, "Sending RA to %s", address_text);
#ifdef SKY
		send_ra(iface, &(current->Address), 0);
#else
		send_ra(iface, &(current->Address));
#endif /* SKY */

		/* If we should only send the RA to a specific address, we are done */
		if (dest != NULL)
			return 0;
	}
	if (dest == NULL)
		return 0;

        /* If we refused a client's solicitation, log it if debugging is high enough */
	char address_text[INET6_ADDRSTRLEN];
	memset(address_text, 0, sizeof(address_text));
	if (get_debuglevel() >= 5)
		inet_ntop(AF_INET6, dest, address_text, INET6_ADDRSTRLEN);

	dlog(LOG_DEBUG, 5, "Not answering request from %s, not configured", address_text);
	return 0;
}

#ifdef SKY
int
send_ra_for_if(const char *pIfName)
{
    struct Interface *iface = NULL;

    /* FIX ME: br0 is hardcoded. Currently, it is the only interface used.
     * Netlink provides the interface on which the link status changes,
     * which is eth0 in this case. Hence, hard-coded for now.
     */
    get_IfEntry ("br0", &iface);
    if (NULL != iface)
    {
        send_ra_forall (iface, NULL);
    }
} /* End of send_ra_for_if() */
#endif /* SKY */

static void
send_ra_inc_len(size_t *len, int add)
{
	*len += add;
	if(*len >= MSG_SIZE_SEND)
	{
		flog(LOG_ERR, "Too many prefixes or routes. Exiting.");
		exit(1);
	}
}

static time_t
time_diff_secs(const struct timeval *time_x, const struct timeval *time_y)
{
	time_t secs_diff;

	secs_diff = time_x->tv_sec - time_y->tv_sec;
	if ((time_x->tv_usec - time_y->tv_usec) >= 500000)
		secs_diff++;

	return secs_diff;
	
}

static void
decrement_lifetime(const time_t secs, uint32_t *lifetime)
{

	if (*lifetime > secs) {
		*lifetime -= secs;	
	} else {
		*lifetime = 0;
	}
}

static void cease_adv_pfx_msg(const char *if_name, struct in6_addr *pfx, const int pfx_len)
{
	char pfx_str[INET6_ADDRSTRLEN];

	print_addr(pfx, pfx_str);

	dlog(LOG_DEBUG, 3, "Will cease advertising %s/%u%%%s, preferred lifetime is 0", pfx_str, pfx_len, if_name);

}

int
send_ra(struct Interface *iface, struct in6_addr *dest, uint8_t is_reset_lft)
{
	uint8_t all_hosts_addr[] = {0xff,0x02,0,0,0,0,0,0,0,0,0,0,0,0,0,1};
	struct sockaddr_in6 addr;
	struct in6_pktinfo *pkt_info;
	struct msghdr mhdr;
	struct cmsghdr *cmsg;
	struct iovec iov;
	char __attribute__((aligned(8))) chdr[CMSG_SPACE(sizeof(struct in6_pktinfo))];
	struct nd_router_advert *radvert;
	struct AdvPrefix *prefix;
	struct AdvRoute *route;
	struct AdvRDNSS *rdnss;
	struct AdvDNSSL *dnssl;
	struct timeval time_now;
	time_t secs_since_last_ra;
#ifdef SKY
	struct AdvPrefix *prev_prefix = NULL;
        uint8_t is_prefix_valid = 1;
        uint8_t is_def_router = 0;
        uint8_t is_def_router_lft_set = 0;
        uint32_t router_lifetime = 0;
#endif /* SKY */
	unsigned char buff[MSG_SIZE_SEND];
	size_t len = 0;
	ssize_t err;

	/* First we need to check that the interface hasn't been removed or deactivated */
	if(check_device(iface) < 0) {
		if (iface->IgnoreIfMissing)  /* a bit more quiet warning message.. */
			dlog(LOG_DEBUG, 4, "interface %s does not exist, ignoring the interface", iface->Name);
		else {
			flog(LOG_WARNING, "interface %s does not exist, ignoring the interface", iface->Name);
		}
		iface->HasFailed = 1;
		/* not really a 'success', but we need to schedule new timers.. */
		return 0;
	} else {
		/* check_device was successful, act if it has failed previously */
		if (iface->HasFailed == 1) {
			flog(LOG_WARNING, "interface %s seems to have come back up, trying to reinitialize", iface->Name);
			iface->HasFailed = 0;
			/*
			 * return -1 so timer_handler() doesn't schedule new timers,
			 * reload_config() will kick off new timers anyway.  This avoids
			 * timer list corruption.
			 */
			reload_config();
			return -1;
		}
	}

	/* Make sure that we've joined the all-routers multicast group */
	if (check_allrouters_membership(iface) < 0)
		flog(LOG_WARNING, "problem checking all-routers membership on %s", iface->Name);

	dlog(LOG_DEBUG, 3, "sending RA on %s", iface->Name);

	if (dest == NULL)
	{
		dest = (struct in6_addr *)all_hosts_addr;
#ifdef SKY
                getcurrtime (&iface->last_multicast);
#else
                gettimeofday(&iface->last_multicast, NULL);
#endif /* SKY */
	}

#ifdef SKY
        getcurrtime (&time_now);
#else
        gettimeofday(&time_now, NULL);
#endif /* SKY */
	secs_since_last_ra = time_diff_secs(&time_now, &iface->last_ra_time);
	if (secs_since_last_ra < 0) {
		secs_since_last_ra = 0;
		flog(LOG_WARNING, "gettimeofday() went backwards!");
	}
	iface->last_ra_time = time_now;

	memset((void *)&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(IPPROTO_ICMPV6);
	memcpy(&addr.sin6_addr, dest, sizeof(struct in6_addr));

	memset(buff, 0, sizeof(buff));
	radvert = (struct nd_router_advert *) buff;

	radvert->nd_ra_type  = ND_ROUTER_ADVERT;
	radvert->nd_ra_code  = 0;
	radvert->nd_ra_cksum = 0;

	radvert->nd_ra_curhoplimit	= iface->AdvCurHopLimit;
	radvert->nd_ra_flags_reserved	=
		(iface->AdvManagedFlag)?ND_RA_FLAG_MANAGED:0;
	radvert->nd_ra_flags_reserved	|=
		(iface->AdvOtherConfigFlag)?ND_RA_FLAG_OTHER:0;
	/* Mobile IPv6 ext */
	radvert->nd_ra_flags_reserved   |=
		(iface->AdvHomeAgentFlag)?ND_RA_FLAG_HOME_AGENT:0;

	if (iface->cease_adv) {
		radvert->nd_ra_router_lifetime = 0;
	} else {
		/* if forwarding is disabled, send zero router lifetime */
		radvert->nd_ra_router_lifetime	 =  !check_ip6_forwarding() ? htons(iface->AdvDefaultLifetime) : 0;
	}
	radvert->nd_ra_flags_reserved   |=
		(iface->AdvDefaultPreference << ND_OPT_RI_PRF_SHIFT) & ND_OPT_RI_PRF_MASK;

	radvert->nd_ra_reachable  = htonl(iface->AdvReachableTime);
	radvert->nd_ra_retransmit = htonl(iface->AdvRetransTimer);

	len = sizeof(struct nd_router_advert);

	prefix = iface->AdvPrefixList;

	/*
	 *	add prefix options
	 */

	while(prefix)
	{
#ifdef SKY
                /* As per Sky requirement, the RA needs to be sent until valid
                 * lifetime expires (becomes zero) and not just the preferred
                 * lifetime
                 */
		if( prefix->enabled && prefix->curr_validlft > 0 )
#else
		if( prefix->enabled && prefix->curr_preferredlft > 0 )
#endif /* SKY */
		{
			struct nd_opt_prefix_info *pinfo;

			pinfo = (struct nd_opt_prefix_info *) (buff + len);

			pinfo->nd_opt_pi_type	     = ND_OPT_PREFIX_INFORMATION;
			pinfo->nd_opt_pi_len	     = 4;
#ifdef SKY
                        /* Prefix length sent out in Router Advertisements must always be
                         * 64. For values other than that, say 56, the LAN side clients
                         * do not consider the prefix for stateless address
                         * auto-configuration
                         */
			pinfo->nd_opt_pi_prefix_len  = RADVD_PREFIX_LEN_STD;
#else
			pinfo->nd_opt_pi_prefix_len  = prefix->PrefixLen;
#endif /* SKY */

			pinfo->nd_opt_pi_flags_reserved  =
				(prefix->AdvOnLinkFlag)?ND_OPT_PI_FLAG_ONLINK:0;
			pinfo->nd_opt_pi_flags_reserved	|=
				(prefix->AdvAutonomousFlag)?ND_OPT_PI_FLAG_AUTO:0;
			/* Mobile IPv6 ext */
			pinfo->nd_opt_pi_flags_reserved |=
				(prefix->AdvRouterAddr)?ND_OPT_PI_FLAG_RADDR:0;

			if (iface->cease_adv && prefix->DeprecatePrefixFlag) {
				/* RFC4862, 5.5.3, step e) */

#ifdef SKY
                                if (prefix->DecrementLifetimesFlag)
                                {
                                    decrement_lifetime(secs_since_last_ra,
                                                       &prefix->curr_validlft);
                                }

                                /* For entries added through config file, "DeprecatePrefixFlag"
                                 * is set. In the case where there is no ULA, there is a possibility
                                 * for WAN prefix to be added through config file. In that case,
                                 * during prefix deprecation on shutdown, the lifetime must be
                                 * the minimum of current valid lifetime of prefix and MIN_AdvValidLifetime
                                 * so that the prefix is valid for the lesser time of the two.
                                 */
				pinfo->nd_opt_pi_valid_time	=
                                ((prefix->curr_validlft > MIN_AdvValidLifetime) ? htonl(MIN_AdvValidLifetime)
                                : htonl (prefix->curr_validlft));
#else
				pinfo->nd_opt_pi_valid_time	= htonl(MIN_AdvValidLifetime);
#endif /* SKY */

				pinfo->nd_opt_pi_preferred_time = 0;

			} else {
				if (prefix->DecrementLifetimesFlag) {
#ifdef SKY
                                        dlog (LOG_DEBUG, "since last ra: %u, curr valid lft: %u",
                                              secs_since_last_ra,prefix->curr_validlft);

                                        /* When the IPv6 address lease is extended, the lifetimes would
                                         * be reset. At that time, it musn't decrement considering the
                                         * last time the RA was sent but must begin the decrement afresh.
                                         */
                                        if (!(prefix->isResetLft))
                                        {
#endif /* SKY */
					    decrement_lifetime(secs_since_last_ra,
								&prefix->curr_validlft);
#ifdef SKY
                                        }

                                        /* When the prefix is deprecated (preferred lifetime is zero),
                                         * the valid lifetime must be set to the minimum of remaining
                                         * valid lifetime and a value slightly greater than 2 hrs
                                         * (MIN_AdvValidLifetime)
                                         */
                                        if ((0 == prefix->curr_preferredlft) &&
                                            (prefix->curr_validlft > MIN_AdvValidLifetime))  
                                        {
                                            prefix->curr_validlft = MIN_AdvValidLifetime;
                                        }
                                        else if ((!prefix->isResetLft) && (prefix->curr_preferredlft > 0))
                                        {
                                            /* Do not decrement lifetime when the lifetimes are reset
                                             * (for instance, on lease renewal)
                                             */
#endif /* SKY */
					    decrement_lifetime(secs_since_last_ra,
							       &prefix->curr_preferredlft);
#ifdef SKY
                                        }

                                        /* Reset the flag so that lifetime shall be decremented,
                                         * unless explicitly stated not to (for instance, during
                                         * prefix lifetime update from signal handler).
                                         */
                                        if (prefix->isResetLft)
                                        {
                                            prefix->isResetLft = 0;
                                        }

                                        /* The prefix with a valid (and not preferred) lifetime of zero
                                         * must no longer be part of the RA sent
                                         */
					if (prefix->curr_validlft == 0)
#else
					if (prefix->curr_preferredlft == 0)
#endif /* SKY */
                                        {
						cease_adv_pfx_msg(iface->Name, &prefix->Prefix, prefix->PrefixLen);
#ifdef SKY
                                                /* Remove the prefix with expired lifetime from the prefix list
                                                 * maintained within the application. Update the prefix pointer,
                                                 * with address of previous prefix, so that iteration to next
                                                 * prefix can continue seamlessly.
                                                 */
                                                prev_prefix->next = prefix->next;
                                                free (prefix);
                                                prefix = prev_prefix;

                                                /* Update flag as this prefix is no longer valid */
                                                is_prefix_valid = 0;
#endif /* SKY */
                                        } /* End of if() - prefix lifetime validation */
#ifdef SKY
                                        /* As the radvd application is not restarted for every
                                         * prefix addition/deletion, the validation of whether the router
                                         * is a default router is performed here. If there is atleast,
                                         * one WAN prefix with a valid lifetime, the router is considered as
                                         * the default router. The ULA prefix is skipped since the
                                         * "DecrementLifetimesFlag" is not set for it and this validation is
                                         * performed only for the prefixes with the flag set.
                                         */
                                        else
                                        {
                                            is_def_router = 1;

                                        } /* End of else() - prefix has a valid lifetime */
#endif /* SKY */
				} /* End of if() - decrement lifetime flag set */

#ifdef SKY
                                /* Condition required as memory for the prefix node, with expired
                                 * valid lifetime, is released.
                                 */
                                if (is_prefix_valid)
                                {
				    pinfo->nd_opt_pi_valid_time	= htonl(prefix->curr_validlft);

                                    /* During graceful shutdown of the system, the prefix is deprecated when
                                     * "DeprecatePrefix" flag is set. As the WAN prefix is released during
                                     * shutdown, it is preferrable to deprecate the WAN prefix in this case.
                                     * Even in case of ULA prefix, if ULA is randomly generated, then it can
                                     * have a different ULA prefix after reboot and hence is preferrable to
                                     * deprecate it as well. For ULA prefix, the "DeprecatePrefixFlag" is turned
                                     * on by default (default value of the flag is 1 [on]) whereas for prefixes
                                     * added on sigusr2, the flag is turned off. Hence, in order to deprecate the
                                     * prefix in this case, the cease_adv flag is used which is set when the
                                     * radvd application gracefully terminates.
                                     */
                                    if (iface->cease_adv)
                                    {
				        pinfo->nd_opt_pi_preferred_time = 0;
                                    }
                                    else
                                    {
				        pinfo->nd_opt_pi_preferred_time = htonl(prefix->curr_preferredlft);
                                    }
                                }
#else
				pinfo->nd_opt_pi_valid_time	= htonl(prefix->curr_validlft);
				pinfo->nd_opt_pi_preferred_time = htonl(prefix->curr_preferredlft);
#endif /* SKY */
			}
			pinfo->nd_opt_pi_reserved2	= 0;

#ifdef SKY
                        /* Ensure prefix with expired valid lifetime is not sent in the RA */
                        if (is_prefix_valid)
#endif /* SKY */
                        {
			    memcpy(&pinfo->nd_opt_pi_prefix, &prefix->Prefix,
			           sizeof(struct in6_addr));

			    send_ra_inc_len(&len, sizeof(*pinfo));
                        }
		}
#ifdef SKY
                /* Necessary to remove the expired prefix from the list */
                prev_prefix = prefix;

                /* Reset the prefix related flags */
                is_prefix_valid = 1;
#endif /* SKY */ 
		prefix = prefix->next;
	}
#ifdef SKY
        /* Reset the default router related parameters, when there is no
         * valid WAN prefix
         */
        if (!is_def_router)
        {
            iface->AdvDefaultLifetime = 0;
            radvert->nd_ra_router_lifetime = 0;
        }
#endif /* SKY */ 

	route = iface->AdvRouteList;

	/*
	 *	add route options
	 */

	while(route)
	{
		struct nd_opt_route_info_local *rinfo;

		rinfo = (struct nd_opt_route_info_local *) (buff + len);

		rinfo->nd_opt_ri_type	     = ND_OPT_ROUTE_INFORMATION;
		/* XXX: the prefixes are allowed to be sent in smaller chunks as well */
		rinfo->nd_opt_ri_len	     = 3;
#ifdef SKY
                /* Prefix length sent out in Router Advertisements must always be
                 * 64. For values other than that, say 56, the LAN side clients
                 * do not consider the prefix for stateless address
                 * auto-configuration
                 */
		rinfo->nd_opt_ri_prefix_len  = RADVD_PREFIX_LEN_STD;
#else
		rinfo->nd_opt_ri_prefix_len  = route->PrefixLen;
#endif;

		rinfo->nd_opt_ri_flags_reserved  =
			(route->AdvRoutePreference << ND_OPT_RI_PRF_SHIFT) & ND_OPT_RI_PRF_MASK;
		if (iface->cease_adv && route->RemoveRouteFlag) {
			rinfo->nd_opt_ri_lifetime	= 0;
		} else {
			rinfo->nd_opt_ri_lifetime	= htonl(route->AdvRouteLifetime);
		}

		memcpy(&rinfo->nd_opt_ri_prefix, &route->Prefix,
		       sizeof(struct in6_addr));
		send_ra_inc_len(&len, sizeof(*rinfo));

		route = route->next;
	}

	rdnss = iface->AdvRDNSSList;

	/*
	 *	add rdnss options
	 */

	while(rdnss)
	{
		struct nd_opt_rdnss_info_local *rdnssinfo;

		rdnssinfo = (struct nd_opt_rdnss_info_local *) (buff + len);

		rdnssinfo->nd_opt_rdnssi_type	     = ND_OPT_RDNSS_INFORMATION;
		rdnssinfo->nd_opt_rdnssi_len	     = 1 + 2*rdnss->AdvRDNSSNumber;
		rdnssinfo->nd_opt_rdnssi_pref_flag_reserved = 0;

		if (iface->cease_adv && rdnss->FlushRDNSSFlag) {
			rdnssinfo->nd_opt_rdnssi_lifetime	= 0;
		} else {
			rdnssinfo->nd_opt_rdnssi_lifetime	= htonl(rdnss->AdvRDNSSLifetime);
		}

		memcpy(&rdnssinfo->nd_opt_rdnssi_addr1, &rdnss->AdvRDNSSAddr1,
		       sizeof(struct in6_addr));
		memcpy(&rdnssinfo->nd_opt_rdnssi_addr2, &rdnss->AdvRDNSSAddr2,
		       sizeof(struct in6_addr));
		memcpy(&rdnssinfo->nd_opt_rdnssi_addr3, &rdnss->AdvRDNSSAddr3,
		       sizeof(struct in6_addr));
		send_ra_inc_len(&len, sizeof(*rdnssinfo) - (3-rdnss->AdvRDNSSNumber)*sizeof(struct in6_addr));

		rdnss = rdnss->next;
	}

	dnssl = iface->AdvDNSSLList;

	/*
	 *	add dnssl options
	 */

	while(dnssl)
	{
		struct nd_opt_dnssl_info_local *dnsslinfo;
		int i;
		char *buff_ptr;

		dnsslinfo = (struct nd_opt_dnssl_info_local *) (buff + len);

		dnsslinfo->nd_opt_dnssli_type		= ND_OPT_DNSSL_INFORMATION;
		dnsslinfo->nd_opt_dnssli_len 		= 1; /* more further down */
		dnsslinfo->nd_opt_dnssli_reserved	= 0;

		if (iface->cease_adv && dnssl->FlushDNSSLFlag) {
			dnsslinfo->nd_opt_dnssli_lifetime	= 0;
		} else {
			dnsslinfo->nd_opt_dnssli_lifetime	= htonl(dnssl->AdvDNSSLLifetime);
		}

		buff_ptr = dnsslinfo->nd_opt_dnssli_suffixes;
		for (i = 0; i < dnssl->AdvDNSSLNumber; i++) {
			char *label;
			int label_len;

			label = dnssl->AdvDNSSLSuffixes[i];

			while (label[0] != '\0') {
				if (strchr(label, '.') == NULL)
					label_len = strlen(label);
				else
					label_len = strchr(label, '.') - label;

				*buff_ptr++ = label_len;

				memcpy(buff_ptr, label, label_len);
				buff_ptr += label_len;

				label += label_len;

				if (label[0] == '.')
					label++;
				else
					*buff_ptr++ = 0;
			}
		}

		dnsslinfo->nd_opt_dnssli_len		+= ((buff_ptr-dnsslinfo->nd_opt_dnssli_suffixes)+7)/8;

		send_ra_inc_len(&len, dnsslinfo->nd_opt_dnssli_len * 8);

		dnssl = dnssl->next;
	}

	/*
	 *	add MTU option
	 */

	if (iface->AdvLinkMTU != 0) {
		struct nd_opt_mtu *mtu;

		mtu = (struct nd_opt_mtu *) (buff + len);

		mtu->nd_opt_mtu_type     = ND_OPT_MTU;
		mtu->nd_opt_mtu_len      = 1;
		mtu->nd_opt_mtu_reserved = 0;
		mtu->nd_opt_mtu_mtu      = htonl(iface->AdvLinkMTU);

		send_ra_inc_len(&len, sizeof(*mtu));
	}

	/*
	 * add Source Link-layer Address option
	 */

	if (iface->AdvSourceLLAddress && iface->if_hwaddr_len > 0)
	{
		uint8_t *ucp;
		unsigned int i;

		ucp = (uint8_t *) (buff + len);

		*ucp++  = ND_OPT_SOURCE_LINKADDR;
		*ucp++  = (uint8_t) ((iface->if_hwaddr_len + 16 + 63) >> 6);

		send_ra_inc_len(&len, 2 * sizeof(uint8_t));

		i = (iface->if_hwaddr_len + 7) >> 3;
		memcpy(buff + len, iface->if_hwaddr, i);
		send_ra_inc_len(&len, i);
	}

	/*
	 * Mobile IPv6 ext: Advertisement Interval Option to support
	 * movement detection of mobile nodes
	 */

	if(iface->AdvIntervalOpt)
	{
		struct AdvInterval a_ival;
                uint32_t ival;
                if(iface->MaxRtrAdvInterval < Cautious_MaxRtrAdvInterval){
                       ival  = ((iface->MaxRtrAdvInterval +
                                 Cautious_MaxRtrAdvInterval_Leeway ) * 1000);

                }
                else {
                       ival  = (iface->MaxRtrAdvInterval * 1000);
                }
 		a_ival.type	= ND_OPT_RTR_ADV_INTERVAL;
		a_ival.length	= 1;
		a_ival.reserved	= 0;
		a_ival.adv_ival	= htonl(ival);

		memcpy(buff + len, &a_ival, sizeof(a_ival));
		send_ra_inc_len(&len, sizeof(a_ival));
	}

	/*
	 * Mobile IPv6 ext: Home Agent Information Option to support
	 * Dynamic Home Agent Address Discovery
	 */

	if(iface->AdvHomeAgentInfo &&
	   (iface->AdvMobRtrSupportFlag || iface->HomeAgentPreference != 0 ||
	    iface->HomeAgentLifetime != iface->AdvDefaultLifetime))

	{
		struct HomeAgentInfo ha_info;
 		ha_info.type		= ND_OPT_HOME_AGENT_INFO;
		ha_info.length		= 1;
		ha_info.flags_reserved	=
			(iface->AdvMobRtrSupportFlag)?ND_OPT_HAI_FLAG_SUPPORT_MR:0;
		ha_info.preference	= htons(iface->HomeAgentPreference);
		ha_info.lifetime	= htons(iface->HomeAgentLifetime);

		memcpy(buff + len, &ha_info, sizeof(ha_info));
		send_ra_inc_len(&len, sizeof(ha_info));
	}

	iov.iov_len  = len;
	iov.iov_base = (caddr_t) buff;

	memset(chdr, 0, sizeof(chdr));
	cmsg = (struct cmsghdr *) chdr;

	cmsg->cmsg_len   = CMSG_LEN(sizeof(struct in6_pktinfo));
	cmsg->cmsg_level = IPPROTO_IPV6;
	cmsg->cmsg_type  = IPV6_PKTINFO;

	pkt_info = (struct in6_pktinfo *)CMSG_DATA(cmsg);
	pkt_info->ipi6_ifindex = iface->if_index;
	memcpy(&pkt_info->ipi6_addr, &iface->if_addr, sizeof(struct in6_addr));

#ifdef HAVE_SIN6_SCOPE_ID
	if (IN6_IS_ADDR_LINKLOCAL(&addr.sin6_addr) ||
		IN6_IS_ADDR_MC_LINKLOCAL(&addr.sin6_addr))
			addr.sin6_scope_id = iface->if_index;
#endif

	memset(&mhdr, 0, sizeof(mhdr));
	mhdr.msg_name = (caddr_t)&addr;
	mhdr.msg_namelen = sizeof(struct sockaddr_in6);
	mhdr.msg_iov = &iov;
	mhdr.msg_iovlen = 1;
	mhdr.msg_control = (void *) cmsg;
	mhdr.msg_controllen = sizeof(chdr);

	err = sendmsg(sock, &mhdr, 0);

	if (err < 0) {
		if (!iface->IgnoreIfMissing || !(errno == EINVAL || errno == ENODEV))
                {
			flog(LOG_WARNING, "sendmsg: %s", strerror(errno));
                }
		else
                {
			dlog(LOG_DEBUG, 3, "sendmsg: %s", strerror(errno));
                }
	}

	return 0;
}

#if 0
#ifdef SKY
int
get_IfEntry (const char *pIfName, struct Interface **pIfEntry)
{
    struct Interface *iface = NULL;

    if ((NULL == pIfName) || (NULL == pIfEntry))
    {
        flog (LOG_WARNING, "Invalid input arguments");
        return -1;
    }

    *pIfEntry = NULL;
    for (iface = IfaceList; (iface && (NULL == *pIfEntry)); iface = iface->next)
    {
        if (0 == cmsUtl_strncmp (iface->Name, pIfName, cmsUtl_strlen (iface->Name)))
        {
            *pIfEntry = iface;
        }
    }

    return 0;

}
#endif /* SKY */
#endif
