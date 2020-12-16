// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Red Hat, Inc.
 * Copyright (c) 2020 Li Wang <liwang@redhat.com>
 * Copyright (c) 2020 Richard Palethorpe <rpalethorpe@suse.com>
 */

#define TST_NO_DEFAULT_MAIN

#include <stdio.h>
#include <mntent.h>
#include <sys/mount.h>

#include "tst_test.h"
#include "lapi/fcntl.h"
#include "lapi/mount.h"
#include "tst_cgroup.h"

/* At most we can have one cgroup V1 tree for each controller and one
 * (empty) v2 tree.
 */
#define TST_CGROUP_MAX_TREES (TST_CGROUP_MAX + 1)

/* CGroup tree */
struct tst_cgroup_root {
	enum tst_cgroup_ver ver;
	/* Mount path */
	char path[PATH_MAX/2];
	/* Subsystems (controllers) bit field */
	uint32_t ss_field;

	/* Path FDs for use with openat syscalls */
	/* Mount directory */
	int mnt_dir;
	/* LTP CGroup directory, contains drain and test dirs */
	int ltp_dir;
	/* Drain CGroup, see tst_cgroup_cleanup */
	int drain_dir;
	/* CGroup for current test */
	int tst_dir;

	int we_mounted_it:1;
	int we_created_ltp_dir:1;
	/* cpuset is in compatability mode */
	int no_prefix:1;
};

/* 'ss' stands for subsystem which is the same as 'controller' */
struct tst_cgroup_ss {
	enum tst_cgroup_ctrl indx;
	const char *name;
	struct tst_cgroup_root *tree;
};

struct tst_cgroup_features {
	const char *memory_max_name;
	const char *memory_max_swap_name;
	int memory_has_kmem:1;
	int memory_has_swap:1;
};

/* Always use first item for unified hierarchy */
static struct tst_cgroup_root trees[TST_CGROUP_MAX_TREES + 1];

static struct tst_cgroup_ss css[TST_CGROUP_MAX + 1] = {
	{ TST_CGROUP_MEMORY, "memory", NULL },
	{ TST_CGROUP_CPUSET, "cpuset", NULL },
};

static const struct tst_cgroup_opts default_opts = { 0 };
static struct tst_cgroup_features features;

static const char *ltp_cgroup_dir = "ltp";
static const char *ltp_cgroup_drain_dir = "drain";
static char test_cgroup_dir[PATH_MAX/4];
static const char *ltp_mount_prefix = "/tmp/cgroup_";
static const char *ltp_v2_mount = "unified";

#define tst_for_each_v1_root(t) \
	for ((t) = trees + 1; (t)->ver; (t)++)
#define tst_for_each_root(t) \
	for ((t) = trees[0].ver ? trees : trees + 1; (t)->ver; (t)++)
#define tst_for_each_css(ss) for ((ss) = css; (ss)->indx; (ss)++)

static int tst_cgroup_v2_mounted(void)
{
	return !!trees[0].path[0];
}

static struct tst_cgroup_ss *tst_cgroup_get_ss(enum tst_cgroup_ctrl type)
{
	return &css[type - 1];
}

static int tst_cgroup_ss_on_v2(const struct tst_cgroup_ss *ss)
{
	return ss->tree->ver == TST_CGROUP_V2;
}

void tst_cgroup_print_config(void)
{
	struct tst_cgroup_root *t;
	struct tst_cgroup_ss *ss;

	tst_res(TINFO, "Detected Controllers:");

	tst_for_each_css(ss) {
		t = ss->tree;

		if (!t)
			continue;

		tst_res(TINFO, "\t%.10s %s @ %s:%s",
			ss->name,
			t->no_prefix ? "[noprefix]" : "",
			t->ver == TST_CGROUP_V1 ? "V1" : "V2",
			t->path);
	}

	if (!(features.memory_has_kmem || features.memory_has_swap))
		return;

	tst_res(TINFO, "Detected Feature flags: %s%s",
		features.memory_has_kmem ? "kmem " : "",
		features.memory_has_swap ? "swap" : "");
}

ssize_t safe_cgroup_read(const char *file, const int lineno,
		      enum tst_cgroup_ctrl type,
		      const char *path, char *buf, size_t nbyte)
{
	return safe_file_readat(file, lineno,
				tst_cgroup_get_ss(type)->tree->tst_dir,
				path, buf, nbyte);
}

void safe_cgroup_printf(const char *file, const int lineno,
			enum tst_cgroup_ctrl type,
			const char *path, const char *fmt, ...)
{
	struct tst_cgroup_ss *ss = tst_cgroup_get_ss(type);
	va_list va;

	va_start(va, fmt);
	safe_file_vprintfat(file, lineno, ss->tree->tst_dir, path, fmt, va);
	va_end(va);
}

void tst_cgroup_move_current(enum tst_cgroup_ctrl type)
{
	if (tst_cgroup_ss_on_v2(tst_cgroup_get_ss(type)))
		SAFE_CGROUP_PRINTF(type, "cgroup.procs", "%d", getpid());
	else
		SAFE_CGROUP_PRINTF(type, "tasks", "%d", getpid());
}

void tst_cgroup_mem_set_maxbytes(size_t memsz)
{
	SAFE_CGROUP_PRINTF(TST_CGROUP_MEMORY,
			   features.memory_max_name, "%zu", memsz);
}

int tst_cgroup_mem_swapacct_enabled(void)
{
	if (!features.memory_has_swap)
		tst_res(TCONF, "memcg swap accounting is disabled");

	return features.memory_has_swap;
}

void tst_cgroup_mem_set_maxswap(size_t memsz)
{
	if (!features.memory_has_swap)
		return;

	SAFE_CGROUP_PRINTF(TST_CGROUP_MEMORY,
			   features.memory_max_swap_name, "%zu", memsz);
}

static void tst_cgroup_cpuset_path(const char *name, char *buf)
{
	struct tst_cgroup_ss *ss = tst_cgroup_get_ss(TST_CGROUP_CPUSET);
	char *path = buf;

	if (!ss->tree->no_prefix) {
		strcpy(buf, "cpuset.");
		path += strlen(buf);
	}

	strcpy(path, name);
}

void tst_cgroup_cpuset_read_files(const char *name, char *buf, size_t nbyte)
{
	char path[PATH_MAX/4];

	tst_cgroup_cpuset_path(name, path);
	SAFE_CGROUP_READ(TST_CGROUP_CPUSET, path, buf, nbyte);
}

void tst_cgroup_cpuset_write_files(const char *name, const char *buf)
{

	char path[PATH_MAX/4];

	tst_cgroup_cpuset_path(name, path);
	SAFE_CGROUP_PRINTF(TST_CGROUP_CPUSET, path, "%s", buf);
}

/* Determine if a mounted cgroup hierarchy (tree) is unique and record it if so.
 *
 * For CGroups V2 this is very simple as there is only one
 * hierarchy. We just record which controllers are available and check
 * if this matches what we read from any previous mounts to verify our
 * own logic (and possibly detect races).
 *
 * For V1 the set of controllers S is partitioned into sets {P_1, P_2,
 * ..., P_n} with one or more controllers in each partion. Each
 * partition P_n can be mounted multiple times, but the same
 * controller can not appear in more than one partition. Usually each
 * partition contains a single controller, but, for example, cpu and
 * cpuacct are often mounted together in the same partiion.
 *
 * Each controller partition has its own hierarchy/root/tree which we
 * must track and update independently.
 */
static void tst_cgroup_get_tree(const char *type, const char *path, char *opts)
{
	struct tst_cgroup_root *t = trees;
	struct tst_cgroup_ss *c;
	uint32_t ss_field = 0;
	int no_prefix = 0;
	char buf[BUFSIZ];
	char *tok;
	int dfd = SAFE_OPEN(path, O_PATH | O_DIRECTORY);

	if (!strcmp(type, "cgroup"))
		goto v1;

	SAFE_FILE_READAT(dfd, "cgroup.controllers", buf, sizeof(buf));

	for (tok = strtok(buf, " "); tok; tok = strtok(NULL, " ")) {
		tst_for_each_css(c)
			ss_field |= (!strcmp(c->name, tok)) << c->indx;
	}

	if (t->ver && ss_field == t->ss_field)
		goto discard;

	if (t->ss_field)
		tst_brk(TBROK, "Available V2 controllers are changing between scans?");

	t->ver = TST_CGROUP_V2;

	goto backref;

v1:
	for (tok = strtok(opts, ","); tok; tok = strtok(NULL, ",")) {
		tst_for_each_css(c)
			ss_field |= (!strcmp(c->name, tok)) << c->indx;

		no_prefix |= !strcmp("noprefix", tok);
	}

	if (!ss_field)
		goto discard;

	tst_for_each_v1_root(t) {
		if (!(ss_field & t->ss_field))
			continue;

		if (ss_field == t->ss_field)
			goto discard;

		tst_brk(TBROK,
			"The intersection of two distinct sets of mounted controllers should be null?"
			"Check '%s' and '%s'", t->path, path);
	}

	if (t >= trees + TST_CGROUP_MAX_TREES) {
		tst_brk(TBROK, "Unique controller mounts have exceeded our limit %d?",
			TST_CGROUP_MAX_TREES);
	}

	t->ver = TST_CGROUP_V1;

backref:
	strcpy(t->path, path);
	t->mnt_dir = dfd;
	t->ss_field = ss_field;
	t->no_prefix = no_prefix;

	tst_for_each_css(c) {
		if (t->ss_field & (1 << c->indx))
			c->tree = t;
	}

	return;

discard:
	close(dfd);
}

void tst_cgroup_scan(void)
{
	struct mntent *mnt;
	FILE *f = setmntent("/proc/self/mounts", "r");

	if (!f)
		tst_brk(TBROK | TERRNO, "Can't open /proc/self/mounts");

	mnt = getmntent(f);
	if (!mnt)
		tst_brk(TBROK | TERRNO, "Can't read mounts or no mounts?");

	do {
		if (strncmp(mnt->mnt_type, "cgroup", 6))
			continue;

		tst_cgroup_get_tree(mnt->mnt_type, mnt->mnt_dir, mnt->mnt_opts);
	} while ((mnt = getmntent(f)));
}

static void tst_cgroup_mount_v2(void)
{
	char path[PATH_MAX];
	int made_dir = 0;

	sprintf(path, "%s%s", ltp_mount_prefix, ltp_v2_mount);

	if (!mkdir(path, 0777)) {
		made_dir = 1;
		goto mount;
	}

	if (errno == EEXIST)
		goto mount;

	if (errno == EACCES) {
		tst_res(TINFO | TERRNO,
			"Lack permission to make %s, premake it or run as root",
			path);
		return;
	}

	tst_brk(TBROK | TERRNO, "mkdir(%s, 0777)", path);

mount:
	if (!mount("cgroup2", path, "cgroup2", 0, NULL)) {
		tst_res(TINFO, "Mounted V2 CGroups on %s", path);
		tst_cgroup_scan();
		trees[0].we_mounted_it = 1;
		return;
	}

	tst_res(TINFO | TERRNO, "Could not mount V2 CGroups on %s", path);

	if (made_dir)
		SAFE_RMDIR(path);
}

static void tst_cgroup_mount_v1(enum tst_cgroup_ctrl type)
{
	struct tst_cgroup_ss *ss = tst_cgroup_get_ss(type);
	char path[PATH_MAX];
	int made_dir = 0;

	sprintf(path, "%s%s", ltp_mount_prefix, ss->name);

	if (!mkdir(path, 0777)) {
		made_dir = 1;
		goto mount;
	}

	if (errno == EEXIST)
		goto mount;

	if (errno == EACCES) {
		tst_res(TINFO | TERRNO,
			"Lack permission to make %s, premake it or run as root",
			path);
		return;
	}

	tst_brk(TBROK | TERRNO, "mkdir(%s, 0777)", path);

mount:
	if (mount(ss->name, path, "cgroup", 0, ss->name)) {
		tst_res(TINFO | TERRNO,
			"Could not mount V1 CGroup on %s", path);

		if (made_dir)
			SAFE_RMDIR(path);
		return;
	}

	tst_res(TINFO, "Mounted V1 %s CGroup on %s", ss->name, path);
	tst_cgroup_scan();
	ss->tree->we_mounted_it = 1;
	if (type == TST_CGROUP_MEMORY) {
		SAFE_FILE_PRINTFAT(ss->tree->mnt_dir,
				   "memory.use_hierarchy", "%d", 1);
	}
}

static int tst_cgroup_exists(struct tst_cgroup_ss *ss, const char *path)
{
	int ret = faccessat(ss->tree->tst_dir, path, F_OK, 0);

	if (!ret)
		return 1;

	if (errno == ENOENT)
		return 0;

	tst_brk(TBROK | TERRNO, "faccessat(%d, %s, F_OK, 0)",
		ss->tree->tst_dir, path);

	return 0;
}

static void tst_cgroup_scan_features(enum tst_cgroup_ctrl type)
{
	struct tst_cgroup_ss *ss = tst_cgroup_get_ss(type);

	if (type == TST_CGROUP_MEMORY) {
		if (ss->tree->ver == TST_CGROUP_V1) {
			features.memory_max_name = "memory.limit_in_bytes";
			features.memory_max_swap_name =
				"memory.memsw.limit_in_bytes";
		} else {
			features.memory_max_name = "memory.max";
			features.memory_max_swap_name = "memory.swap.max";
		}

		features.memory_has_kmem =
			tst_cgroup_exists(ss, "memory.kmem.limit_in_bytes");
		features.memory_has_swap =
			tst_cgroup_exists(ss, features.memory_max_swap_name);
	}
}

static void tst_cgroup_copy_cpuset(struct tst_cgroup_root *t)
{
	char buf[BUFSIZ];
	const char *mems = t->no_prefix ? "mems" : "cpuset.mems";
	const char *cpus = t->no_prefix ? "cpus" : "cpuset.cpus";


	SAFE_FILE_READAT(t->mnt_dir, mems, buf, sizeof(buf));
	SAFE_FILE_PRINTFAT(t->ltp_dir, mems, "%s", buf);

	SAFE_FILE_READAT(t->mnt_dir, cpus, buf, sizeof(buf));
	SAFE_FILE_PRINTFAT(t->ltp_dir, cpus, "%s", buf);
}

/* Ensure the specified controller is available and contains the LTP group
 *
 * First we check if the specified controller has a known mount point,
 * if not then we scan the system. If we find it then we goto ensuring
 * the LTP group exists in the hierarchy the controller is using.
 *
 * If we can't find the controller, then we try to create it. First we
 * check if the V2 hierarchy/tree is mounted. If it isn't then we try
 * mounting it and look for the controller. If it is already mounted
 * then we know the controller is not available on V2 on this system.
 *
 * If we can't mount V2 or the controller is not on V2, then we try
 * mounting it on its own V1 tree.
 *
 * Once we have mounted the controller somehow, we create hierarchy of
 * cgroups. If we are on V2 we first need to enable the controller for
 * all subtrees in root. Then we create the following hierarchy.
 *
 * ltp -> test_<pid>
 *
 * Having a cgroup per test allows tests to be run in parallel. The
 * LTP group could allow for tests to run with less privileges if it
 * is pre-setup for us.
 *
 * If we are using V1 cpuset then we copy the available mems and cpus
 * from root to the ltp group and set clone_children on the ltp group
 * to distribute these settings to the test cgroups. This means the
 * test author does not have to copy these settings before using the
 * cpuset.
 *
 */
void tst_cgroup_require(enum tst_cgroup_ctrl type,
			const struct tst_cgroup_opts *options)
{
	struct tst_cgroup_ss *ss = tst_cgroup_get_ss(type);
	struct tst_cgroup_root *t;
	char path[PATH_MAX];

	if (!options)
		options = &default_opts;

	if (ss->tree)
		goto ltpdir;

	tst_cgroup_scan();

	if (ss->tree)
		goto ltpdir;

	if (!tst_cgroup_v2_mounted() && !options->only_mount_v1)
		tst_cgroup_mount_v2();

	if (ss->tree)
		goto ltpdir;

	tst_cgroup_mount_v1(type);

	if (!ss->tree) {
		tst_brk(TCONF,
			"%s controller required, but not available", ss->name);
	}

ltpdir:
	t = ss->tree;

	if (tst_cgroup_ss_on_v2(ss)) {
		tst_file_printfat(t->mnt_dir, "cgroup.subtree_control",
				  "+%s", ss->name);
	}

	sprintf(path, "%s/%s", t->path, ltp_cgroup_dir);

	if (!mkdir(path, 0777)) {
		t->we_created_ltp_dir = 1;
		goto draindir;
	}

	if (errno == EEXIST)
		goto draindir;

	if (errno == EACCES) {
		tst_brk(TCONF | TERRNO,
			"Lack permission to make %s; premake it or run as root",
			path);
	}

	tst_brk(TBROK | TERRNO, "mkdir(%s, 0777)", path);

draindir:
	if (!t->ltp_dir)
		t->ltp_dir = SAFE_OPEN(path, O_PATH | O_DIRECTORY);

	if (tst_cgroup_ss_on_v2(ss)) {
		SAFE_FILE_PRINTFAT(t->ltp_dir, "cgroup.subtree_control",
				   "+%s", ss->name);
	} else {
		SAFE_FILE_PRINTFAT(t->ltp_dir, "cgroup.clone_children",
				   "%d", 1);

		if (type == TST_CGROUP_CPUSET)
			tst_cgroup_copy_cpuset(t);
	}

	sprintf(path, "%s/%s/%s",
		t->path, ltp_cgroup_dir, ltp_cgroup_drain_dir);

	if (!mkdir(path, 0777) || errno == EEXIST)
		goto testdir;

	if (errno == EACCES) {
		tst_brk(TCONF | TERRNO,
			"Lack permission to make %s; grant access or run as root",
			path);
	}

	tst_brk(TBROK | TERRNO, "mkdir(%s, 0777)", path);

testdir:
	if (!t->drain_dir)
		t->drain_dir = SAFE_OPEN(path, O_PATH | O_DIRECTORY);

	if (!test_cgroup_dir[0])
		sprintf(test_cgroup_dir, "test-%d", getpid());

	sprintf(path, "%s/%s/%s",
		ss->tree->path, ltp_cgroup_dir, test_cgroup_dir);

	if (!mkdir(path, 0777) || errno == EEXIST)
		goto scan;

	if (errno == EACCES) {
		tst_brk(TCONF | TERRNO,
			"Lack permission to make %s; grant access or run as root",
			path);
	}

	tst_brk(TBROK | TERRNO, "mkdir(%s, 0777)", path);

scan:
	if (!t->tst_dir)
		t->tst_dir = SAFE_OPEN(path, O_PATH | O_DIRECTORY);

	tst_cgroup_scan_features(type);
}

static void tst_cgroup_drain(enum tst_cgroup_ver ver, int source, int dest)
{
	char buf[BUFSIZ];
	char *tok;
	const char *fname = ver == TST_CGROUP_V1 ? "tasks" : "cgroup.procs";
	int fd, ret;

	SAFE_FILE_READAT(source, fname, buf, sizeof(buf));

	fd = SAFE_OPENAT(dest, fname, O_WRONLY);
	for (tok = strtok(buf, "\n"); tok; tok = strtok(NULL, "\n")) {
		ret = dprintf(fd, "%s", tok);

		if (ret < (int)strlen(tok))
			tst_brk(TBROK | TERRNO, "Failed to drain %s", tok);
	}
	SAFE_CLOSE(fd);
}

static void close_path_fds(struct tst_cgroup_root *t)
{
	if (t->tst_dir > 0)
		SAFE_CLOSE(t->tst_dir);
	if (t->ltp_dir > 0)
		SAFE_CLOSE(t->ltp_dir);
	if (t->drain_dir > 0)
		SAFE_CLOSE(t->drain_dir);
	if (t->mnt_dir > 0)
		SAFE_CLOSE(t->mnt_dir);
}

/* Maybe remove CGroups used during testing and clear our data
 *
 * This will never remove CGroups we did not create to allow tests to
 * be run in parallel (see enum tst_cgroup_cleanup).
 *
 * Each test process is given its own unique CGroup. Unless we want to
 * stress test the CGroup system. We should at least remove these
 * unique test CGroups.
 *
 * We probably also want to remove the LTP parent CGroup, although
 * this may have been created by the system manager or another test
 * (see notes on parallel testing).
 *
 * On systems with no initial CGroup setup we may try to destroy the
 * CGroup roots we mounted so that they can be recreated by another
 * test. Note that successfully unmounting a CGroup root does not
 * necessarily indicate that it was destroyed.
 *
 * The ltp/drain CGroup is required for cleaning up test CGroups when
 * we can not move them to the root CGroup. CGroups can only be
 * removed when they have no members and only leaf or root CGroups may
 * have processes within them. As test processes create and destroy
 * their own CGroups they must move themselves either to root or
 * another leaf CGroup. So we move them to drain while destroying the
 * unique test CGroup.
 *
 * If we have access to root and created the LTP CGroup we then move
 * the test process to root and destroy the drain and LTP
 * CGroups. Otherwise we just leave the test process to die in the
 * drain, much like many a unwanted terrapin.
 *
 * Finally we clear any data we have collected on CGroups. This will
 * happen regardless of whether anything was removed.
 */
void tst_cgroup_cleanup(const struct tst_cgroup_opts *opts)
{
	struct tst_cgroup_root *t;
	struct tst_cgroup_ss *ss;

	if (!opts)
		opts = &default_opts;

	if (opts->cleanup == TST_CGROUP_CLEANUP_NONE)
		goto clear_data;

	tst_for_each_root(t) {
		if (!t->tst_dir)
			continue;

		tst_cgroup_drain(t->ver, t->tst_dir, t->drain_dir);
		SAFE_UNLINKAT(t->ltp_dir, test_cgroup_dir, AT_REMOVEDIR);
	}

	if (opts->cleanup == TST_CGROUP_CLEANUP_TEST)
		goto clear_data;

	tst_for_each_root(t) {
		if (!t->we_created_ltp_dir)
			continue;

		tst_cgroup_drain(t->ver, t->drain_dir, t->mnt_dir);
		SAFE_UNLINKAT(t->ltp_dir, ltp_cgroup_drain_dir, AT_REMOVEDIR);
		SAFE_UNLINKAT(t->mnt_dir, ltp_cgroup_dir, AT_REMOVEDIR);
	}

	if (opts->cleanup == TST_CGROUP_CLEANUP_LTP)
		goto clear_data;

	tst_for_each_css(ss) {
		if (!tst_cgroup_ss_on_v2(ss) || !ss->tree->we_mounted_it)
			continue;

		SAFE_FILE_PRINTFAT(ss->tree->mnt_dir, "cgroup.subtree_control",
				   "-%s", ss->name);
	}

	tst_for_each_root(t)
		close_path_fds(t);

	tst_for_each_root(t) {
		if (!t->we_mounted_it)
			continue;

		/* This probably does not result in the CGroup root
		 * being destroyed */
		if (umount2(t->path, MNT_DETACH))
			continue;

		SAFE_RMDIR(t->path);
	}

clear_data:
	tst_for_each_css(ss)
		memset(ss, 0, sizeof(*ss));

	tst_for_each_root(t) {
		close_path_fds(t);
		memset(t, 0, sizeof(*t));
	}

	memset(&features, 0, sizeof(features));
}
