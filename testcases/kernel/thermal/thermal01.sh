#!/usr/bin/env bash
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

thermal_zone_numbers=""
temp=""
temp_high=""
status=0

tst_test()
{
	line=$(grep "Thermal event interrupts" /proc/interrupts)
	if [ $? -eq 0 ]; then
		interrupt_array_init=$(echo "$line" | tr -d "a-zA-Z:" | awk '{$1=$1;print}')
		tst_res TDEBUG "Initial values of thermal interrupt counters: $interrupt_array_init"
		num=$(nproc)
		tst_res TDEBUG "Number of logical cores: $num"
	else
		tst_brk TCONF "Thermal event interrupts is not found."
	fi

	# Below we check for the thermal_zone which uses x86_pkg_temp driver
	thermal_zone_numbers=$(grep -l x86_pkg_temp /sys/class/thermal/thermal_zone*/type | sed 's/[^0-9]//g' | tr -t '\n' ' ')
	tst_res TINFO "x86_pkg_temp thermal zones: $thermal_zone_numbers"

	if [ -z $thermal_zone_numbers ]; then
		tst_brk TCONF "No x86_pkg_temp thermal zones found"
	fi
	for i in $thermal_zone_numbers; do
		tst_res TINFO "Currently testing x86_pkg_temp thermal_zone$i"
		TEMP=/sys/class/thermal/thermal_zone$i/temp
		temp=$(cat "$TEMP")
		tst_res TDEBUG "thermal_zone$i current temperature is $temp"
		case $temp in
		[0-9]*) ;;
		*)
			tst_brk TBROK "Unexpected zone temperature value $temp";;
		esac
		trip=$(cat /sys/class/thermal/thermal_zone$i/trip_point_1_temp)
		# Setting trip_point_1_temp for termal_zone$i to $temp + 10 (0.001°C)
		temp_high=$(( temp + 10 ))
		echo $temp_high > /sys/class/thermal/thermal_zone$i/trip_point_1_temp
		run_time=30
		sleep_time=10
		while [ $sleep_time -gt 0 ]; do
			stress-ng --matrix 0 --timeout $run_time --quiet
			temp_cur=$(cat "$TEMP")
			tst_res TDEBUG "temp_cur: $temp_cur"
			[ $temp_cur -gt $temp_high ] && break
			sleep $sleep_time
			run_time=$(( run_time - 3 ))
			sleep_time=$(( sleep_time - 1 ))
		done
		[ $temp_cur -gt $temp_high ] || tst_res TFAIL "Zone temperature is not rising as expected"

		# Restore the original trip_point_1_temp value
		echo $trip > /sys/class/thermal/thermal_zone$i/trip_point_1_temp

		# Check whether thermal interrupts count actually increased
		interrupt_array_later=$(grep "Thermal event interrupts" /proc/interrupts | \
			tr -d "a-zA-Z:" | awk '{$1=$1;print}')
		tst_res TDEBUG "Current values of thermal interrupt counters: $interrupt_array_later"
		for j in $(seq 1 "$num"); do
			interrupt_later=$(echo "$interrupt_array_later" | cut -d " " -f  "$j")
			interrupt_init=$(echo "$interrupt_array_init" | cut -d " " -f  "$j")
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
