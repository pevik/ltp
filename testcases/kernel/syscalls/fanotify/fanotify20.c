// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2021 Collabora Ltd.
 *
 * Author: Gabriel Krisman Bertazi <gabriel@krisman.be>
 * Based on previous work by Amir Goldstein <amir73il@gmail.com>
 */

/*\
 * [Description]
 * Check fanotify FAN_ERROR_FS events triggered by intentionally
 * corrupted filesystems:
 *
 * - Generate a broken filesystem
 * - Start FAN_FS_ERROR monitoring group
 * - Make the file system notice the error through ordinary operations
 * - Observe the event generated
 */

#define _GNU_SOURCE
#include "config.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/syscall.h>
#include "tst_test.h"
#include <sys/fanotify.h>
#include <sys/types.h>
#include <fcntl.h>

#ifdef HAVE_SYS_FANOTIFY_H
#include "fanotify.h"

#ifndef FAN_FS_ERROR
#define FAN_FS_ERROR		0x00008000

#define FAN_EVENT_INFO_TYPE_ERROR	4

#ifndef FILEID_INVALID
#define	FILEID_INVALID		0xff
#endif

#ifndef FILEID_INO32_GEN
#define FILEID_INO32_GEN	1
#endif

struct fanotify_event_info_error {
	struct fanotify_event_info_header hdr;
	__s32 error;
	__u32 error_count;
};
#endif

#define BUF_SIZE 256
static char event_buf[BUF_SIZE];
int fd_notify;

#define MOUNT_PATH "test_mnt"

#define EXT4_ERR_ESHUTDOWN 16

static void do_debugfs_request(const char *dev, char *request)
{
	char *cmd[] = {"debugfs", "-w", dev, "-R", request, NULL};

	SAFE_CMD(cmd, NULL, NULL);
}

static void trigger_fs_abort(void)
{
	SAFE_MOUNT(tst_device->dev, MOUNT_PATH, tst_device->fs_type,
		   MS_REMOUNT|MS_RDONLY, "abort");
}

#define TCASE2_BASEDIR "tcase2"
#define TCASE2_BAD_DIR TCASE2_BASEDIR"/bad_dir"

static unsigned int tcase2_bad_ino;
static void tcase2_prepare_fs(void)
{
	struct stat stat;

	SAFE_MKDIR(MOUNT_PATH"/"TCASE2_BASEDIR, 0777);
	SAFE_MKDIR(MOUNT_PATH"/"TCASE2_BAD_DIR, 0777);

	SAFE_STAT(MOUNT_PATH"/"TCASE2_BAD_DIR, &stat);
	tcase2_bad_ino = stat.st_ino;

	SAFE_UMOUNT(MOUNT_PATH);
	do_debugfs_request(tst_device->dev, "sif " TCASE2_BAD_DIR " mode 0xff");
	SAFE_MOUNT(tst_device->dev, MOUNT_PATH, tst_device->fs_type, 0, NULL);
}

static void tcase2_trigger_lookup(void)
{
	int ret;

	/* SAFE_OPEN cannot be used here because we expect it to fail. */
	ret = open(MOUNT_PATH"/"TCASE2_BAD_DIR, O_RDONLY, 0);
	if (ret != -1 && errno != EUCLEAN)
		tst_res(TFAIL, "Unexpected lookup result(%d) of %s (%d!=%d)",
			ret, TCASE2_BAD_DIR, errno, EUCLEAN);
}

static const struct test_case {
	char *name;
	int error;
	unsigned int error_count;

	/* inode can be null for superblock errors */
	unsigned int *inode;
	void (*trigger_error)(void);
	void (*prepare_fs)(void);
} testcases[] = {
	{
		.name = "Trigger abort",
		.trigger_error = &trigger_fs_abort,
		.error_count = 1,
		.error = EXT4_ERR_ESHUTDOWN,
		.inode = NULL
	},
	{
		.name = "Lookup of inode with invalid mode",
		.prepare_fs = tcase2_prepare_fs,
		.trigger_error = &tcase2_trigger_lookup,
		.error_count = 1,
		.error = 0,
		.inode = &tcase2_bad_ino,
	}
};

struct fanotify_event_info_header *get_event_info(
					struct fanotify_event_metadata *event,
					int info_type)
{
	struct fanotify_event_info_header *hdr = NULL;
	char *start = (char *) event;
	int off;

	for (off = event->metadata_len; (off+sizeof(*hdr)) < event->event_len;
	     off += hdr->len) {
		hdr = (struct fanotify_event_info_header *) &(start[off]);
		if (hdr->info_type == info_type)
			return hdr;
	}
	return NULL;
}

#define get_event_info_error(event)					\
	((struct fanotify_event_info_error *)				\
	 get_event_info((event), FAN_EVENT_INFO_TYPE_ERROR))

#define get_event_info_fid(event)					\
	((struct fanotify_event_info_fid *)				\
	 get_event_info((event), FAN_EVENT_INFO_TYPE_FID))

int check_error_event_info_fid(struct fanotify_event_info_fid *fid,
				 const struct test_case *ex)
{
	int fail = 0;
	struct file_handle *fh = (struct file_handle *) &fid->handle;

	if (!ex->inode) {
		uint32_t *h = (uint32_t *) fh->f_handle;

		if (!(fh->handle_type == FILEID_INVALID && !h[0] && !h[1])) {
			tst_res(TFAIL, "%s: file handle should have been invalid",
				ex->name);
			fail++;
		}
		return fail;
	} else if (fh->handle_type == FILEID_INO32_GEN) {
		uint32_t *h = (uint32_t *) fh->f_handle;

		if (h[0] != *ex->inode) {
			tst_res(TFAIL,
				"%s: Unexpected file handle inode (%u!=%u)",
				ex->name, *ex->inode, h[0]);
			fail++;
		}
	} else {
		tst_res(TFAIL, "%s: Test can't handle received FH type (%d)",
			ex->name, fh->handle_type);
	}

	return fail;
}

int check_error_event_info_error(struct fanotify_event_info_error *info_error,
				 const struct test_case *ex)
{
	int fail = 0;

	if (info_error->error_count != ex->error_count) {
		tst_res(TFAIL, "%s: Unexpected error_count (%d!=%d)",
			ex->name, info_error->error_count, ex->error_count);
		fail++;
	}

	if (info_error->error != ex->error) {
		tst_res(TFAIL, "%s: Unexpected error code value (%d!=%d)",
			ex->name, info_error->error, ex->error);
		fail++;
	}

	return fail;
}

int check_error_event_metadata(struct fanotify_event_metadata *event)
{
	int fail = 0;

	if (event->mask != FAN_FS_ERROR) {
		fail++;
		tst_res(TFAIL, "got unexpected event %llx",
			(unsigned long long)event->mask);
	}

	if (event->fd != FAN_NOFD) {
		fail++;
		tst_res(TFAIL, "Weird FAN_FD %llx",
			(unsigned long long)event->mask);
	}
	return fail;
}

void check_event(char *buf, size_t len, const struct test_case *ex)
{
	struct fanotify_event_metadata *event =
		(struct fanotify_event_metadata *) buf;
	struct fanotify_event_info_error *info_error;
	struct fanotify_event_info_fid *info_fid;
	int fail = 0;

	if (len < FAN_EVENT_METADATA_LEN)
		tst_res(TFAIL, "No event metadata found");

	if (check_error_event_metadata(event))
		return;

	info_error = get_event_info_error(event);
	fail += info_error ? check_error_event_info_error(info_error, ex) : 1;

	info_fid = get_event_info_fid(event);
	fail += info_fid ? check_error_event_info_fid(info_fid, ex) : 1;

	if (!fail)
		tst_res(TPASS, "Successfully received: %s", ex->name);
}

static void do_test(unsigned int i)
{
	const struct test_case *tcase = &testcases[i];
	size_t read_len;

	tcase->trigger_error();

	read_len = SAFE_READ(0, fd_notify, event_buf, BUF_SIZE);

	check_event(event_buf, read_len, tcase);
}

static void setup(void)
{
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(testcases); i++)
		if (testcases[i].prepare_fs)
			testcases[i].prepare_fs();

	fd_notify = SAFE_FANOTIFY_INIT(FAN_CLASS_NOTIF|FAN_REPORT_FID,
				       O_RDONLY);

	SAFE_FANOTIFY_MARK(fd_notify, FAN_MARK_ADD|FAN_MARK_FILESYSTEM,
			   FAN_FS_ERROR, AT_FDCWD, MOUNT_PATH);
}

static void cleanup(void)
{
	if (fd_notify > 0)
		SAFE_CLOSE(fd_notify);
}

static struct tst_test test = {
	.test = do_test,
	.tcnt = ARRAY_SIZE(testcases),
	.setup = setup,
	.cleanup = cleanup,
	.mount_device = 1,
	.mntpoint = MOUNT_PATH,
	.all_filesystems = 0,
	.needs_root = 1,
	.dev_fs_type = "ext4"

};

#else
	TST_TEST_TCONF("system doesn't have required fanotify support");
#endif
