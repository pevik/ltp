#!/bin/sh

export PATH=$PATH:$PWD:$PWD/tests/

./tests/shell_test01
./tests/shell_test02
./tests/shell_test03
./tests/shell_test04
./tests/shell_test05
./tests/shell_test06
./tst_run_shell shell_loader.sh
./tst_run_shell shell_loader_all_filesystems.sh
