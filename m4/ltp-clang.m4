dnl SPDX-License-Identifier: GPL-2.0-or-later
dnl Copyright (c) 2021 SUSE LLC

dnl Find the libraries and tools necessary to build tools/clang-check

dnl Note that it is possible to use libclang without the clang
dnl executable or llvm-config. However it then means we have to start
dnl searching the system for various Clang resources.

AC_DEFUN([LTP_CHECK_CLANG],
	[AC_CHECK_TOOL(LLVM_CONFIG, llvm-config, :)
	 AC_CHECK_TOOL(CLANG, clang, :)
	 ltp_save_CFLAGS=$CFLAGS
	 ltp_save_LDFLAGS=$LDFLAGS
	 if test $LLVM_CONFIG != ":"; then
	    CFLAGS=$($LLVM_CONFIG --cflags)
	    LDFLAGS=$($LLVM_CONFIG --ldflags)
	 fi
	 AC_CHECK_LIB([clang], [clang_createIndex], [have_libclang=yes])
	 AC_CHECK_HEADERS([clang-c/Index.h], [have_clang_h=1])
	 if test x$have_libclang != xyes -o x$have_clang_h != x1 -o "x$CLANG" = "x:"; then
	    AC_MSG_WARN(Libclang and Clang are needed for test development)
	 else
	    AC_SUBST(HAVE_CLANG_C_INDEX_H, $have_clang_h)
	    AC_SUBST(CLANG_LIBS, -lclang)
	 fi
	 CFLAGS=$ltp_save_CFLAGS
	 LDFLAGS=$ltp_save_LDFLAGS
])
