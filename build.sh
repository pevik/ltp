#!/bin/sh
# Copyright (c) 2017-2021 Petr Vorel <pvorel@suse.cz>
# Script for CI builds.

set -e

CFLAGS="${CFLAGS:--Wformat -Werror=format-security -Werror=implicit-function-declaration -Werror=return-type -fno-common}"
CC="${CC:-gcc}"

DEFAULT_PREFIX="$HOME/ltp-install"
DEFAULT_BUILD="native"

CONFIGURE_OPTS="--with-open-posix-testsuite --with-realtime-testsuite $CONFIGURE_OPT_EXTRA"

SRC_DIR="$(cd $(dirname $0); pwd)"
BUILD_DIR="$SRC_DIR/../ltp-build"

MAKE_OPTS="-j$(getconf _NPROCESSORS_ONLN)"

run_configure()
{
	local opts="$CONFIGURE_OPTS --prefix=$prefix $@"

	export CC CFLAGS LDFLAGS PKG_CONFIG_LIBDIR
	echo "CC='$CC' CFLAGS='$CFLAGS' LDFLAGS='$LDFLAGS' PKG_CONFIG_LIBDIR='$PKG_CONFIG_LIBDIR'"

	echo "=== ./configure $opts ==="
	if ! ./configure $opts; then
		echo "== ERROR: ./configure failed, config.log =="
		cat config.log
		exit 1
	fi

	echo "== include/config.h =="
	cat include/config.h
}

configure_32()
{
	local prefix="$1"
	local arch="$(uname -m)"
	local dir

	echo "===== 32-bit build into $prefix ====="

	if [ -z "$PKG_CONFIG_LIBDIR" ]; then
		if [ "$arch" != "x86_64" ]; then
			echo "ERROR: auto-detection not supported platform $arch, export PKG_CONFIG_LIBDIR!"
			exit 1
		fi

		for dir in /usr/lib/i386-linux-gnu/pkgconfig \
			/usr/lib32/pkgconfig /usr/lib/pkgconfig; do
			if [ -d "$dir" ]; then
				PKG_CONFIG_LIBDIR="$dir"
				break
			fi
		done
		if [ -z "$PKG_CONFIG_LIBDIR" ]; then
			echo "WARNING: PKG_CONFIG_LIBDIR not found, build might fail"
		fi
	fi

	CFLAGS="-m32 $CFLAGS" LDFLAGS="-m32 $LDFLAGS"
	run_configure
}

configure_native()
{
	local prefix="$1"

	echo "===== native build into $prefix ====="
	run_configure
}

configure_cross()
{
	local prefix="$1"
	local host=$(basename "${CC%-gcc}")

	if [ "$host" = "gcc" ]; then
		echo "Invalid CC variable for cross compilation: $CC (clang not supported)" >&2
		exit 1
	fi

	echo "===== cross-compile ${host} build into $prefix ====="
	run_configure "--host=$host"
}

build()
{
	echo "=== build ==="
	make $MAKE_OPTS
}

test()
{
	echo "=== test ==="
	make $1
}

install()
{
	echo "=== install ==="
	make $MAKE_OPTS install
}

usage()
{
	cat << EOF
Usage:
$0 [ -c CC ] [ -i ] [ -p DIR ] [-r RUN ] [ -t TYPE ]
$0 -h

Options:
-h       Print this help
-c CC    Define compiler (\$CC variable), needed only for configure step
-i       Run 'make install', needed only for install step
-p DIR   Change installation directory (--prefix)
         Default: '$DEFAULT_PREFIX'
-r RUN   Run only certain step (usable for CI), default: all
-t TYPE  Specify build type, default: $DEFAULT_BUILD, only for configure step

TYPES:
32       32-bit build (PKG_CONFIG_LIBDIR auto-detection for x86_64)
cross    cross-compile build (requires set compiler via -c switch)
native   native build

RUN:
autotools   run only 'make autotools'
configure   run only 'configure'
build       run only 'make'
test        run only 'make test' (not supported for cross-compile build)
test-c      run only 'make test-c' (not supported for cross-compile build)
test-shell  run only 'make test-shell' (not supported for cross-compile build)
install     run only 'make install'

Default configure options: $CONFIGURE_OPTS

configure options can extend the default with \$CONFIGURE_OPT_EXTRA environment variable
EOF
}

prefix="$DEFAULT_PREFIX"
build="$DEFAULT_BUILD"
install=
run=

while getopts "c:hip:r:t:" opt; do
	case "$opt" in
	c) CC="$OPTARG";;
	h) usage; exit 0;;
	i) install=1;;
	p) prefix="$OPTARG";;
	r) case "$OPTARG" in
		autotools|configure|build|test|test-c|test-shell|install) run="$OPTARG";;
		*) echo "Wrong run type '$OPTARG'" >&2; usage; exit 1;;
		esac;;
	t) case "$OPTARG" in
		32|cross|native) build="$OPTARG";;
		*) echo "Wrong build type '$OPTARG'" >&2; usage; exit 1;;
		esac;;
	?) usage; exit 1;;
	esac
done

cd $SRC_DIR

if [ -z "$run" -o "$run" = "autotools" ]; then
	make autotools
fi

if [ -z "$run" -o "$run" = "configure" ]; then
	eval configure_$build $prefix
fi

if [ -z "$run" -o "$run" = "build" ]; then
	build
fi

if [ -z "$run" -o "$run" = "test" -o "$run" = "test-c" -o "$run" = "test-shell" ]; then
	if [ "$build" = "cross" ]; then
		echo "cross-compile build, skipping running tests" >&2
	else
		test $run
	fi
fi

if [ -z "$run" -o "$run" = "install" ]; then
	if [ "$install" = 1 ]; then
		install
	else
		echo "make install skipped, use -i to run it"
	fi
fi

exit $?
