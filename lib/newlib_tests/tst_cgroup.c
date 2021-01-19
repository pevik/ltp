#include <stdio.h>

#include "tst_test.h"
#include "tst_cgroup.h"

static char *only_mount_v1;
static char *no_cleanup;
static struct tst_option opts[] = {
	{"v", &only_mount_v1, "-v\tOnly try to mount CGroups V1"},
	{"n", &no_cleanup, "-n\tLeave CGroups created by test"},
	{NULL, NULL, NULL},
};
struct tst_cgroup_opts cgopts;

static void do_test(void)
{
	char buf[BUFSIZ];
	pid_t pid = SAFE_FORK();

	if (!pid) {
		tst_cgroup_mem_set_maxbytes((1UL << 24) - 1);
		tst_cgroup_mem_set_maxswap(1UL << 31);
		tst_cgroup_move_current(TST_CGROUP_MEMORY);

		tst_cgroup_cpuset_read_files("mems", buf, sizeof(buf));
		tst_cgroup_cpuset_write_files("mems", buf);
		tst_cgroup_move_current(TST_CGROUP_CPUSET);

		tst_res(TPASS, "Cgroup mount test");
	}

	tst_reap_children();
}

static void setup(void)
{
	cgopts.only_mount_v1 = !!only_mount_v1,
	cgopts.cleanup =
		no_cleanup ? TST_CGROUP_CLEANUP_NONE : TST_CGROUP_CLEANUP_ROOT;

	tst_cgroup_scan();
	tst_cgroup_print_config();

	tst_cgroup_require(TST_CGROUP_MEMORY, &cgopts);
	tst_cgroup_print_config();
	tst_cgroup_require(TST_CGROUP_CPUSET, &cgopts);
	tst_cgroup_print_config();
}

static void cleanup(void)
{
	tst_cgroup_cleanup(&cgopts);
}

static struct tst_test test = {
	.test_all = do_test,
	.setup = setup,
	.cleanup = cleanup,
	.options = opts,
	.forks_child = 1,
};
