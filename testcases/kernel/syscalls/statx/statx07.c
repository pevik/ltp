// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (c) Zilogic Systems Pvt. Ltd., 2018
 *  Email : code@zilogic.com
 */

/*
 * Test statx
 *
 * This code tests the following flags:
 * 1) AT_STATX_FORCE_SYNC
 * 2) AT_STATX_DONT_SYNC
 *
 * By exportfs cmd creating NFS setup.
 *
 * A test file is created in server folder and statx is being
 * done in client folder.
 *
 * TESTCASE 1:
 * BY AT_STATX_SYNC_AS_STAT getting predefined mode value.
 * Then, by using AT_STATX_FORCE_SYNC getting new updated vaue
 * from server file changes.
 *
 * TESTCASE 2:
 * BY AT_STATX_SYNC_AS_STAT getting predefined mode value.
 * AT_STATX_FORCE_SYNC is called to create cache data of the file.
 * Then, by using DONT_SYNC_FILE getting old cached data in client folder,
 * but mode has been chaged in server file.
 *
 * Minimum kernel version required is 4.11.
 */

#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <sys/mount.h>
#include "tst_test.h"
#include "lapi/stat.h"

#define MODE(X) (X & (~S_IFMT))
#define F_PATH_SIZE 50
#define BUFF_SIZE (PATH_MAX + 50)
#define DEFAULT_MODE 0644
#define CURRENT_MODE 0777

#define CLIENT_PATH "client"
#define SERVER_PATH "server"
#define FORCE_SYNC_FILE  "force_sync_file"
#define DONT_SYNC_FILE "dont_sync_file"

static char cwd[PATH_MAX];
static char cmd[BUFF_SIZE];
static char server_path[BUFF_SIZE];
static char client_path[BUFF_SIZE];
static int mounted;

static int get_mode(char *file_name, int flag_type, char *flag_name)
{
	char client_path[F_PATH_SIZE];
	struct statx buff;

	snprintf(client_path, sizeof(client_path), "%s/%s",
		 CLIENT_PATH, file_name);

	TEST(statx(AT_FDCWD, client_path, flag_type, STATX_ALL, &buff));

	if (TST_RET == -1)
		tst_brk(TFAIL | TST_ERR,
			"statx(AT_FDCWD, %s, %s, STATX_ALL, &buff)",
			file_name, flag_name);
	else
		tst_res(TINFO, "statx(AT_FDCWD, %s, %s, STATX_ALL, &buff)",
			file_name, flag_name);

	return buff.stx_mode;
}

static void test_for_dont_sync(void)
{
	unsigned int cur_mode;
	char server_path[F_PATH_SIZE];

	snprintf(server_path, sizeof(server_path), "%s/%s", SERVER_PATH,
		 DONT_SYNC_FILE);

	get_mode(DONT_SYNC_FILE, AT_STATX_FORCE_SYNC,
		 "AT_STATX_FORCE_SYNC");

	SAFE_CHMOD(server_path, CURRENT_MODE);
	cur_mode = get_mode(DONT_SYNC_FILE, AT_STATX_DONT_SYNC,
			    "AT_STATX_DONT_SYNC");

	if (MODE(cur_mode) == DEFAULT_MODE)
		tst_res(TPASS,
			"statx() with AT_STATX_DONT_SYNC for mode %o",
			DEFAULT_MODE);
	else
		tst_res(TFAIL,
			"statx() with AT_STATX_DONT_SYNC for mode %o: %o",
			DEFAULT_MODE, MODE(cur_mode));
}

static void test_for_force_sync(void)
{
	unsigned int cur_mode;
	char server_path[F_PATH_SIZE];

	snprintf(server_path, sizeof(server_path), "%s/%s", SERVER_PATH,
		 FORCE_SYNC_FILE);

	SAFE_CHMOD(server_path, CURRENT_MODE);
	cur_mode = get_mode(FORCE_SYNC_FILE, AT_STATX_FORCE_SYNC,
			    "AT_STATX_FORCE_SYNC");

	if (MODE(cur_mode) == CURRENT_MODE)
		tst_res(TPASS,
			"statx() with AT_STATX_FORCE_SYNC for mode %o",
			CURRENT_MODE);
	else
		tst_res(TFAIL,
			"statx() with AT_STATX_FORCE_SYNC for mode %o: %o",
			CURRENT_MODE, MODE(cur_mode));
}

const struct test_cases {
	void (*func)(void);
} tcases[] = {
	{test_for_dont_sync},
	{test_for_force_sync}
};

static void test_statx(unsigned int nr)
{
	tcases[nr].func();
}

static void setup(void)
{
	int ret;
	char force_sync_file[F_PATH_SIZE];
	char dont_sync_file[F_PATH_SIZE];
	char mount_data[F_PATH_SIZE];
	char *ip = "127.0.0.1";

	TESTPTR(getcwd(cwd, PATH_MAX));
	if (TST_RET_PTR == NULL)
		tst_brk(TBROK | TST_ERR, "Failed to get PWD");

	snprintf(force_sync_file, sizeof(force_sync_file), "%s/%s",
		 SERVER_PATH, FORCE_SYNC_FILE);
	snprintf(dont_sync_file, sizeof(dont_sync_file), "%s/%s",
		 SERVER_PATH, DONT_SYNC_FILE);

	SAFE_MKDIR(SERVER_PATH, DEFAULT_MODE);
	SAFE_MKDIR(CLIENT_PATH, DEFAULT_MODE);
	SAFE_CREAT(force_sync_file, DEFAULT_MODE);
	SAFE_CREAT(dont_sync_file, DEFAULT_MODE);

	snprintf(server_path, sizeof(server_path), ":%s/%s", cwd, SERVER_PATH);
	snprintf(client_path, sizeof(client_path), "%s/%s", cwd, CLIENT_PATH);
	snprintf(mount_data, sizeof(mount_data), "addr=%s", ip);

	snprintf(cmd, sizeof(cmd),
		 "exportfs -i -o no_root_squash,rw,sync,no_subtree_check *%.1024s",
		 server_path);

	ret = tst_system(cmd);
	if (WEXITSTATUS(ret) == 127)
		tst_brk(TCONF | TST_ERR, "%s not found", cmd);
	if (ret == -1)
		tst_brk(TBROK | TST_ERR, "failed to exportfs");

	SAFE_MOUNT(server_path, client_path, "nfs", 0, mount_data);
	mounted = 1;
}

static void cleanup(void)
{
	snprintf(cmd, sizeof(cmd), "exportfs -u *%.1024s", server_path);
	if (tst_system(cmd) == -1)
		tst_res(TWARN | TST_ERR, "failed to clear exportfs");

	if (mounted)
		SAFE_UMOUNT(CLIENT_PATH);
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(tcases),
	.test = test_statx,
	.setup = setup,
	.cleanup = cleanup,
	.min_kver = "4.11",
	.needs_tmpdir = 1,
	.dev_fs_type = "nfs",
	.needs_root = 1,
};
