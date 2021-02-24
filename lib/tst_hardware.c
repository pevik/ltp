// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020-2021 Cyril Hrubis <chrubis@suse.cz>
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>

#include "tst_hardware.h"
#include "tst_hwconf.h"

#define TST_NO_DEFAULT_MAIN
#include "tst_test.h"

static struct tst_hwconf *cur_conf;
static char *reconfigure_path;
static struct tst_hwconf *conf_root;
static unsigned int conf_cnt;

static int new_hwconf(char *buf, size_t start_off, size_t end_off)
{
	struct tst_hwconf *conf = malloc(sizeof(struct tst_hwconf));

	if (!conf) {
		tst_res(TWARN, "malloc() failed!");
		return 0;
	}

	memset(conf, 0, sizeof(*conf));

	conf->json.json = buf + start_off;
	conf->json.len = end_off - start_off;

	struct tst_json_val val = {
		.buf = conf->uid,
		.buf_size = sizeof(conf->uid)
	};

	TST_JSON_OBJ_FOREACH(&conf->json, &val) {
		switch (val.type) {
		case TST_JSON_STR:
			if (!strcmp(val.id, "uid"))
				goto done;
		break;
		case TST_JSON_ARR:
			tst_json_arr_skip(&conf->json);
		break;
		case TST_JSON_OBJ:
			tst_json_obj_skip(&conf->json);
		break;
		default:
		break;
		}
	}

	free(conf);

	return 1;
done:
	tst_json_reset(&conf->json);

	if (!cur_conf)
		conf_root = conf;
	else
		cur_conf->next = conf;

	cur_conf = conf;
	conf_cnt++;

	tst_res(TINFO, "Added hardware configuration UID='%s'", conf->uid);

	return 0;
}

static void parse_devices(struct tst_json_buf *json)
{
	struct tst_json_val val = {};
	size_t start_off;

	TST_JSON_ARR_FOREACH(json, &val) {
		switch (val.type) {
		case TST_JSON_OBJ:
			start_off = json->sub_off;
			tst_json_obj_skip(json);
			if (new_hwconf(json->buf, start_off, json->off))
				tst_json_err(json, "Missing 'uid' in hwconf entry!");
		break;
		case TST_JSON_ARR:
		case TST_JSON_INT:
		case TST_JSON_STR:
			tst_json_err(json, "Invalid record in hwconf list!");
		break;
		case TST_JSON_VOID:
		break;
		}
	}
}

static void save_reconfigure(const char *reconfigure, const char *ltproot)
{
	if (reconfigure[0] == '/') {
		reconfigure_path = strdup(reconfigure);
		if (!reconfigure_path)
			tst_brk(TBROK, "strdup() failed");
	} else {
		if (asprintf(&reconfigure_path, "%s/%s", ltproot, reconfigure) < 0)
			tst_brk(TBROK, "asprintf() failed");
	}
}

unsigned int tst_hwlist_cnt(void)
{
	return conf_cnt;
}

static void reconfigure(void)
{
	if (!reconfigure_path)
		return;

	if (!cur_conf)
		return;

	const char *const argv[] = {reconfigure_path, cur_conf->uid, NULL};

	tst_res(TINFO, "Running reconfigure '%s %s'", reconfigure_path, cur_conf->uid);

	tst_cmd(argv, NULL, NULL, TST_CMD_TCONF_ON_MISSING);
}

void tst_hwlist_reset(void)
{
	cur_conf = conf_root;

	reconfigure();
}

void tst_hwlist_next(void)
{
	cur_conf = cur_conf->next;

	reconfigure();
}

struct tst_json_buf *tst_hwconf_get(void)
{
	if (!cur_conf)
		return NULL;

	return &cur_conf->json;
}

unsigned int tst_hwlist_discover(const char *hardware_uid)
{
	const char *ltproot = getenv("LTPROOT");
	const char *hardware_discovery = getenv("HARDWARE_DISCOVERY");
	char buf[2048];

	if (!hardware_discovery) {
		if (!ltproot) {
			tst_res(TCONF, "No LTPROOT nor HARDWARE_DISCOVERY set!");
			return 0;
		}

		snprintf(buf, sizeof(buf), "%s/hardware-discovery.sh", ltproot);

		hardware_discovery = buf;
	}

	const char *const argv[] = {hardware_discovery, hardware_uid, NULL};

	tst_res(TINFO, "Executing '%s %s'", hardware_discovery, hardware_uid);

	//TODO: read the data from a pipe instead
	unlink("/tmp/hwlist.json");
	tst_cmd(argv, "/tmp/hwlist.json", NULL, TST_CMD_TCONF_ON_MISSING);

	struct tst_json_buf *json = tst_json_load("/tmp/hwlist.json");

	if (!json)
		tst_brk(TBROK, "Failed to load JSON hardware description");

	if (tst_json_start(json) != TST_JSON_OBJ)
		tst_brk(TBROK, "JSON hardware description must start with object!");

	struct tst_json_val val = {.buf = buf, .buf_size = sizeof(buf)};

	TST_JSON_OBJ_FOREACH(json, &val) {
		switch (val.type) {
		case TST_JSON_STR:
			if (!strcmp(val.id, "reconfigure"))
				save_reconfigure(val.val_str, ltproot);
			else
				tst_json_err(json, "Invalid object attr id='%s'", val.id);
		break;
		case TST_JSON_INT:
		case TST_JSON_OBJ:
			tst_json_err(json, "Invalid object attr id='%s'", val.id);
		break;
		case TST_JSON_ARR:
			if (!strcmp(val.id, "hwconfs"))
				parse_devices(json);
			else
				tst_json_err(json, "Invalid array attr id='%s'", val.id);
		break;
		case TST_JSON_VOID:
		break;
		}
	}

	if (tst_json_is_err(json)) {
		tst_json_err_print(stderr, json);
		tst_brk(TBROK, "Failed to parse JSON hardware description");
	}

	return conf_cnt;
}
