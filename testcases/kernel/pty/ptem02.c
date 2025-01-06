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
 * slave device and set/get window size.
 */

#define _GNU_SOURCE

#include "tst_test.h"

#define MASTERCLONE "/dev/ptmx"

static void run(void)
{
	int masterfd, slavefd;
	char *slavename;
	struct winsize wsz;
	struct winsize wsz1 = { 24, 80, 5, 10 };
	struct winsize wsz2 = { 60, 100, 11, 777 };

	masterfd = SAFE_OPEN(MASTERCLONE, O_RDWR);
	slavename = SAFE_PTSNAME(masterfd);

	TST_EXP_PASS(grantpt(masterfd));
	TST_EXP_PASS(unlockpt(masterfd));

	slavefd = SAFE_OPEN(slavename, O_RDWR);

	TST_EXP_POSITIVE(ioctl(masterfd, TIOCSWINSZ, &wsz1));
	TST_EXP_POSITIVE(ioctl(slavefd, TIOCGWINSZ, &wsz));

	TST_EXP_EQ_LI(wsz.ws_row, wsz1.ws_row);
	TST_EXP_EQ_LI(wsz.ws_col, wsz1.ws_col);
	TST_EXP_EQ_LI(wsz.ws_xpixel, wsz1.ws_xpixel);
	TST_EXP_EQ_LI(wsz.ws_ypixel, wsz1.ws_ypixel);

	TST_EXP_POSITIVE(ioctl(masterfd, TIOCGWINSZ, &wsz));

	TST_EXP_EQ_LI(wsz.ws_row, wsz1.ws_row);
	TST_EXP_EQ_LI(wsz.ws_col, wsz1.ws_col);
	TST_EXP_EQ_LI(wsz.ws_xpixel, wsz1.ws_xpixel);
	TST_EXP_EQ_LI(wsz.ws_ypixel, wsz1.ws_ypixel);

	TST_EXP_POSITIVE(ioctl(slavefd, TIOCSWINSZ, &wsz2));
	TST_EXP_POSITIVE(ioctl(slavefd, TIOCGWINSZ, &wsz));

	TST_EXP_EQ_LI(wsz.ws_row, wsz2.ws_row);
	TST_EXP_EQ_LI(wsz.ws_col, wsz2.ws_col);
	TST_EXP_EQ_LI(wsz.ws_xpixel, wsz2.ws_xpixel);
	TST_EXP_EQ_LI(wsz.ws_ypixel, wsz2.ws_ypixel);

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
