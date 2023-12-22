// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines Corp., 2007
 * Created by <rsalveti@linux.vnet.ibm.com>
 *
 */

/*\
 * [Description]
 *
 * This test case checks whether swapon(2) system call returns:
 *  - EPERM when there are more than MAX_SWAPFILES already in use.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/swap.h>
#include "tst_test.h"
#include "libswap.h"

#define TESTFILE "testfile"

static int setup_swap(void);
static int clean_swap(void);
static int check_and_swapoff(const char *filename);
static int swapfiles;

static void verify_swapon(void)
{
	TST_EXP_FAIL(swapon(TESTFILE, 0), EPERM, "swapon(%s, 0)", TESTFILE);
}

static int setup_swap(void)
{
	pid_t pid;
	int j, max_swapfiles, used_swapfiles;
	int status;
	char filename[FILENAME_MAX];

	if (seteuid(0) < 0)
		tst_brk(TFAIL | TERRNO, "Failed to call seteuid");

	/* Determine how many more files are to be created */
	max_swapfiles = (int)get_maxswapfiles();
	used_swapfiles = GET_USED_SWAPFILES();
	swapfiles = max_swapfiles - used_swapfiles;
	if (swapfiles > max_swapfiles)
		swapfiles = max_swapfiles;
	pid = SAFE_FORK();
	if (pid == 0) {
		/*create and turn on remaining swapfiles */
		for (j = 0; j < swapfiles; j++) {

			/* prepare filename for the iteration */
			if (sprintf(filename, "swapfile%02d", j + 2) < 0) {
				printf("sprintf() failed to create "
				       "filename");
				exit(1);
			}

			/* Create the swapfile */
			make_swapfile(filename, 0);

			/* turn on the swap file */
			TST_EXP_PASS_SILENT(swapon(filename, 0));
		}
		exit(0);
	} else
		waitpid(pid, &status, 0);

	if (WEXITSTATUS(status))
		tst_brk(TFAIL, "Failed to setup swaps");

	tst_res(TINFO, "Successfully created %d swapfiles", swapfiles);
	make_swapfile(TESTFILE, 0);

	return 0;
}

/*
 * Turn off all swapfiles previously turned on
 */
static int clean_swap(void)
{
	int j;
	char filename[FILENAME_MAX];

	for (j = 0; j < swapfiles; j++) {
		if (snprintf(filename, sizeof(filename),
			     "swapfile%02d", j + 2) < 0) {
			tst_res(TWARN, "sprintf() failed to create filename");
			tst_res(TWARN, "Failed to turn off swap files. System"
				 " reboot after execution of LTP test"
				 " suite is recommended");
			return -1;
		}
		if (check_and_swapoff(filename) != 0) {
			tst_res(TWARN, "Failed to turn off swap file %s.", filename);
			return -1;
		}
	}

	if (check_and_swapoff("testfile") != 0) {
		tst_res(TWARN, "Failed to turn off swap file testfile");
		return -1;
	}

	return 0;
}

/*
 * Check if the file is at /proc/swaps and remove it giving swapoff
 */
static int check_and_swapoff(const char *filename)
{
	char cmd_buffer[256];
	int rc = -1;

	if (snprintf(cmd_buffer, sizeof(cmd_buffer),
		     "grep -q '%s.*file' /proc/swaps", filename) < 0) {
		tst_res(TWARN, "sprintf() failed to create the command string");
	} else {
		rc = 0;
		if (system(cmd_buffer) == 0) {
			/* now we need to swapoff the file */
			if (swapoff(filename) != 0) {
				tst_res(TWARN, "Failed to turn off swap "
					 "file. system reboot after "
					 "execution of LTP test suite "
					 "is recommended");
				rc = -1;
			}

		}
	}

	return rc;
}

static void setup(void)
{
	if (access("/proc/swaps", F_OK))
		tst_brk(TCONF, "swap not supported by kernel");

	is_swap_supported("./tstswap");

	if (setup_swap() < 0) {
		clean_swap();
		tst_brk(TBROK, "Setup failed, quitting the test");
	}
}

static void cleanup(void)
{
	clean_swap();
}

static struct tst_test test = {
	.needs_root = 1,
	.needs_tmpdir = 1,
	.forks_child = 1,
	.test_all = verify_swapon,
	.setup = setup,
	.cleanup = cleanup
};
