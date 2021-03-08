// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020-2021 SUSE LLC <rpalethorpe@suse.com>
 */
#ifndef TST_CGROUP_ITEM_H
#define TST_CGROUP_ITEM_H

/* CGroup Item Declarations */


typedef const struct tst_cgroup_item *const *tst_cgroup_item_ptr;
/* Base CGroup Item operations
 *
 * Note, for all callbacks, we pass a pointer to the item field within
 * the containing object. Then use the container_of trick to get the
 * specific tst_cgroup_* struct. This avoids passing a void pointer
 * which has no type checking atall.
 */
struct tst_cgroup_item {
	int (*exists)(const char *, const int, tst_cgroup_item_ptr);
};

struct tst_cgroup_file {
	const struct tst_cgroup_item *item;

	unsigned int n;
	struct tst_cgroup_location locations[TST_CGROUP_MAX_TREES];
};

/* Generic Control Group (cgroup) parameters */
struct tst_cgroup_cgroup {
	const struct tst_cgroup_item *item;

	/* tasks on V1, cgroup.procs on V2 */
	struct tst_cgroup_file procs;
	/* Only V1 */
	struct tst_cgroup_file clone_children;
	/* Only V2 */
	struct tst_cgroup_file subtree_control;
};

/* On V1 this is memsw */
struct tst_cgroup_memory_swap {
	const struct tst_cgroup_item *item;

	struct tst_cgroup_file current;
	struct tst_cgroup_file max;
};

/* Only on V1 (but same info may be in memory.stat on V2) */
struct tst_cgroup_memory_kmem {
	const struct tst_cgroup_item *item;

	struct tst_cgroup_file current;
	struct tst_cgroup_file max;
};

/* memory (memcg) controller */
struct tst_cgroup_memory {
	const struct tst_cgroup_item *item;
	enum tst_cgroup_ver ver;

	struct tst_cgroup_memory_kmem kmem;
	struct tst_cgroup_memory_swap swap;

	/* usage_in_bytes on V1 */
	struct tst_cgroup_file current;
	/* limit_in_bytes on V1 */
	struct tst_cgroup_file max;
	/* Only on V1 as of 5.11 */
	struct tst_cgroup_file swappiness;
};

/* cpuset controller */
struct tst_cgroup_cpuset {
	const struct tst_cgroup_item *item;
	enum tst_cgroup_ver ver;

	struct tst_cgroup_file cpus;
	struct tst_cgroup_file mems;

	struct tst_cgroup_file memory_migrate;
};

/* A Control Group in LTP's aggregated hierarchy */
struct tst_cgroup {
	const struct tst_cgroup_item *item;

	unsigned int n;
	struct tst_cgroup_tree *trees[TST_CGROUP_MAX_TREES];

	struct tst_cgroup_cgroup cgroup;
	struct tst_cgroup_cpuset cpuset;
	struct tst_cgroup_memory memory;
};

/* Get the default CGroup for the test. It allocates memory (in a
 * guarded buffer) so should always be called from setup
 */
const struct tst_cgroup *tst_cgroup_get_default(void);
/* Get the shared drain group. Also should be called from setup */
const struct tst_cgroup *tst_cgroup_get_drain(void);
/* Create a descendant CGroup */
struct tst_cgroup *tst_cgroup_mk(const struct tst_cgroup *parent,
				 const char *name);
/* Remove a descendant CGroup */
struct tst_cgroup *tst_cgroup_rm(struct tst_cgroup *cg);

#define SAFE_CGROUP_READ(file, out, len) \
	safe_cgroup_read(__FILE__, __LINE__, (file), (out), (len));

ssize_t safe_cgroup_read(const char *file, const int lineno,
			 const struct tst_cgroup_file *cgf,
			 char *out, size_t len);

/* Print to a control file in the test's default CGroup */
#define SAFE_CGROUP_PRINTF(file, fmt, ...) \
	safe_cgroup_printf(__FILE__, __LINE__, (file), (fmt), __VA_ARGS__)

#define SAFE_CGROUP_PRINT(file, str) \
	safe_cgroup_printf(__FILE__, __LINE__, (file), "%s", (str))

void safe_cgroup_printf(const char *file, const int lineno,
			const struct tst_cgroup_file *cgf,
			const char *fmt, ...)
			__attribute__ ((format (printf, 4, 5)));

#define SAFE_CGROUP_SCANF(file, fmt, ...)				\
	safe_cgroup_scanf(__FILE__, __LINE__, (file), (fmt), __VA_ARGS__)

void safe_cgroup_scanf(const char *file, const int lineno,
		       const struct tst_cgroup_file *cgf,
		       const char *fmt, ...)
		       __attribute__ ((format (scanf, 4, 5)));

#define TST_CGROUP_HAS(obj) \
	(((obj)->item)->exists(__FILE__, __LINE__, &(obj)->item))

#endif /* TST_CGROUP_ITEM_H */
