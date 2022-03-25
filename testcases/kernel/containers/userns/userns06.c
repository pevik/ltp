// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Huawei Technologies Co., Ltd., 2015
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that when a process with non-zero user IDs performs an execve(),
 * the process's capability sets are cleared.
 * When a process with zero user IDs performs an execve(), the process's
 * capability sets are set.
 */

#include "tst_test.h"
#include "config.h"

#ifdef HAVE_LIBCAP
#define _GNU_SOURCE

#include <stdio.h>
#include "common.h"

#define TEST_APP "userns06_capcheck"

#define CHILD1UID 0
#define CHILD1GID 0
#define CHILD2UID 200
#define CHILD2GID 200

/*
 * child_fn1() - Inside a new user namespace
 */
static int child_fn1(void)
{
	char *const args[] = { TEST_APP, "privileged", NULL };

	TST_CHECKPOINT_WAIT(0);

	/* execv will replace the main function and it will end this child
	 * accordingly.
	 */
	execv(args[0], args);

	return 0;
}

/*
 * child_fn2() - Inside a new user namespace
 */
static int child_fn2(void)
{
	int uid, gid;
	char *const args[] = { TEST_APP, "unprivileged", NULL };

	TST_CHECKPOINT_WAIT(1);

	uid = geteuid();
	gid = getegid();

	if (uid != CHILD2UID || gid != CHILD2GID) {
		tst_res(TFAIL, "unexpected uid=%d gid=%d", uid, gid);
		return 1;
	}

	tst_res(TPASS, "expected uid and gid");

	/* execv will replace the main function and it will end this child
	 * accordingly.
	 */
	execv(args[0], args);

	return 0;
}

static void setup(void)
{
	check_newuser();
}

static void run(void)
{
	pid_t cpid1;
	pid_t cpid2;
	int parentuid;
	int parentgid;
	char path[BUFSIZ];
	int fd;

	parentuid = geteuid();
	parentgid = getegid();

	cpid1 = ltp_clone_quick(CLONE_NEWUSER | SIGCHLD, (void *)child_fn1, NULL);
	if (cpid1 < 0)
		tst_brk(TBROK | TTERRNO, "cpid1 clone failed");

	cpid2 = ltp_clone_quick(CLONE_NEWUSER | SIGCHLD, (void *)child_fn2, NULL);
	if (cpid2 < 0)
		tst_brk(TBROK | TTERRNO, "cpid2 clone failed");

	if (access("/proc/self/setgroups", F_OK) == 0) {
		sprintf(path, "/proc/%d/setgroups", cpid1);

		fd = SAFE_OPEN(path, O_WRONLY, 0644);
		SAFE_WRITE(1, fd, "deny", 4);
		SAFE_CLOSE(fd);

		sprintf(path, "/proc/%d/setgroups", cpid2);

		fd = SAFE_OPEN(path, O_WRONLY, 0644);
		SAFE_WRITE(1, fd, "deny", 4);
		SAFE_CLOSE(fd);
	}

	updatemap(cpid1, UID_MAP, CHILD1UID, parentuid);
	updatemap(cpid2, UID_MAP, CHILD2UID, parentuid);

	updatemap(cpid1, GID_MAP, CHILD1GID, parentgid);
	updatemap(cpid2, GID_MAP, CHILD2GID, parentgid);

	TST_CHECKPOINT_WAKE(0);
	TST_CHECKPOINT_WAKE(1);
}

static const char *const resource_files[] = {
	TEST_APP,
	NULL,
};

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.needs_root = 1,
	.needs_checkpoints = 1,
	.resource_files = resource_files,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_USER_NS",
		NULL,
	},
};

#else
TST_TEST_TCONF("System is missing libcap");
#endif
