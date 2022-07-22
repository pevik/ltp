// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) International Business Machines Corp., 2009
 * Copyright (c) Serge Hallyn <serue@us.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Test mqueuefs manipulation from child/parent namespaces.
 *
 * [Algorithm]
 *
 * - parent creates mqueue folder in <tmpdir>
 * - child mounts mqueue there
 * - child creates /MQ1 mqueue
 * - parent checks for <tmpdir>/mqueue/MQ1 existence
 * - child exits
 * - parent checks for <tmpdir>/mqueue/MQ1 existence
 * - parent tries 'touch <tmpdir>/mqueue/MQ2' -> should fail
 * - parent umount mqueuefs
 */
#define _GNU_SOURCE

#include <stdio.h>
#include "common.h"
#include "tst_safe_stdio.h"

#define CHECK_MQ_OPEN_RET(x) ((x) >= 0 || ((x) == -1 && errno != EMFILE))

#define MQNAME1 "/MQ1"
#define MQNAME2 "/MQ2"

static char *str_op;
static int use_clone;
static char *devdir;
static char *mqueue1;
static char *mqueue2;
static int *mq_freed;

static int check_mqueue(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	mqd_t mqd;

	mqd = TST_RETRY_FUNC(
		mq_open(MQNAME1, O_RDWR | O_CREAT | O_EXCL, 0755, NULL),
		CHECK_MQ_OPEN_RET);
	if (mqd == -1)
		tst_brk(TBROK | TERRNO, "mq_open failed");

	SAFE_MQ_CLOSE(mqd);

	SAFE_MOUNT("mqueue", devdir, "mqueue", 0, NULL);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	return 0;
}

static void run(void)
{
	int rc;
	int status;
	struct stat statbuf;

	tst_res(TINFO, "Checking mqueue filesystem lifetime");

	clone_unshare_test(use_clone, CLONE_NEWIPC, check_mqueue, NULL);

	TST_CHECKPOINT_WAIT(0);

	SAFE_STAT(mqueue1, &statbuf);
	tst_res(TPASS, "%s child's mqueue can be accessed from parent", mqueue1);

	TST_CHECKPOINT_WAKE(0);

	tst_res(TINFO, "Waiting child to exit");

	SAFE_WAIT(&status);

	if (!WIFEXITED(status))
		tst_brk(TBROK, "Child did not exit normally: %s", tst_strstatus(status));

	SAFE_STAT(mqueue1, &statbuf);
	tst_res(TPASS, "%s child's mqueue can be accessed from parent after child's dead", mqueue1);

	rc = creat(mqueue2, 0755);
	if (rc != -1)
		tst_res(TFAIL, "Parent was able to create a file in dead child's mqueuefs");
	else
		tst_res(TPASS, "Parent is not able to create a file in dead child's mqueuefs");

	SAFE_UMOUNT(devdir);

	*mq_freed = 1;
}

static void setup(void)
{
	char *tmpdir;

	use_clone = get_clone_unshare_enum(str_op);

	if (use_clone == T_NONE)
		tst_brk(TBROK, "Plain process is not supported by this test");

	tmpdir = tst_get_tmpdir();

	SAFE_ASPRINTF(&devdir, "%s/mqueue", tmpdir);
	SAFE_MKDIR(devdir, 0755);

	SAFE_ASPRINTF(&mqueue1, "%s" MQNAME1, devdir);
	SAFE_ASPRINTF(&mqueue2, "%s" MQNAME2, devdir);

	mq_freed = SAFE_MMAP(NULL,
		sizeof(int),
		PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS,
		-1, 0);
}

static void cleanup(void)
{
	if (tst_is_mounted(devdir))
		SAFE_UMOUNT(devdir);

	if (!*mq_freed) {
		SAFE_MQ_UNLINK(MQNAME1);
		SAFE_MQ_UNLINK(MQNAME2);
	}
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.forks_child = 1,
	.needs_tmpdir = 1,
	.needs_checkpoints = 1,
	.options = (struct tst_option[]) {
		{ "m:", &str_op, "Test execution mode <clone|unshare>" },
		{},
	},
};
