// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Test if keyctl revoke is correctly recognized by watch queue.
 */

#define _FCNTL_H

#include "tst_test.h"
#include "lapi/keyctl.h"
#include "common.h"

static int *keep_polling;

static void saw_key_revoked(struct watch_notification *n, size_t len, unsigned int wtype)
{
	if (wqueue_key_event(n, len, wtype, NOTIFY_KEY_REVOKED))
		tst_res(TPASS, "keyctl revoke has been recognized");
	else
		tst_res(TFAIL, "keyctl revoke has not been recognized");
}

static void setup(void)
{
	keep_polling = SAFE_MMAP(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

static void cleanup(void)
{
	*keep_polling = 0;
	SAFE_MUNMAP(keep_polling, sizeof(int));
}

static void run(void)
{
	int fd;
	int pid;
	key_serial_t key;

	fd = wqueue_watch(256, &wqueue_key_filter);
	key = wqueue_add_key(fd);

	*keep_polling = 1;

	pid = SAFE_FORK();
	if (pid > 0) {
		wqueue_consumer(fd, keep_polling, saw_key_revoked);
		return;
	}

	safe_keyctl(KEYCTL_REVOKE, key);

	*keep_polling = 0;

	SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.forks_child = 1
};
