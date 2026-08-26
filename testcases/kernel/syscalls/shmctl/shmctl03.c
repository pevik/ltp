// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Call shmctl() with IPC_INFO flag and check that the data are consistent with
 * /proc/sys/kernel/shm*.
 */

#define _GNU_SOURCE
#include "tst_test.h"
#include "tst_safe_sysv_ipc.h"
#include "tse_newipc.h"

static void verify_ipcinfo(void)
{
	struct shminfo info;

	TEST(shmctl(0, IPC_INFO, (struct shmid_ds *)&info));

	if (TST_RET < 0) {
		tst_res(TFAIL | TTERRNO,
			"shmctl(0, IPC_INFO, ...) returned %li", TST_RET);
		return;
	}

	if (info.shmmin != 1)
		tst_res(TFAIL, "shmmin = %li, expected 1", info.shmmin);
	else
		tst_res(TPASS, "shmmin = 1");

	if (tst_is_compat_mode()) {
		/*
		 * On 64-bit kernel, shmmax is clamped to INT_MAX for 32-bit
		 * compat syscall, while shmmni and shmall are truncated
		 * to 32-bit.
		 */
		TST_ASSERT_ULONG(PATH_KERN_SHMMAX, info.shmmax, TST_ASSERT_SATURATED_INT);
		TST_ASSERT_ULONG(PATH_KERN_SHMMNI, info.shmmni, TST_ASSERT_TRUNC_32BIT);
		TST_ASSERT_ULONG(PATH_KERN_SHMALL, info.shmall, TST_ASSERT_TRUNC_32BIT);
	} else {
		TST_ASSERT_ULONG(PATH_KERN_SHMMAX, info.shmmax);
		TST_ASSERT_ULONG(PATH_KERN_SHMMNI, info.shmmni);
		TST_ASSERT_ULONG(PATH_KERN_SHMALL, info.shmall);
	}
}

static struct tst_test test = {
	.test_all = verify_ipcinfo,
};
