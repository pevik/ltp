// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2013 Oracle and/or its affiliates. All Rights Reserved.
 * Author: Stanislav Kholmanskikh <stanislav.kholmanskikh@oracle.com>
 */

#include <errno.h>

#define TST_NO_DEFAULT_MAIN

#include "tst_test.h"
#include "libswap.h"
#include "lapi/syscalls.h"
#include "tst_kconfig.h"
#include "tst_safe_stdio.h"

/*
 * Make a swap file
 */
int make_swapfile(const char *swapfile, int safe)
{
	if (!tst_fs_has_free(".", sysconf(_SC_PAGESIZE) * 10, TST_BYTES))
		tst_brk(TBROK, "Insufficient disk space to create swap file");

	/* create file */
	if (tst_fill_file(swapfile, 0, sysconf(_SC_PAGESIZE), 10) != 0)
		tst_brk(TBROK, "Failed to create swapfile");

	/* make the file swapfile */
	const char *argv[2 + 1];
	argv[0] = "mkswap";
	argv[1] = swapfile;
	argv[2] = NULL;

	return tst_cmd(argv, "/dev/null", "/dev/null", safe);
}

/*
 * Check swapon/swapoff support status of filesystems or files
 * we are testing on.
 */
void is_swap_supported(const char *filename)
{
	int fibmap = tst_fibmap(filename);
	long fs_type = tst_fs_type(filename);
	const char *fstype = tst_fs_type_name(fs_type);

	int ret = make_swapfile(filename, 1);
	if (ret != 0) {
		if (fibmap == 1)
			tst_brk(TCONF, "mkswap on %s not supported", fstype);
		else
			tst_brk(TFAIL, "mkswap on %s failed", fstype);
	}

	TEST(tst_syscall(__NR_swapon, filename, 0));
	if (TST_RET == -1) {
		if (errno == EPERM)
			tst_brk(TCONF, "Permission denied for swapon()");
		else if (fibmap == 1 && errno == EINVAL)
			tst_brk(TCONF, "Swapfile on %s not implemented", fstype);
		else
			tst_brk(TFAIL | TTERRNO, "swapon() on %s failed", fstype);
	}

	TEST(tst_syscall(__NR_swapoff, filename, 0));
	if (TST_RET == -1)
		tst_brk(TFAIL | TTERRNO, "swapoff on %s failed", fstype);
}

/*
 * Get kernel constant MAX_SWAPFILES value
 */
unsigned int get_maxswapfiles(void)
{
	unsigned int max_swapfile = 32;
	unsigned int swp_migration_num = 0, swp_hwpoison_num = 0, swp_device_num = 0, swp_pte_marker_num = 0;
	struct tst_kconfig_var migration_kconfig = TST_KCONFIG_INIT("CONFIG_MIGRATION");
	struct tst_kconfig_var memory_kconfig = TST_KCONFIG_INIT("CONFIG_MEMORY_FAILURE");
	struct tst_kconfig_var device_kconfig = TST_KCONFIG_INIT("CONFIG_DEVICE_PRIVATE");
	struct tst_kconfig_var marker_kconfig = TST_KCONFIG_INIT("CONFIG_PTE_MARKER");

	tst_kconfig_read(&migration_kconfig, 1);
	tst_kconfig_read(&memory_kconfig, 1);
	tst_kconfig_read(&device_kconfig, 1);
	tst_kconfig_read(&marker_kconfig, 1);

	if (migration_kconfig.choice == 'y') {
		if (tst_kvercmp(5, 19, 0) < 0)
			swp_migration_num = 2;
		else
			swp_migration_num = 3;
	}

	if (memory_kconfig.choice == 'y')
		swp_hwpoison_num = 1;

	if (device_kconfig.choice == 'y') {
		if (tst_kvercmp(4, 14, 0) >= 0)
			swp_device_num = 2;
		if (tst_kvercmp(5, 14, 0) >= 0)
			swp_device_num = 4;
	}

	if (marker_kconfig.choice == 'y') {
		if (tst_kvercmp(5, 19, 0) >= 0)
			swp_pte_marker_num = 1;
	}

	return max_swapfile - swp_migration_num - swp_hwpoison_num - swp_device_num - swp_pte_marker_num;
}
