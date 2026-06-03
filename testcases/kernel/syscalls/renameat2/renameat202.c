// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2015 Cedric Hnyda <chnyda@suse.com>
 * Copyright (c) 2026 Petr Vorel <pvorel@suse.cz>
 */

/*\
 * Verify that :manpage:`renameat2(2)` with RENAME_EXCHANGE swapps the content.
 */

#define _GNU_SOURCE

#include "lapi/fcntl.h"
#include "tst_test.h"
#include "renameat2.h"

#define TEST_DIR "test_dir/"
#define TEST_DIR2 "test_dir2/"

#define TEST_FILE "test_file"
#define TEST_FILE2 "test_file2"

#define CONTENT "content"

static int olddirfd;
static int newdirfd;
static int fd = -1;
static long fs_type;

/* workaround for iterations not being exposed by lib/tst_test.c */
static int cnt;


static void setup(void)
{
	fs_type = tst_fs_type(".");

	SAFE_MKDIR(TEST_DIR, 0700);
	SAFE_MKDIR(TEST_DIR2, 0700);

	SAFE_TOUCH(TEST_DIR TEST_FILE, 0600, NULL);
	SAFE_TOUCH(TEST_DIR2 TEST_FILE2, 0600, NULL);

	olddirfd = SAFE_OPEN(TEST_DIR, O_DIRECTORY);
	newdirfd = SAFE_OPEN(TEST_DIR2, O_DIRECTORY);

	SAFE_FILE_PRINTF(TEST_DIR TEST_FILE, "%s", CONTENT);

}

static void cleanup(void)
{
	if (olddirfd > 0)
		SAFE_CLOSE(olddirfd);

	if (newdirfd > 0)
		SAFE_CLOSE(newdirfd);

	if (fd > 0)
		SAFE_CLOSE(fd);
}

static void renameat2_verify(void)
{
	char str[BUFSIZ] = { 0 };
	struct stat st;
	char *emptyfile;
	char *contentfile;

	TST_EXP_PASS(renameat2(olddirfd, TEST_FILE, newdirfd, TEST_FILE2,
						   RENAME_EXCHANGE));
	if (!TST_PASS)
		return;

	/*
	 * TODO
	if (TEST_ERRNO == EINVAL && TST_BTRFS_MAGIC == fs_type) {
		tst_brk(TCONF, "RENAME_EXCHANGE flag is not implemeted on %s",
			tst_fs_type_name(fs_type));
	}
	*/

	if (cnt % 2 == 0) {
		emptyfile = TEST_DIR TEST_FILE;
		contentfile = TEST_DIR2 TEST_FILE2;
	} else {
		emptyfile = TEST_DIR2 TEST_FILE2;
		contentfile = TEST_DIR TEST_FILE;
	}
	cnt++;

	fd = SAFE_OPEN(contentfile, O_RDONLY);

	SAFE_STAT(emptyfile, &st);

	SAFE_READ(SAFE_READ_ANY_EAGAIN, fd, str, BUFSIZ);
	SAFE_CLOSE(fd);

	TST_EXP_EQ_STRN(CONTENT, str, sizeof(CONTENT) - 1);
	if (!TST_PASS)
		return;

	if (st.st_size) {
		tst_res(TFAIL, "emptyfile has non-zero file size");
		return;
	}

	tst_res(TPASS, "renameat2() test passed");
}

static struct tst_test test = {
	.test_all = renameat2_verify,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
};
