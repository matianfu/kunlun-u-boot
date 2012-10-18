/*
 * Copyright (c) 2010 Wind River Systems, Inc.
 *
 * (C) Copyright 2008 - 2009 Texas Instruments.
 *
 * Configuration settings for the kunlun board, based on 3430 TI Zoom2 board.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

#ifndef __CONFIG_H
#define __CONFIG_H

/*
 * High Level Configuration Options
 */
#define CONFIG_ARMCORTEXA8	1    /* This is an ARM V7 CPU core */
#define CONFIG_OMAP		1    /* in a TI OMAP core */
#define CONFIG_OMAP36XX		1    /* which is a 36XX */
#define CONFIG_OMAP34XX		1    /* reuse the 34XX setup */
#define CONFIG_OMAP3430		1    /* which is in a 3430 */
#define CONFIG_3630KUNLUN	1    /* working on kunlun board */
#define CONFIG_FASTBOOT	        1    /* Using fastboot interface */

#if defined(CONFIG_3630KUNLUN_KL9C) || defined(CONFIG_3630KUNLUN_P2)
#define CONFIG_LCD_RM68041    1
#define CONFIG_BACKLIGHT_CAT3637  1  /*use CAT3637 backlight control chip */
#define CONFIG_OPTICAL_MOUSE    1       //support optical mouse
//#define CONFIG_LCD_R61529       1
#elif defined(CONFIG_3630KUNLUN_WUDANG)
#define CONFIG_LCD_NT35510    1
#define CONFIG_BACKLIGHT_TWL5030_PWM  1  /*use TWL5030_PWM backlight control */
//#define CONFIG_SUPPORT_STE_MDM 	/*supoort for STE MODEM*/

/* Haier N710E. */
#elif defined(CONFIG_3630KUNLUN_N710E)
#define CONFIG_LCD_RM68041_N710E	1
#define CONFIG_BACKLIGHT_CAT3648	1  /*use CAT3648 backlight control chip */
#define CONFIG_CAM_OV2655		1  /*sensor type is ov2655*/

#else
#define CONFIG_LCD_ILI9327	1    /*use ILI9327 LCD drvier IC on kunlun board */
#define CONFIG_BACKLIGHT_CAT3648  1  /*use CAT3648 backlight control chip */
#define CONFIG_CAM_OV2655 1  /*sensor type is ov2655*/
#endif

#if defined(CONFIG_LCD_ILI9327) \
	|| defined(CONFIG_LCD_RM68041) \
	|| defined(CONFIG_LCD_R61529) \
	|| defined(CONFIG_LCD_NT35510) \
	|| defined(CONFIG_LCD_RM68041_N710E)
#define BOARD_WITH_LCD_SHOW
#endif

/* When Lcd size is enlarged , bss is enlarged too.it's bss end will reach and overlaped with fat load base address , so cause fat load error! zwzhu*/
#define FAT_LOAD_BASEADDR "0x82000000" /*old is 0x81000000*/
/* When Lcd size is enlarged , bss is enlarged too.it's bss end will reach and overlaped with fastboot load base address , so cause fastboot load error! zwzhu*/
#define FAST_BOOT_BUFFER_BASEADDR (PHYS_SDRAM_1 + SZ_32M) /*OLD is (PHYS_SDRAM_1 + SZ_16M)*/
#define FAST_BOOT_BUFFER_SIZE (SZ_256M - SZ_32M)

#if 0
#define CONFIG_ZOOM2_LED	1    /* Using Zoom2 LED's */
#endif
#define CONFIG_TWL4030_KEYPAD   1    /* Use the keypad */
#define CONFIG_TWL4030_USB      1    /* Initialize twl usb */
#define CONFIG_KUNLUN_CAM 1 /*use OV2655 sensor*/
#if !defined(CONFIG_STORAGE_EMMC)
#define CONFIG_STORAGE_NAND     1    /* Flash to NAND using Fastboot */
#endif

#define CONFIG_CMD_TWL4030_KEYPAD  1	/* keypad test command */
//#define CONFIG_3430_AS_3410	1	/* true for 3430 in 3410 mode */
#define CONFIG_BOOTCASE_CHECK	1	/* Enable the bootcase check */
#define CONFIG_ADC_CALIBRATION	1	/* Support ADC Calibration */
#define CONFIG_SILENT_CONSOLE	1	/* enable silent startup	*/
//#define CONFIG_VIA_NAND_BBM	1	       /* Enable bad block management */

#include <asm/arch/cpu.h>        /* get chip and board defs */

/* Clock Defines */
#define V_OSCK                   26000000  /* Clock output from T2 */

#define V_SCLK                   (V_OSCK >> 1)

#if defined(CONFIG_3630KUNLUN_P2)   /* Define the DDR type */
#define CONFIG_DDR_H8KDS0UN0MER_4EM
#endif

//#define PRCM_CLK_CFG2_400MHZ    1    /* VDD2=1.2v - 200MHz DDR */
#define PRCM_CLK_CFG2_332MHZ    1    /* 166MHz DDR */

#define PRCM_PCLK_OPP2          1    /* ARM=500MHz - VDD1=1.20v */

/* PER clock options, only uncomment 1 */
/* Uncomment to run PER M2 at 2x 96MHz */
/* #define CONFIG_PER_M2_192 */
/* Uncomment to run PER SGX at 192MHz */
/*define CONFIG_PER_SGX_192*/

/* Uncommend to run PER at 2x 96MHz */

#undef CONFIG_USE_IRQ                 /* no support for IRQs */

/* Kunlun enable this macro by default, see misc_init_r() implementation in omap3630kunlun.c */
/* UGlee undefine this macro disable misc_init_r() in board.c  */
#ifndef CONFIG_MISC_INIT_R	
#define CONFIG_MISC_INIT_R
#endif

#define CONFIG_CMDLINE_TAG       1    /* enable passing of ATAGs */
#define CONFIG_SETUP_MEMORY_TAGS 1
#define CONFIG_INITRD_TAG        1
#define CONFIG_REVISION_TAG      1

/*
 * Size of malloc() pool
 */
#define CFG_ENV_SIZE             SZ_128K    /* Total Size Environment Sector */
#define CFG_MALLOC_LEN           (CFG_ENV_SIZE + SZ_128K)
#define CFG_GBL_DATA_SIZE        128  /* bytes reserved for initial data */

/*
 * Hardware drivers
 */

/*
 * NS16550 Configuration:
 */

#define V_NS16550_CLK            (48000000)  /* 48MHz */
#define CFG_NS16550
#define CFG_NS16550_SERIAL
#define CFG_NS16550_REG_SIZE     (-4)
#define CFG_NS16550_CLK          V_NS16550_CLK
#define CFG_NS16550_COM1         OMAP34XX_UART1
#define CFG_NS16550_COM3         OMAP34XX_UART3

/* allow to overwrite serial and ethaddr */
#define CONFIG_ENV_OVERWRITE
#define CONFIG_CONS_INDEX        3 /** UGlee: 3 restored, can be changed to 1 **/
#define CONFIG_BAUDRATE          115200
#define CFG_BAUDRATE_TABLE       {4800, 9600, 19200, 38400, 57600, 115200}

#define CONFIG_MMC               1
#define CFG_MMC_BASE             0xF0000000
#define CONFIG_DOS_PARTITION     1

/*  #define NET_CMDS                 (CFG_CMD_DHCP|CFG_CMD_NFS|CFG_CMD_NET)  */

#ifndef CONFIG_OPTIONAL_NOR_POPULATED
//#define C_MSK (CFG_CMD_FLASH | CFG_CMD_IMLS)
#define C_MSK (CFG_CMD_IMLS | CFG_CMD_FLASH)
#endif

#define N_MSK (CFG_CMD_NET)
/* Config CMD*/
//#define CFG_ROCKBOX_FAT
#define CONFIG_COMMANDS		(((CFG_CMD_I2C | CONFIG_CMD_DFL | \
                CFG_CMD_FAT | CFG_CMD_MMC |\
                CFG_CMD_NAND) & ~(C_MSK)) & ~(N_MSK))

#define CONFIG_BOOTP_MASK        CONFIG_BOOTP_DEFAULT

/* this must be included AFTER the definition of CONFIG_COMMANDS (if any) */
#include <cmd_confdefs.h>

#if (CONFIG_COMMANDS & CFG_CMD_NET)
/*
 * SMC91c96 Etherent
 */
#define CONFIG_DRIVER_SMSC9118
#define CONFIG_SMSC9118_BASE	 (DEBUG_BASE)
#endif

#if (CONFIG_COMMANDS & CFG_CMD_I2C)
#define CFG_I2C_SPEED            400
#define CFG_I2C_SLAVE            1
#define CFG_I2C_BUS              0
#define CFG_I2C_BUS_SELECT       1
#define CONFIG_DRIVER_OMAP34XX_I2C 1
#endif

/*
 *  Board NAND Info.
 */
#define CFG_NAND_ADDR NAND_BASE  /* physical address to access nand*/
#define CFG_NAND_BASE NAND_BASE  /* physical address to access nand at CS0*/
#define CFG_NAND_WIDTH_16

#define CFG_MAX_NAND_DEVICE      1 /* Max number of NAND devices */
#define SECTORSIZE               512

/* To use the 256/512 byte s/w ecc define CFG_SW_ECC_(256/512) */
/* Use the 512 byte ROM CODE HW ecc */
#define CFG_HW_ECC_ROMCODE     1

#define NAND_ALLOW_ERASE_ALL
#define ADDR_COLUMN              1
#define ADDR_PAGE                2
#define ADDR_COLUMN_PAGE         3

#define NAND_ChipID_UNKNOWN      0x00
#define NAND_MAX_FLOORS          1
#define NAND_MAX_CHIPS           1
#define NAND_NO_RB               1
#define CFG_NAND_WP

#if 0
    #undef CONFIG_ZERO_BOOTDELAY_CHECK
    #define CONFIG_BOOTDELAY         0  //disable bootdelay in release version
#else
    #define CONFIG_BOOTDELAY         3
#endif

#if 0
#define CONFIG_EXTRA_ENV_SETTINGS		\
"loadaddr=0x81c00000\0"			\
"nandloadaddr=0x81000000\0"			\
"console=ttyS3,115200n8\0"			\
"mmcroot=/dev/mmcblk0p2\0"			\
"nandroot=/dev/ram0\0"				\
"mmcargs=setenv bootargs console=${console} "	\
		"root=${mmcroot} rootdelay=2\0"	\
"nandargs=setenv bootargs console=${console} "	\
		"rootdelay=2\0"			\
"loaduimage=fatload mmc 0:1 ${loadaddr} uImage\0"\
"mmcboot=echo Booting from mmc ...;"		\
	" run mmcargs;"				\
	" bootm ${loadaddr}\0"			\
"nandboot=echo Booting from nand ...;"		\
		" nand unlock;"			\
		" nand read.i ${nandloadaddr}"	\
		" ${kernel_nand_offset}"	\
		" ${kernel_nand_size};"		\
		" run nandargs;"		\
		" bootm ${nandloadaddr}\0"	\
"autoboot=if mmc init 0; then" 			\
		" run loaduimage;" 		\
		" run mmcboot;" 		\
	" else run nandboot;"			\
	" fi;\0"				\

#define CONFIG_BOOTCOMMAND "run autoboot"

#ifdef NFS_BOOT_DEFAULTS
#define CONFIG_BOOTARGS "mem=64M console=ttyS3,115200n8 noinitrd root=/dev/nfs rw nfsroot=128.247.77.158:/home/a0384864/wtbu/rootfs ip=dhcp"
#else
#define CONFIG_BOOTARGS "root=/dev/ram0 rw mem=64M console=ttyS3,115200n8 initrd=0x80600000,8M ramdisk_size=8192"
#endif



#define CONFIG_NETMASK           255.255.254.0
#define CONFIG_IPADDR            128.247.77.90
#define CONFIG_SERVERIP          128.247.77.158
#define CONFIG_BOOTFILE          "uImage"
#endif
#define CONFIG_BOOTARGS "root=/dev/mmcblk0p2 rw rootdelay=1 console=ttyS2,115200n8 init=/init"
#define CONFIG_BOOTCOMMAND  "mmcinit 0;fatload mmc 0 0x80800000 uImage;bootm 0x80800000"
#define CONFIG_EXTRA_ENV_SETTINGS "boot_kunlun=mmcinit 0;fatload mmc 0 0x80800000 uImage;bootm 0x80800000\0"

#define CONFIG_AUTO_COMPLETE     1
/*
 * Miscellaneous configurable options
 */
#define V_PROMPT                 "OMAP36XX KUNLUN # "

#define CFG_LONGHELP             /* undef to save memory */
#define CFG_PROMPT               V_PROMPT
#define CFG_CBSIZE               256  /* Console I/O Buffer Size */
/* Print Buffer Size */
#define CFG_PBSIZE               (CFG_CBSIZE+sizeof(CFG_PROMPT)+16)
#define CFG_MAXARGS              24          /* max number of command args */
#define CFG_BARGSIZE             CFG_CBSIZE  /* Boot Argument Buffer Size */

#define CFG_MEMTEST_START        (OMAP34XX_SDRC_CS0)  /* memtest works on */
#define CFG_MEMTEST_END          (OMAP34XX_SDRC_CS0+SZ_31M)

#undef	CFG_CLKS_IN_HZ           /* everything, incl board info, in Hz */

#define CFG_LOAD_ADDR            (OMAP34XX_SDRC_CS0) /* default load address */

/* 2430 has 12 GP timers, they can be driven by the SysClk (12/13/19.2) or by
 * 32KHz clk, or from external sig. This rate is divided by a local divisor.
 */
#define V_PVT                    7

#define CFG_TIMERBASE            OMAP34XX_GPT2
#define CFG_PVT                  V_PVT  /* 2^(pvt+1) */
#define CFG_HZ                   ((V_SCLK)/(2 << CFG_PVT))

/*-----------------------------------------------------------------------
 * Stack sizes
 *
 * The stack sizes are set up in start.S using the settings below
 */
#define CONFIG_STACKSIZE	SZ_128K /* regular stack */
#ifdef CONFIG_USE_IRQ
#define CONFIG_STACKSIZE_IRQ	SZ_4K   /* IRQ stack */
#define CONFIG_STACKSIZE_FIQ	SZ_4K   /* FIQ stack */
#endif

/*-----------------------------------------------------------------------
 * Physical Memory Map
 */
#define CONFIG_NR_DRAM_BANKS	2       /* CS1 may or may not be populated */
#define PHYS_SDRAM_1		OMAP34XX_SDRC_CS0
#if 0
#define PHYS_SDRAM_1_SIZE	SZ_128M            /* at least 32 meg */
#define PHYS_SDRAM_2		OMAP34XX_SDRC_CS1
#endif
/* SDRAM Bank Allocation method */
/*#define SDRC_B_R_C		1 */
/*#define SDRC_B1_R_B0_C	1 */
#define SDRC_R_B_C		1

/*-----------------------------------------------------------------------
 * FLASH and environment organization
 */

/* **** PISMO SUPPORT *** */

/* Configure the PISMO */
/** REMOVE ME ***/
#define PISMO1_NOR_SIZE_SDPV2	GPMC_SIZE_128M
#define PISMO1_NOR_SIZE		GPMC_SIZE_64M

#define PISMO1_NAND_SIZE	GPMC_SIZE_128M
#define PISMO1_ONEN_SIZE	GPMC_SIZE_128M
#define DBG_MPDB_SIZE		GPMC_SIZE_16M
#define PISMO2_SIZE		0

#define CFG_MAX_FLASH_SECT	(520)		/* max number of sectors on one chip */
#define CFG_MAX_FLASH_BANKS      2		/* max number of flash banks */
#define CFG_MONITOR_LEN		SZ_256K 	/* Reserve 2 sectors */

#define PHYS_FLASH_SIZE_SDPV2	SZ_128M
#define PHYS_FLASH_SIZE		SZ_32M

#define CFG_FLASH_BASE		boot_flash_base
#define PHYS_FLASH_SECT_SIZE	boot_flash_sec
/* Dummy declaration of flash banks to get compilation right */
#define CFG_FLASH_BANKS_LIST	{0, 0}

#define CFG_MONITOR_BASE	CFG_FLASH_BASE /* Monitor at start of flash */

#if 1
#define CFG_ENV_IS_IN_NAND	1
#define ENV_IS_VARIABLE		1
#else
#define CFG_ENV_IS_NOWHERE	1
#endif

#ifdef CONFIG_OPTIONAL_NOR_POPULATED
# define CFG_ENV_IS_IN_FLASH	1
#endif

#define SMNAND_ENV_OFFSET	0x1c0000  /* environment starts here  */
#define SMNAND_ENV_LENGTH	0x0040000 /* environment starts here  */

#define CFG_ENV_SECT_SIZE	boot_flash_sec
#define CFG_ENV_OFFSET		boot_flash_off
#define CFG_ENV_ADDR		boot_flash_env_addr


/*-----------------------------------------------------------------------
 * CFI FLASH driver setup
 */
#ifndef CONFIG_OPTIONAL_NOR_POPULATED
#define CFG_NO_FLASH	1            /* Disable NOR Flash support */
#else
#define CFG_FLASH_CFI		1    /* Flash memory is CFI compliant */
#define CFG_FLASH_CFI_DRIVER	1    /* Use drivers/cfi_flash.c */
#if (!ENV_IS_VARIABLE)
/* saveenv fails when this variable is defined.
   If env is variable, do not use buffered writes */
#define CFG_FLASH_USE_BUFFER_WRITE 1    /* Use buffered writes (~10x faster) */
#endif
#define CFG_FLASH_PROTECTION	1    /* Use hardware sector protection */
#define CFG_FLASH_QUIET_TEST	1    /* Dont crib abt missing chips */
#define CFG_FLASH_CFI_WIDTH	0x02
/* timeout values are in ticks */
#define CFG_FLASH_ERASE_TOUT	(100*CFG_HZ) /* Timeout for Flash Erase */
#define CFG_FLASH_WRITE_TOUT	(100*CFG_HZ) /* Timeout for Flash Write */

/* Flash banks JFFS2 should use */
#define CFG_MAX_MTD_BANKS	(CFG_MAX_FLASH_BANKS+CFG_MAX_NAND_DEVICE)
#define CFG_JFFS2_MEM_NAND
#define CFG_JFFS2_FIRST_BANK	CFG_MAX_FLASH_BANKS /* use flash_info[2] */
#define CFG_JFFS2_NUM_BANKS	1
#define CONFIG_LED_INFOnand_read_buf16
#define CONFIG_LED_LEN		16
#endif  /* optional NOR flash */

#ifndef __ASSEMBLY__
extern unsigned int nand_cs_base;
extern unsigned int boot_flash_base;
extern volatile unsigned int boot_flash_env_addr;
extern unsigned int boot_flash_off;
extern unsigned int boot_flash_sec;
extern unsigned int boot_flash_type;
#endif

#define WRITE_NAND_COMMAND(d, adr) __raw_writew(d, (nand_cs_base + GPMC_NAND_CMD))
#define WRITE_NAND_ADDRESS(d, adr) __raw_writew(d, (nand_cs_base + GPMC_NAND_ADR))
#define WRITE_NAND(d, adr) __raw_writew(d, (nand_cs_base + GPMC_NAND_DAT))
#define READ_NAND(adr) __raw_readw((nand_cs_base + GPMC_NAND_DAT))

/* Other NAND Access APIs */
#define NAND_WP_OFF()  do {*(volatile u32 *)(GPMC_CONFIG) |= 0x00000010;} while(0)
#define NAND_WP_ON()  do {*(volatile u32 *)(GPMC_CONFIG) &= ~0x00000010;} while(0)
#define NAND_DISABLE_CE(nand)
#define NAND_ENABLE_CE(nand)
#define NAND_WAIT_READY(nand)	udelay(10)

/* Fastboot variables */
/* Not used */
#define CFG_FASTBOOT_TRANSFER_BUFFER FAST_BOOT_BUFFER_BASEADDR
#define CFG_FASTBOOT_TRANSFER_BUFFER_SIZE FAST_BOOT_BUFFER_SIZE
#define CFG_FASTBOOT_PREBOOT_KEYS         2
#if defined(CONFIG_3630KUNLUN_KL9C) || defined(CONFIG_3630KUNLUN_P2)
#define CFG_FASTBOOT_PREBOOT_KEY1         27 /* 'Enter' */
#define CFG_FASTBOOT_PREBOOT_KEY2         11 /* 'Home' */
/* Haier N710E. Combo: Volume Down + Camera. */
#elif defined(CONFIG_3630KUNLUN_N710E)
#define CFG_FASTBOOT_PREBOOT_KEY1	8  /* Volume Down (C1,R0) */
#define CFG_FASTBOOT_PREBOOT_KEY2	16 /* Camera (C2, R0) */
#else
#define CFG_FASTBOOT_PREBOOT_KEY1         0x02 /* 'Camera' */
#define CFG_FASTBOOT_PREBOOT_KEY2         0x09 /* 'Home' */
#endif
#define CFG_FASTBOOT_PREBOOT_INITIAL_WAIT (0)
#define CFG_FASTBOOT_PREBOOT_LOOP_MAXIMUM (1)
#define CFG_FASTBOOT_PREBOOT_LOOP_WAIT    (0)

#define CFG_HOME_KEY_PRESSED_KEYS         2
#if defined(CONFIG_3630KUNLUN_KL9C) || defined(CONFIG_3630KUNLUN_P2)
#define CFG_HOME_KEY_PRESSED_KEY1         11 /* 'Home' */
#define CFG_HOME_KEY_PRESSED_KEY2         10 /* 'Vol-down' */
#define CFG_HOME_KEY_PRESSED_KEY3         2/* 'Vol-up' */
#elif defined(CONFIG_3630KUNLUN_N710E)
#define CFG_HOME_KEY_PRESSED_KEY1         16 /* 'camera' */
#define CFG_HOME_KEY_PRESSED_KEY2         0  /* 'Vol-up' */
#define CFG_HOME_KEY_PRESSED_KEY3         8	/* 'Vol-down' */
#else
#define CFG_HOME_KEY_PRESSED_KEY1         0x09 /* 'Home' */
#define CFG_HOME_KEY_PRESSED_KEY2         0x12 /* 'Vol-down' */
#define CFG_HOME_KEY_PRESSED_KEY3         0x0a/* 'Vol-up' */
#endif
#define CFG_HOME_KEY_PRESSED_INITIAL_WAIT (0)
#define CFG_HOME_KEY_PRESSED_LOOP_MAXIMUM (1)
#define CFG_HOME_KEY_PRESSED_LOOP_WAIT    (0)

/* Yaffs variables */
#define CFG_NAND_YAFFS_WRITE

/* kunlun Battery charging enable */
#define CFG_BATTERY_CHARGING

/* kunlun Battery threshold */
#define CFG_CHG_LOW_BAT  3200 /*3.20V*/
#define CFG_LOW_BAT 3500 /* 3.5V */
#define CFG_BAT_CHG 4000 /* 4.0V */

/* kunlun charger tries */
#define CFG_CHARGER_TRIES_MAX 10

/* Command shell */
#define CFG_HUSH_PARSER
#define CFG_PROMPT_HUSH_PS2 "> "

/* Clock command */
#define CONFIG_CMD_CLOCK		1
#define CONFIG_CMD_CLOCK_INFO_CPU	1

/* Voltage command */
#define CONFIG_CMD_VOLTAGE		1

#endif                           /* __CONFIG_H */
