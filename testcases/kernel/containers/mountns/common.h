// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2014 Red Hat, Inc.
 * Copyright (C) 2021 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef COMMON_H
#define COMMON_H

#include "tst_test.h"
#include "lapi/namespaces_constants.h"

#define DIRA "A"
#define DIRB "B"

static int dummy_child(void *v)
{
	(void)v;
	return 0;
}

static void check_newns(void)
{
	int pid, status;

	if (tst_kvercmp(2, 4, 19) < 0)
		tst_brk(TCONF, "CLONE_NEWNS not supported");

	pid = ltp_clone_quick(CLONE_NEWNS | SIGCHLD, dummy_child, NULL);
	if (pid < 0)
		tst_brk(TCONF, "CLONE_NEWNS not supported");

	SAFE_WAIT(&status);
}

static void umount_folders(void)
{
	if (tst_is_mounted(DIRA))
		SAFE_UMOUNT(DIRA);

	if (tst_is_mounted(DIRB))
		SAFE_UMOUNT(DIRB);
}

static void create_folders(void)
{
	SAFE_MKDIR(DIRA, 0777);
	SAFE_MKDIR(DIRB, 0777);
	SAFE_TOUCH(DIRA "/A", 0, NULL);
	SAFE_TOUCH(DIRB "/B", 0, NULL);
}

#endif
