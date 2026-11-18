/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * Copyright 2026 IEI
 */

#ifndef __FB_ANDROID_DTBO_H__
#define __FB_ANDROID_DTBO_H__

#include <linux/types.h>

struct dt_table_header;

void android_apply_dtoverlay(struct dt_table_header *dt_img, ulong fdt_addr);

#endif /* __FB_ANDROID_DTBO_H__ */
