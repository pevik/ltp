// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Filip Bozuta Filip.Bozuta@rt-rk.com
 */

/*
 * Test RTC ioctls with requests RTC_AIE_ON, RTC_AIE_OFF,
 * RTC_PIE_ON, RTC_PIE_OFF, RTC_UIE_ON, RTC_UIE_OFF,
 * RTC_WIE_ON, RTC_WIE_OFF
 *
 * Runs ioctls with the above mentioned requests one by one
 * and sequentially turns on and off RTC alarm, periodic,
 * update and watchdog interrupt.
 */

#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <linux/rtc.h>
#include "tst_test.h"

static void setup(void)
{
	int exists = ("/dev/rtc", O_RDONLY);

	if (exists < 0)
		tst_brk(TCONF, "RTC device driver file not available");
}

static char interrupts[][10] = {"alarm", "periodic", "update", "watchdog"};
static int interrupt_requests[] = {
	RTC_AIE_ON, RTC_PIE_ON, RTC_UIE_ON, RTC_WIE_ON,
	RTC_AIE_OFF, RTC_PIE_OFF, RTC_UIE_OFF, RTC_WIE_OFF};
static char requests_text[][15] = {
	"RTC_AIE_ON", "RTC_PIE_ON", "RTC_UIE_ON", "RTC_WIE_ON",
	"RTC_AIE_OFF", "RTC_PIE_OFF", "RTC_UIE_OFF", "RTC_WIE_OFF"};

static void test_request(unsigned int n)
{
	int fd;

	int on_request = interrupt_requests[n];
	int off_request = interrupt_requests[n + 4];

	char on_request_text[15], off_request_text[15];

	strcpy(on_request_text, requests_text[n]);
	strcpy(off_request_text, requests_text[n + 4]);

	fd = SAFE_OPEN("/dev/rtc", O_RDWR);

	if (fd == -1)
		tst_brk(TCONF, "RTC device driver file could not be opened");

	if (ioctl(fd, on_request) == -1) {
		if (errno == ENOTTY) {
			tst_res(TCONF, "ioctl %s not supported on RTC device",
				on_request_text);
		} else {
			tst_res(TFAIL, "unexpected ioctl error");
		}
	} else {
		tst_res(TPASS, "%s interrupt enabled", interrupts[n]);
	}

	if (ioctl(fd, off_request) == -1) {
		if (errno == ENOTTY) {
			tst_res(TCONF, "ioctl %s not supported on RTC device",
				off_request_text);
		} else {
			tst_res(TFAIL, "unexpected ioctl error");
		}
	} else {
		tst_res(TPASS, "%s interrupt disabled", interrupts[n]);
	}

	SAFE_CLOSE(fd);
}

static struct tst_test test = {
	.tcnt = ARRAY_SIZE(interrupts),
	.test = test_request,
	.needs_root = 1,
	.needs_device = 1,
	.setup = setup,
};
