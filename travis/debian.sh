#!/bin/sh
# Copyright (c) 2018-2020 Petr Vorel <pvorel@suse.cz>
set -ex

# workaround for missing oldstable-updates repository
# W: Failed to fetch http://deb.debian.org/debian/dists/oldstable-updates/main/binary-amd64/Packages
grep -v oldstable-updates /etc/apt/sources.list > /tmp/sources.list && mv /tmp/sources.list /etc/apt/sources.list

apt update

[ -z "$MINIMAL" ] && EXTRA_PKG="
	acl-dev
	libacl1
	libacl1-dev
	libaio-dev
	libaio1
	libcap-dev
	libcap2
	libkeyutils-dev
	libkeyutils1
	libmm-dev
	libnuma-dev
	libnuma1
	libselinux1-dev
	libsepol1-dev
	libssl-dev
	libtirpc-dev"

apt install -y --no-install-recommends \
	autoconf \
	automake \
	build-essential \
	clang \
	debhelper \
	devscripts \
	gcc \
	libc6-dev \
	linux-libc-dev \
	lsb-release \
	pkg-config \
	$EXTRA_PKG

df -hT
