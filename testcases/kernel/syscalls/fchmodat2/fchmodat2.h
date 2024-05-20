/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef FCHMODAT2_H
#define FCHMODAT2_H

#include "tst_test.h"
#include "tst_safe_file_at.h"

static inline void verify_mode(int dirfd, const char *path, mode_t mode)
{
	struct stat st;

	SAFE_FSTATAT(dirfd, path, &st, AT_SYMLINK_NOFOLLOW);
	TST_EXP_EQ_LI(st.st_mode, mode);
}

#endif
