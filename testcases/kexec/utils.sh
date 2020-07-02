#!/bin/sh

install() {
    local arg="$1"

    if [ ! -d "/etc/init.d" ]; then
        mkdir /etc/init.d
    fi

    cat << EOF > /etc/init.d/resume-after-kexec
#!/bin/sh
# Run the test
IMAGE=$IMAGE $(realpath $0) $arg
EOF

    chmod +x /etc/init.d/resume-after-kexec
    update-rc.d resume-after-kexec defaults
}

must_be_root() {
    if [ "$EUID" != 0 ]; then
        echo "run this script as root"
        exit
    fi
}

# Since the IMA buffer passing through kexec is only
# implemented on powerpc right now with support for aarch64 in review,
# only support running this test on powerpc or aarch64 systems.
on_correct_machine() {
    case "$(uname -m)" in
        ppc|ppc64|ppcle|ppc64le|aarch64)
            ;;
        *)
            echo "must be on powerpc or aarch64 architecture"
            exit
            ;;
    esac
}

hex2text() {
    sed 's/\([0-9A-F]\{2\}\)/\\\\\\x\1/gI' | xargs printf
}

text2hex() {
    hexdump -v -e '1/1 "%02x"'
}
