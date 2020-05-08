// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef SEMOP_VAR__
#define SEMOP_VAR__

#include <sys/sem.h>
#include "tst_timer.h"
#include "lapi/abisize.h"

static inline int sys_semtimedop(int semid, struct sembuf *sops, size_t nsops,
		void *timeout)
{
	return tst_syscall(__NR_semtimedop, semid, sops, nsops, timeout);
}

static inline int sys_semtimedop_time64(int semid, struct sembuf *sops,
					size_t nsops, void *timeout)
{
	return tst_syscall(__NR_semtimedop_time64, semid, sops, nsops, timeout);
}

struct test_variants {
	int (*semop)(int semid, struct sembuf *sops, size_t nsops);
	int (*semtimedop)(int semid, struct sembuf *sops, size_t nsops, void *timeout);
	enum tst_ts_type type;
	char *desc;
} variants[] = {
	{ .semop = semop, .type = TST_LIBC_TIMESPEC, .desc = "semop: vDSO or syscall"},
#if defined(TST_ABI32)
	{ .semtimedop = sys_semtimedop, .type = TST_LIBC_TIMESPEC, .desc = "semtimedop: syscall with libc spec"},
	{ .semtimedop = sys_semtimedop, .type = TST_KERN_OLD_TIMESPEC, .desc = "semtimedop: syscall with kernel spec32"},
#endif

#if defined(TST_ABI64)
	{ .semtimedop = sys_semtimedop, .type = TST_KERN_TIMESPEC, .desc = "semtimedop: syscall with kernel spec64"},
#endif

#if (__NR_semtimedop_time64 != __LTP__NR_INVALID_SYSCALL)
	{ .semtimedop = sys_semtimedop_time64, .type = TST_KERN_TIMESPEC, .desc = "semtimedop: syscall time64 with kernel spec64"},
#endif
};

static inline int call_semop(struct test_variants *tv, int semid,
		struct sembuf *sops, size_t nsops, struct tst_ts *timeout)
{
	if (tv->semop)
		return tv->semop(semid, sops, nsops);

	return tv->semtimedop(semid, sops, nsops, tst_ts_get(timeout));
}

#endif /* SEMOP_VAR__ */
