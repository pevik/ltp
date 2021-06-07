/*
 * Copyright (c) International Business Machines  Corp., 2001
 *
 * This program is free software;  you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY;  without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 * the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program;  if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
  HISTORY
  03/2001 - Written by Wayne Boyer

  TEST ITEMS:
  Check that a getitimer() call fails as expected
  with an incorrect second argument.
*/


#include "test.h"

#include <errno.h>
#include <sys/time.h>
#include "lapi/syscalls.h"

char *TCID = "getitimer02";
int TST_TOTAL = 1;

#if !defined(UCLINUX)

static void cleanup(void);
static void setup(void);

static int libc_getitimer(int which, void *curr_value)
{
	return getitimer(which, curr_value);
}

static int sys_getitimer(int which, void *curr_value)
{
	return ltp_syscall(__NR_getitimer, which, curr_value);
}

static struct test_variants
{
	int (*getitimer)(int which, void *curr_value);
	char *desc;
} variants[] = {
	{ .getitimer = libc_getitimer, .desc = "libc getitimer()"},

#if (__NR_getitimer != __LTP__NR_INVALID_SYSCALL)
	{ .getitimer = sys_getitimer,  .desc = "__NR_getitimer syscall"},
#endif
};

unsigned int tst_variant_t;
int TST_VARIANTS = ARRAY_SIZE(variants);

int main(int ac, char **av)
{
	int lc, i;
	struct test_variants *tv = &variants[tst_variant_t];

	tst_parse_opts(ac, av, NULL, NULL);

	setup();

	for (lc = 0; TEST_LOOPING(lc); lc++) {
		tst_count = 0;

		for (i = 0; i < TST_VARIANTS; i++) {
			tst_resm(TINFO, "Testing variant: %s", tv->desc);

			/* call with a bad address */
			if (tv->getitimer == libc_getitimer) {
				tst_resm(TCONF, "EFAULT skipped for libc variant");
				tv++;
				continue;
			}

		TEST(tv->getitimer(ITIMER_REAL, (struct itimerval *)-1));

			if (TEST_RETURN == 0) {
				tst_resm(TFAIL, "call failed to produce "
					 "expected error - errno = %d - %s",
					TEST_ERRNO, strerror(TEST_ERRNO));
				continue;
			}
		}
		switch (TEST_ERRNO) {
		case EFAULT:
			tst_resm(TPASS, "expected failure - errno = %d - %s",
				 TEST_ERRNO, strerror(TEST_ERRNO));
			break;
		default:
			tst_resm(TFAIL, "call failed to produce "
				 "expected error - errno = %d - %s",
				 TEST_ERRNO, strerror(TEST_ERRNO));
		}
	}

	cleanup();

	tst_exit();
}

static void setup(void)
{
	tst_sig(NOFORK, DEF_HANDLER, cleanup);

	TEST_PAUSE;
}

static void cleanup(void)
{
}

#else

int main(void)
{
	tst_resm(TINFO, "test is not available on uClinux");
	tst_exit();
}

#endif /* if !defined(UCLINUX) */
