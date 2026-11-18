// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright 2026 IEI
 *
 * Apply device tree overlays from the Android dtbo image. The overlays are
 * selected the same way as on Yocto, "boot_fdt_overlay=yes" plus the dtbo
 * names in "fdt_overlay_files", and also by the older "dtoverlay" env.
 */

#include <asm/byteorder.h>
#include <dt_table.h>
#include <env.h>
#include <fdt_support.h>
#include <linux/libfdt.h>
#include <linux/string.h>
#include <linux/types.h>
#include <malloc.h>
#include <stdio.h>
#include <u-boot/crc.h>
#include "fb_android_dtbo.h"

#define FDT_MAX_SIZE		0xF0000
#define DTOVERLAY_MAX		16

/* Android properties to set under /firmware/android when an overlay is applied */
struct dtoverlay_prop {
	const char *dtbo;
	const char *prop;
	const char *val;
};

static const struct dtoverlay_prop dtoverlay_props[] = {
#if defined(CONFIG_TARGET_IMX95_B0787)
	{ "imx95-smarc-ov5640", "camera.layout", "ov5640" },
#endif
	{ NULL, NULL, NULL }
};

/*
 * Short names from before the name lookup. index is their old dtbo entry,
 * used only with a dtbo image built without entry ids.
 */
struct dtoverlay_alias {
	const char *name;
	const char *dtbo;
	u32 index;
};

static const struct dtoverlay_alias dtoverlay_aliases[] = {
#if defined(CONFIG_TARGET_IMX8MP_B643_PPC)
	{ "dsi-tq101aj02", "imx8mp-b643-ppc-mipi-dsi-tq101aj02", 1 },
#endif
#if defined(CONFIG_TARGET_IMX95_B0787)
	{ "ov5640", "imx95-smarc-ov5640", 1 },
#endif
	{ NULL, NULL, 0 }
};

static struct dt_table_entry *dtbo_entry(struct dt_table_header *dt_img, u32 i)
{
	return (struct dt_table_entry *)((ulong)dt_img +
		be32_to_cpu(dt_img->dt_entries_offset) +
		i * be32_to_cpu(dt_img->dt_entry_size));
}

/* The build sets each overlay entry id to the crc32 of its dtbo name */
static struct dt_table_entry *find_dtbo(struct dt_table_header *dt_img,
					const char *dtbo)
{
	u32 count = be32_to_cpu(dt_img->dt_entry_count);
	u32 id = crc32(0, (const unsigned char *)dtbo, strlen(dtbo));
	u32 i;

	for (i = 1; i < count; i++) {
		if (be32_to_cpu(dtbo_entry(dt_img, i)->id) == id)
			return dtbo_entry(dt_img, i);
	}

	return NULL;
}

static bool dtbo_has_ids(struct dt_table_header *dt_img)
{
	u32 count = be32_to_cpu(dt_img->dt_entry_count);
	u32 i;

	for (i = 1; i < count; i++) {
		if (be32_to_cpu(dtbo_entry(dt_img, i)->id))
			return true;
	}

	return false;
}

static int fdt_grow(void *fdt, u32 size)
{
	if (fdt_totalsize(fdt) > FDT_MAX_SIZE ||
	    size > FDT_MAX_SIZE - fdt_totalsize(fdt))
		return -FDT_ERR_NOSPACE;
	return fdt_increase_size(fdt, size);
}

/* The base fdt is restored if the overlay fails to apply */
static int apply_dtbo(void *fdt, struct dt_table_header *dt_img,
		      struct dt_table_entry *entry)
{
	u32 img_size = be32_to_cpu(dt_img->total_size);
	u32 offset = be32_to_cpu(entry->dt_offset);
	u32 size = be32_to_cpu(entry->dt_size);
	u32 fdt_size = fdt_totalsize(fdt);
	void *dtbo, *backup;
	int ret;

	if (size < sizeof(struct fdt_header) || offset > img_size ||
	    size > img_size - offset)
		return -FDT_ERR_BADOFFSET;

	dtbo = malloc(size);
	backup = malloc(fdt_size);
	if (!dtbo || !backup) {
		ret = -FDT_ERR_NOSPACE;
		goto out;
	}
	memcpy(dtbo, (void *)((ulong)dt_img + offset), size);
	ret = fdt_check_header(dtbo);
	if (!ret && fdt_totalsize(dtbo) > size)
		ret = -FDT_ERR_TRUNCATED;
	if (ret)
		goto out;

	memcpy(backup, fdt, fdt_size);
	ret = fdt_grow(fdt, size);
	if (!ret)
		ret = fdt_overlay_apply(fdt, dtbo);
	if (ret)
		memcpy(fdt, backup, fdt_size);
out:
	free(backup);
	free(dtbo);
	return ret;
}

static int set_android_prop(void *fdt, const char *prop, const char *val)
{
	int node, ret;

	ret = fdt_grow(fdt, 64);
	if (ret)
		return ret;
	node = fdt_path_offset(fdt, "/firmware/android");
	if (node < 0)
		return node;
	return fdt_setprop_string(fdt, node, prop, val);
}

void android_apply_dtoverlay(struct dt_table_header *dt_img, ulong fdt_addr)
{
	void *fdt = (void *)fdt_addr;
	const struct dtoverlay_alias *a, *alias;
	const struct dtoverlay_prop *p;
	struct dt_table_entry *entry;
	const char *dtbo;
	struct dt_table_entry *applied[DTOVERLAY_MAX];
	int i, n_applied = 0;
	char buf[256] = "";
	char *env, *name, *ext;
	int ret;

	env = env_get("boot_fdt_overlay");
	if (env && !strcmp(env, "yes")) {
		env = env_get("fdt_overlay_files");
		if (env)
			strlcpy(buf, env, sizeof(buf));
	}
	env = env_get("dtoverlay");
	if (env) {
		strlcat(buf, " ", sizeof(buf));
		strlcat(buf, env, sizeof(buf));
	}

	for (name = strtok(buf, " "); name; name = strtok(NULL, " ")) {
		ext = strstr(name, ".dtbo");
		if (ext && !ext[5])
			*ext = '\0';
		if (!strcmp(name, "no_overlay"))
			continue;

		dtbo = name;
		alias = NULL;
		for (a = dtoverlay_aliases; a->name; a++) {
			if (!strcmp(name, a->name)) {
				alias = a;
				dtbo = a->dtbo;
				break;
			}
		}

		entry = find_dtbo(dt_img, dtbo);
		if (!entry && alias && !dtbo_has_ids(dt_img) &&
		    alias->index < be32_to_cpu(dt_img->dt_entry_count))
			entry = dtbo_entry(dt_img, alias->index);
		if (!entry) {
			printf("ANDROID: dtoverlay %s not found, skip\n", name);
			continue;
		}
		for (i = 0; i < n_applied; i++) {
			if (applied[i] == entry)
				break;
		}
		if (i < n_applied)
			continue;
		if (n_applied == DTOVERLAY_MAX) {
			printf("ANDROID: too many dtoverlays, skip %s\n", name);
			continue;
		}

		ret = apply_dtbo(fdt, dt_img, entry);
		if (ret) {
			printf("ANDROID: dtoverlay %s failed, ret=%d\n", dtbo, ret);
			continue;
		}
		applied[n_applied++] = entry;
		printf("ANDROID: dtoverlay %s applied\n", dtbo);

		for (p = dtoverlay_props; p->dtbo; p++) {
			if (strcmp(dtbo, p->dtbo))
				continue;
			ret = set_android_prop(fdt, p->prop, p->val);
			if (ret)
				printf("ANDROID: set %s failed, ret=%d\n", p->prop, ret);
		}
	}
}
