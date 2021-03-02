// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020-2021 SUSE LLC <rpalethorpe@suse.com>
 */
#ifndef TST_CGROUP_CORE_H
#define TST_CGROUP_CORE_H

enum tst_cgroup_ver {
	TST_CGROUP_V1 = 1,
	TST_CGROUP_V2 = 2,
};

enum tst_cgroup_ctrl {
	TST_CGROUP_MEMORY = 1,
	TST_CGROUP_CPUSET = 2,
};
#define TST_CGROUP_MAX TST_CGROUP_CPUSET

/* At most we can have one cgroup V1 tree for each controller and one
 * (empty) v2 tree.
 */
#define TST_CGROUP_MAX_TREES (TST_CGROUP_MAX + 1)


/* Used to specify CGroup hierarchy configuration options, allowing a
 * test to request a particular CGroup structure.
 */
struct tst_cgroup_opts {
	/* Only try to mount V1 CGroup controllers. This will not
	 * prevent V2 from being used if it is already mounted, it
	 * only indicates that we should mount V1 controllers if
	 * nothing is present. By default we try to mount V2 first. */
	int only_mount_v1:1;
};

struct tst_cgroup_tree;

struct tst_cgroup_location {
	const char *name;
	const struct tst_cgroup_tree *tree;
};

/* Search the system for mounted cgroups and available
 * controllers. Called automatically by tst_cgroup_require.
 */
void tst_cgroup_scan(void);
/* Print the config detected by tst_cgroup_scan */
void tst_cgroup_print_config(void);

/* Ensure the specified controller is available in the test's default
 * CGroup, mounting/enabling it if necessary */
void tst_cgroup_require(enum tst_cgroup_ctrl type,
			const struct tst_cgroup_opts *options);

/* Tear down any CGroups created by calls to tst_cgroup_require */
void tst_cgroup_cleanup(void);

#endif /* TST_CGROUP_CORE_H */
