// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2024 Cyril Hrubis <chrubis@suse.cz>
 */
#include <sys/mount.h>

#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"
#include "tst_safe_stdio.h"
#include "ujson.h"

static char *shell_filename;

static void run_shell(void)
{
	tst_run_shell(shell_filename, NULL);
}

struct tst_test test = {
	.test_all = run_shell,
	.runs_script = 1,
};

static void print_help(void)
{
	printf("Usage: tst_shell_loader ltp_shell_test.sh ...");
}

static char *metadata;
static size_t metadata_size;
static size_t metadata_used;

static void metadata_append(const char *line)
{
	size_t linelen = strlen(line);

	if (metadata_size - metadata_used < linelen + 1) {
		metadata_size += 128;
		metadata = SAFE_REALLOC(metadata, metadata_size);
	}

	strcpy(metadata + metadata_used, line);
	metadata_used += linelen;
}

static ujson_obj_attr test_attrs[] = {
	UJSON_OBJ_ATTR("all_filesystems", UJSON_BOOL),
	UJSON_OBJ_ATTR("dev_min_size", UJSON_INT),
	UJSON_OBJ_ATTR("filesystems", UJSON_ARR),
	UJSON_OBJ_ATTR("format_device", UJSON_BOOL),
	UJSON_OBJ_ATTR("min_cpus", UJSON_INT),
	UJSON_OBJ_ATTR("min_mem_avail", UJSON_INT),
	UJSON_OBJ_ATTR("min_kver", UJSON_STR),
	UJSON_OBJ_ATTR("min_swap_avail", UJSON_INT),
	UJSON_OBJ_ATTR("mntpoint", UJSON_STR),
	UJSON_OBJ_ATTR("mount_device", UJSON_BOOL),
	UJSON_OBJ_ATTR("needs_abi_bits", UJSON_INT),
	UJSON_OBJ_ATTR("needs_devfs", UJSON_BOOL),
	UJSON_OBJ_ATTR("needs_device", UJSON_BOOL),
	UJSON_OBJ_ATTR("needs_hugetlbfs", UJSON_BOOL),
	UJSON_OBJ_ATTR("needs_rofs", UJSON_BOOL),
	UJSON_OBJ_ATTR("needs_root", UJSON_BOOL),
	UJSON_OBJ_ATTR("needs_tmpdir", UJSON_BOOL),
	UJSON_OBJ_ATTR("restore_wallclock", UJSON_BOOL),
	UJSON_OBJ_ATTR("skip_filesystems", UJSON_ARR),
	UJSON_OBJ_ATTR("skip_in_compat", UJSON_BOOL),
	UJSON_OBJ_ATTR("skip_in_lockdown", UJSON_BOOL),
	UJSON_OBJ_ATTR("skip_in_secureboot", UJSON_BOOL),
	UJSON_OBJ_ATTR("supported_archs", UJSON_ARR),
};

static ujson_obj test_obj = {
	.attrs = test_attrs,
	.attr_cnt = UJSON_ARRAY_SIZE(test_attrs),
};

/* Must match the order of test_attrs. */
enum test_attr_ids {
	ALL_FILESYSTEMS,
	DEV_MIN_SIZE,
	FILESYSTEMS,
	FORMAT_DEVICE,
	MIN_CPUS,
	MIN_MEM_AVAIL,
	MIN_KVER,
	MIN_SWAP_AVAIL,
	MNTPOINT,
	MOUNT_DEVICE,
	NEEDS_ABI_BITS,
	NEEDS_DEVFS,
	NEEDS_DEVICE,
	NEEDS_HUGETLBFS,
	NEEDS_ROFS,
	NEEDS_ROOT,
	NEEDS_TMPDIR,
	RESTORE_WALLCLOCK,
	SKIP_FILESYSTEMS,
	SKIP_IN_COMPAT,
	SKIP_IN_LOCKDOWN,
	SKIP_IN_SECUREBOOT,
	SUPPORTED_ARCHS,
};

static const char *const *parse_strarr(ujson_reader *reader, ujson_val *val)
{
	unsigned int cnt = 0, i = 0;
	char **ret;

	ujson_reader_state state = ujson_reader_state_save(reader);

	UJSON_ARR_FOREACH(reader, val) {
		if (val->type != UJSON_STR) {
			ujson_err(reader, "Expected string!");
			return NULL;
		}

		cnt++;
	}

	ujson_reader_state_load(reader, state);

	ret = SAFE_MALLOC(sizeof(char*) * (cnt + 1));

	UJSON_ARR_FOREACH(reader, val) {
		ret[i++] = strdup(val->val_str);
	}

	ret[i] = NULL;

	return (const char *const *)ret;
}

static ujson_obj_attr fs_attrs[] = {
	UJSON_OBJ_ATTR("mkfs_opts", UJSON_ARR),
	UJSON_OBJ_ATTR("mkfs_size_opt", UJSON_STR),
	UJSON_OBJ_ATTR("mnt_flags", UJSON_ARR),
	UJSON_OBJ_ATTR("type", UJSON_STR),
};

static ujson_obj fs_obj = {
	.attrs = fs_attrs,
	.attr_cnt = UJSON_ARRAY_SIZE(fs_attrs),
};

/* Must match the order of fs_attrs. */
enum fs_ids {
	MKFS_OPTS,
	MKFS_SIZE_OPT,
	MNT_FLAGS,
	TYPE,
};

static int parse_mnt_flags(ujson_reader *reader, ujson_val *val)
{
	int ret = 0;

	UJSON_ARR_FOREACH(reader, val) {
		if (val->type != UJSON_STR) {
			ujson_err(reader, "Expected string!");
			return ret;
		}

		if (!strcmp(val->val_str, "RDONLY"))
			ret |= MS_RDONLY;
		else if (!strcmp(val->val_str, "NOATIME"))
			ret |= MS_NOATIME;
		else if (!strcmp(val->val_str, "NOEXEC"))
			ret |= MS_NOEXEC;
		else if (!strcmp(val->val_str, "NOSUID"))
			ret |= MS_NOSUID;
		else
			ujson_err(reader, "Invalid mount flag");
	}

	return ret;
}

struct tst_fs *parse_filesystems(ujson_reader *reader, ujson_val *val)
{
	unsigned int i = 0, cnt = 0;
	struct tst_fs *ret;

	ujson_reader_state state = ujson_reader_state_save(reader);

	UJSON_ARR_FOREACH(reader, val) {
		if (val->type != UJSON_OBJ) {
			ujson_err(reader, "Expected object!");
			return NULL;
		}
		ujson_obj_skip(reader);
		cnt++;
	}

	ujson_reader_state_load(reader, state);

	ret = SAFE_MALLOC(sizeof(struct tst_fs) * (cnt + 1));
	memset(ret, 0, sizeof(*ret) * (cnt+1));

	UJSON_ARR_FOREACH(reader, val) {
		UJSON_OBJ_FOREACH_FILTER(reader, val, &fs_obj, ujson_empty_obj) {
			switch ((enum fs_ids)val->idx) {
			case MKFS_OPTS:
				ret[i].mkfs_opts = parse_strarr(reader, val);
			break;
			case MKFS_SIZE_OPT:
				ret[i].mkfs_size_opt = strdup(val->val_str);
			break;
			case MNT_FLAGS:
				ret[i].mnt_flags = parse_mnt_flags(reader, val);
			break;
			case TYPE:
				ret[i].type = strdup(val->val_str);
			break;
			}

		}

		i++;
	}

	return ret;
}

static void parse_metadata(void)
{
	ujson_reader reader = UJSON_READER_INIT(metadata, metadata_used, UJSON_READER_STRICT);
	char str_buf[128];
	ujson_val val = UJSON_VAL_INIT(str_buf, sizeof(str_buf));

	UJSON_OBJ_FOREACH_FILTER(&reader, &val, &test_obj, ujson_empty_obj) {
		switch ((enum test_attr_ids)val.idx) {
		case ALL_FILESYSTEMS:
			test.all_filesystems = val.val_bool;
		break;
		case DEV_MIN_SIZE:
			if (val.val_int <= 0)
				ujson_err(&reader, "Device size must be > 0");
			else
				test.dev_min_size = val.val_int;
		break;
		case FILESYSTEMS:
			test.filesystems = parse_filesystems(&reader, &val);
		break;
		case FORMAT_DEVICE:
			test.format_device = val.val_bool;
		break;
		case MIN_CPUS:
			if (val.val_int <= 0)
				ujson_err(&reader, "Minimal number of cpus must be > 0");
			else
				test.min_cpus = val.val_int;
		break;
		case MIN_MEM_AVAIL:
			if (val.val_int <= 0)
				ujson_err(&reader, "Minimal available memory size must be > 0");
			else
				test.min_mem_avail = val.val_int;
		break;
		case MIN_KVER:
			test.min_kver = strdup(val.val_str);
		break;
		case MIN_SWAP_AVAIL:
			if (val.val_int <= 0)
				ujson_err(&reader, "Minimal available swap size must be > 0");
			else
				test.min_swap_avail = val.val_int;
		break;
		case MNTPOINT:
			test.mntpoint = strdup(val.val_str);
		break;
		case MOUNT_DEVICE:
			test.mount_device = val.val_bool;
		break;
		case NEEDS_ABI_BITS:
			if (val.val_int == 32 || val.val_int == 64)
				test.needs_abi_bits = val.val_int;
			else
				ujson_err(&reader, "ABI bits must be 32 or 64");
		break;
		case NEEDS_DEVFS:
			test.needs_devfs = val.val_bool;
		break;
		case NEEDS_DEVICE:
			test.needs_device = val.val_bool;
		break;
		case NEEDS_HUGETLBFS:
			test.needs_hugetlbfs = val.val_bool;
		break;
		case NEEDS_ROFS:
			test.needs_rofs = val.val_bool;
		break;
		case NEEDS_ROOT:
			test.needs_root = val.val_bool;
		break;
		case NEEDS_TMPDIR:
			test.needs_tmpdir = val.val_bool;
		break;
		case RESTORE_WALLCLOCK:
			test.restore_wallclock = val.val_bool;
		break;
		case SKIP_FILESYSTEMS:
			test.skip_filesystems = parse_strarr(&reader, &val);
		break;
		case SKIP_IN_COMPAT:
			test.skip_in_compat = val.val_bool;
		break;
		case SKIP_IN_LOCKDOWN:
			test.skip_in_lockdown = val.val_bool;
		break;
		case SKIP_IN_SECUREBOOT:
			test.skip_in_secureboot = val.val_bool;
		break;
		case SUPPORTED_ARCHS:
			test.supported_archs = parse_strarr(&reader, &val);
		break;
		}
	}

	ujson_reader_finish(&reader);

	if (ujson_reader_err(&reader))
		tst_brk(TBROK, "Invalid metadata");
}

static void extract_metadata(void)
{
	FILE *f;
	char line[4096];
	char path[4096];
	int in_json = 0;

	if (tst_get_path(shell_filename, path, sizeof(path)) == -1)
		tst_brk(TBROK, "Failed to find %s in $PATH", shell_filename);

	f = SAFE_FOPEN(path, "r");

	while (fgets(line, sizeof(line), f)) {
		if (in_json)
			metadata_append(line + 2);

		if (in_json) {
			if (!strcmp(line, "# }\n"))
				in_json = 0;
		} else {
			if (!strcmp(line, "# TEST = {\n")) {
				metadata_append("{\n");
				in_json = 1;
			}
		}
	}

	fclose(f);
}

static void prepare_test_struct(void)
{
	extract_metadata();

	if (metadata)
		parse_metadata();
	else
		tst_brk(TBROK, "No metadata found!");
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
