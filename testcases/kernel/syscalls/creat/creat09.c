// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 SUSE LLC <mdoucha@suse.cz>
 *
 * CVE-2018-13405
 *
 * Check for possible privilege escalation through creating files with setgid
 * bit set inside a setgid directory owned by a group which the user does not
 * belong to. Fixed in:
 *
 *  commit 0fa3ecd87848c9c93c2c828ef4c3a8ca36ce46c7
 *  Author: Linus Torvalds <torvalds@linux-foundation.org>
 *  Date:   Tue Jul 3 17:10:19 2018 -0700
 *
 *  Fix up non-directory creation in SGID directories
 */

#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>
#include "tst_test.h"

#define MODE_RWX        0777
#define MODE_SGID       (S_ISGID|0777)

#define WORKDIR		"testdir"
#define CREAT_FILE	WORKDIR "/creat.tmp"
#define OPEN_FILE	WORKDIR "/open.tmp"

static uid_t orig_uid;
static gid_t bin_gid;
static int fd = -1;

static void setup(void)
{
	struct stat buf;
	struct passwd *ltpuser = SAFE_GETPWNAM("nobody");
	struct group *ltpgroup = SAFE_GETGRNAM("bin");

	orig_uid = getuid();
	bin_gid = ltpgroup->gr_gid;

	/* Create directories and set permissions */
	SAFE_MKDIR(WORKDIR, MODE_RWX);
	SAFE_CHOWN(WORKDIR, ltpuser->pw_uid, bin_gid);
	SAFE_CHMOD(WORKDIR, MODE_SGID);
	SAFE_STAT(WORKDIR, &buf);

	if (!(buf.st_mode & S_ISGID))
		tst_brk(TBROK, "%s: Setgid bit not set", WORKDIR);

	if (buf.st_gid != bin_gid) {
		tst_brk(TBROK, "%s: Incorrect group, %u != %u", WORKDIR,
			buf.st_gid, bin_gid);
	}

	/* Switch user */
	ltpgroup = SAFE_GETGRNAM_FALLBACK("nobody", "nogroup");
	SAFE_SETGID(ltpgroup->gr_gid);
	SAFE_SETREUID(-1, ltpuser->pw_uid);
}

static void file_test(const char *name, gid_t gid)
{
	struct stat buf;

	SAFE_STAT(name, &buf);

	if (buf.st_gid != gid) {
		tst_res(TFAIL, "%s: Incorrect group, %u != %u", name,
			buf.st_gid, gid);
	} else {
		tst_res(TPASS, "%s: Owned by correct group", name);
	}

	if (buf.st_mode & S_ISGID)
		tst_res(TFAIL, "%s: Setgid bit is set", name);
	else
		tst_res(TPASS, "%s: Setgid bit not set", name);
}

static void run(void)
{
	fd = SAFE_CREAT(CREAT_FILE, MODE_SGID);
	SAFE_CLOSE(fd);
	file_test(CREAT_FILE, bin_gid);

	fd = SAFE_OPEN(OPEN_FILE, O_CREAT | O_EXCL | O_RDWR, MODE_SGID);
	file_test(OPEN_FILE, bin_gid);
	SAFE_CLOSE(fd);

	/* Cleanup between loops */
	tst_purge_dir(WORKDIR);
}

static void cleanup(void)
{
	SAFE_SETREUID(-1, orig_uid);

	if (fd >= 0)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.needs_tmpdir = 1,
	.tags = (const struct tst_tag[]) {
		{"linux-git", "0fa3ecd87848"},
		{"CVE", "2018-13405"},
		{}
	},
};
