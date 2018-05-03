#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2018 Petr Vorel <pvorel@suse.cz>
# Copyright (c) 2017 Oracle and/or its affiliates. All Rights Reserved.
# Author: Alexey Kodanev <alexey.kodanev@oracle.com>

TST_NEEDS_TMPDIR=1
TST_TESTFUNC=do_test
TST_SETUP=do_setup
TST_CLEANUP=tst_ipsec_cleanup
. ipsec_lib.sh

max_requests=10

do_setup()
{
	# Configure SAD/SPD
	if [ -n "$IPSEC_MODE" -a -n "$IPSEC_PROTO" ]; then
		tst_ipsec lhost $(tst_ipaddr) $(tst_ipaddr rhost)
		tst_ipsec rhost $(tst_ipaddr rhost) $(tst_ipaddr)
	fi
}

do_test()
{
	tst_netload -H $(tst_ipaddr rhost) -n $2 -N $2 -r $IPSEC_REQUESTS \
		-R $max_requests
}

tst_run
