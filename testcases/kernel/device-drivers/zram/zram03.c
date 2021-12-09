// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2010  Red Hat, Inc.
 */

/*\
 * [Description]
 *
 * zram: generic RAM based compressed R/W block devices
 * http://lkml.org/lkml/2010/8/9/227
 *
 * This case check whether data read from zram device is consistent with
 * thoese are written.
 */


#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "tst_test.h"

#define PATH_ZRAM	"/sys/block/zram0"
#define OBSOLETE_ZRAM_FILE	"/sys/block/zram0/num_reads"
#define PATH_ZRAM_STAT	"/sys/block/zram0/stat"
#define PATH_ZRAM_MM_STAT	"/sys/block/zram0/mm_stat"
#define SIZE		(512 * 1024 * 1024L)
#define DEVICE		"/dev/zram0"

static int modprobe;
static const char *const cmd_rmmod[] = {"rmmod", "zram", NULL};

static void set_disksize(void)
{
	tst_res(TINFO, "create a zram device with %ld bytes in size.", SIZE);
	SAFE_FILE_PRINTF(PATH_ZRAM "/disksize", "%ld", SIZE);
}

static void write_device(void)
{
	int fd;
	char *s;

	tst_res(TINFO, "map this zram device into memory.");
	fd = SAFE_OPEN(DEVICE, O_RDWR);
	s = SAFE_MMAP(NULL, SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

	tst_res(TINFO, "write all the memory.");
	memset(s, 'a', SIZE - 1);
	s[SIZE - 1] = '\0';

	SAFE_MUNMAP(s, SIZE);
	SAFE_CLOSE(fd);
}

static void verify_device(void)
{
	int fd;
	long i = 0, fail = 0;
	char *s;

	tst_res(TINFO, "verify contents from device.");
	fd = SAFE_OPEN(DEVICE, O_RDONLY);
	s = SAFE_MMAP(NULL, SIZE, PROT_READ, MAP_PRIVATE, fd, 0);

	while (s[i] && i < SIZE - 1) {
		if (s[i] != 'a')
			fail++;
		i++;
	}
	if (i != SIZE - 1) {
		tst_res(TFAIL, "expect size: %ld, actual size: %ld.",
			 SIZE - 1, i);
	} else if (s[i] != '\0') {
		tst_res(TFAIL, "zram device seems not null terminated");
	} else if (fail) {
		tst_res(TFAIL, "%ld failed bytes found.", fail);
	} else {
		tst_res(TPASS, "data read from zram device is consistent "
			 "with those are written");
	}

	SAFE_MUNMAP(s, SIZE);
	SAFE_CLOSE(fd);
}

static void reset(void)
{
	tst_res(TINFO, "%s...", __func__);
	SAFE_FILE_PRINTF(PATH_ZRAM "/reset", "1");
}

static void print(char *string)
{
	char filename[BUFSIZ], value[BUFSIZ];

	sprintf(filename, "%s/%s", PATH_ZRAM, string);
	SAFE_FILE_SCANF(filename, "%s", value);
	tst_res(TINFO, "%s is %s", filename, value);
}

static void print_stat(char *nread, char *nwrite)
{
	char nread_val[BUFSIZ], nwrite_val[BUFSIZ];

	SAFE_FILE_SCANF(PATH_ZRAM_STAT, "%s %*s %*s %*s %s", nread_val, nwrite_val);
	tst_res(TINFO, "%s from %s is %s", nread, PATH_ZRAM_STAT, nread_val);
	tst_res(TINFO, "%s from %s is %s", nwrite, PATH_ZRAM_STAT, nwrite_val);
}

static void print_mm_stat(char *orig, char *compr, char *mem, char *zero)
{
	char orig_val[BUFSIZ], compr_val[BUFSIZ];
	char mem_val[BUFSIZ], zero_val[BUFSIZ];

	SAFE_FILE_SCANF(PATH_ZRAM_MM_STAT, "%s %s %s %*s %*s %s",
			orig_val, compr_val, mem_val, zero_val);
	tst_res(TINFO, "%s from %s is %s", orig, PATH_ZRAM_MM_STAT, orig_val);
	tst_res(TINFO, "%s from %s is %s", compr, PATH_ZRAM_MM_STAT, compr_val);
	tst_res(TINFO, "%s from %s is %s", mem, PATH_ZRAM_MM_STAT, mem_val);
	tst_res(TINFO, "%s from %s is %s", zero, PATH_ZRAM_MM_STAT, zero_val);
}

static void dump_info(void)
{
	print("initstate");
	print("disksize");
	if (!access(OBSOLETE_ZRAM_FILE, F_OK)) {
		print("orig_data_size");
		print("compr_data_size");
		print("mem_used_total");
		print("zero_pages");
		print("num_reads");
		print("num_writes");
	} else {
		print_mm_stat("orig_data_size", "compr_data_size",
			      "mem_used_total", "zero/same_pages");
		print_stat("num_reads", "num_writes");
	}
}

static void run(void)
{
	set_disksize();

	write_device();
	dump_info();
	verify_device();

	reset();
	dump_info();
}

static void setup(void)
{
	const char *const cmd_modprobe[] = {"modprobe", "zram", NULL};

	if (tst_cmd(cmd_rmmod, NULL, NULL, TST_CMD_PASS_RETVAL)) {
		if (errno == EBUSY)
			tst_brk(TCONF, "zram module may being used!");
	}
	if (errno == ENOENT)
		SAFE_CMD(cmd_modprobe, NULL, NULL);

	modprobe = 1;
}

static void cleanup(void)
{
	if (modprobe == 1)
		SAFE_CMD(cmd_rmmod, NULL, NULL);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.needs_root = 1,
	.needs_drivers = (const char *const []) {
		"zram",
		NULL
	},
	.needs_cmds = (const char *[]) {
		"modprobe",
		"rmmod",
		NULL
	}
};
