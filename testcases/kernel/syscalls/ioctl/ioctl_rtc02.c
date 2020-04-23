// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Filip Bozuta Filip.Bozuta@rt-rk.com
 */

/*
 * Test RTC ioctls with RTC_ALM_READ and RTC_ALM_SET requests
 *
 * Reads the current alarm time from RTC device using
 * RTC_ALM_READ request and displays the alarm time
 * information as follows: hour:minute:second
 *
 * Sets a new alarm time in RTC device using RTC_ALM_SET
 * request and displays the new alarm time information
 * as follows: hour:minute:second
 *
 * Reads the new alarm time from RTC device using RTC_ALM_READ
 * request and checks whether the read alarm time information
 * is same as the one set by RTC_ALM_SET
 *
 * Runs RTC_ALM_SET to set back the current alarm time read by
 * RTC_ALM_READ at the beginning of the test
 */

#include <stdint.h>
#include <errno.h>
#include <linux/rtc.h>
#include "tst_test.h"

static void setup(void)
{
	int exists = ("/dev/rtc", O_RDONLY);

	if (exists < 0)
		tst_brk(TCONF, "RTC device driver file not available");
}

char *read_alarm_request = "RTC_ALM_READ";
char *set_alarm_request = "RTC_ALM_SET";

static void run(void)
{
	int fd;

	struct rtc_time rtc_read_alarm;
	struct rtc_time rtc_cur_alarm;
	struct rtc_time rtc_set_alarm = {
		.tm_sec = 13, .tm_min = 35, .tm_hour = 12};

	int alarm_read_supported, alarm_set_supported = 0;

	fd = SAFE_OPEN("/dev/rtc", O_RDONLY);

	if (fd == -1)
		tst_brk(TCONF, "RTC device driver file could not be opened");

	if (ioctl(fd, RTC_ALM_READ, &rtc_cur_alarm) == -1) {
		if (errno == ENOTTY)
			tst_res(TCONF, "ioctl %s not supported on RTC device",
				read_alarm_request);
		else
			tst_res(TFAIL | TERRNO, "unexpected ioctl error");
	} else {
		tst_res(TPASS, "alarm time successfully read from RTC device");
		tst_res(TINFO, "current RTC alarm time: %d:%d:%d",
			rtc_cur_alarm.tm_hour, rtc_cur_alarm.tm_min,
			rtc_cur_alarm.tm_sec);
		alarm_read_supported = 1;
	}

	if (ioctl(fd, RTC_ALM_SET, &rtc_set_alarm) == -1) {
		if (errno == ENOTTY)
			tst_res(TCONF, "ioctl %s not supported on RTC device",
				set_alarm_request);
		else
			tst_res(TFAIL | TERRNO, "unexpected ioctl error");
	} else {
		tst_res(TPASS, "alarm time successfully set to RTC device");
		tst_res(TINFO, "new RTC alarm time: %d:%d:%d",
			rtc_set_alarm.tm_hour, rtc_set_alarm.tm_min,
			rtc_set_alarm.tm_sec);
		alarm_set_supported = 1;
	}

	if (alarm_read_supported && alarm_set_supported) {
		ioctl(fd, RTC_ALM_READ, &rtc_read_alarm);

		char alarm_data[][10] = {"second", "minute", "hour"};
		int read_alarm_data[] = {
			rtc_read_alarm.tm_sec, rtc_read_alarm.tm_min,
			rtc_read_alarm.tm_hour};
		int set_alarm_data[] = {
			rtc_set_alarm.tm_sec, rtc_set_alarm.tm_min,
			rtc_set_alarm.tm_hour};
		for (int i = 0; i < 3; i++)
			if (read_alarm_data[i] == set_alarm_data[i])
				tst_res(TPASS, "%s reads new %s as expected",
					read_alarm_request, alarm_data[i]);
			else
				tst_res(TPASS, "%s reads different %s than set",
					read_alarm_request, alarm_data[i]);
	}

	if (alarm_set_supported)
		ioctl(fd, RTC_ALM_SET, &rtc_cur_alarm);

	SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.test_all = run,
	.needs_root = 1,
	.needs_device = 1,
	.setup = setup,
};
