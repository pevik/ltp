// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 SUSE LLC <rpalethorpe@suse.com>
 */

#ifndef TST_SAFE_FILE_AT_H
#define TST_SAFE_FILE_AT_H

#include <sys/types.h>
#include <stdarg.h>

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

#endif
