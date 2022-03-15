// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) Huawei Technologies Co., Ltd., 2015
 * Copyright (C) 2022 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

#ifndef COMMON_H
#define COMMON_H

#include "tst_test.h"
#include "lapi/namespaces_constants.h"

#define UID_MAP 0
#define GID_MAP 1

static int dummy_child(void *v)
{
	(void)v;
	return 0;
}

static inline int check_newuser(void)
{
	int pid, status;

	if (tst_kvercmp(3, 8, 0) < 0)
		tst_brk(TCONF, "CLONE_NEWUSER not supported");

	pid = ltp_clone_quick(CLONE_NEWUSER | SIGCHLD, dummy_child, NULL);
	if (pid == -1)
		tst_brk(TCONF | TTERRNO, "CLONE_NEWUSER not supported");

	SAFE_WAIT(&status);

	return 0;
}

static inline void updatemap(int cpid, bool type, int idnum, int parentmappid)
{
	char path[BUFSIZ];
	char content[BUFSIZ];
	int fd;

	if (type == UID_MAP)
		sprintf(path, "/proc/%d/uid_map", cpid);
	else if (type == GID_MAP)
		sprintf(path, "/proc/%d/gid_map", cpid);
	else
		tst_brk(TBROK, "invalid type parameter");

	sprintf(content, "%d %d 1", idnum, parentmappid);

	fd = SAFE_OPEN(path, O_WRONLY, 0644);
	SAFE_WRITE(1, fd, content, strlen(content));
	SAFE_CLOSE(fd);
}

#endif
