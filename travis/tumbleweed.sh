#!/bin/sh
# Copyright (c) 2018-2020 Petr Vorel <pvorel@suse.cz>
set -ex

zypper --non-interactive install --force-resolution --no-recommends \
	asciidoc \
	asciidoctor \
	autoconf \
	automake \
	clang \
	findutils \
	gcc \
	git \
	gzip \
	make \
	kernel-default-devel \
	keyutils-devel \
	libacl-devel \
	libaio-devel \
	libcap-devel \
	libmnl-devel \
	libnuma-devel \
	libopenssl-devel \
	libselinux-devel \
	libtirpc-devel \
	linux-glibc-devel \
	lsb-release \
	perl-JSON \
	pkg-config
