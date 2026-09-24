// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Cyril Hrubis <chrubis@suse.cz>
 * Copyright (c) Linux Test Project, 2026
 */

/*\
 * Call :manpage:`shmctl(2)` with IPC_INFO flag and check that the data are
 * consistent with /proc/sys/kernel/shm*.
 */

#define _GNU_SOURCE
#include "tst_test.h"
#include "tst_safe_sysv_ipc.h"
#include "tse_newipc.h"

static void verify_ipcinfo(void)
{
	struct shminfo info;

	safe_shmctl(__FILE__, __LINE__, 0, IPC_INFO, ((struct shmid_ds *)&info));
	TST_EXP_LE_LU(info.shmmin, 1);

	if (tst_is_compat_mode() && info.shmmax == INT_MAX) {
		unsigned long long shmmax;
		SAFE_FILE_SCANF(PATH_KERN_SHMMAX, "%llu", &shmmax);
		TST_EXP_LE_LU(INT_MAX, shmmax);
	} else {
		TST_ASSERT_ULONG(PATH_KERN_SHMMAX, info.shmmax);
	}

	TST_ASSERT_ULONG(PATH_KERN_SHMMNI, info.shmmni);
	TST_ASSERT_ULONG(PATH_KERN_SHMALL, info.shmall);
}

static struct tst_test test = {
	.test_all = verify_ipcinfo,
};
