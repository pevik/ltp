// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2025 SUSE LLC Wei Gao <wegao@suse.com>
 */

/*\
 * [Description]
 *
 * Basic mount_setattr() test.
 * Test basic propagation mount attributes are set correctly.
 */

#define _GNU_SOURCE

#include <sys/statvfs.h>
#include "tst_test.h"
#include "lapi/fsmount.h"
#include "tst_safe_stdio.h"

#define DIRA "/DIRA_PROPAGATION_CHECK"

static bool is_shared_mount(const char *path)
{
	FILE *file = SAFE_FOPEN("/proc/self/mountinfo", "r");

	char line[PATH_MAX];
	bool found = false;

	while (fgets(line, sizeof(line), file)) {
		char mntpoint[PATH_MAX];
		char opts[256];

		if (sscanf(line, "%*d %*d %*d:%*d %*s %255s %*s %255s",
					mntpoint, opts) != 2)
			continue;

		if (strcmp(mntpoint, path) != 0)
			continue;

		if (strstr(opts, "shared:") != NULL) {
			found = true;
			break;
		}
	}

	fclose(file);
	return found;
}

static void cleanup(void)
{
	SAFE_RMDIR(DIRA);

}

static void setup(void)
{
	fsopen_supported_by_kernel();

	SAFE_MKDIR(DIRA, 0777);
}

static void run(void)
{

	SAFE_UNSHARE(CLONE_NEWNS);
	SAFE_MOUNT(NULL, "/", NULL, MS_REC | MS_PRIVATE, 0);
	SAFE_MOUNT("testing", DIRA, "tmpfs", MS_NOATIME | MS_NODEV, "");

	struct mount_attr attr = {
		.attr_set       = MOUNT_ATTR_RDONLY | MOUNT_ATTR_NOEXEC | MOUNT_ATTR_RELATIME,
		.attr_clr       = MOUNT_ATTR__ATIME,
	};

	TST_EXP_PASS_SILENT(mount_setattr(-1, DIRA, 0, &attr, sizeof(attr)));
	TST_EXP_EQ_LI(is_shared_mount(DIRA), 0);

	attr.propagation = -1;
	TST_EXP_FAIL_SILENT(mount_setattr(-1, DIRA, 0, &attr, sizeof(attr)), EINVAL);
	TST_EXP_EQ_LI(is_shared_mount(DIRA), 0);

	attr.propagation = MS_SHARED;
	TST_EXP_PASS_SILENT(mount_setattr(-1, DIRA, 0, &attr, sizeof(attr)));
	TST_EXP_EQ_LI(is_shared_mount(DIRA), 1);

	attr.propagation = MS_PRIVATE;
	TST_EXP_PASS_SILENT(mount_setattr(-1, DIRA, 0, &attr, sizeof(attr)));
	TST_EXP_EQ_LI(is_shared_mount(DIRA), 0);

	attr.propagation = MS_SLAVE;
	TST_EXP_PASS_SILENT(mount_setattr(-1, DIRA, 0, &attr, sizeof(attr)));
	TST_EXP_EQ_LI(is_shared_mount(DIRA), 0);

	SAFE_UMOUNT(DIRA);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
};
