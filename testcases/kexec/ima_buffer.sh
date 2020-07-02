#!/bin/sh

ASCII_MEASUREMENTS="/sys/kernel/security/integrity/ima/ascii_runtime_measurements"
SCRIPT_DIR="$(dirname $(realpath $0))"
IMAGE=$(realpath "${IMAGE:-$SCRIPT_DIR/Image}")
LOG_FILE="$SCRIPT_DIR/kexec_ima_buffer.log"

. $SCRIPT_DIR/utils.sh

must_be_root
on_correct_machine

case $1 in
    start)
        # Start the state machine
        cp $ASCII_MEASUREMENTS /etc/saved-ima-buffer

        install 1
        if ! kexec -s $IMAGE --reuse-cmdline; then
            echo "kexec failed: $?" >> $LOG_FILE
        fi
        ;;
    1)
        update-rc.d resume-after-kexec remove
        rm /etc/init.d/resume-after-kexec

        n_lines=$(wc -l /etc/saved-ima-buffer | cut -d' ' -f1)
        if cat $ASCII_MEASUREMENTS | \
            head -n $n_lines | \
            cmp -s - /etc/saved-ima-buffer
        then
            echo "test succeeded" > $LOG_FILE
        else
            echo "test failed" > $LOG_FILE
        fi

        rm /etc/saved-ima-buffer
        ;;
    *)
        echo "You must run '$0 start' to begin the test"
        ;;
esac
