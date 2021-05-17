#!/bin/sh -eu
# Check the exported symbols from an object file or archive

obj_file=$1

non_prefix_syms() {
    nm $obj_file --defined-only -o -g -l | \
	awk -F ' ' "\$3 !~ /tst_|safe_|ltp_/ { print }"
}

ignore_list=$(tr -d '\n' <<EOF
parse_opts|
STD_LOOP_COUNT|
usc_global_setup_hook|
usc_test_looping|
parse_ranges|
random_bit|
random_range|
random_rangel|
random_rangell|
random_range_seed|
range_max|
range_min|
range_mult|
lio_check_asyncio|
lio_help1|
lio_help2|
Lio_info1|
Lio_info2|
lio_parse_io_arg1|
lio_parse_io_arg2|
lio_random_methods|
lio_read_buffer|
lio_set_debug|
Lio_SysCall|
lio_wait4asyncio|
lio_write_buffer|
stride_bounds|
TEST_ERRNO|
TEST_RETURN|
TST_RET_PTR|
TCID|
TST_ERR|
TST_PASS|
TST_RET
EOF
)

ignore_regex="\$3 !~ /$ignore_list/"
fmt_error_statement=$(cat <<"EOF"
{
  ("basename " $4) | getline cfile
  print "ERROR: lib symbol exported without tst_ or safe_: " cfile"::"$3
  failed = 1
}
END {
  if (failed == 1) {
      exit(1)
  }
}
EOF
)

non_prefix_syms | awk -F ' ' "$ignore_regex $fmt_error_statement"
