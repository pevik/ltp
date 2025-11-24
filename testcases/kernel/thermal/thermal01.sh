#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2025 Intel - http://www.intel.com/
#
# ---
# doc
# Tests the CPU package thermal sensor interface for Intel platforms.
#
# It works by checking the initial count of thermal interrupts. Then it
# decreases the threshold for sending a thermal interrupt to just above
# the current temperature and runs a workload on the CPU. Finally, it restores
# the original thermal threshold and checks whether the number of thermal
# interrupts increased.
# ---
#
# ---
# env
# {
#  "needs_root": true,
#  "supported_archs": ["x86", "x86_64"],
#  "needs_cmds": ["stress-ng"],
#  "min_runtime": 180
# }
# ---

. tst_loader.sh

tst_test()
{
	local thermal_zone_numbers=""
	local temp
	local temp_high=""
	local status=0

	local interrupt_array_init=$(awk -F'[^0-9]*' '/Thermal event interrupts/ {$1=$1;print}' /proc/interrupts)
	if [ $? -eq 0 ]; then
		tst_res TDEBUG "Initial values of thermal interrupt counters: $interrupt_array_init"
		local num=$(tst_getconf _NPROCESSORS_ONLN)
		tst_res TDEBUG "Number of logical cores: $num"
	else
		tst_brk TCONF "Thermal event interrupts is not found"
	fi

	# Below we check for the thermal_zone which uses x86_pkg_temp driver
	local thermal_zone_numbers=$(grep -l x86_pkg_temp /sys/class/thermal/thermal_zone*/type | xargs dirname)
	tst_res TINFO "x86_pkg_temp thermal zones: $thermal_zone_numbers"

	if [ -z $thermal_zone_numbers ]; then
		tst_brk TCONF "No x86_pkg_temp thermal zones found"
	fi
	for i in $thermal_zone_numbers; do
		tst_res TINFO "Currently testing x86_pkg_temp $i"
		local TEMP="$i/temp"
		local temp=$(cat "$TEMP")
		tst_res TDEBUG "$i's current temperature is $temp"
		case $temp in
		[0-9]*) ;;
		*)
			tst_brk TBROK "Unexpected zone temperature value $temp";;
		esac
		local trip=$(cat $i/trip_point_1_temp)
		# Setting trip_point_1_temp for $i to $temp + 10 (0.001°C)
		local temp_high=$(( temp + 10 ))
		echo "$temp_high" > $i/trip_point_1_temp
		local run_time=30
		local sleep_time=10
		while [ $sleep_time -gt 0 ]; do
			ROD stress-ng --matrix 0 --timeout $run_time --quiet
			local temp_cur=$(cat "$TEMP")
			tst_res TDEBUG "temp_cur: $temp_cur"
			[ $temp_cur -gt $temp_high ] && break
			tst_sleep $sleep_time
			run_time=$(( run_time - 3 ))
			sleep_time=$(( sleep_time - 1 ))
		done
		[ $temp_cur -gt $temp_high ] || tst_res TFAIL "Zone temperature is not rising as expected"

		# Restore the original trip_point_1_temp value
		echo "$trip" > $i/trip_point_1_temp

		# Check whether thermal interrupts count actually increased
		local interrupt_array_later=$(awk -F'[^0-9]*' '/Thermal event interrupts/ {$1=$1;print}' /proc/interrupts)
		tst_res TDEBUG "Current values of thermal interrupt counters: $interrupt_array_later"
		for j in $(seq 1 "$num"); do
			local interrupt_later=$(echo "$interrupt_array_later" | awk -v j=$j '{print $j}')
			local interrupt_init=$(echo "$interrupt_array_init" | awk -v j=$j '{print $j}')
			if [ $interrupt_later -le $interrupt_init ]; then
				status=1
			fi
		done
	done

	if [ $status -eq 0 ]; then
		tst_res TPASS "x86 package thermal interrupt triggered"
	else
		tst_res TFAIL "x86 package thermal interrupt did not trigger"
	fi
}

. tst_run.sh
