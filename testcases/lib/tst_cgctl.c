#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "tst_cgroup.h"

static int cgctl_require(const char *ctrl, int test_pid)
{
    struct tst_cgroup_opts opts;

    memset(&opts, 0, sizeof(opts));
    opts.test_pid = test_pid;

    tst_cgroup_require(ctrl, &opts);
    tst_cgroup_print_config();

    return 0;
}

static int cgctl_cleanup(const char *config)
{
    tst_cgroup_scan();
    tst_cgroup_load_config(config);
    tst_cgroup_cleanup();

    return 0;
}

static int cgctl_print(void)
{
    tst_cgroup_scan();
    tst_cgroup_print_config();

    return 0;
}

static int cgctl_process_cmd(int argc, char *argv[])
{
    int test_pid;
    const char *cmd_name = argv[1];

    if (!strcmp(cmd_name, "require")) {
        test_pid = atoi(argv[3]);
        if (!test_pid) {
            fprintf(stderr, "tst_cgctl: Invalid test_pid '%s' given\n",
                    argv[3]);
            return 1;
        }
        return cgctl_require(argv[2], test_pid);
    } else if (!strcmp(cmd_name, "cleanup")) {
        return cgctl_cleanup(argv[2]);
    } else if (!strcmp(cmd_name, "print")) {
        return cgctl_print();
    }

    fprintf(stderr, "tst_cgctl: Unknown command '%s' given\n", cmd_name);
    return 1;
}

int main(int argc, char *argv[])
{
    if (argc < 2 || argc > 4) {
        fprintf(stderr, "tst_cgctl: Invalid number of arguements given");
        return 1;
    }

    return cgctl_process_cmd(argc, argv);
}
