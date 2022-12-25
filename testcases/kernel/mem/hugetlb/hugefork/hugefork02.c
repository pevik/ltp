// SPDX-License-Identifier: LGPL-2.1-or-later
/*
 * Copyright (C) 2005-2006 David Gibson & Adam Litke, IBM Corporation.
 * Author: David Gibson & Adam Litke
 */

/*\
 * [Description]
 *
 * Test shared memory behavior when multiple threads are attached to a
 * segment. A segment is created and then children are spawned which
 * attach, write, read (verify), and detach from the shared memory segment.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <sys/mount.h>
#include <limits.h>
#include <sys/param.h>
#include <setjmp.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/shm.h>

#include "tst_safe_sysv_ipc.h"
#include "hugetlb.h"

static int shmid = -1;

#define NR_HUGEPAGES 5
#define NUMPROCS 20
#define MNTPOINT "hugetlbfs/"
#define MAX_PROCS 200
#define BUF_SZ 256

static long hpage_size;

static void do_child(int thread, unsigned long size)
{
	volatile char *shmaddr;
	int j;
	unsigned long k;

	for (j = 0; j < 5; j++) {
		shmaddr = SAFE_SHMAT(shmid, 0, SHM_RND);

		for (k = 0; k < size; k++)
			shmaddr[k] = (char) (k);
		for (k = 0; k < size; k++)
			if (shmaddr[k] != (char)k) {
				tst_res(TFAIL, "Thread %d, Offset %lu mismatch", thread, k);
				goto cleanup;
			}

		SAFE_SHMDT((const void *)shmaddr);
	}
cleanup:
	exit(0);
}

static void run_test(void)
{
	unsigned long size;
	int pid;
	int i;

	size = hpage_size * NR_HUGEPAGES;
	shmid = SAFE_SHMGET(2, size, SHM_HUGETLB|IPC_CREAT|SHM_R|SHM_W);

	for (i = 0; i < NUMPROCS; i++) {
		pid = SAFE_FORK();

		if (pid == 0)
			do_child(i, size);
	}

	tst_reap_children();
	tst_res(TPASS, "Successfully tested shared hugetlb memory with multiple procs");
}

static void setup(void)
{
	hpage_size = SAFE_READ_MEMINFO(MEMINFO_HPAGE_SIZE)*1024;
}

static void cleanup(void)
{
	if (shmid >= 0)
		SAFE_SHMCTL(shmid, IPC_RMID, NULL);
}

static struct tst_test test = {
	.needs_root = 1,
	.mntpoint = MNTPOINT,
	.needs_hugetlbfs = 1,
	.needs_tmpdir = 1,
	.forks_child = 1,
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run_test,
	.hugepages = {NR_HUGEPAGES, TST_NEEDS},
};
