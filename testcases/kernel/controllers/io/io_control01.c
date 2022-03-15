// SPDX-License-Identifier: GPL-2.0
/* [Description]
 *
 * Perform some I/O on a file and check if at least some of it is
 * recorded by the I/O controller.
 *
 * The exact amount of I/O performed is dependent on the file system,
 * page cache, scheduler and block driver. We call sync and drop the
 * file's page cache to force reading and writing. We also write
 * random data to try to prevent compression.
 *
 * The pagecache is a particular issue for reading. If the call to
 * fadvise is ignored then the data may only be read from the
 * cache. So that no I/O requests are made.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/sysmacros.h>
#include <mntent.h>

#include "tst_test.h"

static unsigned int dev_major, dev_minor;

static void run(void)
{
	int i, fd;
	char *line, *buf_ptr;
	const size_t pgsz = SAFE_SYSCONF(_SC_PAGESIZE);
	char *buf = SAFE_MALLOC(MAX((size_t)BUFSIZ, pgsz));
	unsigned long st_rbytes = 0, st_wbytes = 0, st_rios = 0, st_wios = 0;

	SAFE_CG_READ(tst_cg, "io.stat", buf, BUFSIZ - 1);
	line = strtok_r(buf, "\n", &buf_ptr);
	while (line) {
		unsigned int mjr, mnr;
		unsigned long dbytes, dios;

		const int convs =
			sscanf(line,
			       "%u:%u rbytes=%lu wbytes=%lu rios=%lu wios=%lu dbytes=%lu dios=%lu",
			       &mjr, &mnr, &st_rbytes, &st_wbytes, &st_rios, &st_wios,
			       &dbytes, &dios);

		if (convs < 2)
			continue;

		tst_res(TINFO, "Found %u:%u in io.stat", dev_major, dev_minor);

		if (mjr == dev_major || mnr == dev_minor)
			break;

		line = strtok_r(NULL, "\n", &buf_ptr);
	}

	SAFE_CG_PRINTF(tst_cg, "cgroup.procs", "%d", getpid());

	fd = SAFE_OPEN("/dev/urandom", O_RDONLY, 0600);
	SAFE_READ(1, fd, buf, pgsz);
	SAFE_CLOSE(fd);

	fd = SAFE_OPEN("mnt/dat", O_WRONLY | O_CREAT, 0600);

	for (i = 0; i < 4; i++) {
		SAFE_WRITE(1, fd, buf, pgsz);
		SAFE_FSYNC(fd);
		TST_EXP_PASS_SILENT(posix_fadvise(fd, pgsz * i, pgsz, POSIX_FADV_DONTNEED));
	}

	SAFE_CLOSE(fd);
	fd = SAFE_OPEN("mnt/dat", O_RDONLY, 0600);

	for (i = 0; i < 4; i++)
		SAFE_READ(1, fd, buf, pgsz);

	tst_res(TPASS, "Did some IO in the IO controller");

	SAFE_CG_READ(tst_cg, "io.stat", buf, BUFSIZ - 1);
	line = strtok_r(buf, "\n", &buf_ptr);
	while (line) {
		unsigned int mjr, mnr;
		unsigned long rbytes, wbytes, rios, wios, dbytes, dios;

		const int convs =
			sscanf(line,
			       "%u:%u rbytes=%lu wbytes=%lu rios=%lu wios=%lu dbytes=%lu dios=%lu",
			       &mjr, &mnr, &rbytes, &wbytes, &rios, &wios,
			       &dbytes, &dios);

		if (convs < 8)
			break;

		if (mjr != dev_major || mnr != dev_minor) {
			line = strtok_r(NULL, "\n", &buf_ptr);
			continue;
		}

		tst_res(TPASS, "Found %u:%u in io.stat", dev_major, dev_minor);
		TST_EXP_EXPR(rbytes > st_rbytes, "(rbytes=%lu) > (st_rbytes=%lu)", rbytes, st_rbytes);
		TST_EXP_EXPR(wbytes > st_wbytes, "(wbytes=%lu) > (st_wbytes=%lu)", wbytes, st_wbytes);
		TST_EXP_EXPR(rios > st_rios, "(rios=%lu) > (st_rios=%lu)", rios, st_rios);
		TST_EXP_EXPR(wios > st_wios, "(wios=%lu) > (st_wios=%lu)", wios, st_wios);

		goto out;
	}

	tst_res(TINFO, "io.stat:\n%s", buf);
	tst_res(TFAIL, "Did not find %u:%u in io.stat", dev_major, dev_minor);
out:
	free(buf);
	SAFE_CLOSE(fd);
	SAFE_UNLINK("mnt/dat");
}

static void setup(void)
{
	char buf[PATH_MAX] = { 0 };
	char *path = SAFE_GETCWD(buf, PATH_MAX - sizeof("mnt") - 1);
	struct mntent *mnt;
	FILE *mntf = setmntent("/proc/self/mounts", "r");
	struct stat st;

	strcpy(path + strlen(path), "/mnt");

	if (!mntf) {
		tst_brk(TBROK | TERRNO, "Can't open /proc/self/mounts");
		return;
	}

	mnt = getmntent(mntf);
	if (!mnt) {
		tst_brk(TBROK | TERRNO, "Can't read mounts or no mounts?");
		return;
	}

	do {
		if (strcmp(mnt->mnt_dir, path))
			continue;

		SAFE_STAT(mnt->mnt_fsname, &st);
		dev_major = major(st.st_rdev);
		dev_minor = minor(st.st_rdev);

		return;

	} while ((mnt = getmntent(mntf)));

	tst_brk(TBROK, "Could not find mount device");
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.needs_device = 1,
	.mntpoint = "mnt",
	.mount_device = 1,
	.all_filesystems = 1,
	.skip_filesystems = (const char *const[]){ "ntfs", "tmpfs", NULL },
	.needs_cgroup_ver = TST_CG_V2,
	.needs_cgroup_ctrls = (const char *const[]){ "io", NULL },
};
