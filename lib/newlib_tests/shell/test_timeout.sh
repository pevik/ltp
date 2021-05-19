#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2019 Petr Vorel <pvorel@suse.cz>
# Copyright (c) 2021 Joerg Vehlow <joerg.vehlow@aox-tech.de>

PATH="$(dirname $0):$(dirname $0)/../../../testcases/lib/:$PATH"

SHELLS="
bash
dash
busybox sh
zsh
ash
ksh
"

# Test cases are separated by newlines.
# Every test has the following fields in this order:
# file
# timeout_mul
# use_cat
# max_runtime
# expected_exit_code
# expected passes
# expected failed
# expected broken
# description
# Whole lines can be commented out using "#"
DATA="
timeout01.sh|     |0|  |0
timeout02.sh|     |0|  |0
timeout02.sh|  foo|0|  |2
timeout02.sh|    2|0|  |0
timeout01.sh|    2|0|  |0
timeout02.sh|  1.1|0|  |0
timeout02.sh|  -10|0|  |2
timeout02.sh| -0.1|0|  |0
timeout02.sh| -1.1|0|  |2
timeout02.sh|-10.1|0|  |2
timeout03.sh|     |0|12|137| | | |Test kill if test does not terminate by SIGINT
timeout04.sh|     |0|  |  2|0|0|1|Verify that timeout is enforced
timeout02.sh|    2|1| 2|   |1|0|0|Test termination of timeout process
"

run_shell()
{
	local shell=$1
	shift

	eval $shell "$@"
	return $?
}

# Executes a test
# Parameter:
#  - shell:    The shell to be used while executing the test
#  - shellarg: First parameter of shell (e.g. for busybox sh)
#  - test:     The test to execute
#  - timeout:  The timeout multiplicator (optional)
#  - use_cat:  Pipe the output of the command through cat (optional)
#              If this is used, the exit code is NOT returned!
#
# The function returns the following global variables:
# - test_exit:     The exit code of the test
# - test_duration: The duration of the test in seconds
# - test_output:   The full output of the test
# - test_passed:   The number of passed tests parsed from the summary
# - test_failed:   The number of failed tests parsed from the summary
# - test_broken:   The number of broken tests parsed from the summary
exec_test()
{
	local shell=$1
	local shellarg=$2
	local test=$3
	local timeout=$4
	local use_cat=$5
	local tmpfile start end;

	tmpfile=$(mktemp -t ltp_timeout_XXXXXXXX)
	start=$(date +%s)
	# We have to use set monitor mode (set -m) here.
	# In most shells in most cases this creates a
	# new process group for the next command.
	# A process group is required for the timeout functionality,
	# because it sends signals to the whole process group.
	set -m
	# The use_cat is for testing if any programm using stdout
	# is still running, after the test finished.
	# cat will wait for its stdin to be closed.
	#
	# In the pure shell implementation of the timeout handling,
	# the sleep process was never killed, when a test finished
	# before the timeout.
	if [ "$use_cat" = "1" ]; then
		LTP_TIMEOUT_MUL=$timeout $shell $shellarg $test 2>&1 | cat >$tmpfile
	else
		LTP_TIMEOUT_MUL=$timeout $shell $shellarg $test 1>$tmpfile 2>&1
	fi
	test_exit=$?
	set +m
	end=$(date +%s)

	test_duration=$((end - start))
	test_output=$(cat $tmpfile)
	rm $tmpfile

	test_passed=-1
	test_failed=-1
	test_broken=-1
	eval $(echo "$test_output" | awk '
		BEGIN {sum=0}
		$1 == "Summary:" {
			sum=1;
		}
		sum && ( \
			   $1 == "passed" \
			|| $1 == "failed" \
			|| $1 == "broken") {
			print "test_" $1 "=" $2
		}
	')
}

do_test()
{
	local failed test_nr old_ifs shell CLEANED_DATA test_max
	local file cur_fails timeout use_cat max_runtime exp_exit
	local exp_passed exp_failed exp_broken description

	shell="$1"
	shellarg="$2"

	failed=0
	test_nr=0

	old_ifs="$IFS"
	IFS=$(printf "\n\b")

	# Remove comments and empty lines
	CLEANED_DATA=$(echo "$DATA" | sed '/^\s*#/d;/^\s*$/d')
	test_max=$(echo "$CLEANED_DATA" | wc -l)
	for d in $CLEANED_DATA; do
		IFS="$old_ifs"

		file=$(echo $d | cut -d'|' -f1 | xargs)
		timeout=$(echo $d | cut -d'|' -f2 | xargs)
		use_cat=$(echo $d | cut -d'|' -f3 | xargs)
		max_runtime=$(echo $d | cut -d'|' -f4 | xargs)
		max_runtime=${max_runtime:--1}
		exp_exit=$(echo $d | cut -d'|' -f5 | xargs)
		exp_exit=${exp_exit:--1}
		exp_passed=$(echo $d | cut -d'|' -f6 | xargs)
		exp_passed=${exp_passed:--1}
		exp_failed=$(echo $d | cut -d'|' -f7 | xargs)
		exp_failed=${exp_failed:--1}
		exp_broken=$(echo $d | cut -d'|' -f8 | xargs)
		exp_broken=${exp_broken:--1}
		description=$(echo $d | cut -d'|' -f9)

		test_nr=$(($test_nr + 1))

		cur_fails=0

		if [ -z "$description" ]; then
			description="$file (LTP_TIMEOUT_MUL='$timeout')"
		fi

		echo "=== $test_nr/$test_max $description ==="
		exec_test "$shell" "$shellarg" "$file" "$timeout" "$use_cat"

		if [ $max_runtime -ne -1 ] && [ $test_duration -gt $max_runtime ]; then
			echo "FAILED (runtime: $test_duration, expected less than $max_runtime)"
			cur_fails=$((cur_fails + 1))
		fi

		if [ $exp_passed -ne -1 ] && [ $exp_passed -ne $test_passed ]; then
			echo "FAILED (passes: $test_passed, expected $exp_passed)"
			cur_fails=$((cur_fails + 1))
		fi

		if [ $exp_failed -ne -1 ] && [ $exp_failed -ne $test_failed ]; then
			echo "FAILED (failed: $test_failed, expected $exp_failed)"
			cur_fails=$((cur_fails + 1))
		fi

		if [ $exp_broken -ne -1 ] && [ $exp_broken -ne $test_broken ]; then
			echo "FAILED (broken: $test_broken, expected $exp_broken)"
			cur_fails=$((cur_fails + 1))
		fi

		if [ $exp_exit -ne -1 ] && [ $test_exit -ne $exp_exit ]; then
			echo "FAILED (exit code: $test_exit, expected $exp_exit)"
			cur_fails=$((cur_fails + 1))
		fi

		if [ $cur_fails -gt 0 ]; then
			failed=$((failed + 1))
			echo "--------"
			echo "$test_output"
			echo "--------"
		else
			echo "PASSED"
		fi
		echo
	done
	IFS="$old_ifs"

	echo "Failed tests: $failed"
	return $failed
}


print_results()
{
	echo
	if [ -n "$raw_shell" ]; then
		result=$(printf "%s\n%-15s %s" "$result" "$raw_shell" "INTERRUPTED")
		failed_shells=$((failed_shells + 1))
	fi
	echo
	echo "----------------------------------------"
	echo
	echo "Summary:"
	echo "$result"
	echo
	if [ $failed_shells -ne 0 ]; then
		echo "A total number of $total_fails failed for $failed_shells shells"
	else
		echo "All tests passed"
	fi
}

# For some reason at least in zsh, it can happen, that the whole
# testrunner is killed, when the test result is piped through cat.
# If the test was aborted using CTRL^C or kill, the output can be ignored,
# otherwise these messages should never be visible.
trap 'echo; echo "Test unexpectedly killed by SIGINT."; print_results; exit 1' INT
trap 'echo; echo "Test unexpectedly killed by SIGTERM."; print_results; exit 1' TERM

old_ifs="$IFS"
IFS=$(printf "\n\b")

failed_shells=0
total_fails=0

# Remove comments and empty lines
CLEANED_SHELLS=$(echo "$SHELLS" | sed '/^\s*#/d;/^\s*$/d')
shell_max=$(echo "$CLEANED_SHELLS" | wc -l)
shell_nr=0
result=""
for raw_shell in $CLEANED_SHELLS; do
	shell_nr=$(( shell_nr + 1 ))
	echo "($shell_nr/$shell_max) Testing timeout in shell API with '$raw_shell'"
	shellarg=$(echo "$raw_shell" | cut -sd' ' -f2)
	shell=$(echo "$raw_shell" | cut -d' ' -f1)
	res="BROKEN"
	if ! $shell $shellarg -c true 2>/dev/null; then
		echo "SKIPED: Shell not found"
		res="SKIPED"
	else
		res="PASSED"
		do_test "$shell" "$shellarg"
		if [ $? -ne 0 ]; then
			res="FAILED ($?)"
			total_fails=$((total_fails + $?))
			failed_shells=$((failed_shells + 1))
		fi
	fi
	result=$(printf "%s\n%-15s %s" "$result" "$raw_shell" "$res")
done
raw_shell=""

print_results
exit $failed_shells
