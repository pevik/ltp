# LTP Network Tests

Both single and two host configurations support debugging via
`TST_NET_RHOST_RUN_DEBUG=1` environment variable.

## Single Host Configuration

It is the default configuration (if the `RHOST` environment variable is not
defined). LTP adds `ltp_ns` network namespace and auto-configure `veth` pair
according to LTP network environment variables.

## Two Host Configuration

This setup requires the `RHOST` environment variable to be set properly and
configured SSH access to a remote host.

The `RHOST` variable must be set to the hostname of the server (test management
link) and public key setup or login without password is required.

Some of the network stress tests which hasn't been ported to network API were
designed to be tested with `rsh` via `LTP_RSH` environment variable. Now it's
by default used `ssh`, for details see `testcases/network/stress/README`.

## Server Services Configuration
Verify that the below daemon services are running. If not, please install
and start them:

dhcp-server, dnsmasq, http-server, nfs-kernel-server, rpcbind, telnet-server, vsftpd

Note: If any of the above daemon is not running on server, the test related to
that service running from client will fail.

### FTP and telnet setup
Both tests require environment variables `RHOST` (remote machine), `RUSER`
(remote user) and `PASSWD` (remote password). NOTE: `RHOST` will imply two host
configuratioe for other tests.

If `RHOST` is set to `root`, either of these steps is required:

* In `/etc/ftpusers` (or `/etc/vsftpd.ftpusers`), comment the line containing
"root" string. This file lists all those users who are not given access to do ftp
on the current system.

* If you don’t want to do the previous step, put following entry into `/root/.netrc`:
```
machine <remote_server_name>
login root
password <remote_root_password>
```

## LTP setup
Install LTP testsuite. In case of two hosts configuration, LTP needs to be installed
and `LTPROOT` and `PATH` environment variables set on both client and server
machines (assuming using the default prefix `/opt/ltp`):

```sh
export LTPROOT="/opt/ltp"; export PATH="$LTPROOT/testcases/bin:$PATH"
```
Default values for all LTP network parameters are set in `testcases/lib/tst_net.sh`.
Network stress parameters are documented in `testcases/network/stress/README`.

## Running the tests

```sh
TEST_VARS ./network.sh OPTIONS
```
Where
* `TEST_VARS` - non-default network parameters
* `OPTIONS` - test group(s), use `-h` to see available ones.
