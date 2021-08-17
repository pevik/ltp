// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021. All rights reserved.
 * Author: Xie Ziyao <xieziyao@huawei.com>
 */

/*\
 * [Description]
 *
 * Check that epoll_ctl returns zero with different combinations of events on
 * success.
 */

#include <poll.h>
#include <sys/epoll.h>

#include "tst_test.h"

#define WIDTH_EPOLL_EVENTS 256

static int epfd, fds[2];
static struct epoll_event events = {.events = EPOLLIN};

static void run_all(void)
{
	unsigned int index, events_bitmap;

	for (index = 0; index < WIDTH_EPOLL_EVENTS; index++) {
		events_bitmap = ((EPOLLIN * ((index & 0x01) >> 0)) |
				(EPOLLOUT * ((index & 0x02) >> 1)) |
				(EPOLLPRI * ((index & 0x04) >> 2)) |
				(EPOLLERR * ((index & 0x08) >> 3)) |
				(EPOLLHUP * ((index & 0x10) >> 4)) |
				(EPOLLET * ((index & 0x20) >> 5)) |
				(EPOLLONESHOT * ((index & 0x40) >> 6)) |
				(EPOLLRDHUP * ((index & 0x80) >> 7)));

		events.events = events_bitmap;
		TST_EXP_PASS(epoll_ctl(epfd, EPOLL_CTL_MOD, fds[0], &events),
			     "epoll_ctl(..., EPOLL_CTL_MOD, ...) with events.events=%x",
			     events.events);
	}
}

static void setup(void)
{
	epfd = epoll_create(1);
	if (epfd == -1)
		tst_brk(TBROK | TERRNO, "fail to create epoll instance");

	SAFE_PIPE(fds);
	events.data.fd = fds[0];

	if (epoll_ctl(epfd, EPOLL_CTL_ADD, fds[0], &events))
		tst_brk(TBROK | TERRNO, "epoll_clt(..., EPOLL_CTL_ADD, ...)");
}

static void cleanup(void)
{
	if (epfd)
		SAFE_CLOSE(epfd);

	if (fds[0])
		SAFE_CLOSE(fds[0]);

	if (fds[1])
		SAFE_CLOSE(fds[1]);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run_all,
	.min_kver = "2.6.17",
};
