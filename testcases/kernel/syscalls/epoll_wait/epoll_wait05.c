// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 SUSE LLC Andrea Cervesato <andrea.cervesato@suse.com>
 */

/*\
 * [Description]
 *
 * Verify that epoll receives EPOLLRDHUP event when we hang a reading
 * half-socket we are polling on.
 */

#define _GNU_SOURCE
#include "tst_test.h"
#include "tst_epoll.h"

static int epfd;
static in_port_t *sock_port;

static void create_server(void)
{
	int sockfd;
	socklen_t len;
	struct sockaddr_in serv_addr;
	struct sockaddr_in sin;

	memset(&serv_addr, 0, sizeof(struct sockaddr_in));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = 0;

	sockfd = SAFE_SOCKET(AF_INET, SOCK_STREAM, 0);
	SAFE_BIND(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	SAFE_LISTEN(sockfd, 10);

	len = sizeof(sin);
	memset(&sin, 0, sizeof(struct sockaddr_in));
	SAFE_GETSOCKNAME(sockfd, (struct sockaddr *)&sin, &len);

	*sock_port = sin.sin_port;

	tst_res(TINFO, "Listening on port %d", *sock_port);

	TST_CHECKPOINT_WAKE_AND_WAIT(0);

	SAFE_CLOSE(sockfd);
}

static void run(void)
{
	int sockfd;
	struct sockaddr_in client_addr;
	struct epoll_event evt_req;
	struct epoll_event evt_rec;

	if (!SAFE_FORK()) {
		create_server();
		return;
	}

	TST_CHECKPOINT_WAIT(0);

	tst_res(TINFO, "Connecting to port %d", *sock_port);

	sockfd = SAFE_SOCKET(AF_INET, SOCK_STREAM, 0);

	memset(&client_addr, 0, sizeof(struct sockaddr));
	client_addr.sin_family = AF_INET;
	client_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	client_addr.sin_port = *sock_port;

	SAFE_CONNECT(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr));

	tst_res(TINFO, "Polling on socket");

	epfd = SAFE_EPOLL_CREATE1(0);
	evt_req.events = EPOLLRDHUP;
	SAFE_EPOLL_CTL(epfd, EPOLL_CTL_ADD, sockfd, &evt_req);

	tst_res(TINFO, "Hang socket");

	TST_EXP_PASS_SILENT(shutdown(sockfd, SHUT_RD));
	SAFE_EPOLL_WAIT(epfd, &evt_rec, 1, 2000);

	if (evt_rec.events & EPOLLRDHUP)
		tst_res(TPASS, "Received EPOLLRDHUP");
	else
		tst_res(TFAIL, "EPOLLRDHUP has not been received");

	SAFE_CLOSE(epfd);
	SAFE_CLOSE(sockfd);

	TST_CHECKPOINT_WAKE(0);
}

static void setup(void)
{
	sock_port = SAFE_MMAP(NULL, sizeof(in_port_t), PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS, -1, 0);
}

static void cleanup(void)
{
	if (sock_port)
		SAFE_MUNMAP(sock_port, sizeof(in_port_t));

	if (epfd > 0)
		SAFE_CLOSE(epfd);
}

static struct tst_test test = {
	.setup = setup,
	.cleanup = cleanup,
	.test_all = run,
	.forks_child = 1,
	.needs_checkpoints = 1,
};
