#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2018 Petr Vorel <pvorel@suse.cz>
# Copyright (c) 2016 Oracle and/or its affiliates. All Rights Reserved.
# Author: Alexey Kodanev <alexey.kodanev@oracle.com>

TST_SETUP=tst_ipsec_setup_vti
TST_CLEANUP=tst_ipsec_cleanup
TST_TESTFUNC=do_test
. ipsec_lib.sh

do_test()
{
	PING_MAX="$IPSEC_REQUESTS"

	tst_res TINFO "Sending ICMP messages..."
	tst_ping $tst_vti $ip_rmt_tun $IPSEC_SIZE_ARRAY
}

tst_run
