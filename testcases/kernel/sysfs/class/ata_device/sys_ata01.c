// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Sanity checks for the ATA device attributes exported under
 * /sys/class/ata_device/<dev>/.
 *
 * For every ATA device the test verifies that:
 *
 * - class is one of the known libata device classes (ata, atapi, pmp, semb,
 *   unknown)
 * - dma_mode, pio_mode and xfer_mode, when non-empty, start with the ``XFER_``
 *   prefix used by all libata transfer mode names
 * - spdn_cnt (speed-down count) is non-negative
 *
 * dma_mode is legitimately empty for devices that only support PIO, so it is
 * only checked when present.
 *
 * The test skips with TCONF when no ATA device is present.
 */

#include <string.h>
#include <dirent.h>
#include "tst_test.h"
#include "tst_sysfs_assert.h"

#define ATA_DEVICE "/sys/class/ata_device"

static const char *const class_allowed[] = {
	"ata", "atapi", "pmp", "semb", "unknown", NULL
};

static void check_mode(const char *dev, const char *attr)
{
	char mode[32];

	if (!TST_SYSFS_READ_STR(mode, sizeof(mode),
				ATA_DEVICE "/%s/%s", dev, attr)) {
		tst_res(TCONF, "%s/%s does not exist", dev, attr);
		return;
	}

	if (mode[0] == '\0') {
		tst_res(TCONF, "%s/%s is empty", dev, attr);
		return;
	}

	if (!strncmp(mode, "XFER_", 5))
		tst_res(TPASS, "%s/%s = '%s'", dev, attr, mode);
	else
		tst_res(TFAIL, "%s/%s = '%s' has no XFER_ prefix",
			dev, attr, mode);
}

static void check_device(const char *dev)
{
	tst_res(TINFO, "Checking ata device '%s'", dev);

	TST_SYSFS_ASSERT_ONEOF(class_allowed, ATA_DEVICE "/%s/class", dev);
	check_mode(dev, "dma_mode");
	check_mode(dev, "pio_mode");
	check_mode(dev, "xfer_mode");

	TST_SYSFS_ASSERT_RANGELL(0, LONG_MAX, ATA_DEVICE "/%s/spdn_cnt", dev);
}

static void do_test(void)
{
	DIR *d;
	struct dirent *ent;
	int found = 0;

	if (!tst_sysfs_exists(ATA_DEVICE))
		tst_brk(TCONF, ATA_DEVICE ": not present");

	d = SAFE_OPENDIR(ATA_DEVICE);

	while ((ent = SAFE_READDIR(d))) {
		if (ent->d_name[0] == '.')
			continue;

		found = 1;
		check_device(ent->d_name);
	}

	SAFE_CLOSEDIR(d);

	if (!found)
		tst_res(TCONF, "No ATA device found");
}

static struct tst_test test = {
	.test_all = do_test,
};
