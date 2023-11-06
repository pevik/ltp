// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Linux Test Project, 2023
 * Copyright (C) 2023, Red Hat, Inc.
 *
 * Author: David Hildenbrand <david@redhat.com>
 * Port-to-LTP: Li Wang <liwang@redhat.com>
 */

/*\
 * [Description]
 *
 * Migration code will first unmap the old page and replace the present PTE
 * by a migration entry. Then migrate the page. Once that succeeded (and there
 * are no unexpected page references), we replace the migration entries by
 * proper present PTEs pointing at the new page.
 *
 * For ordinary pages we handle PTEs. For 2 MiB hugetlb/THP, it's PMDs.
 * For 1 GiB hugetlb, it's PUDs.
 *
 * So without below commit, GUP-fast code was simply not aware that we could
 * have migration entries stored in PUDs. Migration + GUP-fast code should be
 * able to handle any such races.
 *
 * For example, GUP-fast will re-verify the PUD after pinning to make sure it
 * didn't change. If it did change, it backs off.
 *
 * Migration code should detect the additional page references and back off
 * as well.
 *
 *  commit 15494520b776aa2eadc3e2fefae524764cab9cea
 *  Author: Qiujun Huang <hqjagain@gmail.com>
 *  Date:   Thu Jan 30 22:12:10 2020 -0800
 *
 *      mm: fix gup_pud_range
 *
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <linux/mempolicy.h>
#include <linux/mman.h>

#include "lapi/syscalls.h"
#include "tst_safe_pthread.h"
#include "numa_helper.h"
#include "hugetlb.h"

static char *mem;
static size_t pagesize;
static size_t hugetlbsize;
static volatile int looping = 1;

static void *migration_thread_fn(void *arg LTP_ATTRIBUTE_UNUSED)
{
	while (looping) {
		TST_EXP_PASS_SILENT(syscall(__NR_mbind, mem, hugetlbsize,
			MPOL_LOCAL, NULL, 0x7fful, MPOL_MF_MOVE));
	}

	return NULL;
}

static void run_test(void)
{
	ssize_t transferred;
	struct iovec iov;
	int fds[2];

	pthread_t migration_thread;

	pagesize = getpagesize();
	hugetlbsize = 1 * 1024 * 1024 * 1024u;

	mem = mmap(NULL, hugetlbsize, PROT_READ|PROT_WRITE,
		   MAP_PRIVATE|MAP_ANON|MAP_HUGETLB|MAP_HUGE_1GB,
		   -1, 0);
	if (mem == MAP_FAILED)
		tst_brk(TBROK | TERRNO, "mmap() failed");

	memset(mem, 1, hugetlbsize);

	/* Keep migrating the page around ... */
	SAFE_PTHREAD_CREATE(&migration_thread, NULL, migration_thread_fn, NULL);

	while (looping) {
		SAFE_PIPE(fds);

		iov.iov_base = mem;
		iov.iov_len = pagesize;
		transferred = vmsplice(fds[1], &iov, 1, 0);
		if (transferred <= 0)
			tst_brk(TBROK | TERRNO, "vmsplice() failed");

		SAFE_CLOSE(fds[0]);
		SAFE_CLOSE(fds[1]);

		if (!tst_remaining_runtime()) {
			tst_res(TINFO, "Runtime exhausted, exiting");
			looping = 0;
		}
	}

	SAFE_PTHREAD_JOIN(migration_thread, NULL);

	tst_res(TPASS, "Test completed successfully");
}

static struct tst_test test = {
	.needs_root = 1,
	.test_all = run_test,
	.max_runtime = 60,
	.taint_check = TST_TAINT_W | TST_TAINT_D,
	.hugepages = {2, TST_NEEDS, TST_GIGANTIC},
	.needs_kconfigs = (const char *const[]){
		"CONFIG_NUMA=y",
		NULL
	},
	.tags = (struct tst_tag[]) {
	    {"linux-git", "15494520b776"},
	    {}
	},
};
