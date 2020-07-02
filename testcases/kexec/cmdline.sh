#!/bin/sh

ASCII_MEASUREMENTS="/sys/kernel/security/integrity/ima/ascii_runtime_measurements"
SCRIPT_DIR="$(dirname $(realpath $0))"
IMAGE=$(realpath "${IMAGE:-$SCRIPT_DIR/Image}")
LOG_FILE="$SCRIPT_DIR/kexec_cmdline.log"

. $SCRIPT_DIR/utils.sh

must_be_root
on_correct_machine

case $1 in
    start)
        # Start the state machine
        echo "$(cat /proc/cmdline | tr -d '\n' | text2hex)" > /etc/saved-kexec-cmdline

        install 1
        if ! kexec -s $IMAGE --reuse-cmdline; then
            echo "kexec failed: $?" >> $LOG_FILE
        fi
        ;;
    1)
        cmdline="$(cat /proc/cmdline | tr -d '\n')"
        printf "$cmdline" | text2hex | xargs echo >> /etc/saved-kexec-cmdline

        install 2
        if ! kexec -s $IMAGE --append="$cmdline"; then
            echo "kexec failed: $?" >> $LOG_FILE
        fi
        ;;
    2)
        cmdline="$(cat /proc/cmdline | tr -d '\n')"
        printf "$cmdline" | text2hex | xargs echo >> /etc/saved-kexec-cmdline

        install 3
        if ! kexec -s $IMAGE --command-line="$cmdline"; then
            echo "kexec failed: $?" >> $LOG_FILE
        fi
        ;;
    3)
        update-rc.d resume-after-kexec remove
        rm /etc/init.d/resume-after-kexec

        success=true

        grep "kexec-cmdline" $ASCII_MEASUREMENTS \
        | tail -n "$(wc -l /etc/saved-kexec-cmdline | cut -d' ' -f1)" \
        | paste -d'|' /etc/saved-kexec-cmdline - \
        | while IFS="|" read -r saved_cmdline logged_line
        do
            # saved_cmdline is encoded in hex
            algorithm=$(echo "$logged_line" | cut -d' ' -f4 | cut -d':' -f1)
            digest=$(echo "$logged_line" | cut -d' ' -f4 | cut -d':' -f2)
            logged_cmdline=$(echo "$logged_line" | cut -d' ' -f6)
            saved_digest=$(echo "$saved_cmdline" | hex2text | ${algorithm}sum | cut -d' ' -f1)

            if [ "$saved_cmdline" != "$logged_cmdline" ]; then
                echo "saved cmdline != logged cmdline" >> $LOG_FILE
                success=false
            fi

            if [ "$saved_digest" != "$digest" ]; then
                echo "computed digest != logged digest" >> $LOG_FILE
                success=false
            fi
        done

        if $success; then
            echo "test succeeded" >> $LOG_FILE
        else
            echo "test failed" >> $LOG_FILE
        fi
        ;;
    *)
        echo "You must run '$0 start' to begin the test"
        ;;
esac
