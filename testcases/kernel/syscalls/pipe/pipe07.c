// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2002
 *  Ported by Paul Larson
 * Copyright (c) 2013 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (c) 2023 SUSE LLC Avinesh Kumar <avinesh.kumar@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that, pipe(2) syscall can open the maximum number of
 * file descriptors permitted.
 */

#include "tst_test.h"
#include <stdlib.h>

static int *opened_fds, *pipe_fds;
static int num_opened_fds, num_pipe_fds, exp_num_pipe_fds;

static void record_open_fds(void)
{
	DIR *dir;
	struct dirent *ent;
	int fd;

	dir = SAFE_OPENDIR("/proc/self/fd");

	while ((ent = SAFE_READDIR(dir))) {
		if (!strcmp(ent->d_name, ".") ||
			!strcmp(ent->d_name, ".."))
			continue;
		fd = atoi(ent->d_name);
		opened_fds = SAFE_REALLOC(opened_fds, (num_opened_fds + 1) * sizeof(int));
		opened_fds[num_opened_fds++] = fd;
	}
}

static void setup(void)
{
	int max_fds;

	max_fds = getdtablesize();
	tst_res(TINFO, "getdtablesize() = %d", max_fds);
	pipe_fds = SAFE_MALLOC(max_fds * sizeof(int));

	record_open_fds();
	tst_res(TINFO, "open fds before pipe() calls: %d", num_opened_fds);

	exp_num_pipe_fds = max_fds - num_opened_fds;
	exp_num_pipe_fds = (exp_num_pipe_fds % 2) ?
						(exp_num_pipe_fds - 1) : exp_num_pipe_fds;
	tst_res(TINFO, "expected max fds to be opened by pipe(): %d", exp_num_pipe_fds);
}

static void run(void)
{
	int fds[2];

	do {
		TEST(pipe(fds));
		if (TST_RET != -1) {
			pipe_fds[num_pipe_fds++] = fds[0];
			pipe_fds[num_pipe_fds++] = fds[1];
		}
	} while (TST_RET != -1);

	TST_EXP_EQ_LI(errno, EMFILE);
	TST_EXP_EQ_LI(exp_num_pipe_fds, num_pipe_fds);

	for (int i = 0; i < num_pipe_fds; i++)
		SAFE_CLOSE(pipe_fds[i]);

	num_pipe_fds = 0;
}

static void cleanup(void)
{
	if (opened_fds)
		free(opened_fds);

	if (pipe_fds)
		free(pipe_fds);

	for (int i = 0; i < num_pipe_fds; i++)
		if (pipe_fds[i])
			SAFE_CLOSE(pipe_fds[i]);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run
};
