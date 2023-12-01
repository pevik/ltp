// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2010-2017  Red Hat, Inc.
 */
/*\
 * [Description]
 *
 * Kernel Samepage Merging (KSM)
 *
 * This adds a new ksm (kernel samepage merging) test to evaluate the new
 * smart scan feature. It allocates a page and fills it with 'a'
 * characters. It captures the pages_skipped counter, waits for a few
 * iterations and captures the pages_skipped counter again. The expectation
 * is that over 50% of the page scans are skipped (There is only one page
 * that has KSM enabled and it gets scanned during each iteration and it
 * cannot be de-duplicated).
 *
 * Prerequisites:
 *
 * 1) ksm and ksmtuned daemons need to be disabled. Otherwise, it could
 *    distrub the testing as they also change some ksm tunables depends
 *    on current workloads.
 */

#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "../include/mem.h"
#include "ksm_common.h"

static void verify_ksm(void)
{
	create_memory_for_smartscan();
}

static struct tst_test test = {
	.needs_root = 1,
	.forks_child = 1,
	.options = (struct tst_option[]) {
		{}
	},
	.save_restore = (const struct tst_path_val[]) {
		{"/sys/kernel/mm/ksm/run", NULL, TST_SR_TBROK},
		{"/sys/kernel/mm/ksm/sleep_millisecs", NULL, TST_SR_TBROK},
		{"/sys/kernel/mm/ksm/smart_scan", "1",
			TST_SR_SKIP_MISSING | TST_SR_TBROK_RO},
		{}
	},
	.needs_kconfigs = (const char *const[]){
		"CONFIG_KSM=y",
		NULL
	},
	.test_all = verify_ksm,
};
