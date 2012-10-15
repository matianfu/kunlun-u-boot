/*
 * =====================================================================================
 *
 *       Filename:  backlight.c
 *
 *    Description: control LCD backlight level which use twl5030_pwm
 *
 *        Version:  1.0
 *        Created:  04/12/2010 09:59:24 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Daqing Li (daqli), daqli@via-telecom.com
 *        Company:  VIA TELECOM
 *
 * =====================================================================================
 */

#include <config.h>

#ifdef CONFIG_BACKLIGHT_TWL5030_PWM

#define CONFIG_BACKLIGHT_TWL5030_PWM_DEBUG 1

#if CONFIG_BACKLIGHT_TWL5030_PWM_DEBUG
#define DEBUG_BL(fmt,args...)  printf(fmt,##args)
#else
#define DEBUG_BL(fmt,args...)
#endif

#include <common.h>
#include <version.h>
#include <stdarg.h>
#include <linux/types.h>
#include <devices.h>
#include <asm/io.h>
#include <command.h>

typedef struct
{
    u8 pwm_on;
    u8 pwm_off;
}PWM_OnOff_t;

static PWM_OnOff_t pwm_onoff[32] ;
#define PWM_LENGTH (128)

extern void sr32(u32 addr, u32 start_bit, u32 num_bits, u32 value);

void backlight_set_level (int percent );

#define BACKLIGHT_DEFAULT_PERCENTS 50

static gpio_t *gpio5_base;
static int twl5030_pwm_initialized = 0;
static int twl5030_pwm_percents = BACKLIGHT_DEFAULT_PERCENTS;

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  twl5030_pwm_init
 *  Description:  initialize GPIO pin which twl5030_pwm use as input
 * =====================================================================================
 */
static void twl5030_pwm_init(void)
{
    u32 reg_value=0;
    int i=0;
    u8 step=4;

    gpio5_base = (gpio_t *)OMAP34XX_GPIO5_BASE;
    /*enable GPIO bank 5*/
    sr32(CM_ICLKEN_PER,CLKEN_PER_EN_GPIO5_BIT, 1, 1);
    sr32(CM_FCLKEN_PER,CLKEN_PER_EN_GPIO5_BIT, 1, 1);

    /*set GPIO155 direction output*/
    /*set GPIO155 low level,LCD BL shutdown */
    set_gpio_dataout(155,1);


    udelay(2000);




    for(i=0;i<32;i++)
    {
        pwm_onoff[i].pwm_on =i*step+1;
        pwm_onoff[i].pwm_off =PWM_LENGTH-1;
    }

    //reg_value |=1<<2;//pad mux select pwm0
    reg_value |=3<<4;//pad mux select pwm1
    i2c_write(0x49, 0x92, 1, &reg_value, 1);
    i2c_read(0x49, 0x92, 1, &reg_value, 1);
    twl5030_pwm_initialized = 1;
    DEBUG_BL("Twl5030_pwm backlight initialized.\n");

}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  twl5030_pwm_set_led_current
 *  Description:  set led current
 *  Parameters :  @level: range of 0 - 31,0 is full scale and 31 is zero current
 * =====================================================================================
 */
static void twl5030_pwm_set_led_current (int level )
{
    u8 reg_value=0;

    if(level < 0 || level > 31){
        printf("invalid parameter:range of level:0-31\n");
        return ;
    }
    if(!twl5030_pwm_initialized){
        twl5030_pwm_init();
    }

    if(level ==0)
    {
        i2c_read(0x49, 0x91, 1, &reg_value, 1);
        //reg_value &= ~((1<<0)|(1<<2));
        reg_value &= ~((1<<1)|(1<<3));
        i2c_write(0x49, 0x91, 1,&reg_value,1);
        i2c_write(0x4a, 0xfb, 1,&pwm_onoff[level].pwm_on,1);
        i2c_write(0x4a, 0xfc, 1,&pwm_onoff[level].pwm_on,1);
    }
    else
    {
        i2c_read(0x49, 0x91, 1, &reg_value, 1);
        //reg_value |= (1<<0)|(1<<2);
        reg_value |= (1<<1)|(1<<3);
        i2c_write(0x49, 0x91, 1,&reg_value,1);
        i2c_write(0x4a, 0xfb, 1,&pwm_onoff[level].pwm_on,1);
        i2c_write(0x4a, 0xfc, 1,&pwm_onoff[level].pwm_off,1);
    }




}		/* -----  end of function twl5030_pwm_set_led_current  ----- */



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  backlight_on
 *  Description:  enable backlight
 * =====================================================================================
 */
void backlight_on (void)
{

    backlight_set_level(twl5030_pwm_percents);
}		/* -----  end of function backlight_on  ----- */



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  backlight_off
 *  Description:  disable backlight
 * =====================================================================================
 */
void backlight_off (void)
{
    backlight_set_level(0);
}		/* -----  end of function backlight_off  ----- */



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  backlight_set_level
 *  Description:  set backlight level,exported for external call
 *  Parameters : @percent: range of 0 - 100,where 0 is the darkesst,100 is the brightest
 * =====================================================================================
 */
void backlight_set_level (int percent )
{
    int level = 0;
    if(percent < 0 || percent > 100){
        printf("invalid parameter: percent should be 0 - 100\n");
        return;
    }
    twl5030_pwm_percents = percent;

    level = 31 * percent / 100;
    //level = 31 - level;
    twl5030_pwm_set_led_current(level);
}		/* -----  end of function backlight_set_level  ----- */

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  backlight_test
 *  Description:  test backlight control
 * =====================================================================================
 */

static int backlight_test(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    u8 level;
    if (argc != 2) {
        printf("Usage: pwm xxx \n       xxx is brightness level(0 - 100)\n");
        return -1 ;
    }
    level = simple_strtoul(argv[1],NULL,10);
    backlight_set_level(level);
    return 0;
}


U_BOOT_CMD(
	pwm,	2,	1,	backlight_test,
	"lcdbl  - control backlight brightness through twl5030_pwm\n",
	"<brightness>: 0 - 100\n"
);
#endif
