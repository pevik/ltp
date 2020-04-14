// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef CLOCK_GETTIME_H
#define CLOCK_GETTIME_H

#include "tst_timer.h"
#include "tst_test.h"
#include "lapi/syscalls.h"

#ifdef TST_ABI32
static inline int libc_clock_gettime(clockid_t clk_id, void *tp)
{
	return clock_gettime(clk_id, tp);
}
#endif

static inline int sys_clock_gettime(clockid_t clk_id, void *tp)
{
	return tst_syscall(__NR_clock_gettime, clk_id, tp);
}

#if (__NR_clock_gettime64 != __LTP__NR_INVALID_SYSCALL)
static inline int sys_clock_gettime64(clockid_t clk_id, void *tp)
{
	return tst_syscall(__NR_clock_gettime64, clk_id, tp);
}
#endif

#endif /* CLOCK_GETTIME_H */
