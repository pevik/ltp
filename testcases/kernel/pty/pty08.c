// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2002
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that slave pseudo-terminal fails reading/writing if master has been
 * closed.
 */

#define _GNU_SOURCE

#include "tst_test.h"

#define MASTERCLONE "/dev/ptmx"

static void run(void)
{
	int masterfd, slavefd;
	char *slavename;
	char buf;

	masterfd = SAFE_OPEN(MASTERCLONE, O_RDWR);
	slavename = SAFE_PTSNAME(masterfd);

	tst_res(TINFO, "pts device location is %s", slavename);

	if (grantpt(masterfd) == -1)
		tst_brk(TBROK | TERRNO, "grantpt() error");

	if (unlockpt(masterfd) == -1)
		tst_brk(TBROK | TERRNO, "unlockpt() error");

	slavefd = SAFE_OPEN(slavename, O_RDWR);

	tst_res(TINFO, "Closing master communication");
	SAFE_CLOSE(masterfd);

	TST_EXP_FAIL(read(slavefd, &buf, 1), EIO);
	TST_EXP_FAIL(write(slavefd, &buf, 1), EIO);

	SAFE_CLOSE(slavefd);
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
