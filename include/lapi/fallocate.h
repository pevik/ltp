// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) International Business Machines  Corp., 2007
 * Copyright (c) 2014 Fujitsu Ltd.
 */

#ifndef LAPI_FALLOCATE_H__
#define LAPI_FALLOCATE_H__

#define _GNU_SOURCE

#include "config.h"
#include <sys/types.h>
#include <endian.h>

#include "lapi/abisize.h"
#include "lapi/fcntl.h"
#include "lapi/seek.h"
#include "lapi/syscalls.h"

#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"

#ifndef FALLOC_FL_KEEP_SIZE
# define FALLOC_FL_KEEP_SIZE 0x01
#endif

#ifndef FALLOC_FL_PUNCH_HOLE
# define FALLOC_FL_PUNCH_HOLE 0x02
#endif

#ifndef FALLOC_FL_COLLAPSE_RANGE
# define FALLOC_FL_COLLAPSE_RANGE 0x08
#endif

#ifndef FALLOC_FL_ZERO_RANGE
# define FALLOC_FL_ZERO_RANGE 0x10
#endif

#ifndef FALLOC_FL_INSERT_RANGE
# define FALLOC_FL_INSERT_RANGE 0x20
#endif

#ifndef HAVE_FALLOCATE
static inline long fallocate(int fd, int mode, loff_t offset, loff_t len)
{
	/* Deal with 32bit ABIs that have 64bit syscalls. */
# if LTP_USE_64_ABI
	return tst_syscall(__NR_fallocate, fd, mode, offset, len);
# else
	return (long)tst_syscall(__NR_fallocate, fd, mode,
				 __LONG_LONG_PAIR((off_t) (offset >> 32),
						  (off_t) offset),
				 __LONG_LONG_PAIR((off_t) (len >> 32),
						  (off_t) len));
# endif
}
#endif /* HAVE_FALLOCATE */

#define SAFE_FALLOCATE(fd, mode, offset, len) \
	safe_fallocate(__FILE__, __LINE__, (path), (mode), (offset), (len), #mode)

/*
 * static inlinde due off_t, see tst_safe_macros_inline.h.
 * Kept separate because fallocate() requires _GNU_SOURCE.
 */
static inline int safe_fallocate(const char *file, const int lineno,
	int fd, int mode, off_t offset, off_t len, const char *smode)
{
	int rval;

	rval = fallocate(fd, mode, offset, len);

	if (rval == -1) {
		if (tst_fs_type(".") == TST_NFS_MAGIC && (errno == EOPNOTSUPP ||
							  errno == ENOSYS)) {
			tst_brk_(file, lineno, TCONF | TERRNO,
					 "fallocate(%d, %s, %ld, %ld) unsupported",
					 fd, smode, (long)offset, (long)len);
		}
		tst_brk_(file, lineno, TBROK | TERRNO,
				 "fallocate(%d, %s, %ld, %ld) failed",
				 fd, smode, (long)offset, (long)len);
	} else if (rval < 0) {
		tst_brk_(file, lineno, TBROK | TERRNO,
			"Invalid fallocate(%d, %s, %ld, %ld) return value %d",
				 fd, smode, (long)offset, (long)len, rval);
	}

	return rval;
}

#endif /* LAPI_FALLOCATE_H__ */
