#include <common.h>
#include <command.h>
#include <asm/arch/mux.h>
#include "gpio.h"
#include "lg_lcd.h"

#define LCD_CS				0
#define LCD_SCL				1
#define LCD_SDA				2

#define HALF_CYCLE			25
#define INTER_FRAME			500

#define HIGH(GPIO_INDEX)	set_gpio_dataout(gpio_pins[GPIO_INDEX].gpio, 1);
#define LOW(GPIO_INDEX)		set_gpio_dataout(gpio_pins[GPIO_INDEX].gpio, 0);

/** in future this struct should contain a pointer to a complete table, including
	pin name, pad name, etc. **/
typedef struct {
	char 	pinname[16];
	int		gpio;		
	u32		padconf;		/** this is address, should be converted to 
								(volatile unsigned char/short/int*) when used.
								example:
									(*(volatile unsigned char *)(a) = (v))
								or use 
									__raw_writeb, __raw_writew, __raw_writel
									__raw_readb,  __raw_readw,  __raw_readl
								instead.							**/
	u16		default_mux;	/** default mux, used for restore mux after test **/
} gpio_pin;

static gpio_pin gpio_pins[3] = {	
						{	"LCD_CS" ,0, 0, (IEN | PTD | DIS | M4)},
						{	"LCD_SCL",0, 0, (IEN | PTD | DIS | M4)},
						{
							"LCD_SDA",
							0,
							0,
							(IEN | PTD | DIS | M4)
						}
					 };

static int initialized = 0;

static void lg_3wire_write(int addr, u8 val);
static int lg_3wire_read(int addr);

/** This function must be called before any commands can be used
	The best place to call this function is in misc_init_r() in board.c **/
void lg_3wire_init(u32 pd_cs, int io_cs, 
					   u32 pd_scl, int io_scl, 
	                   u32 pd_sda, int io_sda,
					   int checkmux, int print) {
	int i;
	
	/** init data **/
	gpio_pins[0].padconf = pd_cs;
	gpio_pins[0].gpio = io_cs;
	gpio_pins[1].padconf = pd_scl;
	gpio_pins[1].gpio = io_scl;
	gpio_pins[2].padconf = pd_sda;
	gpio_pins[2].gpio = io_sda;

	if (checkmux) {
		for (i = 0; i < 3; i++) {
			//TODO
		}
	}

	/** enable gpio bank **/
	enable_gpio_bank(1);
	enable_gpio_bank(2);
	enable_gpio_bank(3);
	enable_gpio_bank(4);
	enable_gpio_bank(5);
	enable_gpio_bank(6);

	/** drive cs and scl **/
	set_gpio_dataout(gpio_pins[LCD_CS].gpio, 1);
	set_gpio_dataout(gpio_pins[LCD_SCL].gpio, 1);

	initialized = 1;

	printf("lg 3wire interface initialized.\n");
}

static void lg_3wire_write(int addr, u8 val) {

        unsigned int bits = 0;
        int i;

		if (!initialized) {
			printf("lg 3wire interface not initialized. operation unavailable.\n");
			return;
		}

        bits = (addr << 10) & 0xFC00;

        /* D9 is 0 for write */
        /* D8 is anything for write */

        bits |= val & 0xFF;  /* important! this is actually a byte, higher bits will mess up address */

        printf("lg_3wire_write, addr 0x%02x, val 0x%04x, bits 0x%04x\n", addr, val, bits);
        
		/** drive cs low **/
		LOW(LCD_CS)

        for (i = 15; i >= 0; i--)
        {
                /** drive scl low **/
				LOW(LCD_SCL)
                if ((1 << i) & bits) {
                	/** drive sda high **/
					HIGH(LCD_SDA)
                }
                else {
                	/** drive sda low **/
					LOW(LCD_SDA)
                }
                udelay(HALF_CYCLE);
				/** drive scl high **/
                HIGH(LCD_SCL)
                udelay(HALF_CYCLE);

        }

        /** drive cs high **/
		HIGH(LCD_CS)

        udelay(INTER_FRAME);
}

static int lg_3wire_read(int addr)
{
        int i;
        int val = 0;
        int tmp;

		if (!initialized) {
			printf("lg 3wire interface not initialized. operation unavailable.\n");
			return -1;
		}

        /** chip select **/
		LOW(LCD_CS)        

        /** write address **/
        for (i = 5; i >= 0; i--) {

                LOW(LCD_SCL)

                if ((1 << i) & addr) {
                        HIGH(LCD_SDA)
                }
                else {
                        LOW(LCD_SDA)
                }

                udelay(HALF_CYCLE);
                HIGH(LCD_SCL)
                udelay(HALF_CYCLE);
        }

        /** write w/r bit **/
        LOW(LCD_SCL)
        HIGH(LCD_SDA)       /* 1 for read */
        udelay(HALF_CYCLE);
        HIGH(LCD_SCL)
        udelay(HALF_CYCLE);

        /** Hi-Z **/
		set_gpio_oe(gpio_pins[LCD_SDA].gpio, 1);	/** set oe bit enabling input **/
        LOW(LCD_SCL)
        udelay(HALF_CYCLE);
        HIGH(LCD_SCL)
        udelay(HALF_CYCLE);
        /** data **/
        for (i = 7; i >= 0; i--) {

                /** according to datasheet, in read mode, the returned data
                        should be latched at the rising edge of SCL by external
                        controller. It should be understood as reading the returned
                        data after rising edge, I guess. **/
                LOW(LCD_SCL)
                udelay(HALF_CYCLE);
                HIGH(LCD_SCL)

                tmp = get_gpio_datain(gpio_pins[LCD_SDA].gpio);
                if (tmp < 0 || tmp > 1) {
                        printf("lg_lcd.c, read(), read_gpio_input return error %d \n", tmp);
                        hang();
                }
                if (tmp == 1) {
                        val |= (1<<i);
                }
                udelay(HALF_CYCLE);
                /** no need to clear bit since val is init as 0 **/
        }

        HIGH(LCD_CS)
        udelay(INTER_FRAME);

        return val;
}


static int cmd_lg_write(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[]) {

        int reg, val;
        
        if (argc != 3) {
                printf("example: lg_write 0 1\n");
		}

		reg = simple_strtol(argv[1], 0, 0);
		val = simple_strtol(argv[2], 0, 0);

		if ((reg != 0) && (reg != 0x01) && (reg != 0x02) && (reg != 0x03) &&
			(reg != 0x0A) && (reg != 0x0D) && (reg != 0x0E) && (reg != 0x0F) &&
			(reg != 0x10)) {

			printf("invalid register number. Check the datasheet.\n");

			return 0;
		}

		if (val < 0 || val > 0xFF) {
			
			printf("value to set is out of range.\n");

			return 0;
		}

		lg_3wire_write(reg, val);
		printf("cmd_lg_write: reg 0x%02x, val 0x%02x \n", reg, val);
		return 0;
}

U_BOOT_CMD(
        lg_write, 3, 0, cmd_lg_write,
        "lg_write - write value to lg lcd register.\n",
        "nothing.\n");

static int cmd_lg_read(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[]) {

	int reg, val = 0;

	if (argc != 2) {
		printf("example: lg_read 0 \n");
	}

	reg = simple_strtol(argv[1], 0, 0);
	
	if ((reg != 0) && (reg != 0x01) && (reg != 0x02) && (reg != 0x03) &&
		(reg != 0x0A) && (reg != 0x0D) && (reg != 0x0E) && (reg != 0x0F) &&
		(reg != 0x10)) {

		printf("invalid register number. check the datasheet.\n");

		return 0;
	}

	val = lg_3wire_read(reg);
	printf("cmd_lg_read: reg: 0x%02x, return val: 0x%02x \n", reg, val);

	return 0;
}

U_BOOT_CMD(
		lg_read, 2, 0, cmd_lg_read,
		"lg_read - read value from lg lcd register.\n",
		"nothing.\n");

static void reg_rw_print(int step, int reg, int write, int read) {

		printf("step %d, rw mismatch, reg: 0x%02x, write: 0x%02x, read: %02x \n", step, reg, write, read);
}

static void lg_3wire_write_read(int reg, int val) {

		int readback;

		lg_3wire_write(reg, val);
		readback = lg_3wire_read(reg);
		if (readback == val) {
			printf("write_read successful, reg: 0x%02x, val: 0x%02x \n", reg, val);
		}
		else {
			printf("write_read mismatch, reg: 0x%02x, write: 0x%02x, read: 0x%02x \n", reg, val, readback);
		}
}

void lg_3wire_init_regs(int normalblack) {

	if (initialized == 0) {

		printf("lg 3wire interface not properly initialized.\n");
		return;

	}
	
	if (normalblack) {
		printf("lg lcd is being initialized to normal black.\n");
	}
	else {
		printf("lg lcd is being initialized to normal white.\n");
	}

	// this recommended settings are exactly same with kindle fire source code
	// normal black 	
	
        lg_3wire_write_read(0x00, 0x21);    /* Reset */
        lg_3wire_write_read(0x00, 0xA5);    /* Into Standby mode */
        lg_3wire_write_read(0x01, 0x30);    /* Enable FRC/Dither (CABC Off Mode) */
        lg_3wire_write_read(0x02, 0x40);    /* Enable Normally Black */
        lg_3wire_write_read(0x0E, 0x5F);    /* Enter Test mode (1) */
        lg_3wire_write_read(0x0F, 0xA4);    /* Enter Test mode (2) */
        lg_3wire_write_read(0x0D, 0x00);    /* Increase line charging time */
	
	/** Adjust charge sharing time **/
	if (normalblack) {
        	lg_3wire_write_read(0x02, 0x43); 	/* set D6 bit for normal black */
	}
	else {
        	lg_3wire_write_read(0x02, 0x03);    	/* clear D6 bit for normal white */
	}

        lg_3wire_write_read(0x0A, 0x28);    /* Trigger bias reduction */
        lg_3wire_write_read(0x10, 0x41);    /* Adopt 2 Line / 1 Dot */
	/** default value 0xAD, cannot change to HSD/VSD mode, use 0xAC */
        lg_3wire_write_read(0x00, 0xAD);    /* PWM On, Released standby mode */
	

}

/** dirty, software **/
static void bl_test(void) {
}









