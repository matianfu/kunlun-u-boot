
#include <common.h>
#include <command.h>
#include <asm-arm/io.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>

/********************************************************
 *
 *	CSB shoud be mux as gpio, output, pullup
 *	SCL shoud be mux as gpio, output, pullup
 *	SDA shoud be mux as gpio, input/output, no pull
 * 	
 *	GPIO_PWM_BITBANG should be mux as gpio, input/output, no pull
 *
 ********************************************************/

#define GPIO_CSB		171 	/* S105 on tomahawk, McSPI1_CLK */
#define GPIO_SCL		172	/* S104 on tomahawk, McSPI1_SIMO */	
#define GPIO_SDA		173	/* S103 on tomahawk, McSPI1_SOMI */
#define GPIO_PWM_BITBANG	174 	/* S102 on tomahawk, McSPI1_CS0 */

#define CSB_HIGH		set_gpio_dataout(GPIO_CSB, 1)
#define CSB_LOW			set_gpio_dataout(GPIO_CSB, 0)
#define SCL_HIGH		set_gpio_dataout(GPIO_SCL, 1)
#define SCL_LOW			set_gpio_dataout(GPIO_SCL, 0)
#define SDA_HIGH		set_gpio_dataout(GPIO_SDA, 1)
#define SDA_LOW			set_gpio_dataout(GPIO_SDA, 0)
#define HALF_CYCLE		500
#define INTER_FRAME		10000
#define PWM_UNIT		(1000 * 10)

extern void set_gpio_dataout(int gpio, int val);
extern void set_gpio_to_input(int gpio);
extern int read_gpio_input(int gpio);

static void lg_3wire_write(int addr, int val);
static void lg_3wire_write2(int addr, int val);
static int lg_3wire_read(int addr);
static void gpio_pwm_on(void);
static void gpio_pwm_off(void);
static void gpio_pwm_bitbang(int on, int off, int total);

static void print_mux(char* desc, unsigned short mux);

/*enable GPIO #bank function and interface clock*/
static void gpio_enable(int bank)
{
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
    return;
}

static int cmd_gpio_enable(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{

	gpio_enable(1);
	gpio_enable(2);
	gpio_enable(3);
	gpio_enable(4);
	gpio_enable(5);
	gpio_enable(6);

	printf("cmd gpioen: all six gpio banks are enabled now.\n");
	return 0;
}

U_BOOT_CMD(
	gpioen, 1, 1, cmd_gpio_enable,
	"gpioen - enable all gpio banks\n",
	"no params");

static void lg_3wire_write_read(unsigned int reg, unsigned int val)
{
	unsigned int rval;
	printf("write reg:%02x, val:%04x\n", reg, val);
	lg_3wire_write2(reg, val);

	rval = lg_3wire_read(reg);
	printf("read  reg:%02x, val:%04x\n", reg, rval);
}

static void init_lg_3wire(void)
{
	unsigned int tmp;
	unsigned short mux;

	CSB_HIGH;
	SCL_HIGH;

	mux = __raw_readw(OMAP34XX_CTRL_BASE + CONTROL_PADCONF_McSPI1_SOMI);
	print_mux("MCSPI1_SOMI as LCD_SDA", mux);

	// lg_3wire_write_read(0x00, 0xAD);
	
	// tmp=lg_3wire_read(0x01);
	// printf("test read R01 result is %02x\n", tmp);
	// lg_3wire_write2(0x00, 0x5A5A);

	/** according to datasheet
		recommended register setting for CABC off Mode **/


	lg_3wire_write2(0x00, 0x21);	/* Reset */
	lg_3wire_write2(0x00, 0x65);	/* Into Standby mode */
	lg_3wire_write2(0x01, 0x30);	/* Enable FRC/Dither (CABC Off Mode) */
	lg_3wire_write2(0x02, 0x40);	/* Enable Normally Black */
	lg_3wire_write2(0x0E, 0x5F);	/* Enter Test mode (1) */
	lg_3wire_write2(0x0F, 0xA4);	/* Enter Test mode (2) */
	lg_3wire_write2(0x0D, 0x00);	/* Increase line charging time */

	// lg_3wire_write2(0x02, 0x43);	/* Adjust charge sharing time */
	lg_3wire_write2(0x02, 0x03);    /* clear D6 bit for normal white */

	lg_3wire_write2(0x0A, 0x28);	/* Trigger bias reduction */
	lg_3wire_write2(0x10, 0x41);	/* Adopt 2 Line / 1 Dot */
	lg_3wire_write2(0x00, 0xAD);	/* PWM On, Released standby mode */	


	CSB_HIGH;
	SCL_HIGH;

	mux = __raw_readw(OMAP34XX_CTRL_BASE + CONTROL_PADCONF_McSPI1_SOMI);
	print_mux("MCSPI1_SOMI as LCD_SDA", mux);

}

static int do_lg_init(cmd_tbl_t* cmdtp, int flag, int arc, char* argv[])
{
	init_lg_3wire();
}

U_BOOT_CMD(
	lginit, 1, 0, do_lg_init,
	"lginit - init lg lcd via 3wire.\n",
	"");

/**************************************
 *
 * set software pwm gpio to high
 *
 **************************************/
static void gpio_pwm_on(void) {

	set_gpio_dataout(GPIO_PWM_BITBANG, 1);
}

/**************************************
 *
 * set software pwm gpio to low
 *
 **************************************/
static void gpio_pwm_off(void) {

	set_gpio_dataout(GPIO_PWM_BITBANG, 0);
}

/**************************************
 *
 *
 *
 **************************************/
static int cmd_pwm_bitbang_gpio_high(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[]) {
	
	gpio_pwm_on();

	return 0;
}

U_BOOT_CMD(
	pbioh, 1, 1, cmd_pwm_bitbang_gpio_high,
	"set hyena v1 pwm bitbang gpio to high\n",
	"no parameter\n");


static int cmd_pwm_bitbang_gpio_low(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[]) {

	gpio_pwm_off();

	return 0;
}

U_BOOT_CMD(
	pbiol, 1, 1, cmd_pwm_bitbang_gpio_low,
	"set hyena v1 pwm bitbang gpio to low\n",
	"no parameter\n");

static void gpio_pwm(int gpio, int on, int off, int total) {

	int i,j;

	if (gpio<0 || gpio>192)
	{
		printf("error: gpio_pwm_bitbang, invalid gpio parameter.\n");
		return;
	}

	if ((on + off) != 10) 
	{
		printf("gpio pwm bitbang, parameter error, on + off must equals 10 \n");
		return;
	}

	if (total > 3000000)
	{
		printf("gpio pwm bitbang, parameter error, total must be less than or equal to 3000 \n");
		return;
	}

	set_gpio_dataout(gpio, 0);

	for (i = 0; i < total; i++)
	{
		for (j = 0; j < on; j++)
		{
			set_gpio_dataout(gpio, 1);
			udelay(1000);
		}

		for (j = 0; j < off; j++)
		{
			set_gpio_dataout(gpio, 0);
			udelay(1000);
		}
	}

	set_gpio_dataout(gpio, 0);
}

static int cmd_pwm_bitbang(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[]) {
	
	int on, off, total;	

	if (argc != 4) 
	{
		printf("all three parameters required: on, off, total");
		return -1;
	}

	on = simple_strtol(argv[1], 0, 0);
	off = simple_strtol(argv[2], 0, 0);
	total = simple_strtol(argv[3], 0, 0);
	
	gpio_pwm(GPIO_PWM_BITBANG, on, off, total);

	return 0;
}

U_BOOT_CMD(
	pwmbb, 4, 1, cmd_pwm_bitbang,
	"pwm bitbang via gpio 174\n",
	"nothing.\n");

/*******************************************************/

/************
	
	now, many things are hard coded
	refactor it later.
	UGlee

*************/

static void print_mux(char* pinname, unsigned short mux)
{
	printf("print mux, pin name: %s, mux value: 0x%04x.\n", pinname, mux);
	
	if (mux & IEN) {
		printf("  mode: input/output\n");
	}
	else {
		printf("  mode: output only\n");
	}

	if (mux & EN) {
		printf("  internal pull up/down: enabled\n");
	}
	else { 
		printf("  internal pull up/down: disabled\n");
	}


	if (mux & PTU) {
		printf("  pull-up if internal pull enabled\n");
	}
	else {
		printf("  pull-down if internal pull enabled\n");
	}
}

/**
static int do_print_mux(cmd_tbl_t* cmdtp, int flag, int argc, char* argv[])
{
	int
}
**/

static int test_io_output(int gpio)
{
	int on = 5;
	int off = 5;
	int total = 100000;
	unsigned short old_mux, gpio_out_mux, tmp;
	
	gpio_out_mux = (IDIS | PTD | DIS | M4);

	switch (gpio) 
	{
		case 171:
			old_mux = __raw_readw(OMAP34XX_CTRL_BASE + CONTROL_PADCONF_McSPI1_CLK);
			print_mux("McSPI1_CLK as LCD_CSB, read from register", old_mux);
			
			print_mux("McSPI1_CLK as LCD_CSB, being written to register", gpio_out_mux);

			__raw_writew(gpio_out_mux, OMAP34XX_CTRL_BASE + CONTROL_PADCONF_McSPI1_CLK);

			tmp = __raw_readw(OMAP34XX_CTRL_BASE + CONTROL_PADCONF_McSPI1_CLK);
			print_mux("McSPI1_CLK as LCD_CSB, read back from register", tmp);
			
			gpio_pwm(171, on, off, total);

			print_mux("McSPI1_CLK as LCD_CSB, old mux to restore register after test", old_mux);
			__raw_writew(old_mux, OMAP34XX_CTRL_BASE + CONTROL_PADCONF_McSPI1_CLK);

			tmp = __raw_readw(OMAP34XX_CTRL_BASE + CONTROL_PADCONF_McSPI1_CLK);
			print_mux("McSPI1_CLK as LCD_CSB, old mux restored and read back from register", tmp);
			
			break;

		case 172:
			old_mux = __raw_readw(OMAP34XX_CTRL_BASE + CONTROL_PADCONF_McSPI1_SIMO);
			print_mux("McSPI1_SIMO as LCD_SCL", old_mux);
			__raw_writew(gpio_out_mux, OMAP34XX_CTRL_BASE + CONTROL_PADCONF_McSPI1_SIMO);
			gpio_pwm(172, on, off, total);
			__raw_writew(old_mux, OMAP34XX_CTRL_BASE + CONTROL_PADCONF_McSPI1_SIMO);
			break;

		case 173:

			break;

		case 174:
	
			break;
		
		default:
			printf("sorry, gpio %d test is not supported yet.\n");
			return -1;
	}
	
	return 0;
}

static int do_test_io_output(cmd_tbl_t* cmdtp, int flags, int argc, char* argv[])
{
	int gpio;

	if (argc < 2)
	{
		printf("This command needs gpio number as parameter.\n");
	}

	gpio = simple_strtol(argv[1], 0, 0);
	if (gpio <= 0)
	{
		printf("Invalide gpio number.\n");
	}

	return test_io_output(gpio);
}

U_BOOT_CMD(
	tstio, 2, 0, do_test_io_output,
	"test_gpio_output - test gpio output, you should get a rect wave on output for a short period.\n",
	"sample: test_gpio_output 172\n");

/*******************************************************/
static void lg_3wire_write(int addr, int val)
{
	int i;

	/** chip select **/
	CSB_LOW;
	
	/** write address **/
	for (i = 5; i >= 0; i--) {

		SCL_LOW;
		if ((1 << i) & addr) {
			SDA_HIGH;
		}
		else {
			SDA_LOW;
		}

		udelay(HALF_CYCLE);
		SCL_HIGH;
		udelay(HALF_CYCLE);
	}

	/** write w/r bit **/
	SCL_LOW;
	SDA_LOW;	/* 0 for write */
	udelay(HALF_CYCLE);
	SCL_HIGH;
	udelay(HALF_CYCLE);

	/** Hi-Z **/
	SCL_LOW;
	udelay(HALF_CYCLE);
	SCL_HIGH;
	udelay(HALF_CYCLE);

	/** data **/
	for (i = 7; i >= 0; i--) {
		
		SCL_LOW;
		if ((1 << i) & val) {
			SDA_HIGH;
		}
		else {
			SDA_LOW;
		}

		udelay(HALF_CYCLE);
		SCL_HIGH;
		udelay(HALF_CYCLE);
	}

	CSB_HIGH;
	udelay(INTER_FRAME);
}

static void lg_3wire_write2(int addr, int val) {

	unsigned int bits = 0;
	int i;

	bits = (addr << 10) & 0xFC00;
	/* D9 is 0 for write */
	/* D8 is anything for write */
	bits |= val & 0xFF;  /* important! this is actually a byte, higher bits will mess up address */

	printf("lg 3wire write2, addr 0x%02x, val 0x%04x, bits 0x%04x", addr, val, bits);
	CSB_LOW;

	for (i = 15; i >= 0; i--)
	{
		SCL_LOW;
		if ((1 << i) & bits) {
			SDA_HIGH;
		}
		else {
			SDA_LOW;
		}
		udelay(HALF_CYCLE);
		SCL_HIGH;
		udelay(HALF_CYCLE);

	}

	set_gpio_to_input(GPIO_SDA);
	CSB_HIGH;
	udelay(INTER_FRAME);
}


static int lg_3wire_read(int addr)
{
	int i;
	int val = 0;
	int tmp;

	/** chip select **/
	CSB_LOW;
	
	/** write address **/
	for (i = 5; i >= 0; i--) {

		SCL_LOW;

		if ((1 << i) & addr) {
			SDA_HIGH;
		}
		else {
			SDA_LOW;
		}

		udelay(HALF_CYCLE);
		SCL_HIGH;
		udelay(HALF_CYCLE);
	}

	/** write w/r bit **/
	SCL_LOW;
	SDA_HIGH;	/* 1 for read */
	udelay(HALF_CYCLE);
	SCL_HIGH;
	udelay(HALF_CYCLE);

	/** Hi-Z **/
	SCL_LOW;
	set_gpio_to_input(GPIO_SDA);
	udelay(HALF_CYCLE);
	SCL_HIGH;
	udelay(HALF_CYCLE);
	
	/** data **/
	for (i = 7; i >= 0; i--) {
		
		/** according to datasheet, in read mode, the returned data
			should be latched at the rising edge of SCL by external
			controller. It should be understood as reading the returned
			data after rising edge, I guess. **/
		SCL_LOW;
		udelay(HALF_CYCLE);
		SCL_HIGH;		
		
		tmp = read_gpio_input(GPIO_SDA); /** bug removed, yeah !! */
		if (tmp < 0 || tmp > 1) {
			printf("lg_lvds_lcd.c, read(), read_gpio_input return error %d \n", tmp);
			hang();
		}
		if (tmp == 1) {
			val |= (1<<i);
		}
		udelay(HALF_CYCLE);
		/** no need to clear bit since val is init as 0 **/
	}

	CSB_HIGH;
	udelay(INTER_FRAME);

	return val;	
}

/******************************************************************/

