// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2002
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that slave pseudo-terminal can be opened multiple times in parallel.
 */

#define _GNU_SOURCE

#include "tst_test.h"

#define MASTERCLONE "/dev/ptmx"

static unsigned int count_avail_pid(void)
{
	DIR *dir;
	struct dirent *ent;
	struct rlimit limit;
	unsigned int count = 0;
	unsigned int max_pid_num;

	if (access(MASTERCLONE, F_OK))
		tst_brk(TBROK, "%s device doesn't exist", MASTERCLONE);

	SAFE_GETRLIMIT(RLIMIT_NOFILE, &limit);

	dir = SAFE_OPENDIR("/proc/self/fd");
	while ((ent = SAFE_READDIR(dir)))
		count++;

	SAFE_CLOSEDIR(dir);

	max_pid_num = limit.rlim_cur - count;

	tst_res(TINFO, "Available number of pids: %u", max_pid_num);

	return max_pid_num;
}

static void run(void)
{
	int masterfd;
	char *slavename;
	unsigned int max_pid_num;

	masterfd = SAFE_OPEN(MASTERCLONE, O_RDWR);

	slavename = ptsname(masterfd);
	if (slavename == NULL)
		tst_brk(TBROK | TERRNO, "ptsname() error");

	tst_res(TINFO, "pts device location is %s", slavename);

	if (grantpt(masterfd) == -1)
		tst_brk(TBROK | TERRNO, "grantpt() error");

	if (unlockpt(masterfd) == -1)
		tst_brk(TBROK | TERRNO, "unlockpt() error");

	max_pid_num = count_avail_pid();

	int slavefd[max_pid_num];

	for (uint32_t i = 0; i < max_pid_num; i++)
		slavefd[i] = SAFE_OPEN(slavename, O_RDWR);

	tst_res(TPASS, "%s has been opened %d times", slavename, max_pid_num);

	for (uint32_t i = 0; i < max_pid_num; i++)
		SAFE_CLOSE(slavefd[i]);

	SAFE_CLOSE(masterfd);
}

static void setup(void)
{
	if (access(MASTERCLONE, F_OK))
		tst_brk(TBROK, "%s device doesn't exist", MASTERCLONE);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
};
