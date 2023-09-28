#!/bin/sh -eux

zyp="zypper --non-interactive install --force-resolution --no-recommends"

$zyp \
	iproute2 \
	keyutils \
	libaio1 \
	libmnl0 \
	libnuma1 \
	libtirpc3

useradd ltp
