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
  * Block several processes on a mutex, then wake them up.
  */

#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

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

	futex_wait(tv->fntype, futex, *futex, NULL, 0);
	exit(0);
}

static void do_wake(int nr_children)
{
	struct test_variants *tv = &variants[tst_variant];
	int res, i, cnt;

	res = futex_wake(tv->fntype, futex, nr_children, 0);

	if (res != nr_children) {
		tst_res(TFAIL | TTERRNO,
		        "futex_wake() woken up %i children, expected %i",
			res, nr_children);
		return;
	}

	for (cnt = 0, i = 0; i < 100000; i++) {
		while (waitpid(-1, &res, WNOHANG) > 0)
			cnt++;

		if (cnt == nr_children)
			break;

		usleep(100);
	}

	if (cnt != nr_children) {
		tst_res(TFAIL | TTERRNO, "reaped only %i childs, expected %i",
		        cnt, nr_children);
	} else {
		tst_res(TPASS, "futex_wake() woken up %i childs", cnt);
	}
}

static void run(void)
{
	struct test_variants *tv = &variants[tst_variant];
	pid_t pids[55];
	unsigned int i;
	int res;

	for (i = 0; i < ARRAY_SIZE(pids); i++) {
		pids[i] = SAFE_FORK();

		switch (pids[i]) {
		case 0:
			do_child();
		default:
			break;
		}
	}

	for (i = 0; i < ARRAY_SIZE(pids); i++)
		process_state_wait2(pids[i], 'S');

	for (i = 1; i <= 10; i++)
		do_wake(i);

	res = futex_wake(tv->fntype, futex, 1, 0);

	if (res) {
		tst_res(TFAIL | TTERRNO, "futex_wake() woken up %u, none were waiting",
			res);
	} else {
		tst_res(TPASS, "futex_wake() woken up 0 children");
	}
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
