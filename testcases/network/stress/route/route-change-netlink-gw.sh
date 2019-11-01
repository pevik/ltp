#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2020 Petr Vorel <pvorel@suse.cz>
#
# Change route gateway via netlink
# lhost: 10.23.1.1, gw (on rhost): 10.23.1.x, rhost: 10.23.0.1

TST_TESTFUNC="test_netlink"
. route-lib.sh

setup()
{
	local cnt=0

	check_max_ip
	setup_gw

	while [ $cnt -lt $NS_TIMES -a $cnt -lt $MAX_IP ]; do
		gw="$(tst_ipaddr_un -h 2,$max_ip_limit 1 $(($cnt + 1)))"
		gw_all="$gw^$gw_all"
		tst_add_ipaddr -s -q -a $gw rhost
		cnt=$((cnt+1))
	done

	ROUTE_CHANGE_NETLINK_PARAMS="-g "$gw_all" -l $lhost -r $rhost"
}

tst_run
