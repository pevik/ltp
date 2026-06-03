// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Cedric Hnyda <chnyda@suse.com>
 * Copyright (c) 2026 Petr Vorel <pvorel@suse.cz>
 */

/*\
 * Verify that :manpage:`renameat2(2)` returns -1 and sets errno to:
 *
 * 1. EEXIST when newpath already exists and the flag RENAME_NOREPLACE is used.
 * 2. ENOENT when the flag RENAME_EXCHANGE is used and newpath does not exist.
 * 3. EINVAL when RENAME_NOREPLACE and RENAME_EXCHANGE are used together
 * 4. EINVAL when RENAME_WHITEOUT and RENAME_EXCHANGE are used together
 */

#define _GNU_SOURCE

#include "tst_test.h"
#include "lapi/fcntl.h"
#include "lapi/stdio.h"

#define TEST_DIR "test_dir/"
#define TEST_DIR2 "test_dir2/"

#define TEST_PATH TEST_DIR TEST_FILE
#define TEST_PATH2 TEST_DIR2 TEST_FILE2

#define TEST_FILE "test_file"
#define TEST_FILE2 "test_file2"
#define NON_EXIST "non_exist"

#define FLAGS_ERRNO_DESC(x, y) .flags = x, .exp_errno = y, .desc = "flags: " #x ", errno: " #y

static struct tcase {
	const char *newpath;
	int flags;
	int exp_errno;
	char *desc;
} tcases[] = {
	{TEST_FILE2, FLAGS_ERRNO_DESC(RENAME_NOREPLACE, EEXIST)},
	{NON_EXIST, FLAGS_ERRNO_DESC(RENAME_EXCHANGE, ENOENT)},
	{TEST_FILE2, FLAGS_ERRNO_DESC(RENAME_NOREPLACE | RENAME_EXCHANGE, EINVAL)},
	{TEST_FILE2, FLAGS_ERRNO_DESC(RENAME_WHITEOUT | RENAME_EXCHANGE, EINVAL)}
};

static int olddirfd;
static int newdirfd;
static long fs_type;

static void setup(void)
{
	fs_type = tst_fs_type(".");

	SAFE_MKDIR(TEST_DIR, 0700);
	SAFE_MKDIR(TEST_DIR2, 0700);

	olddirfd = SAFE_OPEN(TEST_DIR, O_DIRECTORY);
	newdirfd = SAFE_OPEN(TEST_DIR2, O_DIRECTORY);

	SAFE_TOUCH(TEST_PATH, 0600, NULL);
	SAFE_TOUCH(TEST_PATH2, 0600, NULL);
}

static void cleanup(void)
{
	if (olddirfd > 0)
		SAFE_CLOSE(olddirfd);

	if (newdirfd > 0)
		SAFE_CLOSE(newdirfd);
}

static void renameat2_verify(unsigned int i)
{
	struct tcase *tc = &tcases[i];

	tst_res(TINFO, "Testing flag: %s", tc->desc);

	TST_EXP_FAIL(renameat2(olddirfd, TEST_FILE, newdirfd, tc->newpath,
						   tc->flags), tc->exp_errno);
}

static struct tst_test test = {
	.test = renameat2_verify,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
