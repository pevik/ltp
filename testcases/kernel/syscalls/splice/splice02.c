// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Jens Axboe <axboe@kernel.dk>, 2009
 * Copyright (c) 2021 Petr Vorel <pvorel@suse.cz>
 */

/*\
 * [DESCRIPTION]
 * Original reproducer for kernel fix
 * bf40d3435caf NFS: add support for splice writes
 * from v2.6.31-rc1.
 *
 * http://lkml.org/lkml/2009/4/2/55
 *
 * [ALGORITHM]
 * - create pipe
 * - fork(), child replace stdin with pipe
 * - parent write to pipe
 * - child slice from pipe
\*/

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "tst_test.h"
#include "lapi/splice.h"

#define SPLICE_SIZE (64*1024)
#define DEFAULT_NARG 20000

static char *narg;
static int num = DEFAULT_NARG;
static int pipe_fd[2];

static void setup(void)
{
	if (tst_parse_int(narg, &num, 1, INT_MAX))
		tst_brk(TBROK, "invalid number of input '%s'", narg);
}

static void do_child(void)
{
	int fd;

	SAFE_CLOSE(pipe_fd[1]);
	close(STDIN_FILENO);
	SAFE_DUP2(pipe_fd[0], STDIN_FILENO);

	TST_CHECKPOINT_WAIT(0);
	fd = SAFE_OPEN("splice02-temp", O_WRONLY | O_CREAT | O_TRUNC, 0644);

	TEST(splice(STDIN_FILENO, NULL, fd, NULL, SPLICE_SIZE, 0));
	if (TST_RET < 0) {
		tst_res(TFAIL, "splice failed - errno = %d : %s",
			TST_ERR, strerror(TST_ERR));
	} else {
		tst_res(TPASS, "splice() system call Passed");
	}

	SAFE_CLOSE(fd);

	exit(0);
}

static void run(void)
{
	int i;

	SAFE_PIPE(pipe_fd);

	if (SAFE_FORK())
		do_child();

	tst_res(TINFO, "writting %d times", num);

	for (i = 0; i < num; i++)
		SAFE_WRITE(1, pipe_fd[1], "x", 1);

	tst_res(TINFO, "F_GETPIPE_SZ: %d", fcntl(pipe_fd[1], F_GETPIPE_SZ)); // FIXME: debug

	TST_CHECKPOINT_WAKE(0);
	tst_reap_children();

	SAFE_CLOSE(pipe_fd[0]);
	SAFE_CLOSE(pipe_fd[1]);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_checkpoints = 1,
	.needs_tmpdir = 1,
	.forks_child = 1,
	.min_kver = "2.6.17",
	.options = (struct tst_option[]) {
		{"n:", &narg, "-n x     Number of input"},
		{}
	},
};
