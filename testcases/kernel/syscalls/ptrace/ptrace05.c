// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (C) 2009, Ngie Cooper
 * Copyright (c) 2023 Wei Gao <wegao@suse.com>
 */

/*\
 * [Description]
 *
 *  ptrace05 - an app which ptraces itself as per arbitrarily specified signals
 *
 */

#include <stdlib.h>
#include <config.h>
#include "ptrace.h"

#include "lapi/signal.h"
#include "tst_test.h"

static void run(void)
{

	int end_signum = SIGRTMAX;
	int signum = 0;
	int start_signum = 0;
	int status;

	pid_t child;

	for (signum = start_signum; signum <= end_signum; signum++) {

		switch (child = SAFE_FORK()) {
		case -1:
			tst_brk(TBROK | TERRNO, "fork() failed");
		case 0:

			TEST(ptrace(PTRACE_TRACEME, 0, NULL, NULL));
			if (TST_RET != -1) {
				tst_res(TINFO, "[child] Sending kill(.., %d)",
						signum);
				SAFE_KILL(getpid(), signum);
			} else {
				tst_brk(TFAIL | TERRNO,
					 "Failed to ptrace(PTRACE_TRACEME, ...) "
					 "properly");
			}

			exit(0);
			break;

		default:

			SAFE_WAITPID(child, &status, 0);

			switch (signum) {
			case 0:
				if (WIFEXITED(status)
				    && WEXITSTATUS(status) == 0) {
					tst_res(TPASS,
						 "kill(.., 0) exited "
						 "with 0, as expected.");
				} else {
					tst_brk(TFAIL | TERRNO,
						 "kill(.., 0) didn't exit "
						 "with 0.");
				}
				break;
			case SIGKILL:
				if (WIFSIGNALED(status)) {
					/* SIGKILL must be uncatchable. */
					if (WTERMSIG(status) == SIGKILL) {
						tst_res(TPASS,
							 "Killed with SIGKILL, "
							 "as expected.");
					} else {
						tst_brk(TFAIL | TERRNO,
							 "Didn't die with "
							 "SIGKILL (?!) ");
					}
				} else if (WIFEXITED(status)) {
					tst_brk(TFAIL | TERRNO,
						 "Exited unexpectedly instead "
						 "of dying with SIGKILL.");
				} else if (WIFSTOPPED(status)) {
					tst_brk(TFAIL | TERRNO,
						 "Stopped instead of dying "
						 "with SIGKILL.");
				}
				break;
				/* All other processes should be stopped. */
			default:
				if (WIFSTOPPED(status)) {
					tst_res(TPASS, "Stopped as expected");
				} else {
					tst_brk(TFAIL | TERRNO, "Didn't stop as "
						 "expected.");
				}
				break;
			}
		}

		if (signum != 0 && signum != 9)
			SAFE_PTRACE(PTRACE_CONT, child, NULL, NULL);
	}
}

static struct tst_test test = {
	.test_all = run,
	.forks_child = 1,
};
