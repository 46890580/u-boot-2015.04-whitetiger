/*
 * Copyright 2004-2006,2010 Freescale Semiconductor, Inc. All Rights Reserved.
 * Copyright (C) 2008 by Sascha Hauer <kernel@pengutronix.de>
 * Copyright (C) 2009 by Jan Weitzel Phytec Messtechnik GmbH,
 *                       <armlinux@phytec.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/arch/clock.h>
#include <asm/arch/iomux.h>
#include <asm/arch/imx-regs.h>
#include <asm/gpio.h>

#if	defined(CONFIG_MX23)
#define	DRIVE_OFFSET	0x200
#define	PULL_OFFSET	0x400
#elif	defined(CONFIG_MX28)
#define	DRIVE_OFFSET	0x300
#define	PULL_OFFSET	0x600
#else
#error "Please select CONFIG_MX23 or CONFIG_MX28"
#endif

/*
 * configures a single pad in the iomuxer
 */
int mxs_iomux_setup_pad(iomux_cfg_t pad)
{
	u32 reg, ofs, bp, bm;
	void *iomux_base = (void *)MXS_PINCTRL_BASE;
	struct mxs_register_32 *mxs_reg;

	/* muxsel */
	ofs = 0x100;
	ofs += PAD_BANK(pad) * 0x20 + PAD_PIN(pad) / 16 * 0x10;
	bp = PAD_PIN(pad) % 16 * 2;
	bm = 0x3 << bp;
	reg = readl(iomux_base + ofs);
	reg &= ~bm;
	reg |= PAD_MUXSEL(pad) << bp;
	writel(reg, iomux_base + ofs);

	/* drive */
	ofs = DRIVE_OFFSET;
	ofs += PAD_BANK(pad) * 0x40 + PAD_PIN(pad) / 8 * 0x10;
	/* mA */
	if (PAD_MA_VALID(pad)) {
		bp = PAD_PIN(pad) % 8 * 4;
		bm = 0x3 << bp;
		reg = readl(iomux_base + ofs);
		reg &= ~bm;
		reg |= PAD_MA(pad) << bp;
		writel(reg, iomux_base + ofs);
	}
	/* vol */
	if (PAD_VOL_VALID(pad)) {
		bp = PAD_PIN(pad) % 8 * 4 + 2;
		mxs_reg = (struct mxs_register_32 *)(iomux_base + ofs);
		if (PAD_VOL(pad))
			writel(1 << bp, &mxs_reg->reg_set);
		else
			writel(1 << bp, &mxs_reg->reg_clr);
	}

	/* pull */
	if (PAD_PULL_VALID(pad)) {
		ofs = PULL_OFFSET;
		ofs += PAD_BANK(pad) * 0x10;
		bp = PAD_PIN(pad);
		mxs_reg = (struct mxs_register_32 *)(iomux_base + ofs);
		if (PAD_PULL(pad))
			writel(1 << bp, &mxs_reg->reg_set);
		else
			writel(1 << bp, &mxs_reg->reg_clr);
	}

	return 0;
}

static void mxs_reinit_all_pins(void)
{
	u32  ofs, doe_ofs, dout;
	/* void *iomux_base = (void *)MXS_PINCTRL_BASE; */

	/* mux all pins as gpio */
	ofs = MXS_PINCTRL_BASE + 0x100; writel(0x0000ffff, ofs);/* [BANK - 0]	MUX_00 : PIN_00 ~ 07, bit 16-31 16 bitsreserved */
	ofs = MXS_PINCTRL_BASE + 0x110; writel(0x03ffffff, ofs);/* 			MUX_01 : PIN-16 ~ 28, bit 26-31 06 bits reserved */
	ofs = MXS_PINCTRL_BASE + 0x120; writel(0xffffffff, ofs);/* [BANK - 1]	MUX_02 : PIN-00 ~ 15 */
	ofs = MXS_PINCTRL_BASE + 0x130; writel(0xffffffff, ofs);/* 			MUX_03 : PIN-16 ~ 31 */
	ofs = MXS_PINCTRL_BASE + 0x140; writel(0xff3fffff, ofs);/* [BANK - 2]	MUX_04 : PIN-00 ~ 15, bit 22-23 02 bits reserved */
	ofs = MXS_PINCTRL_BASE + 0x150; writel(0x00ff0fff, ofs);/* 			MUX_05 : PIN-16 ~ 27, bit 24-31 08 bits, 12-15 04 bits reserved */
	ofs = MXS_PINCTRL_BASE + 0x160; writel(0xffffffff, ofs);/* [BANK - 3]	MUX_06 : PIN-00 ~ 15 */
	ofs = MXS_PINCTRL_BASE + 0x170; writel(0x3fffff3f, ofs);/* 			MUX_07 : PIN-16 ~ 30, bit 30-31 02 bits, 6-7, 02 bits reserved */
	ofs = MXS_PINCTRL_BASE + 0x180; writel(0xffffffff, ofs);/* [BANK - 4]	MUX_08 : PIN-00 ~ 15 */
	ofs = MXS_PINCTRL_BASE + 0x190; writel(0x00000303, ofs);/* 			MUX_09 : PIN-16 , 20 */
	ofs = MXS_PINCTRL_BASE + 0x1a0; writel(0xffffffff, ofs);/* [BANK - 5]	MUX_10 : PIN-00 ~ 15 */
	ofs = MXS_PINCTRL_BASE + 0x1b0; writel(0x0030ffff, ofs);/* 			MUX_11 : PIN-16 ~ 26, bit 16-19, 22-31, 14 bits reserved */
	ofs = MXS_PINCTRL_BASE + 0x1c0; writel(0x3fffffff, ofs);/* [BANK - 6]	MUX_12 : PIN-00 ~ 14, bit 30-31, 2 bits reserved. all disabled */
	ofs = MXS_PINCTRL_BASE + 0x1d0; writel(0x0003ffff, ofs);/* 			MUX_13 : PIN-16 ~ 24, bit 18-31, 14 bits reserved. all disabled */
	/* set all output value as 0, except:
	 *     gpio_0_26, 1: turn off fec 3v3
	 *     gpio_1_23, 1: disable watch dog
	 *     gpio_3_28, 1: turn on vbat_gsm
	 */
	dout = MXS_PINCTRL_BASE + 0x700; writel(0x04000000, dout);
	dout = MXS_PINCTRL_BASE + 0x710; writel(0x00800000, dout);
	dout = MXS_PINCTRL_BASE + 0x720; writel(0, dout);
	dout = MXS_PINCTRL_BASE + 0x730; writel(0x10000000, dout);
	dout = MXS_PINCTRL_BASE + 0x740; writel(0, dout);

#if 0
	/* config all pins as input, except: i2c0-clk i2c0-data as output-level0 */
	doe_ofs = MXS_PINCTRL_BASE + 0xb04; writel(0, doe_ofs); /* set */
	doe_ofs = MXS_PINCTRL_BASE + 0xb10; writel(0, doe_ofs);
	doe_ofs = MXS_PINCTRL_BASE + 0xb20; writel(0, doe_ofs);
	doe_ofs = MXS_PINCTRL_BASE + 0xb30; writel(0x30000, doe_ofs);		/* 3_16, 3_17, i2c0-scl, i2c0-sda as output */
	doe_ofs = MXS_PINCTRL_BASE + 0xb40; writel(0x4000, doe_ofs);			/* 4_14, ext pwr switch: output */
#elif 1
	/* config all pins as output */
	/*doe_ofs = MXS_PINCTRL_BASE + 0xb04; writel(0x1fff00ff, doe_ofs);*/ /* set */
	doe_ofs = MXS_PINCTRL_BASE + 0xb04; writel(0x1fff00ff, doe_ofs); /* set */
	doe_ofs = MXS_PINCTRL_BASE + 0xb10; writel(0xff7fffff, doe_ofs); /* gpio_1_23 input, disable WD */
	doe_ofs = MXS_PINCTRL_BASE + 0xb20; writel(0x0fffffff, doe_ofs);
	doe_ofs = MXS_PINCTRL_BASE + 0xb30; writel(0x7fffffff, doe_ofs);
	doe_ofs = MXS_PINCTRL_BASE + 0xb40; writel(0x001fffff, doe_ofs);
#elif 0
	/* configure all pins as input */
	doe_ofs = MXS_PINCTRL_BASE + 0xb00; writel(0x0, doe_ofs);
	doe_ofs = MXS_PINCTRL_BASE + 0xb10; writel(0x0, doe_ofs);
	doe_ofs = MXS_PINCTRL_BASE + 0xb20; writel(0x0, doe_ofs);
	doe_ofs = MXS_PINCTRL_BASE + 0xb30; writel(0x0, doe_ofs);
	doe_ofs = MXS_PINCTRL_BASE + 0xb40; writel(0x0, doe_ofs);
#endif

}

int mxs_iomux_setup_multiple_pads(const iomux_cfg_t *pad_list, unsigned count)
{
	const iomux_cfg_t *p = pad_list;
	int i;
	int ret;

	mxs_reinit_all_pins();										/* all pins output, level 0 */
	gpio_direction_input(MX28_PAD_GPMI_CE0N__GPIO_0_16);		/* tca6416 INT pin, input, must be before vccio_3v3 */
	udelay(10000);												/* just turned of vccio, wait for 10ms to let peripherals shutdown */
	gpio_direction_output(MX28_PAD_ENET0_COL__GPIO_4_14, 1);    /* switch on vccio_3v3 */

	for (i = 0; i < count; i++) {
		ret = mxs_iomux_setup_pad(*p);
		if (ret)
			return ret;
		p++;
	}
	gpio_direction_output(MX28_PAD_GPMI_RESETN__GPIO_0_28, 1);	/* keep emmc reset high, not reset */
    gpio_direction_input(MX28_PAD_GPMI_RDN__GPIO_0_24);         /* 0_24 input, high:512M DDR, low:256M */
	gpio_direction_input(MX28_PAD_LCD_RESET__GPIO_3_30);        /* 3_30 input, high:boot-emmc, low:boot-tftp/nfs */

	return 0;
}
