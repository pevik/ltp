// SPDX-License-Identifier: GPL-2.0-or-later
/*\
 * Verify splice-family functions (and sendfile) generate IN_ACCESS
 * for what they read and IN_MODIFY for what they write.
 *
 * Regression test for 983652c69199 and
 * https://lore.kernel.org/linux-fsdevel/jbyihkyk5dtaohdwjyivambb2gffyjs3dodpofafnkkunxq7bu@jngkdxx65pux/t/#u
 */

#define _GNU_SOURCE
#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdbool.h>
#include <inttypes.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/sendfile.h>

#include "tst_test.h"
#include "tst_safe_macros.h"
#include "inotify.h"

#if defined(HAVE_SYS_INOTIFY_H)
#include <sys/inotify.h>


static int pipes[2] = {-1, -1};
static int inotify = -1;
static int memfd = -1;
static int data_pipes[2] = {-1, -1};

static void watch_rw(int fd) {
  char buf[64];
  sprintf(buf, "/proc/self/fd/%d", fd);
  SAFE_MYINOTIFY_ADD_WATCH(inotify, buf, IN_ACCESS | IN_MODIFY);
}

static int compar(const void *l, const void *r) {
  const struct inotify_event *lie = l;
  const struct inotify_event *rie = r;
  return lie->wd - rie->wd;
}

static void get_events(size_t evcnt, struct inotify_event evs[static evcnt]) {
  struct inotify_event tail, *itr = evs;
  for (size_t left = evcnt; left; --left)
    SAFE_READ(true, inotify, itr++, sizeof(struct inotify_event));

  TEST(read(inotify, &tail, sizeof(struct inotify_event)));
  if (TST_RET != -1)
    tst_brk(TFAIL, "expect %zu events", evcnt);
  if (TST_ERR != EAGAIN)
    tst_brk(TFAIL | TTERRNO, "expected EAGAIN");

  qsort(evs, evcnt, sizeof(struct inotify_event), compar);
}

static void expect_event(struct inotify_event *ev, int wd, uint32_t mask) {
  if (ev->wd != wd)
    tst_brk(TFAIL, "expect event for wd %d got %d", wd, ev->wd);
  if (ev->mask != mask)
    tst_brk(TFAIL, "expect event with mask %" PRIu32 " got %" PRIu32 "", mask,
            ev->mask);
}

#define F2P(splice)                                                            \
  SAFE_WRITE(SAFE_WRITE_RETRY, memfd, __func__, sizeof(__func__));             \
  SAFE_LSEEK(memfd, 0, SEEK_SET);                                              \
  watch_rw(memfd);                                                             \
  watch_rw(pipes[0]);                                                          \
  TEST(splice);                                                                \
  if (TST_RET == -1)                                                           \
    tst_brk(TBROK | TERRNO, #splice);                                          \
  if (TST_RET != sizeof(__func__))                                             \
    tst_brk(TBROK, #splice ": %" PRId64 "", TST_RET);                          \
                                                                               \
  /*expecting: IN_ACCESS memfd, IN_MODIFY pipes[0]*/                           \
  struct inotify_event events[2];                                              \
  get_events(ARRAY_SIZE(events), events);                                      \
  expect_event(events + 0, 1, IN_ACCESS);                                      \
  expect_event(events + 1, 2, IN_MODIFY);                                      \
                                                                               \
  char buf[sizeof(__func__)];                                                  \
  SAFE_READ(true, pipes[0], buf, sizeof(__func__));                            \
  if (memcmp(buf, __func__, sizeof(__func__)))                                 \
    tst_brk(TFAIL, "buf contents bad");
static void splice_file_to_pipe(void) {
  F2P(splice(memfd, NULL, pipes[1], NULL, 128 * 1024 * 1024, 0));
}
static void sendfile_file_to_pipe(void) {
  F2P(sendfile(pipes[1], memfd, NULL, 128 * 1024 * 1024));
}

static void splice_pipe_to_file(void) {
  SAFE_WRITE(SAFE_WRITE_RETRY, pipes[1], __func__, sizeof(__func__));
  watch_rw(pipes[0]);
  watch_rw(memfd);
  TEST(splice(pipes[0], NULL, memfd, NULL, 128 * 1024 * 1024, 0));
  if(TST_RET == -1)
		tst_brk(TBROK | TERRNO, "splice");
	if(TST_RET != sizeof(__func__))
		tst_brk(TBROK, "splice: %" PRId64 "", TST_RET);

	// expecting: IN_ACCESS pipes[0], IN_MODIFY memfd
	struct inotify_event events[2];
	get_events(ARRAY_SIZE(events), events);
	expect_event(events + 0, 1, IN_ACCESS);
	expect_event(events + 1, 2, IN_MODIFY);

  char buf[sizeof(__func__)];
  SAFE_LSEEK(memfd, 0, SEEK_SET);
  SAFE_READ(true, memfd, buf, sizeof(__func__));
  if (memcmp(buf, __func__, sizeof(__func__)))
                tst_brk(TFAIL, "buf contents bad");
}

#define P2P(splice)                                                            \
  SAFE_WRITE(SAFE_WRITE_RETRY, data_pipes[1], __func__, sizeof(__func__));     \
  watch_rw(data_pipes[0]);                                                     \
  watch_rw(pipes[1]);                                                          \
  TEST(splice);                                                                \
  if (TST_RET == -1)                                                           \
                tst_brk(TBROK | TERRNO, #splice);                              \
  if (TST_RET != sizeof(__func__))                                             \
                tst_brk(TBROK, #splice ": %" PRId64 "", TST_RET);              \
                                                                               \
  /* expecting: IN_ACCESS data_pipes[0], IN_MODIFY pipes[1] */                 \
  struct inotify_event events[2];                                              \
  get_events(ARRAY_SIZE(events), events);                                      \
  expect_event(events + 0, 1, IN_ACCESS);                                      \
  expect_event(events + 1, 2, IN_MODIFY);                                      \
                                                                               \
  char buf[sizeof(__func__)];                                                  \
  SAFE_READ(true, pipes[0], buf, sizeof(__func__));                            \
  if (memcmp(buf, __func__, sizeof(__func__)))                                 \
                tst_brk(TFAIL, "buf contents bad");
static void splice_pipe_to_pipe(void) {
  P2P(splice(data_pipes[0], NULL, pipes[1], NULL, 128 * 1024 * 1024, 0));
}
static void tee_pipe_to_pipe(void) {
  P2P(tee(data_pipes[0], pipes[1], 128 * 1024 * 1024, 0));
}

static char vmsplice_pipe_to_mem_dt[32 * 1024];
static void vmsplice_pipe_to_mem(void) {
  memcpy(vmsplice_pipe_to_mem_dt, __func__, sizeof(__func__));
  watch_rw(pipes[0]);
  TEST(vmsplice(pipes[1],
                &(struct iovec){.iov_base = vmsplice_pipe_to_mem_dt,
                                .iov_len = sizeof(vmsplice_pipe_to_mem_dt)},
                1, SPLICE_F_GIFT));
  if (TST_RET == -1)
    tst_brk(TBROK | TERRNO, "vmsplice");
  if (TST_RET != sizeof(vmsplice_pipe_to_mem_dt))
    tst_brk(TBROK, "vmsplice: %" PRId64 "", TST_RET);

  // expecting: IN_MODIFY pipes[0]
  struct inotify_event event;
  get_events(1, &event);
  expect_event(&event, 1, IN_MODIFY);

  char buf[sizeof(__func__)];
  SAFE_READ(true, pipes[0], buf, sizeof(__func__));
  if (memcmp(buf, __func__, sizeof(__func__)))
    tst_brk(TFAIL, "buf contents bad");
}

static void vmsplice_mem_to_pipe(void) {
  char buf[sizeof(__func__)];
  SAFE_WRITE(SAFE_WRITE_RETRY, pipes[1], __func__, sizeof(__func__));
  watch_rw(pipes[1]);
  TEST(vmsplice(pipes[0],
                &(struct iovec){.iov_base = buf, .iov_len = sizeof(buf)}, 1,
                0));
  if (TST_RET == -1)
    tst_brk(TBROK | TERRNO, "vmsplice");
  if (TST_RET != sizeof(buf))
    tst_brk(TBROK, "vmsplice: %" PRId64 "", TST_RET);

  // expecting: IN_ACCESS pipes[1]
  struct inotify_event event;
  get_events(1, &event);
  expect_event(&event, 1, IN_ACCESS);
  if (memcmp(buf, __func__, sizeof(__func__)))
    tst_brk(TFAIL, "buf contents bad");
}

typedef void (*tests_f)(void);
#define TEST_F(f) { f, #f }
static const struct {
        tests_f f;
        const char *n;
} tests[] = {
    TEST_F(splice_file_to_pipe),  TEST_F(sendfile_file_to_pipe),
    TEST_F(splice_pipe_to_file),  TEST_F(splice_pipe_to_pipe),
    TEST_F(tee_pipe_to_pipe),     TEST_F(vmsplice_pipe_to_mem),
    TEST_F(vmsplice_mem_to_pipe),
};

static void run_test(unsigned int n)
{
	tst_res(TINFO, "%s", tests[n].n);

	SAFE_PIPE2(pipes, O_CLOEXEC);
	SAFE_PIPE2(data_pipes, O_CLOEXEC);
	inotify = SAFE_MYINOTIFY_INIT1(IN_NONBLOCK | IN_CLOEXEC);
	if((memfd = memfd_create(__func__, MFD_CLOEXEC)) == -1)
		tst_brk(TCONF | TERRNO, "memfd");
	tests[n].f();
	tst_res(TPASS, "ок");
}

static void cleanup(void)
{
	if (memfd != -1)
		SAFE_CLOSE(memfd);
	if (inotify != -1)
		SAFE_CLOSE(inotify);
	if (pipes[0] != -1)
		SAFE_CLOSE(pipes[0]);
	if (pipes[1] != -1)
		SAFE_CLOSE(pipes[1]);
	if (data_pipes[0] != -1)
		SAFE_CLOSE(data_pipes[0]);
	if (data_pipes[1] != -1)
		SAFE_CLOSE(data_pipes[1]);
}

static struct tst_test test = {
	.max_runtime = 10,
	.cleanup = cleanup,
	.test = run_test,
	.tcnt = ARRAY_SIZE(tests),
	.tags = (const struct tst_tag[]) {
		{"linux-git", "983652c69199"},
		{}
	},
};

#else
	TST_TEST_TCONF("system doesn't have required inotify support");
#endif
