// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone two processes using CLONE_NEWUTS, change hostname from the first
 * container and check if hostname didn't change inside the second one.
 */

#define _GNU_SOURCE

#include "tst_test.h"
#include "utsname.h"

#define HLEN 100
#define HOSTNAME "LTP_HOSTNAME"

static char *str_op = "clone";
static int use_clone;
static char *hostname1;
static char *hostname2;
static char originalhost[HLEN];

static void reset_hostname(void)
{
	SAFE_SETHOSTNAME(originalhost, strlen(originalhost));
}

static int child1_run(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	SAFE_SETHOSTNAME(HOSTNAME, strlen(HOSTNAME));
	SAFE_GETHOSTNAME(hostname1, HLEN);

	TST_CHECKPOINT_WAKE(0);

	return 0;
}

static int child2_run(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	TST_CHECKPOINT_WAIT(0);

	SAFE_GETHOSTNAME(hostname2, HLEN);

	return 0;
}

static void run(void)
{
	pid_t pid1, pid2;
	int status1, status2;

	memset(hostname1, 0, HLEN);
	memset(hostname2, 0, HLEN);

	pid1 = clone_unshare_test(use_clone, CLONE_NEWUTS, child1_run, NULL);
	pid2 = clone_unshare_test(use_clone, CLONE_NEWUTS, child2_run, NULL);

	SAFE_WAITPID(pid1, &status1, 0);
	SAFE_WAITPID(pid2, &status2, 0);

	if (WIFSIGNALED(status1) || WIFSIGNALED(status2))
		return;

	TST_EXP_PASS(strcmp(hostname1, HOSTNAME));
	TST_EXP_PASS(strcmp(hostname2, originalhost));

	reset_hostname();
}

static void setup(void)
{
	use_clone = get_clone_unshare_enum(str_op);

	if (use_clone != T_CLONE && use_clone != T_UNSHARE)
		tst_brk(TCONF, "Only clone and unshare clone are supported");

	check_newuts();

	hostname1 = SAFE_MMAP(NULL, sizeof(char) * HLEN, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	hostname2 = SAFE_MMAP(NULL, sizeof(char) * HLEN, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	memset(originalhost, 0, HLEN);

	SAFE_GETHOSTNAME(originalhost, HLEN);
}

static void cleanup(void)
{
	SAFE_MUNMAP(hostname1, HLEN);
	SAFE_MUNMAP(hostname2, HLEN);

	reset_hostname();
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.forks_child = 1,
	.needs_checkpoints = 1,
	.options = (struct tst_option[]) {
		{ "m:", &str_op, "Test execution mode <clone|unshare>" },
		{},
	},
};
