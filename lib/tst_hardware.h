// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020-2021 Cyril Hrubis <chrubis@suse.cz>
 */

/*
 * Hardware discovery code.
 *
 * Each hardware type is uniquely identified by an hardware unique id. Test
 * that needs a particular hardware sets the .needs_hardware field in the
 * tst_test structure which is then passed to the discovery code.
 *
 * The discovery launches a script/binary that returns a JSON in a format:
 *
 * {
 *  "reconfigure": "/path/to/reconfigure/script",
 *  "hwconfs": [
 *   {
 *     "uid": "device_unique_id01",
 *     ...
 *   },
 *   {
 *    "uid": "device_unique_id02",
 *    ...
 *   }
 *  ]
 * }
 *
 * The optional reconfigure script could be used to set up the hardware, e.g.
 * connect different serial ports together with relays, the only parameter
 * it takes is the device uid. E.g. to set up the testing environment for the
 * first device in the list above LTP will execute command:
 * '/path/to/reconfigure/script "device_unique_id01"' etc.
 *
 * The second parameter in the JSON is "hwconfs" array that describes all
 * available hardware configurations. These objects, apart from "uid" are not
 * interpreted by the test libary but rather passed down to the test one object
 * per test iteration.
 */

#ifndef TST_HARDWARE_H__
#define TST_HARDWARE_H__

#include "tst_json.h"

struct tst_hwconf {
	struct tst_hwconf *next;
	/* unique id for the hardware configuration */
	char uid[128];
	/* buffer with the JSON description */
	struct tst_json_buf json;
};

/*
 * Discovers a hardware based on hardware_uid.
 *
 * Exits the test with TBROK on error and with TCONF if no hardware was discovered.
 */
unsigned int tst_hwlist_discover(const char *hardware_uid);

/*
 * Returns number of parsed entries.
 */
unsigned int tst_hwlist_cnt(void);

/*
 * Resets current tst_hwconf to point to the first hardware entry.
 */
void tst_hwlist_reset(void);

/*
 * Allows to loop over all entries in the hareware list.
 *
 * If needed calls the reconfigure script.
 */
void tst_hwlist_next(void);

/*
 * Free the hwlist.
 */
void tst_hwlist_free(void);

#endif /* TST_HARDWARE_H__ */
