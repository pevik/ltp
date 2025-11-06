// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Linux Test Project, 2009-2025
 * Copyright (c) International Business Machines Corp., 2007
 * Created by <rsalveti@linux.vnet.ibm.com>
 */

/*\
 * Test checks whether :man2:`swapon` system call returns EPERM when the maximum
 * number of swap files are already in use.
 *
 * NOTE: test does not try to calculate MAX_SWAPFILES from the internal
 * kernel implementation (which is currently <23, 29> depending on kernel
 * configuration). Instead test exptect that at least 15 swap files minus
 * currently used swap can be created.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/swap.h>
#include "tst_test.h"
#include "lapi/syscalls.h"
#include "libswap.h"

#define NUM_SWAP_FILES 15
#define MNTPOINT	"mntpoint"
#define TEST_FILE	MNTPOINT"/testswap"

static int *swapfiles;
static char *tmpdir;

static void setup_swap(void)
{
	pid_t pid;
	int status, used_swapfiles, expected_swapfiles;
	char filename[FILENAME_MAX];

	SAFE_SETEUID(0);

	used_swapfiles = tst_count_swaps();
	expected_swapfiles = NUM_SWAP_FILES - used_swapfiles;

	if (expected_swapfiles < 0)
		tst_brk(TCONF, "too many used swap files (%d)", used_swapfiles);

	pid = SAFE_FORK();
	if (pid == 0) {
		while (true) {
			/* Create the swapfile */
			snprintf(filename, sizeof(filename), "%s%02d", TEST_FILE, *swapfiles);
			MAKE_SMALL_SWAPFILE(filename);

			/* Quit on a first swap file over max */
			if (swapon(filename, 0) == -1)
				break;
			(*swapfiles)++;
		}
		exit(0);
	} else {
		waitpid(pid, &status, 0);
	}

	if (WEXITSTATUS(status) || *swapfiles == 0)
		tst_brk(TBROK, "Failed to setup swap files");

	if (*swapfiles < expected_swapfiles) {
		tst_res(TWARN, "Successfully created only %d swap files (>= %d expected)",
			*swapfiles, expected_swapfiles);
	} else {
		tst_res(TINFO, "Successfully created %d swap files", *swapfiles);
	}
}

/*
 * Check if the file is at /proc/swaps and remove it giving swapoff
 */
static void check_and_swapoff(const char *filename)
{
	char cmd_buffer[256];

	snprintf(cmd_buffer, sizeof(cmd_buffer), "grep -q '%s.*file' /proc/swaps", filename);

	if (system(cmd_buffer) == 0 && swapoff(filename) != 0)
		tst_res(TWARN, "Failed to swapoff %s", filename);
}

/*
 * Turn off all swapfiles previously turned on
 */
static void clean_swap(void)
{
	int j;
	char filename[FILENAME_MAX];

	for (j = 0; j < *swapfiles; j++) {
		snprintf(filename, sizeof(filename), "%s/%s%02d", tmpdir, TEST_FILE, j);
		check_and_swapoff(filename);
	}

	snprintf(filename, sizeof(filename), "%s/%s", tmpdir, TEST_FILE);
	check_and_swapoff(filename);
}

static void verify_swapon(void)
{
	TST_EXP_FAIL(swapon(TEST_FILE, 0), EPERM, "swapon(%s, 0)", TEST_FILE);
}

static void setup(void)
{
	if (access("/proc/swaps", F_OK))
		tst_brk(TCONF, "swap not supported by kernel");

	is_swap_supported(TEST_FILE);

	tmpdir = tst_tmpdir_path();

	swapfiles = SAFE_MMAP(NULL, sizeof(*swapfiles), PROT_READ | PROT_WRITE,
			MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*swapfiles = 0;

	setup_swap();
}

static void cleanup(void)
{
	if (swapfiles) {
		clean_swap();
		SAFE_MUNMAP(swapfiles, sizeof(*swapfiles));
	}
}

static struct tst_test test = {
	.mntpoint = MNTPOINT,
	.mount_device = 1,
	.all_filesystems = 1,
	.needs_root = 1,
	.forks_child = 1,
	.test_all = verify_swapon,
	.setup = setup,
	.cleanup = cleanup
};
