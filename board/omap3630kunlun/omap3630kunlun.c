/*
 * Copyright (c) 2010 Wind River Systems, Inc.
 *
 * (C) Copyright 2009
 * Texas Instruments, <www.ti.com>
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/arch/bits.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/clocks.h>
#include <asm/arch/mem.h>
#include <i2c.h>
#include <asm/mach-types.h>
#include <linux/mtd/nand_ecc.h>
#include <twl4030.h>

#include "../../cpu/omap3/mmc_host_def.h"

#include "gpio.h"
#include "lg_lcd.h"

#define mdelay(n)       udelay((n)*1000)
int get_boot_type(void);
void v7_flush_dcache_all(int, int);
void l2cache_enable(void);
void setup_auxcr(int, int);
void eth_init(void *);


/*****************************************
 * Routine: cdmagps_init
 * Description: Init GPS for CP.
 *****************************************/
void cdmagps_init(void)
{
    gpio_t *gpio1_base = (gpio_t *)OMAP34XX_GPIO1_BASE;
    //gpio_t *gpio5_base = (gpio_t *)OMAP34XX_GPIO5_BASE;
    /*enable GPIO 1 function clock*/
    puts("Enable GPS in U-BOOT\n");
    sr32(0x48004c00,3,1,1);
    /*enable GPIO 5 interface clock*/
    sr32(0x48004c10,3,1,1);
    /*enable GPIO 5 function clock*/
    sr32(0x48005000,16,1,1);
    /*enable GPIO 5 interface clock*/
    sr32(0x48005010,16,1,1);
    //sr32((u32)&gpio1_base->oe, 16, 1, 0);   /* GPIO 16 OE   GPS ENABLE*/
    udelay(5000);
    set_gpio_dataout(16, 1);

    return;
}

static void omap_pbias_init(void)
{
    uchar buf[1];
    unsigned int temp = 0;
    gpio_t *gpio1_base = (gpio_t *)OMAP34XX_GPIO1_BASE;
    gpio_t *gpio6_base = (gpio_t *)OMAP34XX_GPIO6_BASE;

    /*init gps for cp*/
    cdmagps_init();

    temp=CONTROL_PBIAS_LITE;
    /* set PWRDNZ1 to low, VMODE to high, for GPIO126-129 */
    CONTROL_PBIAS_LITE= ((temp & ~(1 << 9)) | (1 << 8));

    /* Enable VSIM 3.0V */
    buf[0] = 0xE0;
    i2c_write(0x4b,0x92,1,buf,sizeof(buf));

    buf[0] = 0x05;
    i2c_write(0x4b,0x95,1,buf,sizeof(buf));

    udelay(50000);

    temp=CONTROL_PBIAS_LITE;
    //printf("CONTROL_PBIAS_LITE:%x\n",temp);
    CONTROL_PBIAS_LITE= (temp | (1 << 9));

    temp = CONTROL_WKUP_CTRL;
    CONTROL_WKUP_CTRL = (temp | (1 << 6));

    /* Enable GPIO1 CLK */
    sr32(CM_FCLKEN_WKUP, 3, 1, 1);
    sr32(CM_ICLKEN_WKUP, 3, 1, 1);

    /* Enable GPIO2 CLK */
    sr32(CM_FCLKEN_PER, CLKEN_PER_EN_GPIO2_BIT, 1, 1);
    sr32(CM_ICLKEN_PER, CLKEN_PER_EN_GPIO2_BIT, 1, 1);

    /* Enable GPIO3 CLK */
    sr32(CM_FCLKEN_PER, CLKEN_PER_EN_GPIO3_BIT, 1, 1);
    sr32(CM_ICLKEN_PER, CLKEN_PER_EN_GPIO3_BIT, 1, 1);

    /* Enable GPIO4 CLK */
    sr32(CM_FCLKEN_PER, CLKEN_PER_EN_GPIO4_BIT, 1, 1);
    sr32(CM_ICLKEN_PER, CLKEN_PER_EN_GPIO4_BIT, 1, 1);

    /* Enable GPIO5 CLK */
    sr32(CM_FCLKEN_PER, CLKEN_PER_EN_GPIO5_BIT, 1, 1);
    sr32(CM_ICLKEN_PER, CLKEN_PER_EN_GPIO5_BIT, 1, 1);

    /* Enable GPIO6 CLK */
    sr32(CM_FCLKEN_PER, CLKEN_PER_EN_GPIO6_BIT, 1, 1);
    sr32(CM_ICLKEN_PER, CLKEN_PER_EN_GPIO6_BIT, 1, 1);

    /*config USB2_TXDAT GPIO177 input, USB2_TXSE0 GPIO29 input*/
    sr32((u32)&gpio1_base->oe, 29, 1, 1);   /* GPIO 29 input*/
    sr32((u32)&gpio6_base->oe, 17, 1, 1);   /* GPIO177 input*/
    set_gpio_dataout(149, 0); //MDM_RST
    set_gpio_dataout(126, 0); //MDM_PWR_EN
    return ;
}

void omap_cbp_power_on(void)
{
    gpio_t *gpio5_base = (gpio_t *)OMAP34XX_GPIO5_BASE;
    gpio_t *gpio4_base = (gpio_t *)OMAP34XX_GPIO4_BASE;
    gpio_t *gpio1_base = (gpio_t *)OMAP34XX_GPIO1_BASE;

    puts("CBP POWER ON\n");

    set_gpio_dataout(21,0);
    set_gpio_dataout(127,1); //AP_RDY
    set_gpio_dataout(149,0); //MDM_RST

    /*simulate a power key pressed to power on CP*/
    set_gpio_dataout(126,1); //MDM_PWR_EN
    mdelay(500);
    set_gpio_dataout(126,0); //MDM_PWR_EN

    return;
}

void omap_cbp_power_off(void)
{
    gpio_t *gpio5_base = (gpio_t *)OMAP34XX_GPIO5_BASE;
    gpio_t *gpio4_base = (gpio_t *)OMAP34XX_GPIO4_BASE;
    //gpio_t *gpio1_base = (gpio_t *)OMAP34XX_GPIO1_BASE;

    set_gpio_dataout(127,1); //AP_RDY
    set_gpio_dataout(126,0); //MDM_PWR_EN
    udelay(5000);

    set_gpio_dataout(149,1); //MDM_RST
    mdelay(100);
    set_gpio_dataout(149,0);
    return;
}

#if defined(CONFIG_3630KUNLUN_WUDANG)
void turn_kpd_led(int on)
{
     gpio_t *gpio3_base = (gpio_t *)OMAP34XX_GPIO3_BASE;

     if(on){
	 set_gpio_dataout(92,1);
     }else{
        set_gpio_dataout(92,0);
     }
}

void shine_kpd_led(int delay)
{
     gpio_t *gpio3_base = (gpio_t *)OMAP34XX_GPIO3_BASE;
     static int sw = 0;


     set_gpio_dataout(92,0);
     mdelay(delay);

     set_gpio_dataout(92,1);
     mdelay(delay);

     set_gpio_dataout(92,0);
     mdelay(delay);
}

void omap_ste_power_on(void)
{
    gpio_t *gpio2_base = (gpio_t *)OMAP34XX_GPIO2_BASE;
    gpio_t *gpio3_base = (gpio_t *)OMAP34XX_GPIO3_BASE;
    gpio_t *gpio5_base = (gpio_t *)OMAP34XX_GPIO5_BASE;
    gpio_t *gpio6_base = (gpio_t *)OMAP34XX_GPIO6_BASE;

    set_gpio_dataout(162,1);

    set_gpio_dataout(159,0);
    mdelay(300);

    set_gpio_dataout(159,1);
    mdelay(300);

    set_gpio_dataout(159,0);

    set_gpio_dataout(92,0);

    set_gpio_dataout(41,0);

    set_gpio_dataout(152,0);

    set_gpio_dataout(61,0);


    return ;
}

void omap_ste_power_off(void)
{
    gpio_t *gpio5_base = (gpio_t *)OMAP34XX_GPIO5_BASE;
    gpio_t *gpio6_base = (gpio_t *)OMAP34XX_GPIO6_BASE;

    /* GPIO 159 OE*/
    /* GPIO 159 ouput 0, STE power on */
    set_gpio_dataout(159,0);
    /* GPIO 162 OE*/
    /* GPIO 162 ouput 0, STE RST */
    set_gpio_dataout(162,0);
    mdelay(300);
    /* GPIO 162 ouput 1, STE RST */
    set_gpio_dataout(162,1);

    return;
}
#endif

/*******************************************************
 * Routine: delay
 * Description: spinning delay to use before udelay works
 ******************************************************/
static inline void delay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"
			  "bne 1b":"=r" (loops):"0"(loops));
}

/*****************************************
 * Routine: board_init
 * Description: Early hardware init.
 *****************************************/
int board_init(void)
{
	DECLARE_GLOBAL_DATA_PTR;

	gpmc_init();		/* in SRAM or SDRAM, finish GPMC */

	gd->bd->bi_arch_number = MACH_TYPE_OMAP_KUNLUN; /* Linux mach id*/

	gd->bd->bi_boot_params = (OMAP34XX_SDRC_CS0 + 0x100); /* boot param addr */

	return 0;
}

static int home_key_pressed(void)
{
#if (defined(CONFIG_TWL4030_KEYPAD) && (CONFIG_TWL4030_KEYPAD))
        int i;
        unsigned char key1, key2;
        int keys;
        udelay(CFG_HOME_KEY_PRESSED_INITIAL_WAIT);
        for (i = 0; i < CFG_HOME_KEY_PRESSED_LOOP_MAXIMUM; i++) {
                key1 = key2 = 0;
                keys = twl4030_keypad_keys_pressed(&key1, &key2);
                if ((1 == CFG_HOME_KEY_PRESSED_KEYS) &&
                    (1 == keys)) {
					if ((CFG_HOME_KEY_PRESSED_KEY1 == key1) ||
                        (CFG_HOME_KEY_PRESSED_KEY1 == key2))
						return 1;
                } else if ((2 == CFG_HOME_KEY_PRESSED_KEYS) &&
                           (2 == keys)) {

                    if ((CFG_HOME_KEY_PRESSED_KEY1 == key1) &&
                            (CFG_HOME_KEY_PRESSED_KEY2 == key2))
						return 1;

                    if ((CFG_HOME_KEY_PRESSED_KEY1 == key2) &&
                            (CFG_HOME_KEY_PRESSED_KEY2 == key1))
						return 1;
                }
                udelay(CFG_HOME_KEY_PRESSED_LOOP_WAIT);
        }
#endif
        return 0;
}
#define VIA_BOOT_BYPASS_KEY
#ifdef VIA_BOOT_BYPASS_KEY
static int bypass_key_pressed(void)
{
    /*check home key and vol*/
	int i;
        unsigned char key1, key2;
        int keys;
        udelay(CFG_HOME_KEY_PRESSED_INITIAL_WAIT);
		//printf("boot_bypass_by_key\n");
        for (i = 0; i < CFG_HOME_KEY_PRESSED_LOOP_MAXIMUM; i++) {
                key1 = key2 = 0;
                keys = twl4030_keypad_keys_pressed(&key1, &key2);
                if (2 == keys) {
					printf("boot_bypass_by_key:Key 1:0x%x Key 2:0x%x\n",key1,key2);
#if defined(CONFIG_3630KUNLUN_N710E)	
					if ((CFG_HOME_KEY_PRESSED_KEY2 == key1) &&
							(CFG_HOME_KEY_PRESSED_KEY3 == key2))
						return 1;			
					if ((CFG_HOME_KEY_PRESSED_KEY2 == key2) &&
							(CFG_HOME_KEY_PRESSED_KEY3 == key1))
						return 1;
#else
					if ((CFG_HOME_KEY_PRESSED_KEY1 == key1) &&
                           (CFG_HOME_KEY_PRESSED_KEY3 == key2))
                                return 1;
                    if ((CFG_HOME_KEY_PRESSED_KEY1 == key2) &&
                           (CFG_HOME_KEY_PRESSED_KEY3 == key1))
                                return 1;
#endif
                }
                udelay(CFG_HOME_KEY_PRESSED_LOOP_WAIT);
        }
       return 0;
}
#endif

/*****************************************
 * Routine: secure_unlock
 * Description: Setup security registers for access
 * (GP Device only)
 *****************************************/
void secure_unlock_mem(void)
{
	/* Permission values for registers -Full fledged permissions to all */
	#define UNLOCK_1 0xFFFFFFFF
	#define UNLOCK_2 0x00000000
	#define UNLOCK_3 0x0000FFFF

	/* Protection Module Register Target APE (PM_RT)*/
	__raw_writel(UNLOCK_1, RT_REQ_INFO_PERMISSION_1);
	__raw_writel(UNLOCK_1, RT_READ_PERMISSION_0);
	__raw_writel(UNLOCK_1, RT_WRITE_PERMISSION_0);
	__raw_writel(UNLOCK_2, RT_ADDR_MATCH_1);

	__raw_writel(UNLOCK_3, GPMC_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, GPMC_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, GPMC_WRITE_PERMISSION_0);

	__raw_writel(UNLOCK_3, OCM_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, OCM_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, OCM_WRITE_PERMISSION_0);
	__raw_writel(UNLOCK_2, OCM_ADDR_MATCH_2);

	/* IVA Changes */
	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_0);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_0);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_0);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_1);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_1);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_1);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_2);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_2);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_2);

	__raw_writel(UNLOCK_3, IVA2_REQ_INFO_PERMISSION_3);
	__raw_writel(UNLOCK_3, IVA2_READ_PERMISSION_3);
	__raw_writel(UNLOCK_3, IVA2_WRITE_PERMISSION_3);

	__raw_writel(UNLOCK_1, SMS_RG_ATT0); /* SDRC region 0 public */
}


/**********************************************************
 * Routine: secureworld_exit()
 * Description: If chip is EMU and boot type is external
 *		configure secure registers and exit secure world
 *  general use.
 ***********************************************************/
void secureworld_exit(void)
{
	unsigned long i;

	/* configrue non-secure access control register */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c1, 2":"=r" (i));
	/* enabling co-processor CP10 and CP11 accesses in NS world */
	__asm__ __volatile__("orr %0, %0, #0xC00":"=r"(i));
	/* allow allocation of locked TLBs and L2 lines in NS world */
	/* allow use of PLE registers in NS world also */
	__asm__ __volatile__("orr %0, %0, #0x70000":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c1, 2":"=r" (i));

	/* Enable ASA and IBE in ACR register */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c0, 1":"=r" (i));
	__asm__ __volatile__("orr %0, %0, #0x50":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c0, 1":"=r" (i));

	/* Exiting secure world */
	__asm__ __volatile__("mrc p15, 0, %0, c1, c1, 0":"=r" (i));
	__asm__ __volatile__("orr %0, %0, #0x31":"=r"(i));
	__asm__ __volatile__("mcr p15, 0, %0, c1, c1, 0":"=r" (i));
}

/**********************************************************
 * Routine: try_unlock_sram()
 * Description: If chip is GP/EMU(special) type, unlock the SRAM for
 *  general use.
 ***********************************************************/
void try_unlock_memory(void)
{
	int mode;
	int in_sdram = running_in_sdram();

	/* if GP device unlock device SRAM for general use */
	/* secure code breaks for Secure/Emulation device - HS/E/T*/
	mode = get_device_type();
	if (mode == GP_DEVICE) {
		secure_unlock_mem();
	}
	/* If device is EMU and boot is XIP external booting
	 * Unlock firewalls and disable L2 and put chip
	 * out of secure world
	 */
	/* Assuming memories are unlocked by the demon who put us in SDRAM */
	if ((mode <= EMU_DEVICE) && (get_boot_type() == 0x1F)
		&& (!in_sdram)) {
		secure_unlock_mem();
		secureworld_exit();
	}

	return;
}

/**********************************************************
 * Routine: s_init
 * Description: Does early system init of muxing and clocks.
 * - Called path is with SRAM stack.
 **********************************************************/
void s_init(void)
{
	int i;
	int external_boot = 0;
	int in_sdram = running_in_sdram();

#ifdef CONFIG_3430VIRTIO
	in_sdram = 0;  /* allow setup from memory for Virtio */
#endif
	watchdog_init();

	external_boot = (get_boot_type() == 0x1F) ? 1 : 0;
	/* Right now flushing at low MPU speed. Need to move after clock init */
	v7_flush_dcache_all(get_device_type(), external_boot);

	try_unlock_memory();

#ifdef CONFIG_3430_AS_3410
	/* setup the scalability control register for
	 * 3430 to work in 3410 mode
	 */
	__raw_writel(0x5A80, CONTROL_SCALABLE_OMAP_OCP);
#endif

	if (cpu_is_3410()) {
		/* Lock down 6-ways in L2 cache so that effective size of L2 is 64K */
		__asm__ __volatile__("mov %0, #0xFC":"=r" (i));
		__asm__ __volatile__("mcr p15, 1, %0, c9, c0, 0":"=r" (i));
	}

#ifndef CONFIG_ICACHE_OFF
	icache_enable();
#endif

#ifdef CONFIG_L2_OFF
	l2cache_disable();
#else
	l2cache_enable();
#endif
	set_muxconf_regs();
	delay(100);

	/* Writing to AuxCR in U-boot using SMI for GP/EMU DEV */
	/* Currently SMI in Kernel on ES2 devices seems to have an isse
	 * Once that is resolved, we can postpone this config to kernel
	 */
	setup_auxcr(get_device_type(), external_boot);

	prcm_init();

	per_clocks_enable();

	if (!in_sdram)
		sdrc_init();
}

/*******************************************************

	do my test, all dirty jobs here

*********************************************************/
extern void kunlun_lcd_init(void);

void do_my_test(void) {

	enable_gpio_bank(1);
	enable_gpio_bank(2);
	enable_gpio_bank(3);
	enable_gpio_bank(4);
	enable_gpio_bank(5);
	enable_gpio_bank(6);

	set_gpio_dataout(69, 1);
	udelay(5000);

	lg_3wire_init( 	
		CONTROL_PADCONF_McSPI1_CS0, 174,
		CONTROL_PADCONF_McSPI1_CLK, 171,
		CONTROL_PADCONF_McSPI1_SIMO, 172,
		0, 0);
	
	lg_3wire_init_regs(0); // normal white

	kunlun_lcd_init();
}

/*******************************************************
 * Routine: misc_init_r
 * Description: Init ethernet (done here so udelay works)
 ********************************************************/
int misc_init_r(void)
{
	
#if 0
#ifdef CONFIG_3430ZOOM2
	{
		/* GPIO LEDs
		   154 blue , bank 5, index 26
		   173 red  , bank 6, index 13
		    61 blue2, bank 2, index 29
		   94 green,  bank 3, index 30

		   GPIO to query for debug board
		   158 db board query, bank 5, index 30
		   This is an input only, no additional setup is needed */

		gpio_t *gpio2_base = (gpio_t *)OMAP34XX_GPIO2_BASE;
		gpio_t *gpio5_base = (gpio_t *)OMAP34XX_GPIO5_BASE;
		gpio_t *gpio6_base = (gpio_t *)OMAP34XX_GPIO6_BASE;
		gpio_t *gpio3_base = (gpio_t *)OMAP34XX_GPIO3_BASE;

		/* Configure GPIOs to output */
		sr32((u32)&gpio2_base->oe, 29, 1, 0);
		sr32((u32)&gpio5_base->oe, 26, 1, 0);
		sr32((u32)&gpio6_base->oe, 13, 1, 0);
		sr32((u32)&gpio3_base->oe, 30, 1, 0);

		sr32((u32)&gpio6_base->cleardataout, 13, 1, 1); /* red off */
		sr32((u32)&gpio5_base->setdataout, 26, 1, 1);   /* blue on */
		sr32((u32)&gpio2_base->setdataout, 29, 1, 1);   /* blue 2 on */
		sr32((u32)&gpio3_base->cleardataout, 30, 1, 1); /* green off */
	}

#endif
#ifdef CONFIG_DRIVER_OMAP34XX_I2C
	unsigned char data;

	i2c_init(CFG_I2C_SPEED, CFG_I2C_SLAVE);
	twl4030_usb_init();
	twl4030_power_reset_init();
	/* see if we need to activate the power button startup */
	char *s = getenv("pbboot");
	if (s) {
		/* figure out why we have booted */
		i2c_read(0x4b, 0x3a, 1, &data, 1);

		/* if status is non-zero, we didn't transition
		 * from WAIT_ON state
		 */
		if (data) {
			printf("Transitioning to Wait State (%x)\n", data);

			/* clear status */
			data = 0;
			i2c_write(0x4b, 0x3a, 1, &data, 1);

			/* put PM into WAIT_ON state */
			data = 0x01;
			i2c_write(0x4b, 0x46, 1, &data, 1);

			/* no return - wait for power shutdown */
			while (1) {;}
		}
		printf("Transitioning to Active State (%x)\n", data);

		/* turn on long pwr button press reset*/
		data = 0x40;
		i2c_write(0x4b, 0x46, 1, &data, 1);
		printf("Power Button Active\n");
	}
#endif

#if defined(CONFIG_TWL4030_KEYPAD)
    twl4030_keypad_init();
#endif
    ether_init();	/* better done here so timers are init'ed */
    dieid_num_r();

    if (home_key_pressed()){
        setenv("HOME_Key_Pressed", "1");
    } else {
        setenv("HOME_Key_Pressed", "0");
#ifdef VIA_BOOT_BYPASS_KEY
        if(bypass_key_pressed()) {
            u8 tmp[512];
            int val;
            val=getenv_r ("nandargs", tmp, sizeof (tmp));
            printf("old nandargs:%s\n",tmp);
            strcat(tmp," enable_bypass");
            printf("new nandargs:%s\n",tmp);
            setenv("nandargs", tmp);
        }
#endif
    }
    /*init the pbias and Vsim*/
    omap_pbias_init();
    /* Added by sguan, power on the CDMA */
#if defined(CONFIG_BOOTCASE_CHECK)
    extern int ets_process(void);
    extern char *bootcase_name;
    if(bootcase_name && strncmp(bootcase_name, "charger", strlen("charger")))
#endif
        omap_cbp_power_on();

#if defined(CONFIG_3630KUNLUN_WUDANG)
#ifdef CONFIG_SUPPORT_STE_MDM
    omap_ste_power_on();
#endif
    //shine_kpd_led(500);
#endif

#if defined(CONFIG_BOOTCASE_CHECK) && defined(CONFIG_ADC_CALIBRATION) && defined(CONFIG_SILENT_CONSOLE)
/*Calibrate the ADC if boot by fixture*/
    extern int ets_process(void);
    extern char *bootcase_name;
    if(bootcase_name && !strncmp(bootcase_name, "fixture", strlen("fixture"))){
        puts("Enter calibration mode...\n");
        ets_process();
        puts("Exit calibration mode...\n");
    }
#endif
#endif 	/* UGlee, remove all operations here */
	do_my_test();
	return (0);
}

/******************************************************
 * Routine: wait_for_command_complete
 * Description: Wait for posting to finish on watchdog
 ******************************************************/
void wait_for_command_complete(unsigned int wd_base)
{
	int pending = 1;
	do {
		pending = __raw_readl(wd_base + WWPS);
	} while (pending);
}

/****************************************
 * Routine: watchdog_init
 * Description: Shut down watch dogs
 *****************************************/
void watchdog_init(void)
{
	/* There are 3 watch dogs WD1=Secure, WD2=MPU, WD3=IVA. WD1 is
	 * either taken care of by ROM (HS/EMU) or not accessible (GP).
	 * We need to take care of WD2-MPU or take a PRCM reset.  WD3
	 * should not be running and does not generate a PRCM reset.
	 */

	sr32(CM_FCLKEN_WKUP, 5, 1, 1);
	sr32(CM_ICLKEN_WKUP, 5, 1, 1);
	wait_on_value(BIT5, 0x20, CM_IDLEST_WKUP, 5); /* some issue here */

	__raw_writel(WD_UNLOCK1, WD2_BASE + WSPR);
	wait_for_command_complete(WD2_BASE);
	__raw_writel(WD_UNLOCK2, WD2_BASE + WSPR);
}

/*******************************************************************
 * Routine:ether_init
 * Description: take the Ethernet controller out of reset and wait
 *  		   for the EEPROM load to complete.
 ******************************************************************/
void ether_init(void)
{
#ifdef CONFIG_DRIVER_LAN91C96
	int cnt = 20;

	__raw_writew(0x0, LAN_RESET_REGISTER);
	do {
		__raw_writew(0x1, LAN_RESET_REGISTER);
		udelay(100);
		if (cnt == 0)
			goto h4reset_err_out;
		--cnt;
	} while (__raw_readw(LAN_RESET_REGISTER) != 0x1);

	cnt = 20;

	do {
		__raw_writew(0x0, LAN_RESET_REGISTER);
		udelay(100);
		if (cnt == 0)
			goto h4reset_err_out;
		--cnt;
	} while (__raw_readw(LAN_RESET_REGISTER) != 0x0000);
	udelay(1000);

	*((volatile unsigned char *)ETH_CONTROL_REG) &= ~0x01;
	udelay(1000);

      h4reset_err_out:
	return;

#elif CONFIG_3430LABRADOR
	DECLARE_GLOBAL_DATA_PTR;
	eth_init(gd->bd);
#endif
}

/**********************************************
 * Routine: dram_init
 * Description: sets uboots idea of sdram size
 **********************************************/
int dram_init(void)
{
    #define NOT_EARLY 0
    DECLARE_GLOBAL_DATA_PTR;
	unsigned int size0 = 0, size1 = 0;
	u32 mtype, btype;

	btype = get_board_type();
	mtype = get_mem_type();
#ifndef CONFIG_3430ZEBU
	/* fixme... dont know why this func is crashing in ZeBu */
	display_board_info(btype);
#endif
    /* If a second bank of DDR is attached to CS1 this is
     * where it can be started.  Early init code will init
     * memory on CS0.
     */
	if ((mtype == DDR_COMBO) || (mtype == DDR_STACKED)) {
		do_sdrc_init(SDRC_CS1_OSET, NOT_EARLY);
	}
	size0 = get_sdr_cs_size(SDRC_CS0_OSET);
	size1 = get_sdr_cs_size(SDRC_CS1_OSET);

	gd->bd->bi_dram[0].start = PHYS_SDRAM_1;
	gd->bd->bi_dram[0].size = size0;
	gd->bd->bi_dram[1].start = PHYS_SDRAM_1+size0;
	gd->bd->bi_dram[1].size = size1;

	return 0;
}

#define 	MUX_VAL(OFFSET,VALUE)\
		__raw_writew((VALUE), OMAP34XX_CTRL_BASE + (OFFSET));

#define		CP(x)	(CONTROL_PADCONF_##x)
/***********************************************/
//removed by myself
#if 0
#if defined(CONFIG_3630KUNLUN)
#undef MUX_DEFAULT_ES2
#include "kunlun_mux_table.h"
#endif
#endif
/***********************************************/
#if defined(CONFIG_3630KUNLUN_KL9C)
#undef MUX_DEFAULT_ES2
#include "kl9c_mux_table.h"
#endif
#if defined(CONFIG_3630KUNLUN_WUDANG)
#undef MUX_DEFAULT_ES2
#include "wudang_mux_table.h"
#endif

#if defined(CONFIG_3630KUNLUN_P2) || defined(CONFIG_3630PUMA_V1)
#undef MUX_DEFAULT_ES2
#include "kunlunp2_mux_table.h"
#endif

#if defined(CONFIG_3630KUNLUN_N710E)
#undef MUX_DEFAULT_ES2
#include "kunlun_n710e_mux_table.h"
#endif

/**********************************************************
 * Routine: set_muxconf_regs
 * Description: Setting up the configuration Mux registers
 *              specific to the hardware. Many pins need
 *              to be moved from protect to primary mode.
 *********************************************************/
void set_muxconf_regs(void)
{
	MUX_DEFAULT_ES2();
	/* Set ZOOM2 specific mux */
#ifdef CONFIG_3430ZOOM2
		/* IDCC modem Power On */
	MUX_VAL(CP(CAM_D11),        (IEN  | PTU | EN | M4)) /*gpio_110*/
	MUX_VAL(CP(CAM_D4),         (IEN  | PTU | EN | M4)) /*GPIO_103 */

		/* GPMC CS7 has LAN9211 device */
	MUX_VAL(CP(GPMC_nCS7),      (IDIS | PTU | EN  | M0)) /*GPMC_nCS7 lab*/
	MUX_VAL(CP(McBSP1_DX),      (IEN  | PTD | DIS | M4)) /*gpio_158 lab: for LAN9221 on zoom2*/
	MUX_VAL(CP(McSPI1_CS2),     (IEN  | PTD | EN  | M0)) /*mcspi1_cs2 zoom2*/

		/* GPMC CS3 has Serial TL16CP754C device */
	MUX_VAL(CP(GPMC_nCS3),      (IDIS | PTU | EN  | M0)) /*GPMC_nCS3 lab*/
		/* Toggle Reset pin of TL16CP754C device */
	MUX_VAL(CP(McBSP4_CLKX),    (IEN  | PTU | EN  | M4)) /*gpio_152 lab*/
	delay (100);
	MUX_VAL(CP(McBSP4_CLKX),    (IEN  | PTD | EN  | M4)) /*gpio_152 lab*/
	MUX_VAL(CP(sdrc_cke1),      (IDIS | PTU | EN  | M0)) /*sdrc_cke1 */

	  /* leds */
	MUX_VAL(CP(McSPI1_SOMI),  (IEN | PTD | EN | M4))  /* gpio_173 red */
	MUX_VAL(CP(McBSP4_DX),    (IEN  | PTD | EN | M4))  /* gpio_154 blue */
	MUX_VAL(CP(GPMC_nBE1),    (IEN  | PTD | EN | M4))  /* gpio_61 blue2 */
	MUX_VAL(CP(CAM_HS),       (IEN  | PTD | EN | M4))  /* gpio_94 green */
		/* Keep UART3 RX line pulled-up:
		 * Crashs have been seen on Zoom2 otherwise
		 */
	MUX_VAL(CP(UART3_RX_IRRX),(IEN  | PTU | DIS | M0)) /*UART3_RX_IRRX*/

	MUX_VAL(CP(CAM_XCLKA), (IDIS | PTD | DIS | M4)) /*gpio_96 LCD reset*/
	MUX_VAL(CP(CAM_VS),    (IEN  | PTU | DIS | M4)) /*gpio_95 for TVOut*/
	MUX_VAL(CP(CAM_PCLK),  (IEN  | PTU | DIS | M4)) /*gpio_97 for HDMI*/
#endif
}

/******************************************************************************
 * Routine: update_mux()
 * Description:Update balls which are different between boards.  All should be
 *             updated to match functionality.  However, I'm only updating ones
 *             which I'll be using for now.  When power comes into play they
 *             all need updating.
 *****************************************************************************/
void update_mux(u32 btype, u32 mtype)
{
	/* NOTHING as of now... */
}
