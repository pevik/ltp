// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2026 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * Verify that :manpage:`mmap(2)` with the MAP_32BIT flag restricts all mappings
 * to the first 2GB of the process address space (< 0x80000000).
 *
 * MAP_32BIT is supported only on x86-64 for 64-bit programs.
 *
 * [Algorithm]
 *
 * - Repeatedly call mmap() allocating 32MB chunks with MAP_32BIT until ENOMEM
 * - Verify that every returned address satisfies (addr + size) <= 0x80000000
 * - Verify that memory in each chunk is readable and writable
 * - Verify that at least one chunk was mapped and failure errno is ENOMEM
 * - Unmap all allocated chunks in cleanup
 */

#include "tst_test.h"
#include "lapi/mmap.h"

#define ADDR_LIMIT 0x80000000UL
#define CHUNK_SZ (32UL * TST_MB)
#define MAX_CHUNKS 64

static void *addrs[MAX_CHUNKS];
static size_t num_chunks;

static void cleanup(void)
{
	size_t i;

	for (i = 0; i < num_chunks; i++) {
		if (addrs[i]) {
			SAFE_MUNMAP(addrs[i], CHUNK_SZ);
			addrs[i] = NULL;
		}
	}
	num_chunks = 0;
}

static void run(void)
{
	size_t i;
	int failed_with_enomem = 0;

	for (i = 0; i < MAX_CHUNKS; i++) {
		void *addr = mmap(NULL, CHUNK_SZ, PROT_READ | PROT_WRITE,
				  MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);

		if (addr == MAP_FAILED) {
			if (errno == ENOMEM)
				failed_with_enomem = 1;
			else
				tst_res(TFAIL | TERRNO,
					"mmap() failed with unexpected errno");
			break;
		}

		if ((unsigned long)addr + CHUNK_SZ > ADDR_LIMIT) {
			tst_res(TFAIL, "mapping at %p + %zu exceeds 2GB limit",
				addr, CHUNK_SZ);
			addrs[num_chunks++] = addr;
			cleanup();
			return;
		}

		((char *)addr)[0] = 'a';
		((char *)addr)[CHUNK_SZ - 1] = 'z';

		addrs[num_chunks++] = addr;
	}

	if (!failed_with_enomem) {
		tst_res(TFAIL, "32-bit address space was not exhausted");
	} else if (!num_chunks) {
		tst_res(TFAIL, "failed to map any chunk with MAP_32BIT");
	} else {
		tst_res(TPASS,
			"Mapped %zu MB across %zu chunks within 2GB before ENOMEM",
			(num_chunks * CHUNK_SZ) / TST_MB, num_chunks);
	}

	cleanup();
}

static struct tst_test test = {
	.cleanup = cleanup,
	.test_all = run,
	.supported_archs = (const char *const []){
		"x86_64",
		NULL
	},
};
