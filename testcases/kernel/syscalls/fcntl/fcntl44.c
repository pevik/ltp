/*
 * [Description]
 * Tests killing a child while several other processes using different OFD
 * lock contexts are blocking. When the child is killed there is a cleanup
 * routine going on. The parent will check if there were any unexpected
 * side-effects, e.g. unlock all previous acquired locks, happened.
 *
 * It is recommended to run watch -n 0.1 "dlm_tool plocks $LS"
 * aside to monitor dlm plock handling.
 *
 * [How to use it]
 * Call it with TMPDIR=/mnt ./fcntl44 where TMPDIR is a gfs2 mountpoint.
 * Try it on other filesystems to compare results.
 *
 * [0] https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/fs/dlm/plock.c?h=v6.3#n432
 */

#include <sys/wait.h>

#include "tst_test.h"

static int fd, fd2;

void do_child1(void)
{
	const struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 0L,
		.l_len = 1L,
	};

	SAFE_FCNTL(fd2, F_OFD_SETLKW, &fl);
	tst_res(TINFO, "child1 lock region 0-0 acquired");

	TST_CHECKPOINT_WAIT(1);

	tst_res(TPASS, "Child1 passed!");
}

void do_child2(void)
{
	const struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 1L,
		.l_len = 1L,
	};

	SAFE_FCNTL(fd2, F_OFD_SETLKW, &fl);
}

static void fcntl40_test(void)
{
	struct flock fl = {
		.l_type = F_WRLCK,
		.l_whence = SEEK_SET,
		.l_start = 0L,
		.l_len = 2L,
	};
	pid_t pid, pid2;
	int rv;

	SAFE_FCNTL(fd, F_SETLK, &fl);
	tst_res(TINFO, "parent lock 0-1");

	pid = SAFE_FORK();
	if (pid == 0) {
		do_child1();
		return;
	}

	pid2 = SAFE_FORK();
	if (pid2 == 0) {
		do_child2();
		return;
	}

	/* wait until childs wait to acquire */
	sleep(2);

	kill(pid2, SIGKILL);
	/* wait until linux killed the process */
	sleep(3);
	tst_res(TPASS, "Child2 killed!");

	fl.l_type = F_UNLCK;
	SAFE_FCNTL(fd, F_SETLK, &fl);

	/* let child1 acquire 0-0 */
	sleep(2);

	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_len = 1;
	rv = fcntl(fd, F_OFD_SETLK, &fl);
	if (rv == -1 && errno == EAGAIN)
		tst_res(TPASS, "region 1-1 was locked");
	else
		tst_res(TFAIL, "region 1-1 was unlocked");

	TST_CHECKPOINT_WAKE(1);

	/* due bug child1 does not return because child2 killing removed waiters */
	wait(NULL);

	tst_res(TPASS, "Parent passed!");
}

static void setup(void)
{
	fd = SAFE_OPEN("filename", O_RDWR | O_CREAT, 0700);
	fd2 = SAFE_OPEN("filename", O_RDWR | O_CREAT, 0700);
}

static void cleanup(void)
{
	if (fd > -1)
		SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.forks_child = 1,
	.needs_checkpoints = 1,
	.test_all = fcntl40_test,
	.setup = setup,
	.cleanup = cleanup,
};
