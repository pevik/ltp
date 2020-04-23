#!/bin/sh
# Copyright (c) 2018-2020 Petr Vorel <pvorel@suse.cz>
set -ex

[ -z "$MINIMAL" ] && EXTRA_PKG="
	keyutils-devel
	libacl-devel
	libaio-devel
	libcap-devel
	libnuma-devel
	libopenssl-devel
	libselinux-devel
	libtirpc-devel"

zypper --non-interactive install --no-recommends \
	autoconf \
	automake \
	clang \
	findutils \
	gcc \
	gzip \
	kernel-default-devel \
	linux-glibc-devel \
	lsb-release \
	make \
	pkg-config \
	$EXTRA_PKG
