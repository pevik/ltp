// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2001
 * Copyright (c) 2025 Linux Test Project
 * 07/2001 Ported by Wayne Boyer
 */

/*\
 * [Description]
 *
 * Call mmap() to map a file creating a mapped region with execute access
 * under the following conditions:
 * - The prot parameter is set to PROT_EXEC
 * - The file descriptor is open for read
 * - The file being mapped has execute permission bit set
 * - The minimum file permissions should be 0555
 * The call should succeed to map the file creating mapped memory with the
 * required attributes.
 *
 * mmap() should succeed returning the address of the mapped region,
 * and the mapped region should contain the contents of the mapped file.
 * but with ia64, PARISC/hppa and x86_64 (with PKU), an attempt to access
 * the contents of the mapped region should give rise to the signal SIGSEGV.
 */

#include <setjmp.h>
#include <stdlib.h>

#include "tst_kconfig.h"
#include "tst_test.h"

#define TEMPFILE "mmapfile"

static size_t page_sz;
static char *addr;
static char *dummy;
static int fildes;
static volatile int sig_flag = -1;
static sigjmp_buf env;

/*
 * This function gets executed when the test process receives the signal
 * SIGSEGV while trying to access the contents of memory which is not accessible.
 */
static void sig_handler(int sig)
{
	if (sig == SIGSEGV) {
		/* Set the global variable and jump back */
		sig_flag = 1;
		siglongjmp(env, 1);
	} else {
		tst_brk(TBROK, "received an unexpected signal: %d", sig);
	}
}

static void setup(void)
{
	char *tst_buff;

	SAFE_SIGNAL(SIGSEGV, sig_handler);

	page_sz = getpagesize();

	/* Allocate space for the test buffer */
	tst_buff = SAFE_CALLOC(page_sz, sizeof(char));

	/* Fill the test buffer with the known data */
	memset(tst_buff, 'A', page_sz);

	/* Create a temporary file used for mapping */
	fildes = SAFE_OPEN(TEMPFILE, O_WRONLY | O_CREAT);

	/* Write test buffer contents into temporary file */
	SAFE_WRITE(SAFE_WRITE_ALL, fildes, tst_buff, page_sz);

	/* Free the memory allocated for test buffer */
	free(tst_buff);

	/* Make sure proper permissions set on file */
	SAFE_FCHMOD(fildes, 0555);

	/* Close the temporary file opened for write */
	SAFE_CLOSE(fildes);

	/* Allocate and initialize dummy string of system page size bytes */
	dummy = SAFE_CALLOC(page_sz, sizeof(char));

	/* Open the temporary file again for reading */
	fildes = SAFE_OPEN(TEMPFILE, O_RDONLY);
}

static void run(void)
{
	addr = SAFE_MMAP(0, page_sz, PROT_EXEC, MAP_FILE | MAP_SHARED, fildes,
			 0);

	/* Read the file contents into the dummy variable. */
	SAFE_READ(0, fildes, dummy, page_sz);

	/*
	 * Check whether the mapped memory region
	 * has the file contents. With ia64, PARISC/hppa and x86_64 (with PKU),
	 * this should generate a SIGSEGV which will be caught below.
	 */
	if (sigsetjmp(env, 1) == 0) {
		if (memcmp(dummy, addr, page_sz)) {
			tst_res(TINFO, "memcmp returned non-zero");
			tst_res(TFAIL,
				"mapped memory region contains invalid data");
		} else {
			tst_res(TINFO, "memcmp returned zero");
			tst_res(TPASS, "mmap() functionality is correct");
		}
	}

#if defined(__ia64__) || defined(__hppa__) || defined(__mips__)
	if (sig_flag > 0)
		tst_res(TPASS, "Got SIGSEGV as expected");
	else
		tst_res(TFAIL,
			"Mapped memory region with NO access is accessible");
#elif defined(__x86_64__)
	struct tst_kconfig_var kconfig =
		TST_KCONFIG_INIT("CONFIG_X86_INTEL_MEMORY_PROTECTION_KEYS");
	tst_kconfig_read(&kconfig, 1);
	if (kconfig.choice == 'y') {
		if (sig_flag > 0)
			tst_res(TPASS, "Got SIGSEGV as expected");
		else
			tst_res(TFAIL,
				"Mapped memory region with NO access is accessible");
	}
#else
	if (sig_flag > 0)
		tst_res(TFAIL, "Got unexpected SIGSEGV");
#endif

	/* Clean up things in case we are looping */
	sig_flag = -1;
	SAFE_MUNMAP(addr, page_sz);
}

static void cleanup(void)
{
	free(dummy);
	if (fildes > 0)
		SAFE_CLOSE(fildes);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
};
