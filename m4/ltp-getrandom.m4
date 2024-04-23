dnl SPDX-License-Identifier: GPL-2.0-or-later
dnl Copyright (c) 2024 Petr Vorel <pvorel@suse.cz>
dnl Checks for getrandom support (glibc 2.25+, musl 1.1.20+).
dnl Support for Linux 3.17+ ignored.

AC_DEFUN([LTP_CHECK_GETRANDOM], [
    AC_MSG_CHECKING(for getrandom())
    AC_LINK_IFELSE(
		[AC_LANG_PROGRAM([[
		   #include <stdlib.h>  /* for NULL */
		   #include <sys/random.h>
		]],
		[[ return getrandom(NULL, 0U, 0U); ]] )],
		[AC_DEFINE([HAVE_GETRANDOM], [1], [Define to 1 if you have the `getrandom' function.])
		AC_MSG_RESULT([yes])],
		[AC_MSG_RESULT([no])])

	AC_SUBST(HAVE_GETRANDOM)
])
