// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Walks all hardware monitoring devices under /sys/class/hwmon/ and performs
 * sanity and cross-checks on the sensor channels exported per the stable
 * hwmon sysfs ABI (Documentation/hwmon/sysfs-interface.rst).
 *
 * For every discovered channel it verifies that:
 *
 * - tempN_input is within a plausible range (-55000 to 200000 millidegree
 *   Celsius, i.e. within the extended-industrial silicon sensor rating floor
 *   and below a generously high bound)
 * - tempN_min <= tempN_max <= tempN_crit and tempN_lcrit <= tempN_min, when
 *   the respective pair of attributes is present
 * - tempN_alarm, tempN_crit_alarm, tempN_min_alarm, tempN_max_alarm and
 *   tempN_fault are booleans, when present
 * - fanN_input is within a plausible range (0 to 100000 RPM, comfortably
 *   above even exotic small high-speed blower fans)
 * - fanN_min <= fanN_max, when present
 * - fanN_alarm, fanN_fault, fanN_min_alarm and fanN_max_alarm are booleans,
 *   when present
 * - inN_input is within a plausible range (-100000 to 100000 millivolt,
 *   allowing for negative supply rails, not just positive ones)
 * - inN_min <= inN_max <= inN_crit and inN_lcrit <= inN_min, when present
 *   (inN channels are numbered from 0, unlike the other channel types which
 *   start at 1)
 * - inN_alarm and related alarm attributes are booleans, when present
 * - currN_input is within a plausible range (-500000 to 500000 milliampere,
 *   allowing for negative values on bidirectional/battery charge-discharge
 *   current sensors)
 * - currN_min <= currN_max, when present
 * - powerN_input is within a plausible range (0 to 2000000000 microwatt,
 *   i.e. 2 kW, chosen to stay within LONG_MAX on 32bit architectures as well)
 * - pwmN is in the documented range [0, 255]
 * - update_interval, when present, is within a plausible range (1 to
 *   3600000 millisecond, i.e. up to an hour)
 *
 * All checks skip silently when a particular channel or attribute is not
 * present, since the set of exposed sensors varies wildly between chips and
 * platforms and up to MAX_CHANNELS indices are probed for every channel type
 * regardless of how many are actually implemented by a given chip.
 *
 * Note: the ordering assumptions between minimum/maximum/critical thresholds
 * reflect typical sensor configuration conventions, they are not enforced by
 * the kernel itself, so in principle a chip could be configured in an
 * unusual way that still is not actually a bug.
 */

#include <string.h>
#include <dirent.h>

#include "tst_test.h"
#include "tst_sysfs_assert.h"

#define HWMON "/sys/class/hwmon"
#define MAX_CHANNELS 32

static void check_temp_channel(const char *hwmon, int idx)
{
	TST_SYSFS_ASSERT_RANGELL_SILENT(-55000, 200000,
				      HWMON "/%s/temp%d_input", hwmon, idx);

	TST_SYSFS_ASSERT_LE_SUFFIX_SILENT("lcrit", "min",
					  HWMON "/%s/temp%d_", hwmon, idx);
	TST_SYSFS_ASSERT_LE_SUFFIX_SILENT("min", "max",
					  HWMON "/%s/temp%d_", hwmon, idx);
	TST_SYSFS_ASSERT_LE_SUFFIX_SILENT("max", "crit",
					  HWMON "/%s/temp%d_", hwmon, idx);

	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/temp%d_alarm", hwmon, idx);
	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/temp%d_crit_alarm", hwmon, idx);
	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/temp%d_min_alarm", hwmon, idx);
	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/temp%d_max_alarm", hwmon, idx);
	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/temp%d_fault", hwmon, idx);
}

static void check_fan_channel(const char *hwmon, int idx)
{
	TST_SYSFS_ASSERT_RANGELL_SILENT(0, 100000,
				      HWMON "/%s/fan%d_input", hwmon, idx);

	TST_SYSFS_ASSERT_LE_SUFFIX_SILENT("min", "max",
					  HWMON "/%s/fan%d_", hwmon, idx);

	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/fan%d_alarm", hwmon, idx);
	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/fan%d_min_alarm", hwmon, idx);
	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/fan%d_max_alarm", hwmon, idx);
	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/fan%d_fault", hwmon, idx);
}

static void check_in_channel(const char *hwmon, int idx)
{
	TST_SYSFS_ASSERT_RANGELL_SILENT(-100000, 100000,
				      HWMON "/%s/in%d_input", hwmon, idx);

	TST_SYSFS_ASSERT_LE_SUFFIX_SILENT("lcrit", "min",
					  HWMON "/%s/in%d_", hwmon, idx);
	TST_SYSFS_ASSERT_LE_SUFFIX_SILENT("min", "max",
					  HWMON "/%s/in%d_", hwmon, idx);
	TST_SYSFS_ASSERT_LE_SUFFIX_SILENT("max", "crit",
					  HWMON "/%s/in%d_", hwmon, idx);

	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/in%d_alarm", hwmon, idx);
	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/in%d_min_alarm", hwmon, idx);
	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/in%d_max_alarm", hwmon, idx);
	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/in%d_crit_alarm", hwmon, idx);
	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/in%d_lcrit_alarm", hwmon, idx);
}

static void check_curr_channel(const char *hwmon, int idx)
{
	/*
	 * Unlike fan/power, current can be negative on bidirectional sensors,
	 * e.g. a battery charge/discharge current monitor.
	 */
	TST_SYSFS_ASSERT_RANGELL_SILENT(-500000, 500000,
				      HWMON "/%s/curr%d_input", hwmon, idx);

	TST_SYSFS_ASSERT_LE_SUFFIX_SILENT("lcrit", "min",
					  HWMON "/%s/curr%d_", hwmon, idx);
	TST_SYSFS_ASSERT_LE_SUFFIX_SILENT("min", "max",
					  HWMON "/%s/curr%d_", hwmon, idx);
	TST_SYSFS_ASSERT_LE_SUFFIX_SILENT("max", "crit",
					  HWMON "/%s/curr%d_", hwmon, idx);

	TST_SYSFS_ASSERT_BOOL_SILENT(HWMON "/%s/curr%d_alarm", hwmon, idx);
}

static void check_power_channel(const char *hwmon, int idx)
{
	/*
	 * 2000000000 uW (2 kW) comfortably covers any single-component power
	 * sensor (CPU, GPU, PSU rail, ...) while still fitting within
	 * LONG_MAX on 32bit architectures.
	 */
	TST_SYSFS_ASSERT_RANGELL_SILENT(0, 2000000000,
				      HWMON "/%s/power%d_input", hwmon, idx);
}

static void check_pwm_channel(const char *hwmon, int idx)
{
	TST_SYSFS_ASSERT_RANGELL_SILENT(0, 255, HWMON "/%s/pwm%d", hwmon, idx);
}

static void check_hwmon_device(const char *hwmon)
{
	char name[64];
	int i;

	if (TST_SYSFS_READ_STR(name, sizeof(name), HWMON "/%s/name", hwmon))
		tst_res(TINFO, "%s: name = '%s'", hwmon, name);

	TST_SYSFS_ASSERT_RANGELL_SILENT(1, 3600000,
				      HWMON "/%s/update_interval", hwmon);

	/* inN channels are numbered from 0, per the hwmon ABI */
	for (i = 0; i <= MAX_CHANNELS; i++)
		check_in_channel(hwmon, i);

	for (i = 1; i <= MAX_CHANNELS; i++) {
		check_temp_channel(hwmon, i);
		check_fan_channel(hwmon, i);
		check_curr_channel(hwmon, i);
		check_power_channel(hwmon, i);
		check_pwm_channel(hwmon, i);
	}
}

static void do_test(void)
{
	DIR *d;
	struct dirent *ent;
	int found = 0;

	d = SAFE_OPENDIR(HWMON);

	while ((ent = SAFE_READDIR(d))) {
		if (strncmp(ent->d_name, "hwmon", 5))
			continue;

		found = 1;
		check_hwmon_device(ent->d_name);
	}

	SAFE_CLOSEDIR(d);

	if (!found)
		tst_res(TCONF, "No hwmon device found");
}

static struct tst_test test = {
	.test_all = do_test,
};
