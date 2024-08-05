// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 FUJITSU LIMITED. All Rights Reserved.
 * Copyright (c) Linux Test Project, 2024
 * Author: Ma Xinjian <maxj.fnst@fujitsu.com>
 *
 */

/*\
 * [Description]
 *
 * Verify that getcpu(2) fails with
 *
 * - EFAULT arguments point outside the calling process's address
 *          space.
 */

#define _GNU_SOURCE
#include <errno.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <stdlib.h>

#include "tst_test.h"
#include "lapi/sched.h"

static void *bad_addr;

static void setup(void)
{
	bad_addr = tst_get_bad_addr(NULL);
}

static void check_bad_cpu_id(void *bad_addr, unsigned int *node_id)
{
	int status;
	pid_t pid;

	pid = SAFE_FORK();
	if (!pid) {
		TST_EXP_FAIL(getcpu(bad_addr, node_id), EFAULT);

		exit(0);
	}

	SAFE_WAITPID(pid, &status, 0);

	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV) {
		tst_res(TPASS, "getcpu() caused SIGSEGV");
		return;
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return;

	tst_res(TFAIL, "child %s", tst_strstatus(status));
}

static void check_bad_node_id(unsigned int *cpu_id, void *bad_addr)
{
	int status;
	pid_t pid;

	pid = SAFE_FORK();
	if (!pid) {
		TST_EXP_FAIL(getcpu(cpu_id, bad_addr), EFAULT);

		exit(0);
	}

	SAFE_WAITPID(pid, &status, 0);

	if (WIFSIGNALED(status) && WTERMSIG(status) == SIGSEGV) {
		tst_res(TPASS, "getcpu() caused SIGSEGV");
		return;
	}

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		return;

	tst_res(TFAIL, "child %s", tst_strstatus(status));
}

static void run_test(void)
{
	unsigned int cpu_id, node_id = 0;

	check_bad_cpu_id(bad_addr, &node_id);
	check_bad_node_id(&cpu_id, bad_addr);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run_test,
	.forks_child = 1,
};
