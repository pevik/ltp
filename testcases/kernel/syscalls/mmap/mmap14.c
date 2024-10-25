// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2013 FNST, DAN LI <li.dan@cn.fujitsu.com>
 *
 */

/*\
 * [Description]
 *
 * Test Description:
 *  Verify MAP_LOCKED works fine.
 *  "Lock the pages of the mapped region into memory in the manner of mlock(2)."
 *
 * Expected Result:
 *  mmap() should succeed returning the address of the mapped region,
 *  and this region should be locked into memory.
 */
#include <stdio.h>
#include <sys/mman.h>

#include "tst_test.h"

#define TEMPFILE        "mmapfile"
#define MMAPSIZE        (1UL<<20)
#define LINELEN         256

static char *addr;

static void getvmlck(unsigned int *lock_sz);

static void run(unsigned int n)
{
	unsigned int sz_before;
	unsigned int sz_after;
	unsigned int sz_ch;

	getvmlck(&sz_before);

	addr = mmap(NULL, MMAPSIZE, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_LOCKED | MAP_ANONYMOUS,
			-1, 0);

	if (addr == MAP_FAILED)
		tst_res(TFAIL | TERRNO, "mmap of %s failed", TEMPFILE);

	getvmlck(&sz_after);

	sz_ch = sz_after - sz_before;
	if (sz_ch == MMAPSIZE / 1024) {
		tst_res(TPASS, "Functionality of mmap() "
				"successful");
	} else {
		tst_res(TFAIL, "Expected %luK locked, "
				"get %uK locked",
				MMAPSIZE / 1024, sz_ch);
	}

	if (munmap(addr, MMAPSIZE) != 0)
		tst_res(TFAIL | TERRNO, "munmap failed");
}

void getvmlck(unsigned int *lock_sz)
{
	int ret;
	char line[LINELEN];
	FILE *fstatus = NULL;

	fstatus = fopen("/proc/self/status", "r");
	if (fstatus == NULL)
		tst_res(TFAIL | TERRNO, "Open dev status failed");

	while (fgets(line, LINELEN, fstatus) != NULL)
		if (strstr(line, "VmLck") != NULL)
			break;

	ret = sscanf(line, "%*[^0-9]%d%*[^0-9]", lock_sz);
	if (ret != 1)
		tst_res(TFAIL | TERRNO, "Get lock size failed");

	fclose(fstatus);
}

static struct tst_test test = {
	.needs_root = 1,
	.test = run,
	.tcnt = 1,
};
