// SPDX-License-Identifier: GPL-2.0-or-later
/* Copyright (c) International Business Machines  Corp., 2001 */

/*
 * NAME
 *	semop05.c
 *
 * DESCRIPTION
 *	semop05 - test for EINTR and EIDRM errors
 *
 * ALGORITHM
 *	create a semaphore set with read and alter permissions
 *	loop if that option was specified
 *	set up the s_buf buffer
 *	initialize the primitive semaphores
 *	fork a child process
 *	child calls semop() and sleeps
 *	parent either removes the semaphore set or sends a signal to the child
 *	parent then exits
 *	child gets a return from the semop() call
 *	check the errno value
 *	  issue a PASS message if we get EINTR or EIDRM
 *	otherwise, the tests fails
 *	  issue a FAIL message
 *	call cleanup
 *
 * USAGE:  <for command-line>
 *  semop05 [-c n] [-e] [-i n] [-I x] [-P x] [-t]
 *     where,  -c n : Run n copies concurrently.
 *             -e   : Turn on errno logging.
 *	       -i n : Execute test n times.
 *	       -I x : Execute test for x seconds.
 *	       -P x : Pause for x seconds between iterations.
 *	       -t   : Turn on syscall timing.
 *
 * HISTORY
 *	03/2001 - Written by Wayne Boyer
 *      14/03/2008 Matthieu Fertré (Matthieu.Fertre@irisa.fr)
 *      - Fix concurrency issue. Due to the use of usleep function to
 *        synchronize processes, synchronization issues can occur on a loaded
 *        system. Fix this by using pipes to synchronize processes.
 *
 * RESTRICTIONS
 *	none
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include "tst_test.h"
#include "libnewipc.h"
#include "lapi/semun.h"

static key_t semkey;
static int sem_id = -1;
static struct sembuf s_buf;

struct test_case_t {
	union semun semunptr;
	short op;
	short flg;
	short num;
	int error;
} tc[] = {
	/* EIRDM sem_op = 0 */
	{ {
	1}, 0, 0, 2, EIDRM},
	    /* EIRDM sem_op = -1 */
	{ {
	0}, -1, 0, 3, EIDRM},
	    /* EINTR sem_op = 0 */
	{ {
	1}, 0, 0, 4, EINTR},
	    /* EINTR sem_op = -1 */
	{ {
	0}, -1, 0, 5, EINTR}
};

#ifdef UCLINUX
#define PIPE_NAME	"semop05"
static void do_child_uclinux();
static int i_uclinux;
#endif

static inline int process_state_wait2(pid_t pid, const char state)
{
	char proc_path[128], cur_state;

	snprintf(proc_path, sizeof(proc_path), "/proc/%i/stat", pid);

	for (;;) {
		FILE *f = fopen(proc_path, "r");
		if (!f) {
			tst_res(TFAIL, "Failed to open '%s': %s\n", proc_path,
				strerror(errno));
			return 1;
		}

		if (fscanf(f, "%*i %*s %c", &cur_state) != 1) {
			fclose(f);
			tst_res(TFAIL, "Failed to read '%s': %s\n", proc_path,
				strerror(errno));
			return 1;
		}
		fclose(f);

		if (state == cur_state)
			return 0;

		usleep(10000);
	}
}

static void do_child(int i)
{
	TEST(semop(sem_id, &s_buf, 1));
	if (TST_RET != -1) {
		tst_res(TFAIL, "call succeeded when error expected");
		exit(-1);
	}

	if (TST_ERR == tc[i].error)
		tst_res(TPASS | TTERRNO, "expected failure");
	else
		tst_res(TFAIL | TTERRNO, "unexpected failure");

	exit(0);
}

static void sighandler(int sig)
{
	if (sig != SIGHUP)
		tst_brk(TBROK, "unexpected signal %d received", sig);
}

static void setup(void)
{
	SAFE_SIGNAL(SIGHUP, sighandler);

	/* get an IPC resource key */
	semkey = GETIPCKEY();

	/*
	 * create a semaphore set with read and alter permissions and PSEMS
	 * "primitive" semaphores.
	 */
	if ((sem_id = semget(semkey, PSEMS, IPC_CREAT | IPC_EXCL | SEM_RA)) ==
	    -1)
		tst_brk(TBROK | TERRNO, "couldn't create semaphore in setup");
}

static void cleanup(void)
{
	union semun arr;

	if (sem_id != -1) {
		if (semctl(sem_id, 0, IPC_RMID, arr) == -1)
			tst_res(TINFO, "WARNING: semaphore deletion failed.");
	}
}

static void run(unsigned int i)
{
	pid_t pid;

#ifdef UCLINUX
	maybe_run_child(&do_child_uclinux, "dd", &i_uclinux, &sem_id);
#endif
	/* initialize the s_buf buffer */
	s_buf.sem_op = tc[i].op;
	s_buf.sem_flg = tc[i].flg;
	s_buf.sem_num = tc[i].num;

	/* initialize all of the primitive semaphores */
	if (semctl(sem_id, tc[i].num, SETVAL, tc[i].semunptr) == -1)
		tst_brk(TBROK | TERRNO, "semctl() failed");

	pid = SAFE_FORK();

	if (pid == 0) {	/* child */
#ifdef UCLINUX
		if (self_exec(av[0], "dd", i, sem_id) < 0)
			tst_brk(TBROK, "could not self_exec");
#else
		do_child(i);
#endif
	} else {
		process_state_wait2(pid, 'S');

		/*
		 * If we are testing for EIDRM then remove
		 * the semaphore, else send a signal that
		 * must be caught as we are testing for
		 * EINTR.
		 */
		if (tc[i].error == EIDRM) {
			/* remove the semaphore resource */
			cleanup();
		} else {
			SAFE_KILL(pid, SIGHUP);
		}

		/* let the child carry on */
		waitpid(pid, NULL, 0);
	}

	/*
	 * recreate the semaphore resource if needed
	 */
	if (tc[i].error == EINTR)
		return;

	if ((sem_id = semget(semkey, PSEMS, IPC_CREAT | IPC_EXCL | SEM_RA)) ==
	    -1)
		tst_brk(TBROK | TERRNO, "couldn't recreate semaphore");
}

#ifdef UCLINUX
/*
 * do_child_uclinux() - capture signals, re-initialize s_buf then call do_child
 *                      with the appropriate argument
 */
static void do_child_uclinux(void)
{
	int i = i_uclinux;

	/* initialize the s_buf buffer */
	s_buf.sem_op = tc[i].op;
	s_buf.sem_flg = tc[i].flg;
	s_buf.sem_num = tc[i].num;

	do_child(i);
}
#endif

static struct tst_test test = {
	.test = run,
	.tcnt = ARRAY_SIZE(tc),
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.forks_child = 1,
};
