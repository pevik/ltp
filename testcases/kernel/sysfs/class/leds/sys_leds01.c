// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2026 Cyril Hrubis <chrubis@suse.cz>
 */

/*\
 * Sanity checks for the LED class devices exported under
 * /sys/class/leds/<led>/.
 *
 * For every LED the test verifies that:
 *
 * - max_brightness is greater than zero
 * - brightness is in the range [0, max_brightness]
 * - trigger is a bracketed-choice file listing the available triggers with
 *   exactly one of them selected via [] (the set of triggers is not checked
 *   as it is highly kernel- and hardware-dependent)
 *
 * The test skips with TCONF when no LED device is present.
 */

#include <limits.h>
#include <dirent.h>
#include "tst_test.h"
#include "tst_sysfs_assert.h"

#define LEDS "/sys/class/leds"

static void check_led(const char *name)
{
	char trigger[64];

	tst_res(TINFO, "Checking led '%s'", name);

	TST_SYSFS_ASSERT_RANGELL(1, LONG_MAX, LEDS "/%s/max_brightness", name);

	TST_SYSFS_ASSERT_RANGELF(0, "brightness", "max_brightness",
				  LEDS "/%s/", name);

	TST_SYSFS_ASSERT_CHOICE(NULL, trigger, sizeof(trigger),
				LEDS "/%s/trigger", name);
}

static void do_test(void)
{
	DIR *d;
	struct dirent *ent;
	int found = 0;

	d = SAFE_OPENDIR(LEDS);

	while ((ent = SAFE_READDIR(d))) {
		if (ent->d_name[0] == '.')
			continue;

		found = 1;
		check_led(ent->d_name);
	}

	SAFE_CLOSEDIR(d);

	if (!found)
		tst_res(TCONF, "No LED device found");
}

static struct tst_test test = {
	.test_all = do_test,
};
