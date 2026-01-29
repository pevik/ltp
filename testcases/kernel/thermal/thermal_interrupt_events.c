// SPDX-License-Identifier: GPL-2.0-or-later

/*
 * Copyright (C) 2025-2026 Intel - http://www.intel.com/
 */

/*\
 * Tests the CPU package thermal sensor interface for Intel platforms.

 * Works by checking the initial count of thermal interrupts. Then it
 * decreases the threshold for sending a thermal interrupt to just above
 * the current temperature and runs a workload on the CPU. Finally, it restores
 * the original thermal threshold and checks whether the number of thermal
 * interrupts increased.
 */

#include "tst_safe_stdio.h"
#include "tst_test.h"

#define	RUNTIME		30
#define	SLEEP		10
#define	TEMP_INCREMENT	10

static bool x86_pkg_temp_tz_found;
static bool *x86_pkg_temp_tz;
static char trip_path[NAME_MAX];
static int nproc, trip, tz_counter;
static uint64_t *interrupt_init, *interrupt_later;

static void interrupts(uint64_t *interrupt_array, const int nproc)
{
	bool interrupts_found = false;
	char line[8192];

	memset(interrupt_array, 0, nproc * sizeof(*interrupt_array));
	FILE *fp = SAFE_FOPEN("/proc/interrupts", "r");

	while (fgets(line, sizeof(line), fp)) {
		if (strstr(line, "Thermal event interrupts")) {
			interrupts_found = true;
			char *token = strtok(line, " ");

			token = strtok(NULL, " ");
			int i = 0;

			while (!!strncmp(token, "Thermal", 7)) {
				interrupt_array[i++] = atoll(token);
				token = strtok(NULL, " ");
				tst_res(TDEBUG, "interrupts[%d]: %ld", i - 1, interrupt_array[i - 1]);
			}
			break;
		}
	}
	SAFE_FCLOSE(fp);
	if (!interrupts_found)
		tst_brk(TCONF, "No Thermal event interrupts line in /proc/interrupts");
}

static void setup(void)
{
	char line[8192];

	nproc = tst_ncpus();
	tst_res(TDEBUG, "Number of logical cores: %d", nproc);
	interrupt_init = calloc(nproc, sizeof(uint64_t));
	interrupt_later = calloc(nproc, sizeof(uint64_t));

	DIR *dir = SAFE_OPENDIR("/sys/class/thermal/");
	struct dirent *entry;

	tz_counter = 0;

	while ((entry = SAFE_READDIR(dir))) {
		if ((strncmp(entry->d_name, "thermal_zone", sizeof("thermal_zone"))) > 0)
			tz_counter++;
	}
	SAFE_CLOSEDIR(dir);
	tst_res(TDEBUG, "Found %d thermal zone(s)", tz_counter);

	interrupts(interrupt_init, nproc);

	x86_pkg_temp_tz = calloc(tz_counter, sizeof(bool));

	for (int i = 0; i < tz_counter; i++) {
		char path[NAME_MAX];

		snprintf(path, NAME_MAX, "/sys/class/thermal/thermal_zone%d/type", i);
		tst_res(TDEBUG, "Checking whether %s is x86_pkg_temp", path);

		SAFE_FILE_SCANF(path, "%s", line);
		if (strstr(line, "x86_pkg_temp")) {
			tst_res(TDEBUG, "Thermal zone %d uses x86_pkg_temp", i);
			x86_pkg_temp_tz[i] = 1;
			x86_pkg_temp_tz_found = 1;
		}
	}

	if (!x86_pkg_temp_tz_found)
		tst_brk(TCONF, "No thermal zone uses x86_pkg_temp");
}

static void *cpu_workload(double run_time)
{
	time_t start_time = time(NULL);
	int num = 2;

	while (difftime(time(NULL), start_time) < run_time) {
		for (int i = 2; i * i <= num; i++) {
			if (num % i == 0)
				break;
		}
		num++;
	}
	return NULL;
}

static void test_zone(int i)
{
			char path[NAME_MAX], temp_path[NAME_MAX];
			int sleep_time = SLEEP, temp_high, temp;
			double run_time = RUNTIME;

			snprintf(path, NAME_MAX, "/sys/class/thermal/thermal_zone%d/", i);
			strncpy(temp_path, path, NAME_MAX);
			strncat(temp_path, "temp", 4);
			tst_res(TINFO, "Testing %s", temp_path);
			SAFE_FILE_SCANF(temp_path, "%d", &temp);
			if (temp < 0)
				tst_brk(TINFO, "Unexpected zone temperature value %d", temp);
			tst_res(TDEBUG, "Current temperature for %s: %d", path, temp);

			temp_high = temp + TEMP_INCREMENT;

			strncpy(trip_path, path, NAME_MAX);
			strncat(trip_path, "trip_point_1_temp", 17);

			tst_res(TDEBUG, "Setting new trip_point_1_temp value: %d", temp_high);
			SAFE_FILE_SCANF(trip_path, "%d", &trip);
			SAFE_FILE_PRINTF(trip_path, "%d", temp_high);

			while (sleep_time > 0) {
				tst_res(TDEBUG, "Running for %f seconds, then sleeping for %d seconds", run_time, sleep_time);

				for (int j = 0; j < nproc; j++) {
					if (!SAFE_FORK()) {
						cpu_workload(run_time);
						exit(0);
					}
				}

				tst_reap_children();

				SAFE_FILE_SCANF(temp_path, "%d", &temp);
				tst_res(TDEBUG, "Temperature for %s after a test: %d", path, temp);

				if (temp > temp_high)
					break;
				sleep(sleep_time--);
				run_time -= 3;
			}

			if (temp <= temp_high)
				tst_brk(TCONF, "Zone temperature is not rising as expected");
}

static void cleanup(void)
{
	SAFE_FILE_PRINTF(trip_path, "%d", trip);
	free(interrupt_init);
	free(interrupt_later);
}

static void run(void)
{
	for (int i = 0; i < tz_counter; i++) {
		if (x86_pkg_temp_tz[i])
			test_zone(i);
	}
	interrupts(interrupt_later, nproc);

	for (int i = 0; i < nproc; i++) {
		if (interrupt_later[i] < interrupt_init[i])
			tst_res(TFAIL, "CPU %d interrupt counter: %ld (previous: %ld)",
				i, interrupt_later[i], interrupt_init[i]);
	}

	tst_res(TPASS, "x86 package thermal interrupt triggered");
}

static struct tst_test test = {
	.cleanup = cleanup,
	.forks_child = 1,
	.min_runtime = 180,
	.needs_root = 1,
	.setup = setup,
	.supported_archs = (const char *const []) {
		"x86",
		"x86_64",
		NULL
	},
	.test_all = run
};
