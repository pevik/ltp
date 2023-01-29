// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2023 Wei Gao <wegao@suse.com>
 */

/*\
 * Test add some coverage to CVE-2022-0185.
 * Try to trigger a crash.
 * References links:
 * https://www.openwall.com/lists/oss-security/2022/01/25/14
 * https://github.com/Crusaders-of-Rust/CVE-2022-0185
 */

#include "tst_test.h"
#include "lapi/fsmount.h"

#define MNTPOINT	"mntpoint"

static int fd = -1;

static void setup(void)
{
	fsopen_supported_by_kernel();

	TEST(fd = fsopen(tst_device->fs_type, 0));
	if (fd == -1)
		tst_brk(TBROK | TTERRNO, "fsopen() failed");

}

static void cleanup(void)
{
	if (fd != -1)
		SAFE_CLOSE(fd);
}

static void run(void)
{
	char *val = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

	for (unsigned int i = 0; i < 2; i++) {
		TEST(fsconfig(fd, FSCONFIG_SET_STRING, "\x00", val, 0));
		if (TST_RET == -1)
			tst_brk(TFAIL | TTERRNO, "fsconfig(FSCONFIG_SET_STRING) failed");
	}
	tst_res(TPASS, "Try fsconfig overflow on %s done!", tst_device->fs_type);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.format_device = 1,
	.mntpoint = MNTPOINT,
	.all_filesystems = 1,
	.skip_filesystems = (const char *const []){"fuse", "ext2", "xfs", "tmpfs", NULL},
};
