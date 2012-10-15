/*
 * Copyright (C) 2007-2008 Texas Instruments, Inc.
 *
 * USB
 * Imported from omap3-dev drivers/usb/twl4030_usb.c
 * This is unique part of the copyright
 *
 * twl4030_usb - TWL4030 USB transceiver, talking to OMAP OTG controller
 *
 * (C) Copyright 2009 Atin Malaviya (atin.malaviya@gmail.com)
 *
 * Based on: twl4030_usb.c in linux 2.6 (drivers/i2c/chips/twl4030_usb.c)
 * Copyright (C) 2004-2007 Texas Instruments
 * Copyright (C) 2008 Nokia Corporation
 * Contact: Felipe Balbi <felipe.balbi@nokia.com>
 *
 * Author: Atin Malaviya (atin.malaviya@gmail.com)
 *
 *
 * Keypad
 *
 * (C) Copyright 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
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
 *
 */
#include <twl4030.h>
#include <asm/arch/usb34xx.h>
#include <asm/arch/led.h>
#include <asm/io.h>
/*
 * Battery
 */
#define mdelay(n) ({ unsigned long msec = (n); while (msec--) udelay(1000); })

/* Functions to read and write from TWL4030 */
static inline int twl4030_i2c_write_u8(u8 chip_no, u8 val, u8 reg)
{
	return i2c_write(chip_no, reg, 1, &val, 1);
}

static inline int twl4030_i2c_read_u8(u8 chip_no, u8 *val, u8 reg)
{
	return i2c_read(chip_no, reg, 1, val, 1);
}

/*
 * Sets and clears bits on an given register on a given module
 */
static inline int clear_n_set(u8 chip_no, u8 clear, u8 set, u8 reg)
{
	int ret;
	u8 val = 0;

	/* Gets the initial register value */
	ret = twl4030_i2c_read_u8(chip_no, &val, reg);
	if (ret) {
		printf("a\n");
		return ret;
	}

	/* Clearing all those bits to clear */
	val &= ~(clear);

	/* Setting all those bits to set */
	val |= set;

	/* Update the register */
	ret = twl4030_i2c_write_u8(chip_no, val, reg);
	if (ret) {
		printf("b\n");
		return ret;
	}
	return 0;
}

/*
 * Disable/Enable AC Charge funtionality.
 */
static int twl4030_ac_charger_enable(int enable)
{
	int ret;

	if (enable) {
		/* forcing the field BCIAUTOAC (BOOT_BCI[0]) to 1 */
		ret = clear_n_set(TWL4030_CHIP_PM_MASTER, 0,
			       (CONFIG_DONE | BCIAUTOWEN | BCIAUTOAC),
			       REG_BOOT_BCI);
		if (ret)
			return ret;
	} else {
		/* forcing the field BCIAUTOAC (BOOT_BCI[0]) to 0 */
		ret = clear_n_set(TWL4030_CHIP_PM_MASTER, BCIAUTOAC,
			       (CONFIG_DONE | BCIAUTOWEN),
			       REG_BOOT_BCI);
		if (ret)
			return ret;
	}
	return 0;
}

/*
 * Disable/Enable USB Charge funtionality.
 */
static int twl4030_usb_charger_enable(int enable)
{
	u8 value;
	int ret;

	if (enable) {
		/* enable access to BCIIREF1 */
		ret = twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE, 0xE7,
			REG_BCIMFKEY);
		if (ret)
			return ret;

		/* forcing the field BCIAUTOUSB (BOOT_BCI[1]) to 1 */
		ret = clear_n_set(TWL4030_CHIP_PM_MASTER, 0,
			(CONFIG_DONE | BCIAUTOWEN | BCIAUTOUSB),
			REG_BOOT_BCI);
		if (ret)
			return ret;

		/* Enabling interfacing with usb thru OCP */
		ret = clear_n_set(TWL4030_CHIP_USB, 0, PHY_DPLL_CLK,
			REG_PHY_CLK_CTRL);
		if (ret)
			return ret;

		value = 0;

		while (!(value & PHY_DPLL_CLK)) {
			udelay(10);
			ret = twl4030_i2c_read_u8(TWL4030_CHIP_USB, &value,
				REG_PHY_CLK_CTRL_STS);
			if (ret)
				return ret;
		}

		/* OTG_EN (POWER_CTRL[5]) to 1 */
		ret = clear_n_set(TWL4030_CHIP_USB, 0, OTG_EN,
			REG_POWER_CTRL);
		if (ret)
			return ret;

		mdelay(50);

		/* forcing USBFASTMCHG(BCIMFSTS4[2]) to 1 */
		ret = clear_n_set(TWL4030_CHIP_MAIN_CHARGE, 0,
			USBFASTMCHG, REG_BCIMFSTS4);
		if (ret)
			return ret;
	} else {
		ret = clear_n_set(TWL4030_CHIP_PM_MASTER, BCIAUTOUSB,
			(CONFIG_DONE | BCIAUTOWEN), REG_BOOT_BCI);
		if (ret)
			return ret;
	}

	return 0;
}

/*
 * Setup the twl4030 MADC module to measure the backup
 * battery voltage.
 */
static int twl4030_madc_setup(void)
{
	int ret = 0;

	/* turning MADC clocks on */
	ret = clear_n_set(TWL4030_CHIP_INTBR, 0,
		(MADC_HFCLK_EN | DEFAULT_MADC_CLK_EN), REG_GPBR1);
	if (ret)
		return ret;

	/* turning adc_on */
	ret = twl4030_i2c_write_u8(TWL4030_CHIP_MADC, MADC_ON,
		REG_CTRL1);
	if (ret)
		return ret;

	/* setting MDC channel 9 and 12 to trigger by SW1 */
	ret = clear_n_set(TWL4030_CHIP_MADC, 0, SW1_CH9_SEL | SW1_CH12_SEL,
		REG_SW1SELECT_MSB);

	/* Average 9 and 12 to trigger by SW1 */
	ret = clear_n_set(TWL4030_CHIP_MADC, 0, SW1_CH9_SEL | SW1_CH12_SEL,
		REG_SW1AVG_MSB);

	return ret;
}

/*
 * Charge backup battery through main battery
 */
static int twl4030_charge_backup_battery(void)
{
	int ret;

	ret = clear_n_set(TWL4030_CHIP_PM_RECEIVER, 0xff,
			  (BBCHEN | BBSEL_3200mV | BBISEL_150uA), REG_BB_CFG);
	if (ret)
		return ret;

	return 0;
}

/*
 * Helper function to read a 2-byte register on BCI module
 */
static int read_bci_val(u8 reg)
{
	int ret = 0, temp = 0;
	u8 val;

	/* reading MSB */
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_MAIN_CHARGE, &val, reg + 1);
	if (ret)
		return ret;

	temp = ((int)(val & 0x03)) << 8;

	/* reading LSB */
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_MAIN_CHARGE, &val, reg);
	if (ret)
		return ret;

	return temp + val;
}

/*
 * Triggers the sw1 request for the twl4030 module to measure the sw1 selected
 * channels
 */
static int twl4030_madc_sw1_trigger(void)
{
	u8 val;
	int ret;

	/* Triggering SW1 MADC convertion */
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_MADC, &val, REG_CTRL_SW1);
	if (ret)
		return ret;

	val |= SW1_TRIGGER;

	ret = twl4030_i2c_write_u8(TWL4030_CHIP_MADC, val, REG_CTRL_SW1);
	if (ret)
		return ret;

	/* Waiting until the SW1 conversion ends*/
	val =  BUSY;

	while (!((val & EOC_SW1) && (!(val & BUSY)))) {
		ret = twl4030_i2c_read_u8(TWL4030_CHIP_MADC, &val,
					  REG_CTRL_SW1);
		if (ret)
			return ret;
		mdelay(10);
	}

	return 0;
}

/*
 * Return battery voltage
 */
static int twl4030_get_battery_voltage(void)
{
	int volt;

	volt = read_bci_val(T2_BATTERY_VOLT);
	return (volt * 588) / 100;
}

/*
 * Return the battery backup voltage
 */
static int twl4030_get_backup_battery_voltage(void)
{
	int ret, temp;
	u8 volt;

	/* trigger MADC convertion */
	twl4030_madc_sw1_trigger();

	ret = twl4030_i2c_read_u8(TWL4030_CHIP_MADC, &volt, REG_GPCH9 + 1);
	if (ret)
		return ret;

	temp = ((int) volt) << 2;

	ret = twl4030_i2c_read_u8(TWL4030_CHIP_MADC, &volt, REG_GPCH9);
	if (ret)
		return ret;

	temp = temp + ((int) ((volt & MADC_LSB_MASK) >> 6));

	return  (temp * 441) / 100;
}

#if 0 /* Maybe used in future */
/*
 * Return the AC power supply voltage
 */
static int twl4030_get_ac_charger_voltage(void)
{
	int volt = read_bci_val(T2_BATTERY_ACVOLT);
	return (volt * 735) / 100;
}

/*
 * Return the USB power supply voltage
 */
static int twl4030_get_usb_charger_voltage(void)
{
	int volt = read_bci_val(T2_BATTERY_USBVOLT);
	return (volt * 2058) / 300;
}
#endif

static void twl4030_improve_backup_battery(void)
{
    twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0xC0, PM_MASTER_PROTECT_KEY);
    twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0x0C, PM_MASTER_PROTECT_KEY);
    /*decrease the power for RTC clock */
    clear_n_set( TWL4030_CHIP_PM_MASTER,
                          0x00,
                          0x80,
                          PM_MASTER_CFG_BOOT);
    twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0x00, PM_MASTER_PROTECT_KEY);
}

static int twl4030_chgi_ref_set(int level)
{
	int ret;
       u8 lsb=0, msb=0;
	u8 val;

	twl4030_i2c_read_u8(TWL4030_CHIP_MAIN_CHARGE, &val, REG_BCICTL1);
	if (val & CGAIN){
            /* slope of 3.33 mA/LSB */
           level = (level * BCI_CHGI_REF_STEP) / (BCI_CHGI_REF_PSR_R1);
	}
	else{
            /* slope of 1.665 mA/LSB */
	     level = (level * BCI_CHGI_REF_STEP) / (BCI_CHGI_REF_PSR_R2);
	}

       if(level > BCI_CHGI_REF_MASK){
            level = BCI_CHGI_REF_MASK;
       }

       level |= BCI_CHGI_REF_OFF;

       lsb = (u8)(level & CHGI_LSB);
       msb = (u8)((level >> BCI_REG_SIZE) & CHGI_MSB);

       /* Change charging current */
	twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE, 0xE7,
			REG_BCIMFKEY);

       ret = twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE,
			      lsb,
			      REG_BCIIREF1);

       twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE, 0xE7,
			REG_BCIMFKEY);

       ret = twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE,
			      msb,
			      REG_BCIIREF2);

	return ret;
}

/*
 * Battery charging main function called from board-specific file
 */
int twl4030_init_battery_charging(void)
{
	u8 hwsts;
	u8 clear, set;
	int battery_volt = 0, charger_present = 0;
	int battery_volt_start = CFG_LOW_BAT;
	int ret = 0, ac_t2_enabled = 0, charger_tries = 0;

#ifdef CONFIG_3430ZOOM2
	/* For Zoom2 enable Main charge Automatic mode:
	 * by enabling MADC clocks
	 */

	OMAP3_LED_ERROR_ON();

	/* Disable USB, enable AC: 0x35 defalut */
	ret = clear_n_set(TWL4030_CHIP_PM_MASTER, BCIAUTOUSB,
			       BCIAUTOAC,
			       REG_BOOT_BCI);

	/* Enable AC charging : ROM code is shutting down MADC CLK */
	ret = clear_n_set(TWL4030_CHIP_INTBR, 0,
		(MADC_HFCLK_EN | DEFAULT_MADC_CLK_EN), REG_GPBR1);

	udelay(100);

	ret = clear_n_set(TWL4030_CHIP_PM_MASTER,
				BCIAUTOAC | CVENAC,
				0,
				REG_BOOT_BCI);

	/* Change charging current */
	ret = twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE, 0xE7,
			REG_BCIMFKEY);
	/* set 1 Amp charging */
	ret = twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE, 0x58,
			REG_BCIIREF1);
	ret = twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE, 0xE7,
			REG_BCIMFKEY);
	ret = twl4030_i2c_write_u8(TWL4030_CHIP_MAIN_CHARGE, 0x03,
			REG_BCIIREF2);

	/* Set CGAIN=1 */
	ret = clear_n_set(TWL4030_CHIP_MAIN_CHARGE,
				0,
				CGAIN,
				REG_BCICTL1);

	ret = clear_n_set(TWL4030_CHIP_PM_MASTER,
				0,
				BCIAUTOAC | CVENAC,
				REG_BOOT_BCI);
#endif

       twl4030_improve_backup_battery();

#ifdef CFG_BATTERY_CHARGING
	/* Read the sts_hw_conditions register */
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_MASTER, &hwsts,
			  REG_STS_HW_CONDITIONS);

       /* AC charging is enabled regardless of the whether the
	* charger is attached
	*/
	ret = twl4030_ac_charger_enable(1);
	if (ret)
		return ret;
	udelay(10000); /* 0.01 sec */

	/* AC T2 charger present */
	if (hwsts & STS_CHG) {
		charger_present = 1;
	}

	/* USB charger present */
	if ((hwsts & STS_VBUS) | (hwsts & STS_USB)) {
		charger_present = 1;
	}

	ret = twl4030_madc_setup();
	if (ret) {
		printf("twl4030 madc setup error %d\n", ret);
		return ret;
	}

	/* usb charging is enabled regardless of the whether the
	* charger is attached, otherwise we will not be able to enable
	* usb charging at a later stage. The BCI will enable auto
	*/
	ret = twl4030_usb_charger_enable(1);
	if (ret)
		return ret;
	udelay(10000); /* 0.01 sec */

	/* backup battery charges through main battery */
	ret = twl4030_charge_backup_battery();
	if (ret) {
		printf("backup battery charging error\n");
		return ret;
	}

#if defined(CONFIG_BOOTCASE_CHECK)
    /*To make the charge display faster, decrease the Voltage threshold for boot while charge*/
    extern char *bootcase_name;
    if(bootcase_name && !strncmp(bootcase_name, "charger", strlen("charger"))){
        battery_volt_start = CFG_CHG_LOW_BAT;
    }
#endif

	battery_volt = twl4030_get_battery_voltage();
	printf("Battery levels: main %d mV, backup %d mV\n",
	battery_volt, twl4030_get_backup_battery_voltage());
	if(battery_volt < battery_volt_start){
		printf("Main battery charge is not enough to %d!\n", battery_volt_start);
		printf("Please connect USB or AC charger to continue.\n");
	}
	while (battery_volt < battery_volt_start) {
		charger_tries = 0;
		charger_present = 0;
		/*
		* Main charging loop
		* If the battery volage is below CFG_LOW_BAT, attempt is made
		* to recharge the battery to CFG_BAT_CHG.  The main led will
		* blink red until either the ac or the usb charger is connected.
		*/
		do {
#if defined(CONFIG_BOOTCASE_CHECK) && defined(CONFIG_ADC_CALIBRATION)
			/*skip the VBat check if boot by fixture*/
			extern char *bootcase_name;
			if(bootcase_name && !strncmp(bootcase_name, "fixture", strlen("fixture"))){
				break;
			}
#endif
			/* Read the sts_hw_conditions register */
			twl4030_i2c_read_u8(TWL4030_CHIP_PM_MASTER, &hwsts,
			REG_STS_HW_CONDITIONS);

			if (hwsts & STS_CHG) {
				OMAP3_LED_OK_OFF(); /* Blue LED - off */
				charger_present = 1;
				set = CHG_PRES_FALLING;
				clear = CHG_PRES_RISING;
			}else {
				OMAP3_LED_ERROR_OFF();
				set = CHG_PRES_RISING;
				clear = CHG_PRES_FALLING;
				udelay(10000); /* 0.01 sec */
			}
			clear_n_set(TWL4030_CHIP_INT, clear, set, REG_PWR_EDR1);

			if (charger_present){
				OMAP3_LED_OK_ON(); /* Blue LED - on */
				udelay(10000); /* 0.01 sec */
			}else {
				OMAP3_LED_ERROR_ON();
			}

			charger_tries++;

			if (ctrlc()) {
				printf("Battery charging terminated by user\n");
				printf("Battery charged to %d\n", battery_volt);
				goto out;
			}else if (charger_tries > CFG_CHARGER_TRIES_MAX) {
				printf("No charger connected, giving up on charging.\n");
				break;
			}
		} while (((battery_volt = twl4030_get_battery_voltage())
                    < CFG_BAT_CHG) && (!charger_present));
		/* If debug board charger is connected,
		*  battery_volt is approximately 4100mV
		*/
		//printf("battery_volt = %d, charger_present=%d\n", battery_volt, charger_present);
#if defined(CONFIG_BOOTCASE_CHECK)
		if(!charger_present){
			extern void twl4030_poweroff(void);
			printf("Power down for too lower Vbat and no charger plugged\n");
			twl4030_poweroff();
			udelay(500000); /* 0.5 sec */
		}
#endif
	}
out:
	OMAP3_LED_OK_OFF(); /* Blue LED - off */
	OMAP3_LED_ERROR_OFF();

#endif

	return ret;

}

#if (defined(CONFIG_TWL4030_KEYPAD) && (CONFIG_TWL4030_KEYPAD))
/*
 * Keypad
 */
int twl4030_keypad_init(void)
{
	int ret = 0;
	u8 ctrl;
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_KEYPAD, &ctrl, KEYPAD_KEYP_CTRL_REG);
	if (!ret) {
		ctrl |= KEYPAD_CTRL_KBD_ON | KEYPAD_CTRL_SOFT_NRST;
		ctrl &= ~KEYPAD_CTRL_SOFTMODEN;
		ret = twl4030_i2c_write_u8(TWL4030_CHIP_KEYPAD, ctrl, KEYPAD_KEYP_CTRL_REG);
	}
	return ret;
}

int twl4030_keypad_reset(void)
{
	int ret = 0;
	u8 ctrl;
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_KEYPAD, &ctrl, KEYPAD_KEYP_CTRL_REG);
	if (!ret) {
		ctrl &= ~KEYPAD_CTRL_SOFT_NRST;
		ret = twl4030_i2c_write_u8(TWL4030_CHIP_KEYPAD, ctrl, KEYPAD_KEYP_CTRL_REG);
	}
	return ret;
}

int twl4030_keypad_keys_pressed(unsigned char *key1, unsigned char *key2)
{
	int ret = 0;
	u8 cb, c, rb, r;
	for (cb = 0; cb < 8; cb++) {
		c = 0xff & ~(1 << cb);
		twl4030_i2c_write_u8(TWL4030_CHIP_KEYPAD, c, KEYPAD_KBC_REG);
		twl4030_i2c_read_u8(TWL4030_CHIP_KEYPAD, &r, KEYPAD_KBR_REG);
		for (rb = 0; rb < 8; rb++) {
			if (!(r & (1 << rb))) {
				if (!ret)
					*key1 = cb << 3 | rb;
				else if (1 == ret)
					*key2 = cb << 3 | rb;
				ret++;
			}
		}
	}
	return ret;
}

#endif

/* USB */

#if (defined(CONFIG_TWL4030_USB) && (CONFIG_TWL4030_USB))

static int twl4030_usb_write(u8 address, u8 data)
{
	int ret = 0;
	ret = twl4030_i2c_write_u8(TWL4030_CHIP_USB, data, address);
	if (ret != 0)
		printf("TWL4030:USB:Write[0x%x] Error %d\n", address, ret);

	return ret;
}

static int twl4030_usb_read(u8 address)
{
	u8 data;
	int ret = 0;
	ret = twl4030_i2c_read_u8(TWL4030_CHIP_USB, &data, address);
	if (ret == 0)
		ret = data;
	else
		printf("TWL4030:USB:Read[0x%x] Error %d\n", address, ret);

	return ret;
}

static int twl4030_usb_set_bits(u8 reg, u8 bits)
{
	return twl4030_usb_write(reg + 1, bits);
}

static int twl4030_usb_clear_bits(u8 reg, u8 bits)
{
	return twl4030_usb_write(reg + 2, bits);
}

static void twl4030_usb_ldo_init(void)
{
	/* Enable writing to power configuration registers */
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0xC0, PM_MASTER_PROTECT_KEY);
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0x0C, PM_MASTER_PROTECT_KEY);

	/* put VUSB3V1 LDO in active state */
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_RECEIVER, 0x00, PM_RECEIVER_VUSB_DEDICATED2);

	/* input to VUSB3V1 LDO is from VBAT, not VBUS */
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_RECEIVER, 0x14, PM_RECEIVER_VUSB_DEDICATED1);

	/* turn on 3.1V regulator */
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_RECEIVER, 0x20, PM_RECEIVER_VUSB3V1_DEV_GRP);
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_RECEIVER, 0x00, PM_RECEIVER_VUSB3V1_TYPE);

	/* turn on 1.5V regulator */
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_RECEIVER, 0x20, PM_RECEIVER_VUSB1V5_DEV_GRP);
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_RECEIVER, 0x00, PM_RECEIVER_VUSB1V5_TYPE);

	/* turn on 1.8V regulator */
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_RECEIVER, 0x20, PM_RECEIVER_VUSB1V8_DEV_GRP);
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_RECEIVER, 0x00, PM_RECEIVER_VUSB1V8_TYPE);

	/* disable access to power configuration registers */
	twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0x00, PM_MASTER_PROTECT_KEY);
}

static void twl4030_phy_power(int on)
{
	u8 pwr;

       if(on){
	    pwr = twl4030_usb_read(USB_PHY_PWR_CTRL);
	    pwr &= ~USB_PHYPWD;
	    twl4030_usb_write(USB_PHY_PWR_CTRL, pwr);
	    twl4030_usb_write(USB_PHY_CLK_CTRL, twl4030_usb_read(USB_PHY_CLK_CTRL) |
				(USB_CLOCKGATING_EN | USB_CLK32K_EN));
       }else{
           pwr = twl4030_usb_read(USB_PHY_PWR_CTRL);
	    pwr |= USB_PHYPWD;
	    twl4030_usb_write(USB_PHY_PWR_CTRL, pwr);
	    twl4030_usb_write(USB_PHY_CLK_CTRL, twl4030_usb_read(USB_PHY_CLK_CTRL) &
				(~(USB_CLOCKGATING_EN | USB_CLK32K_EN)));

       }
}

int twl4030_usb_init(void)
{
	unsigned long timeout;

	/* twl4030 ldo init */
	twl4030_usb_ldo_init();

	/* Enable the twl4030 phy */
	twl4030_phy_power(1);

	/* enable DPLL to access PHY registers over I2C */
	twl4030_usb_write(USB_PHY_CLK_CTRL, twl4030_usb_read(USB_PHY_CLK_CTRL) |
			  USB_REQ_PHY_DPLL_CLK);
	timeout = 1000 * 1000; /* 1 sec */
	while (!(twl4030_usb_read(USB_PHY_CLK_CTRL_STS) & USB_PHY_DPLL_CLK) &&
		0 < timeout) {
		udelay(10);
		timeout -= 10;
	}
	if (!(twl4030_usb_read(USB_PHY_CLK_CTRL_STS) & USB_PHY_DPLL_CLK)) {
		printf("Timeout setting T2 HSUSB PHY DPLL clock\n");
		return -1;
	}

	/* Enable ULPI mode */
	twl4030_usb_clear_bits(USB_IFC_CTRL, USB_CARKITMODE);
	twl4030_usb_set_bits(USB_POWER_CTRL, USB_OTG_ENAB);
	twl4030_usb_clear_bits(USB_FUNC_CTRL, USB_XCVRSELECT_MASK | USB_OPMODE_MASK);
	/* let ULPI control the DPLL clock */
	twl4030_usb_write(USB_PHY_CLK_CTRL, twl4030_usb_read(USB_PHY_CLK_CTRL) &
				~USB_REQ_PHY_DPLL_CLK);
	return 0;
}

void twl4030_usb_force_standby(void)
{
       volatile u8  *pwr  = (volatile u8  *) OMAP34XX_USB_POWER;
       volatile u32 *otg_sysconfig = (volatile u32 *) OMAP34XX_OTG_SYSCONFIG;
       volatile u32 *otg_interfsel	= (volatile u32 *) OMAP34XX_OTG_INTERFSEL;
       volatile u32 *otg_forcestdby = (volatile u32 *) OMAP34XX_OTG_FORCESTDBY;

       twl4030_phy_power(0);

	/* Set the interface */
	*otg_interfsel = OMAP34XX_OTG_INTERFSEL_OMAP;

	/* Clear force standby */
	*otg_forcestdby |= OMAP34XX_OTG_FORCESTDBY_STANDBY;

	/* Reset MUSB */
	*pwr &= ~MUSB_POWER_SOFTCONN;
       *otg_sysconfig = OMAP34XX_OTG_SYSCONFIG_SOFTRESET;
}
#endif

/*
 * Power Reset
 */
void twl4030_power_reset_init(void)
{
#if defined(CONFIG_3430ZOOM2)
	clear_n_set(TWL4030_CHIP_PM_MASTER, 0, SW_EVENTS_STOPON_PWRON,
		    PM_MASTER_P1_SW_EVENTS);
#endif
}

#ifdef CONFIG_CMD_VOLTAGE

/* Override the weakly defined voltage_info function */
void voltage_info (void)
{
	u8 vdd1_dev_grp, vdd1_type, vdd1_remap, vdd1_cfg, vdd1_misc_cfg;
	u8 vdd1_test1, vdd1_test2, vdd1_osc, vdd1_vsel, vdd1_vmode_cfg;
	u8 vdd1_vfloor, vdd1_vroof, vdd1_step;
	u8 vdd2_dev_grp, vdd2_type, vdd2_remap, vdd2_cfg, vdd2_misc_cfg;
	u8 vdd2_test1, vdd2_test2, vdd2_osc, vdd2_vsel, vdd2_vmode_cfg;
	u8 vdd2_vfloor, vdd2_vroof, vdd2_step;
	unsigned int vdd1 = 0;
	unsigned int vdd2 = 0;
	/* Units are in micro volts */
	unsigned int base = 600000;
	unsigned int scale = 12500;

	/* VDD1 */
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_dev_grp,
			    PM_RECEIVER_VDD1_DEV_GRP);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_type,
			    PM_RECEIVER_VDD1_TYPE);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_remap,
			    PM_RECEIVER_VDD1_REMAP);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_cfg,
			    PM_RECEIVER_VDD1_CFG);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_misc_cfg,
			    PM_RECEIVER_VDD1_MISC_CFG);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_test1,
			    PM_RECEIVER_VDD1_TEST1);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_test2,
			    PM_RECEIVER_VDD1_TEST2);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_osc,
			    PM_RECEIVER_VDD1_OSC);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_vsel,
			    PM_RECEIVER_VDD1_VSEL);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_vmode_cfg,
			    PM_RECEIVER_VDD1_VMODE_CFG);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_vfloor,
			    PM_RECEIVER_VDD1_VFLOOR);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_vroof,
			    PM_RECEIVER_VDD1_VROOF);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd1_step,
			    PM_RECEIVER_VDD1_STEP);

	printf("VDD1 Regs\n");
	printf("\tDEV_GRP   0x%x\n", vdd1_dev_grp);
	printf("\tTYPE      0x%x\n", vdd1_type);
	printf("\tREMAP     0x%x\n", vdd1_remap);
	printf("\tCFG       0x%x\n", vdd1_cfg);
	printf("\tMISC_CFG  0x%x\n", vdd1_misc_cfg);
	printf("\tTEST1     0x%x\n", vdd1_test1);
	printf("\tTEST2     0x%x\n", vdd1_test2);
	printf("\tOSC       0x%x\n", vdd1_osc);
	printf("\tVSEL      0x%x\n", vdd1_vsel);
	printf("\tVMODE_CFG 0x%x\n", vdd1_vmode_cfg);
	printf("\tVFLOOR    0x%x\n", vdd1_vfloor);
	printf("\tVROOF     0x%x\n", vdd1_vroof);
	printf("\tSTEP      0x%x\n", vdd1_step);

	/* VDD2 */
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_dev_grp,
			    PM_RECEIVER_VDD2_DEV_GRP);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_type,
			    PM_RECEIVER_VDD2_TYPE);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_remap,
			    PM_RECEIVER_VDD2_REMAP);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_cfg,
			    PM_RECEIVER_VDD2_CFG);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_misc_cfg,
			    PM_RECEIVER_VDD2_MISC_CFG);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_test1,
			    PM_RECEIVER_VDD2_TEST1);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_test2,
			    PM_RECEIVER_VDD2_TEST2);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_osc,
			    PM_RECEIVER_VDD2_OSC);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_vsel,
			    PM_RECEIVER_VDD2_VSEL);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_vmode_cfg,
			    PM_RECEIVER_VDD2_VMODE_CFG);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_vfloor,
			    PM_RECEIVER_VDD2_VFLOOR);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_vroof,
			    PM_RECEIVER_VDD2_VROOF);
	twl4030_i2c_read_u8(TWL4030_CHIP_PM_RECEIVER, &vdd2_step,
			    PM_RECEIVER_VDD2_STEP);

	printf("VDD2 Regs\n");
	printf("\tDEV_GRP   0x%x\n", vdd2_dev_grp);
	printf("\tTYPE      0x%x\n", vdd2_type);
	printf("\tREMAP     0x%x\n", vdd2_remap);
	printf("\tCFG       0x%x\n", vdd2_cfg);
	printf("\tMISC_CFG  0x%x\n", vdd2_misc_cfg);
	printf("\tTEST1     0x%x\n", vdd2_test1);
	printf("\tTEST2     0x%x\n", vdd2_test2);
	printf("\tOSC       0x%x\n", vdd2_osc);
	printf("\tVSEL      0x%x\n", vdd2_vsel);
	printf("\tVMODE_CFG 0x%x\n", vdd2_vmode_cfg);
	printf("\tVFLOOR    0x%x\n", vdd2_vfloor);
	printf("\tVROOF     0x%x\n", vdd2_vroof);
	printf("\tSTEP      0x%x\n", vdd2_step);

	/* Calculated the voltages */
	printf("\n");
	if (!(vdd1_cfg & 1))
	{
		/* voltage controled by vsel */
		vdd1 = scale * vdd1_vsel + base;
		printf("VDD1 calculated to be ");
		if (!(vdd1 % 1000)) {
			printf("%d mV\n", vdd1 / 1000);
		} else {
			printf("%d uV\n", vdd1);
		}
	} else {
		printf("VDD1 calculation unsupport for this mode\n");
	}

	if (!(vdd2_cfg & 1))
	{
		/* voltage controled by vsel */
		vdd2 = scale * vdd2_vsel + base;
		printf("VDD2 calculated to be ");
		if (!(vdd2 % 1000)) {
			printf("%d mV\n", vdd2 / 1000);
		} else {
			printf("%d uV\n", vdd2);
		}
	} else {
		printf("VDD2 calculation unsupport for this mode\n");
	}
}


#endif

#if defined(CONFIG_BOOTCASE_CHECK)
/*
 * Return the ADC value of battery voltage
 */
int twl4030_battery_adc(void)
{
    int ret = -1, temp;
    u8 adc;

    /* trigger MADC convertion */
    twl4030_madc_sw1_trigger();

    ret = twl4030_i2c_read_u8(TWL4030_CHIP_MADC, &adc, REG_GPCH12 + 1);
    if (ret){
        temp = -1;
        goto end;
    }
    temp = ((int) adc) << 2;

    ret = twl4030_i2c_read_u8(TWL4030_CHIP_MADC, &adc, REG_GPCH12);
    if (ret){
        temp = -1;
        goto end;
    }
    temp = temp + ((int) ((adc & MADC_LSB_MASK) >> 6));

end:
    return temp;
}

#define PM_RECEIVER_WATCHDOG_CFG (0x5E)
#define STARTON_SWBUG (1<<7)
#define STARTON_VBAT    (1<<4)
#define STARTON_RTC      (1<<3)
#define STARTON_CHG      (1<<1)
#define STARTON_PWON   (1<<0)

#define STS_BOOT_BACKUP (1<<7)
#define STS_BOOT_WATCHDOG (1<<5)
#define STS_BOOT_SETUP_DONE_BCK (1<<2)

#define STS_HW_PWON (1<<0)

#define POWERKEY_DOWN_GAP (300)  //ms
#define POWERKEY_CHECK_GAP (100)  //ms


#define RTC_CTRL_REG       (0x29)
#define RTC_CTRL_REG_STOP_RTC  (1<<0)

#define RTC_STATUS_REG  (0x2A)
#define RTC_STATUS_REG_RUN       (1<<1)
#define RTC_STATUS_REG_ALARM   (1<<6)

#define RTC_INTERRUPTS_REG (0x2B)
#define RTC_INTERRUPTS_REG_IT_ALARM  (1<<3)

char *bootcase_cmdname = "bootcase";
char *bootcase_name = NULL;

struct boot_case_check_entry{
    char * name;
    /*return 1 while boot by this case, otherwise return 0*/
    int (*check)(u8);
};
static int boot_by_watchdog(u8 sysboot);
static int boot_by_rtc(u8 sysboot);
static int boot_by_cold_reset(u8 sysboot);
static int boot_by_warm_reset(u8 sysboot);
static int boot_by_charger(u8 sysboot);
static int boot_by_powerkey( u8 sysboot);
static int boot_by_fixture(u8 sysboot);
static int boot_by_download(u8 sysboot);

/* boot case list, the priority discreased*/
struct boot_case_check_entry boot_case_table[ ] = {
    {"download", boot_by_download},
    {"fixture", boot_by_fixture},
    {"watchdog", boot_by_watchdog},
    {"cold_reset", boot_by_cold_reset},
    {"warm_reset", boot_by_warm_reset},
    {"charger", boot_by_charger},
    {"rtc", boot_by_rtc},
    {"powerkey", boot_by_powerkey},
    {NULL, NULL},
};

void twl4030_poweroff(void)
{
    /* Clear the STARTON_VBAT and STARTON_SWBUG
     * STARTON_VBAT will cause the auto reboot if battery insert;
     * STARTON_SWBUG will cause the auto reboot if watchdog has been expired
     * Mark the STARTON_PWON, which enable switch on transition if power on has been pressed
     */
    printf("Power Off .................\n");
    twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0xC0, PM_MASTER_PROTECT_KEY);
    twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0x0C, PM_MASTER_PROTECT_KEY);

    clear_n_set(TWL4030_CHIP_PM_MASTER,
            STARTON_SWBUG | STARTON_VBAT,
            STARTON_PWON | STARTON_RTC | STARTON_CHG,
            PM_MASTER_CFG_P1_TRANSITION);

    clear_n_set(TWL4030_CHIP_PM_MASTER,
            STARTON_SWBUG | STARTON_VBAT,
            STARTON_PWON | STARTON_RTC | STARTON_CHG,
            PM_MASTER_CFG_P2_TRANSITION);

    clear_n_set(TWL4030_CHIP_PM_MASTER,
            STARTON_SWBUG | STARTON_VBAT,
            STARTON_PWON | STARTON_RTC | STARTON_CHG,
            PM_MASTER_CFG_P3_TRANSITION);

    twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0x00, PM_MASTER_PROTECT_KEY);

    /*fire the watchdog*/
    twl4030_i2c_write_u8(TWL4030_CHIP_PM_RECEIVER, 0x01, PM_RECEIVER_WATCHDOG_CFG);

    while(1) ;
}

void twl4030_restart(void)
{
    /* Clear the STARTON_VBAT and STARTON_SWBUG
     * STARTON_VBAT will cause the auto reboot if battery insert;
     * STARTON_SWBUG will cause the auto reboot if watchdog has been expired
     * Mark the STARTON_PWON, which enable switch on transition if power on has been pressed
     */
    twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0xC0, PM_MASTER_PROTECT_KEY);
    twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0x0C, PM_MASTER_PROTECT_KEY);

    clear_n_set(TWL4030_CHIP_PM_MASTER,
            STARTON_VBAT,
            STARTON_SWBUG |STARTON_PWON | STARTON_RTC | STARTON_CHG,
            PM_MASTER_CFG_P1_TRANSITION);

    clear_n_set(TWL4030_CHIP_PM_MASTER,
            STARTON_VBAT,
            STARTON_SWBUG |STARTON_PWON | STARTON_RTC | STARTON_CHG,
            PM_MASTER_CFG_P2_TRANSITION);

    clear_n_set(TWL4030_CHIP_PM_MASTER,
            STARTON_VBAT,
            STARTON_SWBUG |STARTON_PWON | STARTON_RTC | STARTON_CHG,
            PM_MASTER_CFG_P3_TRANSITION);

    twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0x00, PM_MASTER_PROTECT_KEY);

    /*Mark SETUP_DONE_BCK to identif thar reset by user*/
    twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, STS_BOOT_SETUP_DONE_BCK, PM_MASTER_STS_BOOT);

    /*fire the watchdog*/
    twl4030_i2c_write_u8(TWL4030_CHIP_PM_RECEIVER, 0x01, PM_RECEIVER_WATCHDOG_CFG);
}

#define FIXTURE_GPIO (170)
static int boot_by_fixture(u8 sysboot)
{
    unsigned int ret = 0;
    gpio_t *gpio6_base = (gpio_t *)OMAP34XX_GPIO6_BASE;

    /*Enable GPIO6 function CLK */
    sr32(CM_FCLKEN_PER, CLKEN_PER_EN_GPIO6_BIT, 1, 1);
    sr32(CM_ICLKEN_PER, CLKEN_PER_EN_GPIO6_BIT, 1, 1);

    sr32((u32)&gpio6_base->oe, 10, 1, 1);   /* GPIO170 input*/
    ret = __raw_readl((u32)&gpio6_base->datain);

    /*GPIO 170 level low, then go into fixture*/
    if(ret & (1 << 10)){
        return 0;
    }else{
        return 1;
    }
}

static int boot_by_watchdog(u8 sysboot)
{
    u8 val = 0;
    int ret = 0;

    if((sysboot & STS_BOOT_WATCHDOG) && !(sysboot & STS_BOOT_SETUP_DONE_BCK)){
        twl4030_i2c_read_u8(TWL4030_CHIP_PM_MASTER, &val, PM_MASTER_CFG_P1_TRANSITION);
        if(val & STARTON_SWBUG){
            ret = 1;
        }
    }

    return ret;
}

static int boot_by_cold_reset(u8 sysboot)
{
    u8 val = 0;
    int ret = 0;

    if(sysboot & 0x08){
        ret = 1;
    }
    
    return ret;
}

static int boot_by_warm_reset(u8 sysboot)
{
    u8 val = 0;
    int ret = 0;

#if 1
    /*Restart by OMAP4030, all the SDRAM has been keep*/
    u32 l;
    char b1, b2, b3, b4;
    u32 mask = 0x000000FF;

#define OMAP343X_CTRL_BASE 0x48002000
#define OMAP343X_SCRATCHPAD		(OMAP343X_CTRL_BASE + 0x910)
    l = __raw_readl(OMAP343X_SCRATCHPAD + 4);
    b4 = (l >> 24) & mask;
    b3 = (l >> 16) & mask;
    b2 = (l >> 8) & mask;
    b1 = (l >> 0) & mask;
    if( b4 == 'B' && b3 == 'M'){
        return 1;
    }
#endif

    if((sysboot & STS_BOOT_WATCHDOG) && (sysboot & STS_BOOT_SETUP_DONE_BCK)){
        twl4030_i2c_read_u8(TWL4030_CHIP_PM_MASTER, &val, PM_MASTER_CFG_P1_TRANSITION);
        if(val & STARTON_SWBUG){
            ret = 1;
        }
    }

    return ret;
}

static int boot_by_rtc(u8 sysboot)
{
    u8 val = 0;
    int ret = 0;

    twl4030_i2c_read_u8(TWL4030_CHIP_PM_MASTER, &val, PM_MASTER_CFG_P1_TRANSITION);

    if(val & STARTON_RTC){
        val = 0;
        twl4030_i2c_read_u8(TWL4030_CHIP_RTC, &val, RTC_CTRL_REG);
        /*RTC is alive*/
        if(val & RTC_CTRL_REG_STOP_RTC){
            val = 0;
            twl4030_i2c_read_u8(TWL4030_CHIP_RTC, &val, RTC_STATUS_REG);
            /*ALARM has been generated*/
            if(val & RTC_STATUS_REG_ALARM){
                val = 0;
                twl4030_i2c_read_u8(TWL4030_CHIP_RTC, &val, RTC_INTERRUPTS_REG);
                /*RTC requset*/
                if(val & RTC_INTERRUPTS_REG_IT_ALARM){
                    ret = 1;
                }
            }
        }
    }

    /*clear the RTC ALARM*/
    if(ret){
        /*clear the alarm interrupt*/
        twl4030_i2c_write_u8(TWL4030_CHIP_RTC, RTC_STATUS_REG_ALARM, RTC_STATUS_REG);
        /*disable the alarm*/
        twl4030_i2c_read_u8(TWL4030_CHIP_RTC, &val, RTC_CTRL_REG);
        twl4030_i2c_write_u8(TWL4030_CHIP_RTC, val & (~RTC_CTRL_REG_STOP_RTC), RTC_CTRL_REG);
    }
    return ret;
}

static int boot_by_charger(u8 sysboot)
{
    u8 val = 0;
    int ret = 0;

    twl4030_i2c_read_u8(TWL4030_CHIP_PM_MASTER, &val, PM_MASTER_CFG_P1_TRANSITION);

    if(val & STARTON_CHG){
        val = 0;
        /* Read the sts_hw_conditions register */
        twl4030_i2c_read_u8(TWL4030_CHIP_PM_MASTER, &val, REG_STS_HW_CONDITIONS);
        if (val & STS_CHG) {
            ret = 1;
        }
    }

    return ret;
}

static int boot_by_powerkey(u8 sysboot)
{
    u8 val;
    int ret = 0;
    int during = POWERKEY_DOWN_GAP;

    /*make sure that the powerkey has been pressed for POWERKEY_DOWN_GAP*/
    while(during > 0){
        val = 0;
        twl4030_i2c_read_u8(TWL4030_CHIP_PM_MASTER, &val, PM_MASTER_STS_HW_CONDITIONS);
        /*key is pressed*/
        if(val & STS_HW_PWON){
            mdelay(POWERKEY_CHECK_GAP);
            during -= POWERKEY_CHECK_GAP;
        }else{
            break;
        }
    }

    if(during <= 0){
        ret = 1;
    }

    return ret;
}

/*USB musb be plugged befor download boot*/
static int boot_by_download(u8 sysboot)
{
    int ret = 0;

    /*boot by fixture and USB inserted*/
    if(boot_by_fixture(sysboot) && boot_by_charger(sysboot)){
        ret = 1;
    }

    return ret;
}

/**********************************************
 * Routine: powerkey_check
 * Description: Power on if the key pressed
 **********************************************/
void bootcase_check(void)
{

    u8 sysboot;
    int i = 0;
    extern void omap_cbp_power_off(void);
    omap_cbp_power_off();
    twl4030_improve_backup_battery();
    twl4030_i2c_read_u8(TWL4030_CHIP_PM_MASTER, &sysboot, PM_MASTER_STS_BOOT);

    while(boot_case_table[i].check){
        if((boot_case_table[i].check)(sysboot)){
            bootcase_name = boot_case_table[i].name;
            break;
        }
        i++;
    }

    /*clear the flag in STS_BOOT*/
    twl4030_i2c_write_u8(TWL4030_CHIP_PM_MASTER, 0x00, PM_MASTER_STS_BOOT);

    /*unknow boot case, so power off*/
    if(!bootcase_name){
        printf("Unknow boot case, so power off.\n");
        twl4030_poweroff();
    }else{
        printf("Bootcase is %s.\n", bootcase_name);
    }
/*
    if(!strcmp(bootcase_name, "download")){
        do_fastboot(NULL, 1, 1, NULL);
    }*/
    twl4030_init_battery_charging();
}
#endif
