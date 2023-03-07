// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2012 Christian Brauner <brauner-AT-kernel.org>
 * Copyright (c) 2023 SUSE LLC <wegao@suse.com>
 */

/*\
 * [Description]
 *
 * Basic cgroup kill test.
 *
 */

#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include "lapi/syscalls.h"
#include "tst_test.h"

#define MAX_PID_NUM 100
#define pid_num MIN(MAX_PID_NUM, (tst_ncpus_available() + 1))
#define buf_len (20 * pid_num)

static char *buf;
static struct tst_cg_group *cg_child_test_simple;

static int wait_for_pid(pid_t pid)
{
	int status, ret;

again:
	ret = waitpid(pid, &status, 0);
	if (ret == -1) {
		if (errno == EINTR)
			goto again;

		return -1;
	}

	if (!WIFEXITED(status))
		return -1;

	return WEXITSTATUS(status);
}

/*
 * A simple process running in a sleep loop until being
 * re-parented.
 */
static void child_fn(void)
{
	int ppid = getppid();

	while (getppid() == ppid)
		usleep(1000);

}

static int cg_run_nowait(const struct tst_cg_group *const cg,
		  void (*fn)(void))
{
	int pid;

	pid = SAFE_FORK();
	if (pid == 0) {
		SAFE_CG_PRINTF(cg, "cgroup.procs", "%d", getpid());
		fn();
	}

	return pid;
}

static int cg_wait_for_proc_count(const struct tst_cg_group *cg, int count)
{
	int attempts;
	char *ptr;

	for (attempts = 100; attempts >= 0; attempts--) {
		int nr = 0;

		SAFE_CG_READ(cg, "cgroup.procs", buf, buf_len);

		for (ptr = buf; *ptr; ptr++)
			if (*ptr == '\n')
				nr++;

		if (nr >= count)
			return 0;

		usleep(100000);
	}

	return -1;
}

static void run(void)
{
	pid_t pids[MAX_PID_NUM];
	int i;

	cg_child_test_simple = tst_cg_group_mk(tst_cg, "cg_test_simple");

	memset(buf, 0, buf_len);

	for (i = 0; i < pid_num; i++)
		pids[i] = cg_run_nowait(cg_child_test_simple, child_fn);

	TST_EXP_PASS(cg_wait_for_proc_count(cg_child_test_simple, pid_num));
	SAFE_CG_PRINTF(cg_child_test_simple, "cgroup.kill", "%d", 1);

	for (i = 0; i < pid_num; i++) {
		TST_EXP_PASS_SILENT(wait_for_pid(pids[i]) == SIGKILL);
	}

	cg_child_test_simple = tst_cg_group_rm(cg_child_test_simple);
}

static void setup(void)
{
	buf = tst_alloc(buf_len);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.forks_child = 1,
	.max_runtime = 15,
	.needs_cgroup_ctrls = (const char *const []){ "memory", NULL },
	.needs_cgroup_ver = TST_CG_V2,
};
