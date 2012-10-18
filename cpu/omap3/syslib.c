/*
 * (C) Copyright 2006
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
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
#include <asm/arch/mem.h>
#include <asm/arch/clocks.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>

/************************************************************
 * sdelay() - simple spin loop.  Will be constant time as
 *  its generally used in bypass conditions only.  This
 *  is necessary until timers are accessible.
 *
 *  not inline to increase chances its in cache when called
 *************************************************************/
void sdelay(unsigned long loops)
{
	__asm__ volatile ("1:\n" "subs %0, %1, #1\n"  
					  "bne 1b":"=r" (loops):"0"(loops));
}

/*****************************************************************
 * sr32 - clear & set a value in a bit range for a 32 bit address
 *****************************************************************/
void sr32(u32 addr, u32 start_bit, u32 num_bits, u32 value)
{
	u32 tmp, msk = 0;
	msk = 1 << num_bits;
	--msk;
	tmp = __raw_readl(addr) & ~(msk << start_bit);
	tmp |=  value << start_bit;

	__raw_writel(tmp, addr);
}

/** get gpio bank base **/
static gpio_t* get_gpio_base(int bank)
{
	gpio_t* gpio_base;

	if (bank<0||bank>5)
		return 0;

    switch(bank)
    {
        case 0:
            gpio_base = (gpio_t *)OMAP34XX_GPIO1_BASE;
            break;
        case 1:
            gpio_base = (gpio_t *)OMAP34XX_GPIO2_BASE;
            break;
        case 2:
            gpio_base = (gpio_t *)OMAP34XX_GPIO3_BASE;
            break;
        case 3:
            gpio_base = (gpio_t *)OMAP34XX_GPIO4_BASE;
            break;
        case 4:
            gpio_base = (gpio_t *)OMAP34XX_GPIO5_BASE;
            break;
        case 5:
            gpio_base = (gpio_t *)OMAP34XX_GPIO6_BASE;
            break;
    }

	return gpio_base;			
}

/** UGlee **/
/** original code **/
void set_gpio_dataout(int gpio, int value)
{
    int bank=0,offset=0;
    u32 base,tmp=0;
    gpio_t *gpio_base;

    if(gpio>=192||gpio<0)
        return;

    bank= gpio/32;
    offset = gpio%32;
	gpio_base = get_gpio_base(bank);

    /** see TRM p3615, clear bit to enable output **/
    tmp=__raw_readl((u32)&gpio_base->oe)&~(1<<offset);
    __raw_writel(tmp,(u32)&gpio_base->oe);

    /** set or clear, manipulate only 1 bit **/
    tmp = 1<<offset;
    if(value)
        __raw_writel(tmp,(u32)&gpio_base->setdataout);
    else
        __raw_writel(tmp,(u32)&gpio_base->cleardataout);
}

/** UGlee **/
/** set gpio to input mode **/
void set_gpio_to_input(int gpio)
{
	int bank=0,offset=0;
	u32 base,tmp=0;
	gpio_t *gpio_base;

	if(gpio>=192||gpio<0)
		return;
	
	bank = gpio/32;
	offset=gpio%32;
	gpio_base = get_gpio_base(bank);

	/** see TRM p3615, set bit to enable input **/
	
	// printf("try to set gpio %d to input\n", gpio);
	tmp=__raw_readl((u32)&gpio_base->oe);
	// printf("oe read: %04x\n", tmp);
	tmp |= (1<<offset);
	// printf("oe bit set: %04x\n", tmp);
	__raw_writel(tmp,(u32)&gpio_base->oe);
	// tmp=__raw_readl((u32)&gpio_base->oe);
	// printf("oe read back: %04x\n", tmp);
}

/****************************************************

	read gpio input
	be sure to check return value

	return 1, input is high
	return 0, input is low
	return -1, invalid gpio parameter
	return -2, gpio is not in input mode

*****************************************************/
int read_gpio_input(int gpio)
{
	int bank=0,offset=0;
	u32 tmp=0;
	gpio_t* gpio_base;

	if (gpio>=192||gpio<0)
		return -1;

	bank = gpio/32;
	offset = gpio%32;
	gpio_base = get_gpio_base(bank);

	/** check if the gpio is in input mode **/
	tmp=__raw_readl((u32)&gpio_base->oe);

	/** test code for output **/
	// printf("read_gpio_input, gpio: %d, bank: %d, offset: %d, oe: 0x%04x\n", gpio, bank, offset, tmp);
	if ((tmp & (1<<offset)) == 0) /* oe bit not set */
	{
		return -2;
	}
	
	/** read standard datain register **/
	tmp=__raw_readl((u32)&gpio_base->datain);
	return (tmp & (1<<offset)) ? 1 : 0;
}



/*********************************************************************
 * wait_on_value() - common routine to allow waiting for changes in
 *   volatile regs.
 *********************************************************************/
u32 wait_on_value(u32 read_bit_mask, u32 match_value, u32 read_addr, u32 bound)
{
	u32 i = 0, val;
	do {
		++i;
		val = __raw_readl(read_addr) & read_bit_mask;
		if (val == match_value)
			return (1);
		if (i == bound)
			return (0);
	} while (1);
}

