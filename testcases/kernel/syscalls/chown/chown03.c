// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 *
 * 07/2001 Ported by Wayne Boyer
 */

/*\
 * [Description]
 *
 * Verify that, chown(2) succeeds to change the group of a file specified
 * by path when called by non-root user with the following constraints,
 * - euid of the process is equal to the owner of the file.
 * - the intended gid is either egid, or one of the supplementary gids
 *   of the process.
 * Also, verify that chown() clears the setuid/setgid bits set on the file.
 *
 * [Algorithm]
 *
 * - Execute system call
 * - Check return code, if system call failed (return=-1)
 * -   Log the errno and Issue a FAIL message
 * - Otherwise
 * -   Verify the Functionality of system call
 * -   if successful
 * -     Issue Functionality-Pass message
 * -   Otherwise
 * -     Issue Functionality-Fail message
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <grp.h>
#include <pwd.h>

#include "tst_test.h"
#include "compat_tst_16.h"

#define FILE_MODE	(S_IFREG|S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH)
#define NEW_PERMS	(S_IFREG|S_IRWXU|S_IRWXG|S_ISUID|S_ISGID)
#define TESTFILE	"testfile"

static void run(void)
{
	struct stat stat_buf;	/* stat(2) struct contents */
	uid_t user_id;		/* Owner id of the test file. */
	gid_t group_id;		/* Group id of the test file. */

	UID16_CHECK((user_id = geteuid()), "chown");
	GID16_CHECK((group_id = getegid()), "chown");

	TEST(CHOWN(TESTFILE, -1, group_id));
	if (TST_RET == -1)
		tst_res(TFAIL | TTERRNO, "chown(%s, ..) failed",
			TESTFILE);

	SAFE_STAT(TESTFILE, &stat_buf);

	if (stat_buf.st_uid != user_id || stat_buf.st_gid != group_id)
		tst_res(TFAIL, "%s: Incorrect ownership"
			       "set to %d %d, Expected %d %d",
			TESTFILE, stat_buf.st_uid,
			stat_buf.st_gid, user_id, group_id);

	if (stat_buf.st_mode != (NEW_PERMS & ~(S_ISUID | S_ISGID)))
		tst_res(TFAIL, "%s: incorrect mode permissions"
			       " %#o, Expected %#o", TESTFILE,
			stat_buf.st_mode,
			NEW_PERMS & ~(S_ISUID | S_ISGID));
	else
		tst_res(TPASS, "chown(%s, ..) was successful",
			TESTFILE);
}

static void setup(void)
{
	int fd;		/* file handler for testfile */
	struct passwd *ltpuser;

	ltpuser = SAFE_GETPWNAM("nobody");
	if (ltpuser == NULL)
		tst_brk(TBROK | TTERRNO,
			"getpwnam(\"nobody\") failed");
	SAFE_SETEGID(ltpuser->pw_gid);
	SAFE_SETEUID(ltpuser->pw_uid);

	/* Create a test file under temporary directory */
	fd = SAFE_OPEN(TESTFILE, O_RDWR | O_CREAT, FILE_MODE);

	SAFE_SETEUID(0);
	SAFE_FCHOWN(fd, -1, 0);
	SAFE_FCHMOD(fd, NEW_PERMS);
	SAFE_SETEUID(ltpuser->pw_uid);
	SAFE_CLOSE(fd);
}

static void cleanup(void)
{
	SAFE_SETEGID(0);
	SAFE_SETEUID(0);
}

static struct tst_test test = {
	.needs_root = 1,
	.needs_tmpdir = 1,
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
};

