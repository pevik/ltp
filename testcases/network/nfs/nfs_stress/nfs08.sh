#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2023 Petr Vorel <pvorel@suse.cz>
# Test for broken NFS cache invalidation for directories.
# Kernel patch broke cache invalidation, which caused the second 'ls'
# not shown '2'.
# https://lore.kernel.org/linux-nfs/167649314509.15170.15885497881041431304@noble.neil.brown.name/
# Based on reproducer from Neil Brown <neilb@suse.de>

TST_TESTFUNC="do_test"
TST_SKIP_FILESYSTEMS="vfat,exfat"
TST_SETUP="do_setup"

do_setup()
{
	local util_version

	nfs_setup

	util_version=$(mount.nfs -V | sed 's/.*nfs-utils \([0-9]\)\..*/\1/')
	if ! tst_is_int "$util_version"; then
		tst_res TWARN "Failed to detect mount.nfs major version"
	elif [ "$util_version" -lt 2 ]; then
		tst_brk TCONF "Testing requires nfs-utils > 1"
	fi
}

do_test()
{
	tst_res TINFO "testing NFS cache invalidation for directories"

	touch 1
	EXPECT_PASS 'ls | grep 1'
	touch 2
	EXPECT_PASS 'ls | grep 2'
}

. nfs_lib.sh
tst_run
