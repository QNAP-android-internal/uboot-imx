/*
 * Copyright 2021 NXP
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef IMX8MP_B643_PPC_ANDROID_H
#define IMX8MP_B643_PPC_ANDROID_H

#define FSL_FASTBOOT_FB_DEV "mmc"

#ifdef CONFIG_SYS_MALLOC_LEN
#undef CONFIG_SYS_MALLOC_LEN
#define CONFIG_SYS_MALLOC_LEN           (32 * SZ_1M)
#endif

#undef CFG_EXTRA_ENV_SETTINGS
#undef CONFIG_BOOTCOMMAND

#define PANEL_ENV_SETTINGS \
	"panelargs=" \
		"if test $dtoverlay = dsi-tq101aj02; then " \
			"setenv panel_rot 90; " \
			"setenv panel_density 240; " \
		"else " \
			"setenv panel_rot 0; " \
			"setenv panel_density 320; " \
		"fi; " \
		"setenv bootargs ${bootargs} " \
		"androidboot.lcd_density=${panel_density} " \
		"androidboot.hwrotation=${panel_rot}; \0"

#define CFG_EXTRA_ENV_SETTINGS		\
	PANEL_ENV_SETTINGS	\
	"bootcmd=" \
	"run panelargs; " \
	"boota "__stringify(FSL_FASTBOOT_FB_DEV)"2\0" \
	"splashpos=m,m\0"			\
	"splashimage=0x50000000\0"		\
	"fdt_high=0xffffffffffffffff\0"		\
	"initrd_high=0xffffffffffffffff\0"	\
	"dtoverlay=no_overlay\0"	\
	"bootargs="	\
	"stack_depot_disable=on "	\
	"kasan.stacktrace=off "		\
	"console=ttymxc1,115200 "	\
	"earlycon=ec_imx6q,0x30890000,115200 "	\
	"init=/init "				\
	"firmware_class.path=/vendor/firmware "	\
	"loop.max_part=7 "			\
	"transparent_hugepage=never "	\
	"swiotlb=65536 "				\
	"pci=nomsi "					\
	"cma=1184M@0x400M-0x1000M "		\
	"bootconfig "					\
	"androidboot.console=ttymxc1 "	\
	"androidboot.hardware=nxp "		\
	"androidboot.primary_display=imx-drm "	\
	"androidboot.displaymode=1080p "	\
	"androidboot.wificountrycode=CN "	\
	"buildvariant=userdebug "		\
	"fbcon=logo-pos:center "		\
	"fbcon=logo-count:1 "			\
	"quiet\0"

/* Enable mcu firmware flash */
#ifdef CONFIG_FLASH_MCUFIRMWARE_SUPPORT
#define ANDROID_MCU_FRIMWARE_DEV_TYPE DEV_MMC
#define ANDROID_MCU_FIRMWARE_START 0x500000
#define ANDROID_MCU_OS_PARTITION_SIZE 0x40000
#define ANDROID_MCU_FIRMWARE_SIZE  0x20000
#define ANDROID_MCU_FIRMWARE_HEADER_STACK 0x20020000
#endif

#define CFG_SYS_SPL_PTE_RAM_BASE    0x41580000

#ifdef CONFIG_IMX_TRUSTY_OS
#define BOOTLOADER_RBIDX_OFFSET  0x3FE000
#define BOOTLOADER_RBIDX_START   0x3FF000
#define BOOTLOADER_RBIDX_LEN     0x08
#define BOOTLOADER_RBIDX_INITVAL 0
#endif

#ifdef CONFIG_IMX_TRUSTY_OS
#define AVB_RPMB
#define KEYSLOT_HWPARTITION_ID 2
#define KEYSLOT_BLKS             0x1FFF
#define NS_ARCH_ARM64 1
#endif

#endif
