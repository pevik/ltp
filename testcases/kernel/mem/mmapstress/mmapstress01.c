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
#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"

#define MAXLOOPS	500	/* max pages for map children to write */
#define	FILESIZE	4096	/* initial filesize set up by parent */
#define TEST_FILE	"mmapstress01.out"

#ifdef roundup
#undef roundup
#endif
#define roundup(x, y)	((((x)+((y)-1))/(y))*(y))

static char *usage =
	"-p nprocs [-t minutes -f filesize -S sparseoffset -r -o -m -l -d]";

static unsigned int initrand(void);
static void finish(int sig);
static void child_mapper(char *file, unsigned int procno, unsigned int nprocs);
static int fileokay(char *file, unsigned char *expbuf);
static int finished;
static int leavefile;

static float alarmtime;
static unsigned int nprocs;

static pid_t *pidarray;
static int wait_stat;
static int check_for_sanity;
static unsigned char *buf;

static int debug;
#ifdef LARGE_FILE
static off64_t filesize = FILESIZE;
static off64_t sparseoffset;
#else /* LARGE_FILE */
static off_t filesize = FILESIZE;
static off_t sparseoffset;
#endif /* LARGE_FILE */
static unsigned int randloops;
static unsigned int dosync;
static unsigned int do_offset;
static unsigned int pattern;

int main(int argc, char *argv[])
{
	char *progname;
	int fd;
	int c;
	extern char *optarg;
	unsigned int procno;
	pid_t pid;
	unsigned int seed;
	int pagesize = sysconf(_SC_PAGE_SIZE);
	struct sigaction sa;
	unsigned int i;
	int write_cnt;
	unsigned char data;
#ifdef LARGE_FILE
	off64_t bytes_left;
#else /* LARGE_FILE */
	off_t bytes_left;
#endif /* LARGE_FILE */

	progname = *argv;
	if (argc < 2)
		tst_brk(TBROK, "usage: %s %s", progname, usage);

	while ((c = getopt(argc, argv, "S:omdlrf:p:t:")) != -1) {
		switch (c) {
		case 'd':
			debug = 1;
			break;
		case 't':
			alarmtime = atof(optarg) * 60;
			break;
		case 'p':
			nprocs = atoi(optarg);
			break;
		case 'l':
			leavefile = 1;
			break;
		case 'f':
#ifdef LARGE_FILE
			filesize = atoll(optarg);
#else /* LARGE_FILE */
			filesize = atoi(optarg);
#endif /* LARGE_FILE */
			if (filesize < 0)
				tst_brk(TBROK, "error: negative filesize");
			break;
		case 'r':
			randloops = 1;
			break;
		case 'm':
			dosync = 1;
			break;
		case 'o':
			do_offset = 1;
			break;
		case 'S':
#ifdef LARGE_FILE
			sparseoffset = atoll(optarg);
#else /* LARGE_FILE */
			sparseoffset = atoi(optarg);
#endif /* LARGE_FILE */
			if (sparseoffset % pagesize != 0)
				tst_brk(TBROK,
					"sparseoffset must be pagesize multiple");
			break;
		default:
			tst_brk(TBROK, "usage: %s %s", progname, usage);
		}
	}

	/* nprocs is >= 0 since it's unsigned */
	if (nprocs > 255)
		tst_brk(TBROK, "invalid nprocs %d - (range 0-255)", nprocs);

	seed = initrand();
	pattern = seed & 0xff;

	if (debug) {
#ifdef LARGE_FILE
		tst_res(TINFO, "creating file <%s> with %lld bytes, pattern %d",
			TEST_FILE, filesize, pattern);
#else /* LARGE_FILE */
		tst_res(TINFO, "creating file <%s> with %ld bytes, pattern %d",
			TEST_FILE, filesize, pattern);
#endif /* LARGE_FILE */
		if (alarmtime)
			tst_res(TINFO, "running for %f minutes",
				alarmtime / 60);
		else
			tst_res(TINFO, "running with no time limit");
	}

	tst_reinit();

	/*
	 *  Plan for death by signal.  User may have specified
	 *  a time limit, in which set an alarm and catch SIGALRM.
	 *  Also catch and cleanup with SIGINT.
	 */
	sa.sa_handler = finish;
	sa.sa_flags = 0;
	if (sigemptyset(&sa.sa_mask))
		tst_brk(TFAIL, "sigemptyset error");

	if (sigaction(SIGINT, &sa, 0) == -1)
		tst_brk(TFAIL, "sigaction error SIGINT");
	if (sigaction(SIGQUIT, &sa, 0) == -1)
		tst_brk(TFAIL, "sigaction error SIGQUIT");
	if (sigaction(SIGTERM, &sa, 0) == -1)
		tst_brk(TFAIL, "sigaction error SIGTERM");

	if (alarmtime) {
		if (sigaction(SIGALRM, &sa, 0) == -1)
			tst_brk(TFAIL, "sigaction error");
		(void)alarm(alarmtime);
	}
#ifdef LARGE_FILE
	if ((fd = open64(TEST_FILE, O_CREAT | O_TRUNC | O_RDWR, 0664)) == -1) {
#else /* LARGE_FILE */
	if ((fd = open(TEST_FILE, O_CREAT | O_TRUNC | O_RDWR, 0664)) == -1) {
#endif /* LARGE_FILE */
		tst_brk(TFAIL, "open error");
	}

	if ((buf = malloc(pagesize)) == NULL
	    || (pidarray = malloc(nprocs * sizeof(pid_t))) == NULL) {
		tst_brk(TFAIL, "malloc error");
	}

	for (i = 0; i < nprocs; i++)
		*(pidarray + i) = 0;

	for (i = 0, data = 0; i < pagesize; i++) {
		*(buf + i) = (data + pattern) & 0xff;
		if (++data == nprocs)
			data = 0;
	}
#ifdef LARGE_FILE
	if (lseek64(fd, (off64_t)sparseoffset, SEEK_SET) < 0) {
#else /* LARGE_FILE */
	if (lseek(fd, (off_t)sparseoffset, SEEK_SET) < 0) {
#endif /* LARGE_FILE */
		tst_brk(TFAIL, "lseek");
	}
	for (bytes_left = filesize; bytes_left; bytes_left -= c) {
		write_cnt = MIN(pagesize, (int)bytes_left);
		if ((c = write(fd, buf, write_cnt)) != write_cnt) {
			if (c == -1)
				tst_res(TINFO, "write error");
			else
				tst_res(TINFO, "write: wrote %d of %d bytes",
					c, write_cnt);
			(void)close(fd);
			(void)unlink(TEST_FILE);
			tst_brk(TFAIL, "write error");
		}
	}

	(void)close(fd);

	/*
	 *  Fork off mmap children.
	 */
	for (procno = 0; procno < nprocs; procno++) {
		switch (pid = fork()) {

		case -1:
			tst_brk(TFAIL, "fork error");
			break;

		case 0:
			child_mapper(TEST_FILE, procno, nprocs);
			exit(0);

		default:
			pidarray[procno] = pid;
		}
	}

	/*
	 *  Now wait for children and refork them as needed.
	 */

	while (!finished) {
		pid = wait(&wait_stat);
		/*
		 *  Block signals while processing child exit.
		 */

		if (sighold(SIGALRM) || sighold(SIGINT))
			tst_brk(TFAIL, "sighold error");

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
				child_mapper(TEST_FILE, i, nprocs);
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
		if (sigrelse(SIGALRM) || sigrelse(SIGINT))
			tst_brk(TFAIL, "sigrelse error");
	}

	/*
	 *  Finished!  Check the file for sanity, then kill all
	 *  the children and done!.
	 */

	if (sighold(SIGALRM))
		tst_brk(TFAIL, "sighold error");
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
				(void)unlink(TEST_FILE);
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
#ifdef LARGE_FILE
	struct stat64 statbuf;
	off64_t filesize;
	off64_t offset;
#else /* LARGE_FILE */
	struct stat statbuf;
	off_t filesize;
	off_t offset;
#endif /* LARGE_FILE */
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

#ifdef LARGE_FILE
	if (stat64(file, &statbuf) == -1)
#else /* LARGE_FILE */
	if (stat(file, &statbuf) == -1)
#endif /* LARGE_FILE */
		tst_brk(TFAIL, "stat error");
	filesize = statbuf.st_size;

#ifdef LARGE_FILE
	if ((fd = open64(file, O_RDWR)) == -1)
#else /* LARGE_FILE */
	if ((fd = open(file, O_RDWR)) == -1)
#endif /* LARGE_FILE */
		tst_brk(TFAIL, "open error");

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
			procno, getpid(), seed, filesize, (long)mapsize,
			offset / pagesize, nloops);
#ifdef LARGE_FILE
	if ((maddr = mmap64(0, mapsize, PROT_READ | PROT_WRITE, MAP_SHARED,
			    fd, (off64_t)offset)) == (caddr_t) - 1)
#else /* LARGE_FILE */
	if ((maddr = mmap(0, mapsize, PROT_READ | PROT_WRITE, MAP_SHARED,
			  fd, (off_t)offset)) == (caddr_t) - 1)
#endif /* LARGE_FILE */
		tst_brk(TFAIL, "mmap error");

	(void)close(fd);

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
	if (dosync) {
		/*
		 * Exercise msync() as well!
		 */
		randpage = lrand48() % mappages;
		paddr = maddr + (randpage * pagesize);	/* page address */
		if (msync(paddr, (mappages - randpage) * pagesize,
			  MS_SYNC) == -1)
			tst_brk(TFAIL, "msync failed");
	}
	if (munmap(maddr, mapsize) == -1)
		tst_brk(TFAIL, "munmap failed");
	exit(0);
}

/*
 *  Make sure file has all the correct data.
 */
int fileokay(char *file, unsigned char *expbuf)
{
#ifdef LARGE_FILE
	struct stat64 statbuf;
#else /* LARGE_FILE */
	struct stat statbuf;
#endif /* LARGE_FILE */
	size_t mapsize;
	unsigned int mappages;
	unsigned int pagesize = sysconf(_SC_PAGE_SIZE);
	unsigned char readbuf[pagesize];
	int fd;
	int cnt;
	unsigned int i, j;

#ifdef LARGE_FILE
	if ((fd = open64(file, O_RDONLY)) == -1)
#else /* LARGE_FILE */
	if ((fd = open(file, O_RDONLY)) == -1)
#endif /* LARGE_FILE */
		tst_brk(TFAIL, "open error");

#ifdef LARGE_FILE
	if (fstat64(fd, &statbuf) == -1)
#else /* LARGE_FILE */
	if (fstat(fd, &statbuf) == -1)
#endif /* LARGE_FILE */
		tst_brk(TFAIL, "stat error");

#ifdef LARGE_FILE
	if (lseek64(fd, sparseoffset, SEEK_SET) < 0)
#else /* LARGE_FILE */
	if (lseek(fd, sparseoffset, SEEK_SET) < 0)
#endif /* LARGE_FILE */
		tst_brk(TFAIL, "lseek");

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
				close(fd);
				return 0;
			}
		}
		/*
		 *  Compare read bytes of data.
		 */
		for (j = 0; j < (unsigned int)cnt; j++) {
			if (expbuf[j] != readbuf[j]) {
				tst_res(TINFO, "read bad data: exp %c got %c)",
					expbuf[j], readbuf[j]);
#ifdef LARGE_FILE
				tst_res(TINFO, ", pg %d off %d, "
					"(fsize %lld)", i, j, statbuf.st_size);
#else /* LARGE_FILE */
				tst_res(TINFO, ", pg %d off %d, "
					"(fsize %ld)", i, j, statbuf.st_size);
#endif /* LARGE_FILE */
				close(fd);
				return 0;
			}
		}
	}
	close(fd);

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
	.cleanup = cleanup,
};
