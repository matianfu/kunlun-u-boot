/*
 * =====================================================================================
 *
 *       Filename:  backlight.c
 *
 *    Description: control LCD backlight level which use CAT3648
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

#if defined(CONFIG_BACKLIGHT_CAT3648) || defined(CONFIG_BACKLIGHT_CAT3637)

#define CONFIG_BACKLIGHT_CAT3648_DEBUG 1

#if CONFIG_BACKLIGHT_CAT3648_DEBUG
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

extern void sr32(u32 addr, u32 start_bit, u32 num_bits, u32 value);

void backlight_set_level (int percent );

#define BACKLIGHT_DEFAULT_PERCENTS 50

static gpio_t *gpio5_base;
static int cat3648_initialized = 0;
static int cat3648_percents = BACKLIGHT_DEFAULT_PERCENTS;

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  cat3648_init
 *  Description:  initialize GPIO pin which cat3648 use as input
 * =====================================================================================
 */
static void cat3648_init(void)
{
    gpio5_base = (gpio_t *)OMAP34XX_GPIO5_BASE;
    /*enable GPIO bank 5*/
    sr32(CM_ICLKEN_PER,CLKEN_PER_EN_GPIO5_BIT, 1, 1);
    sr32(CM_FCLKEN_PER,CLKEN_PER_EN_GPIO5_BIT, 1, 1);

    /*set GPIO155 direction output*/
    /*set GPIO155 low level,LCD BL shutdown */
    set_gpio_dataout(155,0);
    udelay(2000);
    DEBUG_BL("cat3648 initialized\n");
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  cat3648_set_led_current
 *  Description:  set led current
 *  Parameters :  @level: range of 0 - 31,0 is full scale and 31 is zero current
 * =====================================================================================
 */
static void cat3648_set_led_current (int level )
{
#if defined(CONFIG_BACKLIGHT_CAT3637)
    if(level < 0 || level > 15){
        printf("invalid parameter:range of level:0-15\n");
        return ;
    }
#elif defined(CONFIG_BACKLIGHT_CAT3648)
    if(level < 0 || level > 31){
        printf("invalid parameter:range of level:0-31\n");
        return ;
    }
#endif
    if(!cat3648_initialized){
        cat3648_init();
    }

    /*make sure the initial status is FULL SCALE*/

   set_gpio_dataout(155,0);
    udelay(2000);

    set_gpio_dataout(155,1);
    udelay(50);
    /*produce level times pulses*/
    while(level--){

        set_gpio_dataout(155,0);
        udelay(1);

        set_gpio_dataout(155,1);
    }

}		/* -----  end of function cat3648_set_led_current  ----- */



/*
 * ===  FUNCTION  ======================================================================
 *         Name:  backlight_on
 *  Description:  enable backlight
 * =====================================================================================
 */
void backlight_on (void)
{
    backlight_set_level(cat3648_percents);
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
    cat3648_percents = percent;
#if defined(CONFIG_BACKLIGHT_CAT3637)
    level = 15 * percent / 100;
	level = 15 - level;
#elif defined(CONFIG_BACKLIGHT_CAT3648)
    level = 31 * percent / 100;
    level = 31 - level;
#endif
    cat3648_set_led_current(level);
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
        printf("Usage: lcdbl xxx \n       xxx is brightness level(0 - 100)\n");
        return -1 ;
    }
    level = simple_strtoul(argv[1],NULL,10);
    backlight_set_level(level);
    return 0;
}


U_BOOT_CMD(
	lcdbl,	2,	1,	backlight_test,
	"lcdbl  - control backlight brightness through CAT3648\n",
	"<brightness>: 0 - 100\n"
);

#endif
