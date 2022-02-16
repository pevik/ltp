// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2022 FUJITSU LIMITED. All rights reserved.
 * Author: Yang Xu <xuyang2018.jy@fujitsu.com>
 */

/*\
 * [Description]
 *
 * Tests basic error handling of the pidfd_open syscall.
 *
 * - EBADF pidfd is not a valid PID file descriptor
 * - EBADF targetfd is not an open file descriptor in the process referred
 *   to by pidfd
 * - EINVAL flags is not 0
 * - ESRCH the process referred to by pidfd does not exist(it has terminated and
 *   been waited on))
 * - EPERM the calling process doesn't have PTRACE_MODE_ATTACH_REALCREDS permissions
 *   over the process referred to by pidfd
 */

#include <stdlib.h>
#include <pwd.h>
#include "tst_test.h"
#include "lapi/pidfd_open.h"
#include "lapi/pidfd_getfd.h"

static int valid_pidfd, expire_pidfd, invalid_pidfd = -1;
static uid_t uid;

static struct tcase {
	char *name;
	int *pidfd;
	int targetfd;
	int flags;
	int exp_errno;
} tcases[] = {
	{"invalid pidfd", &invalid_pidfd, 0, 0, EBADF},
	{"invalid targetfd", &valid_pidfd, -1, 0, EBADF},
	{"invalid flags", &valid_pidfd, 0, 1, EINVAL},
	{"the process referred to by pidfd doesn't exist", &expire_pidfd, 0, 0, ESRCH},
	{"lack of required permission", &valid_pidfd, 0, 0, EPERM},
};

static void setup(void)
{
	struct passwd *pw;

	pw = SAFE_GETPWNAM("nobody");
	uid = pw->pw_uid;

	pidfd_open_supported();
	pidfd_getfd_supported();

	TST_EXP_FD(pidfd_open(getpid(), -9));
	if (!TST_PASS)
		tst_brk(TFAIL | TTERRNO, "pidfd_open failed");
	valid_pidfd = TST_RET;
}

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];
	int pid;

	switch (tc->exp_errno) {
	case EPERM:
		pid = SAFE_FORK();
		if (!pid) {
			SAFE_SETUID(uid);
			TST_EXP_FAIL2(pidfd_getfd(valid_pidfd, tc->targetfd, tc->flags),
				tc->exp_errno, "pidfd_getfd(%d, %d, %d) with %s",
				valid_pidfd, tc->targetfd, tc->flags, tc->name);
			TST_CHECKPOINT_WAKE(0);
			exit(0);
		}
		TST_CHECKPOINT_WAIT(0);
		SAFE_WAIT(NULL);
		return;
	break;
	case ESRCH:
		pid = SAFE_FORK();
		if (!pid) {
			TST_CHECKPOINT_WAIT(0);
			exit(0);
		}
		TST_EXP_FD_SILENT(pidfd_open(pid, 0), "pidfd_open");
		*tc->pidfd = TST_RET;
		TST_CHECKPOINT_WAKE(0);
		SAFE_WAIT(NULL);
	break;
	default:
	break;
	};

	TST_EXP_FAIL2(pidfd_getfd(*tc->pidfd, tc->targetfd, tc->flags),
		tc->exp_errno, "pidfd_getfd(%d, %d, %d) with %s",
		*tc->pidfd, tc->targetfd, tc->flags, tc->name);
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcases),
	.test = run,
	.setup = setup,
	.needs_root = 1,
	.forks_child = 1,
	.needs_checkpoints = 1,
};
