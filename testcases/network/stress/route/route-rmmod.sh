#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>
# Copyright (c) International Business Machines Corp., 2006
# Author: Mitsuru Chinen <mitch@jp.ibm.com>
# Rewrite into new shell API: Petr Vorel

TST_SETUP="setup"
TST_CLEANUP="cleanup"
TST_TESTFUNC="do_test"

module='veth'
TST_NEEDS_DRIVERS="$module"

. route-lib.sh
TST_CNT=$ROUTE_CHANGE_IP

setup()
{
	tst_res TINFO "adding IPv$TST_IPVER route destination and delete network driver $ROUTE_CHANGE_IP times"
}

cleanup()
{
	modprobe $module
	route_cleanup
}

do_test()
{
	local iface="$(tst_iface)"
	local rt="$(tst_ipaddr_un -p $1)"
	local rhost="$(tst_ipaddr_un $1 1)"

	tst_res TINFO "testing route '$rt'"

	tst_add_ipaddr -s -q -a $rhost rhost
	ROD ip route add $rt dev $iface
	EXPECT_PASS_BRK ping$TST_IPV6 -c1 -I $(tst_ipaddr) $rhost \>/dev/null

	ROD rmmod $module
	ROD modprobe $module
	reset_ltp_netspace
}

tst_run
