// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2002
 * Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that it's possible to open a pseudo-terminal via /dev/ptmx, obtain a
 * slave device and to check if it's possible to open it multiple times.
 */

#define _GNU_SOURCE

#include "tst_test.h"

#define MASTERCLONE "/dev/ptmx"
#define NUM_SLAVES 10

static void run(void)
{
	int masterfd;
	int slavefd[NUM_SLAVES];
	char *slavename;

	masterfd = SAFE_OPEN(MASTERCLONE, O_RDWR);
	slavename = SAFE_PTSNAME(masterfd);

	TST_EXP_PASS(grantpt(masterfd));
	TST_EXP_PASS(unlockpt(masterfd));

	for (int i = 0; i < NUM_SLAVES; i++)
		slavefd[i] = TST_EXP_FD(open(slavename, O_RDWR));

	for (int i = 0; i < NUM_SLAVES; i++)
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
