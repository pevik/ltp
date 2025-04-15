#!/bin/sh -x
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Shell script to communicate with Patchwork via REST API.
# It has been mainly created for CI purposes, but it can be used in the shell
# by satisfying minimum requirements.
#
# Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@suse.com>

PATCHWORK_URL="${PATCHWORK_URL:-https://patchwork.ozlabs.org}"
PATCHWORK_SINCE="${PATCHWORK_SINCE:-3600}"

command_exists() {
        for cmd in "$@"; do
                if ! command -v "$cmd" >/dev/null 2>&1; then
                        echo "'$1' must be present in the system" >&2
                        exit 1
                fi
        done
}

command_exists "curl" "jq"

fetch_series() {
        local current_time=$(date +%s)
        local since_time=$(expr $current_time - $PATCHWORK_SINCE)
        local date=$(date -u -d @$since_time +"%Y-%m-%dT%H:%M:%SZ")

        curl -k -G "$PATCHWORK_URL/api/events/" \
                --data "category=series-completed" \
                --data "project=ltp" \
                --data "state=new" \
                --data "since=$date" \
                --data "archive=no" |
                jq -r '.[] | "\(.payload.series.id):\(.payload.series.mbox)"'

        [ $? -eq 0 ] || exit 1
}

get_patches() {
        local series_id="$1"

        curl -k -G "$PATCHWORK_URL/api/patches/" \
                --data "project=ltp" \
                --data "series=$series_id" |
                jq -r '.[] | "\(.id)"'

        [ $? -eq 0 ] || exit 1
}

verify_token_exists() {
        if [ -z "$PATCHWORK_TOKEN" ]; then
                echo "For this feature you need \$PATCHWORK_TOKEN"
                exit 1
        fi
}

set_patch_state() {
        local patch_id="$1"
        local state="$2"

        verify_token_exists

        curl -k -X PATCH \
                -H "Authorization: Token $PATCHWORK_TOKEN" \
                -F "state=$state" \
                "$PATCHWORK_URL/api/patches/$patch_id/"

        [ $? -eq 0 ] || exit 1
}

set_series_state() {
        local series_id="$1"
        local state="$2"

        get_patches "$series_id" | while IFS= read -r patch_id; do
                [ "$patch_id" ] && set_patch_state "$patch_id" "$state"
        done
}

get_checks() {
        local patch_id="$1"

        curl -k -G "$PATCHWORK_URL/api/patches/$patch_id/checks/" |
                jq -r '.[] | "\(.id)"'

        [ $? -eq 0 ] || exit 1
}

already_tested() {
        local series_id="$1"

        get_patches "$series_id" | while IFS= read -r patch_id; do
                [ "$patch_id" ] || continue

                get_checks "$patch_id" | while IFS= read -r check_id; do
                        if [ -n "$check_id" ]; then
                                echo "$check_id"
                                return
                        fi
                done
        done
}

verify_new_patches() {
        local tmp=$(mktemp -d)
        local output="$tmp/series_ids.txt"

        echo -n '' >"$output"

        fetch_series | while IFS=: read -r series_id series_mbox; do
                [ "$series_id" ] || continue

                tested=$(already_tested "$series_id")
                [ "$tested" ] && continue

                echo "$series_id|$series_mbox" >>"$output"
        done

        cat "$output"
}

send_results() {
        local series_id="$1"
        local target_url="$2"

        verify_token_exists

        local context=$(echo "$3" | sed 's/:/_/g; s/\//-/g; s/\./-/g')

        [ "$CC" ] && context="${context}_${CC}"
        [ "$ARCH" ] && context="${context}_${ARCH}"

        local result="$4"
        [ "$result" == "cancelled" ] && return

        local state="fail"
        [ "$result" == "success" ] && state="success"

        get_patches "$series_id" | while IFS= read -r patch_id; do
                [ "$patch_id" ] || continue

                curl -k -X POST \
                        -H "Authorization: Token $PATCHWORK_TOKEN" \
                        -F "state=$state" \
                        -F "context=$context" \
                        -F "target_url=$target_url" \
                        -F "description=$result" \
                        "$PATCHWORK_URL/api/patches/$patch_id/checks/"

                [ $? -eq 0 ] && exit 1
        done
}

case "$1" in
state)
        set_series_state "$2" "$3"
        ;;
check)
        send_results "$2" "$3" "$4" "$5"
        ;;
verify)
        verify_new_patches
        ;;
*)
        echo "Available commands: state, check, verify"
        exit 1
        ;;
esac
