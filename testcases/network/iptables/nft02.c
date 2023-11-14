// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (C) 2023 SUSE LLC
 * Author: Marcos Paulo de Souza <mpdesouza@suse.com>
 * LTP port: Martin Doucha <mdoucha@suse.cz>
 */

/*\
 * CVE-2023-31248
 *
 * Test for use-after-free when adding a new rule to a chain deleted
 * in the same netlink message batch.
 *
 * Kernel bug fixed in:
 *
 *  commit 515ad530795c118f012539ed76d02bacfd426d89
 *  Author: Thadeu Lima de Souza Cascardo <cascardo@canonical.com>
 *  Date:   Wed Jul 5 09:12:55 2023 -0300
 *
 *  netfilter: nf_tables: do not ignore genmask when looking up chain by id
 */

#include <linux/netlink.h>
#include <linux/tcp.h>
#include <arpa/inet.h>
#include <linux/netfilter.h>
#include <linux/netfilter/nf_tables.h>
#include <linux/netfilter/nfnetlink.h>
#include "tst_test.h"
#include "tst_netlink.h"

#define TABNAME "ltp_table1"
#define CHAINNAME "ltp_chain1"

static const uint8_t rule_proto = IPPROTO_TCP;
static const uint32_t rule_dport = 80;

static uint32_t chain_id;
static uint32_t cmp_sreg;
static uint32_t cmp_op;
static struct tst_netlink_context *ctx;

/* Table creation config */
static const struct tst_netlink_attr_list table_config[] = {
	{NFTA_TABLE_NAME, TABNAME, strlen(TABNAME) + 1, NULL},
	{0, NULL, -1, NULL}
};

/* Chain creation and deletion config */
static const struct tst_netlink_attr_list newchain_config[] = {
	{NFTA_TABLE_NAME, TABNAME, strlen(TABNAME) + 1, NULL},
	{NFTA_CHAIN_NAME, CHAINNAME, strlen(CHAINNAME) + 1, NULL},
	{NFTA_CHAIN_ID, &chain_id, sizeof(chain_id), NULL},
	{0, NULL, -1, NULL}
};

static const struct tst_netlink_attr_list delchain_config[] = {
	{NFTA_TABLE_NAME, TABNAME, strlen(TABNAME) + 1, NULL},
	{NFTA_CHAIN_NAME, CHAINNAME, strlen(CHAINNAME) + 1, NULL},
	{0, NULL, -1, NULL}
};

/* Rule creation config */
static const struct tst_netlink_attr_list rule_data_config1[] = {
	{NFTA_CMP_SREG, &cmp_sreg, sizeof(cmp_sreg), NULL},
	{NFTA_CMP_OP, &cmp_op, sizeof(cmp_op), NULL},
	{NFTA_CMP_DATA, NULL, 0, (const struct tst_netlink_attr_list[]) {
		{NFTA_DATA_VALUE, &rule_proto, sizeof(rule_proto), NULL},
		{0, NULL, -1, NULL}
	}},
	{0, NULL, -1, NULL}
};

static const struct tst_netlink_attr_list rule_data_config2[] = {
	{NFTA_CMP_SREG, &cmp_sreg, sizeof(cmp_sreg), NULL},
	{NFTA_CMP_OP, &cmp_op, sizeof(cmp_op), NULL},
	{NFTA_CMP_DATA, NULL, 0, (const struct tst_netlink_attr_list[]) {
		{NFTA_DATA_VALUE, &rule_dport, sizeof(rule_dport), NULL},
		{0, NULL, -1, NULL}
	}},
	{0, NULL, -1, NULL}
};

static const struct tst_netlink_attr_list rule_expr_config[] = {
	{NFTA_LIST_ELEM, NULL, 0, (const struct tst_netlink_attr_list[]) {
		{NFTA_EXPR_NAME, "cmp", 4, NULL},
		{NFTA_EXPR_DATA, NULL, 0, rule_data_config1},
		{0, NULL, -1, NULL}
	}},
	{NFTA_LIST_ELEM, NULL, 0, (const struct tst_netlink_attr_list[]) {
		{NFTA_EXPR_NAME, "cmp", 4, NULL},
		{NFTA_EXPR_DATA, NULL, 0, rule_data_config2},
		{0, NULL, -1, NULL}
	}},
	{0, NULL, -1, NULL}
};

static const struct tst_netlink_attr_list rule_config[] = {
	{NFTA_RULE_EXPRESSIONS, NULL, 0, rule_expr_config},
	{NFTA_RULE_TABLE, TABNAME, strlen(TABNAME) + 1, NULL},
	{NFTA_RULE_CHAIN_ID, &chain_id, sizeof(chain_id), NULL},
	{0, NULL, -1, NULL}
};

static void setup(void)
{
	tst_setup_netns();

	chain_id = htonl(77);
	cmp_sreg = htonl(NFT_REG_1);
	cmp_op = htonl(NFT_CMP_EQ);
}

static void run(void)
{
	struct nlmsghdr header;
	struct nfgenmsg nfpayload;

	memset(&header, 0, sizeof(header));
	memset(&nfpayload, 0, sizeof(nfpayload));
	nfpayload.version = NFNETLINK_V0;

	ctx = NETLINK_CREATE_CONTEXT(NETLINK_NETFILTER);

	/* Start netfilter batch */
	header.nlmsg_type = NFNL_MSG_BATCH_BEGIN;
	header.nlmsg_flags = NLM_F_REQUEST;
	nfpayload.nfgen_family = AF_UNSPEC;
	nfpayload.res_id = htons(NFNL_SUBSYS_NFTABLES);
	NETLINK_ADD_MESSAGE(ctx, &header, &nfpayload, sizeof(nfpayload));

	/* Add table */
	header.nlmsg_type = (NFNL_SUBSYS_NFTABLES << 8) | NFT_MSG_NEWTABLE;
	header.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK;
	nfpayload.nfgen_family = NFPROTO_IPV4;
	nfpayload.res_id = htons(0);
	NETLINK_ADD_MESSAGE(ctx, &header, &nfpayload, sizeof(nfpayload));
	NETLINK_ADD_ATTR_LIST(ctx, table_config);

	/* Add chain */
	header.nlmsg_type = (NFNL_SUBSYS_NFTABLES << 8) | NFT_MSG_NEWCHAIN;
	header.nlmsg_flags = NLM_F_REQUEST | NLM_F_CREATE | NLM_F_ACK;
	nfpayload.nfgen_family = NFPROTO_IPV4;
	nfpayload.res_id = htons(0);
	NETLINK_ADD_MESSAGE(ctx, &header, &nfpayload, sizeof(nfpayload));
	NETLINK_ADD_ATTR_LIST(ctx, newchain_config);

	/* Delete the above chain */
	header.nlmsg_type = (NFNL_SUBSYS_NFTABLES << 8) | NFT_MSG_DELCHAIN;
	header.nlmsg_flags = NLM_F_REQUEST | NLM_F_ACK;
	nfpayload.nfgen_family = NFPROTO_IPV4;
	nfpayload.res_id = htons(0);
	NETLINK_ADD_MESSAGE(ctx, &header, &nfpayload, sizeof(nfpayload));
	NETLINK_ADD_ATTR_LIST(ctx, delchain_config);

	/* Add rule to deleted chain */
	header.nlmsg_type = (NFNL_SUBSYS_NFTABLES << 8) | NFT_MSG_NEWRULE;
	header.nlmsg_flags = NLM_F_REQUEST | NLM_F_APPEND | NLM_F_CREATE |
		NLM_F_ACK;
	nfpayload.nfgen_family = NFPROTO_IPV4;
	nfpayload.res_id = htons(0);
	NETLINK_ADD_MESSAGE(ctx, &header, &nfpayload, sizeof(nfpayload));
	NETLINK_ADD_ATTR_LIST(ctx, rule_config);

	/* End batch */
	header.nlmsg_type = NFNL_MSG_BATCH_END;
	header.nlmsg_flags = NLM_F_REQUEST;
	nfpayload.nfgen_family = AF_UNSPEC;
	nfpayload.res_id = htons(NFNL_SUBSYS_NFTABLES);
	NETLINK_ADD_MESSAGE(ctx, &header, &nfpayload, sizeof(nfpayload));

	NETLINK_SEND_VALIDATE(ctx);
	NETLINK_DESTROY_CONTEXT(ctx);
	ctx = NULL;

	/* Wait for kernel page fault error */
	usleep(100000);

	if (tst_taint_check()) {
		tst_res(TFAIL, "Kernel is vulnerable");
		return;
	}

	tst_res(TPASS, "Nothing bad happened, probably");
}

static void cleanup(void)
{
	NETLINK_DESTROY_CONTEXT(ctx);
}

static struct tst_test test = {
	.test_all = run,
	.setup = setup,
	.cleanup = cleanup,
	.taint_check = TST_TAINT_W | TST_TAINT_D,
	.needs_kconfigs = (const char *[]) {
		"CONFIG_USER_NS=y",
		"CONFIG_NF_TABLES",
		NULL
	},
	.save_restore = (const struct tst_path_val[]) {
		{"/proc/sys/user/max_user_namespaces", "1024", TST_SR_SKIP},
		{}
	},
	.tags = (const struct tst_tag[]) {
		{"linux-git", "515ad530795c"},
		{"CVE", "2023-31248"},
		{}
	}
};
