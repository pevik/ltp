// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Linux Test Project, 2026
 */

/*\
 * Verify that epoll_wait fails with EINTR when a signal arrives while waiting
 * for events.
 *
 * [Algorithm]
 *
 * - Create an epoll instance and register a socket fd for EPOLLIN.
 * - Fork a child that waits for the parent to enter epoll_wait, then sends
 *   SIGUSR1 to interrupt it.
 * - Verify that epoll_wait returns -1 with errno set to EINTR.
 */

#include <stdlib.h>
#include <sys/epoll.h>

#include "tst_test.h"
#include "tst_epoll.h"

static int efd = -1;

static void sighandler(int sig LTP_ATTRIBUTE_UNUSED)
{
}

static void setup(void)
{
	static struct sigaction sa = {
		.sa_handler = sighandler,
	};

	SAFE_SIGEMPTYSET(&sa.sa_mask);
	SAFE_SIGACTION(SIGUSR1, &sa, NULL);

	efd = SAFE_EPOLL_CREATE1(0);
}

static void run(void)
{
	pid_t pid = SAFE_FORK();

	if (!pid) {
		struct epoll_event ev;

		TST_EXP_FAIL(epoll_wait(efd, &ev, 1, -1), EINTR,
			     "epoll_wait() interrupted by signal");
		exit(0);
	}

	TST_PROCESS_STATE_WAIT(pid, 'S', 0);
	SAFE_KILL(pid, SIGUSR1);
}

static void cleanup(void)
{
	if (efd != -1)
		SAFE_CLOSE(efd);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.forks_child = 1,
};
