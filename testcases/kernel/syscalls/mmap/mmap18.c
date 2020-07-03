// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Zilogic Systems Pvt. Ltd., 2020
 * Email: code@zilogic.com
 */

/*
 * Test mmap() MAP_GROWSDOWN flag
 *
 * We assign the memory region allocated using MAP_GROWSDOWN to a thread,
 * as a stack, to test the effect of MAP_GROWSDOWN. This is because the
 * kernel only grows the memory region when the stack pointer, is within
 * guard page, when the guard page is touched.
 *
 * 1. Map an anyonymous memory region of size X, and unmap it.
 * 2. Split the unmapped memory region into two.
 * 3. The lower memory region is left unmapped.
 * 4. The higher memory region is mapped for use as stack, using
 *    MAP_FIXED | MAP_GROWSDOWN.
 * 5. The higher memory region is provided as stack to a thread, where
 *    a recursive function is invoked.
 * 6. The stack grows beyond the allocated region, into the lower memory area.
 * 7. If this results in the memory region being extended, into the
 *    unmapped region, the test is considered to have passed.
 */

#include <unistd.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdbool.h>

#include "tst_test.h"
#include "tst_safe_pthread.h"

#define UNITS(x) ((x) * PTHREAD_STACK_MIN)

static void *stack;

static bool check_stackgrow_up(int *local_var_1)
{
	int local_var_2;

	return !(local_var_1 < &local_var_2);
}

static void setup(void)
{
	int local_var_1;

	if (check_stackgrow_up(&local_var_1))
		tst_brk(TCONF, "Test can't be performed with stack grows up architecture");
}

static void cleanup(void)
{
	if (stack)
		SAFE_MUNMAP(stack, UNITS(8));
}

static void *find_free_range(size_t size)
{
	void *mem;

	mem = SAFE_MMAP(NULL, size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	SAFE_MUNMAP(mem, size);

	return mem;
}

static void split_unmapped_plus_stack(void *start, size_t size)
{
	/* start           start + size
	 * +---------------------+----------------------+
	 * + unmapped            | mapped               |
	 * +---------------------+----------------------+
	 *                       stack
	 */
	stack = SAFE_MMAP(start + size, size, PROT_READ | PROT_WRITE,
			  MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN,
			  -1, 0);
}

static void *check_depth_recursive(void *limit)
{
	if ((off_t) &limit < (off_t) limit)
		return NULL;

	return check_depth_recursive(limit);
}

static void grow_stack(void *stack, size_t size, void *limit)
{
	pthread_t test_thread;
	pthread_attr_t attr;
	int ret;

	ret = pthread_attr_init(&attr);
	if (ret)
		tst_brk(TBROK, "pthread_attr_init failed during setup");

	ret = pthread_attr_setstack(&attr, stack, size);
	if (ret)
		tst_brk(TBROK, "pthread_attr_setstack failed during setup");
	SAFE_PTHREAD_CREATE(&test_thread, &attr, check_depth_recursive, limit);
	SAFE_PTHREAD_JOIN(test_thread, NULL);

	exit(0);
}

static void run_test(void)
{
	void *mem;
	pid_t child_pid;
	int wstatus;

	mem = find_free_range(UNITS(16));
	split_unmapped_plus_stack(mem, UNITS(8));

	child_pid = SAFE_FORK();
	if (!child_pid)
		grow_stack(stack, UNITS(8), mem + UNITS(1));

	SAFE_WAIT(&wstatus);
	if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0)
		tst_res(TPASS, "Stack grows in unmapped region");
	else if (WIFSIGNALED(wstatus))
		tst_res(TFAIL, "Child killed by %s", tst_strsig(WTERMSIG(wstatus)));
	else
		tst_res(TFAIL, "Child %s", tst_strstatus(wstatus));
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run_test,
	.forks_child = 1,
};
