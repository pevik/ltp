#!/bin/bash

export TIMEOUT=10

STRESSES="fanotify_flush_stress fanotify_init_stress \
fanotify_mark_stress fanotify_watch inotify_watch \
inotify_add_rm_stress inotify_init_stress inotify_inmodify_stress"
IO_OPES="writefile freadfile fwritefile readfile"

NR_CPUS=$(grep -c processor /proc/cpuinfo)
[ $NR_CPUS -lt 4 ] && NR_CPUS=4

function cleanup_processes()
{
	while ps -eo pid,comm | \
		grep -q -E "fanotify|inotify|rw_files|readfile|writefile" ; do
		killall rw_files.sh $STRESSES $IO_OPES > /dev/null 2>&1
		sleep 1
	done
}

function cleanup()
{
	sleep 3 # waiting possible unfinished processes
	cleanup_processes
	rm -f $STRESSES $IO_OPES fanotify.log inotify.log tmp
	cd /
	sync
}

function run_stress()
{
	local i j rcnt=0
	echo -e "* CPU count: $NR_CPUS"
	echo -e "* TIMEOUT for each subcase: $TIMEOUT"

	echo -e "* Starting fsnotify stress on directory and regular file"
	touch $TMPDIR/testfile
	>tmp
	for i in $STRESSES $IO_OPES rw_files.sh; do
		for j in $STRESSES ; do
			[ "$i" == "$j" ] && continue
			# skip duplicate combinations
			grep -w $j tmp | grep -qw $i && continue
			echo -e "* Starting $i and $j, rcnt $rcnt"
			./$i $TMPDIR $TIMEOUT > /dev/null 2>&1 &
			./$i $TMPDIR/testfile $TIMEOUT > /dev/null 2>&1 &
			./$j $TMPDIR $TIMEOUT > /dev/null 2>&1 &
			./$j $TMPDIR/testfile $TIMEOUT > /dev/null 2>&1 &
			sleep $TIMEOUT
			cleanup_processes
			echo -e "$i $j" >> tmp
			((rcnt++))
		done
	done
}

trap "cleanup; exit;" 2

echo "***** Starting tests *****"
run_stress

echo -e "\n***** Cleanup fanotify inotify device *****"
cleanup
