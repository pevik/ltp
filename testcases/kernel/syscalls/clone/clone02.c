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
#define TRUE 1
#define FALSE 0

static int fd_parent;
static int parent_variable;
static char *cwd_parent;
static int parent_got_signal, child_pid;
static void *child_stack;
static char cwd_child[FILENAME_MAX];

static void sig_child_defined_handler(int pid LTP_ATTRIBUTE_UNUSED)
{
	TEST(tst_syscall(__NR_gettid));
	if (TST_RET == child_pid)
		tst_res(TWARN, "Child got SIGUSR2 signal");
	else
		parent_got_signal = TRUE;
}

/*
 * function to change parent_variable from child's
 * address space. If CLONE_VM flag is set, child shares
 * the memory space with parent so this will be visible
 * to parent also.
 */
static int test_VM(void)
{
	parent_variable = CHILD_VALUE;
	return 0;
}

/*
 * This function closes a file descriptor opened by
 * parent. If CLONE_FILES flag is set, the parent and
 * the child process share the same file descriptor
 * table. so this affects the parent also
 */
static int test_FILES(void)
{
	return close(fd_parent);;
}

/*
 * This function changes the current working directory
 * of the child process. If CLONE_FS flag is set, this
 * will be visible to parent also.
 */
static int test_FS(void)
{
	int rval;

	rval = SAFE_CHDIR(cwd_child);
	return rval;
}

/*
 * This function changes the signal handler for SIGUSR2
 * signal for child. If CLONE_SIGHAND flag is set, this
 * affects parent also.
 */
static int test_SIG(void)
{
	struct sigaction new_act;

	new_act.sa_handler = sig_child_defined_handler;
	new_act.sa_flags = SA_RESTART;
	SAFE_SIGEMPTYSET(&new_act.sa_mask);

	if (SAFE_SIGACTION(SIGUSR2, &new_act, NULL)) {
		return -1;
	}

	return SAFE_KILL(getppid(), SIGUSR2);
}

/*
 * This function is called by parent process to check
 * whether child's modification to parent_variable
 * is visible to parent
 */
static int modified_VM(void)
{
	return parent_variable == CHILD_VALUE;
}

/*
 * This function checks for file descriptor table
 * modifications done by child
 */
static int modified_FILES(void)
{
	char buff[20];
	if (((read(fd_parent, buff, sizeof(buff))) == -1) && (errno == EBADF))
		return 1;

	SAFE_CLOSE(fd_parent);
	return 0;
}

/*
 * This function checks parent's current working directory
 * to see whether its modified by child or not.
 */
static int modified_FS(void)
{
	char cwd[FILENAME_MAX];

	SAFE_GETCWD(cwd, sizeof(cwd));
	return strcmp(cwd, cwd_parent);
}

/*
 * This function checks whether child has changed
 * parent's signal handler for signal, SIGUSR2
 */
static int modified_SIG(void)
{
	return parent_got_signal;
}

static void sig_parent_default_handler(int pid LTP_ATTRIBUTE_UNUSED)
{

}

static int child_fn(void *arg LTP_ATTRIBUTE_UNUSED)
{
	TEST(tst_syscall(__NR_gettid));
	child_pid = TST_RET;

	if (test_VM() == 0 && test_FILES() == 0 && test_FS() == 0 &&
	    test_SIG() == 0)
		_exit(0);
	_exit(1);
}

static int parent_test1(void)
{

	/*
	 * Verify that the parent process resource has changed
	 */
	if (modified_VM() && modified_FILES() && modified_FS() &&
	    modified_SIG())
		return 0;
	return -1;
}

static int parent_test2(void)
{
	/*
	 * Verify that the parent process resource has not changed
	 */
	if (modified_VM() || modified_FILES() || modified_FS() ||
	    modified_SIG())
		return -1;

	return 0;
}

struct tcase {
	unsigned long flags;
	int (*parent_fn) ();
	char *desc;
} tcases[] = {
	{FLAG_ALL, parent_test1, "clone() with all resources shared"},
	{FLAG_NONE, parent_test2, "clone() with all no resources shared"}
};

static void verify_clone(void)
{
	TST_EXP_POSITIVE(ltp_clone(tcases[tst_variant].flags, child_fn, NULL,
				CHILD_STACK_SIZE, child_stack),
				"%s", tcases[tst_variant].desc);

	if (!TST_PASS)
		return;

	tst_reap_children();

	TST_EXP_PASS(tcases[tst_variant].parent_fn(), "%s", tcases[tst_variant].desc);
}


static void cleanup(void)
{
	SAFE_CHDIR(cwd_parent);
	SAFE_UNLINK(TESTFILE);
	SAFE_RMDIR(cwd_child);

	free(cwd_parent);
	free(child_stack);
}

static void setup(void)
{
	struct sigaction def_act;

	/* Save current working directory of parent */
	cwd_parent = tst_get_tmpdir();

	/*
	 * Set value for parent_variable in parent, which will be
	 * changed by child in test_VM(), for testing CLONE_VM flag
	 */
	parent_variable = PARENT_VALUE;

	/*
	 * Open file from parent, which will be closed by
	 * child in test_FILES(), used for testing CLONE_FILES flag
	 */
	fd_parent = SAFE_OPEN(TESTFILE, O_CREAT | O_RDWR, 0777);

	/*
	 * set parent_got_signal to FALSE, used for testing
	 * CLONE_SIGHAND flag
	 */
	parent_got_signal = FALSE;

	def_act.sa_handler = sig_parent_default_handler;
	def_act.sa_flags = SA_RESTART;
	SAFE_SIGEMPTYSET(&def_act.sa_mask);
	SAFE_SIGACTION(SIGUSR2, &def_act, NULL);

	SAFE_MKDIR(TESTDIR, 0777);
	sprintf(cwd_child, "%s/%s", cwd_parent, TESTDIR);
	child_stack = SAFE_MALLOC(CHILD_STACK_SIZE);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_variants = ARRAY_SIZE(tcases),
	.test_all = verify_clone,
	.needs_tmpdir = 1,
};
