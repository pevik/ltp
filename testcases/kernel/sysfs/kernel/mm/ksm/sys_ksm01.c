// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Sanity checks for the Kernel Samepage Merging (KSM) attributes exported
 * under /sys/kernel/mm/ksm/.
 *
 * The test verifies that:
 *
 * - run is one of 0 (stop), 1 (run) or 2 (unmerge)
 * - merge_across_nodes and use_zero_pages are booleans
 * - max_page_sharing is greater than zero
 * - pages_to_scan and sleep_millisecs are non-negative
 * - the page accounting counters (pages_shared, pages_sharing, pages_unshared,
 *   pages_volatile, stable_node_chains, stable_node_dups) are non-negative
 *
 * All checks skip gracefully with TCONF when a particular attribute is not
 * present, since the set of KSM tunables differs between kernel versions.
 */

#include <limits.h>
#include "tst_test.h"
#include "tst_sysfs_assert.h"

#define KSM "/sys/kernel/mm/ksm"

static const char *const nonneg_counters[] = {
	"pages_shared",
	"pages_sharing",
	"pages_unshared",
	"pages_volatile",
	"stable_node_chains",
	"stable_node_dups",
	"pages_to_scan",
	"sleep_millisecs",
};

static void do_test(void)
{
	unsigned int i;

	/* 0 = stop, 1 = merge, 2 = unmerge */
	TST_SYSFS_ASSERT_RANGELL(0, 2, KSM "/run");

	TST_SYSFS_ASSERT_BOOL(KSM "/merge_across_nodes");
	TST_SYSFS_ASSERT_BOOL(KSM "/use_zero_pages");

	TST_SYSFS_ASSERT_RANGELL(1, LONG_MAX, KSM "/max_page_sharing");

	for (i = 0; i < ARRAY_SIZE(nonneg_counters); i++) {
		TST_SYSFS_ASSERT_RANGELL(0, LONG_MAX, KSM "/%s",
				       nonneg_counters[i]);
	}
}

static struct tst_test test = {
	.test_all = do_test,
	.needs_kconfigs = (const char *const[]){
		"CONFIG_KSM=y",
		NULL
	},
};
