#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2015-2018 Oracle and/or its affiliates. All Rights Reserved.
# Copyright (c) International Business Machines  Corp., 2001
#
#  PURPOSE: Stresses NFS by opening a large number of files on a nfs
#           mounted filesystem.
#
# Ported by Robbie Williamson (robbiew@us.ibm.com)

TST_TESTFUNC="do_test"
TST_CLEANUP="do_cleanup"

do_cleanup()
{
	echo "pids: '$pids'" >&2 # FIXME: debug
	[ -n "$pids" ] && kill -9 $pids
	nfs_cleanup
}

do_test()
{
	local n=0
	local pids

	tst_res TINFO "starting 'nfs01_open_files $NFILES'"

	# ROD nfs01_open_files $NFILES
	for i in $VERSION; do
		nfs01_open_files $NFILES &
		pids="$pids $!"
		n=$(( n + 1 ))
	done

	tst_res TINFO "waiting for pids:$pids"
	for p in $pids; do
		wait $p || tst_brk TFAIL "nfs01_open_files process failed"
		tst_res TINFO "fsstress '$p' completed"
	done
	pids=

	tst_res TPASS "test finished successfully"
}

. nfs_lib.sh
tst_run
