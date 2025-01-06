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
 * a slave device and to set baudrate to B0 (which means hang up).
 */

#define _GNU_SOURCE

#include <termios.h>
#include "tst_test.h"

#define MASTERCLONE "/dev/ptmx"

static void run(void)
{
	int masterfd, slavefd;
	struct termios termios;
	char *slavename;

	masterfd = SAFE_OPEN(MASTERCLONE, O_RDWR);
	slavename = SAFE_PTSNAME(masterfd);

	TST_EXP_PASS(grantpt(masterfd));
	TST_EXP_PASS(unlockpt(masterfd));

	slavefd = SAFE_OPEN(slavename, O_RDWR);

	TST_EXP_PASS(ioctl(slavefd, TCGETS, &termios));
	termios.c_cflag &= ~CBAUD;
	termios.c_cflag |= B0 & CBAUD;
	TST_EXP_PASS(ioctl(slavefd, TCSETS, &termios));

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
