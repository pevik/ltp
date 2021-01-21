// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Red Hat, Inc.
 * Copyright (c) 2020 Li Wang <liwang@redhat.com>
 * Copyright (c) 2020 Richard Palethorpe <rpalethorpe@suse.com>
 */

#ifndef TST_CGROUP_H
#define TST_CGROUP_H

enum tst_cgroup_ver {
	TST_CGROUP_V1 = 1,
	TST_CGROUP_V2 = 2,
};

enum tst_cgroup_ctrl {
	TST_CGROUP_MEMORY = 1,
	TST_CGROUP_CPUSET = 2,
};
#define TST_CGROUP_MAX TST_CGROUP_CPUSET

/* The level of CGroup cleanup which will be attempted after the
 * test. When running tests in parallel you should select
 * TST_CGORUP_CLEANUP_{TEST,NONE}. Note that we only try to cleanup
 * CGroups we mounted/created in the current test.
 */
enum tst_cgroup_cleanup {
	/* Unmount the CGroup roots */
	TST_CGROUP_CLEANUP_ROOT,
	/* Cleanup up test CGroups and the overall LTP CGroup */
	TST_CGROUP_CLEANUP_LTP,
	/* Cleanup up individual test CGroups */
	TST_CGROUP_CLEANUP_TEST,
	/* Don't drain or delete any groups, including the individual
	 * test CGroups
	 */
	TST_CGROUP_CLEANUP_NONE,
};

/* Used to specify CGroup hierarchy configuration options, allowing a
 * test to request a particular CGroup structure.
 *
 * It is expected this will expand to include options for changing
 * things such as the cgroup names, threading, inheritance etc.
 */
struct tst_cgroup_opts {
	/* Only try to mount V1 CGroup controllers. This will not
	 * prevent V2 from being used if it is already mounted, it
	 * only indicates that we should mount V1 controllers if
	 * nothing is present. By default we try to mount V2 first. */
	int only_mount_v1:1;
	enum tst_cgroup_cleanup cleanup;
};

/* Search the system for mounted cgroups and available
 * controllers. Called automatically by tst_cgroup_require.
 */
void tst_cgroup_scan(void);
/* Print the config detected by tst_cgroup_scan */
void tst_cgroup_print_config(void);

/* Read from a controller file in the test's default CGroup. Use
 * tst_cgroup_require to ensure the specified controller is active on
 * the test's default CGroup
 */
#define SAFE_CGROUP_READ(type, path, buf, nbyte) \
	safe_cgroup_printf(__FILE__, __LINE__, (type), (path), (buf), (nbyte))

ssize_t safe_cgroup_read(const char *file, const int lineno,
			 enum tst_cgroup_ctrl type,
			 const char *path, char *buf, size_t nbyte);

/* Print to a control file in the test's default CGroup */
#define SAFE_CGROUP_PRINTF(type, path, fmt, ...) \
	safe_cgroup_printf(__FILE__, __LINE__, (type), (path), (fmt), __VA_ARGS__)

void safe_cgroup_printf(const char *file, const int lineno,
			enum tst_cgroup_ctrl type,
			const char *path, const char *fmt, ...)
			__attribute__ ((format (printf, 5, 6)));

/* Ensure the specified controller is available in the test's default
 * CGroup, mounting/enabling it if necessary */
void tst_cgroup_require(enum tst_cgroup_ctrl type,
			const struct tst_cgroup_opts *options);

/* Tear down any CGroups created by calls to tst_cgroup_require */
void tst_cgroup_cleanup(const struct tst_cgroup_opts *options);

/* Move the current process to the test's default CGroup */
void tst_cgroup_move_current(enum tst_cgroup_ctrl type);

/* Set of functions to set knobs under the test's default memory
 * controller.
 */
void tst_cgroup_mem_set_maxbytes(size_t memsz);
int  tst_cgroup_mem_swapacct_enabled(void);
void tst_cgroup_mem_set_maxswap(size_t memsz);

/* Set of functions to read/write the test's default cpuset controller
 * files content.
 */
void tst_cgroup_cpuset_read_files(const char *name, char *buf, size_t nbyte);
void tst_cgroup_cpuset_write_files(const char *name, const char *buf);

#endif /* TST_CGROUP_H */
