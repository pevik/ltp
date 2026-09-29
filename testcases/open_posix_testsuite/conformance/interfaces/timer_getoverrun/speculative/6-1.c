/*
 * Copyright (c) 2002, Intel Corporation. All rights reserved.
 * Created by:  julie.n.fleischer REMOVE-THIS AT intel DOT com
 * This file is licensed under the GPL license.  For the full content
 * of this license, see the COPYING file at the top level of this
 * source tree.
 *
 * Test to see if timer_getoverrun() sets errno==EINVAL for an invalid
 * timerid. Since this is a "may" assertion, succeeding or crashing
 * are equally valid outcomes.
 */

#include <time.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include "posixtest.h"

#define BOGUSTID 9999

int test_main(int argc PTS_ATTRIBUTE_UNUSED, char **argv PTS_ATTRIBUTE_UNUSED)
{
	pid_t pid;
	int status;

	pid = fork();
	if (pid == -1) {
		perror("fork");
		return PTS_UNRESOLVED;
	}

	if (pid == 0) {
		timer_t tid = (timer_t) BOGUSTID;

		if (timer_getoverrun(tid) == -1) {
			if (errno == EINVAL)
				_exit(PTS_PASS);
			_exit(PTS_FAIL);
		}
		_exit(PTS_PASS);
	}

	if (waitpid(pid, &status, 0) == -1) {
		perror("waitpid");
		return PTS_UNRESOLVED;
	}

	if (WIFSIGNALED(status) || (WIFEXITED(status) && WEXITSTATUS(status) == 0)) {
		printf("Test PASSED\n");
		return PTS_PASS;
	}

	printf("Test FAILED\n");
	return PTS_FAIL;
}
