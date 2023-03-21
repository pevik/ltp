// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright 2023 Mike Galbraith <efault-AT-gmx.de> */
/*\
 *
 * [Description]
 *
 * Thread starvation test.
 * This case copy from following link:
 * https://lore.kernel.org/lkml/9fd2c37a05713c206dcbd5866f67ce779f315e9e.camel@gmx.de/
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <sched.h>

#include "tst_test.h"

static unsigned long loop = 10000000;

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

	if (WIFSIGNALED(status))
		return 0;

	return -1;
}

static void setup(void)
{
	cpu_set_t mask;

	CPU_ZERO(&mask);

	CPU_SET(0, &mask);

	TST_EXP_POSITIVE(sched_setaffinity(0, sizeof(mask), &mask));
}

static void handler(void)
{
	if (loop > 0)
		--loop;
}

static int child(void)
{
	pid_t ppid = getppid();

	TST_CHECKPOINT_WAIT(0);

	while (1)
		kill(ppid, SIGUSR1);
}

static void do_test(void)
{
	pid_t child_pid;

	child_pid = SAFE_FORK();

	if (!child_pid)
		child();

	SAFE_SIGNAL(SIGUSR1, handler);
	TST_CHECKPOINT_WAKE(0);

	while (loop)
		sleep(1);

	kill(child_pid, SIGTERM);
	TST_EXP_PASS(wait_for_pid(child_pid));
}

static struct tst_test test = {
	.test_all = do_test,
	.setup = setup,
	.forks_child = 1,
	.needs_checkpoints = 1,
	.max_runtime = 120,
};
