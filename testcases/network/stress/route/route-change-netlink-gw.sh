#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>
#
# Change route gateway via netlink
# lhost: 10.23.1.1, gw (on rhost): 10.23.1.x, rhost: 10.23.0.1

TST_TESTFUNC="test_gw"
#. route-lib.sh
TST_SETUP="setup" # FIXME: debug
TST_CLEANUP="cleanup" # FIXME: debug
. tst_net.sh # FIXME: debug

MAX_IP=${MAX_IP:-5}

setup()
{
	tst_res TINFO "change IPv$TST_IPVER route gateway $NS_TIMES times"

	rt="$(tst_ipaddr_un -p 0 0)"
	lhost="$(tst_ipaddr_un 1 1)"
	rhost="$(tst_ipaddr_un 0 1)"
	tst_add_ipaddr -s -q -a $lhost
	tst_add_ipaddr -s -q -a $rhost rhost

	local cnt=0
	local max_ip_limit=254
	[ "$TST_IPV6" ] && max_ip_limit=65534
	local gw

	tst_is_int $MAX_IP || tst_brk TBROK "\$MAX_IP not int ($MAX_IP)"
	[ "$MAX_IP" -gt $max_ip_limit ] && MAX_IP=$max_ip_limit

	while [ $cnt -lt $NS_TIMES -a $cnt -lt $MAX_IP ]; do
		#gw="$(tst_ipaddr_un -h 2,254 1 $(($cnt + 1)))" # FIXME: debug
		gw="$(tst_ipaddr_un -h 2,$max_ip_limit 1 $(($cnt + 1)))"
		gw_all="$gw^$gw_all"
		# NOTE: in shell test part of testing
		tst_add_ipaddr -s -q -a $gw rhost
		#tst_add_ipaddr -s -a $(tst_ipaddr_un $cnt $LHOST)
		#tst_add_ipaddr -s -a $(tst_ipaddr_un $cnt $RHOST) rhost
		cnt=$((cnt+1))
	done
	MAX_IP=$cnt
}

test_gw()
{
	local ip_flag
	[ "$TST_IPV6" ] && ip_flag="-6"

	# TODO: adapt from concept the same ip on lhost and rhost to:
	# lhost: 10.23.1.1, gw (on rhost): 10.23.1.x, rhost: 10.23.0.1
	EXPECT_PASS route-change-netlink -d $(tst_iface) $ip_flag -g "$gw_all" -l $lhost -r $rhost
}

# FIXME: debug
cleanup()
{
	tst_restore_ipaddr
	tst_restore_ipaddr rhost
}
tst_run
