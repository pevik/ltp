/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (c) 2021 Linux Test Project
 */

#ifndef TST_NETDEVICE_H
#define TST_NETDEVICE_H

/* Find device index for given network interface name. */
int tst_netdevice_index(const char *file, const int lineno, const char *ifname);
#define NETDEVICE_INDEX(ifname) \
	tst_netdevice_index(__FILE__, __LINE__, (ifname))

/* Activate or deactivate network interface */
int tst_netdevice_activate(const char *file, const int lineno,
	const char *ifname, int up);
#define NETDEVICE_ACTIVATE(ifname, up) \
	tst_netdevice_activate(__FILE__, __LINE__, (ifname), (up))

/* Create a connected pair of virtual network devices */
int tst_create_veth_pair(const char *file, const int lineno,
	const char *ifname1, const char *ifname2);
#define CREATE_VETH_PAIR(ifname1, ifname2) \
	tst_create_veth_pair(__FILE__, __LINE__, (ifname1), (ifname2))

int tst_remove_netdevice(const char *file, const int lineno,
	const char *ifname);
#define REMOVE_NETDEVICE(ifname) \
	tst_remove_netdevice(__FILE__, __LINE__, (ifname))

int tst_netdevice_add_address(const char *file, const int lineno,
	const char *ifname, unsigned int family, const void *address,
	unsigned int prefix, size_t addrlen, unsigned int flags);
#define NETDEVICE_ADD_ADDRESS(ifname, family, address, prefix, addrlen, flags) \
	tst_netdevice_add_address(__FILE__, __LINE__, (ifname), (family), \
		(address), (prefix), (addrlen), (flags))

int tst_netdevice_add_address_inet(const char *file, const int lineno,
	const char *ifname, in_addr_t address, unsigned int prefix,
	unsigned int flags);
#define NETDEVICE_ADD_ADDRESS_INET(ifname, address, prefix, flags) \
	tst_netdevice_add_address_inet(__FILE__, __LINE__, (ifname), \
		(address), (prefix), (flags))

int tst_netdevice_remove_address(const char *file, const int lineno,
	const char *ifname, unsigned int family, const void *address,
	size_t addrlen);
#define NETDEVICE_REMOVE_ADDRESS(ifname, family, address, addrlen) \
	tst_netdevice_remove_address(__FILE__, __LINE__, (ifname), (family), \
		(address), (addrlen))

int tst_netdevice_remove_address_inet(const char *file, const int lineno,
	const char *ifname, in_addr_t address);
#define NETDEVICE_REMOVE_ADDRESS_INET(ifname, address) \
	tst_netdevice_remove_address_inet(__FILE__, __LINE__, (ifname), \
		(address))

int tst_netdevice_change_ns_fd(const char *file, const int lineno,
	const char *ifname, int nsfd);
#define NETDEVICE_CHANGE_NS_FD(ifname, nsfd) \
	tst_netdevice_change_ns_fd(__FILE__, __LINE__, (ifname), (nsfd))

int tst_netdevice_change_ns_pid(const char *file, const int lineno,
	const char *ifname, pid_t nspid);
#define NETDEVICE_CHANGE_NS_PID(ifname, nspid) \
	tst_netdevice_change_ns_pid(__FILE__, __LINE__, (ifname), (nspid))

/*
 * Add new static entry to main routing table. If you specify gateway address,
 * the interface name is optional.
 */
int tst_netdevice_add_route(const char *file, const int lineno,
	const char *ifname, unsigned int family, const void *srcaddr,
	unsigned int srcprefix, size_t srclen, const void *dstaddr,
	unsigned int dstprefix, size_t dstlen, const void *gateway,
	size_t gatewaylen);
#define NETDEVICE_ADD_ROUTE(ifname, family, srcaddr, srcprefix, srclen, \
	dstaddr, dstprefix, dstlen, gateway, gatewaylen) \
	tst_netdevice_add_route(__FILE__, __LINE__, (ifname), (family), \
		(srcaddr), (srcprefix), (srclen), (dstaddr), (dstprefix), \
		(dstlen), (gateway), (gatewaylen))

/*
 * Simplified function for adding IPv4 static route. If you set srcprefix
 * or dstprefix to 0, the corresponding address will be ignored. Interface
 * name is optional if gateway address is non-zero.
 */
int tst_netdevice_add_route_inet(const char *file, const int lineno,
	const char *ifname, in_addr_t srcaddr, unsigned int srcprefix,
	in_addr_t dstaddr, unsigned int dstprefix, in_addr_t gateway);
#define NETDEVICE_ADD_ROUTE_INET(ifname, srcaddr, srcprefix, dstaddr, \
	dstprefix, gateway) \
	tst_netdevice_add_route_inet(__FILE__, __LINE__, (ifname), (srcaddr), \
		(srcprefix), (dstaddr), (dstprefix), (gateway))

/*
 * Remove static entry from main routing table.
 */
int tst_netdevice_remove_route(const char *file, const int lineno,
	const char *ifname, unsigned int family, const void *srcaddr,
	unsigned int srcprefix, size_t srclen, const void *dstaddr,
	unsigned int dstprefix, size_t dstlen, const void *gateway,
	size_t gatewaylen);
#define NETDEVICE_REMOVE_ROUTE(ifname, family, srcaddr, srcprefix, srclen, \
	dstaddr, dstprefix, dstlen, gateway, gatewaylen) \
	tst_netdevice_remove_route(__FILE__, __LINE__, (ifname), (family), \
		(srcaddr), (srcprefix), (srclen), (dstaddr), (dstprefix), \
		(dstlen), (gateway), (gatewaylen))

/*
 * Simplified function for removing IPv4 static route.
 */
int tst_netdevice_remove_route_inet(const char *file, const int lineno,
	const char *ifname, in_addr_t srcaddr, unsigned int srcprefix,
	in_addr_t dstaddr, unsigned int dstprefix, in_addr_t gateway);
#define NETDEVICE_REMOVE_ROUTE_INET(ifname, srcaddr, srcprefix, dstaddr, \
	dstprefix, gateway) \
	tst_netdevice_remove_route_inet(__FILE__, __LINE__, (ifname), \
		(srcaddr), (srcprefix), (dstaddr), (dstprefix), (gateway))

#endif /* TST_NETDEVICE_H */
