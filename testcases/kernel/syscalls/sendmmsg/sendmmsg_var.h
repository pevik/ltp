// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2018 Google, Inc.
 */

#ifndef SENDMMSG_VAR__
#define SENDMMSG_VAR__

#include "lapi/syscalls.h"

static int do_sendmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
		       int flags)
{
	switch (tst_variant) {
	case 0:
		return tst_syscall(__NR_sendmmsg, sockfd, msgvec, vlen, flags);
	case 1:
#ifdef HAVE_SENDMMSG
		return sendmmsg(sockfd, msgvec, vlen, flags);
#else
		tst_brk(TCONF, "libc sendmmsg() not present");
#endif
	}

	return -1;
}

static int do_recvmmsg(int sockfd, struct mmsghdr *msgvec, unsigned int vlen,
		       int flags, struct timespec *timeout)
{
	switch (tst_variant) {
	case 0:
		return tst_syscall(__NR_recvmmsg, sockfd, msgvec, vlen, flags,
				   timeout);
	case 1:
#ifdef HAVE_RECVMMSG
		return recvmmsg(sockfd, msgvec, vlen, flags, timeout);
#else
		tst_brk(TCONF, "libc recvmmsg() not present");
#endif
	}

	return -1;
}

static const char *variant_desc[] = {
	"raw sendmmsg() and recvmmsg() syscalls",
	"libc sendmmsg() and recvmmsg()",
	NULL
};

#endif /* SENDMMSG_VAR__ */
