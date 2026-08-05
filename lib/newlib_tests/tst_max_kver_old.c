// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Petr Vorel <pvorel@suse.cz>
 */

/*
 * Basic test to test .max_kver detection works.
 * Test should TCONF due running on newer kernel.
 */

#include "tst_test.h"

static void do_test(void)
{
	tst_res(TFAIL, "Really running on kernel 1.0?");
}

static struct tst_test test = {
	.max_kver = "1.0",
	.test_all = do_test,
};
