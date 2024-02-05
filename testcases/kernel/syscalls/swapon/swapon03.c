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

#include "tst_test.h"
#include "lapi/syscalls.h"
#include "swaponoff.h"
#include "libswap.h"

#define MNTPOINT	"mntpoint"
#define TEST_FILE	MNTPOINT"/testswap"

static int swapfiles;
static int testfiles = 3;

static struct swap_testfile_t {
	char *filename;
} swap_testfiles[] = {
	{"firstswapfile"},
	{"secondswapfile"},
	{"thirdswapfile"}
};

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
			if (tst_syscall(__NR_swapoff, filename) != 0) {

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

/*
 * Create 33 and activate 30 swapfiles.
 */
static int setup_swap(void)
{
	pid_t pid;
	int j, fd;
	int status;
	int res = 0;
	char filename[FILENAME_MAX];
	char buf[BUFSIZ + 1];

	/* Find out how many swapfiles (1 line per entry) already exist */
	swapfiles = 0;

	SAFE_SETEUID(0);

	fd = SAFE_OPEN("/proc/swaps", O_RDONLY);
	do {
		char *p = buf;

		res = SAFE_READ(0, fd, buf, BUFSIZ);
		buf[res] = '\0';
		while ((p = strchr(p, '\n'))) {
			p++;
			swapfiles++;
		}
	} while (res >= BUFSIZ);
	SAFE_CLOSE(fd);

	/* don't count the /proc/swaps header */
	if (swapfiles)
		swapfiles--;

	if (swapfiles < 0)
		tst_brk(TFAIL, "Failed to find existing number of swapfiles");

	/* Determine how many more files are to be created */
	swapfiles = MAX_SWAPFILES - swapfiles;
	if (swapfiles > MAX_SWAPFILES)
		swapfiles = MAX_SWAPFILES;

	pid = SAFE_FORK();
	if (pid == 0) {
		/*create and turn on remaining swapfiles */
		for (j = 0; j < swapfiles; j++) {
			sprintf(filename, "swapfile%02d", j + 2);
			make_swapfile(filename, 10, 0);

			/* turn on the swap file */
			res = tst_syscall(__NR_swapon, filename, 0);
			if (res != 0) {
				if (errno == EPERM) {
					tst_res(TINFO, "Successfully created %d swapfiles", j);
					break;
				}

				tst_res(TWARN | TERRNO, "Failed to create swapfile: %s", filename);
				exit(1);
			}
		}
		exit(0);
	} else {
		waitpid(pid, &status, 0);
	}

	if (WEXITSTATUS(status))
		tst_brk(TFAIL, "Failed to setup swaps");

	/* Create all needed extra swapfiles for testing */
	for (j = 0; j < testfiles; j++)
		make_swapfile(swap_testfiles[j].filename, 10, 0);

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

	for (j = 0; j < testfiles; j++) {
		if (check_and_swapoff(swap_testfiles[j].filename) != 0) {
			tst_res(TWARN, "Failed to turn off swap file %s.",
				 swap_testfiles[j].filename);
			return -1;
		}
	}

	return 0;
}

static void verify_swapon(void)
{
	if (setup_swap() < 0) {
		clean_swap();
		tst_brk(TBROK, "Setup failed, quitting the test");
	}

	TST_EXP_FAIL(tst_syscall(__NR_swapon, swap_testfiles[0].filename, 0),
		     EPERM);
}

static void setup(void)
{
	if (access("/proc/swaps", F_OK))
		tst_brk(TCONF, "swap not supported by kernel");

	if (tst_fs_type(".") == TST_TMPFS_MAGIC)
		tst_brk(TCONF, "swap not supported on tmpfs");

	is_swap_supported(TEST_FILE);
}

static void cleanup(void)
{
	clean_swap();
}

static struct tst_test test = {
	.mntpoint = MNTPOINT,
	.mount_device = 1,
	.all_filesystems = 1,
	.needs_root = 1,
	.needs_tmpdir = 1,
	.forks_child = 1,
	.test_all = verify_swapon,
	.setup = setup,
	.cleanup = cleanup
};
