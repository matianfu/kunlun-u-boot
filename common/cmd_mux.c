#include <common.h>
#include <command.h>
#include <asm/io.h>
#include <asm/arch/mux.h>



#define     MUX_VAL(OFFSET,VALUE) __raw_writew((VALUE), OMAP34XX_CTRL_BASE + (OFFSET))

int do_mux_sda (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int val = 0;

    val = __raw_readw(OMAP34XX_CTRL_BASE + CONTROL_PADCONF_I2C2_SDA);
	printf("------before----------I2C2_SDA reg:0x%x DIS:%d IE:%d M4:%d\n", val, DIS, IEN, M4);

	val &= ~(1 << 3);
	val |= 1 << 8;
// 	MUX_VAL(CONTROL_PADCONF_I2C2_SDA, (EN  | (0 << 3)  | M4)) /*I2C2_SDA,Camera*/;
	__raw_writew(val, OMAP34XX_CTRL_BASE + (CONTROL_PADCONF_I2C2_SDA));

	val = __raw_readw(OMAP34XX_CTRL_BASE + CONTROL_PADCONF_I2C2_SDA);
	printf("------after----------I2C2_SDA reg:0x%x\n", val);

	return 0;
}

U_BOOT_CMD(mux,    CFG_MAXARGS,    1,  do_mux_sda, "mux - test mux\n", "[<option>]\n""    - with <option> argument: mux <option> \n");

