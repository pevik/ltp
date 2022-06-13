// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) International Business Machines Corp., 2009
 * Copyright (c) Nadia Derbey, 2009 <Nadia.Derbey@bull.net>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Create a mqueue with the same name in both father and isolated/forked child,
 * then check namespace isolation.
 */

#include <mqueue.h>
#include "mqns.h"

#define MQNAME "/MQ1"

static mqd_t mqd;
static char *str_op;
static int use_clone;

static int check_mqueue(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	mqd_t mqd1;

	mqd1 = mq_open(MQNAME, O_RDWR | O_CREAT | O_EXCL, 0777, NULL);

	if (use_clone == T_NONE) {
		if (mqd1 == -1)
			tst_res(TPASS, "Can't create queue from plain cloned process");
		else
			tst_res(TFAIL, "Queue has been created form plain cloned process");

		return 0;
	}

	if (mqd1 == -1) {
		tst_res(TFAIL, "Queue hasn't been created from isolated process");
	} else {
		tst_res(TPASS, "Created queue from isolated process");

		mq_close(mqd1);
		mq_unlink(MQNAME);
	}

	return 0;
}

static void run(void)
{
	tst_res(TINFO, "Checking namespaces isolation from parent to child");

	clone_unshare_test(use_clone, CLONE_NEWIPC, check_mqueue, NULL);
}

static void setup(void)
{
	use_clone = get_clone_unshare_enum(str_op);

	if (use_clone != T_NONE)
		check_newipc();

	mqd = mq_open(MQNAME, O_RDWR | O_CREAT | O_EXCL, 0777, NULL);
	if (mqd == -1)
		tst_brk(TBROK | TERRNO, "mq_open failed");
}

static void cleanup(void)
{
	if (mqd != -1) {
		mq_close(mqd);
		mq_unlink(MQNAME);
	}
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.forks_child = 1,
	.min_kver = "2.6.30",
	.options = (struct tst_option[]) {
		{ "m:", &str_op, "Test execution mode <clone|unshare|none>" },
		{},
	},
};
