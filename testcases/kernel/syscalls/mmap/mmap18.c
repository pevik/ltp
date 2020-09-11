// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Zilogic Systems Pvt. Ltd., 2020
 * Email: code@zilogic.com
 */

/*
 * Test mmap() MAP_GROWSDOWN flag
 *
 * # Test1:
 *   We assign the memory region allocated using MAP_GROWSDOWN to a thread,
 *   as a stack, to test the effect of MAP_GROWSDOWN. This is because the
 *   kernel only grows the memory region when the stack pointer, is within
 *   guard page, when the guard page is touched.
 *
 *   1. Map an anyonymous memory region of size X, and unmap it.
 *   2. Split the unmapped memory region into two.
 *   3. The lower memory region is left unmapped.
 *   4. The higher memory region is mapped for use as stack, using
 *      MAP_FIXED | MAP_GROWSDOWN.
 *   5. The higher memory region is provided as stack to a thread, where
 *      a recursive function is invoked.
 *   6. The stack grows beyond the allocated region, into the lower memory area.
 *   7. If this results in the memory region being extended, into the
 *      unmapped region, the test is considered to have passed.
 *
 * # Test2:
 *   Steps mostly like Test1, but mmaping a page in the space the stack is
 *   supposed to grow into. To verify that the stack grows to within a page
 *   of the high end of the next lower map‐ping, at which point touching
 *   the "guard" page will result in a SIGSEGV signal.
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
static long stack_size = UNITS(8);

static bool __attribute__((noinline)) check_stackgrow_up(void)
{
	char local_var;
	static char *addr;

       if (!addr) {
               addr = &local_var;
               return check_stackgrow_up();
       }

       return (addr < &local_var);
}

static void setup(void)
{
	if (check_stackgrow_up())
		tst_brk(TCONF, "Test can't be performed with stack grows up architecture");
}

static void allocate_stack(size_t size)
{
	void *start;

	/*
	 * Since the newer kernel does not allow a MAP_GROWSDOWN mapping to grow
	 * closer than 'stack_guard_gap' pages away from a preceding mapping.
	 * The guard then ensures that the next-highest mapped page remains more
	 * than 'stack_guard_gap' below the lowest stack address, and if not then
	 * it will trigger a segfault. So, here adding 256 pages memory spacing
	 * for stack growing safely.
	 *
	 * Btw, kernel default 'stack_guard_gap' size is '256 * getpagesize()'.
	 */
	long total_size = 256 * getpagesize() + size * 2;

	start = SAFE_MMAP(NULL, total_size, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	SAFE_MUNMAP(start, total_size);

	/* start                             stack
	 * +-----------+---------------------+----------------------+
	 * | 256 pages | unmapped (size)     | mapped (size)        |
	 * +-----------+---------------------+----------------------+
	 *
	 */
	stack = SAFE_MMAP((start + total_size - size), size, PROT_READ | PROT_WRITE,
			  MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS | MAP_GROWSDOWN,
			  -1, 0);

	tst_res(TINFO, "start = %p, stack = %p", start, stack);
}

static __attribute__((noinline)) void *check_depth_recursive(void *limit)
{
	if ((off_t) &limit < (off_t) limit) {
		tst_res(TINFO, "&limit = %p, limit = %p", &limit, limit);
		return NULL;
	}

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

	if (stack)
		SAFE_MUNMAP(stack, stack_size);

	exit(0);
}

static void run_test(void)
{
	pid_t child_pid;
	int wstatus;

	/* Test 1 */
	child_pid = SAFE_FORK();
	if (!child_pid) {
		allocate_stack(stack_size);
		grow_stack(stack, stack_size, stack - stack_size + UNITS(1));
	}

	SAFE_WAIT(&wstatus);
	if (WIFEXITED(wstatus) && WEXITSTATUS(wstatus) == 0)
		tst_res(TPASS, "Stack grows in unmapped region");
	else
		tst_res(TFAIL, "Child: %s", tst_strstatus(wstatus));

	/* Test 2 */
	child_pid = SAFE_FORK();
	if (!child_pid) {
		tst_no_corefile(0);
		allocate_stack(stack_size);

		SAFE_MMAP(stack - stack_size, UNITS(1), PROT_READ | PROT_WRITE,
			  MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		/* This will cause to segment fault (SIGSEGV) */
		grow_stack(stack, stack_size, stack - stack_size + UNITS(1));
	}

	SAFE_WAIT(&wstatus);
        if (WIFSIGNALED(wstatus) && WTERMSIG(wstatus) == SIGSEGV)
		tst_res(TPASS, "Child ended by %s as expected", tst_strsig(SIGSEGV));
        else
                tst_res(TFAIL, "Child: %s", tst_strstatus(wstatus));
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run_test,
	.forks_child = 1,
};
