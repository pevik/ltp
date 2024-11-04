#!/bin/sh
#
# ---
# doc
#
# [Description]
#
# This is an example how to run C child from shell.
# ---
#
# ---
# env
# {
# }
# ---

. tst_loader.sh

if [ -n "LTP_IPC_PATH" ]; then
	tst_res TPASS "LTP_IPC_PATH=$LTP_IPC_PATH!"
fi

tst_res TINFO "Running C child"
shell_c_child
