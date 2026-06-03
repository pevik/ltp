// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Cedric Hnyda <chnyda@suse.com>
 * Copyright (c) 2026 Petr Vorel <pvorel@suse.cz>
 */

/*\
 * Verify that :manpage:`renameat2(2)` works on these flags:
 *
 * 1. RENAME_EXCHANGE
 * 2. RENAME_NOREPLACE
 */

#define _GNU_SOURCE

#include "lapi/fcntl.h"
#include "tst_test.h"
#include "renameat2.h"

#define TEST_DIR "test_dir/"
#define TEST_DIR2 "test_dir2/"

#define TEST_FILE "test_file"
#define TEST_FILE2 "test_file2"

#define TEST_PATH TEST_DIR TEST_FILE
#define TEST_PATH2 TEST_DIR2 TEST_FILE2

#define FLAGS_DESC(x) .flags = x, .desc = #x

static struct tcase {
	int flags;
	char *desc;
} tcases[] = {
	{FLAGS_DESC(RENAME_EXCHANGE)},
	{FLAGS_DESC(RENAME_NOREPLACE)},
};

static int olddirfd;
static int newdirfd;
static long fs_type;

static void setup(void)
{
	// TODO
	fs_type = tst_fs_type(".");

	SAFE_MKDIR(TEST_DIR, 0700);
	SAFE_MKDIR(TEST_DIR2, 0700);

	olddirfd = SAFE_OPEN(TEST_DIR, O_DIRECTORY);
	newdirfd = SAFE_OPEN(TEST_DIR2, O_DIRECTORY);
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

	SAFE_TOUCH(TEST_PATH, 0600, NULL);
	if (!i)
		SAFE_TOUCH(TEST_PATH2, 0600, NULL);

	TST_EXP_PASS(renameat2(olddirfd, TEST_FILE, newdirfd, TEST_FILE2, tc->flags));

	SAFE_UNLINK(TEST_PATH2);
}

static struct tst_test test = {
	.test = renameat2_verify,
	.tcnt = ARRAY_SIZE(tcases),
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
