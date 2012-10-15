#ifdef CONFIG_OPTICAL_MOUSE
#include <config.h>
#include <common.h>
#include <asm/arch/cpu.h>
#include <asm/io.h>
#include <asm/arch/mux.h>
#include <asm/arch/sys_proto.h>
#include <asm/arch/sys_info.h>
#include <asm/arch/clocks.h>
#include <asm/arch/mem.h>
#include <i2c.h>
#include <asm/mach-types.h>
#include <twl4030.h>
#include <command.h>
#include <asm/arch/dss.h>


#define VAUX2_DEV_GRP (TWL4030_BASEADD_PM_RECIEVER + 0x1B)
#define VAUX2_DEDICATED (TWL4030_BASEADD_PM_RECIEVER + 0x1E)

#define TWL_VAUX2_1V8   (0x05)
#define TWL_DEV_GROUP_P1  (0x20)

#define abs(x)    ((x)>=0 ?(x) :-(x))

/*******************************************************************************
  ADBS_A320 Register Addresses
*******************************************************************************/
#define A320_PRODUCTID               0x00    // 0x83
#define A320_REVISIONID              0x01    // 0x01
#define A320_MOTION                  0x02
#define A320_DELTAX                  0x03
#define A320_DELTAY                  0x04
#define A320_SQUAL                   0x05
#define A320_SHUTTERUPPER            0x06
#define A320_SHUTTERLOWER            0x07
#define A320_MAXIMUMPIXEL            0x08
#define A320_PIXELSUM                0x09
#define A320_MINIMUMPIXEL            0x0A
#define A320_PIXELGRAB               0x0B
#define A320_CRC0                    0x0C
#define A320_CRC1                    0x0D
#define A320_CRC2                    0x0E
#define A320_CRC3                    0x0F
#define A320_SELFTEST                0x10
#define A320_CONFIGURATIONBITS       0x11
#define A320_LED_CONTROL             0x1A
#define A320_IO_MODE                 0x1C
#define A320_MOTION_CONTROL          0x1D
#define A320_OBSERVATION             0x2E
#define A320_SOFTRESET               0x3A    // 0x5A
#define A320_SHUTTER_MAX_HI          0x3B
#define A320_SHUTTER_MAX_LO          0x3C
#define A320_INVERSEREVISIONID       0x3E    // 0xFE
#define A320_INVERSEPRODUCTID        0x3F    // 0x7C
#define A320_OFN_ENGINE              0x60
#define A320_OFN_RESOLUTION          0x62
#define A320_OFN_SPEED_CONTROL       0x63
#define A320_OFN_SPEED_ST12          0x64
#define A320_OFN_SPEED_ST21          0x65
#define A320_OFN_SPEED_ST23          0x66
#define A320_OFN_SPEED_ST32          0x67
#define A320_OFN_SPEED_ST34          0x68
#define A320_OFN_SPEED_ST43          0x69
#define A320_OFN_SPEED_ST45          0x6A
#define A320_OFN_SPEED_ST54          0x6B
#define A320_OFN_AD_CTRL             0x6D
#define A320_OFN_AD_ATH_HIGH         0x6E
#define A320_OFN_AD_DTH_HIGH         0x6F
#define A320_OFN_AD_ATH_LOW          0x70
#define A320_OFN_AD_DTH_LOW          0x71
#define A320_OFN_QUANTIZE_CTRL       0x73
#define A320_OFN_XYQ_THRESH          0x74
#define A320_OFN_FPD_CTRL            0x75
#define A320_OFN_ORIENTATION_CTRL    0x77

/* Configuration Register Individual Bit Field Settings */
#define A320_MOTION_MOT       		 0x80
#define A320_MOTION_OVF       		 0x10
#define A320_MOTION_GPIO      		 0x01

#define LEVEL_AUTO                  0
#define LEVEL_LOWER                 1
#define LEVEL_LOW                   2
#define LEVEL_MIDDLE                3
#define LEVEL_HIGH                  4
#define LEVEL_HIGHER                5

#define I2C_READ                    0x01
#define I2C_WRITE                   0x00

#define OFN_MOUSE_DEVICE_ID     (0x33)

#define OFN_POWER_MASK          (15)
#define OFN_SHDN_MASK           (12)//(14)  //GPIO_12
#define OFN_NRST_MASK           (13)        //GPIO_13

enum DEVICE_KEY_CODE
{
    DEVICE_KEY_UP = 0,
    DEVICE_KEY_DOWN,
    DEVICE_KEY_LEFT,
    DEVICE_KEY_RIGHT
};

enum OFN_KEY_CODE
{
	OFN_KEY_NULL = 0,
	OFN_KEY_UP,
	OFN_KEY_DOWN,
	OFN_KEY_LEFT,
	OFN_KEY_RIGHT,
	OFN_KEY_ALL
};

enum FPD_STATE
{
	FINGER_OFF = 0,
	FINGER_ON
};


typedef struct __device_id__
{
    u8 product_id;
    u8 revision_id;
    u8 inverse_product_id;
    u8 inverse_revision_id;
}device_id;

typedef struct __ROCKER_THRESHOLD_STRU
{
    int x_set;
    int y_set;
} ROCKER_THRESHOLD;

/*******************************************************************************
  Local Variables
*******************************************************************************/
unsigned char finger_status = 0;
int delta_x = 0, delta_y = 0;
int rocker_x = 0, rocker_y = 0;
unsigned char key_step = 0;
unsigned char last_step = 0, step_cnt = 0;
unsigned char last_dir = 0, dir_cnt = 0;

unsigned char min_step = 4; // 2
unsigned char step_tilt = 12; // 15

/* Sensitivity */
static u8 speed_level = LEVEL_MIDDLE;
static u8 key_event_times = 2;

static ROCKER_THRESHOLD step_thresh[4] = {
                        {12,12},
                        {60,60},
                        {80,80},
                        {100,100}};

static __inline__ u32 reg32_in(u32 addr)
{
    return  __raw_readl(addr);
}

static __inline__ u32 reg32_out(u32 addr, u32 val)
{
    __raw_writel(val, addr);
    return val;
}

/* Functions to read and write from ofn mouse */
static inline int ofn_i2c_write_u8(u8 reg, u8 val)
{
	return i2c_write(OFN_MOUSE_DEVICE_ID, reg, 1, &val, 1);
}

static inline int ofn_i2c_read_u8(u8 reg, u8 *val)
{
	return i2c_read(OFN_MOUSE_DEVICE_ID, reg, 1, val, 1);
}

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

static u8 ofn_mouse_selftest(void)
{
    u8 crc[4] = {0};
    u8 orient, i;

    ofn_i2c_read_u8(A320_OFN_ORIENTATION_CTRL, &orient);
    //printf("orient: 0x%02X\n", orient);

    // soft reset
    ofn_i2c_write_u8(A320_SOFTRESET, 0x5A);
    udelay(5 * 1000);

    ofn_i2c_write_u8(A320_OFN_ENGINE, 0xF6);
    ofn_i2c_write_u8(A320_OFN_QUANTIZE_CTRL, 0xAA);
    ofn_i2c_write_u8(A320_OFN_SPEED_CONTROL, 0xC4);

    // enable system self test
    ofn_i2c_write_u8(A320_SELFTEST, 0x01);

    // wait more than 250ms
    udelay(1000 * 1000);

    ofn_i2c_read_u8(A320_CRC0, &crc[0]);
    ofn_i2c_read_u8(A320_CRC1, &crc[1]);
    ofn_i2c_read_u8(A320_CRC2, &crc[2]);
    ofn_i2c_read_u8(A320_CRC3, &crc[3]);

    if(orient & 0x01)
    {
        if ((crc[0]!=0x36) || (crc[1]!=0x72) || (crc[2]!=0x7F) || (crc[3]!=0xD6))
		{
			printf("OFN Self Test Error\n");

            // output value
            for(i = 0; i < 4; i++)
            {
                printf("crc[%d] = 0x%02X\n", i, crc[i]);
            }

			return 1;
		}
    }
    else
    {
        if ((crc[0]!=0x33) || (crc[1]!=0x8E) || (crc[2]!=0x24) || (crc[3]!=0x6C))
		{
			printf("OFN Self Test Error\n");
            // output value
            for(i = 0; i < 4; i++)
            {
                printf("crc[%d] = 0x%02X\n", i, crc[i]);
            }

			return 1;
		}
    }

    printf("ofn mouse selftest pass..\n");
    return 0;
}

static void ofn_mouse_softreset(void)
{
    // soft reset
    ofn_i2c_write_u8(A320_SOFTRESET, 0x5A);
    udelay(5 * 1000);
}

static void ofn_mouse_poweron(void)
{
    uchar buf[1];
    gpio_t *gpio_base = (gpio_t *)OMAP34XX_GPIO1_BASE;

    printf("OFN Mouse Power ON.\n");

    // enable gpio2 bank
    gpio_enable(1);

    // GPIO47 --> POWER
    // GPIO46 --> SHTDOWN   //GPIO_12
    // GPIO45 --> NRST      //GPIO_13

    // set gpio to output mode
    //sr32((u32)&gpio_base->oe, OFN_SHDN_MASK, 1, 0);
    //sr32((u32)&gpio_base->oe, OFN_NRST_MASK, 1, 0);

    /*enable VAUX1 (3.0v) output*/
    buf[0] = TWL_VAUX2_1V8;
    i2c_write(TWL4030_CHIP_PM_RECEIVER,VAUX2_DEDICATED,sizeof(buf),buf,sizeof(buf));

    buf[0] = TWL_DEV_GROUP_P1;
    i2c_write(TWL4030_CHIP_PM_RECEIVER,VAUX2_DEV_GRP,sizeof(buf),buf,sizeof(buf));

#if 0
    // power on
    sr32((u32)&gpio_base->oe, OFN_POWER_MASK, 1, 0);
    sr32((u32)&gpio_base->setdataout, OFN_POWER_MASK, 1, 1);
#endif

    // set shutdown low
    set_gpio_dataout(OFN_SHDN_MASK,0);

    // set NRESET high
    set_gpio_dataout(OFN_NRST_MASK,1);

    // init i2c
    select_bus(2, 0x190);

    // Turn on power

    // Reset
    set_gpio_dataout(OFN_NRST_MASK,0);

    udelay(2000);
    set_gpio_dataout(OFN_NRST_MASK,1);
}

/*******************************************************************************
  Clear DeltaX and DeltaY buffer
*******************************************************************************/
static void ofn_reset_status(void)
{
	delta_x  = 0;
	delta_y  = 0;
	rocker_x = 0;
	rocker_y = 0;
	key_step = 0;
	last_step= 0;
	step_cnt = 0;
	last_dir = 0;
	dir_cnt  = 0;
}


static void ofn_mouse_regs_init(void)
{
    uchar motion, ux, uy;

    // Soft Reset. All settings will revert to default values
    ofn_i2c_write_u8(A320_SOFTRESET, 0x5A);
    udelay(3 * 1000);


    /*******************************************************************
      These registers are used to set several properties of the sensor
    *******************************************************************/
    // OFN_Engine
    ofn_i2c_write_u8(A320_OFN_ENGINE, 0xA4);

    // Resolution 500cpi, Wakeup 500cpi
    ofn_i2c_write_u8(A320_OFN_RESOLUTION, 0x12);

    // SPIntval: 16ms; Enable Low cpi; Disable High cpi
    ofn_i2c_write_u8(A320_OFN_SPEED_CONTROL, 0x0E);

        // Speed Switching Thresholds
    ofn_i2c_write_u8(A320_OFN_SPEED_ST12, 0x08);
    ofn_i2c_write_u8(A320_OFN_SPEED_ST21, 0x06);
    ofn_i2c_write_u8(A320_OFN_SPEED_ST23, 0x40);
    ofn_i2c_write_u8(A320_OFN_SPEED_ST32, 0x08);
    ofn_i2c_write_u8(A320_OFN_SPEED_ST34, 0x48);
    ofn_i2c_write_u8(A320_OFN_SPEED_ST43, 0x0A);
    ofn_i2c_write_u8(A320_OFN_SPEED_ST45, 0x50);
    ofn_i2c_write_u8(A320_OFN_SPEED_ST54, 0x48);

    // Assert / De-assert Thresholds
    ofn_i2c_write_u8(A320_OFN_AD_CTRL, 0xC4);
    ofn_i2c_write_u8(A320_OFN_AD_ATH_HIGH, 0x34);
    ofn_i2c_write_u8(A320_OFN_AD_DTH_HIGH, 0x40);//0x3C
    ofn_i2c_write_u8(A320_OFN_AD_ATH_LOW, 0x34);//0x18
    ofn_i2c_write_u8(A320_OFN_AD_DTH_LOW, 0x40);//0x20

    // Finger Presence Detect
    ofn_i2c_write_u8(A320_OFN_FPD_CTRL, 0x50);

    // XY Quantization
    ofn_i2c_write_u8(A320_OFN_QUANTIZE_CTRL, 0x99);
    ofn_i2c_write_u8(A320_OFN_XYQ_THRESH, 0x02);

    // if use burst mode, define MOTION_BURST
#ifdef MOTION_BURST
    ofn_i2c_write_u8(A320_IO_MODE, 0x10);
#endif

    // Read from registers 0x02, 0x03 and 0x04
    ofn_i2c_write_u8(A320_MOTION, &motion);
    ofn_i2c_write_u8(A320_DELTAX, &ux);
    ofn_i2c_write_u8(A320_DELTAY, &uy);

    // Set LED drive current to 13mA
    ofn_i2c_write_u8(A320_LED_CONTROL, 0x00);

}

/*******************************************************************************
  Set sensitivity in rocker mode
*******************************************************************************/
static ofn_set_sensitivity(uchar level)
{
	ofn_reset_status();

	speed_level = level;

	switch (speed_level)
	{
	case 0: // auto
		step_thresh[0].x_set = 14;
		step_thresh[0].y_set = 14;
		step_thresh[1].x_set = 40;
		step_thresh[1].y_set = 40;
		step_thresh[2].x_set = 40;
		step_thresh[2].y_set = 40;
		step_thresh[3].x_set = 40;
		step_thresh[3].y_set = 40;
		key_event_times      = 2;

		ofn_i2c_write_u8(A320_OFN_ENGINE, 0xE4);
		ofn_i2c_write_u8(A320_OFN_RESOLUTION, 0x12);
		ofn_i2c_write_u8(A320_OFN_SPEED_CONTROL, 0x0D);
		break;

	case 1: // lower
		step_thresh[0].x_set = 22;
		step_thresh[0].y_set = 22;
		step_thresh[1].x_set = 180;
		step_thresh[1].y_set = 180;
		step_thresh[2].x_set = 200;
		step_thresh[2].y_set = 200;
		step_thresh[3].x_set = 220;
		step_thresh[3].y_set = 220;
		key_event_times      = 2;

		ofn_i2c_write_u8(A320_OFN_ENGINE, 0xA4);
		ofn_i2c_write_u8(A320_OFN_RESOLUTION, 0x12);
		break;

	case 2: // low
		step_thresh[0].x_set = 20;
		step_thresh[0].y_set = 20;
		step_thresh[1].x_set = 80;
		step_thresh[1].y_set = 80;
		step_thresh[2].x_set = 120;
		step_thresh[2].y_set = 120;
		step_thresh[3].x_set = 140;
		step_thresh[3].y_set = 140;
		key_event_times      = 2;

		ofn_i2c_write_u8(A320_OFN_ENGINE, 0xA4);
		ofn_i2c_write_u8(A320_OFN_RESOLUTION, 0x12);
		break;

	case 3: // middle
		step_thresh[0].x_set = 18;
		step_thresh[0].y_set = 18;
		step_thresh[1].x_set = 60;
		step_thresh[1].y_set = 60;
		step_thresh[2].x_set = 80;
		step_thresh[2].y_set = 80;
		step_thresh[3].x_set = 100;
		step_thresh[3].y_set = 100;
		key_event_times      = 2;

		ofn_i2c_write_u8(A320_OFN_ENGINE, 0xA4);
		ofn_i2c_write_u8(A320_OFN_RESOLUTION, 0x12);
		break;

	case 4: // high
		step_thresh[0].x_set = 15;
		step_thresh[0].y_set = 15;
		step_thresh[1].x_set = 30;
		step_thresh[1].y_set = 30;
		step_thresh[2].x_set = 60;
		step_thresh[2].y_set = 60;
		step_thresh[3].x_set = 80;
		step_thresh[3].y_set = 80;
		key_event_times      = 2;

		ofn_i2c_write_u8(A320_OFN_ENGINE, 0xA4);
		ofn_i2c_write_u8(A320_OFN_RESOLUTION, 0x12);
		break;

	case 5: // higher
		step_thresh[0].x_set = 12;
		step_thresh[0].y_set = 12;
		step_thresh[1].x_set = 22;
		step_thresh[1].y_set = 22;
		step_thresh[2].x_set = 25;
		step_thresh[2].y_set = 25;
		step_thresh[3].x_set = 30;
		step_thresh[3].y_set = 30;
		key_event_times      = 1;

		ofn_i2c_write_u8(A320_OFN_ENGINE, 0xA4);
		ofn_i2c_write_u8(A320_OFN_RESOLUTION, 0x12);
		break;

	default: // auto
		step_thresh[0].x_set = 14;
		step_thresh[0].y_set = 14;
		step_thresh[1].x_set = 40;
		step_thresh[1].y_set = 40;
		step_thresh[2].x_set = 40;
		step_thresh[2].y_set = 40;
		step_thresh[3].x_set = 40;
		step_thresh[3].y_set = 40;
		key_event_times      = 2;

		ofn_i2c_write_u8(A320_OFN_ENGINE, 0xE4);
		ofn_i2c_write_u8(A320_OFN_RESOLUTION, 0x12);
		ofn_i2c_write_u8(A320_OFN_SPEED_CONTROL, 0x0D);

        printf("unkonw config, used default.\n");
		break;
	}
}

/*******************************************************************************
  Check and negate FPD when finger is not present or when there is no motion
*******************************************************************************/
static u8 ofn_check_FPD(void)
{
	unsigned char squal, upper, lower, pix_max, pix_avg, pix_min;
	unsigned short shutter;

	ofn_i2c_read_u8(A320_SQUAL, &squal);
	ofn_i2c_read_u8(A320_SHUTTERUPPER, &upper);
	ofn_i2c_read_u8(A320_SHUTTERLOWER, &lower);
	ofn_i2c_read_u8(A320_MAXIMUMPIXEL, &pix_max);
	ofn_i2c_read_u8(A320_PIXELSUM, &pix_avg);
	ofn_i2c_read_u8(A320_MINIMUMPIXEL, &pix_min);

	shutter = (upper << 8) | lower;
	//printf("shutter: %d, pix: %d, squal: %d\n", shutter, pix_max-pix_min, squal);

	//if ((shutter < 25) || (abs(pix_max - pix_min) > 235) || (squal < 12))
	if ((shutter > 500) || (abs(pix_max - pix_min) > 220) || (squal < 12))
	{
		finger_status = FINGER_OFF;
		return 1;
	}
	return 0;
}

/*******************************************************************************
  Comparison on whether the accumulated motion data exceeds the threshold for
  multiple step rocker mode
*******************************************************************************/
static u8 ofn_toggle_step(int x, int y)
{
	unsigned char cur_dir = OFN_KEY_NULL;

	int abs_x = abs(x);
	int abs_y = abs(y);

	if ((abs_x-abs_y > min_step) && (abs_x*10 > abs_y*step_tilt))
	{
		if (x > 0)
		{
			cur_dir = OFN_KEY_RIGHT;
		}
		else
		{
			cur_dir = OFN_KEY_LEFT;
		}
	}
	else if ((abs_y-abs_x > min_step) && (abs_y*10 > abs_x*step_tilt))
	{
		if (y > 0)
		{
			cur_dir = OFN_KEY_UP;
		}
		else
		{
			cur_dir = OFN_KEY_DOWN;
		}
	}

	return cur_dir;
}
/*******************************************************************************
  Process delta X and delta Y according to the various operation modes
*******************************************************************************/
static u8 ofn_data_process(void)
{
	unsigned char key_code = OFN_KEY_NULL;
	unsigned char cur_dir;
	ROCKER_THRESHOLD rocker_thresh;

	cur_dir = ofn_toggle_step(delta_x, delta_y);
	printf("cur_dir: %d, last_dir: %d\n", cur_dir, last_dir);
	printf("delta_x: %d, delta_y: %d\n", delta_x, delta_y);

	if (cur_dir == OFN_KEY_NULL)
	{
		return key_code;
	}
	else if (cur_dir != last_dir)
	{
		rocker_x = 0;
		rocker_y = 0;
		dir_cnt  = 0;
		last_dir = cur_dir;
	}

	rocker_x += delta_x;
	rocker_y += delta_y;
	printf("rocker_x: %d, rocker_y: %d\n", rocker_x, rocker_y);

	if (cur_dir != last_step)
	{
		rocker_thresh = step_thresh[0];

		if (abs(delta_x) > rocker_thresh.x_set*2 || abs(delta_y) > rocker_thresh.y_set*2)
		{
			printf("derection changed!\n");
			key_code = cur_dir;
			step_cnt = 1;
		}
		else if (abs(rocker_x) > rocker_thresh.x_set || abs(rocker_y) > rocker_thresh.y_set)
		{
			dir_cnt++;
			printf("dir_cnt: %d\n", dir_cnt);

			if (dir_cnt >= key_event_times)//1)//
			{
				printf("derection changed!\n");
				key_code = cur_dir;
				step_cnt = 1;
			}
		}
	}
	else
	{
		rocker_thresh = step_thresh[step_cnt];

		if (abs(rocker_x) > rocker_thresh.x_set || abs(rocker_y) > rocker_thresh.y_set)
		{
			dir_cnt++;
			printf("dir_cnt: %d\n", dir_cnt);

			if (dir_cnt >= key_event_times)
			{
				key_code = cur_dir;

				step_cnt++;
				if (step_cnt > 3)
				{
					step_cnt = 3;
				}
			}
		}
	}
	return key_code;
}

/*******************************************************************************
 Send OFN key code to system
*******************************************************************************/
static void ofn_key_event(void)
{
    unsigned char ofn_key = 0xff;

    switch (key_step)
    {
    case OFN_KEY_UP:
        ofn_key = DEVICE_KEY_UP;
//      kal_set_eg_events(KBD.event, JB_UP, KAL_OR);
        printf("derection: DEVICE_KEY_UP\n");
        break;

    case OFN_KEY_DOWN:
        ofn_key = DEVICE_KEY_DOWN;
//      kal_set_eg_events(KBD.event, JB_DOWN, KAL_OR);
        printf("derection: OFN_KEY_DOWN\n");
        break;

    case OFN_KEY_LEFT:
        ofn_key = DEVICE_KEY_LEFT;
//      kal_set_eg_events(KBD.event, JB_LEFT, KAL_OR);
        printf("derection: DEVICE_KEY_LEFT\n");
        break;

    case OFN_KEY_RIGHT:
        ofn_key = DEVICE_KEY_RIGHT;
        printf("derection: DEVICE_KEY_RIGHT\n");
//      kal_set_eg_events(KBD.event, JB_RIGHT, KAL_OR);
        break;

    default:
        printf("derection: DEVICE_KEY_UNKNOWN\n");
        break;
    }


}


/*******************************************************************************
  This is the main firmware data collecting and processing part
  Central timer, 30ms overwrite speed
*******************************************************************************/
static void ofn_get_motion(void)
{
	// Check if there is a motion occurrence on the sensor
	u8 motion;
	ofn_i2c_read_u8(A320_MOTION, &motion);
//	TRACE("motion: 0x%02x\n", motion);

	// Motion overflow, delta Y and/or X buffer has overflowed since last report
	if (motion & A320_MOTION_OVF)
	{
		printf("motion overflow!\n");

		// Clear the MOT and OVF bits, Delta_X and Delta_Y registers
		ofn_i2c_write_u8(A320_MOTION, 0x00);
	}
	else if (motion & A320_MOTION_MOT)
	{
		unsigned char ux, uy;

		// Read delta_x and delta_y for reporting
		ofn_i2c_read_u8(A320_DELTAX, &ux);
		ofn_i2c_read_u8(A320_DELTAY, &uy);
/*
		if (ofn_check_FPD())
		{
			printf("finger_status: %d\n", finger_status);
            ofn_reset_status();
			ofn_i2c_write_u8(A320_MOTION, 0x00);
			return;
		}
*/
		delta_x = ux;//(char)ux;
		delta_y = uy;//(char)uy;

		// bit7 indicates the moving direction
		if (ux & 0x80)
		{
			 delta_x -= 256;
		}

		if (uy & 0x80)
		{
			if (uy != 0x80)
			{
				delta_y -= 256;
			}
			else
			{
				delta_y = 0;
			}
		}

		if (delta_x || delta_y)
		{
			finger_status = FINGER_ON;

			if ((abs(delta_x) > min_step) || (abs(delta_y) > min_step))
			{
				key_step = ofn_data_process();
				if (key_step)
				{
                    ofn_key_event();
					last_step= key_step;
					rocker_x = 0;
					rocker_y = 0;
					key_step = 0;
					last_dir = 0;
					dir_cnt  = 0;
				}
			}
		}
		ofn_i2c_write_u8(A320_MOTION, 0x00);
	}/*
	else if (finger_status)
	{
		finger_status = motion & A320_MOTION_GPIO;
        ofn_check_FPD();
		if (finger_status == FINGER_OFF)
		{
			printf("finger_status: %d\n", finger_status);
            ofn_reset_status();
		}
	}*/

}


u8 ofn_mouse_id(device_id* ps_id)
{
    if(ps_id)
    {
        ofn_i2c_read_u8(A320_PRODUCTID, &ps_id->product_id);
        ofn_i2c_read_u8(A320_REVISIONID, &ps_id->revision_id);
        ofn_i2c_read_u8(A320_INVERSEPRODUCTID, &ps_id->inverse_product_id);
        ofn_i2c_read_u8(A320_INVERSEREVISIONID, &ps_id->inverse_revision_id);

        if((ps_id->product_id != 0x83)
            || (ps_id->revision_id != 0x01)
            || (ps_id->inverse_product_id != 0x7C)
            || (ps_id->inverse_revision_id != 0xFE))
        {
            printf("OFN mouse ID check bad.\n");

            printf("product_id:0x%02X, revision_id:0x%02X, inv_product_id:0x%02X, inv_revision_id:0x%02X\n",
            ps_id->product_id, ps_id->revision_id,
            ps_id->inverse_product_id, ps_id->inverse_revision_id);

            return 1;
        }
        else
            return 0;
    }
    else
        return 1;
}

u8 ofn_mouse_chip_init(void)
{
    device_id id;
    u8 ret;

    ofn_mouse_poweron();

    if(ofn_mouse_id(&id) || ofn_mouse_selftest())
    {
        printf("ofn_mouse_chip_init error... \n");
        return 1;
    }

    ofn_mouse_regs_init();
    ofn_set_sensitivity(speed_level);

    printf("ofn_mouse_chip_init ok... \n");
    return 0;
}


int do_mousetest ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    device_id id;
    u8 val, val1;

    gpio_t *gpio_base = (gpio_t *)OMAP34XX_GPIO1_BASE;

    if(argc < 1){
        printf("usage:mouse <poweron/reset/id/ioctrl/chipid>\n");
        return -1;
    }

    if(strncmp(argv[1],"poweron",sizeof("poweron") - 1) == 0){
        ofn_mouse_poweron();
        printf("mouse power on...\n");
    }else if(strncmp(argv[1],"id",sizeof("id") - 1) == 0){
        ofn_mouse_id(&id);
    }else if(strncmp(argv[1],"ioctrl",sizeof("ioctrl") - 1) == 0){
        val = (u8)simple_strtoul(argv[2], NULL, 10);
        //val1 = (u8)simple_strtoul(argv[3], NULL, 10);

        // enable gpio2 bank
        gpio_enable(1);

        // GPIO46 --> SHTDOWN
        // GPIO45 --> NRST

        // set gpio to output mode
        //sr32((u32)&gpio_base->oe, OFN_SHDN_MASK, 1, 0);
        //sr32((u32)&gpio_base->oe, OFN_NRST_MASK, 1, 0);

#if 1
        if(1 == val)
        {
           set_gpio_dataout(OFN_SHDN_MASK,1);
            set_gpio_dataout(OFN_NRST_MASK,1);
        }
        else
        {
            set_gpio_dataout(OFN_SHDN_MASK,0);
            set_gpio_dataout(OFN_NRST_MASK,0);

        }
#endif
        printf("set pin to value:%d and actual value:0x%08X\n", val,
            reg32_in(OMAP34XX_GPIO2_BASE + 0x03C));

    }else if(strncmp(argv[1],"chipinit",sizeof("chipinit") - 1) == 0){
        ofn_mouse_chip_init();
    }else if(strncmp(argv[1],"read",sizeof("read") - 1) == 0){
        val = (u8)simple_strtoul(argv[2], NULL, 10);
        val1 = i2c_read(OFN_MOUSE_DEVICE_ID, val, 1, &val, 1);

        printf("read [0x%02x] = 0x%02X\n", val, val1);
    }else if(strncmp(argv[1],"softreset",sizeof("softreset") - 1) == 0){
        ofn_mouse_softreset();
    }else if(strncmp(argv[1],"selftest",sizeof("selftest") - 1) == 0){
        ofn_mouse_selftest();
    }else if(strncmp(argv[1],"run",sizeof("run") - 1) == 0){

        val = (u8)simple_strtoul(argv[2], NULL, 10);

        ofn_mouse_chip_init();

        while(1)
        {
            switch(val)
            {
            case 1:
                ofn_check_FPD();
                return 0;
            case 2:
                ofn_get_motion();
                break;
            default:
                break;
            }

            udelay(1000);
        }
    }
    //ofn_mouse_chip_init();

    return 0;

}


U_BOOT_CMD(
        mouse,	CFG_MAXARGS,	1,	do_mousetest,
        "mouse - test mouse\n",
        "[<option>]\n"
        "    - with <option> argument: mouse <option> \n"
        "<option>:\n"
        "    ---poweron\n"
        "    ---id\n"
        "    ---ioctrl\n"
        "    ---softreset\n"
        "    ---selftest\n"
        "    ---chipinit\n"
        );

#endif
