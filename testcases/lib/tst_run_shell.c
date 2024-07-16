// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Cyril Hrubis <chrubis@suse.cz>
 */
#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"
#include "tst_safe_stdio.h"

static char *shell_filename;

static void run_shell(void)
{
	tst_run_shell(shell_filename, NULL);
}

struct tst_test test = {
	.test_all = run_shell,
	.runs_shell = 1,
};

static void print_help(void)
{
	printf("Usage: tst_shell_loader ltp_shell_test.sh ...");
}

#define FLAG_MATCH(str, name) (!strcmp(str, name "=1"))
#define PARAM_MATCH(str, name) (!strncmp(str, name "=", sizeof(name)))
#define PARAM_VAL(str, name) strdup(str + sizeof(name))

static void prepare_test_struct(void)
{
	FILE *f;
	char line[4096];
	char path[4096];

	if (tst_get_path(shell_filename, path, sizeof(path)) == -1)
		tst_brk(TBROK, "Failed to find %s in $PATH", shell_filename);

	f = SAFE_FOPEN(path, "r");

	while (fscanf(f, "%4096s[^\n]", line) != EOF) {
		if (FLAG_MATCH(line, "needs_tmpdir"))
			test.needs_tmpdir = 1;
		else if (FLAG_MATCH(line, "needs_root"))
			test.needs_root = 1;
		else if (FLAG_MATCH(line, "needs_device"))
			test.needs_device = 1;
		else if (FLAG_MATCH(line, "needs_checkpoints"))
			test.needs_checkpoints = 1;
		else if (FLAG_MATCH(line, "needs_overlay"))
			test.needs_overlay = 1;
		else if (FLAG_MATCH(line, "format_device"))
			test.format_device = 1;
		else if (FLAG_MATCH(line, "mount_device"))
			test.mount_device = 1;
		else if (PARAM_MATCH(line, "TST_MNTPOINT"))
			test.mntpoint = PARAM_VAL(line, "TST_MNTPOINT");
		else if (FLAG_MATCH(line, "all_filesystems"))
			test.all_filesystems = 1;
	}

	fclose(f);
}

int main(int argc, char *argv[])
{
	if (argc < 2)
		goto help;

	shell_filename = argv[1];

	prepare_test_struct();

	tst_run_tcases(argc - 1, argv + 1, &test);
help:
	print_help();
	return 1;
}
