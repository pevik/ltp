// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2002
 * Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that it's possible to open a pseudo-terminal via /dev/ptmx, to obtain
 * a master + slave pair and to open them multiple times.
 */

#define _GNU_SOURCE

#include "tst_test.h"

#define MASTERCLONE "/dev/ptmx"
#define NUM_OPENS 10

static void run(void)
{
	int masterfd[NUM_OPENS];
	int slavefd[NUM_OPENS];
	char *slavename;

	for (int i = 0; i < NUM_OPENS; i++) {
		masterfd[i] = SAFE_OPEN(MASTERCLONE, O_RDWR);

		slavename = ptsname(masterfd[i]);
		if (slavename == NULL)
			tst_res(TFAIL, "Can't get slave device location");
		else
			tst_res(TPASS, "pts device location is %s", slavename);

		TST_EXP_PASS(grantpt(masterfd[i]));
		TST_EXP_PASS(unlockpt(masterfd[i]));

		slavefd[i] = TST_EXP_FD(open(slavename, O_RDWR));
	}

	for (int i = 0; i < NUM_OPENS; i++) {
		SAFE_CLOSE(masterfd[i]);
		SAFE_CLOSE(slavefd[i]);
	}
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
