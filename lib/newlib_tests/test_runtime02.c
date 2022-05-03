// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021, Linux Test Project
 */
/*
 * This test is set up so that the timeout is not long enough to guarantee
 * enough runtime for two iterations, i.e. the timeout without offset and after
 * scaling is too small and the tests ends up with TBROK.
 *
 * You can fix this by exporting LTP_MAX_TEST_RUNTIME=10 before executing the
 * test, in that case the runtime would be divided between iterations and timeout
 * adjusted so that it provides enough safeguards for the test to finish.
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
	.max_iteration_runtime = 5,
	.test_variants = 2
};
