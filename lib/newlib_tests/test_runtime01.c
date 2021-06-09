// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021, Linux Test Project
 */

#include <stdlib.h>
#include <unistd.h>
#include "tst_test.h"

static void run(void)
{
	while (tst_remaining_runtime())
		sleep(1);

	tst_res(TPASS, "Timeout remaining: %d", tst_remaining_runtime());
}

static struct tst_test test = {
	.test_all = run,
	.timeout = 5
};
