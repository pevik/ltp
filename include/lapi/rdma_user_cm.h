/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Copyright (c) 2026 Linux Test Project
 */

#ifndef LAPI_RDMA_USER_CM_H__
#define LAPI_RDMA_USER_CM_H__

#include "config.h"

#ifdef HAVE_RDMA_RDMA_USER_CM_H
# include <rdma/rdma_user_cm.h>
#endif

#include <netinet/in.h>
#include <stdint.h>

#ifndef RDMA_USER_CM_ABI_VERSION

enum {
	RDMA_USER_CM_CMD_CREATE_ID,
	RDMA_USER_CM_CMD_DESTROY_ID,
	RDMA_USER_CM_CMD_BIND_IP,
	RDMA_USER_CM_CMD_RESOLVE_IP,
	RDMA_USER_CM_CMD_RESOLVE_ROUTE,
	RDMA_USER_CM_CMD_QUERY_ROUTE,
	RDMA_USER_CM_CMD_CONNECT,
	RDMA_USER_CM_CMD_LISTEN,
};

enum rdma_ucm_port_space {
	RDMA_PS_IPOIB = 0x0002,
};

struct rdma_ucm_cmd_hdr {
	uint32_t cmd;
	uint16_t in;
	uint16_t out;
};

struct rdma_ucm_create_id {
	uint64_t uid;
	uint64_t response;
	uint16_t ps;
	uint8_t  qp_type;
	uint8_t  reserved[5];
};

struct rdma_ucm_create_id_resp {
	uint32_t id;
};

struct rdma_ucm_destroy_id {
	uint64_t response;
	uint32_t id;
	uint32_t reserved;
};

struct rdma_ucm_bind_ip {
	uint64_t response;
	struct sockaddr_in6 addr;
	uint32_t id;
};

struct rdma_ucm_listen {
	uint32_t id;
	uint32_t backlog;
};

#endif /* RDMA_USER_CM_ABI_VERSION */

#endif /* LAPI_RDMA_USER_CM_H__ */
