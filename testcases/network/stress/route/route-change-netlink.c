// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2019 Petr Vorel <pvorel@suse.cz>
 */

// FIXME: debug
/*
 * dst: lhost: 10.0.0.2, rhost: 10.23.x.1
 * gw:  lhost: 10.23.1.1, gw (on rhost): 10.23.1.x, rhost: 10.23.0.1
 * if:  lhost: 10.23.x.2, gw (on rhost): 10.23.x.1, rhost: 10.23.0.1, switching ifaces on lhost
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

// FIXME: debug
#define NS_TIMES_MAX 255
#define MAX_IP 255

#define IP_ADDR_DELIMITER '^'
#define STR(x) #x
#define CHR2STR(x) STR(x)

static char *d_opt, *g_opt, *ipv6_opt, *l_opt, *r_opt;
static int family = AF_INET;
static unsigned int num_loops = 10;
static unsigned int max;
static int is_ipv6, max_ip = MAX_IP;
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

struct iface {
	char iface[IFNAMSIZ];
	unsigned int index;
	struct iface *next;
};

struct ip_addr {
	char ip[INET6_ADDRSTRLEN];
	struct ip_addr *next;
};

static struct ip_addr *gw, *lhost, *rhost;
static struct iface *iface;
static unsigned int gw_len, iface_len, lhost_len, rhost_len;

void save_iface_str(struct iface **list, const char *item)
{
	struct iface *n = SAFE_MALLOC(sizeof(*n));
	strncpy(n->iface, item, sizeof(n->iface));

	n->index = if_nametoindex(item);
	if (!n->index)
		tst_brk(TBROK, "if_nametoindex failed, '%s' not found", item);
	n->next = *list;
	*list = n;
}

void save_ip_str(struct ip_addr **list, const char *item)
{
	struct ip_addr *n = SAFE_MALLOC(sizeof(*n));
	strncpy(n->ip, item, sizeof(n->ip));
	n->next = *list;
	*list = n;
	fprintf(stderr, "%s:%d %s(): item: '%s'\n", __FILE__, __LINE__, __func__, item); // FIXME: debug
}

int save_iface(struct iface **list, char *item)
{
	int len = 0;

	while ((item = strtok(item, CHR2STR(IP_ADDR_DELIMITER))) != NULL) {
		save_iface_str(list, item);
		item = NULL;
		len++;
	}

	return len;
}

int save_ip(struct ip_addr **list, char *item)
{
	int len = 0;

	while ((item = strtok(item, CHR2STR(IP_ADDR_DELIMITER))) != NULL) {
		save_ip_str(list, item);
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

	if (!d_opt)
		tst_brk(TBROK, "Missing iface (-d IFACE)");

	if (!l_opt)
		tst_brk(TBROK, "Missing local host IP (-l IP)");

	if (!r_opt)
		tst_brk(TBROK, "Missing remote host IP (-r IP)");

	iface_len = save_iface(&iface, d_opt);
	lhost_len = save_ip(&lhost, l_opt);
	rhost_len = save_ip(&rhost, r_opt);

	max = MAX(lhost_len, rhost_len);
	if (lhost_len > 1 && rhost_len > 1 && lhost_len != rhost_len)
		tst_brk(TBROK, "-l and -r have more IP, they need to have the same count");

	max = MAX(iface_len, max);
	if (iface_len > 1 && max > 1 && iface_len != max)
		tst_brk(TBROK, "-d has more NIC and %s has more IP, they need to have the same count",
				lhost_len > 1 ? "-l" : "-r");

	if (g_opt) {
		gw_len = save_ip(&gw, g_opt);
		max = MAX(gw_len, max);

		if (gw_len > 1 && max > 1 && gw_len != max) {
			if (iface_len == max)
				tst_brk(TBROK, "-d has more NIC and -g has more IP, they need to have the same count");
			else
				tst_brk(TBROK, "-g and %s have more IP, they need to have the same count",
						lhost_len > 1 ? "-l" : "-r");
		}
	}
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

	if (!inet_pton(family, destination, &dst))
		tst_brk(TBROK, "inet_pton failed ('%s', errno=%d): %s",
			destination, errno, strerror(errno));

	if (gateway && !inet_pton(family, gateway, &gw))
		tst_brk(TBROK, "inet_pton failed ('%s', errno=%d): %s", gateway,
			errno, strerror(errno));

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

	/*
	 * FIXME: +   tst_setup_addrinfo(remote, port, &hints, &remote_addrinfo);
	 * Getting local unused port and using as a remote port?
	char *port;
	int _port = TST_GET_UNUSED_PORT(family, SOCK_DGRAM);
	SAFE_ASPRINTF(&port, "%d", _port);
	tst_setup_addrinfo(remote, port, &hints, &remote_addrinfo);
	 */

	SAFE_SENDTO(1, fd, remote, strlen(remote), MSG_CONFIRM,
		remote_addrinfo->ai_addr, remote_addrinfo->ai_addrlen);

	close(fd);
}

static void run(void)
{
	unsigned int i;

	/*
	 * TODO: adapt from concept the same ip on lhost and rhost to:
	 * gw: lhost: 10.23.x.2, gw (on rhost): 10.23.x.1, rhost: 10.23.0.1
	 * => needs destination 10.23.0.0 (replace last 1 on rhost to 0)
	 *
	 * dst: lhost: 10.0.0.2, rhost: 10.23.x.1
	 * => needs destination 10.23.x.0 (replace last 1 on rhost to 0)
	 *
	 * if: lhost: 10.23.x.2, gw (on rhost): 10.23.x.1, rhost: 10.23.0.1, switching ifaces on lhost
	 * => needs destination 10.23.0.0 (replace last 1 on rhost to 0)
	 */
	tst_res(TINFO, "Adding and deleting route with different destination %d times", num_loops);

	struct ip_addr *p_gw = gw, *p_lhost = lhost, *p_rhost = rhost;
	struct iface *p_iface = iface;
	char *dst[INET6_ADDRSTRLEN];
	char *p_dst = dst;

	fprintf(stderr, "%s:%d %s(): max: %d, num_loops: %d\n", __FILE__, __LINE__, __func__, max, num_loops); // FIXME: debug
	for (i = 0; i < num_loops; i++) {
		fprintf(stderr, "%s:%d %s(): ===== i: %d =====\n", __FILE__, __LINE__, __func__, i); // FIXME: debug
		fprintf(stderr, "%s:%d %s(): iface: %s (%d)\n", __FILE__, __LINE__, __func__, iface->iface, iface->index); // FIXME: debug
		if (gw)
			fprintf(stderr, "%s:%d %s(): gw: %s\n", __FILE__, __LINE__, __func__, gw->ip); // FIXME: debug
		fprintf(stderr, "%s:%d %s(): lhost: %s\n", __FILE__, __LINE__, __func__, lhost->ip); // FIXME: debug
		fprintf(stderr, "%s:%d %s(): rhost: %s\n", __FILE__, __LINE__, __func__, rhost->ip); // FIXME: debug

		if (lhost->next && lhost->next->ip)
			fprintf(stderr, "%s:%d %s(): lhost next: %s\n", __FILE__, __LINE__, __func__, lhost->next); // FIXME: debug
		if (rhost->next && rhost->next->ip)
			fprintf(stderr, "%s:%d %s(): rhost next: %s\n", __FILE__, __LINE__, __func__, rhost->next); // FIXME: debug

		if (!strncpy(p_dst, rhost->ip, strlen(rhost->ip) - 1))
			tst_brk(TBROK, "failed copy IP '%s'", rhost->ip);
		fprintf(stderr, "%s:%d %s(): before: %d\n", __FILE__, __LINE__, __func__, strlen(rhost->ip) - 1); // FIXME: debug
		p_dst[strlen(rhost->ip) - 1] = '\0';
		fprintf(stderr, "%s:%d %s(): after\n", __FILE__, __LINE__, __func__); // FIXME: debug

		fprintf(stderr, "%s:%d %s(): p_dst: '%s', rhost: '%s', len: %ld\n",
				__FILE__, __LINE__, __func__, p_dst, rhost->ip, strlen(rhost->ip)); // FIXME: debug
		fprintf(stderr, "%s:%d %s(): 2: p_dst: '%s'\n", __FILE__, __LINE__, __func__, p_dst); // FIXME: debug

		if (!strcat(p_dst, "0"))
			tst_brk(TBROK, "strcat failed: '%s'", p_dst);
		fprintf(stderr, "%s:%d %s(): 3: p_dst: '%s'\n", __FILE__, __LINE__, __func__, p_dst); // FIXME: debug

		rtnl_route(iface->index, dst, prefix, gw, RTM_NEWROUTE);
		send_udp(lhost, rhost);
		rtnl_route(iface->index, dst, prefix, gw, RTM_DELROUTE);

		if (gw)
			gw = gw->next ?: p_gw;
		iface = iface->next ?: p_iface;
		lhost = lhost->next ?: p_lhost;
		rhost = rhost->next ?: p_rhost;
	}

	// FIXME: debug
	/*
	int i, j;
	char *destination, *gateway = NULL, *local, *remote;

	for (i = 0; i < num_loops; i++) {
		j = i % max_ip;
		// FIXME: debug
		local = TST_IPADDR_UN(family, j, lhost);
		remote = TST_IPADDR_UN(family, j, rhost);
		if (g_opt) {
			destination = TST_IPADDR_UN(family, 0, 0);
			gateway = TST_IPADDR_UN(family, j, gw);
		} else {
			destination = TST_IPADDR_UN(family, j, 0);
		}

		rtnl_route(iface, destination, prefix, gateway, RTM_NEWROUTE);
		send_udp(local, remote);
		rtnl_route(iface, destination, prefix, gateway, RTM_DELROUTE);
	}
	*/

	tst_res(TPASS, "Routes created and deleted");
}

static struct tst_option options[] = {
	{"6", &ipv6_opt, "-6       Use IPv6 (default is IPv4)"},
	{"d:", &d_opt, "-d IFACE Interface to work on (mandatory)"},
	{"g:", &g_opt, "-g x     Gateway IP"},
	{"l:", &l_opt, "-l x     Local host IP (mandatory)"},
	{"r:", &r_opt, "-r x     Remote host IP (mandatory)\n\n-g, -l, -g IP paramter can contain more IP, separated by "
		CHR2STR(IP_ADDR_DELIMITER)},
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
#endif /* HAVE_LIBNL_CLI3 */
