// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Red Hat, Inc.
 * Copyright (c) 2020 Li Wang <liwang@redhat.com>
 * Copyright (c) 2020-2021 SUSE LLC <rpalethorpe@suse.com>
 */

#define TST_NO_DEFAULT_MAIN

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <mntent.h>
#include <sys/mount.h>

#include "tst_test.h"
#include "lapi/fcntl.h"
#include "lapi/mount.h"
#include "lapi/mkdirat.h"
#include "tst_cgroup.h"

/* CGroup Core Implementation
 *
 * CGroup Item Implementation is towards the bottom.
 */

struct cgroup_root;

/* A node in a single CGroup hierarchy. It exists mainly for
 * convenience so that we do not have to traverse the CGroup structure
 * for frequent operations.
 *
 * This is actually a single-linked list not a tree. We only need to
 * traverse from leaf towards root.
 */
struct tst_cgroup_tree {
	/* The tree's name and parent */
	struct tst_cgroup_location loc;
	/* Shortcut to root */
	const struct cgroup_root *root;

	/* Subsystems (controllers) bit field. Only controllers which
	 * were required and configured for this node are added to
	 * this field. So it may be different from root->ss_field.
	 */
	uint32_t ss_field;

	/* In general we avoid having sprintfs everywhere and use
	 * openat, linkat, etc.
	 */
	int dir;

	int we_created_it:1;
};

/* The root of a CGroup hierarchy/tree */
struct cgroup_root {
	enum tst_cgroup_ver ver;
	/* A mount path */
	char path[PATH_MAX/2];
	/* Subsystems (controllers) bit field. Includes all
	 * controllers found while scanningthis root.
	 */
	uint32_t ss_field;

	/* CGroup hierarchy: mnt -> ltp -> {drain, test -> ??? } We
	 * keep a flat reference to ltp, drain and test for
	 * convenience.
	 */

	/* Mount directory */
	struct tst_cgroup_tree mnt;
	/* LTP CGroup directory, contains drain and test dirs */
	struct tst_cgroup_tree ltp;
	/* Drain CGroup, see cgroup_cleanup */
	struct tst_cgroup_tree drain;
	/* CGroup for current test. Which may have children. */
	struct tst_cgroup_tree test;

	int we_mounted_it:1;
	/* cpuset is in compatability mode */
	int no_prefix:1;
};

/* 'ss' stands for subsystem which is the same as 'controller' */
struct cgroup_ss {
	enum tst_cgroup_ctrl indx;
	const char *name;
	struct cgroup_root *tree;

	int we_require_it:1;
};

/* Always use first item for unified hierarchy */
struct cgroup_root roots[TST_CGROUP_MAX_TREES + 1];

static struct cgroup_ss css[TST_CGROUP_MAX + 1] = {
	{ TST_CGROUP_MEMORY, "memory", NULL, 0 },
	{ TST_CGROUP_CPUSET, "cpuset", NULL, 0 },
};

static const struct tst_cgroup_opts default_opts = { 0 };

/* We should probably allow these to be set in environment
 * variables */
static const char *ltp_cgroup_dir = "ltp";
static const char *ltp_cgroup_drain_dir = "drain";
static char test_cgroup_dir[PATH_MAX/4];
static const char *ltp_mount_prefix = "/tmp/cgroup_";
static const char *ltp_v2_mount = "unified";

#define first_root				\
	(roots[0].ver ? roots : roots + 1)
#define for_each_root(r)			\
	for ((r) = first_root; (r)->ver; (r)++)
#define for_each_v1_root(r)			\
	for ((r) = roots + 1; (r)->ver; (r)++)
#define for_each_css(ss)			\
	for ((ss) = css; (ss)->indx; (ss)++)

static int has_css(uint32_t ss_field, enum tst_cgroup_ctrl type)
{
	return !!(ss_field & (1 << type));
}

static void add_css(uint32_t *ss_field, int cond, enum tst_cgroup_ctrl type)
{
	*ss_field |= (!!cond) << type;
}

struct cgroup_root *tst_cgroup_root_get(void)
{
	return roots[0].ver ? roots : roots + 1;
}

static int cgroup_v2_mounted(void)
{
	return !!roots[0].ver;
}

static int cgroup_v1_mounted(void)
{
	return !!roots[1].ver;
}

static int cgroup_mounted(void)
{
	return cgroup_v2_mounted() || cgroup_v1_mounted();
}

static struct cgroup_ss *cgroup_get_ss(enum tst_cgroup_ctrl type)
{
	return &css[type - 1];
}

static int cgroup_ss_on_v2(const struct cgroup_ss *ss)
{
	return ss->tree->ver == TST_CGROUP_V2;
}

int tst_cgroup_tree_mk(const struct tst_cgroup_tree *parent,
		       const char *name,
		       struct tst_cgroup_tree *new)
{
	char *dpath;

	new->root = parent->root;
	new->loc.name = name;
	new->loc.tree = parent;
	new->ss_field = parent->ss_field;
	new->we_created_it = 0;

	if (!mkdirat(parent->dir, name, 0777)) {
		new->we_created_it = 1;
		goto opendir;
	}

	if (errno == EEXIST)
		goto opendir;

	dpath = tst_decode_fd(parent->dir);

	if (errno == EACCES) {
		tst_brk(TCONF | TERRNO,
			"Lack permission to make '%s/%s'; premake it or run as root",
			dpath, name);
	} else {
		tst_brk(TBROK | TERRNO,
			"mkdirat(%d<%s>, '%s', 0777)", parent->dir, dpath, name);
	}

	return -1;
opendir:
	new->dir = SAFE_OPENAT(parent->dir, name, O_PATH | O_DIRECTORY);

	return 0;
}

void tst_cgroup_print_config(void)
{
	struct cgroup_root *t;
	struct cgroup_ss *ss;

	tst_res(TINFO, "Detected Controllers:");

	for_each_css(ss) {
		t = ss->tree;

		if (!t)
			continue;

		tst_res(TINFO, "\t%.10s %s @ %s:%s",
			ss->name,
			t->no_prefix ? "[noprefix]" : "",
			t->ver == TST_CGROUP_V1 ? "V1" : "V2",
			t->path);
	}
}

/* Determine if a mounted cgroup hierarchy (tree) is unique and record it if so.
 *
 * For CGroups V2 this is very simple as there is only one
 * hierarchy. We just record which controllers are available and check
 * if this matches what we read from any previous mount points.
 *
 * For V1 the set of controllers S is partitioned into sets {P_1, P_2,
 * ..., P_n} with one or more controllers in each partion. Each
 * partition P_n can be mounted multiple times, but the same
 * controller can not appear in more than one partition. Usually each
 * partition contains a single controller, but, for example, cpu and
 * cpuacct are often mounted together in the same partiion.
 *
 * Each controller partition has its own hierarchy which we must track
 * and update independently.
 */
static void cgroup_root_scan(const char *type, const char *path, char *opts)
{
	struct cgroup_root *t = roots;
	struct cgroup_ss *c;
	uint32_t ss_field = 0;
	int no_prefix = 0;
	char buf[BUFSIZ];
	char *tok;
	int dfd = SAFE_OPEN(path, O_PATH | O_DIRECTORY);

	if (!strcmp(type, "cgroup"))
		goto v1;

	SAFE_FILE_READAT(dfd, "cgroup.controllers", buf, sizeof(buf));

	for (tok = strtok(buf, " "); tok; tok = strtok(NULL, " ")) {
		for_each_css(c)
			add_css(&ss_field, !strcmp(c->name, tok), c->indx);
	}

	if (t->ver && ss_field == t->ss_field)
		goto discard;

	if (t->ss_field)
		tst_brk(TBROK, "Available V2 controllers are changing between scans?");

	t->ver = TST_CGROUP_V2;

	goto backref;

v1:
	for (tok = strtok(opts, ","); tok; tok = strtok(NULL, ",")) {
		for_each_css(c)
			add_css(&ss_field, !strcmp(c->name, tok), c->indx);

		no_prefix |= !strcmp("noprefix", tok);
	}

	if (!ss_field)
		goto discard;

	for_each_v1_root(t) {
		if (!(ss_field & t->ss_field))
			continue;

		if (ss_field == t->ss_field)
			goto discard;

		tst_brk(TBROK,
			"The intersection of two distinct sets of mounted controllers should be null?"
			"Check '%s' and '%s'", t->path, path);
	}

	if (t >= roots + TST_CGROUP_MAX_TREES) {
		tst_brk(TBROK, "Unique controller mounts have exceeded our limit %d?",
			TST_CGROUP_MAX_TREES);
	}

	t->ver = TST_CGROUP_V1;

backref:
	strcpy(t->path, path);
	t->mnt.root = t;
	t->mnt.loc.name = t->path;
	t->mnt.dir = dfd;
	t->ss_field = ss_field;
	t->no_prefix = no_prefix;

	for_each_css(c) {
		if (has_css(t->ss_field, c->indx))
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

		cgroup_root_scan(mnt->mnt_type, mnt->mnt_dir, mnt->mnt_opts);
	} while ((mnt = getmntent(f)));
}

static void cgroup_mount_v2(void)
{
	char path[PATH_MAX];

	sprintf(path, "%s%s", ltp_mount_prefix, ltp_v2_mount);

	if (!mkdir(path, 0777)) {
		roots[0].mnt.we_created_it = 1;
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
		roots[0].we_mounted_it = 1;
		return;
	}

	tst_res(TINFO | TERRNO, "Could not mount V2 CGroups on %s", path);

	if (roots[0].mnt.we_created_it) {
		roots[0].mnt.we_created_it = 0;
		SAFE_RMDIR(path);
	}
}

static void cgroup_mount_v1(enum tst_cgroup_ctrl type)
{
	struct cgroup_ss *ss = cgroup_get_ss(type);
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
	ss->tree->mnt.we_created_it = made_dir;

	if (type == TST_CGROUP_MEMORY) {
		SAFE_FILE_PRINTFAT(ss->tree->mnt.dir,
				   "memory.use_hierarchy", "%d", 1);
	}
}

static void cgroup_copy_cpuset(const struct cgroup_root *t)
{
	char buf[BUFSIZ];
	int i;
	const char *n0[] = {"mems", "cpus"};
	const char *n1[] = {"cpuset.mems", "cpuset.cpus"};
	const char **fname = t->no_prefix ? n0 : n1;

	for (i = 0; i < 2; i++) {
		SAFE_FILE_READAT(t->mnt.dir, fname[i], buf, sizeof(buf));
		SAFE_FILE_PRINTFAT(t->ltp.dir, fname[i], "%s", buf);
	}
}

/* Ensure the specified controller is available.
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
 * Once we have mounted the controller somehow, we create a hierarchy
 * of cgroups. If we are on V2 we first need to enable the controller
 * for all children of root. Then we create hierarchy described in
 * tst_cgroup.h.
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
	const char *const cgsc = "cgroup.subtree_control";
	struct cgroup_ss *ss = cgroup_get_ss(type);
	struct cgroup_root *t;
	char buf[BUFSIZ];

	if (!options)
		options = &default_opts;

	if (ss->we_require_it)
		tst_res(TWARN, "Duplicate tst_cgroup_require(%s, )", ss->name);
	ss->we_require_it = 1;

	if (ss->tree)
		goto ltpdir;

	tst_cgroup_scan();

	if (ss->tree)
		goto ltpdir;

	if (!cgroup_v2_mounted() && !options->only_mount_v1)
		cgroup_mount_v2();

	if (ss->tree)
		goto ltpdir;

	cgroup_mount_v1(type);

	if (!ss->tree) {
		tst_brk(TCONF,
			"'%s' controller required, but not available", ss->name);
	}

ltpdir:
	t = ss->tree;
	add_css(&t->mnt.ss_field, 1, type);

	if (cgroup_ss_on_v2(ss) && t->we_mounted_it) {
		SAFE_FILE_PRINTFAT(t->mnt.dir, cgsc, "+%s", ss->name);
	} else if (cgroup_ss_on_v2(ss)) {
		SAFE_FILE_READAT(t->mnt.dir, cgsc, buf, sizeof(buf));
		if (!strstr(buf, ss->name)) {
			tst_brk(TCONF,
				"'%s' controller required, but not in %s/%s",
				ss->name, t->path, cgsc);
		}
	}

	if (!t->ltp.dir)
		tst_cgroup_tree_mk(&t->mnt, ltp_cgroup_dir, &t->ltp);
	else
		t->ltp.ss_field |= t->mnt.ss_field;

	if (!t->ltp.we_created_it)
		goto testdir;

	if (cgroup_ss_on_v2(ss)) {
		SAFE_FILE_PRINTFAT(t->ltp.dir, cgsc, "+%s", ss->name);
	} else {
		SAFE_FILE_PRINTFAT(t->ltp.dir, "cgroup.clone_children",
				   "%d", 1);

		if (type == TST_CGROUP_CPUSET)
			cgroup_copy_cpuset(t);
	}

testdir:
	tst_cgroup_tree_mk(&t->ltp, ltp_cgroup_drain_dir, &t->drain);

	sprintf(test_cgroup_dir, "test-%d", getpid());
	tst_cgroup_tree_mk(&t->ltp, test_cgroup_dir, &t->test);
}

static void cgroup_drain(enum tst_cgroup_ver ver, int source, int dest)
{
	char buf[BUFSIZ];
	char *tok;
	const char *fname = ver == TST_CGROUP_V1 ? "tasks" : "cgroup.procs";
	int fd;
	ssize_t ret;

	ret = SAFE_FILE_READAT(source, fname, buf, sizeof(buf));
	if (ret < 0)
		return;

	fd = SAFE_OPENAT(dest, fname, O_WRONLY);
	if (fd < 0)
		return;

	for (tok = strtok(buf, "\n"); tok; tok = strtok(NULL, "\n")) {
		ret = dprintf(fd, "%s", tok);

		if (ret < (ssize_t)strlen(tok))
			tst_brk(TBROK | TERRNO, "Failed to drain %s", tok);
	}
	SAFE_CLOSE(fd);
}

static void close_path_fds(struct cgroup_root *t)
{
	if (t->test.dir > 0)
		SAFE_CLOSE(t->test.dir);
	if (t->ltp.dir > 0)
		SAFE_CLOSE(t->ltp.dir);
	if (t->drain.dir > 0)
		SAFE_CLOSE(t->drain.dir);
	if (t->mnt.dir > 0)
		SAFE_CLOSE(t->mnt.dir);
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
void tst_cgroup_cleanup(void)
{
	struct cgroup_root *t;
	struct cgroup_ss *ss;

	if (!cgroup_mounted())
		goto clear_data;

	for_each_root(t) {
		if (!t->test.loc.name)
			continue;

		cgroup_drain(t->ver, t->test.dir, t->drain.dir);
		SAFE_UNLINKAT(t->ltp.dir, t->test.loc.name, AT_REMOVEDIR);
	}

	for_each_root(t) {
		if (!t->ltp.we_created_it)
			continue;

		cgroup_drain(t->ver, t->drain.dir, t->mnt.dir);

		if (t->drain.loc.name)
			SAFE_UNLINKAT(t->ltp.dir, t->drain.loc.name, AT_REMOVEDIR);

		if (t->ltp.loc.name)
			SAFE_UNLINKAT(t->mnt.dir, t->ltp.loc.name, AT_REMOVEDIR);
	}

	for_each_css(ss) {
		if (!cgroup_ss_on_v2(ss) || !ss->tree->we_mounted_it)
			continue;

		SAFE_FILE_PRINTFAT(ss->tree->mnt.dir, "cgroup.subtree_control",
				   "-%s", ss->name);
	}

	for_each_root(t) {
		if (!t->we_mounted_it)
			continue;

		/* This probably does not result in the CGroup root
		 * being destroyed */
		if (umount2(t->path, MNT_DETACH))
			continue;

		SAFE_RMDIR(t->path);
	}

clear_data:
	memset(css, 0, sizeof(css));

	for_each_root(t)
		close_path_fds(t);

	memset(roots, 0, sizeof(roots));
}

/* CGroup Item Implementation */

static int cgroup_default_exists(const char * file, const int lineno,
				 tst_cgroup_item_ptr item LTP_ATTRIBUTE_UNUSED)
{
	tst_brk_(file, lineno, TBROK, "Exists method not implemented for item");

	return 0;
}

static const struct tst_cgroup_item cgroup_default_item = {
	.exists = cgroup_default_exists,
};

static int cgroup_ent_exists(const char * file, const int lineno,
			     const struct tst_cgroup_location *loc)
{
	int dirfd, ret;

	if (!loc->tree)
		return 0;

	dirfd = loc->tree->dir;
	ret = faccessat(dirfd, loc->name, F_OK, 0);

	if (!ret)
		return 1;

	if (errno == ENOENT)
		return 0;

	tst_brk_(file, lineno, TBROK | TERRNO,
		 "faccessat(%d<%s>, %s, F_OK, 0)",
		 dirfd, tst_decode_fd(dirfd), loc->name);

	return 0;
}

static int cgroup_file_exists(const char * file, const int lineno,
			      tst_cgroup_item_ptr item)
{
	const struct tst_cgroup_file *cgf =
		tst_container_of(item, typeof(*cgf), item);
	unsigned int i, n = cgf->n;
	const struct tst_cgroup_location *loc = cgf->locations;

	for (i = 0; i < n; i++) {
		if (cgroup_ent_exists(file, lineno, loc + i))
			return 1;
	}

	return 0;
}

static const struct tst_cgroup_item cgroup_file_item = {
	.exists = cgroup_file_exists,
};

static void cgroup_file_init(struct tst_cgroup_file *file)
{
	file->item = &cgroup_file_item;
	file->n = 0;
}

static void cgroup_file_add(struct tst_cgroup_file *file,
			    const char *name,
			    const struct tst_cgroup_tree *tree)
{
	file->locations[file->n].name = name;
	file->locations[file->n].tree = tree;
	file->n++;
}

static int cgroup_memory_swap_exists(const char * file, const int lineno,
				     tst_cgroup_item_ptr item)
{
	const struct tst_cgroup_memory_swap *c =
		tst_container_of(item, typeof(*c), item);

	return cgroup_file_exists(file, lineno, &c->max.item);
}

static const struct tst_cgroup_item cgroup_memory_swap_item = {
	.exists = cgroup_memory_swap_exists,
};

static int cgroup_memory_kmem_exists(const char * file, const int lineno,
				     tst_cgroup_item_ptr item)
{
	const struct tst_cgroup_memory_kmem *c =
		tst_container_of(item, typeof(*c), item);

	return cgroup_file_exists(file, lineno, &c->max.item);
}

static const struct tst_cgroup_item cgroup_memory_kmem_item = {
	.exists = cgroup_memory_kmem_exists,
};

static int cgroup_memory_exists(const char * file, const int lineno,
				tst_cgroup_item_ptr item)
{
	const struct tst_cgroup_memory *c =
		tst_container_of(item, typeof(*c), item);

	return cgroup_file_exists(file, lineno, &c->max.item);
}

static struct tst_cgroup_item cgroup_memory_item = {
	.exists = cgroup_memory_exists,
};

static void cgroup_memory_init(struct tst_cgroup_memory *cgm)
{
	cgm->item = &cgroup_memory_item;
	cgm->ver = 0;
	cgroup_file_init(&cgm->current);
	cgroup_file_init(&cgm->max);
	cgroup_file_init(&cgm->swappiness);

	cgm->swap.item = &cgroup_memory_swap_item;
	cgroup_file_init(&cgm->swap.current);
	cgroup_file_init(&cgm->swap.max);

	cgm->kmem.item = &cgroup_memory_kmem_item;
	cgroup_file_init(&cgm->kmem.current);
	cgroup_file_init(&cgm->kmem.max);
}

static void cgroup_memory_add(struct tst_cgroup_memory *cgm,
			      const struct tst_cgroup_tree *tree)
{
	const char *current, *max, *swap_current, *swap_max;

	cgm->ver = tree->root->ver;

	cgroup_file_add(&cgm->kmem.current, "memory.kmem.usage_in_bytes", tree);
	cgroup_file_add(&cgm->kmem.max, "memory.kmem.limit_in_bytes", tree);

	if (tree->root->ver == TST_CGROUP_V1) {
		current = "memory.usage_in_bytes";
		max = "memory.limit_in_bytes";
		swap_current = "memory.memsw.usage_in_bytes";
		swap_max = "memory.memsw.limit_in_bytes";
	} else {
		current = "memory.current";
		max = "memory.max";
		swap_current = "memory.swap.current";
		swap_max = "memory.swap.max";
	}

	cgroup_file_add(&cgm->current, current, tree);
	cgroup_file_add(&cgm->max, max, tree);
	cgroup_file_add(&cgm->swappiness, "memory.swappiness", tree);

	cgroup_file_add(&cgm->swap.current, swap_current, tree);
	cgroup_file_add(&cgm->swap.max, swap_max, tree);
}

static int cgroup_cpuset_exists(const char * file, const int lineno,
				tst_cgroup_item_ptr item)
{
	const struct tst_cgroup_cpuset *c =
		tst_container_of(item, typeof(*c), item);

	return cgroup_file_exists(file, lineno, &c->cpus.item);
}

static struct tst_cgroup_item cgroup_cpuset_item = {
	.exists = cgroup_cpuset_exists,
};

static void cgroup_cpuset_init(struct tst_cgroup_cpuset *cgc)
{
	cgc->item = &cgroup_cpuset_item;
	cgc->ver = 0;

	cgroup_file_init(&cgc->cpus);
	cgroup_file_init(&cgc->mems);

	cgroup_file_init(&cgc->memory_migrate);
}

static void cgroup_cpuset_add(struct tst_cgroup_cpuset *cgc,
			      const struct tst_cgroup_tree *tree)
{
	const char *cpus, *mems, *memory_migrate;

	cgc->ver = tree->root->ver;

	if (tree->root->no_prefix) {
		cpus = "cpus";
		mems = "mems";
		memory_migrate = "memory_migrate";
	} else {
		cpus = "cpuset.cpus";
		mems = "cpuset.mems";
		memory_migrate = "cpuset.memory_migrate";
	}

	cgroup_file_add(&cgc->cpus, cpus, tree);
	cgroup_file_add(&cgc->mems, mems, tree);
	cgroup_file_add(&cgc->memory_migrate, memory_migrate, tree);
}

static void cgroup_cgroup_init(struct tst_cgroup_cgroup *cgg)
{
	cgg->item = &cgroup_default_item;

	cgroup_file_init(&cgg->procs);
	cgroup_file_init(&cgg->clone_children);
	cgroup_file_init(&cgg->subtree_control);
}

static void cgroup_cgroup_add(struct tst_cgroup_cgroup *cgg,
			      const struct tst_cgroup_tree *tree)
{
	if (tree->root->ver == TST_CGROUP_V1) {
		cgroup_file_add(&cgg->procs, "tasks", tree);
		cgroup_file_add(&cgg->clone_children, "clone_children", tree);
		return;
	}

	cgroup_file_add(&cgg->procs, "cgroup.procs", tree);
	cgroup_file_add(&cgg->subtree_control, "cgroup.subtree_control", tree);
}

static void cgroup_init(struct tst_cgroup *cg)
{
	cg->item = &cgroup_default_item;
	cg->n = 0;

	cgroup_cgroup_init(&cg->cgroup);
	cgroup_cpuset_init(&cg->cpuset);
	cgroup_memory_init(&cg->memory);
}

static void cgroup_add(struct tst_cgroup *cg, struct tst_cgroup_tree *tree)
{
	uint32_t ss_field = tree->ss_field;

	cg->trees[cg->n++] = tree;
	cgroup_cgroup_add(&cg->cgroup, tree);

	if (has_css(ss_field, TST_CGROUP_MEMORY))
		cgroup_memory_add(&cg->memory, tree);

	if (has_css(ss_field, TST_CGROUP_CPUSET))
		cgroup_cpuset_add(&cg->cpuset, tree);
}

struct tst_cgroup *tst_cgroup_mk(const struct tst_cgroup *parent,
				 const char *name)
{
	unsigned int i;
	struct tst_cgroup *cg;
	struct tst_cgroup_tree *tree;

	if (!parent->n)
		tst_brk(TBROK, "Trying to make CGroup in empty parent");

	cg = SAFE_MALLOC(sizeof(*cg));
	cgroup_init(cg);

	for (i = 0; i < parent->n; i++) {
		tree = SAFE_MALLOC(sizeof(*tree));
		tst_cgroup_tree_mk(parent->trees[i], name, tree);
		cgroup_add(cg, tree);
	}

	return cg;
}

struct tst_cgroup *tst_cgroup_rm(struct tst_cgroup *cg)
{
	unsigned int i;

	for (i = 0; i < cg->n; i++) {
		close(cg->trees[i]->dir);
		SAFE_UNLINKAT(cg->trees[i]->loc.tree->dir,
			      cg->trees[i]->loc.name,
			      AT_REMOVEDIR);
		free(cg->trees[i]);
	}

	free(cg);
	return NULL;
}

static struct tst_cgroup *cgroup_from_roots(size_t tree_off)
{
	struct cgroup_root *r;
	struct tst_cgroup_tree *t;
	struct tst_cgroup *cg;

	cg = tst_alloc(sizeof(*cg));
	cgroup_init(cg);

	for_each_root(r) {
		t = (typeof(t))(((char *)r) + tree_off);

		if (t->ss_field)
			cgroup_add(cg, t);
	}

	if (cg->n)
		return cg;

	tst_brk(TBROK,
		"No CGroups found; maybe you forgot to call tst_cgroup_require?");

	return cg;
}

const struct tst_cgroup *tst_cgroup_get_default(void)
{
	return cgroup_from_roots(offsetof(struct cgroup_root, test));
}

const struct tst_cgroup *tst_cgroup_get_drain(void)
{
	return cgroup_from_roots(offsetof(struct cgroup_root, drain));
}

static void cgroup_file_populated(const char *file, const int lineno,
				  const struct tst_cgroup_file *cgf)
{
	if (cgf->n)
		return;

	tst_brk_(file, lineno, TBROK,
		 "CGroup item not populated. Maybe missing tst_cgroup_require() or TST_CGROUP_HAS()");
}

ssize_t safe_cgroup_read(const char *file, const int lineno,
			 const struct tst_cgroup_file *cgf,
			 char *out, size_t len)
{
	unsigned int i;
	size_t min_len;
	const struct tst_cgroup_location *loc = cgf->locations;
	char buf[BUFSIZ];

	cgroup_file_populated(file, lineno, cgf);

	for (i = 0; i < cgf->n; i++) {
		TEST(safe_file_readat(file, lineno,
				      loc[i].tree->dir, loc[i].name, out, len));
		if (TST_RET < 0)
			continue;

		min_len = MIN(sizeof(buf), (size_t)TST_RET);

		if (i > 0 && memcmp(out, buf, min_len)) {
			tst_brk_(file, lineno, TBROK,
				 "%s has different value across roots",
				 loc[i].name);
			break;
		}

		if (i >= cgf->n)
			break;

		memcpy(buf, out, min_len);
	}

	out[MAX(TST_RET, 0)] = '\0';

	return TST_RET;
}

void safe_cgroup_printf(const char *file, const int lineno,
			const struct tst_cgroup_file *cgf,
			const char *fmt, ...)
{
	unsigned int i;
	const struct tst_cgroup_location *loc = cgf->locations;
	va_list va;

	cgroup_file_populated(file, lineno, cgf);

	for (i = 0; i < cgf->n; i++) {
		va_start(va, fmt);
		safe_file_vprintfat(file, lineno,
				    loc[i].tree->dir, loc[i].name, fmt, va);
		va_end(va);
	}
}

void safe_cgroup_scanf(const char *file, const int lineno,
		       const struct tst_cgroup_file *cgf,
		       const char *fmt, ...)
{
	va_list va;
	char buf[BUFSIZ];
	const struct tst_cgroup_location *loc;
	ssize_t len = safe_cgroup_read(file, lineno, cgf, buf, sizeof(buf));

	if (len < 1)
		return;

	va_start(va, fmt);
	if (vsscanf(buf, fmt, va) < 1) {
		loc = cgf->locations;
		tst_brk_(file, lineno, TBROK | TERRNO,
			 "'%s/%s': vsscanf('%s', '%s', ...)",
			 tst_decode_fd(loc->tree->dir), loc->name, buf, fmt);
	}
	va_end(va);
}
