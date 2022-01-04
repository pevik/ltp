// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2021 Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Test if key watch removal is correctly recognized by watch queue.
 */

#define _FCNTL_H

#include "tst_test.h"
#include "lapi/keyctl.h"
#include "common.h"

static int *keep_polling;

static struct watch_notification_filter meta_filter = {
	.nr_filters = 1,
	.__reserved = 0,
	.filters = {
		[0] = {
			.type = WATCH_TYPE_META,
			.subtype_filter[0] = UINT_MAX,
		},
	},
};

static void saw_watch_removal(struct watch_notification *n, size_t len, unsigned int wtype)
{
	if (wtype != WATCH_TYPE_META)
		return;

	if (n->subtype == WATCH_META_REMOVAL_NOTIFICATION)
		tst_res(TPASS, "Meta removal notification received");
	else
		tst_res(TFAIL, "Event not recognized");
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

	fd = wqueue_watch(256, &meta_filter);
	key = wqueue_add_key(fd);

	*keep_polling = 1;

	pid = SAFE_FORK();
	if (pid > 0) {
		wqueue_consumer(fd, keep_polling, saw_watch_removal);
		return;
	}

	/* if watch_id = -1 key is removed from the watch queue */
	safe_keyctl(KEYCTL_WATCH_KEY, key, fd, -1);

	*keep_polling = 0;

	SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.forks_child = 1
};
