dnl SPDX-License-Identifier: GPL-2.0-or-later
dnl Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>

AC_DEFUN([LTP_CHECK_LIBKEYUTILS], [
    PKG_CHECK_MODULES([LIBKEYUTILS], [libkeyutils], [
        AC_DEFINE([HAVE_LIBKEYUTILS], [1], [Define to 1 if you have libkeyutils library and headers])
	], [have_libkeyutils=no])
])
