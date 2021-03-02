/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2012 Cyril Hrubis chrubis@suse.cz
 */

#ifndef TST_SAFE_FILE_OPS
#define TST_SAFE_FILE_OPS

#include "safe_file_ops_fn.h"

#define FILE_SCANF(path, fmt, ...) \
	file_scanf(__FILE__, __LINE__, (path), (fmt), ## __VA_ARGS__)

#define SAFE_FILE_SCANF(path, fmt, ...) \
	safe_file_scanf(__FILE__, __LINE__, NULL, \
	                (path), (fmt), ## __VA_ARGS__)

#define FILE_LINES_SCANF(path, fmt, ...) \
	file_lines_scanf(__FILE__, __LINE__, NULL, 0,\
			(path), (fmt), ## __VA_ARGS__)

#define SAFE_FILE_LINES_SCANF(path, fmt, ...) \
	file_lines_scanf(__FILE__, __LINE__, NULL, 1,\
			(path), (fmt), ## __VA_ARGS__)

#define SAFE_READ_MEMINFO(item) \
       ({long tst_rval; \
        SAFE_FILE_LINES_SCANF("/proc/meminfo", item " %ld", \
                        &tst_rval); \
        tst_rval;})

#define SAFE_READ_PROC_STATUS(pid, item) \
       ({long tst_rval_; \
        char tst_path_[128]; \
        sprintf(tst_path_, "/proc/%d/status", pid); \
        SAFE_FILE_LINES_SCANF(tst_path_, item " %ld", \
                        &tst_rval_); \
        tst_rval_;})

#define FILE_PRINTF(path, fmt, ...) \
	file_printf(__FILE__, __LINE__, \
		    (path), (fmt), ## __VA_ARGS__)

#define SAFE_FILE_PRINTF(path, fmt, ...) \
	safe_file_printf(__FILE__, __LINE__, NULL, \
	                 (path), (fmt), ## __VA_ARGS__)

#define SAFE_CP(src, dst) \
	safe_cp(__FILE__, __LINE__, NULL, (src), (dst))

#define SAFE_TOUCH(pathname, mode, times) \
	safe_touch(__FILE__, __LINE__, NULL, \
			(pathname), (mode), (times))

#define SAFE_MOUNT_OVERLAY() \
	((void) mount_overlay(__FILE__, __LINE__, 1))

#define TST_MOUNT_OVERLAY() \
	(mount_overlay(__FILE__, __LINE__, 0) == 0)

#define SAFE_OPENAT(dirfd, path, oflags, ...)			\
	safe_openat(__FILE__, __LINE__,				\
		    (dirfd), (path), (oflags), ## __VA_ARGS__)

#define SAFE_FILE_READAT(dirfd, path, buf, nbyte)			\
	safe_file_readat(__FILE__, __LINE__,				\
			 (dirfd), (path), (buf), (nbyte))


#define SAFE_FILE_PRINTFAT(dirfd, path, fmt, ...)			\
	safe_file_printfat(__FILE__, __LINE__,				\
			   (dirfd), (path), (fmt), __VA_ARGS__)

#define SAFE_UNLINKAT(dirfd, path, flags)				\
	safe_unlinkat(__FILE__, __LINE__, (dirfd), (path), (flags))

char *tst_decode_fd(int fd);

int safe_openat(const char *file, const int lineno,
		int dirfd, const char *path, int oflags, ...);

ssize_t safe_file_readat(const char *file, const int lineno,
			 int dirfd, const char *path, char *buf, size_t nbyte);

int tst_file_vprintfat(int dirfd, const char *path, const char *fmt, va_list va);
int tst_file_printfat(int dirfd, const char *path, const char *fmt, ...)
			__attribute__ ((format (printf, 3, 4)));

int safe_file_vprintfat(const char *file, const int lineno,
			int dirfd, const char *path,
			const char *fmt, va_list va);

int safe_file_printfat(const char *file, const int lineno,
		       int dirfd, const char *path, const char *fmt, ...)
			__attribute__ ((format (printf, 5, 6)));

int safe_unlinkat(const char *file, const int lineno,
		  int dirfd, const char *path, int flags);

#endif /* TST_SAFE_FILE_OPS */
