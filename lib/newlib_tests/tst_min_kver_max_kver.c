// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Petr Vorel <pvorel@suse.cz>
 */

/*
 * Basic test to test that test with .min_kver < .max_kver will be run.
 */

#include "tst_test.h"

static void do_test(void)
{
	tst_res(TPASS, "Test has sufficient kernel version");
}

static struct tst_test test = {
	.min_kver = "4.4",
	.max_kver = "5.0",
	.test_all = do_test,
};
