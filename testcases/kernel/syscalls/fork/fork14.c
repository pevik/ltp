// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2014  Red Hat, Inc.
 * Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * This test is a reporducer for this patch:
 * https://lore.kernel.org/lkml/1335289853-2923-1-git-send-email-siddhesh.poyarekar@gmail.com/
 * Since vma length in dup_mmap is calculated and stored in a unsigned
 * int, it will overflow when length of mmaped memory > 16 TB. When
 * overflow occurs, fork will incorrectly succeed. The patch above fixed it.
 */

#include "tst_test.h"
#include <stdlib.h>
#include <sys/wait.h>

#define LARGE		(16 * 1024)
#define EXTENT		(16 * 1024 + 10)

static char **memvec;

static void run(void)
{
	int i, j, ret;
	pid_t pid;
	void *mem;
	int prev_failed = 0;
	int passed = 1;
	int failures = 0;

	for (i = 0; i < EXTENT; i++) {
		mem = mmap(NULL, 1 * TST_GB,
			PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS,
			0, 0);

		if (mem == MAP_FAILED) {
			failures++;

			tst_res(TINFO, "mmap() failed");

			if (failures > 10) {
				tst_brk(TCONF, "mmap() fails too many "
					"times, so it's almost impossible to "
					"get a vm_area_struct sized 16TB.");
			}

			continue;
		}

		memvec[i] = mem;

		pid = fork();

		if (pid == -1) {
			/* keep track of the failed fork() and verify that next one
			 * is failing as well.
			 */
			prev_failed = 1;
			continue;
		}

		if (!pid)
			exit(0);

		ret = waitpid(pid, NULL, 0);
		if (ret == -1 && errno != ECHILD)
			tst_brk(TBROK | TERRNO, "waitpid() error");

		if (prev_failed && i >= LARGE) {
			passed = 0;
			break;
		}

		prev_failed = 0;

		tst_res(TINFO, "fork() passed at %d attempt", i);
	}

	for (j = 0; j < i; j++) {
		if (memvec[j])
			SAFE_MUNMAP(memvec[j], 1 * TST_GB);
	}

	if (passed)
		tst_res(TPASS, "fork() failed as expected");
	else
		tst_res(TFAIL, "fork() succeeded incorrectly");
}

static void setup(void)
{
	memvec = SAFE_MALLOC(EXTENT * sizeof(char *));
	memset(memvec, 0, EXTENT);
}

static void cleanup(void)
{
	for (long i = 0; i < EXTENT; i++) {
		if (memvec && memvec[i])
			SAFE_MUNMAP(memvec[i], 1 * TST_GB);
	}

	if (memvec)
		free(memvec);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.forks_child = 1,
	.skip_in_compat = 1,
	.tags = (const struct tst_tag[]) {
		{"linux-git", "7edc8b0ac16c"},
		{}
	}
};
