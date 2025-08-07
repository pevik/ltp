// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Wei Gao <wegao@suse.com>
 */

/*\
 * [Description]
 *
 * Test PROCMAP_QUERY ioctl() errnos:
 *
 * - EINVAL if q->size is too small
 * - E2BIG if q->size is larger than page size
 * - EINVAL on invalid q->flags
 * - EINVAL if only one of q->vma_name_size and q->vma_name_addr is set
 * - EINVAL if only one of q->build_id_size and q->build_id_addr is set
 * - ENAMETOOLONG if build_id_size or name_buf_size is too small
 */

#include "config.h"
#include <stdlib.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fnmatch.h>
#include "tst_test.h"
#include "tst_safe_stdio.h"
#include <sys/sysmacros.h>
#include <linux/fs.h>
#include "lapi/ioctl.h"

#define PROC_MAP_PATH "/proc/self/maps"

struct procmap_query *q;
static int fd = -1;
static char buf[PATH_MAX];
static char small_buf[1];

static void setup_normal(void);
static void setup_big_size(void);

static struct tcase {
	uint64_t size;
	uint64_t query_addr;
	uint64_t query_flags;
	uint64_t vma_name_addr;
	uint32_t vma_name_size;
	uint64_t build_id_addr;
	uint32_t build_id_size;
	int exp_errno;
	void (*setup)(void);
} tcases[] = {
	{
		.size = 1,
		.exp_errno = EINVAL,
		.setup = setup_normal
	},
	{
		.exp_errno = E2BIG,
		.setup = setup_big_size
	},
	{
		.query_flags = -1,
		.exp_errno = EINVAL,
		.setup = setup_normal
	},
	{
		.vma_name_size = sizeof(buf),
		.exp_errno = EINVAL,
		.setup = setup_normal
	},
	{
		.vma_name_addr = (uint64_t)(unsigned long)buf,
		.exp_errno = EINVAL,
		.setup = setup_normal
	},
	{
		.build_id_size = sizeof(buf),
		.exp_errno = EINVAL,
		.setup = setup_normal
	},
	{
		.build_id_addr = (uint64_t)(unsigned long)buf,
		.exp_errno = EINVAL,
		.setup = setup_normal
	},
	{
		.vma_name_addr = (uint64_t)(unsigned long)small_buf,
		.vma_name_size = sizeof(small_buf),
		.exp_errno = ENAMETOOLONG,
		.setup = setup_normal
	},
	{
		.build_id_addr = (uint64_t)(unsigned long)small_buf,
		.build_id_size = sizeof(small_buf),
		.exp_errno = ENAMETOOLONG,
		.setup = setup_normal
	},
};

static unsigned long get_vm_start(void)
{
	FILE *fp = SAFE_FOPEN(PROC_MAP_PATH, "r");
	char line[1024];
	unsigned long start_addr = 0;

	if (fgets(line, sizeof(line), fp) != NULL) {
		if (sscanf(line, "%lx-", &start_addr) != 1)
			tst_brk(TFAIL, "parse maps file /proc/self/maps failed");
		return start_addr;
	}

	SAFE_FCLOSE(fp);
	tst_brk(TFAIL, "parse maps file /proc/self/maps failed");
}

static void setup_normal(void)
{
	q->size = sizeof(*q);
	q->query_addr = (uint64_t)get_vm_start();
	q->query_flags = 0;
}

static void setup_big_size(void)
{
	setup_normal();
	q->size = getpagesize() + 1;
}

static void run(unsigned int n)
{
	struct tcase *tc = &tcases[n];

	memset(q, 0, sizeof(*q));

	tc->setup();

	if (tc->size != 0)
		q->size = tc->size;
	if (tc->query_flags != 0)
		q->query_flags = tc->query_flags;
	if (tc->vma_name_addr != 0)
		q->vma_name_addr = tc->vma_name_addr;
	if (tc->vma_name_size != 0)
		q->vma_name_size = tc->vma_name_size;
	if (tc->build_id_addr != 0)
		q->build_id_addr = tc->build_id_addr;
	if (tc->build_id_size != 0)
		q->build_id_size = tc->build_id_size;

	TST_EXP_FAIL(ioctl(fd, PROCMAP_QUERY, q), tc->exp_errno);
}

static void setup(void)
{
	struct procmap_query q = {};

	fd = SAFE_OPEN(PROC_MAP_PATH, O_RDONLY);

	if (tst_kvercmp(6, 11, 0) < 0) {
		TEST(ioctl(fd, PROCMAP_QUERY, q));

		if ((TST_RET == -1) && (TST_ERR == ENOTTY))
			tst_brk(TCONF,
				"This system does not provide support for ioctl(PROCMAP_QUERY)");
	}

}

static void cleanup(void)
{
	if (fd != -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.setup = setup,
	.test = run,
	.tcnt = ARRAY_SIZE(tcases),
	.cleanup = cleanup,
	.bufs = (struct tst_buffers []) {
		{&q, .size = sizeof(*q)},
		{}
	}
};
