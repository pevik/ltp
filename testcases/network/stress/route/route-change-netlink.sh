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

MAX_IP=${MAX_IP:-5}

route_change_parse_args()
{
	case "$1" in
	g) g_opt="-g $GATEWAY" ;;
	esac
}

setup()
{
	local cnt=0
	local max_ip_limit=254
	[ "$TST_IPV6" ] && max_ip_limit=65534

	tst_is_int $MAX_IP || tst_brk TBROK "\$MAX_IP not int ($MAX_IP)"
	[ "$MAX_IP" -gt $max_ip_limit ] && MAX_IP=$max_ip_limit

	while [ $cnt -lt $NS_TIMES -a $cnt -lt $MAX_IP ]; do
		tst_add_ipaddr -s -a $(tst_ipaddr_un $cnt $LHOST)
		tst_add_ipaddr -s -a $(tst_ipaddr_un $cnt $RHOST) rhost
		cnt=$((cnt+1))
	done
	MAX_IP=$cnt
}

cleanup()
{
	tst_restore_ipaddr
	tst_restore_ipaddr rhost
}

do_test()
{
	local ip_flag
	[ "$TST_IPV6" ] && ip_flag="-6"

	EXPECT_PASS route-change-netlink -c $NS_TIMES -d $(tst_iface) $ip_flag $g_opt -l $LHOST -m $MAX_IP -r $RHOST
}

tst_run
