// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2002
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that write/read is properly working when master and slave
 * pseudo terminals communicate with each other.
 */

#define _GNU_SOURCE

#include "tst_test.h"

#define MASTERCLONE "/dev/ptmx"
#define STRING "Linux Test Project"
#define STRING_LEN strlen(STRING)

static void run(void)
{
	int masterfd;
	int slavefd;
	char *slavename;
	struct stat st;
	char buf[BUFSIZ];

	memset(buf, 0, BUFSIZ);

	masterfd = SAFE_OPEN(MASTERCLONE, O_RDWR);
	slavename = SAFE_PTSNAME(masterfd);

	if (grantpt(masterfd) == -1)
		tst_brk(TBROK | TERRNO, "grantpt() error");

	TST_EXP_PASS_SILENT(unlockpt(masterfd));
	if (TST_RET == -1) {
		SAFE_CLOSE(masterfd);
		return;
	}

	SAFE_STAT(slavename, &st);
	TST_EXP_EQ_LI(st.st_uid, getuid());

	/* grantpt() is a no-op in bionic. */
#ifndef __BIONIC__
	TST_EXP_EQ_LI(st.st_mode, 0620);
#endif

	slavefd = SAFE_OPEN(slavename, O_RDWR);

	tst_res(TINFO, "Send message to master and read from slave");
	SAFE_WRITE(SAFE_WRITE_ALL, masterfd, STRING, STRING_LEN);
	SAFE_WRITE(SAFE_WRITE_ALL, masterfd, "\n", 1);
	SAFE_READ(1, slavefd, buf, STRING_LEN);
	TST_EXP_EQ_STR(STRING, buf);

	tst_res(TINFO, "Send message to slave and read from master");
	SAFE_WRITE(SAFE_WRITE_ALL, slavefd, STRING, STRING_LEN);
	SAFE_WRITE(SAFE_WRITE_ALL, masterfd, "\n", 1);
	SAFE_READ(1, masterfd, buf, STRING_LEN);
	TST_EXP_EQ_STR(STRING, buf);

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
