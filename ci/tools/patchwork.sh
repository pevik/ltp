#!/bin/sh -ex
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Shell script that can be used to communicate with Patchwork via REST API.
# It has been mainly created for CI purposes, but it can be used in the shell
# by satisfing minimum requirements.
#
# Copyright (c) 2025 Andrea Cervesato <andrea.cervesato@suse.com>

PATCHWORK_URL="${PATCHWORK_URL:-https://patchwork.ozlabs.org}"
PATCHWORK_SINCE="${PATCHWORK_SINCE:-3600}"

command_exists() {
        command -v $1 >/dev/null 2>&1

        if [ $? -ne 0 ]; then
                echo "'$1' must be present in the system"
                exit 1
        fi
}

command_exists "curl"
command_exists "jq"

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

        if [ $? -ne 0 ]; then
                exit 1
        fi
}

get_patches() {
        local series_id="$1"

        curl -k -G "$PATCHWORK_URL/api/patches/" \
                --data "project=ltp" \
                --data "series=$series_id" |
                jq -r '.[] | "\(.id)"'

        if [ $? -ne 0 ]; then
                exit 1
        fi
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

        if [ $? -ne 0 ]; then
                exit 1
        fi
}

set_series_state() {
        local series_id="$1"
        local state="$2"

        get_patches "$series_id" | while IFS= read -r patch_id; do
                if [ -n "$patch_id" ]; then
                        set_patch_state "$patch_id" "$state"
                fi
        done
}

get_checks() {
        local patch_id="$1"

        curl -k -G "$PATCHWORK_URL/api/patches/$patch_id/checks/" |
                jq -r '.[] | "\(.id)"'

        if [ $? -ne 0 ]; then
                exit 1
        fi
}

already_tested() {
        local series_id="$1"

        get_patches "$series_id" | while IFS= read -r patch_id; do
                if [ ! -n "$patch_id" ]; then
                        continue
                fi

                get_checks "$patch_id" | while IFS= read -r check_id; do
                        if [ -n "$check_id" ]; then
                                echo "$check_id"
                                return
                        fi
                done
        done
}

verify_new_patches() {
        local output="series_ids.txt"

        echo -n '' >"$output"

        fetch_series | while IFS=: read -r series_id series_mbox; do
                if [ ! -n "$series_id" ]; then
                        continue
                fi

                tested=$(already_tested "$series_id")
                if [ -n "$tested" ]; then
                        continue
                fi

                echo "$series_id|$series_mbox" >>"$output"
        done

        cat "$output"
}

send_results() {
        local series_id="$1"
        local target_url="$2"

        verify_token_exists

        local context=$(echo "$3" | sed 's/:/_/g; s/\//-/g; s/\./-/g')
        if [ -n "$CC" ]; then
                context="${context}_${CC}"
        fi

        if [ -n "$ARCH" ]; then
                context="${context}_${ARCH}"
        fi

        local result="$4"
        if [ "$result" = "cancelled" ]; then
                return
        fi

        local state="fail"
        if [ "$result" = "success" ]; then
                state="success"
        fi

        get_patches "$series_id" | while IFS= read -r patch_id; do
                [ -z "$patch_id" ] || continue
                curl -k -X POST \
                    -H "Authorization: Token $PATCHWORK_TOKEN" \
                    -F "state=$state" \
                    -F "context=$context" \
                    -F "target_url=$target_url" \
                    -F "description=$result" \
                    "$PATCHWORK_URL/api/patches/$patch_id/checks/"

                if [ $? -ne 0 ]; then
                    exit 1
                fi
        done
}

run="$1"

if [ -z "$run" -o "$run" = "state" ]; then
        set_series_state "$2" "$3"
elif [ -z "$run" -o "$run" = "check" ]; then
        send_results "$2" "$3" "$4" "$5"
elif [ -z "$run" -o "$run" = "verify" ]; then
        verify_new_patches
else
        echo "Available commands: state, check, verify"
        exit 1
fi
