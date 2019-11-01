// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2017-2019 Petr Vorel <pvorel@suse.cz>
 */

#ifndef TST_NET_H_
#define TST_NET_H_

#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/ip.h>

void tst_get_in_addr(const char *ip_str, struct in_addr *ip);
void tst_get_in6_addr(const char *ip_str, struct in6_addr *ip6);

/*
 * Find valid connection address for a given bound socket
 */
socklen_t tst_get_connect_address(int sock, struct sockaddr_storage *addr);

/*
 * Initialize AF_INET/AF_INET6 socket address structure with address and port
 */
void tst_init_sockaddr_inet(struct sockaddr_in *sa, const char *ip_str, uint16_t port);
void tst_init_sockaddr_inet_bin(struct sockaddr_in *sa, uint32_t ip_val, uint16_t port);
void tst_init_sockaddr_inet6(struct sockaddr_in6 *sa, const char *ip_str, uint16_t port);
void tst_init_sockaddr_inet6_bin(struct sockaddr_in6 *sa, const struct in6_addr *ip_val, uint16_t port);

void tst_setup_addrinfo(const char *src_addr, const char *port,
		    const struct addrinfo *hints,
		    struct addrinfo **addr_info);

#define TST_IPADDR_UN(ai_family, net, host) \
	(tst_ipaddr_un(ai_family, net, host, 0, 0, 0, 0))

#define TST_IPADDR_UN_HOST(ai_family, net, host, min_host, max_host) \
	(tst_ipaddr_un(ai_family, net, host, min_host, max_host, 0, 0))

#define TST_IPADDR_UN_NET(ai_family, net, host, min_net, max_net) \
	(tst_ipaddr_un(ai_family, net, host, 0, 0, min_net, max_net))
char *tst_ipaddr_un(int ai_family, unsigned int net, unsigned int host, unsigned
		    int min_host, unsigned int max_host, unsigned int min_net,
		    unsigned int max_net);
#endif /* TST_NET_H_ */
