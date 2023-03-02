// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * Copyright (C) 2023 Linux Test Project, Inc.
 */

/*\
 * [Description]
 *
 * Creates a semaphore and two processes.  The processes
 * each go through a loop where they semdown, delay for a
 * random amount of time, and semup, so they will almost
 * always be fighting for control of the semaphore.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include "lapi/sem.h"
#include "tst_test.h"
#include "tst_safe_sysv_ipc.h"

static char *verbose;
static int loops = 100;
static char *opt_loops_str;

static void semup(int semid)
{
	struct sembuf semops;

	semops.sem_num = 0;

	semops.sem_op = 1;

	semops.sem_flg = SEM_UNDO;

	SAFE_SEMOP(semid, &semops, 1);
}

static void semdown(int semid)
{
	struct sembuf semops;

	semops.sem_num = 0;

	semops.sem_op = -1;

	semops.sem_flg = SEM_UNDO;

	SAFE_SEMOP(semid, &semops, 1);
}

static void delayloop(void)
{
	int delay;

	delay = 1 + ((100.0 * rand()) / RAND_MAX);

	if (verbose)
		tst_res(TINFO, "in delay function for %d microseconds", delay);
	usleep(delay);
}

void mainloop(int semid)
{
	int i;

	for (i = 0; i < loops; i++) {
		semdown(semid);
		if (verbose)
			tst_res(TINFO, "Sem is down");
		delayloop();
		semup(semid);
		if (verbose)
			tst_res(TINFO, "Sem is up");
	}
}

static void run(void)
{
	int semid;
	union semun semunion;
	pid_t pid;
	int chstat;

	/* set up the semaphore */
	semid = SAFE_SEMGET((key_t) 9142, 1, 0666 | IPC_CREAT);
	if (semid < 0)
		tst_brk(TBROK, "Error in semget id=%i", semid);

	semunion.val = 1;

	SAFE_SEMCTL(semid, 0, SETVAL, semunion);

	pid = SAFE_FORK();
	if (pid < 0)
		tst_brk(TBROK, "Fork failed pid %i", pid);

	if (pid) {
		/* parent */
		srand(pid);
		mainloop(semid);
		waitpid(pid, &chstat, 0);
		if (!WIFEXITED(chstat)) {
			printf("child exited with status\n");
			exit(-1);
		}
		SAFE_SEMCTL(semid, 0, IPC_RMID, semunion);
		tst_res(TPASS, "Semaphore up/down check success");
	} else {
		/* child */
		mainloop(semid);
	}
}

static void setup(void)
{
	if (opt_loops_str)
		loops = SAFE_STRTOL(opt_loops_str, 1, INT_MAX);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.forks_child = 1,
	.options = (struct tst_option[]) {
		{"l:", &opt_loops_str, "Internal loops for semup/down"},
		{"v", &verbose,
			"Print information about successful semop."},
		{}
	},
	.needs_root = 1,
};
