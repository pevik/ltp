.. SPDX-License-Identifier: GPL-2.0-or-later
.. Copyright (c) Linux Test Project, 2025

Safe macros reference
=====================

LTP provides ``SAFE_*`` wrappers for common system calls and library
functions. Each wrapper calls the underlying function and, on failure,
invokes ``tst_brk(TBROK | TERRNO, ...)`` to abort the test. This
eliminates repetitive error-checking boilerplate.

**When NOT to use SAFE_\* macros:** when the syscall is the *subject*
of the test (e.g. testing ``close()`` error paths). Use ``TEST()`` or
``TST_EXP_*`` macros instead.

Core macros (tst_safe_macros.h)
-------------------------------

File and directory operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

- ``SAFE_ACCESS(path, mode)``
- ``SAFE_CHMOD(path, mode)``
- ``SAFE_CHOWN(path, owner, group)``
- ``SAFE_CHDIR(path)``
- ``SAFE_CHROOT(path)``
- ``SAFE_CREAT(path, mode)``
- ``SAFE_FCHMOD(fd, mode)``
- ``SAFE_FCHOWN(fd, owner, group)``
- ``SAFE_GETCWD(buf, size)``
- ``SAFE_LCHOWN(path, owner, group)``
- ``SAFE_LINK(oldpath, newpath)``
- ``SAFE_LINKAT(olddirfd, oldpath, newdirfd, newpath, flags)``
- ``SAFE_MKDIR(path, mode)``
- ``SAFE_MKFIFO(path, mode)``
- ``SAFE_MKNOD(path, mode, dev)``
- ``SAFE_READLINK(path, buf, bufsiz)``
- ``SAFE_RENAME(oldpath, newpath)``
- ``SAFE_RMDIR(path)``
- ``SAFE_SYMLINK(target, linkpath)``
- ``SAFE_SYMLINKAT(target, newdirfd, linkpath)``
- ``SAFE_UNLINK(path)``

File descriptor operations
~~~~~~~~~~~~~~~~~~~~~~~~~~

- ``SAFE_CLOSE(fd)`` — also sets ``fd = -1``
- ``SAFE_DUP(oldfd)``
- ``SAFE_DUP2(oldfd, newfd)``
- ``SAFE_FCNTL(fd, cmd, ...)``
- ``SAFE_FSYNC(fd)``
- ``SAFE_OPEN(path, oflag, ...)``
- ``SAFE_OPENDIR(name)``
- ``SAFE_CLOSEDIR(dirp)``
- ``SAFE_READDIR(dirp)``
- ``SAFE_PIPE(pipefd)``
- ``SAFE_PIPE2(pipefd, flags)``
- ``SAFE_PTSNAME(fd)``

Reading and writing
~~~~~~~~~~~~~~~~~~~

- ``SAFE_READ(len_strict, fd, buf, count)``
- ``SAFE_READV(fd, iov, iovcnt)``
- ``SAFE_WRITE(len_strict, fd, buf, count)``
- ``SAFE_WRITEV(fd, iov, iovcnt)``

Memory management
~~~~~~~~~~~~~~~~~

- ``SAFE_CALLOC(nmemb, size)``
- ``SAFE_MALLOC(size)``
- ``SAFE_MEMALIGN(alignment, size)``
- ``SAFE_REALLOC(ptr, size)``
- ``SAFE_MMAP(addr, length, prot, flags, fd, offset)``
- ``SAFE_MUNMAP(addr, length)``
- ``SAFE_MLOCK(addr, len)``
- ``SAFE_MUNLOCK(addr, len)``
- ``SAFE_MPROTECT(addr, len, prot)``
- ``SAFE_MSYNC(addr, length, flags)``
- ``SAFE_MINCORE(addr, length, vec)``

Process management
~~~~~~~~~~~~~~~~~~

- ``SAFE_EXECL(path, arg, ...)``
- ``SAFE_EXECLP(file, arg, ...)``
- ``SAFE_EXECVP(file, argv)``
- ``SAFE_KILL(pid, sig)``
- ``SAFE_WAIT(status)``
- ``SAFE_WAITPID(pid, status, opts)``
- ``SAFE_GETPGID(pid)``
- ``SAFE_SETPGID(pid, pgid)``
- ``SAFE_SETSID()``
- ``SAFE_PRCTL(option, ...)``
- ``SAFE_PTRACE(request, pid, addr, data)``
- ``SAFE_PIDFD_OPEN(pid, flags)``

User and group
~~~~~~~~~~~~~~

- ``SAFE_GETPWNAM(name)``
- ``SAFE_GETGRNAM(name)``
- ``SAFE_GETGRNAM_FALLBACK(name)``
- ``SAFE_GETGRGID(gid)``
- ``SAFE_GETGROUPS(size, list)``
- ``SAFE_SETGROUPS(size, list)``
- ``SAFE_SETUID(uid)``
- ``SAFE_SETEUID(euid)``
- ``SAFE_SETREUID(ruid, euid)``
- ``SAFE_SETRESUID(ruid, euid, suid)``
- ``SAFE_GETRESUID(ruid, euid, suid)``
- ``SAFE_SETGID(gid)``
- ``SAFE_SETEGID(egid)``
- ``SAFE_SETREGID(rgid, egid)``
- ``SAFE_SETRESGID(rgid, egid, sgid)``
- ``SAFE_GETRESGID(rgid, egid, sgid)``

Signals
~~~~~~~

- ``SAFE_SIGACTION(signum, act, oldact)``
- ``SAFE_SIGADDSET(set, signum)``
- ``SAFE_SIGDELSET(set, signum)``
- ``SAFE_SIGEMPTYSET(set)``
- ``SAFE_SIGFILLSET(set)``
- ``SAFE_SIGNAL(signum, handler)``
- ``SAFE_SIGPROCMASK(how, set, oldset)``
- ``SAFE_SIGWAIT(set, sig)``

Extended attributes
~~~~~~~~~~~~~~~~~~~

- ``SAFE_GETXATTR(path, name, value, size)``
- ``SAFE_SETXATTR(path, name, value, size, flags)``
- ``SAFE_REMOVEXATTR(path, name)``
- ``SAFE_LSETXATTR(path, name, value, size, flags)``
- ``SAFE_LREMOVEXATTR(path, name)``
- ``SAFE_FSETXATTR(fd, name, value, size, flags)``
- ``SAFE_FREMOVEXATTR(fd, name)``

Mounting
~~~~~~~~

- ``SAFE_MOUNT(source, target, filesystemtype, mountflags, data)``
- ``SAFE_MOUNT2(source, target, filesystemtype, mountflags, data)``
- ``SAFE_UMOUNT(target)``

Miscellaneous
~~~~~~~~~~~~~

- ``SAFE_BASENAME(path)``
- ``SAFE_CMD(argv, stdout_path, stderr_path)``
- ``SAFE_DIRNAME(path)``
- ``SAFE_GETPRIORITY(which, who)``
- ``SAFE_GETRUSAGE(who, usage)``
- ``SAFE_IOCTL(fd, request, ...)``
- ``SAFE_PERSONALITY(persona)``
- ``SAFE_SETENV(name, value, overwrite)``
- ``SAFE_SETNS(fd, nstype)``
- ``SAFE_SSCANF(str, format, ...)``
- ``SAFE_STATVFS(path, buf)``
- ``SAFE_SYSCONF(name)``
- ``SAFE_SYSINFO(info)``
- ``SAFE_UNSHARE(flags)``

String conversion
~~~~~~~~~~~~~~~~~

- ``SAFE_STRTOL(str, min, max)``
- ``SAFE_STRTOUL(str, min, max)``
- ``SAFE_STRTOF(str, min, max)``

Inline macros (tst_safe_macros_inline.h)
----------------------------------------

- ``SAFE_FSTAT(fd, statbuf)``
- ``SAFE_FTRUNCATE(fd, length)``
- ``SAFE_GETRLIMIT(resource, rlim)``
- ``SAFE_LSEEK(fd, offset, whence)``
- ``SAFE_LSTAT(path, statbuf)``
- ``SAFE_MMAP(addr, length, prot, flags, fd, offset)``
- ``SAFE_POSIX_FADVISE(fd, offset, len, advice)``
- ``SAFE_SETRLIMIT(resource, rlim)``
- ``SAFE_STAT(path, statbuf)``
- ``SAFE_STATFS(path, buf)``
- ``SAFE_TRUNCATE(path, length)``

File content operations (tst_safe_file_ops.h)
----------------------------------------------

- ``SAFE_FILE_SCANF(path, fmt, ...)``
- ``SAFE_FILE_LINES_SCANF(path, fmt, ...)``
- ``SAFE_FILE_PRINTF(path, fmt, ...)``
- ``SAFE_TRY_FILE_PRINTF(path, fmt, ...)``
- ``SAFE_FILE_READ_STR(path, buf)``
- ``SAFE_READ_MEMINFO(item)``
- ``SAFE_READ_PROC_STATUS(pid, item)``
- ``SAFE_CP(src, dst)``
- ``SAFE_TOUCH(path, mode, times)``
- ``SAFE_MOUNT_OVERLAY()``

File-at operations (tst_safe_file_at.h)
----------------------------------------

- ``SAFE_OPENAT(dirfd, path, oflag, ...)``
- ``SAFE_FILE_READAT(dirfd, path, buf, bufsiz)``
- ``SAFE_FILE_PRINTFAT(dirfd, path, fmt, ...)``
- ``SAFE_UNLINKAT(dirfd, path, flags)``
- ``SAFE_FCHOWNAT(dirfd, path, owner, group, flags)``
- ``SAFE_FSTATAT(dirfd, path, statbuf, flags)``

Positioned I/O (tst_safe_prw.h)
---------------------------------

- ``SAFE_PREAD(len_strict, fd, buf, count, offset)``
- ``SAFE_PREADV(fd, iov, iovcnt, offset)``
- ``SAFE_PWRITE(len_strict, fd, buf, count, offset)``
- ``SAFE_PWRITEV(fd, iov, iovcnt, offset)``

Networking (tst_safe_net.h)
----------------------------

- ``SAFE_SOCKET(domain, type, protocol)``
- ``SAFE_SOCKETPAIR(domain, type, protocol, sv)``
- ``SAFE_BIND(sockfd, addr, addrlen)``
- ``SAFE_CONNECT(sockfd, addr, addrlen)``
- ``SAFE_LISTEN(sockfd, backlog)``
- ``SAFE_ACCEPT(sockfd, addr, addrlen)``
- ``SAFE_SEND(sockfd, buf, len, flags)``
- ``SAFE_SENDTO(sockfd, buf, len, flags, dest_addr, addrlen)``
- ``SAFE_SENDMSG(sockfd, msg, flags)``
- ``SAFE_RECV(sockfd, buf, len, flags)``
- ``SAFE_RECVMSG(sockfd, msg, flags)``
- ``SAFE_GETSOCKNAME(sockfd, addr, addrlen)``
- ``SAFE_GETSOCKOPT(sockfd, level, optname, optval, optlen)``
- ``SAFE_SETSOCKOPT(sockfd, level, optname, optval, optlen)``
- ``SAFE_SETSOCKOPT_INT(sockfd, level, optname, value)``
- ``SAFE_GETHOSTNAME(name, len)``
- ``SAFE_SETHOSTNAME(name, len)``
- ``SAFE_GETADDRINFO(node, service, hints, res)``

Pthreads (tst_safe_pthread.h)
------------------------------

- ``SAFE_PTHREAD_CREATE(thread, attr, start_routine, arg)``
- ``SAFE_PTHREAD_JOIN(thread, retval)``
- ``SAFE_PTHREAD_CANCEL(thread)``
- ``SAFE_PTHREAD_KILL(thread, sig)``
- ``SAFE_PTHREAD_BARRIER_INIT(barrier, attr, count)``
- ``SAFE_PTHREAD_BARRIER_WAIT(barrier)``
- ``SAFE_PTHREAD_BARRIER_DESTROY(barrier)``
- ``SAFE_PTHREAD_MUTEX_INIT(mutex, attr)``
- ``SAFE_PTHREAD_MUTEX_LOCK(mutex)``
- ``SAFE_PTHREAD_MUTEX_TRYLOCK(mutex)``
- ``SAFE_PTHREAD_MUTEX_TIMEDLOCK(mutex, abstime)``
- ``SAFE_PTHREAD_MUTEX_UNLOCK(mutex)``
- ``SAFE_PTHREAD_MUTEX_DESTROY(mutex)``
- ``SAFE_PTHREAD_MUTEXATTR_INIT(attr)``
- ``SAFE_PTHREAD_MUTEXATTR_SETTYPE(attr, type)``
- ``SAFE_PTHREAD_MUTEXATTR_DESTROY(attr)``

Standard I/O (tst_safe_stdio.h)
--------------------------------

- ``SAFE_FOPEN(path, mode)``
- ``SAFE_FREOPEN(path, mode, stream)``
- ``SAFE_FCLOSE(stream)``
- ``SAFE_FREAD(ptr, size, nmemb, stream)``
- ``SAFE_FWRITE(ptr, size, nmemb, stream)``
- ``SAFE_FFLUSH(stream)``
- ``SAFE_FSEEK(stream, offset, whence)``
- ``SAFE_FTELL(stream)``
- ``SAFE_FILENO(stream)``
- ``SAFE_POPEN(command, type)``
- ``SAFE_ASPRINTF(strp, fmt, ...)``

SysV IPC (tst_safe_sysv_ipc.h)
--------------------------------

- ``SAFE_MSGGET(key, msgflg)``
- ``SAFE_MSGSND(msqid, msgp, msgsz, msgflg)``
- ``SAFE_MSGRCV(msqid, msgp, msgsz, msgtyp, msgflg)``
- ``SAFE_MSGCTL(msqid, cmd, buf)``
- ``SAFE_SEMGET(key, nsems, semflg)``
- ``SAFE_SEMOP(semid, sops, nsops)``
- ``SAFE_SEMCTL(semid, semnum, cmd, ...)``
- ``SAFE_SHMGET(key, size, shmflg)``
- ``SAFE_SHMAT(shmid, shmaddr, shmflg)``
- ``SAFE_SHMDT(shmaddr)``
- ``SAFE_SHMCTL(shmid, cmd, buf)``

POSIX IPC (tst_safe_posix_ipc.h)
----------------------------------

- ``SAFE_MQ_OPEN(name, oflag, ...)``
- ``SAFE_MQ_CLOSE(mqdes)``
- ``SAFE_MQ_SEND(mqdes, msg_ptr, msg_len, msg_prio)``
- ``SAFE_MQ_NOTIFY(mqdes, sevp)``
- ``SAFE_MQ_UNLINK(name)``

Clocks and timers (tst_safe_clocks.h)
--------------------------------------

- ``SAFE_CLOCK_GETRES(clk_id, res)``
- ``SAFE_CLOCK_GETTIME(clk_id, tp)``
- ``SAFE_CLOCK_SETTIME(clk_id, tp)``
- ``SAFE_CLOCK_NANOSLEEP(clk_id, flags, request, remain)``
- ``SAFE_TIMER_CREATE(clockid, sevp, timerid)``
- ``SAFE_TIMER_SETTIME(timerid, flags, new_value, old_value)``
- ``SAFE_TIMER_GETTIME(timerid, curr_value)``
- ``SAFE_TIMER_DELETE(timerid)``

Timer file descriptors (tst_safe_timerfd.h)
--------------------------------------------

- ``SAFE_TIMERFD_CREATE(clockid, flags)``
- ``SAFE_TIMERFD_SETTIME(fd, flags, new_value, old_value)``
- ``SAFE_TIMERFD_GETTIME(fd, curr_value)``

io_uring (tst_safe_io_uring.h)
-------------------------------

- ``SAFE_IO_URING_INIT(entries, params, ring)``
- ``SAFE_IO_URING_ENTER(fd, to_submit, min_complete, flags, sig)``
- ``SAFE_IO_URING_CLOSE(ring)``
