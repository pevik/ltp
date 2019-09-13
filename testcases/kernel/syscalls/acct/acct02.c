// SPDX-License-Identifier: GPL-2.0-or-later
/*
 *  Copyright (c) SUSE LLC, 2019
 *  Author: Christian Amann <camann@suse.com>
 */
/*
 * This tests if the kernel writes correct data to the
 * process accounting file.
 *
 * First, system-wide process accounting is turned on and the output gets
 * directed to a defined file. After that a dummy program is run in order
 * to generate data and the process accounting gets turned off again.
 *
 * To verify the written data, the entries of the accounting file get
 * parsed into the corresponding acct structure. Since it cannot be guaranteed
 * that only the command issued by this test gets written into the accounting
 * file, the contents get parsed until the correct entry is found, or EOF
 * is reached.
 *
 * This is also accidental regression test for:
 * 4d9570158b626 kernel/acct.c: fix the acct->needcheck check in check_free_space()
 */

#include <sys/stat.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "tst_kconfig.h"
#include "tst_test.h"
#include "lapi/acct.h"
#include <stdio.h> // FIXME: debug

#define COMMAND		"acct02_helper"
#define OUTPUT_FILE	"acct_file"

#define UNPACK(x) ((x & 0x1fff) << (((x >> 13) & 0x7) * 3))

static int fd;
static int v3;
static int acct_size;
static int clock_ticks;
static unsigned int rc;
static unsigned int start_time;

static union acct_union {
	struct acct	v0;
	struct acct_v3	v3;
} acct_struct;

static int acct_version_is_3(void)
{
	const char *kconfig_acct_v3[] = {
		"CONFIG_BSD_PROCESS_ACCT_V3",
		NULL
	};

	struct tst_kconfig_res results[1];

	tst_kconfig_read(kconfig_acct_v3, results, 1);

	return results[0].match == 'y';
}

static void run_command(void)
{
	const char *const cmd[] = {COMMAND, NULL};

	rc = tst_run_cmd(cmd, NULL, NULL, 1) << 8;
}

static int verify_acct(struct acct *acc)
{
	int sys_time  = UNPACK(acc->ac_stime);
	int user_time = UNPACK(acc->ac_utime);
	int elap_time = UNPACK(acc->ac_etime);

	fprintf(stderr, "%s:%d %s(): sys_time: %d, user_time: %d, elap_time: %d\n",
			__FILE__, __LINE__, __func__, sys_time, user_time, elap_time); // FIXME: debug

	fprintf(stderr, "%s:%d %s(): strcmp(acc->ac_comm, COMMAND): '%s', '%s': %s\n", __FILE__, __LINE__, __func__, acc->ac_comm, COMMAND, strcmp(acc->ac_comm, COMMAND) ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): acc->ac_btime < start_time, (%d < %d): %s\n", __FILE__, __LINE__, __func__, acc->ac_btime,  start_time, acc->ac_btime < start_time ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): acc->ac_btime - start_time > 1 (%d < %d): %s\n", __FILE__, __LINE__, __func__, acc->ac_btime, start_time, acc->ac_btime - start_time > 1 ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): acc->ac_uid != getuid() (%d != %d): %s\n", __FILE__, __LINE__, __func__, acc->ac_uid, getuid(), acc->ac_uid != getuid() ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): acc->ac_gid != getgid() (%d != %d): %s\n", __FILE__, __LINE__, __func__, acc->ac_gid, getgid(), acc->ac_gid != getgid() ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): user_time/clock_ticks > 1 (%d): %s\n", __FILE__, __LINE__, __func__, user_time/clock_ticks, user_time/clock_ticks > 1 ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): sys_time/clock_ticks (%d): %s\n", __FILE__, __LINE__, __func__, sys_time/clock_ticks, sys_time/clock_ticks > 1? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): elap_time/clock_ticks >= 2 (%d) %s\n", __FILE__, __LINE__, __func__, elap_time/clock_ticks, elap_time/clock_ticks >= 2 ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): acc->ac_exitcode != rc (%d != %d): %s\n", __FILE__, __LINE__, __func__, acc->ac_exitcode, rc, acc->ac_exitcode != rc ? "yes" : "no"); // FIXME: debug

	if (strcmp(acc->ac_comm, COMMAND) ||
		acc->ac_btime < start_time ||
		acc->ac_btime - start_time > 1 ||
		acc->ac_uid != getuid() ||
		acc->ac_gid != getgid() ||
		user_time/clock_ticks > 1 ||
		sys_time/clock_ticks  > 1 ||
		elap_time/clock_ticks >= 2 ||
		acc->ac_exitcode != rc)
		return 0;
	return 1;
}

static int verify_acct_v3(struct acct_v3 *acc)
{
	int sys_time  = UNPACK(acc->ac_stime);
	int user_time = UNPACK(acc->ac_utime);
	int elap_time = acc->ac_etime;

	fprintf(stderr, "%s:%d %s(): sys_time: %d, user_time: %d, elap_time: %d\n",
			__FILE__, __LINE__, __func__, sys_time, user_time, elap_time); // FIXME: debug

	fprintf(stderr, "%s:%d %s(): strcmp(acc->ac_comm, COMMAND): '%s', '%s': %s\n", __FILE__, __LINE__, __func__, acc->ac_comm, COMMAND, strcmp(acc->ac_comm, COMMAND) ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): acc->ac_btime < start_time, (%d < %d): %s\n", __FILE__, __LINE__, __func__, acc->ac_btime,  start_time, acc->ac_btime < start_time ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): acc->ac_btime - start_time > 1 (%d < %d): %s\n", __FILE__, __LINE__, __func__, acc->ac_btime, start_time, acc->ac_btime - start_time > 1 ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): acc->ac_uid != getuid() (%d != %d): %s\n", __FILE__, __LINE__, __func__, acc->ac_uid, getuid(), acc->ac_uid != getuid() ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): acc->ac_gid != getgid() (%d != %d): %s\n", __FILE__, __LINE__, __func__, acc->ac_gid, getgid(), acc->ac_gid != getgid() ? "yes" : "no"); // FIXME: debug

	//  acc->ac_ppid != (uint32_t)getpid() ||
	fprintf(stderr, "%s:%d %s(): acc->ac_ppid != (uint32_t)getpid() (%d != %d): %s\n", __FILE__, __LINE__, __func__, acc->ac_ppid, (uint32_t)getpid(), acc->ac_ppid != (uint32_t)getpid() ? "yes" : "no"); // FIXME: debug

	fprintf(stderr, "%s:%d %s(): user_time/clock_ticks > 1 (%d): %s\n", __FILE__, __LINE__, __func__, user_time/clock_ticks, user_time/clock_ticks > 1 ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): sys_time/clock_ticks (%d): %s\n", __FILE__, __LINE__, __func__, sys_time/clock_ticks, sys_time/clock_ticks > 1? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): elap_time/clock_ticks >= 2 (%d) %s\n", __FILE__, __LINE__, __func__, elap_time/clock_ticks, elap_time/clock_ticks >= 2 ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): acc->ac_exitcode != rc (%d != %d): %s\n", __FILE__, __LINE__, __func__, acc->ac_exitcode, rc, acc->ac_exitcode != rc ? "yes" : "no"); // FIXME: debug

	// new
	fprintf(stderr, "%s:%d %s(): acc->ac_version != 3 (%d) %s\n", __FILE__, __LINE__, __func__, acc->ac_version, acc->ac_version != 3 ? "yes" : "no"); // FIXME: debug
	fprintf(stderr, "%s:%d %s(): acc->ac_pid < 1 (%d) %s\n", __FILE__, __LINE__, __func__, acc->ac_pid, acc->ac_pid < 1 ? "yes" : "no"); // FIXME: debug

	if (strcmp(acc->ac_comm, COMMAND) ||
		acc->ac_btime < start_time ||
		acc->ac_btime - start_time > 1 ||
		acc->ac_uid != getuid() ||
		acc->ac_gid != getgid() ||
		acc->ac_ppid != (uint32_t)getpid() || // new
		user_time/clock_ticks > 1 ||
		sys_time/clock_ticks  > 1 ||
		elap_time/clock_ticks >= 2 ||
		acc->ac_exitcode != rc ||
		acc->ac_version != 3 ||
		acc->ac_pid < 1)
		return 0;
	return 1;
}

static void run(void)
{
	int read_bytes, ret, entry_count;

	fd = SAFE_OPEN(OUTPUT_FILE, O_RDWR | O_CREAT, 0644);

	TEST(acct(OUTPUT_FILE));
	if (TST_RET == -1)
		tst_brk(TBROK | TTERRNO, "Could not set acct output file");

	start_time = time(NULL);
	run_command();
	acct(NULL);

	entry_count = 0;
	int i = 0; // FIXME: debug
	do {
		read_bytes = SAFE_READ(0, fd, &acct_struct, acct_size);

		if (v3)
			ret = verify_acct_v3(&acct_struct.v3);
		else
			ret = verify_acct(&acct_struct.v0);

		fprintf(stderr, "%s:%d %s(): %d: entry_count %d: ret: %d, read_bytes: %d, acct_size: %d\n",
			__FILE__, __LINE__, __func__, i, entry_count, ret, read_bytes, acct_size); // FIXME: debug

		if (read_bytes)
			entry_count++;
		i++; // FIXME: debug
	} while (read_bytes == acct_size && !ret);

	tst_res(TINFO, "Number of accounting file entries tested: %d",
			entry_count);

	if (ret)
		tst_res(TPASS, "acct() wrote correct file contents!");
	else
		tst_res(TFAIL, "acct() wrote incorrect file contents!");

	SAFE_CLOSE(fd);
}

static void setup(void)
{
	struct statfs buf;

	clock_ticks = SAFE_SYSCONF(_SC_CLK_TCK);

	SAFE_STATFS(".", &buf);

	float avail = (100.00 * buf.f_bavail) / buf.f_blocks;

	/* The accounting data are silently discarded on nearly FS */
	if (avail < 4.1) {
		tst_brk(TCONF,
			"Less than 4.1%% (%.2f) of free space on filesystem",
			avail);
	}

	TEST(acct(NULL));
	if (TST_RET == -1)
		tst_brk(TBROK | TTERRNO,
			"acct() system call returned with error");

	v3 = acct_version_is_3();
	if (v3) {
		tst_res(TINFO, "Verifying using 'struct acct_v3'");
		acct_size = sizeof(struct acct_v3);
	} else {
		tst_res(TINFO, "Verifying using 'struct acct'");
		acct_size = sizeof(struct acct);
	}
}

static void cleanup(void)
{
	if (fd > 0)
		SAFE_CLOSE(fd);
	acct(NULL);
}

static const char *kconfigs[] = {
	"CONFIG_BSD_PROCESS_ACCT",
	NULL
};

static struct tst_test test = {
	.test_all = run,
	.needs_kconfigs = kconfigs,
	.setup = setup,
	.cleanup = cleanup,
	.needs_tmpdir = 1,
	.needs_root = 1,
};
