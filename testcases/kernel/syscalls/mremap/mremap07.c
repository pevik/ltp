// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2025 Wei Gao <wegao@suse.com>
 */

/*\
 * LTP test case for mremap() with MREMAP_DONTUNMAP and userfaultfd.
 *
 * This test verifies a fault is triggered on the old memory region
 * and is then correctly handled by a userfaultfd handler.
 */

#define _GNU_SOURCE
#include <poll.h>
#include <pthread.h>

#include "tst_test.h"
#include "tst_safe_pthread.h"
#include "lapi/userfaultfd.h"

#if HAVE_DECL_MREMAP_DONTUNMAP

static int page_size;
static int uffd;
static char *fault_addr;
static char *new_remap_addr;

static const char *test_string = "Hello, world! This is a test string that fills up a page.";

static int sys_userfaultfd(int flags)
{
	return tst_syscall(__NR_userfaultfd, flags);
}

static void fault_handler_thread(void)
{
	struct uffd_msg msg;
	struct uffdio_copy uffdio_copy;

	TST_CHECKPOINT_WAIT(0);

	struct pollfd pollfd;

	pollfd.fd = uffd;
	pollfd.events = POLLIN;

	int nready = poll(&pollfd, 1, -1);

	if (nready <= 0)
		tst_brk(TBROK | TERRNO, "Poll on uffd failed");

	SAFE_READ(1, uffd, &msg, sizeof(msg));

	if (msg.event != UFFD_EVENT_PAGEFAULT)
		tst_brk(TBROK, "Received unexpected UFFD_EVENT: %d", msg.event);

	if ((char *)msg.arg.pagefault.address != fault_addr)
		tst_brk(TBROK, "Page fault on unexpected address: %p", (void *)msg.arg.pagefault.address);

	tst_res(TINFO, "Userfaultfd handler caught a page fault at %p", (void *)msg.arg.pagefault.address);

	uffdio_copy.src = (unsigned long)new_remap_addr;
	uffdio_copy.dst = (unsigned long)fault_addr;
	uffdio_copy.len = page_size;
	uffdio_copy.mode = 0;
	uffdio_copy.copy = 0;

	SAFE_IOCTL(uffd, UFFDIO_COPY, &uffdio_copy);
	tst_res(TPASS, "Userfaultfd handler successfully handled the fault.");
}

static void setup(void)
{
	page_size = getpagesize();
	struct uffdio_api uffdio_api;
	struct uffdio_register uffdio_register;

	TEST(sys_userfaultfd(O_CLOEXEC | O_NONBLOCK));
	if (TST_RET == -1) {
		if (TST_ERR == EPERM) {
			tst_res(TCONF, "Hint: check /proc/sys/vm/unprivileged_userfaultfd");
			tst_brk(TCONF | TTERRNO, "userfaultfd() requires CAP_SYS_PTRACE on this system");
		} else {
			tst_brk(TBROK | TTERRNO, "Could not create userfault file descriptor");
		}
	}

	uffd = TST_RET;
	uffdio_api.api = UFFD_API;
	uffdio_api.features = 0;
	SAFE_IOCTL(uffd, UFFDIO_API, &uffdio_api);

	fault_addr = SAFE_MMAP(NULL, page_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	tst_res(TINFO, "Original mapping created at %p", (void *)fault_addr);

	strcpy(fault_addr, "ABCD");

	uffdio_register.range.start = (unsigned long)fault_addr;
	uffdio_register.range.len = page_size;
	uffdio_register.mode = UFFDIO_REGISTER_MODE_MISSING;
	SAFE_IOCTL(uffd, UFFDIO_REGISTER, &uffdio_register);
}

static void cleanup(void)
{
	if (new_remap_addr != NULL)
		SAFE_MUNMAP(new_remap_addr, page_size);

	if (fault_addr != NULL)
		SAFE_MUNMAP(fault_addr, page_size);

	if (uffd != -1)
		SAFE_CLOSE(uffd);
}

static void run(void)
{
	pthread_t handler_thread;

	SAFE_PTHREAD_CREATE(&handler_thread, NULL,
		(void * (*)(void *))fault_handler_thread, NULL);

	new_remap_addr = mremap(fault_addr, page_size, page_size, MREMAP_DONTUNMAP | MREMAP_MAYMOVE);

	if (new_remap_addr == MAP_FAILED)
		tst_brk(TBROK | TTERRNO, "mremap failed");

	tst_res(TINFO, "New mapping created at %p", (void *)new_remap_addr);

	strcpy(new_remap_addr, test_string);

	TST_CHECKPOINT_WAKE(0);

	tst_res(TINFO, "Main thread accessing old address %p to trigger fault. Access Content is \"%s\"",
			(void *)fault_addr, fault_addr);

	SAFE_PTHREAD_JOIN(handler_thread, NULL);

	if (strcmp(fault_addr, test_string) != 0)
		tst_res(TFAIL, "Verification failed: Content at old address is '%s', expected '%s'", fault_addr, test_string);
	else
		tst_res(TPASS, "Verification passed: Content at old address is correct after fault handling.");
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_checkpoints = 1,
	.cleanup = cleanup,
	.needs_root = 1,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_USERFAULTFD=y",
		NULL,
	},
	.min_kver = "5.7",
};

#else
TST_TEST_TCONF("Missing MREMAP_DONTUNMAP in <linux/mman.h>");
#endif
