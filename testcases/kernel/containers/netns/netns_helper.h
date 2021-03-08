// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) International Business Machines Corp., 2008
 *
 * Author: Veerendra C <vechandr@in.ibm.com>
 *
 * Net namespaces were introduced around 2.6.25.  Kernels before that,
 * assume they are not enabled.  Kernels after that, check for -EINVAL
 * when trying to use CLONE_NEWNET and CLONE_NEWNS.
 */

#define _GNU_SOURCE
#include "config.h"
#include <sched.h>
#include "lapi/syscalls.h"
#include "tst_safe_macros.h"
#include "libclone.h"

#ifndef CLONE_NEWNS
#define CLONE_NEWNS -1
#endif

static void check_iproute(unsigned int spe_ipver)
{
	FILE *ipf;
	int n;
	unsigned int ipver = 0;

	ipf = popen("ip -V", "r");
	if (ipf == NULL)
		tst_brk(TCONF, "Failed while opening pipe for iproute check");

	n = fscanf(ipf, "ip utility, iproute2-ss%u", &ipver);
	if (n < 1) {
		tst_brk(TCONF, "Failed while obtaining version for iproute check");
	}
	if (ipver < spe_ipver) {
		tst_brk(TCONF,
			"The commands in iproute tools do not support required objects");
	}

	pclose(ipf);
}

static int dummy(void *arg)
{
	(void) arg;
	return 0;
}

static void check_netns(void)
{
	int pid, status;
	/* Checking if the kernel supports unshare with netns capabilities. */
	if (CLONE_NEWNS == -1)
		tst_brk(TCONF | TERRNO, "CLONE_NEWNS (%d) not supported",
			 CLONE_NEWNS);

	pid = do_clone_unshare_test(T_UNSHARE, CLONE_NEWNET | CLONE_NEWNS,
	                            dummy, NULL);
	if (pid == -1)
		tst_brk(TCONF | TERRNO,
				"unshare syscall smoke test failed");

	SAFE_WAIT(&status);
}
