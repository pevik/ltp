// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 * Copyright (c) 2012 Wanlong Gao <gaowanlong@cn.fujitsu.com>
 */

/*\
 * [Description]
 *   TEST1
 * - call clone() with all resources shared.
 * - child modify the shared resources.
 * - parent wait for child to finish.
 * - verify that the shared resourses are modified.
 * - if the parent shared resource is modified, then pass.
 *
 *	 TEST2
 * - call clone() with no resources shared.
 * - modify the resources in child's address space.
 * - parent wait for child to finish.
 * - verify that the parent's resourses are not modified.
 * - if the parent shared resource are modified, then pass.
 */

#define _GNU_SOURCE

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include "tst_test.h"
#include "lapi/syscalls.h"
#include "clone_platform.h"

#define TESTFILE "testfile"
#define TESTDIR "testdir"
#define FLAG_ALL (CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND|SIGCHLD)
#define FLAG_NONE SIGCHLD
#define PARENT_VALUE 1
#define CHILD_VALUE 2

static int fd_parent;
static int parent_variable;
static char *cwd_parent;
static int parent_got_signal, child_pid;
static void *child_stack;
static char cwd_child[FILENAME_MAX];

#define CLONE_DESC(x) [x] = #x

static char *res_name[] = {
	CLONE_DESC(CLONE_VM),
	CLONE_DESC(CLONE_FS),
	CLONE_DESC(CLONE_FILES),
	CLONE_DESC(CLONE_SIGHAND),
};

static void sig_child_defined_handler(int pid LTP_ATTRIBUTE_UNUSED)
{
	TEST(tst_syscall(__NR_gettid));

	if (TST_RET == child_pid)
		tst_res(TWARN, "Child got SIGUSR2 signal");
	else
		parent_got_signal = 1;
}

static void modify_resources(void)
{
	struct sigaction new_act;

	parent_variable = CHILD_VALUE;

	close(fd_parent);

	SAFE_CHDIR(cwd_child);

	new_act.sa_handler = sig_child_defined_handler;
	new_act.sa_flags = SA_RESTART;
	SAFE_SIGEMPTYSET(&new_act.sa_mask);

	SAFE_SIGACTION(SIGUSR2, &new_act, NULL);
	SAFE_KILL(getppid(), SIGUSR2);
}

static int verify_resources(void)
{
	char buff[20];
	char cwd[FILENAME_MAX];

	unsigned int changed = 0;

	if (parent_variable == CHILD_VALUE)
		changed |= CLONE_VM;

	if (((read(fd_parent, buff, sizeof(buff))) == -1) && (errno == EBADF))
		changed |= CLONE_FS;
	else
		SAFE_CLOSE(fd_parent);

	SAFE_GETCWD(cwd, sizeof(cwd));
	if (strcmp(cwd, cwd_parent))
		changed |= CLONE_FILES;

	if (parent_got_signal)
		changed |= CLONE_SIGHAND;

	return changed;
}

static void sig_parent_default_handler(int pid LTP_ATTRIBUTE_UNUSED)
{

}

static int child_fn(void *arg LTP_ATTRIBUTE_UNUSED)
{
	TEST(tst_syscall(__NR_gettid));
	child_pid = TST_RET;

	modify_resources();
	_exit(0);
}

static void reset_resources(void)
{
	struct sigaction def_act;

	parent_variable = PARENT_VALUE;
	fd_parent = SAFE_OPEN(TESTFILE, O_CREAT | O_RDWR, 0777);
	parent_got_signal = 0;
	SAFE_CHDIR(cwd_parent);

	def_act.sa_handler = sig_parent_default_handler;
	def_act.sa_flags = SA_RESTART;
	SAFE_SIGEMPTYSET(&def_act.sa_mask);
	SAFE_SIGACTION(SIGUSR2, &def_act, NULL);
}

struct tcase {
	unsigned long flags;
	char *desc;
} tcases[] = {
	{FLAG_ALL, "clone() with all resources shared"},
	{FLAG_NONE, "clone() with all no resources shared"}
};

static void verify_clone(unsigned int tc)
{
	unsigned int flag;
	unsigned int i;

	tst_res(TINFO, "%s", tcases[tc].desc);

	reset_resources();

	TST_EXP_PID_SILENT(ltp_clone(tcases[tc].flags, child_fn, NULL,
				CHILD_STACK_SIZE, child_stack));

	if (!TST_PASS)
		return;

	tst_reap_children();

	flag = verify_resources();

	for (i = CLONE_VM; i <= CLONE_SIGHAND; i <<= 1) {
		if (tcases[tc].flags == FLAG_ALL) {
			if (flag & i)
				tst_res(TPASS, "The resource %s of the parent process has changed", res_name[i >> 8]);
			else
				tst_res(TFAIL, "The resource %s of the parent process has not changed", res_name[i >> 8]);
		} else {
			if (flag & i)
				tst_res(TFAIL, "The resource %s of the parent process has changed", res_name[i >> 8]);
			else
				tst_res(TPASS, "The resource %s of the parent process has not changed", res_name[i >> 8]);
		}
	}
}

static void cleanup(void)
{
	SAFE_CHDIR(cwd_parent);
	SAFE_UNLINK(TESTFILE);
	free(cwd_parent);
}

static void setup(void)
{
	struct sigaction def_act;

	/* Save current working directory of parent */
	cwd_parent = tst_get_tmpdir();

	/*
	 * Set value for parent_variable in parent, which will be
	 * changed by child for testing CLONE_VM flag
	 */
	parent_variable = PARENT_VALUE;

	/*
	 * Open file from parent, which will be closed by
	 * child, used for testing CLONE_FILES flag
	 */
	fd_parent = SAFE_OPEN(TESTFILE, O_CREAT | O_RDWR, 0777);

	/*
	 * set parent_got_signal to 0, used for testing
	 * CLONE_SIGHAND flag
	 */
	parent_got_signal = 0;

	def_act.sa_handler = sig_parent_default_handler;
	def_act.sa_flags = SA_RESTART;
	SAFE_SIGEMPTYSET(&def_act.sa_mask);
	SAFE_SIGACTION(SIGUSR2, &def_act, NULL);

	SAFE_MKDIR(TESTDIR, 0777);
	sprintf(cwd_child, "%s/%s", cwd_parent, TESTDIR);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.tcnt = ARRAY_SIZE(tcases),
	.test = verify_clone,
	.bufs = (struct tst_buffers []) {
		{&child_stack, .size = CHILD_STACK_SIZE},
	},
	.needs_tmpdir = 1,
};
