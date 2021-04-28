// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2014 Red Hat, Inc.
 * Copyright (c) 2021 Petr Vorel <pvorel@suse.cz>
 */

/*\
 * [DESCRIPTION]
 *
 * Tests a netlink interface inside a new network namespace.
 *
 * - Unshares a network namespace (so network related actions
 *   have no effect on a real system).
 * - Forks a child which creates a NETLINK_ROUTE netlink socket
 *   and listens to RTMGRP_LINK (network interface create/delete/up/down)
 *   multicast group.
 * - Child then waits for parent approval to receive data from socket
 * - Parent creates a new TAP interface (dummy0) and immediately
 *   removes it (which should generate some data in child's netlink socket).
 *   Then it allows child to continue.
 * - As the child was listening to RTMGRP_LINK multicast group, it should
 *   detect the new interface creation/deletion (by reading data from netlink
 *   socket), if so, the test passes, otherwise it fails.
\*/

#define _GNU_SOURCE
#include <sys/wait.h>
#include <asm/types.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sched.h>

#include "tst_test.h"
#include "tst_safe_macros.h"
#include "lapi/namespaces_constants.h"

#define MAX_TRIES 1000

int child_func(void)
{
	int fd, len, event_found, tries;
	struct sockaddr_nl sa;
	char buffer[4096];
	struct nlmsghdr *nlh;

	/* child will listen to a network interface create/delete/up/down events */
	memset(&sa, 0, sizeof(sa));
	sa.nl_family = AF_NETLINK;
	sa.nl_groups = RTMGRP_LINK;

	fd = socket(AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd == -1) {
		perror("socket");
		return 1;
	}
	if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) == -1) {
		perror("bind");
		close(fd);
		return 1;
	}

	/* waits for parent to create an interface */
	TST_CHECKPOINT_WAIT(0);

	/* To get rid of "resource temporarily unavailable" errors
	 * when testing with -i option */
	tries = 0;
	event_found = 0;
	nlh = (struct nlmsghdr *) buffer;
	while (tries < MAX_TRIES) {
		len = recv(fd, nlh, sizeof(buffer), MSG_DONTWAIT);
		if (len > 0) {
			/* stop receiving only on interface create/delete
			 * event */
			if (nlh->nlmsg_type == RTM_NEWLINK ||
			    nlh->nlmsg_type == RTM_DELLINK) {
				event_found++;
				break;
			}
		}
		usleep(10000);
		tries++;
	}

	close(fd);

	if (!event_found) {
		perror("recv");
		return 1;
	}

	return 0;
}

static void test_netns_netlink(void)
{
	pid_t pid;
	int status;

	/* unshares the network namespace */
	SAFE_UNSHARE(CLONE_NEWNET);

	pid = SAFE_FORK();
	if (pid == 0) {
		_exit(child_func());
	}

	/* creates TAP network interface dummy0 */
	if (WEXITSTATUS(system("ip tuntap add dev dummy0 mode tap")))
		tst_brk(TBROK, "system() failed");

	/* removes previously created dummy0 device */
	if (WEXITSTATUS(system("ip tuntap del mode tap dummy0")))
		tst_brk(TBROK, "system() failed");

	/* allow child to continue */
	TST_CHECKPOINT_WAKE(0);

	SAFE_WAITPID(pid, &status, 0);
	if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
		tst_res(TFAIL, "netlink interface fail");
		return;
	}
	if (WIFSIGNALED(status)) {
		tst_res(TFAIL, "child was killed with signal %s",
			 tst_strsig(WTERMSIG(status)));
		return;
	}

	tst_res(TPASS, "netlink interface pass");
}


static struct tst_test test = {
	.test_all = test_netns_netlink,
	.needs_checkpoints = 1,
	.needs_tmpdir = 1,
	.needs_root = 1,
	.forks_child = 1,
};
