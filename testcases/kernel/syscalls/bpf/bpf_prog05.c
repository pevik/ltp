// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2021 SUSE LLC <rpalethorpe@suse.com>
 */

/*\
 * [Description]
 *
 * Compare the effects of 32-bit div/mod by zero with the expected
 * behaviour.
 *
 * The commit bpf: fix subprog verifier bypass by div/mod by 0
 * exception, changed div/mod by zero from exiting the current
 * program to setting the destination register to zero (div) or
 * leaving it untouched (mod).
 *
 * This solved one verfier bug which allowed dodgy pointer values, but
 * it turned out that the source register was being 32-bit truncated
 * when it should not be. Also the destination register for mod was
 * not being truncated when it should be.
 *
 * So then we have the following two fixes:
 * - bpf: Fix 32 bit src register truncation on div/mod
 * - bpf: Fix truncation handling for mod32 dst reg wrt zero
 *
 * Testing for all of these issues is a problem. Not least because
 * division by zero is undefined, so in theory any result is
 * acceptable so long as the verifier and runtime behaviour
 * match.
 *
 * However to keep things simple we just check if the source and
 * destination register runtime values match the current upstream
 * behaviour at the time of writing.
 *
 * If the test fails you may have one or more of the above patches
 * missing. In this case it is possible that you are not vulnerable
 * depending on what other backports and fixes have been applied. If
 * upstream changes the behaviour of division by zero, then the test
 * will need updating.
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "config.h"
#include "tst_test.h"
#include "tst_taint.h"
#include "tst_capability.h"
#include "lapi/socket.h"
#include "lapi/bpf.h"
#include "bpf_common.h"

#define BUFSIZE 8192

static const char MSG[] = "Ahoj!";
static char *msg;

static uint32_t *key;
static uint64_t *val;
static char *log;
static union bpf_attr *attr;

static int load_prog(int fd)
{
	struct bpf_insn insn[] = {
		BPF_MOV64_IMM(BPF_REG_6, 1), 			/* r6 = 1 */
		BPF_ALU64_IMM(BPF_LSH, BPF_REG_6, 32),		/* r6 <<= 32 */
		BPF_MOV64_IMM(BPF_REG_7, -1LL),			/* r7 = -1 */
		BPF_ALU32_REG(BPF_DIV, BPF_REG_7, BPF_REG_6),	/* w7 /= w6 */

		BPF_LD_MAP_FD(BPF_REG_1, fd),
		BPF_MOV64_REG(BPF_REG_2, BPF_REG_10),   /* r2 = fp */
		BPF_ALU64_IMM(BPF_ADD, BPF_REG_2, -4),  /* r2 = r2 - 4 */
		BPF_ST_MEM(BPF_W, BPF_REG_2, 0, 0),    /* *r2 = 0 */
		BPF_EMIT_CALL(BPF_FUNC_map_lookup_elem),
		BPF_JMP_IMM(BPF_JNE, BPF_REG_0, 0, 1),
		BPF_EXIT_INSN(),
		BPF_STX_MEM(BPF_DW, BPF_REG_0, BPF_REG_6, 0),

		BPF_LD_MAP_FD(BPF_REG_1, fd),
		BPF_MOV64_REG(BPF_REG_2, BPF_REG_10),
		BPF_ALU64_IMM(BPF_ADD, BPF_REG_2, -4),
		BPF_ST_MEM(BPF_W, BPF_REG_2, 0, 1),    /* *r2 = 1 */
		BPF_EMIT_CALL(BPF_FUNC_map_lookup_elem),
		BPF_JMP_IMM(BPF_JNE, BPF_REG_0, 0, 1),
		BPF_EXIT_INSN(),
		BPF_STX_MEM(BPF_DW, BPF_REG_0, BPF_REG_7, 0),

		BPF_MOV64_IMM(BPF_REG_6, 1),
		BPF_ALU64_IMM(BPF_LSH, BPF_REG_6, 32),
		BPF_MOV64_IMM(BPF_REG_7, -1LL),
		BPF_ALU32_REG(BPF_MOD, BPF_REG_7, BPF_REG_6),	/* w7 %= w6 */

		BPF_LD_MAP_FD(BPF_REG_1, fd),
		BPF_MOV64_REG(BPF_REG_2, BPF_REG_10),
		BPF_ALU64_IMM(BPF_ADD, BPF_REG_2, -4),
		BPF_ST_MEM(BPF_W, BPF_REG_2, 0, 2),    /* *r2 = 2 */
		BPF_EMIT_CALL(BPF_FUNC_map_lookup_elem),
		BPF_JMP_IMM(BPF_JNE, BPF_REG_0, 0, 1),
		BPF_EXIT_INSN(),
		BPF_STX_MEM(BPF_DW, BPF_REG_0, BPF_REG_6, 0),

		BPF_LD_MAP_FD(BPF_REG_1, fd),
		BPF_MOV64_REG(BPF_REG_2, BPF_REG_10),
		BPF_ALU64_IMM(BPF_ADD, BPF_REG_2, -4),
		BPF_ST_MEM(BPF_W, BPF_REG_2, 0, 3),    /* *r2 = 3 */
		BPF_EMIT_CALL(BPF_FUNC_map_lookup_elem),
		BPF_JMP_IMM(BPF_JNE, BPF_REG_0, 0, 1),
		BPF_EXIT_INSN(),
		BPF_STX_MEM(BPF_DW, BPF_REG_0, BPF_REG_7, 0),

		BPF_MOV64_IMM(BPF_REG_0, 0),
		BPF_EXIT_INSN()
	};

	bpf_init_prog_attr(attr, insn, sizeof(insn), log, BUFSIZE);

	return bpf_load_prog(attr, log);
}

static void setup(void)
{
	rlimit_bump_memlock();
	memcpy(msg, MSG, sizeof(MSG));
}

static void run(void)
{
	int map_fd, prog_fd;
	int sk[2];

	memset(attr, 0, sizeof(*attr));
	attr->map_type = BPF_MAP_TYPE_ARRAY;
	attr->key_size = 4;
	attr->value_size = 8;
	attr->max_entries = 4;

	map_fd = bpf_map_create(attr);
	prog_fd = load_prog(map_fd);

	SAFE_SOCKETPAIR(AF_UNIX, SOCK_DGRAM, 0, sk);
	SAFE_SETSOCKOPT(sk[1], SOL_SOCKET, SO_ATTACH_BPF, &prog_fd,
			sizeof(prog_fd));

	SAFE_WRITE(1, sk[0], msg, sizeof(MSG));
	SAFE_CLOSE(sk[0]);
	SAFE_CLOSE(sk[1]);
	SAFE_CLOSE(prog_fd);

	tst_res(TINFO, "Check w7(-1) /= w6(0) [r7 = -1, r6 = 1 << 32]");
	memset(attr, 0, sizeof(*attr));
	attr->map_fd = map_fd;
	attr->key = ptr_to_u64(key);
	attr->value = ptr_to_u64(val);
	*key = 0;
	TST_EXP_PASS_SILENT(bpf(BPF_MAP_LOOKUP_ELEM, attr, sizeof(*attr)));
	if (TST_RET)
		goto out;

	if (*val != 1UL << 32) {
		tst_res(TFAIL,
			"src(r6) = %"PRIu64", but should be %"PRIu64,
			*val, 1UL << 32);
	} else {
		tst_res(TPASS, "src(r6) = %"PRIu64, *val);
	}

	*key = 1;
	TST_EXP_PASS_SILENT(bpf(BPF_MAP_LOOKUP_ELEM, attr, sizeof(*attr)));
	if (TST_RET)
		goto out;

	if (*val)
		tst_res(TFAIL, "dst(r7) = %"PRIu64", but should be zero", *val);

	tst_res(TPASS, "dst(r7) = %"PRIu64, *val);

	tst_res(TINFO, "Check w7(-1) %%= w6(0) [r7 = -1, r6 = 1 << 32]");
	*key = 2;
	TST_EXP_PASS_SILENT(bpf(BPF_MAP_LOOKUP_ELEM, attr, sizeof(*attr)));
	if (TST_RET)
		goto out;

	if (*val != 1UL << 32) {
		tst_res(TFAIL,
			"src(r6) = %"PRIu64", but should be %"PRIu64,
			*val, 1UL << 32);
	} else {
		tst_res(TPASS, "src(r6) = %"PRIu64, *val);
	}

	*key = 3;
	TST_EXP_PASS_SILENT(bpf(BPF_MAP_LOOKUP_ELEM, attr, sizeof(*attr)));
	if (TST_RET)
		goto out;

	if (*val != (uint32_t)-1) {
		tst_res(TFAIL,
			"dst(r7) = %"PRIu64", but should be %"PRIu32,
			*val, (uint32_t)-1);
	} else {
		tst_res(TPASS, "dst(r7) = %"PRIu64, *val);
	}

out:
	SAFE_CLOSE(map_fd);
}

static struct tst_test test = {
	.setup = setup,
	.test_all = run,
	.min_kver = "3.18",
	.taint_check = TST_TAINT_W | TST_TAINT_D,
	.caps = (struct tst_cap []) {
		TST_CAP(TST_CAP_DROP, CAP_SYS_ADMIN),
		{}
	},
	.bufs = (struct tst_buffers []) {
		{&key, .size = sizeof(*key)},
		{&val, .size = sizeof(*val)},
		{&log, .size = BUFSIZE},
		{&attr, .size = sizeof(*attr)},
		{&msg, .size = sizeof(MSG)},
		{}
	},
	.tags = (const struct tst_tag[]) {
		{"linux-git", "f6b1b3bf0d5f"},
		{"linux-git", "468f6eafa6c4"},
		{"linux-git", "e88b2c6e5a4d"},
		{"linux-git", "9b00f1b78809"},
		{"CVE", "CVE-2021-3444"},
		{}
	}
};
