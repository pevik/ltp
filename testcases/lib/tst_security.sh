#!/bin/sh
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2016-2018 Petr Vorel <pvorel@suse.cz>

if [ -z "$TST_LIB_LOADED" ]; then
	echo "please load tst_test.sh first" >&2
	exit 1
fi

[ -n "$TST_SECURITY_LOADED" ] && return 0
TST_SECURITY_LOADED=1

# Detect whether AppArmor profiles are loaded
# Return 0: profiles loaded, 1: none profile loaded or AppArmor disabled
tst_apparmor_enabled()
{
	local f="/sys/kernel/security/apparmor/profiles"
	tst_test_cmds cut wc
	[ -f "$f" ] && [ "$(wc -l $f | cut -d' ' -f1)" -gt 0 ]
}

# Detect whether SELinux is enabled in enforcing mode
# Return 0: enabled in enforcing mode
# Return 1: enabled in permissive mode or disabled
tst_selinux_enabled()
{
	local f="$(_tst_get_enforce)"
	[ -f "$f" ] && [ "$(cat $f)" = "1" ]
}

# Try disable AppArmor
# Return 0: AppArmor disabled
# Return > 0: failed to disable AppArmor
tst_disable_apparmor()
{
	local f="aa-teardown"
	local action

	tst_cmd_available $f && { $f; return; }

	f="/etc/init.d/apparmor"
	if [ -f "$f" ]; then
		for action in teardown kill stop; do
			$f $action >/dev/null 2>&1 && return
		done
	fi
}

# Try disable SELinux
# Return 0: SELinux disabled
# Return > 0: failed to disable SELinux
tst_disable_selinux()
{
	local f="$(_tst_get_enforce)"
	[ -f "$f" ] && cat 0 > $f
}

_tst_get_enforce()
{
	local dir="/sys/fs/selinux"
	[ -d "$dir" ] || dir="/selinux"
	local f="$dir/enforce"
	[ -f "$f" ] && echo "$f"
}
