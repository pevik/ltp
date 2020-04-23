#!/bin/sh
# Copyright (c) 2018-2020 Petr Vorel <pvorel@suse.cz>
set -ex

[ -z "$MINIMAL" ] && EXTRA_PKG="
	libtirpc
	libtirpc-devel"

yum -y install \
	autoconf \
	automake \
	make \
	clang \
	gcc \
	findutils \
	pkg-config \
	redhat-lsb-core \
	$EXTRA_PKG
