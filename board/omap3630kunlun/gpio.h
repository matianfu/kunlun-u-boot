#ifndef GPIOLIB_H
#define GPIOLIB_H

#include <common.h>
/******************************************************************************

	This file declares some useful operation for gpio mux, read and write.

	Some of the functions are defined in gpiolib.c

	Others are defined in <uboot>/cpu/omap3 but declared nowhere.

	GPL code, matianfu@actnova.com

******************************************************************************/


/** General Comments

	All following gpio-related functions do NOT check gpio mux register.
	This is because there are no hardware-defined one-to-one relationship
	between gpio number and pin name. Be sure to check it via mux function
	before using gpio functions, making sure the pin is working in the mode
	you expected.

	When gpio is working as input, the pull setting bits (PTU/PTD, EN/DIS)
	mux register will take effect.		**/

/** Enable gpio bank 

	Enable ICLK & FCLK for given gpio bank. Must be called before using
	any gpio.

    bank: 1,2,3,4,5,6
    defined in gpio.c   **/
void enable_gpio_bank(int bank);

/** Set gpio output 

	The pin could be muxed as either i/o (IEN) or output only (IDIS) mode.
	If the pin is muxed as i/o (IEN) mode AND the pin is working as input,
	the function will change it to output (by clear oe bit).

	defined in <uboot>/cpu/omap3/syslib.c
	gpio: gpio number 0-191
	value: non-zero for 1, zero for 0 				**/
extern void set_gpio_dataout(int gpio, int value);

/** Set gpio input

	It assumes the pin is muxed as i/o (IEN) mode and set the oe bit.

	If value is non-zero, it set oe bit, which set the gpio as input;
	otherwise, clear the bit and set the gpio as output.

	Note: the oe (Output Enable) name is misleading, see TRM p3615 for detail.

	defined in gpio.c		**/
void set_gpio_oe(int gpio, int value);

/** Get gpio data in

	It assumes the pin is muxed as i/o (IEN) mode AND simply returns the 
	datain register value. see gpio_t definition.

	return 1 if input is high.
	return 0 if input is low.
	return -1 if parameter is out-of-range.
	return -2 if gpio oe bit is cleared (output mode).

	Note: the function does NOT set the gpio to input mode automatically,
	because, if there are some capacitance on trace, a delay is needed
	for stable read result. But the delay time cannot be assumed without
	the knowledge of the external circuit. So, be sure the set gpio as
	input and delay explicitly.
	
	**/
int get_gpio_datain(int gpio);
	
/*****************************************************************************/

/** include asm/arch/sys_proto.h in .c file 
	doesn't cure the compiler complaints **/
extern void sr32(u32 addr, u32 start_bit, u32 num_bits, u32 value);



#endif
