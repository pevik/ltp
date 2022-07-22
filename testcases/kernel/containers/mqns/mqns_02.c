// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) International Business Machines Corp., 2009
 * Copyright (c) Nadia Derbey, 2009 <Nadia.Derbey@bull.net>
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Create a mqueue with the same name in both parent and isolated/forked child,
 * then check namespace isolation.
 */

#include "common.h"

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

		SAFE_MQ_CLOSE(mqd1);
		SAFE_MQ_UNLINK(MQNAME);
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

	mqd = SAFE_MQ_OPEN(MQNAME, O_RDWR | O_CREAT | O_EXCL, 0777, NULL);
}

static void cleanup(void)
{
	if (mqd != -1) {
		SAFE_MQ_CLOSE(mqd);
		SAFE_MQ_UNLINK(MQNAME);
	}
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.forks_child = 1,
	.options = (struct tst_option[]) {
		{ "m:", &str_op, "Test execution mode <clone|unshare>" },
		{},
	},
};
