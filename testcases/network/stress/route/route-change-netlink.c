// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2019 Petr Vorel <pvorel@suse.cz>
 */

#include "config.h"
#include "tst_test.h"

#ifdef HAVE_LIBMNL

#include <string.h>

#include <libmnl/libmnl.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <netdb.h>

#include "tst_net.h"
#include "tst_safe_net.h"
#include "tst_safe_stdio.h"

#define NS_TIMES_MAX 255
#define MAX_IP 255

static char *c_opt, *d_opt, *g_opt, *ipv6_opt, *l_opt, *m_opt, *r_opt;
static int family = AF_INET;
static int num_loops = 10000;
static int iface, is_ipv6, gwhost, lhost, max_ip = MAX_IP, rhost;
static unsigned int prefix;
static struct mnl_socket *nl;
static struct addrinfo *local_addrinfo;
static struct addrinfo *remote_addrinfo;
static int fd;

struct in_addr ip;
struct in6_addr ip6;

union {
	in_addr_t ip;
	struct in6_addr ip6;
} dst;

static void setup(void)
{
	prefix = 24;
	if (ipv6_opt) {
		family = AF_INET6;
		is_ipv6 = 1;
		prefix = 64;
	}

	if (tst_parse_int(c_opt, &num_loops, 1, INT_MAX))
		tst_brk(TBROK, "Invalid number of loops: '%s'", c_opt);

	if (!d_opt)
		tst_brk(TBROK, "Missing iface, specify it with -d");

	iface = if_nametoindex(d_opt);
	if (!iface)
		tst_brk(TBROK, "if_nametoindex failed");

	if (g_opt  && tst_parse_int(g_opt, &gwhost, 1, NS_TIMES_MAX))
		tst_brk(TBROK, "Invalid gateway host id: '%s'", g_opt);

	if (!l_opt)
		tst_brk(TBROK, "Missing local host id, specify it with -l");

	if (m_opt && tst_parse_int(m_opt, &max_ip, 1, MAX_IP))
		tst_brk(TBROK, "Invalid max IP: '%s'", m_opt);

	if (tst_parse_int(l_opt, &lhost, 1, NS_TIMES_MAX))
		tst_brk(TBROK, "Invalid local host id: '%s'", l_opt);

	if (!r_opt)
		tst_brk(TBROK, "Missing remote host id, specify it with -r");

	if (tst_parse_int(r_opt, &rhost, 1, NS_TIMES_MAX))
		tst_brk(TBROK, "Invalid remote host id: '%s'", r_opt);

	if (lhost == gwhost || lhost == rhost || rhost == gwhost)
		tst_brk(TBROK, "-g, -l and -r params must be different: %d, %d, %d",
				gwhost, rhost, lhost);
}

static void cleanup(void)
{
	if (fd > 0)
		close(fd);

	if (nl)
		mnl_socket_close(nl);

	if (remote_addrinfo)
		freeaddrinfo(remote_addrinfo);
}

static void rtnl_route(int iface, char *destination, uint32_t prefix, char *gateway, int type)
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

	if (!inet_pton(family, destination, &dst))
		tst_brk(TBROK, "inet_pton failed ('%s', errno=%d): %s", destination, errno, strerror(errno));

	if (gateway && !inet_pton(family, gateway, &gw))
		tst_brk(TBROK, "inet_pton failed ('%s', errno=%d): %s", gateway, errno, strerror(errno));

	nlh = mnl_nlmsg_put_header(buf);
	nlh->nlmsg_type	= type;
	nlh->nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK;
	nlh->nlmsg_seq = seq = time(NULL);

	rtm = mnl_nlmsg_put_extra_header(nlh, sizeof(struct rtmsg));
	rtm->rtm_family = family;
	rtm->rtm_dst_len = prefix;
	rtm->rtm_src_len = 0;
	rtm->rtm_tos = 0;
	rtm->rtm_protocol = RTPROT_STATIC;
	rtm->rtm_table = RT_TABLE_MAIN;
	rtm->rtm_type = RTN_UNICAST;
	rtm->rtm_scope = (gateway) ? RT_SCOPE_UNIVERSE : RT_SCOPE_LINK;
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
		tst_brk(TBROK, "mnl_socket_open failed (errno=%d): %s", errno, strerror(errno));

	if (mnl_socket_bind(nl, 0, MNL_SOCKET_AUTOPID) < 0)
		tst_brk(TBROK, "mnl_socket_bind failed (errno=%d): %s", errno, strerror(errno));

	portid = mnl_socket_get_portid(nl);

	if (mnl_socket_sendto(nl, nlh, nlh->nlmsg_len) < 0)
		tst_brk(TBROK, "mnl_socket_sendto failed (errno=%d): %s", errno, strerror(errno));

	ret = mnl_socket_recvfrom(nl, buf, sizeof(buf));
	if (ret < 0)
		tst_brk(TBROK, "mnl_socket_recvfrom failed (ret=%d, errno=%d): %s", ret, errno, strerror(errno));

	ret = mnl_cb_run(buf, ret, seq, portid, NULL, NULL);
	if (ret < 0)
		tst_brk(TBROK, "mnl_cb_run failed (ret=%d, errno=%d): %s", ret, errno, strerror(errno));

	mnl_socket_close(nl);
}

static void send_udp(char *local, char *remote)
{
	fd = SAFE_SOCKET(family, SOCK_DGRAM, IPPROTO_UDP);

	struct addrinfo hints;
	memset(&hints, 0, sizeof(struct addrinfo));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = 0;
	hints.ai_protocol = 0;
	hints.ai_addr = INADDR_ANY;
	tst_setup_addrinfo(local, NULL, &hints, &local_addrinfo);
	char *port;
	int _port = TST_GET_UNUSED_PORT(family, SOCK_DGRAM);
	SAFE_ASPRINTF(&port, "%d", _port);
	tst_setup_addrinfo(remote, port, &hints, &remote_addrinfo);

	SAFE_BIND(fd, local_addrinfo->ai_addr, local_addrinfo->ai_addrlen);

	SAFE_SENDTO(1, fd, remote, strlen(remote), MSG_CONFIRM,
		remote_addrinfo->ai_addr, remote_addrinfo->ai_addrlen);

	close(fd);
}

static void run(void)
{
	int i, j;
	char *destination, *gateway = NULL, *local, *remote;

	tst_res(TINFO, "Adding and deleting route with different destination %d times", num_loops);
	for (i = 0; i < num_loops; i++) {
		j = i % max_ip;
		local = tst_ipaddr_un(family, j, lhost);
		remote = tst_ipaddr_un(family, j, rhost);
		if (g_opt) {
			destination = tst_ipaddr_un(family, 0, 0);
			gateway = tst_ipaddr_un(family, j, gwhost);
		} else {
			destination = tst_ipaddr_un(family, j, 0);
		}

		rtnl_route(iface, destination, prefix, gateway, RTM_NEWROUTE);
		send_udp(local, remote);
		rtnl_route(iface, destination, prefix, gateway, RTM_DELROUTE);
	}

	tst_res(TPASS, "Routes created and deleted");
}

static struct tst_option options[] = {
	{"6", &ipv6_opt, "-6       Use IPv6 (default is IPv4)"},
	{"c:", &c_opt, "-c x     Number of loops"},
	{"d:", &d_opt, "-d IFACE Interface to work on"},
	{"g:", &g_opt, "-g x     Change gateway instead of destination, x: Host id of gateway"},
	{"l:", &l_opt, "-l x     Local host id"},
	{"m:", &m_opt, "-m x     Max IP addresses, this modulos -c"},
	{"r:", &r_opt, "-r x     Remote host id"},
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
	TST_TEST_TCONF("netlink libraries and headers are required");
#endif /* HAVE_LIBNL_CLI3 */
