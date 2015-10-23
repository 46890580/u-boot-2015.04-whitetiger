/*
 * (C) Copyright 2011 Freescale Semiconductor, Inc.
 * Author: Fabio Estevam <fabio.estevam@freescale.com>
 *
 * Based on m28evk.h:
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __CONFIGS_MX28EVK_H__
#define __CONFIGS_MX28EVK_H__

/* System configurations */
#define CONFIG_MX28				/* i.MX28 SoC */
#define CONFIG_MACH_TYPE	MACH_TYPE_MX28EVK

/* SPL */
#define CONFIG_SPL_SERIAL_SUPPORT

/* U-Boot Commands */
#define CONFIG_SYS_NO_FLASH
#include <config_cmd_default.h>
#define CONFIG_DISPLAY_CPUINFO
#define CONFIG_DOS_PARTITION

#define CONFIG_CMD_CACHE
#define CONFIG_CMD_DATE
#define CONFIG_CMD_DHCP
#define CONFIG_CMD_FAT
#define CONFIG_CMD_GPIO
#define CONFIG_CMD_MII
#define CONFIG_CMD_MMC
#define CONFIG_CMD_NET
#define CONFIG_CMD_NFS
#define CONFIG_CMD_PING
#define CONFIG_CMD_SAVEENV
#define CONFIG_CMD_SETEXPR
#define CONFIG_CMD_SF
#define CONFIG_CMD_SPI
/* #define CONFIG_CMD_USB */
#define CONFIG_CMD_BOOTZ
/* #define CONFIG_CMD_NAND */
/* #define CONFIG_CMD_NAND_TRIMFFS */
/* #define CONFIG_VIDEO */

/* Memory configuration */
#define CONFIG_NR_DRAM_BANKS		1		/* 1 bank of DRAM */
#define PHYS_SDRAM_1			0x40000000	/* Base address */
#define PHYS_SDRAM_1_SIZE		0x40000000	/* Max 1 GB RAM */
#define CONFIG_SYS_SDRAM_BASE		PHYS_SDRAM_1

/* Environment */
#ifndef CONFIG_ENV_IS_IN_SPI_FLASH
#define CONFIG_ENV_SIZE			(16 * 1024)
#else
#define CONFIG_ENV_SIZE			(4 * 1024)
#endif
#define CONFIG_ENV_OVERWRITE

/* Environment is in MMC */
#if defined(CONFIG_CMD_MMC) && defined(CONFIG_ENV_IS_IN_MMC)
#define CONFIG_ENV_OFFSET		(512 * 1024)
#define CONFIG_SYS_MMC_ENV_DEV		0
#endif

/* Environment is in NAND */
#if defined(CONFIG_CMD_NAND) && defined(CONFIG_ENV_IS_IN_NAND)
#define CONFIG_ENV_SIZE_REDUND		CONFIG_ENV_SIZE
#define CONFIG_ENV_SECT_SIZE		(128 * 1024)
#define CONFIG_ENV_RANGE		(512 * 1024)
#define CONFIG_ENV_OFFSET		0x300000
#define CONFIG_ENV_OFFSET_REDUND	\
		(CONFIG_ENV_OFFSET + CONFIG_ENV_RANGE)
#endif

/* Environemnt is in SPI flash */
#if defined(CONFIG_CMD_SF) && defined(CONFIG_ENV_IS_IN_SPI_FLASH)
#define CONFIG_SYS_REDUNDAND_ENVIRONMENT
#define CONFIG_ENV_OFFSET		0x40000		/* 256K */
#define CONFIG_ENV_OFFSET_REDUND	(CONFIG_ENV_OFFSET + CONFIG_ENV_SIZE)
#define CONFIG_ENV_SECT_SIZE		0x1000
#define CONFIG_ENV_SPI_CS		0
#define CONFIG_ENV_SPI_BUS		2
#define CONFIG_ENV_SPI_MAX_HZ		24000000
#define CONFIG_ENV_SPI_MODE		SPI_MODE_0
#endif

/* UBI and NAND partitioning */
#ifdef CONFIG_CMD_NAND
#define CONFIG_CMD_UBI
#define CONFIG_CMD_UBIFS
#define CONFIG_CMD_MTDPARTS
#define CONFIG_RBTREE
#define CONFIG_LZO
#define CONFIG_MTD_DEVICE
#define CONFIG_MTD_PARTITIONS
#define MTDIDS_DEFAULT			"nand0=gpmi-nand"
#define MTDPARTS_DEFAULT			\
	"mtdparts=gpmi-nand:"			\
		"3m(bootloader)ro,"		\
		"512k(environment),"		\
		"512k(redundant-environment),"	\
		"4m(kernel),"			\
		"128k(fdt),"			\
		"896k(pad1),"			\
		"11m(pad2),"			\
		"236m(filesystem)"
#endif

/* FEC Ethernet on SoC */
#ifdef	CONFIG_CMD_NET
#define CONFIG_FEC_MXC
#define CONFIG_NET_MULTI
#define CONFIG_MX28_FEC_MAC_IN_OCOTP
#endif

/* RTC */
#ifdef	CONFIG_CMD_DATE
#define	CONFIG_RTC_MXS
#endif

/* USB */
#ifdef	CONFIG_CMD_USB
#define CONFIG_EHCI_MXS_PORT1
#define CONFIG_USB_MAX_CONTROLLER_COUNT	1
#define	CONFIG_USB_STORAGE
#define	CONFIG_USB_HOST_ETHER
#define	CONFIG_USB_ETHER_ASIX
#define	CONFIG_USB_ETHER_SMSC95XX
#endif

/* SPI */
#ifdef CONFIG_CMD_SPI
#define CONFIG_DEFAULT_SPI_BUS		2
#define CONFIG_DEFAULT_SPI_MODE		SPI_MODE_0

/* SPI Flash */
#ifdef CONFIG_CMD_SF
#define CONFIG_SPI_FLASH
#define CONFIG_SF_DEFAULT_BUS		2
#define CONFIG_SF_DEFAULT_CS		0
/* this may vary and depends on the installed chip */
#define CONFIG_SPI_FLASH_SST
#define CONFIG_SF_DEFAULT_MODE		SPI_MODE_0
#define CONFIG_SF_DEFAULT_SPEED		24000000
#endif

#endif

/* Framebuffer support */
#ifdef CONFIG_VIDEO
#define CONFIG_VIDEO_LOGO
#define CONFIG_SPLASH_SCREEN
#define CONFIG_CMD_BMP
#define CONFIG_BMP_16BPP
#define CONFIG_VIDEO_BMP_RLE8
#define CONFIG_VIDEO_BMP_GZIP
#define CONFIG_SYS_VIDEO_LOGO_MAX_SIZE	(512 << 10)
#endif

/* Boot Linux */
#define CONFIG_BOOTDELAY	3
#define CONFIG_BOOTFILE		"uImage"
#define CONFIG_LOADADDR		0x42000000
#define CONFIG_SYS_LOAD_ADDR	CONFIG_LOADADDR

/* Extra Environment */
#define CONFIG_EXTRA_ENV_SETTINGS \
	"script=boot.scr\0"	\
	"boot_fdt=try\0" \
	"ip_dyn=yes\0" \
	"bootscript=echo Running bootscript from mmc ...; "	\
		"source\0" \
	"erase_filesystem=mmc erase 0x4800 0x71B800\0" \
	"erase_ubtenv=mmc erase 0x400 0x400\0" \
	"erase_all=mmc erase 0 0x720000\0" \
	"update_uboot=mw 0x42000000 0 0x200000; tftp 0x42000000 u-boot.sd; " \
		"mmc erase 0x800 0x1000; mmc write 0x42000000 0x800 0x1000\0" \
	"update_kernel=mw 0x42000000 0 0x400000; tftp 0x42000000 uImage; " \
		"mmc erase 0x1800 0x2000; mmc write 0x42000000 0x1800 0x2000\0" \
	"update_dtb=mw 0x42000000 0 0x100000; tftp 0x42000000 imx28-whitetiger.dtb; " \
		"mmc erase 0x3800 0x800; mmc write 0x42000000 0x3800 0x800\0" \
	"serverip=192.168.0.127\0"\
	"ipaddr=192.168.0.126\0"\
	"netmask=255.255.255.0\0"\
	"bootargs=console=ttyAMA0,115200,8n1  rootfstype=ext3 root=/dev/mmcblk0p4 rw rootwait\0"\
	"emmcboot=mmc read 0x42000000 0x1800 0x2000; mmc read 0x42800000 0x3800 0x800; bootm 0x42000000 - 0x42800000\0" \
	"netbootargs=setenv bootargs console=ttyAMA0,115200,8n1 root=/dev/nfs rw ip=${ipaddr} " \
		"nfsroot=${serverip}:/nfsroot fec_mac=00:04:9f:99:66:df\0" \
	"factoryboot=${erase_all}; run netbootargs; tftp 0x42000000 uImage; " \
		"tftp 0x42800000 imx28-whitetiger.dtb; bootm 0x42000000 - 0x42800000 \0" \
	"netboot=run netbootargs; tftp 0x42000000 uImage; " \
		"tftp 0x42800000 imx28-whitetiger.dtb; bootm 0x42000000 - 0x42800000 \0"


#define CONFIG_BOOTCOMMAND \
	"run emmcboot" 

/* The rest of the configuration is shared */
#include <configs/mxs.h>

#endif /* __CONFIGS_MX28EVK_H__ */
