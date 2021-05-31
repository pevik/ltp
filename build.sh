#!/bin/sh
# Copyright (c) 2017-2021 Petr Vorel <pvorel@suse.cz>
# Script for travis builds.
#
# TODO: Implement comparison of installed files. List of installed files can
# be used only for local builds as Travis currently doesn't support sharing
# file between jobs, see
# https://github.com/travis-ci/travis-ci/issues/6054

set -e

CFLAGS="${CFLAGS:--Wformat -Werror=format-security -Werror=implicit-function-declaration -Werror=return-type -fno-common}"
CC="${CC:-gcc}"

DEFAULT_PREFIX="$HOME/ltp-install"
DEFAULT_BUILD="native"
DEFAULT_TREE="in"

CONFIGURE_OPTS_IN_TREE="--with-open-posix-testsuite --with-realtime-testsuite $CONFIGURE_OPT_EXTRA"
# TODO: open posix testsuite is currently broken in out-tree-build. Enable it once it's fixed.
CONFIGURE_OPTS_OUT_TREE="--with-realtime-testsuite $CONFIGURE_OPT_EXTRA"

SRC_DIR="$(cd $(dirname $0); pwd)"
BUILD_DIR="$SRC_DIR/../ltp-build"

#MAKE_OPTS="-j$(getconf _NPROCESSORS_ONLN)"
MAKE_OPTS="-j1" # FIXME: debug
MAKE_OPTS_OUT_TREE="$MAKE_OPTS -C $BUILD_DIR -f $SRC_DIR/Makefile top_srcdir=$SRC_DIR top_builddir=$BUILD_DIR"

run_configure()
{
	local configure="$1"
	shift

	export CC CFLAGS LDFLAGS PKG_CONFIG_LIBDIR
	echo "CC='$CC' CFLAGS='$CFLAGS' LDFLAGS='$LDFLAGS' PKG_CONFIG_LIBDIR='$PKG_CONFIG_LIBDIR'"

	echo "=== configure $configure $@ ==="
	if ! $configure $@; then
		echo "== ERROR: configure failed, config.log =="
		cat config.log
		exit 1
	fi

	echo "== include/config.h =="
	cat include/config.h
}

configure_in_tree()
{
	run_configure ./configure $CONFIGURE_OPTS_IN_TREE --prefix=$prefix $@
}

configure_out_tree()
{
	mkdir -p $BUILD_DIR
	cd $BUILD_DIR
	echo "pev: configure_out_tree: pwd: $PWD (`pwd`)" # FIXME: debug
	run_configure $SRC_DIR/configure $CONFIGURE_OPTS_OUT_TREE $@
}

build()
{
	local tree="$1"
	shift

	if [ "$tree" = "in" ]; then
		build_in_tree $@
	else
		build_out_tree $@
	fi
}

build_out_tree()
{
	echo "pev: build_out_tree: pwd: $PWD (`pwd`)" # FIXME: debug
	make $MAKE_OPTS_OUT_TREE
}

build_in_tree()
{
	make $MAKE_OPTS
}

build_32()
{
	local dir
	local arch="$(uname -m)"

	echo "===== 32-bit ${1}-tree build into $prefix ====="

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
	build $1 $2
}

build_native()
{
	echo "===== native ${1}-tree build into $prefix ====="
	build $1 $2
}

build_cross()
{
	local host=$(basename "${CC%-gcc}")

	if [ "$host" = "gcc" ]; then
		echo "Invalid CC variable for cross compilation: $CC (clang not supported)" >&2
		exit 1
	fi

	echo "===== cross-compile ${host} ${1}-tree build into $prefix ====="
	build $1 $2 "--host=$host"
}

install_in_tree()
{
	make $MAKE_OPTS install
}

install_out_tree()
{
	make $MAKE_OPTS_OUT_TREE DESTDIR="$prefix" SKIP_IDCHECK=1 install
}

usage()
{
	cat << EOF
Usage:
$0 [ -c CC ] [ -o TREE ] [ -p DIR ] [ -t TYPE ]
$0 -h

Options:
-h       Print this help
-c CC    Define compiler (\$CC variable)
-o TREE  Specify build tree, default: $DEFAULT_TREE
-p DIR   Change installation directory. For in-tree build is this value passed
         to --prefix option of configure script. For out-of-tree build is this
         value passed to DESTDIR variable (i.e. sysroot) of make install
         target, which means that LTP will be actually installed into
         DIR/PREFIX (i.e. DIR/opt/ltp).
         Default for in-tree build: '$DEFAULT_PREFIX'
         Default for out-of-tree build: '$DEFAULT_PREFIX/opt/ltp'
-r RUN   Run only certain step (usable for CI), default: all
-t TYPE  Specify build type, default: $DEFAULT_BUILD

TREE:
in       in-tree build
out      out-of-tree build

TYPES:
32       32-bit build (PKG_CONFIG_LIBDIR auto-detection for x86_64)
cross    cross-compile build (requires set compiler via -c switch)
native   native build

RUN:
autotools
configure
build
install

Default configure options:
in-tree:    $CONFIGURE_OPTS_IN_TREE
out-of-tree $CONFIGURE_OPTS_OUT_TREE

configure options can extend the default with \$CONFIGURE_OPT_EXTRA environment variable
EOF
}

prefix="$DEFAULT_PREFIX"
build="$DEFAULT_BUILD"
tree="$DEFAULT_TREE"
install=
run=

while getopts "c:hio:p:r:t:" opt; do
	case "$opt" in
	c) CC="$OPTARG";;
	h) usage; exit 0;;
	i) install=1;;
	o) case "$OPTARG" in
		in|out) tree="$OPTARG";;
		*) echo "Wrong build tree '$OPTARG'" >&2; usage; exit 1;;
		esac;;
	p) prefix="$OPTARG";;
	r) case "$OPTARG" in
		autotools|configure|build|install) run="$OPTARG";;
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

echo "=== ver_linux ==="
./ver_linux
echo

echo "=== compiler version ==="
$CC --version

if [ -z "$run" -o "$run" = "autotools" ]; then
	make autotools
fi

if [ -z "$run" -o "$run" = "configure" ]; then
	eval configure_${tree}_tree
fi

if [ -z "$run" -o "$run" = "build" ]; then
	echo "=== build ==="
	eval build_$build $tree
fi

if [ "$install" = 1 -o "$run" = "install" ]; then
	eval install_${tree}_tree
else
	echo "make install skipped, use -i to run it"
fi
