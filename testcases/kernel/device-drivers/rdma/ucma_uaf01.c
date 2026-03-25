// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Linux Test Project
 */

/*\
 * Test for use-after-free in RDMA UCMA triggered by concurrent CREATE_ID,
 * BIND_IP, and LISTEN operations via /dev/infiniband/rdma_cm.
 *
 * Requires root to open /dev/infiniband/rdma_cm.
 *
 * Three threads race to create, bind, and listen on RDMA connection manager
 * IDs. On vulnerable kernels, this triggers a use-after-free in
 * cma_listen_on_all() detected by KASAN.
 *
 * Based on a syzbot reproducer:
 * syzbot+db1c219466daac1083df@syzkaller.appspotmail.com
 *
 * Fixed in:
 *
 *  commit 5fe23f262e05
 *  ucma: fix a use-after-free in ucma_resolve_ip()
 */

#include "tst_test.h"
#include "tst_safe_pthread.h"
#include "lapi/rdma_user_cm.h"

#define RDMA_CM_DEV "/dev/infiniband/rdma_cm"

static int cmfd = -1;
static volatile uint32_t shared_id;
static volatile int stop_threads;

static void destroy_id(uint32_t id)
{
	ssize_t ret;

	struct {
		struct rdma_ucm_cmd_hdr hdr;
		struct rdma_ucm_destroy_id destroy;
	} msg = {
		.hdr = {
			.cmd = RDMA_USER_CM_CMD_DESTROY_ID,
			.out = sizeof(struct rdma_ucm_create_id_resp),
		},
		.destroy = {
			.id = id,
		},
	};
	struct rdma_ucm_create_id_resp resp;

	msg.destroy.response = (uintptr_t)&resp;

	/* Errors expected due to racing with stale IDs */
	ret = write(cmfd, &msg, sizeof(msg));
	(void)ret;
}

static void *thread_create(void *arg)
{
	uint32_t id, prev_id = 0;
	int has_prev = 0;

	while (!stop_threads) {
		struct {
			struct rdma_ucm_cmd_hdr hdr;
			struct rdma_ucm_create_id create;
		} msg = {
			.hdr = {
				.cmd = RDMA_USER_CM_CMD_CREATE_ID,
				.out = sizeof(id),
			},
			.create = {
				.response = (uintptr_t)&id,
				.ps = RDMA_PS_IPOIB,
			},
		};

		if (write(cmfd, &msg, sizeof(msg)) > 0) {
			if (has_prev)
				destroy_id(prev_id);
			prev_id = id;
			has_prev = 1;
			shared_id = id;
		}
	}

	if (has_prev)
		destroy_id(prev_id);

	return arg;
}

static void *thread_bind(void *arg)
{
	ssize_t ret;

	while (!stop_threads) {
		struct {
			struct rdma_ucm_cmd_hdr hdr;
			struct rdma_ucm_bind_ip bind;
		} msg = {
			.hdr = {
				.cmd = RDMA_USER_CM_CMD_BIND_IP,
			},
			.bind = {
				.addr = {
					.sin6_family = AF_INET6,
					.sin6_addr = {
						.s6_addr = { 0xff },
					},
				},
				.id = shared_id,
			},
		};

		/* Errors expected due to racing with stale IDs */
		ret = write(cmfd, &msg, sizeof(msg));
		(void)ret;
	}

	return arg;
}

static void *thread_listen(void *arg)
{
	ssize_t ret;

	while (!stop_threads) {
		struct {
			struct rdma_ucm_cmd_hdr hdr;
			struct rdma_ucm_listen listen;
		} msg = {
			.hdr = {
				.cmd = RDMA_USER_CM_CMD_LISTEN,
			},
			.listen = {
				.id = shared_id,
			},
		};

		/* Errors expected due to racing with stale IDs */
		ret = write(cmfd, &msg, sizeof(msg));
		(void)ret;
	}

	return arg;
}

static void setup(void)
{
	cmfd = open(RDMA_CM_DEV, O_WRONLY);
	if (cmfd < 0) {
		if (errno == ENOENT || errno == ENXIO)
			tst_brk(TCONF, RDMA_CM_DEV " not available");
		tst_brk(TBROK | TERRNO, "open(" RDMA_CM_DEV ")");
	}
}

static void cleanup(void)
{
	if (cmfd != -1)
		SAFE_CLOSE(cmfd);
}

static void run(void)
{
	pthread_t threads[3];

	stop_threads = 0;

	SAFE_PTHREAD_CREATE(&threads[0], NULL, thread_create, NULL);
	SAFE_PTHREAD_CREATE(&threads[1], NULL, thread_bind, NULL);
	SAFE_PTHREAD_CREATE(&threads[2], NULL, thread_listen, NULL);

	while (tst_remaining_runtime())
		sleep(1);

	stop_threads = 1;

	SAFE_PTHREAD_JOIN(threads[0], NULL);
	SAFE_PTHREAD_JOIN(threads[1], NULL);
	SAFE_PTHREAD_JOIN(threads[2], NULL);

	if (tst_taint_check())
		tst_res(TFAIL, "Kernel is vulnerable (use-after-free in UCMA)");
	else
		tst_res(TPASS, "No kernel taint detected");
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.runtime = 300,
	.needs_root = 1,
	.taint_check = TST_TAINT_W | TST_TAINT_D,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_INFINIBAND",
		"CONFIG_INFINIBAND_USER_ACCESS",
		NULL
	},
	.tags = (const struct tst_tag[]) {
		{"linux-git", "5fe23f262e05"},
		{}
	},
};
