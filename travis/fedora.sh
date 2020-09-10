#!/bin/sh
# Copyright (c) 2018-2020 Petr Vorel <pvorel@suse.cz>
set -ex

yum -y install \
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
yum -y install libmnl-devel || yum -y install libmnl
yum -y install asciidoctor || true
