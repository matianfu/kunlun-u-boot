#include <asm/arch/misc.h>

/** do nothing now **/
void vib_ctrl(int value) {

#if 0
	u32 val;

	sr32(CM_ICLKEN_WKUP,3, 1, 1);   //GPIO 1 interface clock is enabled
	sr32(CM_FCLKEN_WKUP,3, 1, 1);  //GPIO 1 functional clock is enabled

	val = __raw_readl(0x480025F8);  //GPIO_28 mode reg
	val = val & (0xFFFFFEF8)|(0x04);  // gpio & output mode
	__raw_writel(val, 0x480025F8);

	if(1==value)
	{
    		set_gpio_dataout(28, 1);
	}
 	else
  	{
   		set_gpio_dataout(28, 0);
  	}


#endif
}
