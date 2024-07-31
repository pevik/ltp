#!/bin/sh

tst_script_name=$(basename $0)

if [ -z "$LTP_IPC_PATH" ]; then
	echo "This script has to be executed from a LTP loader!"
	exit 1
fi

tst_brk_()
{
	tst_res_ "$@"

	case "$3" in
		"TBROK") exit 2;;
		*) exit 0;;
	esac
}

alias tst_res="tst_res_ $tst_script_name \$LINENO"
alias tst_brk="tst_brk_ $tst_script_name \$LINENO"
