// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright Red Hat
 * Author: Dennis Brendel <dbrendel@redhat.com>
 */

/*\
 * [Description]
 *
 * Verify that munlockall(2) unlocks all previously locked memory
 */

#include <sys/mman.h>

#include "tst_test.h"

static void verify_munlockall(void)
{
	size_t size = 0;

	SAFE_FILE_LINES_SCANF("/proc/self/status", "VmLck: %ld", &size);

	if (size != 0UL)
		tst_brk(TBROK, "Locked memory after init should be 0 but is "
			       "%ld", size);

	if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0)
		tst_brk(TBROK, "Could not lock memory using mlockall()");

	SAFE_FILE_LINES_SCANF("/proc/self/status", "VmLck: %ld", &size);

	if (size == 0UL)
		tst_brk(TBROK, "Locked memory after mlockall() should be "
			       "greater than 0, but is %ld", size);

	if (munlockall() != 0)
		tst_brk(TBROK, "Could not unlock memory using munlockall()");

	SAFE_FILE_LINES_SCANF("/proc/self/status", "VmLck: %ld", &size);

	if (size != 0UL) {
		tst_res(TFAIL, "Locked memory after munlockall() should be 0 "
			       "but is %ld", size);
	} else {
		tst_res(TPASS, "Test passed");
	}
}

static struct tst_test test = {
	.test_all = verify_munlockall,
};
