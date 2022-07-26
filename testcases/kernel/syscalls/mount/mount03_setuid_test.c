// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) Wipro Technologies Ltd, 2002.  All Rights Reserved.
 * Copyright (c) 2022 Petr Vorel <pvorel@suse.cz>
 */

/*
 * setuid to root program invoked by a non-root process to validate the mount
 * flag MS_NOSUID.
 */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdlib.h>
#include <errno.h>

#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"

/* Save the effective and real UIDs. */

static uid_t ruid;

/* Restore the effective UID to its original value. */

int do_setuid(void)
{
	if (setreuid(ruid, 0) < 0)
		return 1;

	return 0;
}

int main(int argc, char *argv[])
{
	tst_res(TINFO, "START"); // FIXME: debug
	tst_reinit();

	/* Save the real and effective user IDs.  */
	ruid = getuid();

	//TST_EXP_PASS(do_setuid());
	//if (do_setuid())
	int rc = do_setuid();
	fprintf(stderr, "%s:%d %s(): rc: '%d'\n", __FILE__, __LINE__, __func__, rc);
	if (rc)
		tst_res(TPASS, "%s executed", argv[0]);

	return 0;
}
