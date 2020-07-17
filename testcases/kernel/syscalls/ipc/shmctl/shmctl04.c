// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2020 Cyril Hrubis <chrubis@suse.cz>
 */
/*
 * Call shmctl() with SHM_INFO flag and check that:
 *
 * * The returned index points to a valid SHM by calling SHM_STAT_ANY
 * * And the data are consistent with /proc/sysvipc/shm
 *
 * There is a possible race between the call to the shmctl() and read from the
 * proc file so this test cannot be run in parallel with any IPC testcases that
 * adds or removes SHM segments.
 *
 * Note what we create a SHM segment in the test setup to make sure that there
 * is at least one during the testrun.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include "tst_test.h"
#include "tst_safe_sysv_ipc.h"
#include "libnewipc.h"

#define SHM_SIZE 2048

static int shm_id = -1;

static void parse_proc_sysvipc(struct shm_info *info, int max_id)
{
	int page_size = getpagesize();
	FILE *f = fopen("/proc/sysvipc/shm", "r");
	int used_ids = 0;
	int shmid_max = 0;
	unsigned long shm_rss = 0;
	unsigned long shm_swp = 0;
	unsigned long shm_tot = 0;

	/* Eat header */
	for (;;) {
		int c = fgetc(f);

		if (c == '\n' || c == EOF)
			break;
	}

	int shmid, size, rss, swap;

	/*
	 * Sum rss, swap and size for all elements listed, which should equal
	 * the data returned in the shm_info structure.
	 *
	 * Note that the size has to be rounded up to nearest multiple of page
	 * size.
	 */
	while (fscanf(f, "%*i %i %*i %i %*i %*i %*i %*i %*i %*i %*i %*i %*i %*i %i %i",
	              &shmid, &size, &rss, &swap) > 0) {
		used_ids++;
		shm_rss += rss/page_size;
		shm_swp += swap/page_size;
		shm_tot += (size + page_size - 1) / page_size;
		if (shmid > shmid_max)
			shmid_max = shmid;
	}

	if (info->used_ids != used_ids) {
		tst_res(TFAIL, "used_ids = %i, expected %i",
		        info->used_ids, used_ids);
	} else {
		tst_res(TPASS, "used_ids = %i", used_ids);
	}

	if (info->shm_rss != shm_rss) {
		tst_res(TFAIL, "shm_rss = %li, expected %li",
		        info->shm_rss, shm_rss);
	} else {
		tst_res(TPASS, "shm_rss = %li", shm_rss);
	}

	if (info->shm_swp != shm_swp) {
		tst_res(TFAIL, "shm_swp = %li, expected %li",
		        info->shm_swp, shm_swp);
	} else {
		tst_res(TPASS, "shm_swp = %li", shm_swp);
	}

	if (info->shm_tot != shm_tot) {
		tst_res(TFAIL, "shm_tot = %li, expected %li",
		        info->shm_tot, shm_tot);
	} else {
		tst_res(TPASS, "shm_tot = %li", shm_tot);
	}

	if (max_id != shmid_max) {
		tst_res(TFAIL, "highest shmid = %i, expected %i",
		        max_id, shmid_max);
	} else {
		tst_res(TPASS, "highest shmid = %i", max_id);
	}
}

static void verify_shminfo(void)
{
	struct shm_info info;
	struct shmid_ds ds;

	TEST(shmctl(0, SHM_INFO, (struct shmid_ds *)&info));

	if (TST_RET == -1) {
		tst_res(TFAIL | TTERRNO, "shmctl(0, SHM_INFO, ...)");
		return;
	}

	TEST(shmctl(TST_RET, SHM_STAT_ANY, &ds));

	if (TST_RET == -1)
		tst_res(TFAIL | TTERRNO, "SHM_INFO haven't returned a valid index");
	else
		tst_res(TPASS, "SHM_INFO returned valid index %li", TST_RET);

	parse_proc_sysvipc(&info, TST_RET);
}

static void setup(void)
{
	shm_id = SAFE_SHMGET(IPC_PRIVATE, SHM_SIZE, IPC_CREAT | SHM_RW);
}

static void cleanup(void)
{
	if (shm_id >= 0)
		SAFE_SHMCTL(shm_id, IPC_RMID, NULL);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = verify_shminfo,
};
