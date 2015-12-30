/*
 * Freescale i.MX28 Boot setup
 *
 * Copyright (C) 2011 Marek Vasut <marek.vasut@gmail.com>
 * on behalf of DENX Software Engineering GmbH
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
#include <config.h>
#include <asm/io.h>
#include <asm/arch/imx-regs.h>
#include <asm/arch/sys_proto.h>
#include <asm/gpio.h>
#include <linux/compiler.h>

#include "mxs_init.h"

DECLARE_GLOBAL_DATA_PTR;
static gd_t gdata __section(".data");
#ifdef CONFIG_SPL_SERIAL_SUPPORT
static bd_t bdata __section(".data");
#endif

/*
 * This delay function is intended to be used only in early stage of boot, where
 * clock are not set up yet. The timer used here is reset on every boot and
 * takes a few seconds to roll. The boot doesn't take that long, so to keep the
 * code simple, it doesn't take rolling into consideration.
 */
void early_delay(int delay)
{
	struct mxs_digctl_regs *digctl_regs =
		(struct mxs_digctl_regs *)MXS_DIGCTL_BASE;

	uint32_t st = readl(&digctl_regs->hw_digctl_microseconds);
	st += delay;
	while (st > readl(&digctl_regs->hw_digctl_microseconds))
		;
}

#define	MUX_CONFIG_BOOTMODE_PAD	(MXS_PAD_3V3 | MXS_PAD_4MA | MXS_PAD_NOPULL)
static const iomux_cfg_t iomux_boot[] = {
#if defined(CONFIG_MX23)
	MX23_PAD_LCD_D00__GPIO_1_0 | MUX_CONFIG_BOOTMODE_PAD,
	MX23_PAD_LCD_D01__GPIO_1_1 | MUX_CONFIG_BOOTMODE_PAD,
	MX23_PAD_LCD_D02__GPIO_1_2 | MUX_CONFIG_BOOTMODE_PAD,
	MX23_PAD_LCD_D03__GPIO_1_3 | MUX_CONFIG_BOOTMODE_PAD,
	MX23_PAD_LCD_D04__GPIO_1_4 | MUX_CONFIG_BOOTMODE_PAD,
	MX23_PAD_LCD_D05__GPIO_1_5 | MUX_CONFIG_BOOTMODE_PAD,
#endif
};
extern void mxs_power_set_auto_restart(int);
#define DELAY_FOR_RESET 200000

#define XTAL_32768 32768
#define XTAL_32000 32000
#define XTAL_FREQ XTAL_32768

static void mxs_init_rtc_source(void)
{
    struct mxs_rtc_regs *rtc_regs = (struct mxs_rtc_regs *)MXS_RTC_BASE;
    unsigned int persistent0, new0;
    unsigned int persistent2;
    unsigned int secs;
    int is_reset = 0;

    mxs_power_set_auto_restart(0);
    new0 = persistent0 = readl(&rtc_regs->hw_rtc_persistent0);
    secs = readl(&rtc_regs->hw_rtc_seconds);

    persistent2 = readl(&rtc_regs->hw_rtc_persistent2);
    if (persistent2 & 1) {
        printf("RESET:  u-boot\r\n");
        clrbits_le32(&rtc_regs->hw_rtc_persistent2, 1);
        is_reset = 1;
    } else if (persistent2 & 2) {
        printf("RESET:  linux reboot\r\n");
        clrbits_le32(&rtc_regs->hw_rtc_persistent2, 2);
        is_reset = 1;
    } else if (persistent0 & RTC_PERSISTENT0_EXTERNAL_RESET) {
        printf("RESET:  reset pin\r\n");
        is_reset = 1;
    } else if (persistent0 & RTC_PERSISTENT0_THERMAL_RESET) {
        printf("RESET:  thermal reset\r\n");
        is_reset = 1;
    } else if (persistent0 & RTC_PERSISTENT0_AUTO_RESTART) {
        printf("RESET:  auto restart\r\n");
        is_reset = 1;
    } else {
        printf("PWRUP:  cold power on\r\n");
    }

	/* clr dubious bits first */
	new0 = new0
		& (~RTC_PERSISTENT0_CLOCKSOURCE)     /* rtc src, 1: 32K; 0: 24M */
		& (~RTC_PERSISTENT0_XTAL24KHZ_PWRUP) /* 24M oscillator on/off while pwrdwn, 1:on; 0:off */
		& (~RTC_PERSISTENT0_XTAL32KHZ_PWRUP) /* 32K oscillator on/off, 1:on; 0:off */
		& (~RTC_PERSISTENT0_XTAL32_FREQ)     /* freq of 32K xtal: 1:32000; 0:32768 */
		& (~RTC_PERSISTENT0_EXTERNAL_RESET)
		& (~RTC_PERSISTENT0_THERMAL_RESET);

	new0 |= RTC_PERSISTENT0_AUTO_RESTART;

#if 1
/* using 32k as RTC source */
		if (XTAL_32000 == XTAL_FREQ)
			new0 |= RTC_PERSISTENT0_XTAL32_FREQ;
		
		new0 |= RTC_PERSISTENT0_CLOCKSOURCE | RTC_PERSISTENT0_XTAL32KHZ_PWRUP;

#else
/* using 24M as RTC source */
	new0 |= RTC_PERSISTENT0_XTAL24KHZ_PWRUP;
#endif

	if (persistent0 != new0) {
		printf("Updating persistent0...");
		writel(new0,&rtc_regs->hw_rtc_persistent0);
	    while (readl(&rtc_regs->hw_rtc_stat) & RTC_STAT_NEW_REGS_MASK);
		printf("done! hw_rtc_stat:0x%x\r\n", readl(&rtc_regs->hw_rtc_stat));
	}
}

static uint8_t mxs_get_bootmode_index(void)
{
	uint8_t bootmode = 0;
	int i;
	uint8_t masked;

#if defined(CONFIG_MX23)
	/* Setup IOMUX of bootmode pads to GPIO */
	mxs_iomux_setup_multiple_pads(iomux_boot, ARRAY_SIZE(iomux_boot));

	/* Setup bootmode pins as GPIO input */
	gpio_direction_input(MX23_PAD_LCD_D00__GPIO_1_0);
	gpio_direction_input(MX23_PAD_LCD_D01__GPIO_1_1);
	gpio_direction_input(MX23_PAD_LCD_D02__GPIO_1_2);
	gpio_direction_input(MX23_PAD_LCD_D03__GPIO_1_3);
	gpio_direction_input(MX23_PAD_LCD_D05__GPIO_1_5);

	/* Read bootmode pads */
	bootmode |= (gpio_get_value(MX23_PAD_LCD_D00__GPIO_1_0) ? 1 : 0) << 0;
	bootmode |= (gpio_get_value(MX23_PAD_LCD_D01__GPIO_1_1) ? 1 : 0) << 1;
	bootmode |= (gpio_get_value(MX23_PAD_LCD_D02__GPIO_1_2) ? 1 : 0) << 2;
	bootmode |= (gpio_get_value(MX23_PAD_LCD_D03__GPIO_1_3) ? 1 : 0) << 3;
	bootmode |= (gpio_get_value(MX23_PAD_LCD_D05__GPIO_1_5) ? 1 : 0) << 5;
#elif defined(CONFIG_MX28)
	/* The global boot mode will be detected by ROM code and its value
	 * is stored at the fixed address 0x00019BF0 in OCRAM.
	 */
#define GLOBAL_BOOT_MODE_ADDR 0x00019BF0
	bootmode = __raw_readl(GLOBAL_BOOT_MODE_ADDR);
#endif

	for (i = 0; i < ARRAY_SIZE(mxs_boot_modes); i++) {
		masked = bootmode & mxs_boot_modes[i].boot_mask;
		if (masked == mxs_boot_modes[i].boot_pads)
			break;
	}

    printf("BOOT:   ");
    printf(mxs_boot_modes[i].mode);
    printf("\r\n");

	return i;
}

static void mxs_spl_fixup_vectors(void)
{
	/*
	 * Copy our vector table to 0x0, since due to HAB, we cannot
	 * be loaded to 0x0. We want to have working vectoring though,
	 * thus this fixup. Our vectoring table is PIC, so copying is
	 * fine.
	 */
	extern uint32_t _start;

	/* cppcheck-suppress nullPointer */
	memcpy(0x0, &_start, 0x60);
}

static void mxs_spl_console_init(void)
{
#ifdef CONFIG_SPL_SERIAL_SUPPORT
	gd->bd = &bdata;
	gd->baudrate = CONFIG_BAUDRATE;
	serial_init();
	gd->have_console = 1;
#endif
}

void mxs_common_spl_init(const uint32_t arg, const uint32_t *resptr,
			 const iomux_cfg_t *iomux_setup,
			 const unsigned int iomux_size)
{
	struct mxs_spl_data *data = (struct mxs_spl_data *)
		((CONFIG_SYS_TEXT_BASE - sizeof(struct mxs_spl_data)) & ~0xf);
	uint8_t bootmode;
	gd = &gdata;

	mxs_spl_fixup_vectors();

	mxs_iomux_setup_multiple_pads(iomux_setup, iomux_size);

	mxs_spl_console_init();
	early_delay(200000);
	debug("SPL: Serial Console Initialised\n");

    printf("\r\n[SPL]   begin...\r\n");

    bootmode = mxs_get_bootmode_index();

	mxs_power_init();

#if 1
    /* if reset, then delay to let DCDC have time to restore, then we can detect battery properly */
    mxs_init_rtc_source();
#else
    /* no matter reset or cold start, delay always */
    early_delay(DELAY_FOR_RESET);
#endif

	mxs_mem_init();
	data->mem_dram_size = mxs_mem_get_size();

	data->boot_mode_idx = bootmode;

	mxs_power_wait_pswitch();
    printf("[SPL]   done\r\n\r\n");

	if (mxs_boot_modes[data->boot_mode_idx].boot_pads == MXS_BM_JTAG) {
		debug("SPL: Waiting for JTAG user\n");
		asm volatile ("x: b x");
	}
}

/* Support aparatus */
inline void board_init_f(unsigned long bootflag)
{
	for (;;)
		;
}

inline void board_init_r(gd_t *id, ulong dest_addr)
{
	for (;;)
		;
}
