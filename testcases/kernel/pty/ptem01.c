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
 * slave device and configure termos/termios ioctls.
 */

#define _GNU_SOURCE

#include <termio.h>
#include <termios.h>
#include "tst_test.h"

#define MASTERCLONE "/dev/ptmx"

static void run(void)
{
	int masterfd, slavefd;
	char *slavename;
	struct termio termio;
	struct termios termios;

	masterfd = SAFE_OPEN(MASTERCLONE, O_RDWR);

	slavename = ptsname(masterfd);
	if (slavename == NULL)
		tst_res(TFAIL, "Can't get slave device location");
	else
		tst_res(TPASS, "pts device location is %s", slavename);

	TST_EXP_PASS(grantpt(masterfd));
	TST_EXP_PASS(unlockpt(masterfd));

	slavefd = SAFE_OPEN(slavename, O_RDWR);

	TST_EXP_POSITIVE(ioctl(slavefd, TCGETS, &termios));
	TST_EXP_POSITIVE(ioctl(slavefd, TCSETS, &termios));
	TST_EXP_POSITIVE(ioctl(slavefd, TCSETSW, &termios));
	TST_EXP_POSITIVE(ioctl(slavefd, TCSETSF, &termios));
	TST_EXP_POSITIVE(ioctl(slavefd, TCSETS, &termios));
	TST_EXP_POSITIVE(ioctl(slavefd, TCGETA, &termio));
	TST_EXP_POSITIVE(ioctl(slavefd, TCSETA, &termio));
	TST_EXP_POSITIVE(ioctl(slavefd, TCSETAW, &termio));
	TST_EXP_POSITIVE(ioctl(slavefd, TCSETAF, &termio));

	SAFE_CLOSE(slavefd);
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
