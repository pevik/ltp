// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 FUJITSU LIMITED. All rights reserved.
 * Author: Yang Xu <xuyang2018.jy@fujitsu.com>
 */

#ifndef LTP_QUOTACTL_SYSCALL_VAR_H
#define LTP_QUOTACTL_SYSCALL_VAR_H

#include <sys/mount.h>
#include "lapi/quotactl.h"

#define QUOTACTL_SYSCALL_VARIANTS 2
#define MNTPOINT "mntpoint"

static int fd = -1;

static inline int do_quotactl(int fd, int cmd, const char *special, int id, caddr_t addr)
{
	if (tst_variant == 0)
		return quotactl(cmd, special, id, addr);
	return quotactl_fd(fd, cmd, id, addr);
}

static inline void quotactl_info(void)
{
	if (tst_variant == 0)
		tst_res(TINFO, "Test quotactl()");
	else
		tst_res(TINFO, "Test quotactl_fd()");
}

static inline void do_mount(const char *source, const char *target,
	const char *filesystemtype, unsigned long mountflags,
	const void *data)
{
	TEST(mount(source, target, filesystemtype, mountflags, data));

	if (TST_RET == -1 && TST_ERR == ESRCH)
		tst_brk(TCONF, "Kernel or device does not support FS quotas");

	if (TST_RET == -1) {
		tst_brk(TBROK | TTERRNO, "mount(%s, %s, %s, %lu, %p) failed",
			source, target, filesystemtype, mountflags, data);
	}

	if (TST_RET) {
		tst_brk(TBROK | TTERRNO, "mount(%s, %s, %s, %lu, %p) failed",
			source, target, filesystemtype, mountflags, data);
	}
}

#endif /* LTP_QUOTACTL_SYSCALL_VAR_H */
