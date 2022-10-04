/* IBM Corporation */
/* 01/02/2003	Port to LTP avenkat@us.ibm.com */
/* 06/30/2001	Port to Linux	nsharoff@us.ibm.com */
/*
 *   Copyright (c) International Business Machines  Corp., 2003
 *
 *
 *   This program is free software;  you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *   the GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program;  if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 *  This test stresses mmaps, without dealing with fragments or anything!
 *  It forks a specified number of children,
 *  all of whom mmap the same file, make a given number of accesses
 *  to random pages in the map (reading & writing and comparing data).
 *  Then the child exits and the parent forks another to take its place.
 *  Each time a child is forked, it stats the file and maps the full
 *  length of the file.
 *
 *  This program continues to run until it either receives a SIGINT,
 *  or times out (if a timeout value is specified).  When either of
 *  these things happens, it cleans up its kids, then checks the
 *  file to make sure it has the correct data.
 *
 *  usage:
 *	tmnoextend -p nprocs [-t minutes -f filesize -S sparseoffset
 *			      -r -o -m -l -d]
 *  where:
 *	-p nprocs	- specifies the number of mapping children
 *			  to create.  (nprocs + 1 children actually
 *			  get created, since one is the writer child)
 *	-t minutes	- specifies minutes to run.  If not specified,
 *			  default is to run forever until a SIGINT
 *			  is received.
 *	-f filesize	- initial filesize (defaults to FILESIZE)
 *	-S sparseoffset - when non-zero, causes a sparse area to
 *			  be left before the data, meaning that the
 *			  actual initial file size is sparseoffset +
 *			  filesize.  Useful for testing large files.
 *			  (default is 0).
 *	-r		- randomize number of pages map children check.
 *			  (random % MAXLOOPS).  If not specified, each
 *			  child checks MAXLOOPS pages.
 *	-o		- randomize offset of file to map. (default is 0)
 *	-m		- do random msync/fsyncs as well
 *	-l		- if set, the output file is not removed on
 *			  program exit.
 *	-d		- enable debug output
 *
 *  Compile with -DLARGE_FILE to enable file sizes > 2 GB.
 */

#define _GNU_SOURCE 1
#include <stdio.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <limits.h>
#include <float.h>
#include "tst_test.h"

#define MAXLOOPS	500	/* max pages for map children to write */
#define	FILESIZE	4096	/* initial filesize set up by parent */
#define TEST_FILE	"mmapstress01.out"

#ifdef roundup
#undef roundup
#endif
#define roundup(x, y)	((((x)+((y)-1))/(y))*(y))

static unsigned int initrand(void);
static void finish(int sig);
static void child_mapper(char *file, unsigned int procno, unsigned int nprocs);
static int fileokay(char *file, unsigned char *expbuf);
static int finished;

static char *debug;
static char *leavefile;
static char *do_sync;
static char *do_offset;
static char *randloops;
static char *opt_filesize;
static char *opt_nprocs;
static char *opt_sparseoffset;
static char *opt_alarmtime;

static float alarmtime;
static int nprocs;
static long long filesize = FILESIZE;
static long long sparseoffset;
static unsigned int pattern;

static pid_t *pidarray;
static int wait_stat;
static int check_for_sanity;
static unsigned char *buf;

static struct tst_option options[] = {
	{"d", &debug, "Enable debug output"},
	{"f:", &opt_filesize, "Initial filesize (default 4096)"},
	{"l", &leavefile, "Don't remove the output file on program exit"},
	{"m", &do_sync, "Do random msync/fsyncs as well"},
	{"o", &do_offset, "Randomize the offset of file to map"},
	{"p:", &opt_nprocs, "Number of mapping children to create (required)"},
	{"r", &randloops,
	 "Randomize number of pages map children check (random % MAXLOOPS), "
	 "otherwise each child checks MAXLOOPS pages"},
	{"S:", &opt_sparseoffset,
	 "When non-zero, causes the sparse area to be left before the data, "
	 "so that the actual initial filesize is sparseoffset + filesize "
	 "(default 0)"},
	{"t:", &opt_alarmtime, "Number of minutes to run (default 0)"},
	{},
};

static void setup(void)
{
	int pagesize = sysconf(_SC_PAGE_SIZE);

	if (!opt_nprocs)
		tst_brk(TBROK,
			"missing number of mapping children, specify with -p nprocs");

#if _FILE_OFFSET_BITS == 64
	if (tst_parse_filesize(opt_filesize, &filesize, 0, LONG_MAX))
#else
	if (tst_parse_filesize(opt_filesize, &filesize, 0, INT_MAX))
#endif
		tst_brk(TBROK, "invalid initial filesize '%s'", opt_filesize);

#if _FILE_OFFSET_BITS == 64
	if (tst_parse_filesize(opt_sparseoffset, &sparseoffset, LONG_MIN, LONG_MAX))
#else
	if (tst_parse_filesize(opt_sparseoffset, &sparseoffset, INT_MIN, INT_MAX))
#endif
		tst_brk(TBROK, "invalid sparse offset '%s'", opt_sparseoffset);
	if (sparseoffset % pagesize != 0)
		tst_brk(TBROK, "sparseoffset must be pagesize multiple");

	if (tst_parse_int(opt_nprocs, &nprocs, 0, 255))
		tst_brk(TBROK, "invalid number of mapping children '%s'",
			opt_nprocs);

	if (tst_parse_float(opt_alarmtime, &alarmtime, 0, FLT_MAX / 60))
		tst_brk(TBROK, "invalid minutes to run '%s'", opt_alarmtime);

	if (debug) {
		tst_res(TINFO, "creating file <%s> with %lld bytes, pattern %d",
			TEST_FILE, filesize, pattern);
		if (alarmtime)
			tst_res(TINFO, "running for %f minutes", alarmtime);
		else
			tst_res(TINFO, "running with no time limit");
	}
	alarmtime *= 60;
}

static void run(void)
{
	int fd;
	int c;
	int procno;
	pid_t pid;
	unsigned int seed;
	int pagesize = sysconf(_SC_PAGE_SIZE);
	struct sigaction sa;
	int i;
	int write_cnt;
	unsigned char data;
	off_t bytes_left;
	sigset_t set_mask;

	seed = initrand();
	pattern = seed & 0xff;

	/*
	 *  Plan for death by signal.  User may have specified
	 *  a time limit, in which set an alarm and catch SIGALRM.
	 *  Also catch and cleanup with SIGINT.
	 */
	sa.sa_handler = finish;
	sa.sa_flags = 0;
	SAFE_SIGEMPTYSET(&sa.sa_mask);
	SAFE_SIGACTION(SIGINT, &sa, 0);
	SAFE_SIGACTION(SIGQUIT, &sa, 0);
	SAFE_SIGACTION(SIGTERM, &sa, 0);

	if (alarmtime) {
		SAFE_SIGACTION(SIGALRM, &sa, 0);
		(void)alarm(alarmtime);
	}
	fd = SAFE_OPEN(TEST_FILE, O_CREAT | O_TRUNC | O_RDWR, 0664);

	buf = SAFE_MALLOC(pagesize);
	pidarray = SAFE_MALLOC(nprocs * sizeof(pid_t));

	for (i = 0; i < nprocs; i++)
		*(pidarray + i) = 0;

	for (i = 0, data = 0; i < pagesize; i++) {
		*(buf + i) = (data + pattern) & 0xff;
		if (++data == nprocs)
			data = 0;
	}
	SAFE_LSEEK(fd, (off_t)sparseoffset, SEEK_SET);
	for (bytes_left = filesize; bytes_left; bytes_left -= c) {
		write_cnt = MIN(pagesize, (int)bytes_left);
		c = SAFE_WRITE(1, fd, buf, write_cnt);
	}
	SAFE_CLOSE(fd);

	/*
	 *  Fork off mmap children.
	 */
	for (procno = 0; procno < nprocs; procno++) {
		switch (pid = fork()) {

		case -1:
			tst_brk(TFAIL, "fork error");
			break;

		case 0:
			child_mapper(TEST_FILE, (unsigned int)procno, (unsigned int)nprocs);
			exit(0);

		default:
			pidarray[procno] = pid;
		}
	}

	/*
	 *  Now wait for children and refork them as needed.
	 */

	SAFE_SIGEMPTYSET(&set_mask);
	SAFE_SIGADDSET(&set_mask, SIGALRM);
	SAFE_SIGADDSET(&set_mask, SIGINT);
	while (!finished) {
		pid = wait(&wait_stat);
		/*
		 *  Block signals while processing child exit.
		 */

		SAFE_SIGPROCMASK(SIG_BLOCK, &set_mask, NULL);

		if (pid != -1) {
			/*
			 *  Check exit status, then refork with the
			 *  appropriate procno.
			 */
			if (!WIFEXITED(wait_stat)
			    || WEXITSTATUS(wait_stat) != 0)
				tst_brk(TFAIL, "child exit with err <x%x>",
					wait_stat);
			for (i = 0; i < nprocs; i++)
				if (pid == pidarray[i])
					break;
			if (i == nprocs)
				tst_brk(TFAIL, "unknown child pid %d, <x%x>",
					pid, wait_stat);

			if ((pid = fork()) == -1) {
				pidarray[i] = 0;
				tst_brk(TFAIL, "fork error");
			} else if (pid == 0) {	/* child */
				child_mapper(TEST_FILE, (unsigned int)i, (unsigned int)nprocs);
				exit(0);
			} else {
				pidarray[i] = pid;
			}
		} else {
			/*
			 *  wait returned an error.  If EINTR, then
			 *  normal finish, else it's an unexpected
			 *  error...
			 */
			if (errno != EINTR || !finished)
				tst_brk(TFAIL, "unexpected wait error");
		}
		SAFE_SIGPROCMASK(SIG_UNBLOCK, &set_mask, NULL);
	}

	/*
	 *  Finished!  Check the file for sanity, then kill all
	 *  the children and done!.
	 */

	SAFE_SIGEMPTYSET(&set_mask);
	SAFE_SIGADDSET(&set_mask, SIGALRM);
	SAFE_SIGPROCMASK(SIG_BLOCK, &set_mask, NULL);
	(void)alarm(0);
	check_for_sanity = 1;
	tst_res(TPASS, "finished, cleaning up");
}

static void cleanup(void)
{
	for (int i = 0; i < nprocs; i++)
		(void)kill(pidarray[i], SIGKILL);

	while (wait(&wait_stat) != -1 || errno != ECHILD)
		continue;

	if (check_for_sanity) {		/* only check file if no errors */
		if (!fileokay(TEST_FILE, buf)) {
			tst_res(TINFO, "  leaving file <%s>", TEST_FILE);
			tst_brk(TFAIL, "file data incorrect");
		} else {
			tst_res(TINFO, "file data okay");
			if (!leavefile)
				SAFE_UNLINK(TEST_FILE);
			tst_res(TPASS, "test passed");
		}
	} else {
		tst_res(TINFO, "  leaving file <%s>", TEST_FILE);
	}
}

/*
 *  Child process that reads/writes map.  The child stats the file
 *  to determine the size, maps the size of the file, then reads/writes
 *  its own locations on random pages of the map (its locations being
 *  determined based on nprocs & procno).  After a specific number of
 *  iterations, it exits.
 */
void child_mapper(char *file, unsigned int procno, unsigned int nprocs)
{
	struct stat statbuf;
	off_t filesize;
	off_t offset;
	size_t validsize;
	size_t mapsize;
	char *maddr = NULL, *paddr;
	int fd;
	size_t pagesize = sysconf(_SC_PAGE_SIZE);
	unsigned int randpage;
	unsigned int seed;
	unsigned int loopcnt;
	unsigned int nloops;
	unsigned int mappages;
	unsigned int i;

	seed = initrand();	/* initialize random seed */

	SAFE_STAT(file, &statbuf);
	filesize = statbuf.st_size;

	fd = SAFE_OPEN(file, O_RDWR);

	if (statbuf.st_size - sparseoffset > UINT_MAX)
		tst_brk(TFAIL, "size_t overflow when setting up map");
	mapsize = (size_t) (statbuf.st_size - sparseoffset);
	mappages = roundup(mapsize, pagesize) / pagesize;
	offset = sparseoffset;
	if (do_offset) {
		int pageoffset = lrand48() % mappages;
		int byteoffset = pageoffset * pagesize;

		offset += byteoffset;
		mapsize -= byteoffset;
		mappages -= pageoffset;
	}
	nloops = (randloops) ? (lrand48() % MAXLOOPS) : MAXLOOPS;

	if (debug)
		tst_res(TINFO, "child %d (pid %d): seed %d, fsize %lld, mapsize %ld, off %lld, loop %d",
			procno, getpid(), seed, (long long)filesize,
			(long)mapsize, (long long)offset / pagesize, nloops);

	maddr = SAFE_MMAP(0, mapsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd,
			  offset);
	SAFE_CLOSE(fd);

	/*
	 *  Now loop read/writing random pages.
	 */
	for (loopcnt = 0; loopcnt < nloops; loopcnt++) {
		randpage = lrand48() % mappages;
		paddr = maddr + (randpage * pagesize);	/* page address */

		if (randpage < mappages - 1 || !(mapsize % pagesize))
			validsize = pagesize;
		else
			validsize = mapsize % pagesize;

		for (i = procno; i < validsize; i += nprocs) {
			if (*((unsigned char *)(paddr + i))
			    != ((procno + pattern) & 0xff))
				tst_brk(TFAIL, "child %d: invalid data <x%x>\n"
					" at pg %d off %d, exp <x%x>", procno,
					*((unsigned char *)(paddr + i)),
					randpage, i, (procno + pattern) & 0xff);

			/*
			 *  Now write it.
			 */
			*(paddr + i) = (procno + pattern) & 0xff;
		}
	}
	if (do_sync) {
		/*
		 * Exercise msync() as well!
		 */
		randpage = lrand48() % mappages;
		paddr = maddr + (randpage * pagesize);	/* page address */
		if (msync(paddr, (mappages - randpage) * pagesize,
			  MS_SYNC) == -1)
			tst_brk(TFAIL, "msync failed");
	}
	SAFE_MUNMAP(maddr, mapsize);
	exit(0);
}

/*
 *  Make sure file has all the correct data.
 */
int fileokay(char *file, unsigned char *expbuf)
{
	struct stat statbuf;
	size_t mapsize;
	unsigned int mappages;
	unsigned int pagesize = sysconf(_SC_PAGE_SIZE);
	unsigned char readbuf[pagesize];
	int fd;
	int cnt;
	unsigned int i, j;

	fd = SAFE_OPEN(file, O_RDONLY);

	SAFE_FSTAT(fd, &statbuf);
	SAFE_LSEEK(fd, sparseoffset, SEEK_SET);

	if (statbuf.st_size - sparseoffset > UINT_MAX)
		tst_brk(TFAIL, "size_t overflow when setting up map");
	mapsize = (size_t) (statbuf.st_size - sparseoffset);

	mappages = roundup(mapsize, pagesize) / pagesize;

	for (i = 0; i < mappages; i++) {
		cnt = read(fd, readbuf, pagesize);
		if (cnt == -1) {
			tst_brk(TFAIL, "read error");
		} else if ((unsigned int)cnt != pagesize) {
			/*
			 *  Okay if at last page in file...
			 */
			if ((i * pagesize) + cnt != mapsize) {
				tst_res(TINFO, "read %d of %ld bytes",
					(i * pagesize) + cnt, (long)mapsize);
				SAFE_CLOSE(fd);
				return 0;
			}
		}
		/*
		 *  Compare read bytes of data.
		 */
		for (j = 0; j < (unsigned int)cnt; j++) {
			if (expbuf[j] != readbuf[j]) {
				tst_res(TINFO,
					"read bad data: exp %c got %c, pg %d off %d, (fsize %lld)",
					expbuf[j], readbuf[j], i, j,
					(long long)statbuf.st_size);
				SAFE_CLOSE(fd);
				return 0;
			}
		}
	}
	SAFE_CLOSE(fd);

	return 1;
}

void finish(int sig LTP_ATTRIBUTE_UNUSED)
{
	finished++;
}

unsigned int initrand(void)
{
	unsigned int seed;

	/*
	 *  Initialize random seed...  Got this from a test written
	 *  by scooter:
	 *      Use srand/rand to diffuse the information from the
	 *      time and pid.  If you start several processes, then
	 *      the time and pid information don't provide much
	 *      variation.
	 */
	srand((unsigned int)getpid());
	seed = rand();
	srand((unsigned int)time(NULL));
	seed = (seed ^ rand()) % 100000;
	srand48((long)seed);
	return seed;
}

static struct tst_test test = {
	.needs_tmpdir = 1,
	.test_all = run,
	.setup = setup,
	.options = options,
	.cleanup = cleanup,
};
