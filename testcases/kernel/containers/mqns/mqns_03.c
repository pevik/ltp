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
#include <mqueue.h>
#include "tst_safe_stdio.h"
#include "common.h"

#define MQNAME1 "/MQ1"
#define MQNAME2 "/MQ2"

static char *str_op;
static int use_clone;
static char *devdir;
static char *mqueue1;
static char *mqueue2;

static int check_mqueue(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	int rc;
	mqd_t mqd;
	struct stat statbuf;

	mqd = mq_open(MQNAME1, O_RDWR | O_CREAT | O_EXCL, 0777, NULL);
	if (mqd < 0)
		tst_brk(TBROK | TERRNO, "mq_open failed");

	mq_close(mqd);

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

	mq_unlink(MQNAME1);
	mq_unlink(MQNAME2);

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
}

static void cleanup(void)
{
	if (tst_is_mounted(devdir))
		SAFE_UMOUNT(devdir);

	/* if test ends too early, we need to unlink mqueue(s) */
	mq_unlink(MQNAME1);
	mq_unlink(MQNAME2);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.forks_child = 1,
	.needs_tmpdir = 1,
	.min_kver = "2.6.30",
	.options = (struct tst_option[]) {
		{ "m:", &str_op, "Test execution mode <clone|unshare|none>" },
		{},
	},
};
