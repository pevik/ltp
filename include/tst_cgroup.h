// SPDX-License-Identifier: GPL-2.0-or-later
/*
 * Copyright (c) 2020 Red Hat, Inc.
 * Copyright (c) 2020 Li Wang <liwang@redhat.com>
 * Copyright (c) 2020-2021 SUSE LLC <rpalethorpe@suse.com>
 */
/*\
 * [DESCRIPTION]
 *
 * The LTP Control Groups library can be seperated into two parts.
 * - CGroup Core
 * - CGroup Item
 *
 * The CGroup Core is where we scan the current system, build a model
 * of the current CGroup configuration, then add missing configuration
 * necessary for the LTP or give up. Once testing is finished it then
 * also attempts cleanup.
 *
 * The CGroup Item interface is mostly what the test author interacts
 * with. It mirrors the CGroups V2 (unified) kernel API. So even if
 * the system has some nasty V1 configuration, the test author may
 * write their code as if for V2 (with exceptions). It builds on the
 * model created by CGroup Core.
 *
 * You may ask; "Why don't you just mount a simple CGroup hierarchy,
 * instead of scanning the current setup?". The short answer is that
 * it is not possible unless no CGroups are currently active and
 * almost all of our users will have CGroups active. Even if
 * unmounting the current CGroup hierarchy is a reasonable thing to do
 * to the sytem manager, it is highly unlikely the CGroup hierarchy
 * will be destroyed. So users would be forced to remove their CGroup
 * configuration and reboot the system.
 *
 * The core library tries to ensure an LTP CGroup exists on each
 * hierarchy root. Inside the LTP group it ensures a 'drain' group
 * exists and creats a test group for the current test. In the worst
 * case we end up with a set of hierarchies like the follwoing. Where
 * existing system manager created CGroups have been omitted.
 *
 * 	(V2 Root)	(V1 Root 1)	...	(V1 Root N)
 * 	    |		     |			     |
 *	  (ltp)		   (ltp)	...	   (ltp)
 *	 /     \	  /	\		  /	\
 *  (drain) (test-n) (drain)  (test-n)  ...  (drain)  (test-n)
 *
 * V2 CGroup controllers use a single unified hierarchy on a single
 * root. Two or more V1 controllers may share a root or have their own
 * root. However there may exist only one instance of a controller.
 * So you can not have the same V1 controller on multiple roots.
 *
 * It is possible to have both a V2 hierarchy and V1 hierarchies
 * active at the same time. Which is what is shown above. Any
 * controllers attached to V1 hierarchies will not be available in the
 * V2 hierarchy. The reverse is also true.
 *
 * Note that a single hierarchy may be mounted multiple
 * times. Allowing it to be accessed at different locations. However
 * subsequent mount operations will fail if the mount options are
 * different from the first.
 *
 * The user may pre-create the CGroup hierarchies and the ltp CGroup,
 * otherwise the library will try to create them. If the ltp group
 * already exists and has appropriate permissions, then admin
 * privileges will not be required to run the tests.
 *
 * Because the test may not have access to the CGroup root(s), the
 * drain CGroup is created. This can be used to store processes which
 * would otherwise block the destruction of the individual test CGroup
 * or one of its descendants.
 *
 * The test author may create child CGroups within the test CGroup
 * using the CGroup Item API. The library will create the new CGroup
 * in all the relevant hierarchies.
 *
 * There are many differences between the V1 and V2 CGroup APIs. If a
 * controller is on both V1 and V2, it may have different parameters
 * and control files. Some of these control files have a different
 * name, but the same functionality. In this case the Item API uses
 * the V2 names and aliases them to the V1 name when appropriate.
 *
 * Some control files only exist on one of the versions or they can be
 * missing due to other reasons. The Item API allows the user to check
 * if the file exists before trying to use it.
 *
 * In some cases a control file has almost the same functionality
 * between V1 and V2. Which means it can be used in the same way most
 * of the time, but not all. For now this is handled by exposing the
 * API version a controller is using to allow the test author to
 * handle edge cases. (e.g. V2 memory.swap.max accepts "max", but V1
 * memory.memsw.limit_in_bytes does not).
\*/

#ifndef TST_CGROUP_H
#define TST_CGROUP_H

#include "tst_cgroup_core.h"
#include "tst_cgroup_item.h"

#endif /* TST_CGROUP_H */
