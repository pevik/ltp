# IMA + Kexec Tests

Since these tests cannot be run by the usual LTP machinery (runltp, etc)
because the machine you run these on will reboot several times, these
tests must be run standalone.

To run these tests, you must first copy over either the entire LTP repo, or
just the `testcases/kexec` directory.

You must supply a kernel image that will be passed to kexec, either
though the `IMAGE` environment variable, or placed at `testcases/kexec/Image`.

If the kernel is built to require signed kernel images, you must supply
a signed image.

Currently, the only arch that has support in the upstream kernel to pass
the IMA buffer through kexec is powerpc. A patchset that adds support for
IMA buffer passing through kexec on aarch64 is in the process of being
upstreamed. Therefore, these tests will not let you run them on
architectures other than powerpc or aarch64.

Running
-------
- kexec cmdline measurement
    1. `IMAGE=<path to kernel image> testcases/kexec/cmdline.sh start`
    2. Runtime logs will be emitted in `testcases/kexec/kexec_cmdline.log`.

- kexec ima buffer passing
    1. `IMAGE=<path to kernel image> testcases/kexec/ima_buffer.sh start`
    2. Runtime logs will be emitted in `testcases/kexec/kexec_ima_buffer.log`.
