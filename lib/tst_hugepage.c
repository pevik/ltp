// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2019 Red Hat, Inc.
 */

#define TST_NO_DEFAULT_MAIN

#include <stdio.h>
#include "tst_test.h"
#include "tst_hugepage.h"

unsigned long tst_hugepages;
char *nr_opt;
char *Hopt;

size_t tst_get_hugepage_size(void)
{
	if (access(PATH_HUGEPAGES, F_OK))
		return 0;

	return SAFE_READ_MEMINFO("Hugepagesize:") * 1024;
}

size_t tst_get_gigantic_size(void)
{
	DIR *dir;
	struct dirent *ent;
	unsigned long max, g_pgsz;

	max = tst_get_hugepage_size() / 1024;

	/*
	 * Scanning the largest hugepage sisze, for example aarch64 configuration:
	 * hugepages-1048576kB hugepages-32768kB hugepages-2048kB hugepages-64kB
	 */
	dir = SAFE_OPENDIR(PATH_HUGEPAGES);
	while ((ent = SAFE_READDIR(dir)) != NULL) {
		if (sscanf(ent->d_name, "hugepages-%lukB", &g_pgsz)
				&& (g_pgsz > max))
			max = g_pgsz;
	}

	SAFE_CLOSEDIR(dir);
	return max * 1024;
}

unsigned long tst_reserve_hugepages(struct tst_hugepage *hp)
{
	unsigned long val, max_hpages, hpsize;
	char hugepage_path[PATH_MAX];
	struct tst_path_val pvl = {
		.path = hugepage_path,
		.val = NULL,
		.flags = TST_SR_SKIP_MISSING | TST_SR_TCONF_RO
	};

	if (access(PATH_HUGEPAGES, F_OK)) {
		if (hp->policy == TST_NEEDS)
			tst_brk(TCONF, "hugetlbfs is not supported");
		tst_hugepages = 0;
		goto out;
	}

	if (nr_opt)
		tst_hugepages = SAFE_STRTOL(nr_opt, 1, LONG_MAX);
	else
		tst_hugepages = hp->number;

	if (hp->hptype == TST_GIGANTIC)
		hpsize = tst_get_gigantic_size() / 1024;
	else
		hpsize = tst_get_hugepage_size() / 1024;

	sprintf(hugepage_path, PATH_HUGEPAGES"/hugepages-%lukB/nr_hugepages", hpsize);
	if (access(hugepage_path, F_OK)) {
		if (hp->policy == TST_NEEDS)
			tst_brk(TCONF, "Hugepage size %lu not supported", hpsize);
		tst_hugepages = 0;
		goto out;
	}

	if (hp->number == TST_NO_HUGEPAGES) {
		tst_hugepages = 0;
		goto set_hugepages;
	}

	SAFE_FILE_PRINTF("/proc/sys/vm/drop_caches", "3");
	SAFE_FILE_PRINTF("/proc/sys/vm/compact_memory", "1");
	if (hp->policy == TST_NEEDS) {
		/*
		 * In case of the gigantic page configured as the default hugepage size,
		 * we have to garantee the TST_NEEDS take effect.
		 */
		if (tst_get_gigantic_size() != tst_get_hugepage_size())
			goto set_hugepages;

		tst_hugepages += SAFE_READ_MEMINFO("HugePages_Total:");
		goto set_hugepages;
	}

	max_hpages = SAFE_READ_MEMINFO("MemFree:") / hpsize;
	if (tst_hugepages > max_hpages) {
		tst_res(TINFO, "Requested number(%lu) of hugepages is too large, "
				"limiting to 80%% of the max hugepage count %lu",
				tst_hugepages, max_hpages);
		tst_hugepages = max_hpages * 0.8;

		if (tst_hugepages < 1)
			goto out;
	}

set_hugepages:
	tst_sys_conf_save(&pvl);

	SAFE_FILE_PRINTF(hugepage_path, "%lu", tst_hugepages);
	SAFE_FILE_SCANF(hugepage_path, "%lu", &val);

	if (val != tst_hugepages)
		tst_brk(TCONF, "nr_hugepages = %lu, but expect %lu. "
				"Not enough hugepages for testing.",
				val, tst_hugepages);

	if (hp->policy == TST_NEEDS) {
		unsigned long free_hpages;
		sprintf(hugepage_path, PATH_HUGEPAGES"/hugepages-%lukB/free_hugepages", hpsize);
		SAFE_FILE_SCANF(hugepage_path, "%lu", &free_hpages);
		if (hp->number > free_hpages)
			tst_brk(TCONF, "free_hpages = %lu, but expect %lu. "
				"Not enough hugepages for testing.",
				free_hpages, hp->number);
	}

	tst_res(TINFO, "%lu (%luMB) hugepage(s) reserved", tst_hugepages, hpsize/1024);
out:
	return tst_hugepages;
}
