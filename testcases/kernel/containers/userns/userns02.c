// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Huawei Technologies Co., Ltd., 2015
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that the user ID and group ID, which are inside a container,
 * can be modified by its parent process.
 */

#define _GNU_SOURCE

#include <stdio.h>
#include "common.h"
#include "tst_test.h"

/*
 * child_fn1() - Inside a new user namespace
 */
static int child_fn1(void)
{
	int uid, gid;

	TST_CHECKPOINT_WAIT(0);

	uid = geteuid();
	gid = getegid();

	if (uid == 100 && gid == 100)
		tst_res(TPASS, "got expected uid and gid");
	else
		tst_res(TFAIL, "got unexpected uid=%d gid=%d", uid, gid);

	return 0;
}

static void setup(void)
{
	check_newuser();
}

static void run(void)
{
	int childpid;
	int parentuid;
	int parentgid;
	char path[BUFSIZ];
	char content[BUFSIZ];
	int fd;

	childpid = ltp_clone_quick(CLONE_NEWUSER | SIGCHLD, (void *)child_fn1, NULL);
	if (childpid < 0)
		tst_brk(TBROK | TTERRNO, "clone failed");

	parentuid = geteuid();
	parentgid = getegid();

	sprintf(path, "/proc/%d/uid_map", childpid);
	sprintf(content, "100 %d 1", parentuid);

	fd = SAFE_OPEN(path, O_WRONLY, 0644);
	SAFE_WRITE(1, fd, content, strlen(content));
	SAFE_CLOSE(fd);

	if (access("/proc/self/setgroups", F_OK) == 0) {
		sprintf(path, "/proc/%d/setgroups", childpid);

		fd = SAFE_OPEN(path, O_WRONLY, 0644);
		SAFE_WRITE(1, fd, "deny", 4);
		SAFE_CLOSE(fd);
	}

	sprintf(path, "/proc/%d/gid_map", childpid);
	sprintf(content, "100 %d 1", parentgid);

	fd = SAFE_OPEN(path, O_WRONLY, 0644);
	SAFE_WRITE(1, fd, content, strlen(content));
	SAFE_CLOSE(fd);

	TST_CHECKPOINT_WAKE(0);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.needs_root = 1,
	.needs_checkpoints = 1,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_USER_NS",
		NULL,
	},
};
