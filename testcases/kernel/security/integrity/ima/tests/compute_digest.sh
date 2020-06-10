# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (c) 2009 IBM Corporation
# Copyright (c) 2018-2020 Petr Vorel <pvorel@suse.cz>
# Author: Mimi Zohar <zohar@linux.ibm.com>

# TODO: find support for rmd128 rmd256 rmd320 wp256 wp384 tgr128 tgr160
compute_digest()
{
	local algorithm="$1"
	local file="$2"
	local digest

	digest="$(${algorithm}sum $file 2>/dev/null | cut -f1 -d ' ')"
	if [ -n "$digest" ]; then
		echo "$digest"
		return 0
	fi

	digest="$(openssl $algorithm $file 2>/dev/null | cut -f2 -d ' ')"
	if [ -n "$digest" ]; then
		echo "$digest"
		return 0
	fi

	# uncommon ciphers
	local arg="$algorithm"
	case "$algorithm" in
	tgr192) arg="tiger" ;;
	wp512) arg="whirlpool" ;;
	esac

	digest="$(rdigest --$arg $file 2>/dev/null | cut -f1 -d ' ')"
	if [ -n "$digest" ]; then
		echo "$digest"
		return 0
	fi
	return 1
}
