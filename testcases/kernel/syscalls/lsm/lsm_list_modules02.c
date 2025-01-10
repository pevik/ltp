// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2024 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that lsm_list_modules syscall is correctly recognizing LSM(s) enabled
 * inside the system.
 *
 * [Algorithm]
 *
 * - read enabled LSM(s) inside /sys/kernel/security/lsm file
 * - collect LSM IDs using lsm_list_modules syscall
 * - compare the results, verifying that LSM(s) IDs are correct
 */

#include "lsm_common.h"

#define MAX_LSM_NUM 32

static char lsm_names[MAX_LSM_NUM][BUFSIZ];
static size_t lsm_names_count;
static uint32_t page_size;
static uint64_t *ids;
static uint32_t *size;

static void run(void)
{
	uint32_t lsm_num;
	size_t count;

	memset(ids, 0, sizeof(uint64_t) * MAX_LSM_NUM);
	*size = page_size;

	lsm_num = TST_EXP_POSITIVE(lsm_list_modules(ids, size, 0));

	TST_EXP_EQ_LI(lsm_num, lsm_names_count);
	TST_EXP_EQ_LI(*size, lsm_num * sizeof(uint64_t));

	for (uint32_t i = 0; i < lsm_num; i++) {
		char *name = NULL;

		switch (ids[i]) {
		case LSM_ID_CAPABILITY:
			name = "capability";
			break;
		case LSM_ID_SELINUX:
			name = "selinux";
			break;
		case LSM_ID_SMACK:
			name = "smack";
			break;
		case LSM_ID_TOMOYO:
			name = "tomoyo";
			break;
		case LSM_ID_APPARMOR:
			name = "apparmor";
			break;
		case LSM_ID_YAMA:
			name = "yama";
			break;
		case LSM_ID_LOADPIN:
			name = "loadpin";
			break;
		case LSM_ID_SAFESETID:
			name = "safesetid";
			break;
		case LSM_ID_LOCKDOWN:
			name = "lockdown";
			break;
		case LSM_ID_BPF:
			name = "bpf";
			break;
		case LSM_ID_LANDLOCK:
			name = "landlock";
			break;
		case LSM_ID_IMA:
			name = "ima";
			break;
		case LSM_ID_EVM:
			name = "evm";
			break;
		case LSM_ID_IPE:
			name = "ipe";
			break;
		default:
			break;
		}

		if (!name)
			tst_brk(TBROK, "Unsupported LSM: %lu", ids[i]);

		for (count = 0; count < lsm_names_count; count++) {
			if (!strcmp(name, lsm_names[count])) {
				tst_res(TPASS, "'%s' is enabled", name);
				break;
			}
		}

		if (count >= lsm_names_count)
			tst_res(TFAIL, "'%s' has not been found", name);
	}
}

static void setup(void)
{
	int fd;
	char *ptr;
	char data[BUFSIZ];

	memset(data, 0, BUFSIZ);

	page_size = SAFE_SYSCONF(_SC_PAGESIZE);
	fd = SAFE_OPEN("/sys/kernel/security/lsm", O_RDONLY);
	SAFE_READ(0, fd, data, BUFSIZ);
	SAFE_CLOSE(fd);

	ptr = strtok(data, ",");

	while (ptr != NULL) {
		strcpy(lsm_names[lsm_names_count], ptr);
		ptr = strtok(NULL, ",");
		lsm_names_count++;
	}
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.min_kver = "6.8",
	.bufs = (struct tst_buffers []) {
		{&ids, .size = sizeof(uint64_t) * MAX_LSM_NUM},
		{&size, .size = sizeof(uint32_t)},
		{},
	},
};
