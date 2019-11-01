#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2019 Petr Vorel <pvorel@suse.cz>

TST_TESTFUNC="do_test"
TST_OPTS="g"
TST_PARSE_ARGS="route_change_parse_args"
TST_SETUP="setup"
TST_CLEANUP="cleanup"
TST_NEEDS_TMPDIR=1
. tst_net.sh

GATEWAY=1
LHOST=2
RHOST=3

MAX_IP=${MAX_IP:-255}
NS_TIMES=${NS_TIMES:-10000}

route_change_parse_args()
{
	case "$1" in
	g) g_opt="-g $GATEWAY" ;;
	esac
}

setup()
{
	local cnt=0

	while [ $cnt -lt $NS_TIMES -a $cnt -lt $MAX_IP ]; do
		tst_add_ipaddr -s -a $(tst_ipaddr_un $cnt $LHOST)
		tst_add_ipaddr -s -a $(tst_ipaddr_un $cnt $RHOST) rhost
		cnt=$((cnt+1))
	done
}

cleanup()
{
	tst_restore_ipaddr
	tst_restore_ipaddr rhost
}

do_test()
{
	local ip_flag port
	[ "$TST_IPV6" ] && ip_flag="-6"

	EXPECT_PASS route-change-netlink -d $(tst_iface) -c $NS_TIMES $ip_flag $g_opt -l $LHOST -m $MAX_IP -r $RHOST
}

tst_run
