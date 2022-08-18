// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef UTSTEST_H
#define UTSTEST_H

#include <stdlib.h>
#include "tst_test.h"
#include "lapi/syscalls.h"
#include "lapi/namespaces_constants.h"

enum {
	T_CLONE,
	T_UNSHARE,
	T_NONE,
};

static int dummy_child(LTP_ATTRIBUTE_UNUSED void *v)
{
	return 0;
}

static inline void check_newuts(void)
{
	int pid, status;

	pid = ltp_clone_quick(CLONE_NEWUTS | SIGCHLD, dummy_child, NULL);
	if (pid < 0)
		tst_brk(TCONF | TERRNO, "CLONE_NEWIPC not supported");

	SAFE_WAITPID(pid, &status, 0);
}

static inline int get_clone_unshare_enum(const char *str_op)
{
	int use_clone;

	use_clone = T_NONE;

	if (!str_op || !strcmp(str_op, "none"))
		use_clone = T_NONE;
	else if (!strcmp(str_op, "clone"))
		use_clone = T_CLONE;
	else if (!strcmp(str_op, "unshare"))
		use_clone = T_UNSHARE;
	else
		tst_brk(TBROK, "Test execution mode <clone|unshare|none>");

	return use_clone;
}

static inline pid_t clone_test(unsigned long clone_flags, int (*fn1)(void *arg), void *arg1)
{
	pid_t pid;

	pid = ltp_clone_quick(clone_flags | SIGCHLD, fn1, arg1);
	if (pid < 0)
		tst_brk(TBROK | TERRNO, "ltp_clone_quick error");

	return pid;
}

static inline pid_t unshare_test(unsigned long clone_flags, int (*fn1)(void *arg), void *arg1)
{
	pid_t pid;

	pid = SAFE_FORK();
	if (!pid) {
		SAFE_UNSHARE(clone_flags);

		fn1(arg1);
		exit(0);
	}

	return pid;
}

static inline pid_t plain_test(int (*fn1)(void *arg), void *arg1)
{
	pid_t pid;

	pid = SAFE_FORK();
	if (!pid) {
		fn1(arg1);
		exit(0);
	}

	return pid;
}

static inline pid_t clone_unshare_test(int use_clone, unsigned long clone_flags,
			       int (*fn1)(void *arg), void *arg1)
{
	pid_t pid = -1;

	switch (use_clone) {
	case T_NONE:
		pid = plain_test(fn1, arg1);
	break;
	case T_CLONE:
		pid = clone_test(clone_flags, fn1, arg1);
	break;
	case T_UNSHARE:
		pid = unshare_test(clone_flags, fn1, arg1);
	break;
	default:
		tst_brk(TBROK, "%s: bad use_clone option: %d", __FUNCTION__, use_clone);
	break;
	}

	return pid;
}

#endif
