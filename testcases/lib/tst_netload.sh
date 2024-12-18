#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2024 Petr Vorel <pvorel@suse.cz>
# TODO: shell runner => run netstress as a child

tst_netload_brk()
{
	tst_rhost_run -c "cat $TST_TMPDIR/netstress.log"
	cat tst_netload.log
	tst_brk_ $1 $2
}

# Run network load test, see 'netstress -h' for option description
tst_netload()
{
	local rfile="tst_netload.res"
	local expect_res="pass"
	local ret=0
	local type="tcp"
	local hostopt=
	local setup_srchost=0
	# common options for client and server
	local cs_opts=

	local run_cnt="$TST_NETLOAD_RUN_COUNT"
	local c_num="$TST_NETLOAD_CLN_NUMBER"
	local c_requests="$TST_NETLOAD_CLN_REQUESTS"
	local c_opts=

	# number of server replies after which TCP connection is closed
	local s_replies="${TST_NETLOAD_MAX_SRV_REPLIES:-500000}"
	local s_opts=
	local bind_to_device=1

	if [ ! "$TST_NEEDS_TMPDIR" = 1 ]; then
		tst_brk_ TBROK "Using tst_netload requires setting TST_NEEDS_TMPDIR=1"
	fi

	OPTIND=0
	while getopts :a:c:H:n:N:r:R:S:b:t:T:fFe:m:A:D: opt; do
		case "$opt" in
		a) c_num="$OPTARG" ;;
		H) c_opts="${c_opts}-H $OPTARG "
		   hostopt="$OPTARG" ;;
		c) rfile="$OPTARG" ;;
		n) c_opts="${c_opts}-n $OPTARG " ;;
		N) c_opts="${c_opts}-N $OPTARG " ;;
		r) c_requests="$OPTARG" ;;
		A) c_opts="${c_opts}-A $OPTARG " ;;
		R) s_replies="$OPTARG" ;;
		S) c_opts="${c_opts}-S $OPTARG "
		   setup_srchost=1 ;;
		b) cs_opts="${cs_opts}-b $OPTARG " ;;
		t) cs_opts="${cs_opts}-t $OPTARG " ;;
		T) cs_opts="${cs_opts}-T $OPTARG "
		   type="$OPTARG" ;;
		m) cs_opts="${cs_opts}-m $OPTARG " ;;
		f) cs_opts="${cs_opts}-f " ;;
		F) cs_opts="${cs_opts}-F " ;;
		e) expect_res="$OPTARG" ;;
		D) [ "$TST_NETLOAD_BINDTODEVICE" = 1 ] && cs_opts="${cs_opts}-d $OPTARG "
		   bind_to_device=0 ;;
		*) tst_brk_ TBROK "tst_netload: unknown option: $OPTARG" ;;
		esac
	done
	OPTIND=0

	[ "$setup_srchost" = 1 ] && s_opts="${s_opts}-S $hostopt "

	if [ "$bind_to_device" = 1 -a "$TST_NETLOAD_BINDTODEVICE" = 1 ]; then
		c_opts="${c_opts}-d $(tst_iface) "
		s_opts="${s_opts}-d $(tst_iface rhost) "
	fi

	local expect_ret=0
	[ "$expect_res" != "pass" ] && expect_ret=3

	local was_failure=0
	if [ "$run_cnt" -lt 2 ]; then
		run_cnt=1
		was_failure=1
	fi

	s_opts="${cs_opts}${s_opts}-R $s_replies -B $TST_TMPDIR"
	c_opts="${cs_opts}${c_opts}-a $c_num -r $((c_requests / run_cnt)) -c $PWD/$rfile"

	tst_res_ TINFO "run server 'netstress $s_opts'"
	tst_res_ TINFO "run client 'netstress -l $c_opts' $run_cnt times"

	tst_rhost_run -c "pkill -9 netstress\$"
	rm -f tst_netload.log

	local results
	local passed=0

	for i in $(seq 1 $run_cnt); do
		tst_rhost_run -c "netstress $s_opts" > tst_netload.log 2>&1
		if [ $? -ne 0 ]; then
			cat tst_netload.log
			local ttype="TFAIL"
			grep -e 'CONF:' tst_netload.log && ttype="TCONF"
			tst_brk_ $ttype "server failed"
		fi

		local port=$(tst_rhost_run -s -c "cat $TST_TMPDIR/netstress_port")
		netstress -l ${c_opts} -g $port > tst_netload.log 2>&1
		ret=$?
		tst_rhost_run -c "pkill -9 netstress\$"

		if [ "$expect_ret" -ne 0 ]; then
			if [ $((ret & expect_ret)) -ne 0 ]; then
				tst_res_ TPASS "netstress failed as expected"
			else
				tst_res_ TFAIL "expected '$expect_res' but ret: '$ret'"
			fi
			return $ret
		fi

		if [ "$ret" -ne 0 ]; then
			[ $((ret & 32)) -ne 0 ] && \
				tst_netload_brk TCONF "not supported configuration"

			[ $((ret & 3)) -ne 0 -a $was_failure -gt 0 ] && \
				tst_netload_brk TFAIL "expected '$expect_res' but ret: '$ret'"

			tst_res_ TWARN "netstress failed, ret: $ret"
			was_failure=1
			continue
		fi

		[ ! -f $rfile ] && \
			tst_netload_brk TFAIL "can't read $rfile"

		results="$results $(cat $rfile)"
		passed=$((passed + 1))
	done

	if [ "$ret" -ne 0 ]; then
		[ $((ret & 4)) -ne 0 ] && \
			tst_res_ TWARN "netstress has warnings"
		tst_netload_brk TFAIL "expected '$expect_res' but ret: '$ret'"
	fi

	local median=$(tst_get_median $results)
	echo "$median" > $rfile

	tst_res_ TPASS "netstress passed, median time $median ms, data:$results"

	return $ret
}

# Compares results for netload runs.
# tst_netload_compare TIME_BASE TIME THRESHOLD_LOW [THRESHOLD_HI]
# TIME_BASE: time taken to run netstress load test - 100%
# TIME: time that is compared to the base one
# THRESHOD_LOW: lower limit for TFAIL
# THRESHOD_HIGH: upper limit for TWARN
#
# Slow performance can be ignored with setting environment variable
# LTP_NET_FEATURES_IGNORE_PERFORMANCE_FAILURE=1
tst_netload_compare()
{
	local base_time=$1
	local new_time=$2
	local threshold_low=$3
	local threshold_hi=$4
	local ttype='TFAIL'
	local msg res

	if [ -z "$base_time" -o -z "$new_time" -o -z "$threshold_low" ]; then
		tst_brk_ TBROK "tst_netload_compare: invalid argument(s)"
	fi

	res=$(((base_time - new_time) * 100 / base_time))
	msg="performance result is ${res}%"

	if [ "$res" -lt "$threshold_low" ]; then
		if [ "$LTP_NET_FEATURES_IGNORE_PERFORMANCE_FAILURE" = 1 ]; then
			ttype='TINFO';
			tst_res_ TINFO "WARNING: slow performance is not treated as error due LTP_NET_FEATURES_IGNORE_PERFORMANCE_FAILURE=1"
		else
			tst_res_ TINFO "Following slow performance can be ignored with LTP_NET_FEATURES_IGNORE_PERFORMANCE_FAILURE=1"
		fi
		tst_res_ $ttype "$msg < threshold ${threshold_low}%"
		return
	fi

	[ "$threshold_hi" ] && [ "$res" -gt "$threshold_hi" ] && \
		tst_res_ TWARN "$msg > threshold ${threshold_hi}%"

	tst_res_ TPASS "$msg, in range [${threshold_low}:${threshold_hi}]%"
}

. tst_net.sh
