#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2018 Petr Vorel <pvorel@suse.cz>
# Copyright (c) 2017 Oracle and/or its affiliates. All Rights Reserved.
# Author: Alexey Kodanev <alexey.kodanev@oracle.com>

TST_NEEDS_TMPDIR=1
TST_TESTFUNC=do_test
TST_SETUP=do_setup
TST_CLEANUP=tst_ipsec_setup_vti
. ipsec_lib.sh

do_test()
{
	local type="udp"
	[ $2 -gt 3 ] && type="udp_lite"

	tst_netload -H $ip_rmt_tun -T $type -n $2 -N $2 -r $IPSEC_REQUESTS
}

tst_run
