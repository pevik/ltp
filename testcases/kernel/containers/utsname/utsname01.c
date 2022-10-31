// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Clone two plain processes and check if both read the same hostname.
 */

#define _GNU_SOURCE

#include "tst_test.h"
#include "utsname.h"

#define HLEN 100

static char *hostname1;
static char *hostname2;

static int child1_run(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	SAFE_GETHOSTNAME(hostname1, HLEN);

	return 0;
}

static int child2_run(LTP_ATTRIBUTE_UNUSED void *vtest)
{
	SAFE_GETHOSTNAME(hostname2, HLEN);

	return 0;
}

static void run(void)
{
	int status1, status2;
	pid_t pid1, pid2;

	memset(hostname1, 0, HLEN);
	memset(hostname2, 0, HLEN);

	pid1 = clone_unshare_test(T_NONE, 0, child1_run, NULL);
	pid2 = clone_unshare_test(T_NONE, 0, child2_run, NULL);

	SAFE_WAITPID(pid1, &status1, 0);
	SAFE_WAITPID(pid2, &status2, 0);

	if (WIFSIGNALED(status1) || WIFSIGNALED(status2))
		return;

	TST_EXP_PASS(strcmp(hostname1, hostname2));
}

static void setup(void)
{
	hostname1 = SAFE_MMAP(NULL, sizeof(char) * HLEN, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	hostname2 = SAFE_MMAP(NULL, sizeof(char) * HLEN, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

static void cleanup(void)
{
	SAFE_MUNMAP(hostname1, HLEN);
	SAFE_MUNMAP(hostname2, HLEN);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.forks_child = 1,
};
