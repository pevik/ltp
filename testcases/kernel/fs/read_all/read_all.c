// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2017-2022 Richard Palethorpe <rpalethorpe@suse.com>
 */
/*
 * Perform a small read on every file in a directory tree.
 *
 * Useful for testing file systems like proc, sysfs and debugfs or
 * anything which exposes a file like API. This test is not concerned
 * if a particular file in one of these file systems conforms exactly
 * to its specific documented behavior. Just whether reading from that
 * file causes a serious error such as a NULL pointer dereference.
 *
 * It is not required to run this as root, but test coverage will be much
 * higher with full privileges.
 *
 * The reads are preformed by worker processes which are given file paths by a
 * single parent process. The parent process recursively scans a given
 * directory and passes the file paths it finds to the child processes using a
 * queue structure stored in shared memory.
 *
 * This allows the file system and individual files to be accessed in
 * parallel. Passing the 'reads' parameter (-r) will encourage this. The
 * number of worker processes is based on the number of available
 * processors. However this is limited by default to 15 to avoid this becoming
 * an IPC stress test on systems with large numbers of weak cores. This can be
 * overridden with the 'w' parameters.
 */
#include <signal.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fnmatch.h>
#include <lapi/fnmatch.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>

#include "tst_atomic.h"
#include "tst_safe_clocks.h"
#include "tst_test.h"
#include "tst_timer.h"
#include "tst_worker.h"

#define BUFFER_SIZE 1024
#define MAX_PATH 4096
#define MAX_DISPLAY 40

enum dent_action {
	DA_UNKNOWN,
	DA_IGNORE,
	DA_READ,
	DA_VISIT,
};

struct path_worker {
	char cur[MAX_PATH];
	char next[MAX_PATH];
};

static char *verbose;
static char *quiet;
static char *root_dir;
static char *str_reads;
static int reads = 1;
static char *str_worker_count;
static char *str_max_workers;
static long max_workers = 15;
static char *drop_privs;
static char *str_worker_timeout;
static int timeout_warnings_left = 15;

static char *blacklist[] = {
	NULL, /* reserved for -e parameter */
	"/sys/kernel/debug/*",
	"/sys/devices/platform/*/eeprom",
	"/sys/devices/platform/*/nvmem",
	"/sys/*/cpu??*(?)/*",	/* cpu* entries with 2 or more digits */
};

static struct tst_workers workers;
static struct path_worker *worker_privs;
struct tst_evloop evloop;

static void sanitize_str(char *buf, ssize_t count)
{
	int i;

	for (i = 0; i < MIN(count, (ssize_t)MAX_DISPLAY); i++)
		if (!isprint(buf[i]))
			buf[i] = ' ';

	if (count <= MAX_DISPLAY)
		buf[count] = '\0';
	else
		strcpy(buf + MAX_DISPLAY, "...");
}

static int is_blacklisted(const char *path)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(blacklist); i++) {
		if (blacklist[i] && !fnmatch(blacklist[i], path, FNM_EXTMATCH)) {
			if (verbose)
				tst_res(TINFO, "Ignoring %s", path);
			return 1;
		}
	}

	return 0;
}

static void read_test(struct tst_worker *self, char *path)
{
	char buf[BUFFER_SIZE];
	int fd;
	ssize_t count;
	long long elapsed;

	if (is_blacklisted(path))
		return;

	if (verbose)
		tst_res(TINFO, "%s: %s(%s)", tst_worker_idstr(self), __func__, path);

	fd = open(path, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		if (!quiet) {
			tst_res(TINFO | TERRNO, "%s: open(%s)",
			        tst_worker_idstr(self), path);
		}
		return;
	}

	elapsed = tst_chan_elapsed(&self->chan);
	count = read(fd, buf, sizeof(buf) - 1);
	elapsed = tst_chan_elapsed(&self->chan) - elapsed;

	if (count > 0 && verbose) {
		sanitize_str(buf, count);
		tst_res(TINFO,
			"%s: read(%s, buf) = %zi, buf = %s, elapsed = %llus",
			tst_worker_idstr(self), path, count, buf, elapsed);
	} else if (!count && verbose) {
		tst_res(TINFO,
			"%s: read(%s) = EOF, elapsed = %llus",
			tst_worker_idstr(self), path, elapsed);
	} else if (count < 0 && !quiet) {
		tst_res(TINFO | TERRNO,
			"%s: read(%s), elapsed = %llus",
			tst_worker_idstr(self), path, elapsed);
	}

	SAFE_CLOSE(fd);
}

static void maybe_drop_privs(void)
{
	struct passwd *nobody;

	if (!drop_privs)
		return;

	TEST(setgroups(0, NULL));
	if (TST_RET < 0 && TST_ERR != EPERM) {
		tst_brk(TBROK | TTERRNO,
			"Failed to clear suplementary group set");
	}

	nobody = SAFE_GETPWNAM("nobody");

	TEST(setgid(nobody->pw_gid));
	if (TST_RET < 0 && TST_ERR != EPERM)
		tst_brk(TBROK | TTERRNO, "Failed to use nobody gid");

	TEST(setuid(nobody->pw_uid));
	if (TST_RET < 0 && TST_ERR != EPERM)
		tst_brk(TBROK | TTERRNO, "Failed to use nobody uid");
}

static void visit_dir(struct tst_worker *self, const char *path)
{
	DIR *dir;
	struct dirent *dent;
	struct stat dent_st;
	char dent_path[MAX_PATH];
	enum dent_action act;

	dir = opendir(path);
	if (!dir) {
		tst_res(TINFO | TERRNO, "opendir(%s)", path);
		return;
	}

	while (1) {
		errno = 0;
		dent = readdir(dir);
		if (!dent && errno) {
			tst_res(TINFO | TERRNO, "readdir(%s)", path);
			break;
		} else if (!dent) {
			break;
		}

		if (!strcmp(dent->d_name, ".") ||
		    !strcmp(dent->d_name, ".."))
			continue;

		if (dent->d_type == DT_DIR)
			act = DA_VISIT;
		else if (dent->d_type == DT_LNK)
			act = DA_IGNORE;
		else if (dent->d_type == DT_UNKNOWN)
			act = DA_UNKNOWN;
		else
			act = DA_READ;

		snprintf(dent_path, MAX_PATH,
			 "%s/%s", path, dent->d_name);

		if (act == DA_UNKNOWN) {
			if (lstat(dent_path, &dent_st))
				tst_res(TINFO | TERRNO, "lstat(%s)", path);
			else if ((dent_st.st_mode & S_IFMT) == S_IFDIR)
				act = DA_VISIT;
			else if ((dent_st.st_mode & S_IFMT) == S_IFLNK)
				act = DA_IGNORE;
			else
				act = DA_READ;
		}

		if (act == DA_VISIT)
			visit_dir(self, dent_path);
		else if (act == DA_READ)
			tst_chan_send(&self->chan, dent_path, strlen(dent_path) + 1);
	}

	if (closedir(dir))
		tst_res(TINFO | TERRNO, "closedir(%s)", path);
}

static int dir_worker_run(struct tst_worker *self)
{
	visit_dir(self, root_dir);

	tst_res(TINFO, "Dir Worker %d (%d): fin.", self->pid, self->i);
	tst_chan_send(&self->chan, "", 1);

	return 0;
}

static int path_worker_run(struct tst_worker *self)
{
	char buf[BUFFER_SIZE];
	struct sigaction term_sa = {
		.sa_handler = SIG_IGN,
		.sa_flags = 0,
	};

	sigaction(SIGTTIN, &term_sa, NULL);
	maybe_drop_privs();

	while (1) {
		tst_chan_recv(&self->chan, buf, PATH_MAX);

		if (!buf[0])
			break;

		read_test(self, buf);
	}

	tst_flush();
	return 0;
}

static void path_worker_resend(struct tst_worker *const self)
{
	struct path_worker *pw = self->priv;

	tst_chan_send(&self->chan, pw->next, strlen(pw->next));
}

static void do_next_path(struct tst_worker *path_worker)
{
	size_t slen = 1;
	struct tst_worker *const dir_worker = workers.vec;
	struct path_worker *pw = path_worker->priv;

	pw->next[0] = '\0';

	if (TST_STATE_GET(&dir_worker->mach, TST_STATE_ANY) != WS_RUNNING)
		goto send;

	tst_chan_recv(&dir_worker->chan, pw->next, BUFFER_SIZE);

	if (!pw->next[0])
		TST_STATE_SET(&dir_worker->mach, WS_STOPPING);

	slen = dir_worker->chan.in.len;
send:
	tst_chan_send(&path_worker->chan, pw->next, slen);

}

static void path_worker_sent(struct tst_worker *self, char *path, size_t len)
{
	struct path_worker *pw = self->priv;

	memcpy(pw->cur, path, len);

	if (!path[0])
		return;

	do_next_path(self);
}

static int check_timeout_warnings_limit(void)
{
	if (!quiet)
		return 1;

	timeout_warnings_left--;

	if (timeout_warnings_left)
		return 1;

	tst_res(TINFO,
		"Silencing timeout warnings; consider increasing LTP_RUNTIME_MUL or removing -q");

	return 0;
}

static void path_worker_died(struct tst_worker *self)
{
	struct path_worker *pw = self->priv;

	if (pw->cur[0] == '\0') {
		tst_brk(TBROK,
			"%s: Died, but doesn't appear to be reading anything",
			tst_worker_idstr(self));
	}

	if (check_timeout_warnings_limit())
		tst_res(TINFO, "%s: Died; Last sent '%s'", tst_worker_idstr(self), pw->cur);

	tst_worker_start(self);
	path_worker_resend(self);
}

static void spawn_workers(void)
{
	int i;
	long wcount = workers.count;
	struct tst_worker *wa = workers.vec;

	wa[0].name = "Dir";
	wa[0].run = dir_worker_run;
	wa[0].mode = TST_WORKER_SYNC;

	tst_worker_start(wa);

	for (i = 1; i < wcount; i++) {
		wa[i].name = "Path";
		wa[i].run = path_worker_run;
		wa[i].on_sent = path_worker_sent;
		wa[i].on_died = path_worker_died;
		wa[i].mode = TST_WORKER_ASYNC;
		wa[i].priv = worker_privs + i;

		tst_worker_start(wa + i);
		do_next_path(wa + i);
	}
}

static void setup(void)
{
	if (tst_parse_int(str_reads, &reads, 1, INT_MAX))
		tst_brk(TBROK,
			"Invalid reads (-r) argument: '%s'", str_reads);

	if (tst_parse_long(str_max_workers, &max_workers, 2, LONG_MAX)) {
		tst_brk(TBROK,
			"Invalid max workers (-w) argument: '%s'",
			str_max_workers);
	}

	if (tst_parse_long(str_worker_count, &workers.count, 2, LONG_MAX)) {
		tst_brk(TBROK,
			"Invalid worker count (-W) argument: '%s'",
			str_worker_count);
	}

	if (!root_dir)
		tst_brk(TBROK, "The directory argument (-d) is required");

	if (!workers.count)
		workers.count = MIN(MAX(tst_ncpus() - 1, 2L), max_workers);

	workers.vec = SAFE_MALLOC(workers.count * sizeof(workers.vec[0]));
	memset(workers.vec, 0, workers.count * sizeof(workers.vec[0]));
	worker_privs = SAFE_MALLOC(workers.count * sizeof(struct path_worker));

	if (tst_parse_int(str_worker_timeout, (int *)&workers.timeout, 1, INT_MAX)) {
		tst_brk(TBROK,
			"Invalid worker timeout (-t) argument: '%s'",
			str_worker_count);
	}

	if (workers.timeout) {
		tst_res(TINFO, "Worker timeout forcibly set to %lldms",
			workers.timeout);
	} else {
		workers.timeout = 10 * tst_remaining_runtime();
		tst_res(TINFO, "Worker timeout set to 10%% of max_runtime: %lldms",
			workers.timeout);
	}
	workers.timeout *= 1000;

	tst_workers_setup(&workers);
}

static void cleanup(void)
{
	tst_workers_cleanup(&workers);
}

static void run(void)
{
	spawn_workers();
	tst_evloop_run(&workers.evloop);

	tst_res(TPASS, "Finished reading files");
}

static struct tst_test test = {
	.options = (struct tst_option[]) {
		{"v", &verbose,
		 "Print information about successful reads."},
		{"q", &quiet,
		 "Don't print file read or open errors."},
		{"d:", &root_dir,
		 "Path to the directory to read from, defaults to /sys."},
		{"e:", &blacklist[0],
		 "Pattern Ignore files which match an 'extended' pattern, see fnmatch(3)."},
		{"r:", &str_reads,
		 "Count The number of times to schedule a file for reading."},
		{"w:", &str_max_workers,
		 "Count Set the worker count limit, the default is 15."},
		{"W:", &str_worker_count,
		 "Count Override the worker count. Ignores (-w) and the processor count."},
		{"p", &drop_privs,
		 "Drop privileges; switch to the nobody user."},
		{"t:", &str_worker_timeout,
		 "Milliseconds a worker has to read a file before it is restarted"},
		{}
	},
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.forks_child = 1,
	.max_runtime = 100,
};
