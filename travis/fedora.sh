#!/bin/sh
# Copyright (c) 2018-2020 Petr Vorel <pvorel@suse.cz>
set -ex

yum="yum -y install"

$yum \
	asciidoc \
	autoconf \
	automake \
	make \
	clang \
	gcc \
	git \
	findutils \
	libtirpc \
	libtirpc-devel \
	perl-JSON \
	pkg-config \
	redhat-lsb-core

# CentOS 8 fixes
$yum libmnl-devel || $yum libmnl
