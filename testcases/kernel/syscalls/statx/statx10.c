// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 FUJITSU LIMITED. All rights reserved.
 * Author: Yang Xu <xuyang2018.jy@fujitsu.com>
 */

/*\
 * [Description]
 *
 * It is a basic test for STATX_DIOALIGN mask.
 *
 * - STATX_DIOALIGN   Want stx_dio_mem_align and stx_dio_offset_align value
 *
 * Minimum Linux version required is v6.1.
 */

#define _GNU_SOURCE
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include "tst_test.h"
#include "lapi/stat.h"

#ifdef HAVE_STRUCT_STATX_STX_DIO_MEM_ALIGN
#define MNTPOINT "mnt_point"
#define TESTFILE "testfile"

static int fd = -1;

static void verify_statx(void)
{
	struct statx buf;

	memset(&buf, 0, sizeof(buf));
	TST_EXP_PASS(statx(AT_FDCWD, TESTFILE, 0, STATX_DIOALIGN, &buf),
		"statx(AT_FDCWD, %s, 0, STATX_DIOALIGN, &buf)", TESTFILE);

	if (!(buf.stx_mask & STATX_DIOALIGN)) {
		tst_res(TCONF, "STATX_DIOALIGN is not supported until linux 6.1");
		return;
	}

	if (buf.stx_dio_mem_align != 0)
		tst_res(TPASS, "stx_dio_mem_align:%u", buf.stx_dio_mem_align);
	else
		tst_res(TFAIL, "don't get stx_dio_mem_align on supported dio fs");

	if (buf.stx_dio_offset_align != 0)
		tst_res(TPASS, "stx_dio_offset_align:%u", buf.stx_dio_offset_align);
	else
		tst_res(TFAIL, "don't get stx_dio_offset_align on supported dio fs");
}

static void setup(void)
{
	SAFE_FILE_PRINTF(TESTFILE, "AAAA");
	fd = open(TESTFILE, O_RDWR | O_DIRECT);
	if (fd == -1 && errno == EINVAL)
		tst_brk(TCONF, "The regular file is not on a filesystem that support DIO");
}

static void cleanup(void)
{
	if (fd > -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test_all = verify_statx,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.mntpoint = MNTPOINT,
	.mount_device = 1,
	.all_filesystems = 1,
};
#else
TST_TEST_TCONF("test requires struct statx to have the stx_dio_mem_align fields");
#endif
