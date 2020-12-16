#include <stdio.h>
#include <stdlib.h>
#include "lapi/fcntl.h"

#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"

int safe_openat(const char *file, const int lineno,
		int dirfd, const char *path, int oflags, ...)
{
	va_list ap;
	int fd;
	mode_t mode;

	va_start(ap, oflags);
	mode = va_arg(ap, int);
	va_end(ap);

	fd = openat(dirfd, path, oflags, mode);

	if (fd < 0) {
		tst_brk(TBROK | TERRNO, "%s:%d: openat(%d, \"%s\", O_RDONLY)",
			file, lineno, dirfd, path);
	}

	return fd;
}

ssize_t safe_file_readat(const char *file, const int lineno,
			 int dirfd, const char *path, char *buf, size_t nbyte)
{
	int fd = safe_openat(file, lineno, dirfd, path, O_RDONLY);
	ssize_t rval = safe_read(file, lineno, NULL, 0, fd, buf, nbyte - 1);

	close(fd);
	buf[rval] = '\0';

	if (rval >= (ssize_t)nbyte - 1) {
		tst_brk(TBROK,
			"%s:%d: Buffer length %zu too small to read (%d)/%s",
			file, lineno, nbyte, dirfd, path);
	}

	return rval;
}

int tst_file_vprintfat(int dirfd, const char *path, const char *fmt, va_list va)
{
	int fd = openat(dirfd, path, O_WRONLY);

	if (fd < 0) {
		tst_res(TINFO | TERRNO,
			"openat(%d, %s, O_WRONLY)", dirfd, path);
		return fd;
	}

	TEST(vdprintf(fd, fmt, va));
	close(fd);

	if (TST_RET < 0)
		tst_res(TINFO | TTERRNO, "vdprintf(%d, %s, ...)", fd, fmt);
	errno = TST_ERR;

	return TST_RET;
}

int tst_file_printfat(int dirfd, const char *path, char *fmt, ...)
{
	va_list va;
	int rval;

	va_start(va, fmt);
	rval = tst_file_vprintfat(dirfd, path, fmt, va);
	va_end(va);

	return rval;
}

int safe_file_vprintfat(const char *file, const int lineno,
			int dirfd, const char *path,
			const char *fmt, va_list va)
{
	int rval, fd = safe_openat(file, lineno, dirfd, path, O_WRONLY);

	if (fd < 1)
		return fd;

	rval = vdprintf(fd, fmt, va);

	if (rval > -1)
		return rval;

	tst_brk(TBROK | TERRNO, "%s:%d: printfat((%d)/%s, %s, ...)",
		file, lineno, dirfd, path, fmt);

	return 0;
}

int safe_file_printfat(const char *file, const int lineno,
		       int dirfd, const char *path, char *fmt, ...)
{
	va_list va;
	int rval;

	va_start(va, fmt);
	rval = safe_file_vprintfat(file, lineno, dirfd, path, fmt, va);
	va_end(va);

	return rval;
}

int safe_unlinkat(const char *file, const int lineno,
		  int dirfd, const char *path, int flags)
{
	int rval = unlinkat(dirfd, path, flags);

	if (rval < 0) {
		tst_brk(TBROK | TERRNO,
			"%s:%d: unlinkat(%d, %s, %s)", file, lineno,
			dirfd, path,
			flags == AT_REMOVEDIR ? "AT_REMOVEDIR" :
						(flags ? "?" : "0"));
	}

	return rval;
}
