// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) International Business Machines Corp., 2009
 * Copyright (c) Serge Hallyn <serue@us.ibm.com>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Test mqueuefs from an isolated/forked process namespace.
 *
 * [Algorithm]
 *
 * - create /MQ1
 * - mount mqueue inside the temporary folder
 * - check for /MQ1 existance
 * - create /MQ2 inside the temporary folder
 * - umount
 * - mount mqueue inside the temporary folder
 * - check /MQ1 existance
 * - check /MQ2 existance
 * - umount
 */
#define _GNU_SOURCE

#include <stdio.h>
#include "common.h"
#include "tst_safe_stdio.h"

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
	int rc;
	mqd_t mqd;
	struct stat statbuf;

	mqd = SAFE_MQ_OPEN(MQNAME1, O_RDWR | O_CREAT | O_EXCL, 0777, NULL);

	SAFE_MQ_CLOSE(mqd);

	SAFE_MOUNT("mqueue", devdir, "mqueue", 0, NULL);

	SAFE_STAT(mqueue1, &statbuf);
	tst_res(TPASS, "%s exists at first mount", mqueue1);

	rc = SAFE_CREAT(mqueue2, 0755);
	SAFE_CLOSE(rc);

	SAFE_UMOUNT(devdir);

	SAFE_MOUNT("mqueue", devdir, "mqueue", 0, NULL);

	SAFE_STAT(mqueue1, &statbuf);
	tst_res(TPASS, "%s exists at second mount", mqueue1);

	SAFE_STAT(mqueue2, &statbuf);
	tst_res(TPASS, "%s exists at second mount", mqueue2);

	SAFE_UMOUNT(devdir);

	SAFE_MQ_UNLINK(MQNAME1);
	SAFE_MQ_UNLINK(MQNAME2);

	*mq_freed = 1;

	return 0;
}

static void run(void)
{
	tst_res(TINFO, "Checking correct umount+remount of mqueuefs");

	clone_unshare_test(use_clone, CLONE_NEWIPC, check_mqueue, NULL);
}

static void setup(void)
{
	char *tmpdir;

	use_clone = get_clone_unshare_enum(str_op);

	if (use_clone != T_NONE)
		check_newipc();

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
	.options = (struct tst_option[]) {
		{ "m:", &str_op, "Test execution mode <clone|unshare>" },
		{},
	},
};
