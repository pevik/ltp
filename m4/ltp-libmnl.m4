dnl Copyright (c) 2019 Petr Vorel <pvorel@suse.cz>

AC_DEFUN([LTP_CHECK_LIBMNL], [
    PKG_CHECK_MODULES([LIBMNL], [libmnl], [
        AC_DEFINE([HAVE_LIBMNL], [1], [Define to 1 if you have libmnl libraries and headers])
	], [have_libmnl=no])
])
