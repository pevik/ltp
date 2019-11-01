// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>
 */

#include "config.h"
#include "tst_test.h"

#ifdef HAVE_LIBMNL

#include <string.h>

#include <libmnl/libmnl.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>

#include "tst_net.h"
#include "tst_safe_net.h"
#include "tst_safe_stdio.h"

#define NS_TIMES_MAX 65535

#define IP_ADDR_DELIM ','
#define STR(x) #x
#define CHR2STR(x) STR(x)

static char *c_opt, *d_opt, *g_opt, *ipv6_opt, *p_opt, *r_opt;

static int family = AF_INET;
static int fd, num_loops, port;

static unsigned int is_ipv6, max, prefix;

static struct mnl_socket *nl;
static struct addrinfo *rhost_addrinfo, hints;

struct in_addr ip;
struct in6_addr ip6;

union {
	in_addr_t ip;
	struct in6_addr ip6;
} dst;

struct iface {
	unsigned int index;
	struct iface *next;
	char iface[IFNAMSIZ];
};

struct ip_addr {
	char ip[INET6_ADDRSTRLEN];
	struct ip_addr *next;
};

static struct ip_addr *gw, *rhost;
static struct iface *iface;
static unsigned int gw_len, iface_len, rhost_len;

void save_iface(void **data, const char *item)
{
	struct iface *n = SAFE_MALLOC(sizeof(*n));
	struct iface **list = (struct iface**)data;

	strncpy(n->iface, item, sizeof(n->iface));
	n->iface[sizeof(n->iface)-1] = '\0';

	n->index = if_nametoindex(item);
	if (!n->index)
		tst_brk(TBROK, "if_nametoindex failed, '%s' not found", item);
	n->next = *list;
	*list = n;
}

void save_ip(void **data, const char *item)
{
	struct ip_addr *n = SAFE_MALLOC(sizeof(*n));
	struct ip_addr **list = (struct ip_addr**)data;

	strncpy(n->ip, item, sizeof(n->ip));
	n->ip[sizeof(n->ip)-1] = '\0';

	n->next = *list;
	*list = n;
}

int save_item(void **list, char *item, void (*callback)(void **, const char *))
{
	int len = 0;

	while ((item = strtok(item, CHR2STR(IP_ADDR_DELIM))) != NULL) {
		callback(list, item);
		item = NULL;
		len++;
	}

	return len;
}

static void setup(void)
{
	prefix = 24;
	if (ipv6_opt) {
		family = AF_INET6;
		is_ipv6 = 1;
		prefix = 64;
	}
	if (!c_opt)
		tst_brk(TBROK, "missing number of loops (-c num)");

	if (!d_opt)
		tst_brk(TBROK, "missing iface (-d iface)");

	if (!p_opt)
		tst_brk(TBROK, "missing rhost port (-p port)");

	if (!r_opt)
		tst_brk(TBROK, "missing rhost IP (-r IP)");

	if (tst_parse_int(p_opt, &port, 1, 65535))
		tst_brk(TBROK, "invalid rhost port '%s'", p_opt);

	if (tst_parse_int(c_opt, &num_loops, 1, 65535))
		tst_brk(TBROK, "invalid number of loops '%s'", c_opt);

	iface_len = save_item((void **)&iface, d_opt, save_iface);
	rhost_len = save_item((void **)&rhost, r_opt, save_ip);

	max = MAX(iface_len, rhost_len);
	if (iface_len > 1 && rhost_len > 1 && iface_len != max)
		tst_brk(TBROK, "-d specify more NICs and -r specify more IPs, they need to have the same count");

	if (g_opt) {
		gw_len = save_item((void **)&gw, g_opt, save_ip);
		max = MAX(gw_len, max);

		if (gw_len > 1 && max > 1 && gw_len != max) {
			if (iface_len == max)
				tst_brk(TBROK, "-d specify more NICs and -r specify more IPs, they need to have the same count");
			else
				tst_brk(TBROK, "-g and -r have more IP, they need to have the same count");
		}
	}

	fd = SAFE_SOCKET(family, SOCK_DGRAM, IPPROTO_UDP);

	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_addr = INADDR_ANY;
}

static void cleanup(void)
{
	if (fd > 0)
		close(fd);

	if (nl)
		mnl_socket_close(nl);

	if (rhost_addrinfo)
		freeaddrinfo(rhost_addrinfo);
}

static void rtnl_route(int iface, char *destination, uint32_t prefix, char
		       *gateway, int type)
{
	union {
		in_addr_t ip;
		struct in6_addr ip6;
	} dst;
	union {
		in_addr_t ip;
		struct in6_addr ip6;
	} gw;

	struct mnl_socket *nl;
	char buf[MNL_SOCKET_BUFFER_SIZE];
	struct nlmsghdr *nlh;
	struct rtmsg *rtm;
	uint32_t seq, portid;
	int ret;

	tst_res(TINFO, "!!! dst before conversion: '%s'", destination);
	if (gateway)
		tst_res(TINFO, "!!! gw before conversion: '%s'", gateway);

	if (!inet_pton(family, destination, &dst))
		tst_brk(TBROK, "inet_pton failed ('%s', errno=%d): %s",
			destination, errno, strerror(errno));

	if (gateway && !inet_pton(family, gateway, &gw))
		tst_brk(TBROK, "inet_pton failed ('%s', errno=%d): %s", gateway,
			errno, strerror(errno));

	char dst_str[INET6_ADDRSTRLEN];
	if (is_ipv6) {
		if (!inet_ntop(AF_INET6, &dst, dst_str, sizeof(dst_str))) // could be also struct in6_addr
			tst_brk(TBROK, "inet_ntop failed errno=%d: %s",
						errno, tst_strerrno(errno));
	} else {
		if (!inet_ntop(AF_INET, &dst.ip, dst_str, sizeof(dst_str)))
			tst_brk(TBROK, "inet_ntop failed errno=%d: %s",
						errno, tst_strerrno(errno));
	}
	tst_res(TINFO, "!!! dst: '%s'", dst_str);

	if (gateway) {
		char gw_str[INET6_ADDRSTRLEN];
		if (is_ipv6) {
			if (!inet_ntop(AF_INET6, &gw, gw_str, sizeof(gw_str)))
				tst_brk(TBROK, "inet_ntop failed errno=%d: %s",
							errno, tst_strerrno(errno));
		} else {
			if (!inet_ntop(AF_INET, &gw.ip, gw_str, sizeof(gw_str)))
				tst_brk(TBROK, "inet_ntop failed errno=%d: %s",
							errno, tst_strerrno(errno));
		}
		tst_res(TINFO, "!!! gw: '%s'", gw_str);
	}

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= type;

	nlh->nlmsg_flags = NLM_F_ACK;
	if (type == RTM_NEWROUTE)
		nlh->nlmsg_flags |= NLM_F_REQUEST | NLM_F_CREATE | NLM_F_REPLACE;

	nlh->nlmsg_seq = seq = time(NULL);

	rtm = mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtmsg));
	rtm->rtm_family = family;
	rtm->rtm_dst_len = prefix;
	rtm->rtm_src_len = 0;
	rtm->rtm_tos = 0;
	rtm->rtm_protocol = RTPROT_STATIC;
	rtm->rtm_table = RT_TABLE_MAIN;
	rtm->rtm_type = RTN_UNICAST;
	rtm->rtm_scope = gateway ? RT_SCOPE_UNIVERSE : RT_SCOPE_LINK;
	rtm->rtm_flags = 0;

	if (is_ipv6)
		mnl_attr_put(nlh, RTA_DST, sizeof(struct in6_addr), &dst);
	else
		mnl_attr_put_u32(nlh, RTA_DST, dst.ip);

	mnl_attr_put_u32(nlh, RTA_OIF, iface);

	if (gateway) {
		if (is_ipv6)
			mnl_attr_put(nlh, RTA_GATEWAY, sizeof(struct in6_addr),
					&gw.ip6);
		else
			mnl_attr_put_u32(nlh, RTA_GATEWAY, gw.ip);
	}

	nl = mnl_socket_open(NETLINK_ROUTE);
	if (nl == NULL)
		tst_brk(TBROK, "mnl_socket_open failed (errno=%d): %s", errno,
			strerror(errno));

	if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0)
		tst_brk(TBROK, "mnl_socket_bind failed (errno=%d): %s", errno,
			strerror(errno));

	portid = mnl_socket_get_portid(nl);

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0)
		tst_brk(TBROK, "mnl_socket_sendto failed (errno=%d): %s", errno,
			strerror(errno));

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	if (ret < 0)
		tst_brk(TBROK, "mnl_socket_recvfrom failed (ret=%d, errno=%d): %s",
			ret, errno, strerror(errno));

	ret = mnl_cb_run(buf, ret, seq, portid, NULL, NULL);
	if (ret < 0)
		tst_brk(TBROK, "mnl_cb_run failed (ret=%d, errno=%d): %s", ret,
			errno, strerror(errno));

	mnl_socket_close(nl);
}

static void send_udp(char *remote)
{
	const char *msg = "foo";

	tst_setup_addrinfo(remote, p_opt, &hints, &rhost_addrinfo);

	SAFE_SENDTO(1, fd, msg, sizeof(msg), MSG_CONFIRM,
		rhost_addrinfo->ai_addr, rhost_addrinfo->ai_addrlen);
}

static void run(void)
{
	int i;

	tst_res(TINFO, "adding and deleting route %d times",
			num_loops);

	struct ip_addr *p_gw = gw, *p_rhost = rhost;
	struct iface *p_iface = iface;
	char dst[INET6_ADDRSTRLEN];

	for (i = 0; i < num_loops; i++) {
		if (!strncpy(dst, p_rhost->ip, sizeof(dst)))
			tst_brk(TBROK, "failed copy IP '%s'", p_rhost->ip);
		dst[strlen(p_rhost->ip)-1] = '\0';

		if (!strcat(dst, "0"))
			tst_brk(TBROK, "strcat failed: '%s'", dst);

		tst_res(TINFO, "!!! p_gw: %s, p_rhost: %s, dst: %s, p_iface: %s",
					p_gw->ip, p_rhost->ip, dst, p_iface->iface);

		rtnl_route(p_iface->index, dst, prefix, p_gw->ip, RTM_NEWROUTE);
		send_udp(p_rhost->ip);
		rtnl_route(p_iface->index, dst, prefix, p_gw->ip, RTM_DELROUTE);

		if (gw)
			p_gw = p_gw->next ?: gw;
		p_iface = p_iface->next ?: iface;
		p_rhost = p_rhost->next ?: rhost;
	}

	tst_res(TPASS, "routes created and deleted");
}

static struct tst_option options[] = {
	{"6", &ipv6_opt, "-6       Use IPv6 (default is IPv4)"},
	{"c:", &c_opt, "         Num loops (mandatory)"},
	{"d:", &d_opt, "-d iface Interface to work on (mandatory)"},
	{"g:", &g_opt, "-g x     Gateway IP"},
	{"p:", &p_opt, "-p port  Rhost port (mandatory)"},
	{"r:", &r_opt, "-r x     Rhost IP (mandatory)\n\n-g, -r IP parameter can contain more IP, separated by "
		CHR2STR(IP_ADDR_DELIM)},
	{NULL, NULL, NULL}
};
static struct tst_test test = {
	.test_all = run,
	.needs_root = 1,
	.setup = setup,
	.cleanup = cleanup,
	.options = options,
};
#else
	TST_TEST_TCONF("libmnl library and headers are required");
#endif /* HAVE_LIBMNL */
