#include <common.h>
#include <i2c.h>
#include <twl4030.h>	/** this header defines many macro **/

#define REG_INTBR_PMBR1		(0x92)		/* pmbr1 */
#define REG_PWM1ON		(0xFB)
#define REG_PWM1OFF		(0xFC)
#define REG_INTBR_GPBR1		(0x91)

void twl5030_pwm1_init(void) {

	u8 pmbr1 = 0xFF;
	u8 gpbr1 = 0xFF;
	u8 tmp = 0xFF;

	/** step 1: enable PWM1 **/
	i2c_read(TWL4030_CHIP_INTBR, REG_INTBR_PMBR1, 1, &pmbr1, 1);
	printf("twl read, intbr_pmbr1: 0x%02x \n", pmbr1);

	pmbr1 |= ((0x03) << 4);		/** enable pwm1 **/
	i2c_write(TWL4030_CHIP_INTBR, REG_INTBR_PMBR1, 1, &pmbr1, 1);
	printf("twl write, intbr_pmbr1: 0x%02x \n", pmbr1);

	i2c_read(TWL4030_CHIP_INTBR, REG_INTBR_PMBR1, 1, &pmbr1, 1);
	printf("twl read, intbr_pmbr1: 0x%02x \n", pmbr1);

	/** step 2: set duty cycle **/
	/** 	very curious definition, PWM1OFF means how many clocks a PWM cycle has;
	    	PWM1ON means a cycle begins with low level, and after how many clocks,
	    	it drives to high level. 
		p.484, twl5030 datasheet
		uglee, Sun Oct  7 12:52:18 CST 2012
	**/
	tmp = 0x46;	/** duty cycle 30 percents **/
        i2c_write(TWL4030_CHIP_PWM1, REG_PWM1ON, 1, &tmp, 1);         	/* PWM1ON */
	printf("twl write, pwm1on: 0x%02x \n", tmp);	

	tmp = 0x64; 	/** decimal 100 **/
        i2c_write(TWL4030_CHIP_PWM1, REG_PWM1OFF, 1, &tmp, 1);         /* PWM1OFF */
	printf("twl write, pwm1off: 0x%02x \n", tmp);

	/** step 3: enable output and clock **/
	i2c_read(TWL4030_CHIP_INTBR, REG_INTBR_GPBR1, 1, &gpbr1, 1);
	printf("twl read, intbr_gpbr1: 0x%02x \n", gpbr1);
	
	gpbr1 |= (1 << 3); 	/** pwm1 enable **/
	gpbr1 |= (1 << 1);	/** pwm1 clock enable **/ 
	i2c_write(TWL4030_CHIP_INTBR, REG_INTBR_GPBR1, 1, &gpbr1, 1);
	printf("twl write, intbr_gpbr1: 0x%02x \n", gpbr1);

	/** 
        twl_i2c_read_u8(TWL4030_MODULE_INTBR, &gpbr1, REG_INTBR_GPBR1);
        gpbr1 &= ~REG_INTBR_GPBR1_PWM1_OUT_EN_MASK;
        gpbr1 |= (enable ? REG_INTBR_GPBR1_PWM1_OUT_EN : 0);
        twl_i2c_write_u8(TWL4030_MODULE_INTBR, gpbr1, REG_INTBR_GPBR1);	**/
}


