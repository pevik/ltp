// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2009 FUJITSU LIMITED
 * Author: Miao Xie <miaox@cn.fujitsu.com>
 *
 * Rewritten into a single C test using the LTP cgroup library.
 */

/*\
 * Verify the ``cpuset.memory_spread_page`` control file.
 *
 * A task's file-backed page cache is normally allocated on the node the task
 * runs on (local allocation). When ``cpuset.memory_spread_page`` is enabled,
 * the page cache of a task in the cpuset is instead spread evenly over all
 * the memory nodes allowed by ``cpuset.mems``.
 *
 * For every case the test pins itself to the local node, sets ``cpuset.mems``
 * and ``cpuset.memory_spread_page``, then faults a file-backed shared mapping
 * and uses :manpage:`get_mempolicy(2)` (through the tse_numa helpers) to learn
 * the NUMA node each of the mapping's pages resides on. Because only the test
 * mapping's own pages are counted, unrelated page-cache activity on the system
 * cannot perturb the result. The resulting distribution is verified:
 *
 * - with spreading off the pages must land on the expected node (the local
 *   node, or the only node allowed by ``cpuset.mems``);
 * - with spreading on over several nodes the pages must be spread evenly.
 *
 * ``cpuset.memory_spread_page`` exists only in the cgroup v1 cpuset
 * controller, so the test requires a v1 cpuset hierarchy and is skipped
 * (TCONF) when cpuset is mounted on the unified (v2) hierarchy.
 *
 * The test is skipped on tmpfs/ramfs, where the mapping's pages are the
 * backing store itself rather than ordinary page cache, so their placement
 * is not governed by this knob.
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifdef HAVE_NUMA_H
# include <numa.h>
#endif

#include "tst_test.h"
#include "tst_cgroup.h"
#include "tse_numa.h"

#ifdef HAVE_NUMA_V2

/* Path of the file-backed mapping whose page placement we measure. */
#define DATAFILE	"numa_data_file"

/* Size of the mapping. Large enough to measure an even spread reliably. */
#define SIZE_MB		100
#define SIZE		((size_t)SIZE_MB * 1024 * 1024)

/* Each node needs enough free memory to hold the whole mapping plus slack. */
#define MIN_FREE_KB	((SIZE_MB + 50) * 1024)

/*
 * The pages must land on the expected node(s); the other nodes may hold at
 * most this fraction (in percent) of them.
 */
#define UNEXPECTED_TOLERANCE	5

/*
 * When the pages are spread across several expected nodes, the kernel spreads
 * them evenly. Every expected node must hold at least
 * (100 - BALANCE_TOLERANCE) percent of its even share.
 */
#define BALANCE_TOLERANCE	20

static struct tse_nodemap *nodes;

/*
 * The task's CPUs are pinned to the local node (nodes->map[0]) for every case
 * (see setup()); only cpuset.mems and cpuset.memory_spread_page change.
 *
 * mems/expect are strings of single-digit indices into nodes->map: '0' is the
 * local node, '1' the remote one.
 */
static const struct tcase {
	int spread;
	const char *mems;
	const char *expect;
	const char *desc;
} tcases[] = {
	{0, "0",  "0",  "spread off, mems on the local node"},
	{1, "0",  "0",  "spread on, mems on the local node"},
	{0, "1",  "1",  "spread off, mems on the remote node"},
	{1, "1",  "1",  "spread on, mems on the remote node"},
	{0, "01", "0",  "spread off, mems on both nodes (local allocation)"},
	{1, "01", "01", "spread on, mems on both nodes (even spread)"},
};

static void check_result(const char *expect, const char *desc)
{
	unsigned int i, total = 0, expected_sum = 0, n_expect = 0, unexpected;

	for (i = 0; i < nodes->cnt; i++)
		total += nodes->counters[i];

	if (!total) {
		tst_res(TFAIL, "%s: no pages were allocated", desc);
		return;
	}

	for (i = 0; expect[i]; i++) {
		expected_sum += nodes->counters[expect[i] - '0'];
		n_expect++;
	}

	unexpected = total - expected_sum;

	tst_res(TINFO,
		"%s: unexpected nodes hold %.2f%% of the pages (tolerance %d%%)",
		desc, (double)unexpected * 100 / total, UNEXPECTED_TOLERANCE);

	if (unexpected * 100 > total * UNEXPECTED_TOLERANCE) {
		tst_res(TFAIL, "%s: too many pages on unexpected node(s)", desc);
		return;
	}

	/*
	 * Every expected node must hold at least (100 - BALANCE_TOLERANCE)% of
	 * its even share. This enforces an even spread across several expected
	 * nodes and rejects an expected node that received nothing. A single
	 * expected node always passes this check.
	 */
	for (i = 0; expect[i]; i++) {
		unsigned int idx = expect[i] - '0';

		if (nodes->counters[idx] * n_expect * 100 <
		    expected_sum * (100 - BALANCE_TOLERANCE)) {
			tst_res(TFAIL, "%s: node %u holds too few pages",
				desc, nodes->map[idx]);
			return;
		}
	}

	tst_res(TPASS, "%s", desc);
}

static void run(unsigned int n)
{
	const struct tcase *tc = &tcases[n];
	char mems[64];
	size_t len = 0;
	void *ptr;
	unsigned int i;
	int ret;

	for (i = 0; tc->mems[i]; i++) {
		ret = snprintf(mems + len, sizeof(mems) - len, "%s%u",
			       i ? "," : "", nodes->map[tc->mems[i] - '0']);
		if (ret < 0 || (size_t)ret >= sizeof(mems) - len)
			tst_brk(TBROK, "failed to build cpuset.mems string");
		len += ret;
	}

	SAFE_CG_PRINT(tst_cg, "cpuset.mems", mems);
	SAFE_CG_PRINTF(tst_cg, "cpuset.memory_spread_page", "%d", tc->spread);

	ptr = tse_numa_map(DATAFILE, SIZE);
	tse_numa_fault(ptr, SIZE);
	tse_nodemap_reset_counters(nodes);
	tse_nodemap_count_pages(nodes, ptr, SIZE);
	tse_numa_unmap(ptr, SIZE);

	check_result(tc->expect, tc->desc);
}

static void setup(void)
{
	nodes = tse_get_nodemap(TST_NUMA_MEM, MIN_FREE_KB);
	if (nodes->cnt < 2)
		tst_brk(TCONF, "requires at least 2 NUMA memory nodes");

	if (!SAFE_CG_HAS(tst_cg, "cpuset.memory_spread_page"))
		tst_brk(TCONF, "cpuset.memory_spread_page not available");

	SAFE_CG_PRINTF(tst_cg, "cgroup.procs", "%d", getpid());

	/* Pin the task to the local node for every case. */
	if (numa_run_on_node(nodes->map[0]))
		tst_brk(TCONF, "cannot run on node %u (no CPUs?)",
			nodes->map[0]);
}

static void cleanup(void)
{
	if (nodes)
		tse_nodemap_free(nodes);
}

static struct tst_test test = {
	.needs_root = 1,
	.needs_tmpdir = 1,
	.skip_filesystems = (const char *const []){ "tmpfs", "ramfs", NULL },
	.setup = setup,
	.cleanup = cleanup,
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.needs_cgroup_ctrls = (const char *const []){ "cpuset", NULL },
	.needs_cgroup_ver = TST_CG_V1,
};

#else
TST_TEST_TCONF("test requires libnuma >= 2");
#endif
