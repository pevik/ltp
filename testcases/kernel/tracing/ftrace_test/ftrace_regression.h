/* SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (c) 2026 lufei <lufei@uniontech.com>
 *
 * Shared header for ftrace regression tests.
 */

#ifndef FTRACE_REGRESSION_H
#define FTRACE_REGRESSION_H

#include "tst_test.h"
#include "tst_safe_file_ops.h"
#include "tst_safe_stdio.h"

#include <string.h>
#include <regex.h>

#define FTRACE_LINE_BUF_SIZE 1024

/**
 * file_contains - Check if a file contains specific regex pattern.
 */
static inline int file_contains(const char *path, const char *pattern)
{
	FILE *fp = SAFE_FOPEN(path, "r");
	char *buf = SAFE_MALLOC(FTRACE_LINE_BUF_SIZE);
	bool found = false;

	regex_t re;

	if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) != 0) {
		SAFE_FCLOSE(fp);
		return found;
	}

	while (fgets(buf, FTRACE_LINE_BUF_SIZE, fp)) {
		if (regexec(&re, buf, 0, NULL, 0) == 0) {
			found = true;
			break;
		}
	}

	regfree(&re);
	SAFE_FCLOSE(fp);
	free(buf);
	return found;
}

#endif /* FTRACE_REGRESSION_H */
