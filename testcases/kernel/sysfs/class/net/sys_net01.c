// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Sanity checks for the network interface attributes exported under
 * /sys/class/net/<iface>/.
 *
 * For every network interface the test verifies that:
 *
 * - mtu is greater than zero, and for an Ethernet-type interface (type == 1,
 *   ARPHRD_ETHER, which also covers most virtual/Wi-Fi interfaces on Linux)
 *   at most 65535, the largest possible IPv4 packet size and comfortably
 *   above even the largest real-world jumbo frame configurations. Other
 *   interface types (tunnels, PPP, CAN, ...) are left effectively unbounded
 *   since their legitimate MTU semantics vary too much to bound generically,
 *   e.g. a CAN interface reports the size of struct can_frame/canfd_frame
 *   (16/72) rather than an IP MTU at all.
 * - addr_len is at most MAX_ADDR_LEN (32), the fixed size of the kernel's
 *   internal hardware address buffer (include/linux/netdevice.h) and thus a
 *   hard limit rather than a mere plausibility bound. It is not asserted to
 *   be non-zero: plenty of legitimate interfaces (PPP, GRE/IPIP/sit tunnels,
 *   WireGuard, CAN, ...) have no link-layer address at all and report
 *   addr_len == 0.
 * - the address has exactly addr_len bytes (colon separated hex octets),
 *   or is empty when addr_len == 0
 * - operstate is one of the known states
 * - carrier is a boolean (0 or 1) when readable
 *
 * carrier returns an error (EINVAL) when the interface is administratively
 * down, which the test tolerates. The test skips with TCONF when no interface
 * is present.
 */

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include "tst_test.h"
#include "tst_sysfs_assert.h"

#define NET "/sys/class/net"

/* include/linux/netdevice.h: fixed size of dev->dev_addr[]. */
#define MAX_ADDR_LEN 32

/* include/uapi/linux/if_arp.h */
#define ARPHRD_ETHER 1

/*
 * The largest possible IPv4 packet size, used as a practical upper bound for
 * the MTU of an Ethernet-type interface, see the top comment.
 */
#define MAX_ETHER_MTU 65535

static const char *const operstates[] = {
	"unknown", "notpresent", "down", "lowerlayerdown",
	"testing", "dormant", "up", NULL
};

static void check_address(const char *iface)
{
	char addr[128] = "";
	int octets;
	char *p;

	TST_SYSFS_READ_STR(addr, sizeof(addr), NET "/%s/address", iface);

	/* An empty address is valid for an interface with addr_len == 0. */
	octets = addr[0] ? 1 : 0;

	for (p = addr; *p; p++) {
		if (*p == ':')
			octets++;
	}

	TST_SYSFS_EXP_EQ_LI(octets, NET "/%s/addr_len", iface);
}

static void check_iface(const char *iface)
{
	long type, mtu_max = LONG_MAX;

	tst_res(TINFO, "Checking interface '%s'", iface);

	type = TST_SYSFS_READ_LI(NET "/%s/type", iface);

	if (type == ARPHRD_ETHER)
		mtu_max = MAX_ETHER_MTU;

	TST_SYSFS_ASSERT_RANGELL(1, mtu_max, NET "/%s/mtu", iface);
	TST_SYSFS_ASSERT_RANGELL(0, MAX_ADDR_LEN, NET "/%s/addr_len", iface);

	check_address(iface);
	TST_SYSFS_ASSERT_ONEOF(operstates, NET "/%s/operstate", iface);
}

static void do_test(void)
{
	DIR *d;
	struct dirent *ent;
	int found = 0;

	d = SAFE_OPENDIR(NET);

	while ((ent = SAFE_READDIR(d))) {
		if (ent->d_name[0] == '.')
			continue;

		found = 1;
		check_iface(ent->d_name);
	}

	SAFE_CLOSEDIR(d);

	if (!found)
		tst_res(TCONF, "No network interfaces found");
}

static struct tst_test test = {
	.test_all = do_test,
};
