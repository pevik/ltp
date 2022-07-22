// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef MQNS_H
#define MQNS_H

#include <stdlib.h>
#include "lapi/namespaces_constants.h"
#include "tst_test.h"
#include "tst_safe_posix_ipc.h"

enum {
	T_CLONE,
	T_UNSHARE,
	T_NONE,
};

static int dummy_child1(void *v)
{
	(void)v;
	return 0;
}

static inline void check_newipc(void)
{
	int pid, status;

	pid = ltp_clone_quick(CLONE_NEWIPC | SIGCHLD, dummy_child1, NULL);
	if (pid < 0)
		tst_brk(TCONF | TERRNO, "CLONE_NEWIPC not supported");

	SAFE_WAITPID(pid, &status, 0);
}

static inline int get_clone_unshare_enum(const char* str_op)
{
	if (!str_op)
		return T_NONE;
	else if (!strcmp(str_op, "clone"))
		return T_CLONE;
	else if (!strcmp(str_op, "unshare"))
		return T_UNSHARE;

	return T_NONE;
}

static void clone_test(unsigned long clone_flags, int (*fn1)(void *arg), void *arg1)
{
	int pid;

	pid = ltp_clone_quick(clone_flags | SIGCHLD, fn1, arg1);
	if (pid < 0)
		tst_brk(TBROK | TERRNO, "ltp_clone_quick error");
}

static void unshare_test(unsigned long clone_flags, int (*fn1)(void *arg), void *arg1)
{
	int pid;

	pid = SAFE_FORK();
	if (!pid) {
		SAFE_UNSHARE(clone_flags);

		fn1(arg1);
		exit(0);
	}
}

static void plain_test(int (*fn1)(void *arg), void *arg1)
{
	int pid;

	pid = SAFE_FORK();
	if (!pid) {
		fn1(arg1);
		exit(0);
	}
}

static void clone_unshare_test(int use_clone, unsigned long clone_flags,
			       int (*fn1)(void *arg), void *arg1)
{
	switch (use_clone) {
	case T_NONE:
		plain_test(fn1, arg1);
	break;
	case T_CLONE:
		clone_test(clone_flags, fn1, arg1);
	break;
	case T_UNSHARE:
		unshare_test(clone_flags, fn1, arg1);
	break;
	default:
		tst_brk(TBROK, "%s: bad use_clone option: %d", __FUNCTION__, use_clone);
	break;
	}
}

#endif /* MQNS_H */
