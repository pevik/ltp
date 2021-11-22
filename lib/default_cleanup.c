// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2021 Zhao gongyi <zhaogongyi@huawei.com>
 */

#include "test.h"

void __attribute__((destructor)) default_cleanup(void)
{
	tst_rmdir();
}
