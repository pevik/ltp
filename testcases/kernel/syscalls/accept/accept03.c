// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Copyright (C) 2023 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * [Description]
 *
 * Verify that accept() returns ENOTSOCK for non-socket file descriptors.
 */

#include <sys/socket.h>
#include <netinet/in.h>

#include "tst_test.h"

void check_accept(struct tst_fd *fd)
{
	struct sockaddr_in addr = {
		.sin_family = AF_INET,
		.sin_port = 0,
		.sin_addr = {.s_addr = INADDR_ANY},
	};
	socklen_t size = sizeof(addr);

	switch (fd->type) {
	case TST_FD_UNIX_SOCK:
	case TST_FD_INET_SOCK:
		return;
	default:
		break;
	}

	TST_EXP_FAIL2(accept(fd->fd, (void*)&addr, &size),
		ENOTSOCK, "accept() on %s", tst_fd_desc(fd));
}

static void verify_accept(void)
{
	TST_FD_FOREACH(fd)
		check_accept(&fd);
}

static struct tst_test test = {
	.test_all = verify_accept,
};
