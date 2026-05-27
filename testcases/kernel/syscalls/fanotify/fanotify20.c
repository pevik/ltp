// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 Google. All Rights Reserved.
 * Copyright (c) 2022 Petr Vorel <pvorel@suse.cz>
 *
 * Started by Matthew Bobrowski <repnop@google.com>
 */

/*\
 * This source file contains a test case which ensures that the
 * :manpage:`fanotify(7)` API returns an expected error code when provided an
 * invalid initialization flag alongside FAN_REPORT_PIDFD. Additionally, it
 * checks that the operability with existing FAN_REPORT_* flags is maintained
 * and functioning as intended.
 *
 * NOTE: FAN_REPORT_PIDFD support was added in v5.15-rc1 in
 * af579beb666a ("fanotify: add pidfd support to the fanotify API").
 */

#define _GNU_SOURCE
#include "tst_test.h"
#include <errno.h>

#ifdef HAVE_SYS_FANOTIFY_H
#include "fanotify.h"

#define MOUNT_PATH	"fs_mnt"
#define FLAGS_DESC(x) .flags = x, .desc = #x

static int fd;
static int thread_pidfd_unsupported;

static struct test_case_t {
	unsigned int flags;
	char *desc;
	int exp_errno;
	unsigned int needs_thread_pidfd;
} test_cases[] = {
	{
		FLAGS_DESC(FAN_REPORT_PIDFD | FAN_REPORT_TID),
		.exp_errno = EINVAL,
		.needs_thread_pidfd = 1,
	},
	{
		FLAGS_DESC(FAN_REPORT_PIDFD | FAN_REPORT_FID | FAN_REPORT_DFID_NAME),
	},
	{
		FLAGS_DESC(FAN_REPORT_PIDFD | FAN_REPORT_TID | FAN_REPORT_FID | FAN_REPORT_DFID_NAME),
		.exp_errno = EINVAL,
		.needs_thread_pidfd = 1,
	},
};

static void do_setup(void)
{
	/*
	 * An explicit check for FAN_REPORT_PIDFD is performed early on in the
	 * test initialization as it's a prerequisite for all test cases.
	 */
	REQUIRE_FANOTIFY_INIT_FLAGS_SUPPORTED_ON_FS(FAN_REPORT_PIDFD,
						    MOUNT_PATH);

	/*
	 * Check whether the kernel supports FAN_REPORT_PIDFD in combination
	 * with FAN_REPORT_TID. Test cases with the needs_thread_pidfd field
	 * set expect different errno values depending on whether this
	 * combination is supported.
	 */
	thread_pidfd_unsupported = fanotify_init_flags_supported_on_fs(
		FAN_REPORT_PIDFD | FAN_REPORT_TID, MOUNT_PATH);
}

static void do_test(unsigned int i)
{
	struct test_case_t *tc = &test_cases[i];

	int exp_errno = tc->needs_thread_pidfd && !thread_pidfd_unsupported ?
		0 : tc->exp_errno;

	tst_res(TINFO, "Test %s on %s", exp_errno ? "fail" : "pass",
		tc->desc);

	TST_EXP_FD_OR_FAIL(fd = fanotify_init(tc->flags, O_RDONLY),
			   exp_errno);

	if (fd > 0)
		SAFE_CLOSE(fd);
}

static void do_cleanup(void)
{
	if (fd > 0)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.setup = do_setup,
	.test = do_test,
	.tcnt = ARRAY_SIZE(test_cases),
	.cleanup = do_cleanup,
	.all_filesystems = 1,
	.needs_root = 1,
	.mntpoint = MOUNT_PATH,
};

#else
	TST_TEST_TCONF("system doesn't have required fanotify support");
#endif /* HAVE_SYS_FANOTIFY_H */
