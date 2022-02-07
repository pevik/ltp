// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2014 Red Hat, Inc.
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Tests a slave mount: slave mount is like a shared mount except that
 * mount and umount events only propagate towards it.
 *
 * [Algorithm]
 *
 * 1. Creates directories "A", "B" and files "A/A", "B/B"
 * 2. Unshares mount namespace and makes it private (so mounts/umounts
 *    have no effect on a real system)
 * 3. Bind mounts directory "A" to itself
 * 4. Makes directory "A" shared
 * 5. Clones a new child process with CLONE_NEWNS flag and makes "A"
 *    a slave mount
 * 6. There are two testcases (where X is parent namespace and Y child
 *    namespace):
 *    1)
 *	X: bind mounts "B" to "A"
 *	Y: must see the file "A/B"
 *	X: umounts "A"
 *    2)
 *	Y: bind mounts "B" to "A"
 *	X: must see only the "A/A" and must not see "A/B" (as slave
 *	   mount does not forward propagation)
 *	Y: umounts "A"
 */

#define _GNU_SOURCE

#include <sys/wait.h>
#include <sys/mount.h>
#include "common.h"
#include "tst_test.h"

#if defined(MS_SHARED) && defined(MS_PRIVATE) && defined(MS_REC)               \
	&& defined(MS_SLAVE)

static int child_func(LTP_ATTRIBUTE_UNUSED void *arg)
{
	int ret = 0;

	/*
	 * makes mount DIRA a slave of DIRA (all slave mounts have
	 * a master mount which is a shared mount)
	 */
	SAFE_MOUNT("none", DIRA, "none", MS_SLAVE, NULL);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	/* checks that shared mounts propagates to slave mount */
	if (access(DIRA "/B", F_OK) < 0)
		ret = 2;

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	/* bind mounts DIRB to DIRA making contents of DIRB visible in DIRA */
	SAFE_MOUNT(DIRB, DIRA, "none", MS_BIND, NULL);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	SAFE_UMOUNT(DIRA);

	return ret;
}

static void run(void)
{
	int status, ret;

	/* unshares the mount ns */
	SAFE_UNSHARE(CLONE_NEWNS);

	/* makes sure parent mounts/umounts have no effect on a real system */
	SAFE_MOUNT("none", "/", "none", MS_REC | MS_PRIVATE, NULL);

	/* bind mounts DIRA to itself */
	SAFE_MOUNT(DIRA, DIRA, "none", MS_BIND, NULL);

	/* makes mount DIRA shared */
	SAFE_MOUNT("none", DIRA, "none", MS_SHARED, NULL);

	ret = ltp_clone_quick(CLONE_NEWNS | SIGCHLD, child_func, NULL);
	if (ret < 0)
		tst_brk(TBROK, "clone failed");

	/* waits for child to make a slave mount */
	TST_CHECKPOINT_WAIT(0);

	/* bind mounts DIRB to DIRA making contents of DIRB visible in DIRA */
	SAFE_MOUNT(DIRB, DIRA, "none", MS_BIND, NULL);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	SAFE_UMOUNT(DIRA);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	/* checks that slave mount doesn't propagate to shared mount */
	if ((access(DIRA "/A", F_OK) == 0) && (access(DIRA "/B", F_OK) == -1))
		tst_res(TPASS, "propagation from slave mount passed");
	else
		tst_res(TFAIL, "propagation form slave mount failed");

	TST_CHECKPOINT_WAKE(0);

	SAFE_WAIT(&status);

	if (WIFEXITED(status)) {
		if (WEXITSTATUS(status) == 0)
			tst_res(TPASS, "propagation to slave mount passed");
		else
			tst_res(TFAIL, "propagation to slave mount failed");
	}

	if (WIFSIGNALED(status)) {
		tst_brk(TBROK, "child was killed with signal %s",
			tst_strsig(WTERMSIG(status)));
	}

	SAFE_UMOUNT(DIRA);
}

static void setup(void)
{
	check_newns();
	create_folders();
}

static void cleanup(void)
{
	umount_folders();
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.needs_root = 1,
	.needs_checkpoints = 1,
};

#else
TST_TEST_TCONF("needed mountflags are not defined");
#endif
