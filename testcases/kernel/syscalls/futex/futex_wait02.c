/*
 * Copyright (C) 2015 Cyril Hrubis <chrubis@suse.cz>
 *
 * Licensed under the GNU GPLv2 or later.
 * This program is free software;  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;  if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */
 /*
  * Block on a futex and wait for wakeup.
  *
  * This tests uses shared memory page to store the mutex variable.
  */

#include <sys/mman.h>
#include <sys/wait.h>
#include <errno.h>

#include "futextest.h"
#include "futex_utils.h"
#include "lapi/abisize.h"

static futex_t *futex;

static struct test_variants {
	enum futex_fn_type fntype;
	char *desc;
} variants[] = {
#if defined(TST_ABI32)
	{ .fntype = FUTEX_FN_FUTEX, .desc = "syscall with kernel spec32"},
#endif

#if defined(TST_ABI64)
	{ .fntype = FUTEX_FN_FUTEX, .desc = "syscall with kernel spec64"},
#endif

#if (__NR_futex_time64 != __LTP__NR_INVALID_SYSCALL)
	{ .fntype = FUTEX_FN_FUTEX64, .desc = "syscall time64 with kernel spec64"},
#endif
};

static void do_child(void)
{
	struct test_variants *tv = &variants[tst_variant];
	int ret;

	process_state_wait2(getppid(), 'S');

	ret = futex_wake(tv->fntype, futex, 1, 0);

	if (ret != 1)
		tst_res(TFAIL | TTERRNO, "futex_wake() failed");

	exit(0);
}

static void run(void)
{
	struct test_variants *tv = &variants[tst_variant];
	int res, pid;

	pid = SAFE_FORK();

	switch (pid) {
	case 0:
		do_child();
	default:
		break;
	}

	res = futex_wait(tv->fntype, futex, *futex, NULL, 0);
	if (res) {
		tst_res(TFAIL | TTERRNO, "futex_wait() failed");
		return;
	}

	SAFE_WAIT(&res);

	if (WIFEXITED(res) && WEXITSTATUS(res) == TPASS)
		tst_res(TPASS, "futex_wait() woken up");
	else
		tst_res(TFAIL, "child failed");
}

static void setup(void)
{
	tst_res(TINFO, "Testing variant: %s", variants[tst_variant].desc);

	futex = SAFE_MMAP(NULL, sizeof(*futex), PROT_READ | PROT_WRITE,
			  MAP_ANONYMOUS | MAP_SHARED, -1, 0);

	*futex = FUTEX_INITIALIZER;
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.test_variants = ARRAY_SIZE(variants),
	.forks_child = 1,
};
