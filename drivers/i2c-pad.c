/*
 * (C) Copyright 2009 viatelecom
 *  version : v0.9
 *  date    : 2009-06-23
 *  author  : lou hui
 *  func    : through twl5030 get key pad pressed information
 *  log     : 2009-06-23 louhui creat the file
 *
 * See file CREDITS for list of people who contributed to this
 * project.
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
#include <command.h>
#include <i2c.h>
#include <asm/byteorder.h>
#include <twl4030.h>
#include <asm/arch/led.h>

#ifdef CONFIG_CMD_TWL4030_KEYPAD

/* Description:  through twl5030 i2c write u8
 * Parameters :  chip_no:  chip addr
 *               val    :  want to set val
 *               reg    :  register offset
 * Response   : 0 : OK
 *              other :ERROR
 *
 */
static inline int twl5030_i2c_write_u8(u8 chip_no, u8 val, u8 reg)
{
        return i2c_write(chip_no, reg, 1, &val, 1);
}

/* Description:  through twl5030 i2c read u8
 * Parameters :  chip_no:  chip addr
 *               val    :  read data point
 *               reg    :  register offset
 * Response   : 0 : OK
 *              other :ERROR
 *
 */
static inline int twl5030_i2c_read_u8(u8 chip_no, u8 *val, u8 reg)
{
        return i2c_read(chip_no, reg, 1, val, 1);
}

/* Description:  init twl5030 keypad to work in softmode
 * Parameters :  N/A
 * Response   : 0 : OK
 *              other :ERROR
 *
 */
extern int twl5030_keypad_init_softmode(void)
{
	int ret = 0;
	u8 ctrl;
	ret = twl5030_i2c_read_u8(TWL4030_CHIP_KEYPAD, &ctrl, KEYPAD_KEYP_CTRL_REG);
	if (!ret) {
		ctrl |= KEYPAD_CTRL_KBD_ON | KEYPAD_CTRL_SOFT_NRST;
		ctrl &= ~KEYPAD_CTRL_SOFTMODEN;
		ret = twl5030_i2c_write_u8(TWL4030_CHIP_KEYPAD, ctrl, KEYPAD_KEYP_CTRL_REG);
	}
	return ret;
}
#if 0
int twl5030_keypad_reset(void)
{
	int ret = 0;
	u8 ctrl;
	ret = twl5030_i2c_read_u8(TWL4030_CHIP_KEYPAD, &ctrl, KEYPAD_KEYP_CTRL_REG);
	if (!ret) {
		ctrl &= ~KEYPAD_CTRL_SOFT_NRST;
		ret = twl5030_i2c_write_u8(TWL4030_CHIP_KEYPAD, ctrl, KEYPAD_KEYP_CTRL_REG);
	}
	return ret;
}
#endif

/* Description:  scan twl5030 keypad ,find which key or keys is/are pressed
 * Parameters :  exitflag : when two specific keys pressed ,then set it
 *
 * Response   : ret : how many keys are pressed
 *
 *
 */
extern int twl5030_keypad_keys_pressed_softmode(u8 *exitflag)
{
	int ret = 0;
	u8 cb, c, rb, r;
	u8 key1 = 0,key2 = 0;
	for (cb = 0; cb < 8; cb++) {
		c = 0xff & ~(1 << cb);
		twl5030_i2c_write_u8(TWL4030_CHIP_KEYPAD, c, KEYPAD_KBC_REG);
		twl5030_i2c_read_u8(TWL4030_CHIP_KEYPAD, &r, KEYPAD_KBR_REG);
		for (rb = 0; rb < 8; rb++) {
			if (!(r & (1 << rb))) {
                                printf("cb = %d , rb = %d \n",cb,rb);
				if (!ret)
					key1 = cb << 3 | rb;
				else if (1 == ret)
					key2 = cb << 3 | rb;
				ret++;
			}
		}
	}

	if ((key1==1) && (key2==2))
	{
    *exitflag = 1;
  }
	return ret;
}



 /* Description:  set twl5030 keypad hardware mode , do not need software to scan
 * Parameters :  N/A
 * Response   : 0 : OK
 *              other :ERROR
 *
 */
extern int twl5030_keypad_init_hardmode(void)
{
	int ret = 0;
	u8 ctrl;
	ret = twl5030_i2c_read_u8(TWL4030_CHIP_KEYPAD, &ctrl, KEYPAD_KEYP_CTRL_REG);
	if (!ret) {
		ctrl |= KEYPAD_CTRL_KBD_ON | KEYPAD_CTRL_SOFT_NRST;
		ctrl |= KEYPAD_CTRL_SOFTMODEN;
		ret = twl5030_i2c_write_u8(TWL4030_CHIP_KEYPAD, ctrl, KEYPAD_KEYP_CTRL_REG);
	}
	return ret;
}


/* Description:  scan twl5030 keypad ,find which key or keys is/are pressed
 * Parameters :  exitflag : when two specific keys pressed ,then set it
 *
 * Response   : ret : how many keys are pressed
 *
 *
 */
extern int twl5030_keypad_keys_pressed_hardmode(u8 *exitflag)
{
	int ret = 0;
	u8 i,j,key;
	u8 keynum = 0;
	for (i = 0; i < 8; i++) {
		twl5030_i2c_read_u8(TWL4030_CHIP_KEYPAD, &key, KEYPAD_FULL_CODE_7_0 + i);
		for (j = 0; j < 8; j++) {
			if (key & (1 << j)) {
				ret++;
				printf("key %d pressed\n",i*8+j);
				if ((i==0 && j==0) || (i==0 && j==1))  //when key 1 and key 2 all pressed then tell system will exit the test
					keynum++;

			}
		}
	}
	if (keynum == 2)
		*exitflag = 1;

	return ret;
}


/* Description:  scan twl5030 keypad ,find which key or keys is/are pressed
 * Parameters :  @keys: keys  are pressed
                          @nums: len of array keys
 *
 * Response   : ret : how many keys are pressed
 */
extern int twl5030_keypad_keys_pressed_hardmode_2(char *keys,int nums)
{
	int ret = 0;
	u8 i,j,key;
	memset(keys,-1,nums);
	for (i = 0; i < 8; i++) {
		twl5030_i2c_read_u8(TWL4030_CHIP_KEYPAD, &key, KEYPAD_FULL_CODE_7_0 + i);
		for (j = 0; j < 8; j++)
			if (key & (1 << j))
				keys[ret++] = i*8 + j;
	}

	return ret;
}

/* Description:  init twl5030 keypad hardmode ,and loop to find key press
 * Parameters :  uboot CMD needed.
 *
 * Response   : N/A
 */
extern int  do_pad_test_hardmode(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u8 exitflag = 0;
	twl5030_keypad_init_hardmode();

	for (;;)
	{
		twl5030_keypad_keys_pressed_hardmode(&exitflag);
		if (exitflag == 1)
            return 0;
		//udelay(500*1000);
	}

}

/* Description:  init twl5030 keypad softmode ,and loop to find key press
 * Parameters :  uboot CMD needed.
 *
 * Response   : N/A
 *
 */
int do_pad_test_softmode(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	u8 exitflag = 0;
	twl5030_keypad_init_softmode();

	for (;;)
	{
		twl5030_keypad_keys_pressed_softmode(&exitflag);
		if (exitflag == 1)
                   return 0;
		//udelay(500*1000);
	}

}




U_BOOT_CMD(
	itests,	1,	1,	do_pad_test_softmode,
	"itests  - test keypad in softmode\n",
	"\n    -find which is pressed\n"
);

U_BOOT_CMD(
	itesth,	1,	1,	do_pad_test_hardmode,
	"itesth  - test keypad in hardmode\n",
	"\n    -find which is pressed\n"
);
#endif
