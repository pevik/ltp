// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Crackerjack Project., 2007
 * Copyright (c) Manas Kumar Nayak <maknayak@in.ibm.com>
 * Copyright (c) Cyril Hrubis <chrubis@suse.cz> 2011
 * Copyright (c) 2025 SUSE LLC Ricardo B. Marlière <rbm@suse.com>
 */

/*\
 * Basic test of i386 thread-local storage for set_thread_area and
 * get_thread_area syscalls.
 *
 * [Algorithm]
 *
 * The test will first call set_thread_area to a struct user_desc pointer with
 * entry_number = -1, which will be set to a free entry_number upon exiting.
 * Therefore, a subsequent call to get_thread_area using the same pointer will
 * be valid. The same process is done but using -2 instead, which returns EINVAL
 * to both calls. Finally, it's done one more time but to an invalid pointer and
 * therefore an EFAULT is returned.
 */

#include "tst_test.h"

#ifdef __i386__
#include "lapi/ldt.h"

/* When set_thread_area() is passed an entry_number of -1, it  searches
 * for a free TLS entry. If set_thread_area() finds a free TLS entry,
 * the value of u_info->entry_number is set upon return to show which
 * entry was changed.
 */
static struct user_desc entry = { .entry_number = -1 };
static struct user_desc invalid_entry = { .entry_number = -2 };

static int set_thread_area(const struct user_desc *entry)
{
	return tst_syscall(__NR_set_thread_area, entry);
}

static int get_thread_area(const struct user_desc *entry)
{
	return tst_syscall(__NR_get_thread_area, entry);
}

void run(void)
{
	TST_EXP_PASS(set_thread_area(&entry));
	TST_EXP_PASS(get_thread_area(&entry));

	TST_EXP_FAIL(set_thread_area(&invalid_entry), EINVAL);
	TST_EXP_FAIL(get_thread_area(&invalid_entry), EINVAL);

	TST_EXP_FAIL(set_thread_area((void *)-9), EFAULT);
	TST_EXP_FAIL(get_thread_area((void *)-9), EFAULT);
}

static struct tst_test test = {
	.test_all = run,
};

#else
TST_TEST_TCONF("Test supported only on i386");
#endif /* __i386__ */
