# LTP Network Tests

## Single Host Configuration

It is a default configuration (if the `RHOST` environment variable is not
defined). LTP adds `ltp_ns` network namespace and auto-configure `veth` pair
according to LTP network environment variables.

## Two Host Configuration

This setup requires the `RHOST` environment variable to be set properly and
configured SSH access to a remote host.

The 'RHOST' variable name must be set to the hostname of the server
(test management link) and `PASSWD` should be set to the root password
of the remote server.

Some of the network stress tests hasn't been ported to network API and still
use `rsh` via `LTP_RSH` environment variable. To workaround this is best to set
it to SSH, in order to run these tests with RSH following setup is needed:

* Edit the `/root/.rhosts` file. Please note that the file may not exist,
so you must create one if it does not. You must add the fully qualified
hostname of the machine you are testing on to this file. By adding the test
machine's hostname to this file, you will be allowing the machine to rsh to itself,
as root, without the requirement of a password.

```sh
echo $client_hostname >> /root/.rhosts
```

You may need to re-label `.rhost` file to make sure rlogind will have access to it:

```sh
/sbin/restorecon -v /root/.rhosts
```

* Add rlogin, rsh, rexec into `/etc/securetty` file:

```sh
for i in rlogin rsh rexec; do echo $i >> /etc/securetty; done
```

## Server Services Configuration
Verify that the below daemon services are running. If not, please install
and start them:
dhcp-server, dnsmasq, http-server, nfs-kernel-server, rpcbind, rsync, vsftpd

RSH based testing requires also:
rsh-server, telnet-server, finger-server, rdist

Note: If any of the above daemon is not running on server, the test related to
that service running from client will fail.

### FTP setup
* In `/etc/ftpusers` (or `/etc/vsftpd.ftpusers`), comment the line containing
"root" string. This file lists all those users who are not given access to do ftp
on the current system.

* If you don’t want to do the previous step, put following entry into `/root/.netrc`:
```
machine <remote_server_name>
login root
password <remote_root_password>
```
Otherwise, `ftp`, `rlogin` and `telnet` tests fails for `root` user.

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
* TEST_VARS - non-default network parameters
* OPTIONS - test group(s), use `-h` to see available ones.
