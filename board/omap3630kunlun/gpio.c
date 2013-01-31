#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/arch/cpu.h>
#include "gpio.h"



/** get gpio bank base **/
/** bank = 1,2,3,4,5,6 **/
static gpio_t* get_gpio_base(int bank)
{

    switch(bank)
    {
        case 1:
            return (gpio_t *)OMAP34XX_GPIO1_BASE;
        
        case 2:
            return (gpio_t *)OMAP34XX_GPIO2_BASE;
           
        case 3:
            return (gpio_t *)OMAP34XX_GPIO3_BASE;
            
        case 4:
            return (gpio_t *)OMAP34XX_GPIO4_BASE;
            
        case 5:
            return (gpio_t *)OMAP34XX_GPIO5_BASE;
            
        case 6:
            return (gpio_t *)OMAP34XX_GPIO6_BASE;
    }

	return (gpio_t*)0;
}

void enable_gpio_bank(int bank) {

	if (bank < 1 || bank > 6) return;

    if(bank == 1){
        sr32(CM_ICLKEN_WKUP,3, 1, 1);
        sr32(CM_FCLKEN_WKUP,3, 1, 1);
    }else if(bank == 2){
        sr32(CM_ICLKEN_PER,CLKEN_PER_EN_GPIO2_BIT, 1, 1);
        sr32(CM_FCLKEN_PER,CLKEN_PER_EN_GPIO2_BIT, 1, 1);
    }else if(bank == 3){
        sr32(CM_ICLKEN_PER,CLKEN_PER_EN_GPIO3_BIT, 1, 1);
        sr32(CM_FCLKEN_PER,CLKEN_PER_EN_GPIO3_BIT, 1, 1);
    }else if(bank == 4){
        sr32(CM_ICLKEN_PER,CLKEN_PER_EN_GPIO4_BIT, 1, 1);
        sr32(CM_FCLKEN_PER,CLKEN_PER_EN_GPIO4_BIT, 1, 1);
    }else if(bank == 5){
        sr32(CM_ICLKEN_PER,CLKEN_PER_EN_GPIO5_BIT, 1, 1);
        sr32(CM_FCLKEN_PER,CLKEN_PER_EN_GPIO5_BIT, 1, 1);
    }else if(bank == 6){
        sr32(CM_ICLKEN_PER,CLKEN_PER_EN_GPIO6_BIT, 1, 1);
        sr32(CM_FCLKEN_PER,CLKEN_PER_EN_GPIO6_BIT, 1, 1);
    }
}

void set_gpio_oe(int gpio, int value) {

	int bank=0,offset=0;
    	u32 tmp=0;
    	gpio_t *gpio_base;

    	if ( gpio > 191 || gpio < 0) return;

    	bank = gpio/32 + 1;
    	offset=gpio%32;
    	gpio_base = get_gpio_base(bank);

    	/** see TRM p3615, set bit to enable input **/

    	// printf("try to set gpio %d to input\n", gpio);
    	tmp=__raw_readl((u32)&gpio_base->oe);

    	// printf("oe read: %04x\n", tmp);
	if (value) {
		tmp = tmp | (1<<offset);	/** set bit **/
	}
	else {
		tmp = tmp & (~(1 << offset)); /** clear bit **/
	}

    	__raw_writel(tmp,(u32)&gpio_base->oe);
    	// tmp=__raw_readl((u32)&gpio_base->oe);
    	// printf("oe read back: %04x\n", tmp);	
}

int get_gpio_datain(int gpio) {

    int bank=0,offset=0;
    u32 tmp=0;
    gpio_t* gpio_base;

    if (gpio > 191 || gpio < 0) return -1;

    bank = gpio/32 + 1;
    offset = gpio%32;
    gpio_base = get_gpio_base(bank);

    /** check if the gpio is in input mode **/
    tmp=__raw_readl((u32)&gpio_base->oe);

    /** test code for output **/
     //printf("get_gpio_datain, gpio: %d, bank: %d, offset: %d, oe: 0x%04x\n", gpio, bank, offset, tmp);
    if ((tmp & ( 1 << offset) ) == 0) /* oe bit not set */
    {
    	return -2;
    }

    /** read standard datain register **/
    tmp=__raw_readl((u32)&gpio_base->datain);
    return (tmp & ( 1 << offset)) ? 1 : 0;	
}

static int cmd_gpio_out(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[]) {

        int gpio, val;

        if (argc != 3)
        {
                printf("example: gpio_out 171 1\n");
                return -1;
        }

        gpio = simple_strtol(argv[1], 0, 0);
        val = simple_strtol(argv[2], 0, 0);

	enable_gpio_bank(1);
	enable_gpio_bank(2);
	enable_gpio_bank(3);
	enable_gpio_bank(4);
	enable_gpio_bank(5);
	enable_gpio_bank(6);

		if (val) {
			set_gpio_dataout(gpio, 1);
			printf("gpio %d is driven high.\n", gpio);
		}
		else {
			set_gpio_dataout(gpio, 0);
			printf("gpio %d is driven low.\n", gpio);
		}
        

        return 0;
}

U_BOOT_CMD(
        gpio_out, 3, 0, cmd_gpio_out,
        "gpio_out - drive gpio to high or low.\n",
        "nothing.\n");


/** 	gpio soft pwm 

	gpio: gpio number
	frequency: in HZ
	on: on time ratio, in percentage
	off: off time ratio, in percentage
	total: in seconds */
		
static void gpio_pwm(int gpio, int freq, int on, int total) {

        int off, delay, cycle, total_in_cycle;

        if (gpio < 0 || gpio > 191)
        {
                printf("error: gpio_pwm, invalid gpio parameter.\n");
                return;
        }

        if (on < 0 || on > 100)
        {
                printf("error: gpio_pwm, on (time) out of range (0~100).\n");
                return;
        }

        if (total > 600)
        {
                printf("error: gpio_pwm, bitbang, total must be less than or equal to 600 (10 minutes at most).\n");
                return;
        }

	off = 100 - on;
	cycle = (1000000 / freq);
	delay = (10000 / freq);
	if (10000 % freq != 0) {
		printf("warning: gpio_pwm, freq will be round up. To suppress this warning, provide freq that divides 10000 exactly.\n");
	}

	total_in_cycle = total * freq;


	while (total_in_cycle > 0) {
		
		if (on) {
			set_gpio_dataout(gpio, 1);
			udelay(on * delay);
		}
		
		if (off) {
			set_gpio_dataout(gpio, 0);
			udelay(off * delay);
		}
		
		total_in_cycle--;
	}

        set_gpio_dataout(gpio, 0);
}


static int cmd_gpio_pwm(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[]) {

	int gpio, freq, on, total;

	if (argc != 5) {
		printf("example: gpio_pwm gpio frequency on_time(in percentage) total_time(in seconds).\n");
	}

	gpio = simple_strtol(argv[1], 0, 0);
	freq = simple_strtol(argv[2], 0, 0);
	on = simple_strtol(argv[3], 0, 0);
	total = simple_strtol(argv[4], 0, 0);

	gpio_pwm(gpio, freq, on, total);
	return 0;
}

U_BOOT_CMD(
        gpio_pwm, 5, 0, cmd_gpio_pwm,
        "gpio_pwm - drive (bitbang) pwm out of the given gpio\n",
        "nothing.\n");




