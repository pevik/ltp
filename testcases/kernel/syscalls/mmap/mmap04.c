// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 *  07/2001 Ported by Wayne Boyer
 * Copyright (c) 2023 SUSE LLC Avinesh Kumar <avinesh.kumar@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that, after a successful mmap() call, permission bits of the new
 * mapping in /proc/pid/maps file matches the prot and flags arguments in
 * mmap() call.
 */

#include <inttypes.h>
#include "tst_test.h"
#include <stdio.h>

static char *addr1;
static char *addr2;

static struct tcase {
	int prot;
	int flags;
	char *exp_perms;
} tcases[] = {
	{PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, "---p"},
	{PROT_NONE, MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED, "---s"},
	{PROT_READ, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, "r--p"},
	{PROT_READ, MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED, "r--s"},
	{PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, "-w-p"},
	{PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED, "-w-s"},
	{PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, "rw-p"},
	{PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED, "rw-s"},
	{PROT_READ | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, "r-xp"},
	{PROT_READ | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED, "r-xs"},
	{PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, "-wxp"},
	{PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED, "-wxs"},
	{PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, "rwxp"},
	{PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED | MAP_FIXED, "rwxs"}
};

static void run(unsigned int i)
{
	struct tcase *tc = &tcases[i];
	char addr_str[20];
	char perms[8];
	char fmt[1024];
	unsigned int pagesize;

	pagesize = SAFE_SYSCONF(_SC_PAGESIZE);

	/* To avoid new mapping getting merged with existing mappings, we first
	   create a 2-page mapping with the different permissions, and then remap
	   the 2nd page with the perms being tested. */
	if ((tc->prot == PROT_NONE) && (tc->flags == (MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED)))
		addr1 = SAFE_MMAP(NULL, pagesize * 2, PROT_READ, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
	else
		addr1 = SAFE_MMAP(NULL, pagesize * 2, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

	addr2 = SAFE_MMAP(addr1 + pagesize, pagesize, tc->prot, tc->flags, -1, 0);

	sprintf(addr_str, "%" PRIxPTR, (uintptr_t)addr2);
	sprintf(fmt, "%s-%%*x %%s", addr_str);
	SAFE_FILE_LINES_SCANF("/proc/self/maps", fmt, perms);

	if (!strcmp(perms, tc->exp_perms)) {
		tst_res(TPASS, "mapping permissions in /proc matched: %s", perms);
	} else {
		tst_res(TFAIL, "mapping permissions in /proc mismatched, expected: %s, found: %s",
						tc->exp_perms, perms);
	}

	SAFE_MUNMAP(addr1, pagesize * 2);
}

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
};
