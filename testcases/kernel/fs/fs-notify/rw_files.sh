#!/bin/bash

TDIR=$1
TIMEO=$2

[ -z "$TDIR" ] && TDIR=/tmp/
[ -d $TDIR ] || TDIR=/tmp/
[ -z "$TIMEO" ] && TIMEO=1m

[ ! -d $TDIR ] && exit

function add_files()
{
	local i=0

	while true ; do

		touch f-$i
		ln -s f-$i f-$i-sym
		ln f-$i f-$i-hdl

		mkdir d-$i

		mknod c-$i c 1 2
		mknod b-$i b 1 2
		mknod p-$i p

		((i++))
		[ -e stoptest ] && { echo $FUNCNAME; exit; }
	done
}

function mv_files()
{
	local i=0

	while true ; do

		mv -f f-$i f-$i-rename
		mv -f f-$i-rename f-$i
		((i++))
		[ -e stoptest ] && { echo $FUNCNAME; exit; }
	done
}

function read_files()
{
	while true ; do

		find .
		cat f-*
		ls d-*
		[ -e stoptest ] && { echo $FUNCNAME; exit; }
	done
}

function write_files()
{
	while true ; do

		for j in f-* d-* c-* b-* p-* ; do
			echo 1 > $j
			echo 2 >> $j
			[ -e stoptest ] && { echo $FUNCNAME; exit; }
		done
		[ -e stoptest ] && { echo $FUNCNAME; exit; }
	done
}

function rm_files()
{
	while true ; do

		rm -rf d-* f-* c-* b-* p-*
		sleep 3
		[ -e stoptest ] && { echo $FUNCNAME; exit; }
	done
}

pushd $TDIR > /dev/null 2>&1
rm -f stoptest
add_files > /dev/null 2>&1 &
mv_files > /dev/null 2>&1 &
read_files > /dev/null 2>&1 &
write_files > /dev/null 2>&1 &
rm_files > /dev/null 2>&1 &
popd > /dev/null 2>&1

sleep $TIMEO
touch $TDIR/stoptest
