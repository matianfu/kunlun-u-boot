/*********************************************************************
 * (C) Copyright 2009 viatelecom
 *  version : 1.0
 *  date    : 2009-010-23
 *  author  : daqing li
 *  Description    :  Camera test for apollo platform
 ******************************************************************/

#include <config.h>

#if defined(CONFIG_KUNLUN_CAM) && (defined(CONFIG_LCD_ILI9327) || defined(BOARD_WITH_LCD_SHOW))

#if defined(CONFIG_3630KUNLUN_KL9C) || defined(CONFIG_3630KUNLUN_P2) || defined(CONFIG_3630PUMA_V1)
#define CONFIG_CAM_BF3703 1
#define CONFIG_CAM_HI253 1
#define SENSOR_OUTPUT_VGA 1
#elif defined(CONFIG_3630KUNLUN_WUDANG)
#define CONFIG_CAM_OV5640 1
#define CONFIG_CAM_HI253 1
#define SENSOR_OUTPUT_VGA 1
#else
#define CONFIG_CAM_OV2655 1
#define SENSOR_OUTPUT_SVGA 1
#endif

#define DEBUG
#ifdef DEBUG
#define DEBUG_INFO(fmt,args...)  printf(fmt,##args)
#else
#define DEBUG_INFO(fmt,args...)
#endif

#define USE_RESIZER 1
//#define ROTATION_270_MIRROR 1
#define _MAIN_SENSOR_ 0
#define _SUB_SENSOR_   1

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

#ifdef USE_RESIZER
#include "ispresizer.h"
#endif

#ifdef CONFIG_DRIVER_OMAP34XX_I2C
extern int select_bus(int, int);
#endif
#if defined(CONFIG_CAM_OV5640)
#define OV5640_ADDR 0x3C
#endif

#if defined(CONFIG_CAM_OV2655)
#define OV2655_ADDR 0x30
#endif

#if defined(CONFIG_CAM_BF3703)
#define BF3703_ADDR  0x6E
unsigned short  BF3703_sensor_id;
#endif

#if defined(CONFIG_CAM_HI253)
#define HI253_ADDR 0x20
unsigned int HI253_pv_HI253_exposure_lines=0x0249f0,HI253_cp_HI253_exposure_lines=0;

#endif

extern u16 test_lcd[LCD_HEIGH][LCD_WIDTH];

#define ISP_REVISION 0x480BC000
#define ISP_SYSCONFIG 0x480BC004
#define ISP_SYSSTATUS 0x480BC008
#define ISP_IRQ0ENABLE 0x480BC00C
#define ISP_IRQ0STATUS 0x480bC010
#define ISP_CTRL        0x480BC040

#define CCDC_PCR    0x480bc604
#define CCDC_SYN_MODE   0x480bc608
#define CCDC_PIX_LINES  0x480bC610
#define CCDC_HORZ_INFO  0x480bc614
#define CCDC_VERT_START 0x480bc618
#define CCDC_VERT_LINES 0x480bc61c
#define CCDC_HSIZE_OFF  0x480bc624
#define CCDC_SDR_ADDR   0x480bc62c
#define CCDC_SDOFST     0x480bc628
#define CCDC_VDINT      0x480bc648
#define CCDC_CFG        0x480bc654

#define ISP_IRQ0ENABLE_HS_VS_IRQ         (1 << 31) //1 - Generate interrupt,0-Mask interrupt
#define ISP_IRQ0ENABLE_OCP_ERR_IRQ       (1 << 29)
#define ISP_IRQ0ENABLE_MMU_ERR_IRQ       (1 << 28)
#define ISP_IRQ0ENABLE_OVF_IRQ           (1 << 25) //1 - Generate interrupt,0-Mask interrupt
#define ISP_IRQ0ENABLE_CBUFF_IRQ         (1 << 21)
#define ISP_IRQ0ENABLE_PRV_DONE_IRQ      (1 << 20) //1 - Generate interrupt,0-Mask interrupt
#define ISP_IRQ0ENABLE_RSZ_DONE_IRQ      (1 << 24) //1 - Generate interrupt,0-Mask interrupt
#define ISP_IRQ0ENABLE_H3A_AEW_DONE_IRQ  (1 << 13) //1 - Generate interrupt,0-Mask interrupt
#define ISP_IRQ0ENABLE_CCDCLSC_PERROR_IRQ (1 << 19)
#define ISP_IRQ0ENABLE_CCDCLSC_PREFCOMP_IRQ (1 << 18)
#define ISP_IRQ0ENABLE_H3A_AF_DONE_IRQ   (1 << 12) //1 - Generate interrupt,0-Mask interrupt
#define ISP_IRQ0ENABLE_CCDC_ERR_IRQ      (1 << 11)
#define ISP_IRQ0ENABLE_HIST_DONE_IRQ     (1 << 16) //1 - Generate interrupt,0-Mask interrupt
#define ISP_IRQ0ENABLE_CCDC_VD2_IRQ      (1 << 10) //1 - Generate interrupt,0-Mask interrupt
#define ISP_IRQ0ENABLE_CCDC_VD1_IRQ      (1 << 9)  //1 - Generate interrupt,0-Mask interrupt
#define ISP_IRQ0ENABLE_CCDC_VD0_IRQ      (1 << 8)  //1 - Generate interrupt,0-Mask interrupt
#define ISP_IRQ0ENABLE_CSI2_IRQ          (1 << 0)



#define CCDC_PCR_BUSY   (1 << 1)


#define DSS_RFBI_CMD    0x4805084C
#define DSS_RFBI_PARAM  0x48050850
#define DSS_RFBI_DATA   0x48050854

#if defined(SENSOR_OUTPUT_SVGA) && !defined(USE_RESIZER)
#define CAMERA_BUFFER_ITEMSIZE (800 * 600 * 2)
#else
#define CAMERA_BUFFER_ITEMSIZE (LCD_HEIGH * LCD_WIDTH * 2)
#endif

#define CAMERA_BUFFER_ITEMSIZE_ROUND ((CAMERA_BUFFER_ITEMSIZE % 256) ? \
        (CAMERA_BUFFER_ITEMSIZE + 256 - CAMERA_BUFFER_ITEMSIZE % 256):\
        CAMERA_BUFFER_ITEMSIZE)

#define MAX_BUFFERS 3
#define CAM_BUFFERS_START 0x82000000
#define CAM_BUFFERS_SIZE  (MAX_BUFFERS * CAMERA_BUFFER_ITEMSIZE_ROUND)

        static unsigned int CAMERA_BUFFERS[MAX_BUFFERS] = {
            CAM_BUFFERS_START,
            CAM_BUFFERS_START + CAMERA_BUFFER_ITEMSIZE_ROUND,
            CAM_BUFFERS_START + 2 *CAMERA_BUFFER_ITEMSIZE_ROUND
        };

//static unsigned char IMAGE_SAVE_RGB565[CAMERA_BUFFER_ITEMSIZE];
static unsigned char IMAGE_SAVE_RGB565[CAMERA_BUFFER_ITEMSIZE];

#if (defined(CONFIG_CAM_BF3703) || defined(CONFIG_CAM_HI253)) && !defined(CONFIG_CAM_OV5640)
    struct camreg
    {
        unsigned char addr;
        unsigned char data;
    } ;
#else
    struct camreg
    {
        unsigned short addr;
        unsigned char data;
    } ;
#endif

struct isp_description{
    unsigned int ccdc_in_width;
    unsigned int ccdc_in_heigh;
    unsigned int ccdc_out_width;
    unsigned int ccdc_out_heigh;
    unsigned int resz_in_width;
    unsigned int resz_in_heigh;
    unsigned int resz_out_width;
    unsigned int resz_out_heigh;
}isp_des_obj;

#if defined(CONFIG_CAM_BF3703)
const static struct camreg BF3703_common_svgabase[] = {
    {0x11,0x80},
    {0x12,0x00},
    {0x09,0x00},
    {0x13,0x00},
    {0x01,0x13},
    {0x02,0x21},
    {0x8c,0x02},//01 :devided by 2  02 :devided by 1
    {0x8d,0x64},//32: devided by 2  64 :devided by 1
    {0x87,0x18},
    {0x13,0x07},

    //POLARITY of Signal
    {0x15,0x40},
    {0x3a,0x03},

    //black level ,对上电偏绿有改善,如果需要请使用

    {0x05,0x1f},
    {0x06,0x60},
    {0x14,0x1f},
    {0x27,0x03},
    {0x06,0xe0},


    //lens shading
    {0x35,0x68},
    {0x65,0x68},
    {0x66,0x62},
    {0x36,0x05},
    {0x37,0xf6},
    {0x38,0x46},
    {0x9b,0xf6},
    {0x9c,0x46},
    {0xbc,0x01},
    {0xbd,0xf6},
    {0xbe,0x46},

    //AE
    {0x82,0x14},
    {0x83,0x23},
    {0x9a,0x23},//the same as 0x83
    {0x84,0x1a},
    {0x85,0x20},
    {0x89,0x04},//02 :devided by 2  04 :devided by 1
    {0x8a,0x08},//04: devided by 2  05 :devided by 1
    {0x86,0x28},
    {0x96,0xa6},//AE speed
    {0x97,0x0c},//AE speed
    {0x98,0x18},//AE speed
    //AE target
    {0x24,0x78},
    {0x25,0x88},
    {0x94,0x0a},//INT_OPEN
    {0x80,0x55},

    //denoise
    {0x70,0x6f},//denoise
    {0x72,0x4f},//denoise
    {0x73,0x2f},//denoise
    {0x74,0x27},//denoise
    {0x7a,0x4e},//denoise in  low light,0x8e\0x4e\0x0e
    {0x7b,0x28},//the same as 0x86

    //black level
    {0X1F,0x20},//G target
    {0X22,0x20},//R target
    {0X26,0x20},//B target
    //模拟部分参数
    {0X16,0x00},//如果觉得黑色物体不够黑，有点偏红，将0x16写为0x03会有点改善
    {0xbb,0x20},  // deglitch  对xclk整形
    {0xeb,0x30},
    {0xf5,0x21},
    {0xe1,0x3c},
    {0xbb,0x20},
    {0X2f,0X66},
    {0x06,0xe0},

    //anti black sun spot
    {0x61,0xd3},//0x61[3]=0 black sun disable
    {0x79,0x48},//0x79[7]=0 black sun disable

    //Gamma

    {0x3b,0x60},//auto gamma offset adjust in  low light
    {0x3c,0x20},//auto gamma offset adjust in  low light
    {0x56,0x40},
    {0x39,0x80},
    //gamma1
    {0x3f,0xb0},
    {0X40,0X88},
    {0X41,0X74},
    {0X42,0X5E},
    {0X43,0X4c},
    {0X44,0X44},
    {0X45,0X3E},
    {0X46,0X39},
    {0X47,0X35},
    {0X48,0X31},
    {0X49,0X2E},
    {0X4b,0X2B},
    {0X4c,0X29},
    {0X4e,0X25},
    {0X4f,0X22},
    {0X50,0X1F},

    /*gamma2  过曝过度好，高亮度
      {0x3f,0xb0},
      {0X40,0X9b},
      {0X41,0X88},
      {0X42,0X6e},
      {0X43,0X59},
      {0X44,0X4d},
      {0X45,0X45},
      {0X46,0X3e},
      {0X47,0X39},
      {0X48,0X35},
      {0X49,0X31},
      {0X4b,0X2e},
      {0X4c,0X2b},
      {0X4e,0X26},
      {0X4f,0X23},
      {0X50,0X1F},
      */
    /*//gamma3 清晰亮丽 灰阶分布好
      {0X3f,0Xb0},
      {0X40,0X60},
      {0X41,0X60},
      {0X42,0X66},
      {0X43,0X57},
      {0X44,0X4c},
      {0X45,0X43},
      {0X46,0X3c},
      {0X47,0X37},
      {0X48,0X33},
      {0X49,0X2f},
      {0X4b,0X2c},
      {0X4c,0X29},
      {0X4e,0X25},
      {0X4f,0X22},
      {0X50,0X20},

    //gamma 4	low noise
    {0X3f,0Xa8},
    {0X40,0X48},
    {0X41,0X54},
    {0X42,0X4E},
    {0X43,0X44},
    {0X44,0X3E},
    {0X45,0X39},
    {0X46,0X34},
    {0X47,0X30},
    {0X48,0X2D},
    {0X49,0X2A},
    {0X4b,0X28},
    {0X4c,0X26},
    {0X4e,0X22},
    {0X4f,0X20},
    {0X50,0X1E},
    */

    //color matrix
    {0x51,0x08},
    {0x52,0x3a},
    {0x53,0x32},
    {0x54,0x12},
    {0x57,0x7f},
    {0x58,0x6d},
    {0x59,0x50},
    {0x5a,0x5d},
    {0x5b,0x0d},
    {0x5D,0x95},
    {0x5C,0x0e},

    /*

    //
    {0x51,0x00},
    {0x52,0x15},
    {0x53,0x15},
    {0x54,0x12},
    {0x57,0x7d},
    {0x58,0x6a},
    {0x59,0x5c},
    {0x5a,0x87},
    {0x5b,0x2b},
    {0x5D,0x95},
    {0x5C,0x0e},
    //

    //艳丽
    {0x51,0x0d},
    {0x52,0x2b},
    {0x53,0x1e},
    {0x54,0x15},
    {0x57,0x92},
    {0x58,0x7d},
    {0x59,0x5f},
    {0x5a,0x74},
    {0x5b,0x15},
    {0x5c,0x0e},
    {0x5d,0x95},//0x5c[3:0] low light color coefficient，smaller ,lower noise

    //适中
    {0x51,0x08},
    {0x52,0x0E},
    {0x53,0x06},
    {0x54,0x12},
    {0x57,0x82},
    {0x58,0x70},
    {0x59,0x5C},
    {0x5a,0x77},
    {0x5b,0x1B},
    {0x5c,0x0e},//0x5c[3:0] low light color coefficient，smaller ,lower noise
    {0x5d,0x95},


    //color 淡
    {0x51,0x03},
    {0x52,0x0d},
    {0x53,0x0b},
    {0x54,0x14},
    {0x57,0x59},
    {0x58,0x45},
    {0x59,0x41},
    {0x5a,0x5f},
    {0x5b,0x1e},
    {0x5c,0x0e},//0x5c[3:0] low light color coefficient，smaller ,lower noise
    {0x5d,0x95},
    */

    {0x60,0x20},//color open in low light
    //AWB
    {0x6a,0x81},
    {0x23,0x77},//Green gain
    {0xa0,0x03},

    {0xa1,0X31},
    {0xa2,0X0e},
    {0xa3,0X26},
    {0xa4,0X0d},
    {0xa5,0x26},
    {0xa6,0x06},
    {0xa7,0x80},//BLUE Target
    {0xa8,0x7c},//RED Target
    {0xa9,0x28},
    {0xaa,0x28},
    {0xab,0x28},
    {0xac,0x3c},
    {0xad,0xf0},
    {0xc8,0x18},
    {0xc9,0x20},
    {0xca,0x17},
    {0xcb,0x1f},
    {0xaf,0x00},
    {0xc5,0x18},
    {0xc6,0x00},
    {0xc7,0x20},
    {0xae,0x83},
    {0xcc,0x3c},
    {0xcd,0x90},
    {0xee,0x4c},// P_TH

    // color saturation
    {0xb0,0xd0},
    {0xb1,0xc0},
    {0xb2,0xa8},
    {0xb3,0x8a},

    //anti webcamera banding
    {0x9d,0x4c},

    //switch direction
    {0x1e,0x00},//00:normal  10:IMAGE_V_MIRROR   20:IMAGE_H_MIRROR  30:IMAGE_HV_MIRROR
    {0xFF,0xFF},
    {0xFF,0xFF}
};

/*@return 0:success,!0:fail*/
void BF3703_write_cmos_sensor( unsigned char addr,  unsigned char para)
{
    int dwret = 0;
    unsigned char udata[1];
    udata[0] = para;

    //  DEBUG_INFO("sucess :%d in writing to %x addr\r\n",dwret,addr);
    dwret = i2c_write(BF3703_ADDR,addr,1,udata,1);
    //  DEBUG_INFO("sucess :%d in writing to %x addr\r\n",dwret,addr);
    if (dwret != 0)
    {
        DEBUG_INFO("Error:%d in writing to %x addr\r\n",dwret,addr);
    }

    // return dwret;
}
void BF3703_set_dummy(unsigned short int pixels, unsigned short int lines)
{
    BF3703_write_cmos_sensor(0x2A,((pixels&0xF00)>>4));
    BF3703_write_cmos_sensor(0x2B,(pixels&0xFF));
    BF3703_write_cmos_sensor(0x92,(lines&0xFF));
    BF3703_write_cmos_sensor(0x93,((lines&0xFF00)>>8));
}	/* set_BF3703_dummy */



int  bf3703_write_reg(struct camreg *preg)
{
    unsigned char udata[1];
    int dwret = 0;
    unsigned char address = 0;

    udata[0] = preg->data;
    address =preg->addr;

    dwret = i2c_write(BF3703_ADDR,address,1,udata,1);

    if (dwret != 0)
    {
        DEBUG_INFO("Error:%d in writing to %x addr\r\n",dwret,preg->addr);
    }

    return dwret;
}
int bf3703_read_reg(struct camreg *preg)
{
    unsigned char udata[1];
    unsigned short address = preg->addr;

    if(i2c_read(BF3703_ADDR,address,1, udata, 1) != 0){
        DEBUG_INFO("Error:%x read from %x addr\r\n",BF3703_ADDR,address);
        return -1;
    }

    preg->data = udata[0];
    return 0;
}

static void bf3703_write_seq(struct camreg *pStream)
{
    struct camreg *pCamReg=pStream;

    while (pCamReg->addr != 0xff)
    {
        bf3703_write_reg( pCamReg );
        pCamReg++;
    }
    // TODO - never returns false!
    return;
}

#endif //end of CONFIG_CAM_BF3703

#if defined(CONFIG_CAM_HI253)

int  HI253_write_reg(struct camreg *preg)
{
    unsigned char udata[1];
    int dwret = 0;
    unsigned char address = 0;

    udata[0] = preg->data;
    address =preg->addr;

    dwret = i2c_write(HI253_ADDR,address,1,udata,1);

    if (dwret != 0)
    {
        DEBUG_INFO("Error:%d in writing to %x addr\r\n",dwret,preg->addr);
    }

    return dwret;
}
int HI253_read_reg(struct camreg *preg)
{
    unsigned char udata[1];
    unsigned short address = preg->addr;

    if(i2c_read(HI253_ADDR,address,1, udata, 1) != 0){
         DEBUG_INFO("Error:%x read from %x addr\r\n",HI253_ADDR,address);
         return -1;
    }

    preg->data = udata[0];
    return 0;
}

const static struct camreg HI253_yuv_sensor_initial_setting_info[] = {
    /////// Start Sleep ///////
    {0x01, 0xf9}, //sleep on
    {0x08, 0x0f}, //Hi-Z on
    {0x01, 0xf8}, //sleep off

    {0x03, 0x00}, // Dummy 750us START
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00}, // Dummy 750us END

    {0x0e, 0x03}, //PLL On
    {0x0e, 0x73}, //PLLx2

    {0x03, 0x00}, // Dummy 750us START
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00}, // Dummy 750us END

    {0x0e, 0x00}, //PLL off
    {0x01, 0xf1}, //sleep on
    {0x08, 0x00}, //Hi-Z off

    {0x01, 0xf3},
    {0x01, 0xf1},

    // PAGE 20
    {0x03, 0x20}, //page 20
    {0x10, 0x1c}, //ae off

    // PAGE 22
    {0x03, 0x22}, //page 22
    {0x10, 0x69}, //awb off


    //Initial Start
    /////// PAGE 0 START ///////
    {0x03, 0x00},
    {0x10, 0x11}, // Sub1/2_Preview2 Mode_H binning
    {0x11, 0x90},
    {0x12, 0x05},

    {0x0b, 0xaa}, // ESD Check Register
    {0x0c, 0xaa}, // ESD Check Register
    {0x0d, 0xaa}, // ESD Check Register

    {0x20, 0x00}, // Windowing start point Y
    {0x21, 0x04},
    {0x22, 0x00}, // Windowing start point X
    {0x23, 0x07},

    {0x24, 0x04},
    {0x25, 0xb0},
    {0x26, 0x06},
    {0x27, 0x40}, // WINROW END

    {0x40, 0x01}, //Hblank 408
    {0x41, 0x98},
    {0x42, 0x00}, //Vblank 20
    {0x43, 0x14},

    {0x45, 0x04},
    {0x46, 0x18},
    {0x47, 0xd8},

    //BLC
    {0x80, 0x2e},
    {0x81, 0x7e},
    {0x82, 0x90},
    {0x83, 0x00},
    {0x84, 0x0c},
    {0x85, 0x00},
    {0x90, 0x0a}, //BLC_TIME_TH_ON
    {0x91, 0x0a}, //BLC_TIME_TH_OFF
    {0x92, 0xd8}, //BLC_AG_TH_ON
    {0x93, 0xd0}, //BLC_AG_TH_OFF
    {0x94, 0x75},
    {0x95, 0x70},
    {0x96, 0xdc},
    {0x97, 0xfe},
    {0x98, 0x38},

    //OutDoor  BLC
    {0x99, 0x43},
    {0x9a, 0x43},
    {0x9b, 0x43},
    {0x9c, 0x43},

    //Dark BLC
    {0xa0, 0x00},
    {0xa2, 0x00},
    {0xa4, 0x00},
    {0xa6, 0x00},

    //Normal BLC
    {0xa8, 0x43},
    {0xaa, 0x43},
    {0xac, 0x43},
    {0xae, 0x43},

    /////// PAGE 2 START ///////
    {0x03, 0x02},
    {0x12, 0x03},
    {0x13, 0x03},
    {0x16, 0x00},
    {0x17, 0x8C},
    {0x18, 0x4c}, //Double_AG off
    {0x19, 0x00},
    {0x1a, 0x39}, //ADC400->560
    {0x1c, 0x09},
    {0x1d, 0x40},
    {0x1e, 0x30},
    {0x1f, 0x10},

    {0x20, 0x77},
    {0x21, 0xde},
    {0x22, 0xa7},
    {0x23, 0x30}, //CLAMP
    {0x27, 0x3c},
    {0x2b, 0x80},
    {0x2e, 0x11},
    {0x2f, 0xa1},
    {0x30, 0x05}, //For Hi-253 never no change 0x05

    {0x50, 0x20},
    {0x52, 0x01},
    {0x55, 0x1c},
    {0x56, 0x11},
    {0x5d, 0xa2},
    {0x5e, 0x5a},

    {0x60, 0x87},
    {0x61, 0x99},
    {0x62, 0x88},
    {0x63, 0x97},
    {0x64, 0x88},
    {0x65, 0x97},

    {0x67, 0x0c},
    {0x68, 0x0c},
    {0x69, 0x0c},

    {0x72, 0x89},
    {0x73, 0x96},
    {0x74, 0x89},
    {0x75, 0x96},
    {0x76, 0x89},
    {0x77, 0x96},

    {0x7c, 0x85},
    {0x7d, 0xaf},
    {0x80, 0x01},
    {0x81, 0x7f},
    {0x82, 0x13},

    {0x83, 0x24},
    {0x84, 0x7d},
    {0x85, 0x81},
    {0x86, 0x7d},
    {0x87, 0x81},

    {0x92, 0x48},
    {0x93, 0x54},
    {0x94, 0x7d},
    {0x95, 0x81},
    {0x96, 0x7d},
    {0x97, 0x81},

    {0xa0, 0x02},
    {0xa1, 0x7b},
    {0xa2, 0x02},
    {0xa3, 0x7b},
    {0xa4, 0x7b},
    {0xa5, 0x02},
    {0xa6, 0x7b},
    {0xa7, 0x02},

    {0xa8, 0x85},
    {0xa9, 0x8c},
    {0xaa, 0x85},
    {0xab, 0x8c},
    {0xac, 0x10},
    {0xad, 0x16},
    {0xae, 0x10},
    {0xaf, 0x16},

    {0xb0, 0x99},
    {0xb1, 0xa3},
    {0xb2, 0xa4},
    {0xb3, 0xae},
    {0xb4, 0x9b},
    {0xb5, 0xa2},
    {0xb6, 0xa6},
    {0xb7, 0xac},
    {0xb8, 0x9b},
    {0xb9, 0x9f},
    {0xba, 0xa6},
    {0xbb, 0xaa},
    {0xbc, 0x9b},
    {0xbd, 0x9f},
    {0xbe, 0xa6},
    {0xbf, 0xaa},

    {0xc4, 0x2c},
    {0xc5, 0x43},
    {0xc6, 0x63},
    {0xc7, 0x79},

    {0xc8, 0x2d},
    {0xc9, 0x42},
    {0xca, 0x2d},
    {0xcb, 0x42},
    {0xcc, 0x64},
    {0xcd, 0x78},
    {0xce, 0x64},
    {0xcf, 0x78},

    {0xd0, 0x0a},
    {0xd1, 0x09},
    {0xd4, 0x0a}, //DCDC_TIME_TH_ON
    {0xd5, 0x0a}, //DCDC_TIME_TH_OFF
    {0xd6, 0xd8}, //DCDC_AG_TH_ON
    {0xd7, 0xd0}, //DCDC_AG_TH_OFF
    {0xe0, 0xc4},
    {0xe1, 0xc4},
    {0xe2, 0xc4},
    {0xe3, 0xc4},
    {0xe4, 0x00},
    {0xe8, 0x80},
    {0xe9, 0x40},
    {0xea, 0x7f},

    /////// PAGE 3 ///////
    {0x03, 0x03},
    {0x10, 0x10},

    /////// PAGE 10 START ///////
    {0x03, 0x10},
    {0x10, 0x03}, // CrYCbY // For Demoset 0x03
    //{0x10, 0x40}, // CrYCbY // For Demoset 0x03
    {0x12, 0x30},
    {0x13, 0x0a}, // contrast on
    {0x20, 0x00},

    {0x30, 0x00},
    {0x31, 0x00},
    {0x32, 0x00},
    {0x33, 0x00},

    {0x34, 0x30},
    {0x35, 0x00},
    {0x36, 0x00},
    {0x38, 0x00},
    {0x3e, 0x58},
    {0x3f, 0x00},

    {0x40, 0x80}, // YOFS
    {0x41, 0x00}, // DYOFS
    {0x48, 0x80}, // Contrast

    {0x60, 0x67},
    {0x61, 0x7c}, //7e //8e //88 //80
    {0x62, 0x7c}, //7e //8e //88 //80
    {0x63, 0x50}, //Double_AG 50->30
    {0x64, 0x41},

    {0x66, 0x42},
    {0x67, 0x20},

    {0x6a, 0x80}, //8a
    {0x6b, 0x84}, //74
    {0x6c, 0x80}, //7e //7a
    {0x6d, 0x80}, //8e

    //Don't touch//////////////////////////
    //{0x72, 0x84},
    //{0x76, 0x19},
    //{0x73, 0x70},
    //{0x74, 0x68},
    //{0x75, 0x60}, // white protection ON
    //{0x77, 0x0e}, //08 //0a
    //{0x78, 0x2a}, //20
    //{0x79, 0x08},
    ////////////////////////////////////////

    /////// PAGE 11 START ///////
    {0x03, 0x11},
    {0x10, 0x7f},
    {0x11, 0x40},
    {0x12, 0x0a}, // Blue Max-Filter Delete
    {0x13, 0xbb},

    {0x26, 0x31}, // Double_AG 31->20
    {0x27, 0x34}, // Double_AG 34->22
    {0x28, 0x0f},
    {0x29, 0x10},
    {0x2b, 0x30},
    {0x2c, 0x32},

    //Out2 D-LPF th
    {0x30, 0x70},
    {0x31, 0x10},
    {0x32, 0x58},
    {0x33, 0x09},
    {0x34, 0x06},
    {0x35, 0x03},

    //Out1 D-LPF th
    {0x36, 0x70},
    {0x37, 0x18},
    {0x38, 0x58},
    {0x39, 0x09},
    {0x3a, 0x06},
    {0x3b, 0x03},

    //Indoor D-LPF th
    {0x3c, 0x80},
    {0x3d, 0x18},
    {0x3e, 0xa0}, //80
    {0x3f, 0x0c},
    {0x40, 0x09},
    {0x41, 0x06},

    {0x42, 0x80},
    {0x43, 0x18},
    {0x44, 0xa0}, //80
    {0x45, 0x12},
    {0x46, 0x10},
    {0x47, 0x10},

    {0x48, 0x90},
    {0x49, 0x40},
    {0x4a, 0x80},
    {0x4b, 0x13},
    {0x4c, 0x10},
    {0x4d, 0x11},

    {0x4e, 0x80},
    {0x4f, 0x30},
    {0x50, 0x80},
    {0x51, 0x13},
    {0x52, 0x10},
    {0x53, 0x13},

    {0x54, 0x11},
    {0x55, 0x17},
    {0x56, 0x20},
    {0x57, 0x01},
    {0x58, 0x00},
    {0x59, 0x00},

    {0x5a, 0x1f}, //18
    {0x5b, 0x00},
    {0x5c, 0x00},

    {0x60, 0x3f},
    {0x62, 0x60},
    {0x70, 0x06},

    /////// PAGE 12 START ///////
    {0x03, 0x12},
    {0x20, 0x0f},
    {0x21, 0x0f},

    {0x25, 0x00}, //0x30

    {0x28, 0x00},
    {0x29, 0x00},
    {0x2a, 0x00},

    {0x30, 0x50},
    {0x31, 0x18},
    {0x32, 0x32},
    {0x33, 0x40},
    {0x34, 0x50},
    {0x35, 0x70},
    {0x36, 0xa0},

    //Out2 th
    {0x40, 0xa0},
    {0x41, 0x40},
    {0x42, 0xa0},
    {0x43, 0x90},
    {0x44, 0x90},
    {0x45, 0x80},

    //Out1 th
    {0x46, 0xb0},
    {0x47, 0x55},
    {0x48, 0xa0},
    {0x49, 0x90},
    {0x4a, 0x90},
    {0x4b, 0x80},

    //Indoor th
    {0x4c, 0xb0},
    {0x4d, 0x40},
    {0x4e, 0x90},
    {0x4f, 0x90},
    {0x50, 0xa0},
    {0x51, 0x80},

    //Dark1 th
    {0x52, 0xb0},
    {0x53, 0x60},
    {0x54, 0xc0},
    {0x55, 0xc0},
    {0x56, 0xc0},
    {0x57, 0x80},

    //Dark2 th
    {0x58, 0x90},
    {0x59, 0x40},
    {0x5a, 0xd0},
    {0x5b, 0xd0},
    {0x5c, 0xe0},
    {0x5d, 0x80},

    //Dark3 th
    {0x5e, 0x88},
    {0x5f, 0x40},
    {0x60, 0xe0},
    {0x61, 0xe0},
    {0x62, 0xe0},
    {0x63, 0x80},

    {0x70, 0x15},
    {0x71, 0x01}, //Don't Touch register

    {0x72, 0x18},
    {0x73, 0x01}, //Don't Touch register

    {0x74, 0x25},
    {0x75, 0x15},

    {0x80, 0x20},
    {0x81, 0x40},
    {0x82, 0x65},
    {0x85, 0x1a},
    {0x88, 0x00},
    {0x89, 0x00},
    {0x90, 0x5d}, //For Preview

    //Dont Touch register
    {0xD0, 0x0c},
    {0xD1, 0x80},
    {0xD2, 0x67},
    {0xD3, 0x00},
    {0xD4, 0x00},
    {0xD5, 0x02},
    {0xD6, 0xff},
    {0xD7, 0x18},
    //End
    {0x3b, 0x06},
    {0x3c, 0x06},

    {0xc5, 0x00},//55->48
    {0xc6, 0x00},//48->40

    /////// PAGE 13 START ///////
    {0x03, 0x13},
    //Edge
    {0x10, 0xcb},
    {0x11, 0x7b},
    {0x12, 0x07},
    {0x14, 0x00},

    {0x20, 0x15},
    {0x21, 0x13},
    {0x22, 0x33},
    {0x23, 0x05},
    {0x24, 0x09},

    {0x25, 0x0a},

    {0x26, 0x18},
    {0x27, 0x30},
    {0x29, 0x12},
    {0x2a, 0x50},

    //Low clip th
    {0x2b, 0x00}, //Out2 02
    {0x2c, 0x00}, //Out1 02 //01
    {0x25, 0x06},
    {0x2d, 0x0c},
    {0x2e, 0x12},
    {0x2f, 0x12},

    //Out2 Edge
    {0x50, 0x18}, //0x10 //0x16
    {0x51, 0x1c}, //0x14 //0x1a
    {0x52, 0x1a}, //0x12 //0x18
    {0x53, 0x14}, //0x0c //0x12
    {0x54, 0x17}, //0x0f //0x15
    {0x55, 0x14}, //0x0c //0x12

    //Out1 Edge          //Edge
    {0x56, 0x18}, //0x10 //0x16
    {0x57, 0x1c}, //0x13 //0x1a
    {0x58, 0x1a}, //0x12 //0x18
    {0x59, 0x14}, //0x0c //0x12
    {0x5a, 0x17}, //0x0f //0x15
    {0x5b, 0x14}, //0x0c //0x12

    //Indoor Edge
    {0x5c, 0x0a},
    {0x5d, 0x0b},
    {0x5e, 0x0a},
    {0x5f, 0x08},
    {0x60, 0x09},
    {0x61, 0x08},

    //Dark1 Edge
    {0x62, 0x08},
    {0x63, 0x08},
    {0x64, 0x08},
    {0x65, 0x06},
    {0x66, 0x06},
    {0x67, 0x06},

    //Dark2 Edge
    {0x68, 0x07},
    {0x69, 0x07},
    {0x6a, 0x07},
    {0x6b, 0x05},
    {0x6c, 0x05},
    {0x6d, 0x05},

    //Dark3 Edge
    {0x6e, 0x07},
    {0x6f, 0x07},
    {0x70, 0x07},
    {0x71, 0x05},
    {0x72, 0x05},
    {0x73, 0x05},

    //2DY
    {0x80, 0xfd},
    {0x81, 0x1f},
    {0x82, 0x05},
    {0x83, 0x31},

    {0x90, 0x05},
    {0x91, 0x05},
    {0x92, 0x33},
    {0x93, 0x30},
    {0x94, 0x03},
    {0x95, 0x14},
    {0x97, 0x20},
    {0x99, 0x20},

    {0xa0, 0x01},
    {0xa1, 0x02},
    {0xa2, 0x01},
    {0xa3, 0x02},
    {0xa4, 0x05},
    {0xa5, 0x05},
    {0xa6, 0x07},
    {0xa7, 0x08},
    {0xa8, 0x07},
    {0xa9, 0x08},
    {0xaa, 0x07},
    {0xab, 0x08},

    //Out2
    {0xb0, 0x22},
    {0xb1, 0x2a},
    {0xb2, 0x28},
    {0xb3, 0x22},
    {0xb4, 0x2a},
    {0xb5, 0x28},

    //Out1
    {0xb6, 0x22},
    {0xb7, 0x2a},
    {0xb8, 0x28},
    {0xb9, 0x22},
    {0xba, 0x2a},
    {0xbb, 0x28},

    //Indoor
    {0xbc, 0x25},
    {0xbd, 0x2a},
    {0xbe, 0x27},
    {0xbf, 0x25},
    {0xc0, 0x2a},
    {0xc1, 0x27},

    //Dark1
    {0xc2, 0x1e},
    {0xc3, 0x24},
    {0xc4, 0x20},
    {0xc5, 0x1e},
    {0xc6, 0x24},
    {0xc7, 0x20},

    //Dark2
    {0xc8, 0x18},
    {0xc9, 0x20},
    {0xca, 0x1e},
    {0xcb, 0x18},
    {0xcc, 0x20},
    {0xcd, 0x1e},

    //Dark3
    {0xce, 0x18},
    {0xcf, 0x20},
    {0xd0, 0x1e},
    {0xd1, 0x18},
    {0xd2, 0x20},
    {0xd3, 0x1e},

    /////// PAGE 14 START ///////
    {0x03, 0x14},
    {0x10, 0x11},

    {0x14, 0x80}, // GX
    {0x15, 0x80}, // GY
    {0x16, 0x80}, // RX
    {0x17, 0x80}, // RY
    {0x18, 0x80}, // BX
    {0x19, 0x80}, // BY

    {0x20, 0x60}, //X 60 //a0
    {0x21, 0x80}, //Y

    {0x22, 0x80},
    {0x23, 0x80},
    {0x24, 0x80},

    {0x30, 0xc8},
    {0x31, 0x2b},
    {0x32, 0x00},
    {0x33, 0x00},
    {0x34, 0x90},

    {0x40, 0x48}, //31
    {0x50, 0x34}, //23 //32
    {0x60, 0x29}, //1a //27
    {0x70, 0x34}, //23 //32

    /////// PAGE 15 START ///////
    {0x03, 0x15},
    {0x10, 0x0f},

    //Rstep H 16
    //Rstep L 14
    {0x14, 0x42}, //CMCOFSGH_Day //4c
    {0x15, 0x32}, //CMCOFSGM_CWF //3c
    {0x16, 0x24}, //CMCOFSGL_A //2e
    {0x17, 0x2f}, //CMC SIGN

    //CMC_Default_CWF
    {0x30, 0x8f},
    {0x31, 0x59},
    {0x32, 0x0a},
    {0x33, 0x15},
    {0x34, 0x5b},
    {0x35, 0x06},
    {0x36, 0x07},
    {0x37, 0x40},
    {0x38, 0x87}, //86

    //CMC OFS L_A
    {0x40, 0x92},
    {0x41, 0x1b},
    {0x42, 0x89},
    {0x43, 0x81},
    {0x44, 0x00},
    {0x45, 0x01},
    {0x46, 0x89},
    {0x47, 0x9e},
    {0x48, 0x28},

    //{0x40, 0x93},
    //{0x41, 0x1c},
    //{0x42, 0x89},
    //{0x43, 0x82},
    //{0x44, 0x01},
    //{0x45, 0x01},
    //{0x46, 0x8a},
    //{0x47, 0x9d},
    //{0x48, 0x28},

    //CMC POFS H_DAY
    {0x50, 0x02},
    {0x51, 0x82},
    {0x52, 0x00},
    {0x53, 0x07},
    {0x54, 0x11},
    {0x55, 0x98},
    {0x56, 0x00},
    {0x57, 0x0b},
    {0x58, 0x8b},

    {0x80, 0x03},
    {0x85, 0x40},
    {0x87, 0x02},
    {0x88, 0x00},
    {0x89, 0x00},
    {0x8a, 0x00},

    /////// PAGE 16 START ///////
    {0x03, 0x16},
    {0x10, 0x31},
    {0x18, 0x5e},// Double_AG 5e->37
    {0x19, 0x5d},// Double_AG 5e->36
    {0x1a, 0x0e},
    {0x1b, 0x01},
    {0x1c, 0xdc},
    {0x1d, 0xfe},

    //GMA Default
    {0x30, 0x00}, //0x00
    {0x31, 0x04}, //0x0a //06
    {0x32, 0x18}, //0x1f //1a
    {0x33, 0x30}, //0x33 //31
    {0x34, 0x53},
    {0x35, 0x6c},
    {0x36, 0x81},
    {0x37, 0x94},
    {0x38, 0xa4},
    {0x39, 0xb3},
    {0x3a, 0xc0},
    {0x3b, 0xcb},
    {0x3c, 0xd5},
    {0x3d, 0xde},
    {0x3e, 0xe6},
    {0x3f, 0xee},
    {0x40, 0xf5},
    {0x41, 0xfc},
    {0x42, 0xff},

    {0x50, 0x00},
    {0x51, 0x09},
    {0x52, 0x1f},
    {0x53, 0x37},
    {0x54, 0x5b},
    {0x55, 0x76},
    {0x56, 0x8d},
    {0x57, 0xa1},
    {0x58, 0xb2},
    {0x59, 0xbe},
    {0x5a, 0xc9},
    {0x5b, 0xd2},
    {0x5c, 0xdb},
    {0x5d, 0xe3},
    {0x5e, 0xeb},
    {0x5f, 0xf0},
    {0x60, 0xf5},
    {0x61, 0xf7},
    {0x62, 0xf8},

    {0x70, 0x00},
    {0x71, 0x08},
    {0x72, 0x17},
    {0x73, 0x2f},
    {0x74, 0x53},
    {0x75, 0x6c},
    {0x76, 0x81},
    {0x77, 0x94},
    {0x78, 0xa4},
    {0x79, 0xb3},
    {0x7a, 0xc0},
    {0x7b, 0xcb},
    {0x7c, 0xd5},
    {0x7d, 0xde},
    {0x7e, 0xe6},
    {0x7f, 0xee},
    {0x80, 0xf4},
    {0x81, 0xfa},
    {0x82, 0xff},

    /////// PAGE 17 START ///////
    {0x03, 0x17},
    {0x10, 0xf7},

    /////// PAGE 20 START ///////
    {0x03, 0x20},
    {0x11, 0x1c},
    {0x18, 0x30},
    {0x1a, 0x08},
    {0x20, 0x01}, //05_lowtemp Y Mean off
    {0x21, 0x30},
    {0x22, 0x10},
    {0x23, 0x00},
    {0x24, 0x00}, //Uniform Scene Off

    {0x28, 0xe7},
    {0x29, 0x0d}, //20100305 ad->0d
    {0x2a, 0xff},
    {0x2b, 0x04}, //f4->Adaptive off

    {0x2c, 0xc2},
    {0x2d, 0xcf},  //fe->AE Speed option
    {0x2e, 0x33},
    {0x30, 0x78}, //f8
    {0x32, 0x03},
    {0x33, 0x2e},
    {0x34, 0x30},
    {0x35, 0xd4},
    {0x36, 0xfe},
    {0x37, 0x32},
    {0x38, 0x04},

    {0x39, 0x22}, //AE_escapeC10
    {0x3a, 0xde}, //AE_escapeC11

    {0x3b, 0x22}, //AE_escapeC1
    {0x3c, 0xde}, //AE_escapeC2

    {0x50, 0x45},
    {0x51, 0x88},

    {0x56, 0x03},
    {0x57, 0xf7},
    {0x58, 0x14},
    {0x59, 0x88},
    {0x5a, 0x04},

    //New Weight For Samsung
    {0x60, 0xff},
    {0x61, 0xff},
    {0x62, 0xea},
    {0x63, 0xab},
    {0x64, 0xea},
    {0x65, 0xab},
    {0x66, 0xeb},
    {0x67, 0xeb},
    {0x68, 0xeb},
    {0x69, 0xeb},
    {0x6a, 0xea},
    {0x6b, 0xab},
    {0x6c, 0xea},
    {0x6d, 0xab},
    {0x6e, 0xff},
    {0x6f, 0xff},

    //{0x60, 0x55}, // AEWGT1
    //{0x61, 0x55}, // AEWGT2
    //{0x62, 0x6a}, // AEWGT3
    //{0x63, 0xa9}, // AEWGT4
    //{0x64, 0x6a}, // AEWGT5
    //{0x65, 0xa9}, // AEWGT6
    //{0x66, 0x6a}, // AEWGT7
    //{0x67, 0xa9}, // AEWGT8
    //{0x68, 0x6b}, // AEWGT9
    //{0x69, 0xe9}, // AEWGT10
    //{0x6a, 0x6a}, // AEWGT11
    //{0x6b, 0xa9}, // AEWGT12
    //{0x6c, 0x6a}, // AEWGT13
    //{0x6d, 0xa9}, // AEWGT14
    //{0x6e, 0x55}, // AEWGT15
    //{0x6f, 0x55}, // AEWGT16

    {0x70, 0x76}, //6e
    {0x71, 0x89}, //00 //-4

    // haunting control
    {0x76, 0x43},
    {0x77, 0xe2}, //04 //f2
    {0x78, 0x23}, //Yth1
    {0x79, 0x42}, //Yth2 //46
    {0x7a, 0x23}, //23
    {0x7b, 0x22}, //22
    {0x7d, 0x23},

    {0x83, 0x01}, //EXP Normal 33.33 fps
    {0x84, 0x5f},
    {0x85, 0x00},

    {0x86, 0x02}, //EXPMin 5859.38 fps
    {0x87, 0x00},

    {0x88, 0x04}, //EXP Max 10.00 fps
    {0x89, 0x92},
    {0x8a, 0x00},

    {0x8B, 0x75}, //EXP100
    {0x8C, 0x00},
    {0x8D, 0x61}, //EXP120
    {0x8E, 0x00},

    {0x9c, 0x18}, //EXP Limit 488.28 fps
    {0x9d, 0x00},
    {0x9e, 0x02}, //EXP Unit
    {0x9f, 0x00},

    //AE_Middle Time option
    //{0xa0, 0x03},
    //{0xa1, 0xa9},
    //{0xa2, 0x80},

    {0xb0, 0x18},
    {0xb1, 0x14}, //ADC 400->560
    {0xb2, 0xe0}, //d0
    {0xb3, 0x18},
    {0xb4, 0x1a},
    {0xb5, 0x44},
    {0xb6, 0x2f},
    {0xb7, 0x28},
    {0xb8, 0x25},
    {0xb9, 0x22},
    {0xba, 0x21},
    {0xbb, 0x20},
    {0xbc, 0x1f},
    {0xbd, 0x1f},

    //{0xc0, 0x10},
    //{0xc1, 0x2b},
    //{0xc2, 0x2b},
    //{0xc3, 0x2b},
    //{0xc4, 0x08},

    {0xc8, 0x80},
    {0xc9, 0x40},

    /////// PAGE 22 START ///////
    {0x03, 0x22},
    {0x10, 0xfd},
    {0x11, 0x2e},
    {0x19, 0x01}, // Low On //
    {0x20, 0x30},
    {0x21, 0x80},
    {0x24, 0x01},
    //{0x25, 0x00}, //7f New Lock Cond & New light stable

    {0x30, 0x80},
    {0x31, 0x80},
    {0x38, 0x11},
    {0x39, 0x34},

    {0x40, 0xf7}, //
    {0x41, 0x55}, //44
    {0x42, 0x33}, //43

    {0x43, 0xf7},
    {0x44, 0x55}, //44
    {0x45, 0x44}, //33

    {0x46, 0x00},
    {0x50, 0xb2},
    {0x51, 0x81},
    {0x52, 0x98},

    {0x80, 0x40}, //3e
    {0x81, 0x20},
    {0x82, 0x3e},

    {0x83, 0x5e}, //5e
    {0x84, 0x1e}, //24
    {0x85, 0x5e}, //54 //56 //5a
    {0x86, 0x22}, //24 //22

    {0x87, 0x49},
    {0x88, 0x39},
    {0x89, 0x37}, //38
    {0x8a, 0x28}, //2a

    {0x8b, 0x41}, //47
    {0x8c, 0x39},
    {0x8d, 0x34},
    {0x8e, 0x28}, //2c

    {0x8f, 0x53}, //4e
    {0x90, 0x52}, //4d
    {0x91, 0x51}, //4c
    {0x92, 0x4e}, //4a
    {0x93, 0x4a}, //46
    {0x94, 0x45},
    {0x95, 0x3d},
    {0x96, 0x31},
    {0x97, 0x28},
    {0x98, 0x24},
    {0x99, 0x20},
    {0x9a, 0x20},

    {0x9b, 0x77},
    {0x9c, 0x77},
    {0x9d, 0x48},
    {0x9e, 0x38},
    {0x9f, 0x30},

    {0xa0, 0x60},
    {0xa1, 0x34},
    {0xa2, 0x6f},
    {0xa3, 0xff},

    {0xa4, 0x14}, //1500fps
    {0xa5, 0x2c}, // 700fps
    {0xa6, 0xcf},

    {0xad, 0x40},
    {0xae, 0x4a},

    {0xaf, 0x28},  // low temp Rgain
    {0xb0, 0x26},  // low temp Rgain

    {0xb1, 0x00}, //0x20 -> 0x00 0405 modify
    {0xb4, 0xea},
    {0xb8, 0xa0}, //a2: b-2, R+2  //b4 B-3, R+4 lowtemp
    {0xb9, 0x00},

    /////// PAGE 20 ///////
    {0x03, 0x20},
    {0x10, 0x8c},

    // PAGE 20
    {0x03, 0x20}, //page 20
    {0x10, 0x9c}, //ae off

    // PAGE 22
    {0x03, 0x22}, //page 22
    {0x10, 0xe9}, //awb off

    // PAGE 0
    {0x03, 0x00},
    {0x0e, 0x03}, //PLL On
    {0x0e, 0x73}, //PLLx2

    {0x03, 0x00}, // Dummy 750us
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},
    {0x03, 0x00},

    {0x03, 0x00}, // Page 0
    {0x01, 0x50}, // Sleep Off 0xf8->0x50 for solve green line issue

    {0xff, 0xff},
    {0xff, 0xff},
};
/*@return 0:success,!0:fail*/
void HI253_write_cmos_sensor( unsigned char addr,  unsigned char para)
{
    int dwret = 0;
    unsigned char udata[1];
    udata[0] = para;

    //  DEBUG_INFO("sucess :%d in writing to %x addr\r\n",dwret,addr);
    dwret = i2c_write(HI253_ADDR,addr,1,udata,1);
    //  DEBUG_INFO("sucess :%d in writing to %x addr\r\n",dwret,addr);
    if (dwret != 0)
    {
        DEBUG_INFO("Error:%d in writing to %x addr\r\n",dwret,addr);
    }

    // return dwret;
}

unsigned char HI253_read_cmos_sensor(unsigned char addr)
{
    unsigned char udata[1];

    if(i2c_read(HI253_ADDR,addr,1, udata, 1) != 0){
        DEBUG_INFO("Error:%x read from %x addr\r\n",HI253_ADDR,addr);
        return -1;
    }

    return   udata[0];

}
void HI253_yuv_sensor_initial_setting(void)
{
    unsigned int iEcount;
    int temp = 0;
    DEBUG_INFO("HI253_yuv_sensor_initial_setting\n");


    for(iEcount=0;(!((0xff==(HI253_yuv_sensor_initial_setting_info[iEcount].addr))&&(0xff==(HI253_yuv_sensor_initial_setting_info[iEcount].data))));iEcount++)
    {
        HI253_write_cmos_sensor(HI253_yuv_sensor_initial_setting_info[iEcount].addr, HI253_yuv_sensor_initial_setting_info[iEcount].data);
    }

    HI253_write_cmos_sensor(0x03,0x00);
    temp = HI253_read_cmos_sensor(0x12);
    //DEBUG_INFO("HI253_yuv_sensor_initial_setting: REG[0x12]=#%x\n",temp);

    HI253_write_cmos_sensor(0x03, 0x20);

    HI253_pv_HI253_exposure_lines=(HI253_read_cmos_sensor(0x80)<<16 ) |
        ( HI253_read_cmos_sensor(0x81)<<8) |
        HI253_read_cmos_sensor(0x82);


    DEBUG_INFO("HI253_yuv_sensor_initial_setting: REG[0x12]=#%x,EXP_LINE=%d\n",temp,HI253_pv_HI253_exposure_lines);
}

static void HI253_Preview_Setting (void)
{


    HI253_write_cmos_sensor(0x03,0x00);
    //	HI253_Sleep_Mode = (HI253_read_cmos_sensor(0x01) & 0xfe);
    //	HI253_Sleep_Mode |= 0x01;
    //	HI253_write_cmos_sensor(0x01, HI253_Sleep_Mode);

    HI253_write_cmos_sensor(0x03, 0x20);
    HI253_write_cmos_sensor(0x10, 0x1c);
    HI253_write_cmos_sensor(0x03, 0x22);
    HI253_write_cmos_sensor(0x10, 0x69);

    HI253_write_cmos_sensor(0x03, 0x00);
    HI253_write_cmos_sensor(0x10, 0x11);
    HI253_write_cmos_sensor(0x12, 0x05);

    HI253_write_cmos_sensor(0x20, 0x00);
    HI253_write_cmos_sensor(0x21, 0x04);
    HI253_write_cmos_sensor(0x22, 0x00);
    HI253_write_cmos_sensor(0x23, 0x07);

    HI253_write_cmos_sensor(0x40, 0x01);// 408
    HI253_write_cmos_sensor(0x41, 0x98);
    HI253_write_cmos_sensor(0x42, 0x00);
    HI253_write_cmos_sensor(0x43, 0x14);

    HI253_write_cmos_sensor(0x03, 0x10);
    HI253_write_cmos_sensor(0x3f, 0x02);

    HI253_write_cmos_sensor(0x03, 0x12);
    HI253_write_cmos_sensor(0x20, 0x00); //For solve Black dots issue
    HI253_write_cmos_sensor(0x21, 0x00);//For solve Black dots issue
    HI253_write_cmos_sensor(0x90, 0x00);//For solve Black dots issue

    HI253_write_cmos_sensor(0x03, 0x13);
    HI253_write_cmos_sensor(0x80, 0x00);
#if 0

    if (HI253_CP_mode==KAL_TRUE)
    {

#ifdef HI253_DBG_TRACE
        kal_prompt_trace(MOD_ENG, "[HI253]HI253_CP_mode");
#endif
        HI253_write_cmos_sensor(0x03, 0x20);


        HI253_write_cmos_sensor(0x83, HI253_pv_HI253_exposure_lines >> 16);
        HI253_write_cmos_sensor(0x84, (HI253_pv_HI253_exposure_lines >> 8) & 0x000000FF);
        HI253_write_cmos_sensor(0x85, HI253_pv_HI253_exposure_lines & 0x000000FF);
    }
#endif

    HI253_write_cmos_sensor(0x86, 0x02);
    HI253_write_cmos_sensor(0x87, 0x00);

    HI253_write_cmos_sensor(0x9c, 0x18);
    HI253_write_cmos_sensor(0x9d, 0x00);

    HI253_write_cmos_sensor(0x9e, 0x02);
    HI253_write_cmos_sensor(0x9f, 0x00);

    HI253_write_cmos_sensor(0x8b, 0x75);
    HI253_write_cmos_sensor(0x8c, 0x30);
    HI253_write_cmos_sensor(0x8d, 0x61);
    HI253_write_cmos_sensor(0x8e, 0x00);

    //HI253_Sleep_Mode = (HI253_read_cmos_sensor(0x01) & 0xfe);
    //HI253_Sleep_Mode |= 0x00;
    //HI253_write_cmos_sensor(0x01, HI253_Sleep_Mode);

    HI253_write_cmos_sensor(0x03, 0x20);//page 20
    HI253_write_cmos_sensor(0x10, 0x9c);//AE ON

    HI253_write_cmos_sensor(0x03, 0x22);
    HI253_write_cmos_sensor(0x10, 0xe9);//AWB ON

    DEBUG_INFO("HI253_Preview_Setting Fun\n");

}
#define IMAGE_NORMAL 0
#define IMAGE_H_MIRROR 1
#define IMAGE_V_MIRROR 2
#define IMAGE_HV_MIRROR 3

void HI253_Set_Mirror_Flip(unsigned char image_mirror)
{

    unsigned char HI253_HV_Mirror;
    HI253_write_cmos_sensor(0x03,0x00);
    HI253_HV_Mirror = (HI253_read_cmos_sensor(0x11) & 0xfc);

    switch (image_mirror)
    {
        case IMAGE_NORMAL:
            HI253_HV_Mirror |= 0x03;
            break;

        case IMAGE_H_MIRROR:
            HI253_HV_Mirror |= 0x02;
            break;

        case IMAGE_V_MIRROR:
            HI253_HV_Mirror |= 0x01;
            break;

        case IMAGE_HV_MIRROR:
            HI253_HV_Mirror |= 0x00;
            break;

        default:
            HI253_HV_Mirror |= 0x00;
    }

    HI253_write_cmos_sensor(0x11, HI253_HV_Mirror);

}
#endif  //end of CONFIG_CAM_HI253

#if defined(CONFIG_CAM_OV2655)
const static struct camreg ov2655_common_svgabase[] = {
    /* OV2655_SVGA_YUV 30 fps
     * 24 MHz input clock
     * 800*600 resolution
     */
    //Soft Reset
    {0x3012,0x80},
    //delay 100ms after register reset
    {0x0000,0x64},
    {0x308c,0x80},
    {0x308d,0x0e},
    {0x360b,0x00},
    {0x30b0,0xff},
    {0x30b1,0xff},
    {0x30b2,0x24},
    {0x300e,0x34},
    {0x300f,0xa6},
    {0x3010,0x81},
    {0x3082,0x01},
    {0x30f4,0x01},
    {0x3090,0x33},
    {0x3091,0xc0},
    {0x30ac,0x42},
    {0x30d1,0x08},
    {0x30a8,0x56},
    {0x3015,0x03},
    {0x3093,0x00},
    {0x307e,0xe5},
    {0x3079,0x00},
    {0x30aa,0x42},
    {0x3017,0x40},
    {0x30f3,0x82},
    {0x306a,0x0c},
    {0x306d,0x00},
    {0x336a,0x3c},
    {0x3076,0x6a},
    {0x30d9,0x95},
    {0x3016,0x82},
    {0x3601,0x30},
    {0x304e,0x88},
    {0x30f1,0x82},
    {0x306f,0x14},
    {0x302a,0x02},
    {0x302b,0x6a},
    {0x3012,0x10},
    {0x3011,0x01},
    //AEC/AGC
    {0x3013,0xf7},
    {0x301c,0x13},
    {0x301d,0x17},
    {0x3070,0x5d},
    {0x3072,0x4d},
    //D5060
    {0x30af,0x00},
    {0x3048,0x1f},
    {0x3049,0x4e},
    {0x304a,0x20},
    {0x304f,0x20},
    {0x304b,0x02},
    {0x304c,0x00},
    {0x304d,0x02},
    {0x304f,0x20},
    {0x30a3,0x10},
    {0x3013,0xf7},
    {0x3014,0x44},
    {0x3071,0x00},
    {0x3070,0x5d},
    {0x3073,0x00},
    {0x3072,0x4d},
    {0x301c,0x05},
    {0x301d,0x06},
    {0x304d,0x42},
    {0x304a,0x40},
    {0x304f,0x40},
    {0x3095,0x07},
    {0x3096,0x16},
    {0x3097,0x1d},
    //Window Setup
    {0x300e,0x38},
    {0x3020,0x01},
    {0x3021,0x18},
    {0x3022,0x00},
    {0x3023,0x06},
    {0x3024,0x06},
    {0x3025,0x58},
    {0x3026,0x02},
    {0x3027,0x61},
    {0x3088,0x03},
    {0x3089,0x20},
    {0x308a,0x02},
    {0x308b,0x58},
    {0x3316,0x64},
    {0x3317,0x25},
    {0x3318,0x80},
    {0x3319,0x08},
    {0x331a,0x64},
    {0x331b,0x4b},
    {0x331c,0x00},
    {0x331d,0x38},
    {0x3100,0x00},
    //AWB
    {0x3320,0xfa},
    {0x3321,0x11},
    {0x3322,0x92},
    {0x3323,0x01},
    {0x3324,0x97},
    {0x3325,0x02},
    {0x3326,0xff},
    {0x3327,0x0c},
    {0x3328,0x10},
    {0x3329,0x10},
    {0x332a,0x58},
    {0x332b,0x50},
    {0x332c,0xbe},
    {0x332d,0xe1},
    {0x332e,0x43},
    {0x332f,0x36},
    {0x3330,0x4d},
    {0x3331,0x44},
    {0x3332,0xf8},
    {0x3333,0x0a},
    {0x3334,0xf0},
    {0x3335,0xf0},
    {0x3336,0xf0},
    {0x3337,0x40},
    {0x3338,0x40},
    {0x3339,0x40},
    {0x333a,0x00},
    {0x333b,0x00},
    //Color Matrix
    {0x3380,0x28},
    {0x3381,0x48},
    {0x3382,0x10},
    {0x3383,0x23},
    {0x3384,0xc0},
    {0x3385,0xe5},
    {0x3386,0xc2},
    {0x3387,0xb3},
    {0x3388,0x0e},
    {0x3389,0x98},
    {0x338a,0x01},
    //Gamma
    {0x3340,0x0e},
    {0x3341,0x1a},
    {0x3342,0x31},
    {0x3343,0x45},
    {0x3344,0x5a},
    {0x3345,0x69},
    {0x3346,0x75},
    {0x3347,0x7e},
    {0x3348,0x88},
    {0x3349,0x96},
    {0x334a,0xa3},
    {0x334b,0xaf},
    {0x334c,0xc4},
    {0x334d,0xd7},
    {0x334e,0xe8},
    {0x334f,0x20},
    //Lens correction
    {0x3350,0x32},
    {0x3351,0x25},
    {0x3352,0x80},
    {0x3353,0x1e},
    {0x3354,0x00},
    {0x3355,0x85},
    {0x3356,0x32},
    {0x3357,0x25},
    {0x3358,0x80},
    {0x3359,0x1b},
    {0x335a,0x00},
    {0x335b,0x85},
    {0x335c,0x32},
    {0x335d,0x25},
    {0x335e,0x80},
    {0x335f,0x1b},
    {0x3360,0x00},
    {0x3361,0x85},
    {0x3363,0x70},
    {0x3364,0x7f},
    {0x3365,0x00},
    {0x3366,0x00},
    //UVadjust
    {0x3301,0xff},
    {0x338b,0x11},
    {0x338c,0x10},
    {0x338d,0x40},
    //Sharpness/De-noise
    {0x3370,0xd0},
    {0x3371,0x00},
    {0x3372,0x00},
    {0x3373,0x40},
    {0x3374,0x10},
    {0x3375,0x10},
    {0x3376,0x04},
    {0x3377,0x00},
    {0x3378,0x04},
    {0x3379,0x80},
    //BLC
    {0x3069,0x84},
    {0x307c,0x10},
    {0x3087,0x02},
    //Other functions
    {0x3300,0xfc},
    {0x3302,0x11},
    {0x3400,0x00},
    {0x3606,0x20},
    {0x3601,0x30},
    {0x300e,0x34},
    {0x3011,0x00},
    {0x30f3,0x83},
    {0x304e,0x88},
    //
    {0x3090,0x03},
    {0x30aa,0x32},
    {0x30a3,0x80},
    {0x30a1,0x41},
    {0x363b,0x01},
    {0x363c,0xf2},
    {0x3086,0x0f},
    {0x3086,0x00},
    {0xFFFF,0xFF}
};


/*YUYV 422 (16 bits /pixel)*/
const static struct camreg ov2655_yuv422[] = {
    {0x3100,0x00},
    {0x3300,0xfc},
    {0x3301,0xff},
    /*YUV422(Reg 0x3400)
     *0x00:YUYV
     *0x01:YVYU
     *0x02:UYVY
     *0x03:VYUY
     */
    {0x3400,0x00},
    {0x3606,0x20},
    {0xFFFF,0xFF}
};

/*YUYV 422 (16 bits /pixel)*/
const static struct camreg ov2655_rgb565[] = {
    {0x3100,0x00},
    {0x3300,0xfc},
    {0x3301,0xff},
    /*YUV422(Reg 0x3400)
     *0x00:YUYV
     *0x01:YVYU
     *0x02:UYVY
     *0x03:VYUY
     */
    {0x3400,0x40},
    {0x3606,0x20},
    {0xFFFF,0xFF}
};

/*mirror*/
const static struct camreg ov2655_mirror[] = {
    {0x3090,0x08},
    {0xFFFF,0xFF}
};

/*flip*/
const static struct camreg ov2655_flip[] = {
    {0x307c,0x01},
    {0xFFFF,0xFF}
};

/*normal*/
const static struct camreg ov2655_normal[] = {
    {0x307c,0x00},
    {0x3090,0x00},
    {0xFFFF,0xFF}
};

/*@return 0:success,!0:fail*/
int  ov2655_write_reg(struct camreg *preg)
{
    unsigned char udata[1];
    int dwret = 0;
    unsigned address = 0;

    udata[0] = preg->data;
    address = ( (preg->addr & 0xff) << 8) | ((preg->addr >> 8) & 0xff);

    dwret = i2c_write(OV2655_ADDR,address,2,udata,1);

    if (dwret != 0)
    {
        DEBUG_INFO("Error:%d in writing to %x addr\r\n",dwret,preg->addr);
    }

    return dwret;
}


int ov2655_read_reg(struct camreg *preg)
{
    unsigned char udata[1];
    unsigned short address = preg->addr;
    unsigned short temp = address;

    /* Swap the address bytes. Higher address 1st and lower address 2nd byte */
    address = address << 8 & 0xFF00;
    address = address  | ((temp >> 8) & 0xFF);

    if(i2c_read(OV2655_ADDR,address,2, udata, 1) != 0){
        DEBUG_INFO("Error:%d read from %x addr\r\n",OV2655_ADDR,address);
        return -1;
    }
    preg->data = udata[0];
    return 0;
}


static void ov2655_write_seq(struct camreg *pStream)
{
    struct camreg *pCamReg=pStream;

    while (pCamReg->addr != 0xffff)
    {
        if(pCamReg->addr == 0x00)
        {
            if(pCamReg->data < 1)
            {
                udelay(1000);
            }
            else
            {
                udelay(pCamReg->data*1000);
            }
        }
        else
        {
            ov2655_write_reg( pCamReg );
        }
        pCamReg++;
    }
    // TODO - never returns false!
    return;
}

#endif

#if defined(CONFIG_CAM_OV5640)


/*13.1.1 VGA Preview
; for the setting , 24M Mlck input and 24M Plck output
;15fps YUV mode*/
const static struct camreg ov5640_preview_basic_cfg[]={
    {0x3103,0x11},
    {0x3008,0x82},
    {0x3008,0x42},
    {0x3103,0x03},
    {0x3017,0xff},
    {0x3018,0xff},
    {0x3034,0x1a},
    {0x3035,0x11},
    {0x3036,0x46},
    {0x3037,0x13},
    {0x3108,0x01},
    {0x3630,0x36},
    {0x3631,0x0e},
    {0x3632,0xe2},
    {0x3633,0x12},
    {0x3621,0xe0},
    {0x3704,0xa0},
    {0x3703,0x5a},
    {0x3715,0x78},
    {0x3717,0x01},
    {0x370b,0x60},
    {0x3705,0x1a},
    {0x3905,0x02},
    {0x3906,0x10},
    {0x3901,0x0a},
    {0x3731,0x12},
    {0x3600,0x08},
    {0x3601,0x33},
    {0x302d,0x60},
    {0x3620,0x52},
    {0x371b,0x20},
    {0x471c,0x50},
    {0x3a13,0x43},
    {0x3a18,0x00},
    {0x3a19,0xf8},
    {0x3635,0x13},
    {0x3636,0x03},
    {0x3634,0x40},
    {0x3622,0x01},
    {0x3c01,0x34},
    {0x3c04,0x28},
    {0x3c05,0x98},
    {0x3c06,0x00},
    {0x3c07,0x08},
    {0x3c08,0x00},
    {0x3c09,0x1c},
    {0x3c0a,0x9c},
    {0x3c0b,0x40},
    {0x3820,0x41},
    {0x3821,0x07},
    {0x3814,0x31},
    {0x3815,0x31},
    {0x3800,0x00},
    {0x3801,0x00},
    {0x3802,0x00},
    {0x3803,0x04},
    {0x3804,0x0a},
    {0x3805,0x3f},
    {0x3806,0x07},
    {0x3807,0x9b},
    {0x3808,0x02},
    {0x3809,0x80},
    {0x380a,0x01},
    {0x380b,0xe0},
    {0x380c,0x07},
    {0x380d,0x68},
    {0x380e,0x03},
    {0x380f,0xd8},
    {0x3810,0x00},
    {0x3811,0x10},
    {0x3812,0x00},
    {0x3813,0x06},
    {0x3618,0x00},
    {0x3612,0x29},
    {0x3708,0x62},
    {0x3709,0x52},
    {0x370c,0x03},
    {0x3a02,0x03},
    {0x3a03,0xd8},
    {0x3a08,0x01},
    {0x3a09,0x27},
    {0x3a0a,0x00},
    {0x3a0b,0xf6},
    {0x3a0e,0x03},
    {0x3a0d,0x04},
    {0x3a14,0x03},
    {0x3a15,0xd8},
    {0x4001,0x02},
    {0x4004,0x02},
    {0x3000,0x00},
    {0x3002,0x1c},
    {0x3004,0xff},
    {0x3006,0xc3},
    {0x300e,0x58},
    {0x302e,0x00},
    {0x4300,0x30},
    {0x501f,0x00},
    {0x4713,0x03},
    {0x4407,0x04},
    {0x440e,0x00},
    {0x460b,0x35},
    {0x460c,0x22},
    {0x3824,0x02},
    {0x5000,0xa7},
    {0x5001,0xa3},
    {0x5180,0xff},
    {0x5181,0xf2},
    {0x5182,0x00},
    {0x5183,0x14},
    {0x5184,0x25},
    {0x5185,0x24},
    {0x5186,0x09},
    {0x5187,0x09},
    {0x5188,0x09},
    {0x5189,0x75},
    {0x518a,0x54},
    {0x518b,0xe0},
    {0x518c,0xb2},
    {0x518d,0x42},
    {0x518e,0x3d},
    {0x518f,0x56},
    {0x5190,0x46},
    {0x5191,0xf8},
    {0x5192,0x04},
    {0x5193,0x70},
    {0x5194,0xf0},
    {0x5195,0xf0},
    {0x5196,0x03},
    {0x5197,0x01},
    {0x5198,0x04},
    {0x5199,0x12},
    {0x519a,0x04},
    {0x519b,0x00},
    {0x519c,0x06},
    {0x519d,0x82},
    {0x519e,0x38},
    {0x5381,0x1e},
    {0x5382,0x5b},
    {0x5383,0x08},
    {0x5384,0x0a},
    {0x5385,0x7e},
    {0x5386,0x88},
    {0x5387,0x7c},
    {0x5388,0x6c},
    {0x5389,0x10},
    {0x538a,0x01},
    {0x538b,0x98},
    {0x5300,0x08},
    {0x5301,0x30},
    {0x5302,0x10},
    {0x5303,0x00},
    {0x5304,0x08},
    {0x5305,0x30},
    {0x5306,0x08},
    {0x5307,0x16},
    {0x5309,0x08},
    {0x530a,0x30},
    {0x530b,0x04},
    {0x530c,0x06},
    {0x5480,0x01},
    {0x5481,0x08},
    {0x5482,0x14},
    {0x5483,0x28},
    {0x5484,0x51},
    {0x5485,0x65},
    {0x5486,0x71},
    {0x5487,0x7d},
    {0x5488,0x87},
    {0x5489,0x91},
    {0x548a,0x9a},
    {0x548b,0xaa},
    {0x548c,0xb8},
    {0x548d,0xcd},
    {0x548e,0xdd},
    {0x548f,0xea},
    {0x5490,0x1d},
    {0x5580,0x02},
    {0x5583,0x40},
    {0x5584,0x10},
    {0x5589,0x10},
    {0x558a,0x00},
    {0x558b,0xf8},
    {0x5800,0x23},
    {0x5801,0x14},
    {0x5802,0x0f},
    {0x5803,0x0f},
    {0x5804,0x12},
    {0x5805,0x26},
    {0x5806,0x0c},
    {0x5807,0x08},
    {0x5808,0x05},
    {0x5809,0x05},
    {0x580a,0x08},
    {0x580b,0x0d},
    {0x580c,0x08},
    {0x580d,0x03},
    {0x580e,0x00},
    {0x580f,0x00},
    {0x5810,0x03},
    {0x5811,0x09},
    {0x5812,0x07},
    {0x5813,0x03},
    {0x5814,0x00},
    {0x5815,0x01},
    {0x5816,0x03},
    {0x5817,0x08},
    {0x5818,0x0d},
    {0x5819,0x08},
    {0x581a,0x05},
    {0x581b,0x06},
    {0x581c,0x08},
    {0x581d,0x0e},
    {0x581e,0x29},
    {0x581f,0x17},
    {0x5820,0x11},
    {0x5821,0x11},
    {0x5822,0x15},
    {0x5823,0x28},
    {0x5824,0x46},
    {0x5825,0x26},
    {0x5826,0x08},
    {0x5827,0x26},
    {0x5828,0x64},
    {0x5829,0x26},
    {0x582a,0x24},
    {0x582b,0x22},
    {0x582c,0x24},
    {0x582d,0x24},
    {0x582e,0x06},
    {0x582f,0x22},
    {0x5830,0x40},
    {0x5831,0x42},
    {0x5832,0x24},
    {0x5833,0x26},
    {0x5834,0x24},
    {0x5835,0x22},
    {0x5836,0x22},
    {0x5837,0x26},
    {0x5838,0x44},
    {0x5839,0x24},
    {0x583a,0x26},
    {0x583b,0x28},
    {0x583c,0x42},
    {0x583d,0xce},
    {0x5025,0x00},
    {0x3a0f,0x30},
    {0x3a10,0x28},
    {0x3a1b,0x30},
    {0x3a1e,0x26},
    {0x3a11,0x60},
    {0x3a1f,0x14},
    {0x3008,0x02},
    {0x3035,0x21},
    //Band,0x50Hz
    {0x3c01,0xb4},
    {0x3c00,0x04},
    //gain ceiling
    {0x3a19,0x7c},
    //OV5640 LENC setting
    {0x5800,0x2c},
    {0x5801,0x17},
    {0x5802,0x11},
    {0x5803,0x11},
    {0x5804,0x15},
    {0x5805,0x29},
    {0x5806,0x08},
    {0x5807,0x06},
    {0x5808,0x04},
    {0x5809,0x04},
    {0x580a,0x05},
    {0x580b,0x07},
    {0x580c,0x06},
    {0x580d,0x03},
    {0x580e,0x01},
    {0x580f,0x01},
    {0x5810,0x03},
    {0x5811,0x06},
    {0x5812,0x06},
    {0x5813,0x02},
    {0x5814,0x01},
    {0x5815,0x01},
    {0x5816,0x04},
    {0x5817,0x07},
    {0x5818,0x06},
    {0x5819,0x07},
    {0x581a,0x06},
    {0x581b,0x06},
    {0x581c,0x06},
    {0x581d,0x0e},
    {0x581e,0x31},
    {0x581f,0x12},
    {0x5820,0x11},
    {0x5821,0x11},
    {0x5822,0x11},
    {0x5823,0x2f},
    {0x5824,0x12},
    {0x5825,0x25},
    {0x5826,0x39},
    {0x5827,0x29},
    {0x5828,0x27},
    {0x5829,0x39},
    {0x582a,0x26},
    {0x582b,0x33},
    {0x582c,0x24},
    {0x582d,0x39},
    {0x582e,0x28},
    {0x582f,0x21},
    {0x5830,0x40},
    {0x5831,0x21},
    {0x5832,0x17},
    {0x5833,0x17},
    {0x5834,0x15},
    {0x5835,0x11},
    {0x5836,0x24},
    {0x5837,0x27},
    {0x5838,0x26},
    {0x5839,0x26},
    {0x583a,0x26},
    {0x583b,0x28},
    {0x583c,0x14},
    {0x583d,0xee},
    {0x4005,0x1a},
    //color
    {0x5381,0x26},
    {0x5382,0x50},
    {0x5383,0x0c},
    {0x5384,0x09},
    {0x5385,0x74},
    {0x5386,0x7d},
    {0x5387,0x7e},
    {0x5388,0x75},
    {0x5389,0x09},
    {0x538b,0x98},
    {0x538a,0x01},
    //UVAdjust Auto Mode
    {0x5580,0x02},
    {0x5588,0x01},
    {0x5583,0x40},
    {0x5584,0x10},
    {0x5589,0x0f},
    {0x558a,0x00},
    {0x558b,0x3f},
    //De-Noise,0xAuto
    {0x5308,0x25},
    {0x5304,0x08},
    {0x5305,0x30},
    {0x5306,0x10},
    {0x5307,0x20},
    //awb
    {0x5180,0xff},
    {0x5181,0xf2},
    {0x5182,0x11},
    {0x5183,0x14},
    {0x5184,0x25},
    {0x5185,0x24},
    {0x5186,0x10},
    {0x5187,0x12},
    {0x5188,0x10},
    {0x5189,0x80},
    {0x518a,0x54},
    {0x518b,0xb8},
    {0x518c,0xb2},
    {0x518d,0x42},
    {0x518e,0x3a},
    {0x518f,0x56},
    {0x5190,0x46},
    {0x5191,0xf0},
    {0x5192,0x0f},
    {0x5193,0x70},
    {0x5194,0xf0},
    {0x5195,0xf0},
    {0x5196,0x03},
    {0x5197,0x01},
    {0x5198,0x06},
    {0x5199,0x62},
    {0x519a,0x04},
    {0x519b,0x00},
    {0x519c,0x04},
    {0x519d,0xe7},
    {0x519e,0x38},
    {0xffff,0xff}
};


/*@return 0:success,!0:fail*/
int  ov5640_write_reg(struct camreg *preg)
{
    unsigned char udata[1];
    int dwret = 0;
    unsigned address = 0;

    udata[0] = preg->data;
    address = ( (preg->addr & 0xff) << 8) | ((preg->addr >> 8) & 0xff);

    dwret = i2c_write(OV5640_ADDR,address,2,udata,1);

    if (dwret != 0)
    {
        DEBUG_INFO("Error:%d in writing to %x addr\r\n",dwret,preg->addr);
    }

    return dwret;
}


int ov5640_read_reg(struct camreg *preg)
{
    unsigned char udata[1];
    unsigned short address = preg->addr;
    unsigned short temp = address;

    /* Swap the address bytes. Higher address 1st and lower address 2nd byte */
    address = address << 8 & 0xFF00;
    address = address  | ((temp >> 8) & 0xFF);

    if(i2c_read(OV5640_ADDR,address,2, udata, 1) != 0){
        DEBUG_INFO("Error:%d read from %x addr\r\n",OV5640_ADDR,address);
        return -1;
    }
    preg->data = udata[0];
    return 0;
}


static void ov5640_write_seq(struct camreg *pStream)
{
    struct camreg *pCamReg=pStream;

    while (pCamReg->addr != 0xffff)
    {
        if(pCamReg->addr == 0x00)
        {
            if(pCamReg->data < 1)
            {
                udelay(1000);
            }
            else
            {
                udelay(pCamReg->data*1000);
            }
        }
        else
        {
            ov5640_write_reg( pCamReg );
        }
        pCamReg++;
    }
    // TODO - never returns false!
    return;
}

#endif

/*convert yuv422(YUYV...) picture to rgb565 format*/
void yuv422_2_rgb565(unsigned char *src_yuv,
        unsigned char *dst_rgb,
        unsigned int size)
{

    //register u32 r,g,b;
    //register u32 y0,y1,cb,cr;
    register int y0,cb,cr,y1;
    register int src_pixels = 0, dst_pixels = 0;
    int RGB[3];
    for (; src_pixels < size; src_pixels = src_pixels+4, dst_pixels = dst_pixels + 4)
    {
        y0 = src_yuv[src_pixels];
        cb = src_yuv[src_pixels+1];
        y1 = src_yuv[src_pixels+2];
        cr = src_yuv[src_pixels+3];

        y0 -= 16;
        cb -= 128;
        cr -= 128;
        RGB[0] = (298*y0 + 409 * cr + 128) >> 8;
        RGB[1] = (298*y0- 100 * cb - 208 * cr + 128) >> 8;
        RGB[2] = (298*y0+ 516 * cb + 128) >> 8;

        if(RGB[0] > 255)RGB[0] = 255;
        if(RGB[1] > 255)RGB[1] = 255;
        if(RGB[2] > 255)RGB[2] = 255;

        if(RGB[0] < 0)RGB[0] = 0;
        if(RGB[1] < 0)RGB[1] = 0;
        if(RGB[2] < 0)RGB[2] = 0;

        RGB[0] = (RGB[0] >> 3) & 0x1f;
        RGB[1] = (RGB[1] >> 2) & 0x3f;
        RGB[2] = (RGB[2] >> 3) & 0x1f;

        /* convert from RGB888(24) to RGB565(16) - Lower 16 bits */
        *(u16*)(dst_rgb + dst_pixels) = (RGB[0] << 11)
            |(RGB[1]  << 5)
            | RGB[2];

        y1 -= 16;

        RGB[0] = (298 * y1 + 409 * cr + 128) >> 8;
        RGB[1] = (298 * y1 - 100 * cb - 208 * cr + 128) >> 8;
        RGB[2] = (298 * y1 + 516 * cb + 128) >> 8;

        if(RGB[0] > 255)RGB[0] = 255;
        if(RGB[1] > 255)RGB[1] = 255;
        if(RGB[2] > 255)RGB[2] = 255;

        if(RGB[0] < 0)RGB[0] = 0;
        if(RGB[1] < 0)RGB[1] = 0;
        if(RGB[2] < 0)RGB[2] = 0;
        RGB[0] = (RGB[0] >> 3) & 0x1f;
        RGB[1] = (RGB[1] >> 2) & 0x3f;
        RGB[2] = (RGB[2] >> 3) & 0x1f;

        /* convert from RGB888(24) to RGB565(16) - Lower 16 bits */
        *(u16*)(dst_rgb + dst_pixels + 2) = (RGB[0] << 11)
            |(RGB[1]  << 5)
            | RGB[2];

    }
}

#define OUTPUT_RGB565 1
#define OUTPUT_YUV422 2
#define OUTPUT_DEFAULT OUTPUT_RGB565
#if 0
static void isp_enable(int enable)
{
    __raw_writel(((enable)?1:0) << 0,CCDC_PCR);
}
#endif

/*reset sensor*/
static void sensor_reset(int type)
{
#if defined(CONFIG_3630KUNLUN_KL9C) || defined(CONFIG_3630KUNLUN_P2) || defined(CONFIG_3630PUMA_V1)
    if(type==_MAIN_SENSOR_){

        set_gpio_dataout(98,0);
        /*wait for 10ms*/
        udelay(10000);
        set_gpio_dataout(98,1);
    }else{
        set_gpio_dataout(179,0);
        /*wait for 10ms*/
        udelay(10000);
        set_gpio_dataout(179,1);
    }
#elif defined(CONFIG_3630KUNLUN_WUDANG)
    if(type==_MAIN_SENSOR_){
        set_gpio_dataout(43, 0);/*set GPIO43 low level,CAM_RST active */
        /*wait for 10ms*/
        udelay(10000);
        set_gpio_dataout(43, 1);/*set GPIO43 high level,CAM_RST inactive */
    }else{
        set_gpio_dataout(56, 0);/*set GPIO56 low level,sub sensor reset  */
        /*wait for 10ms*/
        udelay(10000);
        set_gpio_dataout(56, 1);   /*set GPIO56 high level,sub sensor reset  */
    }

#else
    set_gpio_dataout(98,0);
    /*wait for 10ms*/
    udelay(10000);
    set_gpio_dataout(98,1);
#endif

    DEBUG_INFO("CAM_RST signal is expected to hold low for 6ms \n");
}


/*power supply on*/
static void sensor_poweron(int type)
{
    unsigned char buf[1];
    /*enable VAUX4 (2.8v)output*/
    select_bus(0,100);
    buf[0] = 0x09;
    i2c_write(0x4b,0x81,1,buf,sizeof(buf));
    buf[0] = 0xE0;
    i2c_write(0x4b,0x7E,1,buf,sizeof(buf));
    udelay(4000);
    DEBUG_INFO("VAUX4 output is expected to 2.8v\n");
#if defined(CONFIG_3630KUNLUN_KL9C) || defined(CONFIG_3630KUNLUN_P2) || defined(CONFIG_3630PUMA_V1)
    if(type==_MAIN_SENSOR_){

        /* Enable VMMC2 1.85V */
        buf[0] = 0xE0;
        i2c_write(0x4b,0x86,1,buf,sizeof(buf));

        buf[0] = 0x06;
        i2c_write(0x4b,0x89,1,buf,sizeof(buf));
        udelay(4000);
        DEBUG_INFO("VMMC2 output is expected to 1.85v\n");
    }
#elif defined(CONFIG_3630KUNLUN_WUDANG)
        /*WUDANG only use VAUX4(2.8V) for AVDD, VAUX3 for optional use*/
#endif

}

/*enable isp clocks for ov2655*/
static void sensor_enable_clocks(int type)
{
#if defined(CONFIG_3630KUNLUN_KL9C) || defined(CONFIG_3630KUNLUN_P2) || defined(CONFIG_3630PUMA_V1)

    /*enable GPIO 4 function clock*/
    sr32(0x48005000,15,1,1);
    /*enable GPIO 4 interface clock*/
    sr32(0x48005010,15,1,1);

    /*enable GPIO 4 function clock*/
    sr32(0x48005000,17,1,1);
    /*enable GPIO 4 interface clock*/
    sr32(0x48005010,17,1,1);


    if(type==_MAIN_SENSOR_){
        set_gpio_dataout(180,1);/*set GPIO180 low level,sub sensor PWD */
        /*wait for 10ms*/
        udelay(1000);
        DEBUG_INFO("SUB_CAM_PWD signal is ectpected HIGH\n");

        set_gpio_dataout(110,1);   /* GPIO 110ouput 1, PWD */
        /*wait for 10ms*/
        udelay(1000);
        set_gpio_dataout(110,0);   /* GPIO 110ouput 0, PWD */
        DEBUG_INFO("MAIN_CAM_PWD signal is ectpected low\n");
    }else{
        set_gpio_dataout(110,1);   /* GPIO 110ouput 1, PWD */
        /*wait for 10ms*/
        udelay(1000);
        DEBUG_INFO("MAIN_CAM_PWD signal is ectpected HIGH\n");

        set_gpio_dataout(180,1);   /*set GPIO180 low level,sub sensor PWD */
        /*wait for 10ms*/
        udelay(1000);
        set_gpio_dataout(180,0);   /* GPIO 110ouput 0, MDM RST */

        DEBUG_INFO("SUB_CAM_PWD signal is ectpected low\n");
    }

    /*wait for 10ms*/
    /*enalbe CAM_MCLK,CAM_FCLK,CAM_ICLK*/
    __raw_writel(0x1 << 0,0x48004F00);

    __raw_writel(0x1 << 0,0x48004F10); //CM_ICLKEN_CAM,PAGE 530

    if(type==_MAIN_SENSOR_){

        __raw_writel(0x9 << 0,0x480bc050); /*2400000 24MHZ MCLK,XCLKA,TCTRL_CTRL PAGE 1446*/
    }else{
        __raw_writel(0x9 << 5,0x480bc050); /*2400000 24MHZ MCLK,XCLKA&XCLKB,TCTRL_CTRL PAGE 1446*/
    }
#elif defined(CONFIG_3630KUNLUN_WUDANG)
    /*enable GPIO 2 function clock*/
    sr32(0x48005000,13,1,1);
    /*enable GPIO 2 interface clock*/
    sr32(0x48005010,13,1,1);

    /*enable GPIO 4 function clock*/
    sr32(0x48005000,15,1,1);
    /*enable GPIO 4 interface clock*/
    sr32(0x48005010,15,1,1);

    /*enable GPIO 4 function clock*/
    sr32(0x48005000,17,1,1);
    /*enable GPIO 4 interface clock*/
    sr32(0x48005010,17,1,1);

    if(type==_MAIN_SENSOR_){
        set_gpio_dataout(57,1);   /*set GPIO57 high level,sub sensor PWD */
        /*wait for 10ms*/
        udelay(1000);
        DEBUG_INFO("SUB_CAM_PWD signal is expected HIGH\n");

        set_gpio_dataout(110,1);   /* GPIO 110ouput 1, PWD */
        /*wait for 10ms*/
        udelay(1000);
        set_gpio_dataout(110,0);   /* GPIO 110ouput 0, power up */
        DEBUG_INFO("MAIN_CAM_PWD signal is expected low\n");

        /*wait for 10ms*/
        /*enalbe CAM_MCLK,CAM_FCLK,CAM_ICLK*/
        __raw_writel(0x1 << 0,0x48004F00);

        __raw_writel(0x1 << 0,0x48004F10); //CM_ICLKEN_CAM,PAGE 530
        __raw_writel(0x9 << 0,0x480bc050); /*2400000 24MHZ MCLK,XCLKA,TCTRL_CTRL PAGE 1446*/

    }else{
        set_gpio_dataout(110,1);   /* GPIO 110ouput 1, PWD */
        /*wait for 10ms*/
        //udelay(1000);
        DEBUG_INFO("MAIN_CAM_PWD signal is expected HIGH\n");

        set_gpio_dataout(57,0);   /* GPIO57 ouput 0 */

        set_gpio_dataout(56,0);   /*set GPIO56 low level,sub sensor reset  */

        set_gpio_dataout(57,1);   /*set GPIO57 high level,sub sensor chip enable */
        /*wait for 5ms*/
        udelay(5000);

        /*enalbe CAM_MCLK,CAM_FCLK,CAM_ICLK*/
        __raw_writel(0x1 << 0,0x48004F00);

        __raw_writel(0x1 << 0,0x48004F10); //CM_ICLKEN_CAM,PAGE 530

        __raw_writel(0x9 << 5,0x480bc050); /*2400000 24MHZ MCLK,XCLKA&XCLKB,TCTRL_CTRL PAGE 1446*/

        set_gpio_dataout(57,0);   /* GPIO57 ouput 0 */

        DEBUG_INFO("SUB_CAM_PWD signal is expected low\n");

        /*wait for 10ms before reset high*/
        udelay(10000);

        set_gpio_dataout(56,1);   /*set GPIO56 high level,sub sensor reset high */
        DEBUG_INFO("SUB_CAM_RESET signal is expected high\n");
    }
#else
    /*wait for 10ms*/
    /*enalbe CAM_MCLK,CAM_FCLK,CAM_ICLK*/
    __raw_writel(0x1 << 0,0x48004F00);

    __raw_writel(0x1 << 0,0x48004F10); //CM_ICLKEN_CAM,PAGE 530
    __raw_writel(0x9 << 0,0x480bc050);
#endif

    /*wait for 100ms*/
    // udelay(100000);

}

/*initialize camera isp*/
void init_isp(int format)
{
    unsigned int syn_mode,cfg,isp_ctl;
    unsigned int sph,nph,slv,nlv,lnoft;
    //reset isp
    __raw_writel(1 << 1,ISP_SYSCONFIG);
    while((__raw_readl(ISP_SYSSTATUS) & 0x1) == 0)
        udelay(1000);
    DEBUG_INFO("ISP reset done\r\n");

#if defined(SENSOR_OUTPUT_SVGA)
    isp_des_obj.ccdc_in_width = 800;
    isp_des_obj.ccdc_in_heigh = 600;
#endif

#if defined(SENSOR_OUTPUT_VGA)
    isp_des_obj.ccdc_in_width = 640;
    isp_des_obj.ccdc_in_heigh = 480;
#endif
    isp_des_obj.ccdc_out_width = LCD_WIDTH;
    isp_des_obj.ccdc_out_heigh = LCD_HEIGH;

#ifdef USE_RESIZER

#if defined(SENSOR_OUTPUT_SVGA)
    isp_des_obj.ccdc_out_width = 800;
    isp_des_obj.ccdc_out_heigh = 600;
    isp_des_obj.resz_in_width = 800;
    isp_des_obj.resz_in_heigh = 600;
#endif
#if defined(SENSOR_OUTPUT_VGA)
    isp_des_obj.ccdc_out_width = 640;
    isp_des_obj.ccdc_out_heigh = 480;
    isp_des_obj.resz_in_width = 640;
    isp_des_obj.resz_in_heigh = 480;
#endif
    isp_des_obj.resz_out_width = LCD_HEIGH;//LCD_WIDTH;
    isp_des_obj.resz_out_heigh = LCD_WIDTH;//LCD_HEIGH;
#endif //end of USE_RESIZER



    //YUV,DATA:16,16bits/pixel,wen,vdhden
    syn_mode = __raw_readl(CCDC_SYN_MODE);
    if(format == OUTPUT_YUV422)
        syn_mode |= 0x1 << 12 | 0x1 << 16 |  0x1 << 17;
    else
        syn_mode |= 0x1 << 16 |  0x1 << 17;

#ifdef USE_RESIZER
    syn_mode &= ~(0x1 << 17);
    syn_mode |= 0x1 << 19;
#endif

    __raw_writel(syn_mode,CCDC_SYN_MODE);

    cfg = __raw_readl(CCDC_CFG);
    cfg |= 0x1 << 15;
    __raw_writel(cfg,CCDC_CFG);

    isp_ctl = __raw_readl(ISP_CTRL);
    if(format == OUTPUT_YUV422){
        //par bridge is enabled,CCDC_CLK_EN,VS rising edge detect,SBL_WR1_RAM_EN
        isp_ctl |= 0x3 << 2 | 0x1 << 8 | 0x3 << 14 | 0x1 << 16 | 0x1 << 19;
    }else{
        //par bridge is enabled,CCDC_CLK_EN,VS rising edge detect,SBL_WR1_RAM_EN
        isp_ctl |= 0x3 << 2 | 0x1 << 8 | 0x3 << 14 | 0x1 << 16 | 0x1 << 19;
    }
#if defined(USE_RESIZER)
    isp_ctl |= 0x1 << 20;
#endif
    __raw_writel(isp_ctl,ISP_CTRL);
    DEBUG_INFO("ISP_CTRL=%#08x\n",__raw_readl(ISP_CTRL));
    DEBUG_INFO("CCDC_SYN_MODE=%#08x\n",__raw_readl(CCDC_SYN_MODE));
    sph = isp_des_obj.ccdc_in_width - isp_des_obj.ccdc_out_width;
    nph = isp_des_obj.ccdc_out_width - 1;
    slv = isp_des_obj.ccdc_in_heigh - isp_des_obj.ccdc_out_heigh;
    nlv = isp_des_obj.ccdc_out_heigh - 1;
    lnoft = isp_des_obj.ccdc_out_width * 2 - isp_des_obj.ccdc_out_width * 2 % 32;

    __raw_writel(sph << 16 | nph ,CCDC_HORZ_INFO);
    __raw_writel(nlv,CCDC_VERT_LINES);
    __raw_writel(slv << 16,CCDC_VERT_START);
    __raw_writel(lnoft,CCDC_HSIZE_OFF);
    __raw_writel((isp_des_obj.ccdc_in_heigh - 200) << 16
            | (isp_des_obj.ccdc_in_heigh  - 200),CCDC_VDINT);

#ifndef USE_RESIZER
    __raw_writel(CAM_BUFFERS_START,CCDC_SDR_ADDR);
#endif

    DEBUG_INFO("CCDC_HORZ_INFO=%#08x\n",__raw_readl(CCDC_HORZ_INFO));
    DEBUG_INFO("CCDC_VERT_LINES=%#08x\n",__raw_readl(CCDC_VERT_LINES));
    //line offset must be a multiple of 32 bytes
    DEBUG_INFO("CCDC_HSIZE_OFF=%#08x\n",__raw_readl(CCDC_HSIZE_OFF));
    DEBUG_INFO("CCDC_SDR_ADDR=%#08x\n",__raw_readl(CCDC_SDR_ADDR));

    /*CCDC_ERR_IRQ,CCDC_ERR_IRQ,CCDC_VD1_IRQ,CCDC_VD0_IRQ ,CBUFF_IRQ,OVF_IRQ,
      MMU_ERR_IRQ,OCP_ERR_IRQ and RSZ_DONE_IRQ are enabled
      */
    __raw_writel(0x1<<11 |  0x1 << 8 | 0x1<< 9 | 0x1<<11 | 0x1<<21 | 0x1 << 24 | 0x1<<25 | 0x1 << 28 |0x1<<29,ISP_IRQ0ENABLE);


    DEBUG_INFO("ISP_IRQ0ENABLE=%#08x\n",__raw_readl(ISP_IRQ0ENABLE));
    DEBUG_INFO("CCDC_VDINT=%#08x\n",__raw_readl(CCDC_VDINT));
#if defined(USE_RESIZER)

    ispresizer_init();
    ispresizer_try_size(&isp_des_obj.resz_in_width,\
            &isp_des_obj.resz_in_heigh,\
            &isp_des_obj.resz_out_width,\
            &isp_des_obj.resz_out_heigh);
    ispresizer_config_size(isp_des_obj.resz_in_width,\
            isp_des_obj.resz_in_heigh,\
            isp_des_obj.resz_out_width, \
            isp_des_obj.resz_out_heigh);

    ispresizer_config_datapath(RSZ_OTFLY_YUV);
    ispresizer_set_outaddr(CAM_BUFFERS_START);
#endif
    DEBUG_INFO("ISP_IRQ0STATUS=%#08x\n",__raw_readl(ISP_IRQ0STATUS));


}


#if 0
static void read_seq(struct camreg *pStream)
{
    struct camreg cam_read = {0,0};
    struct camreg *pCamReg=pStream;

    while (pCamReg->addr != 0xffff)
    {
        cam_read = *pCamReg;
        if(pCamReg->addr == 0x00)
        {
            pCamReg++;
            continue;
        }
        else
        {
            read_reg(&cam_read);
        }
        if(cam_read.data != pCamReg->data){
            DEBUG_INFO("write %x to register %x error,read: %x\n",pCamReg->data,pCamReg->addr,cam_read.data);
        }

        pCamReg++;
    }

    // TODO - never returns false!
    return;
}
#endif

/*initialize sensor*/
int init_sensor(int format,int type)
{

    struct camreg reg;

#ifdef CONFIG_DRIVER_OMAP34XX_I2C
        /*select i2c2 device*/
    #if defined(CONFIG_3630KUNLUN_KL9C) || defined(CONFIG_3630KUNLUN_P2) || defined(CONFIG_3630PUMA_V1)
        select_bus(1,0x190);
    #elif defined(CONFIG_3630KUNLUN_WUDANG)
        select_bus(1,0x190);
    #else
        select_bus(1,100);
    #endif
#endif

#if defined(CONFIG_3630KUNLUN_KL9C) || defined(CONFIG_3630KUNLUN_P2) || defined(CONFIG_3630PUMA_V1)

    sensor_reset(type);

    if(type==_MAIN_SENSOR_)
    {
        reg.addr = 0x04;
        reg.data = 0x00;
        HI253_read_reg(&reg);

        printf("Product id = %x\n",reg.data);

        HI253_yuv_sensor_initial_setting( );
        DEBUG_INFO("sensorHI253_yuv_sensor_initial_setting\n");
        udelay(1000);
        HI253_Preview_Setting( );
        udelay(1000);
        //HI253_Set_Mirror_Flip(1);
        DEBUG_INFO("HI253_Preview_Setting\n");

    }else {


        reg.addr= 0xFC;
        reg.data=0x00;
        bf3703_read_reg(&reg);
        printf("Product HIGH id = %x\n",reg.data);

        reg.addr= 0xFD;
        reg.data=0x00;
        bf3703_read_reg(&reg);
        printf("Product LOW id = %x\n",reg.data);
        /*wait for 10ms*/
        udelay(1000);
        //	BF3703_init_setting( );
        bf3703_write_seq((struct camreg*)BF3703_common_svgabase);
        DEBUG_INFO("BF3703_common_svgabase_initial_setting\n");
        /*wait for 10ms*/
        udelay(30000);

        BF3703_write_cmos_sensor(0x11,0x90);	//MCLK = 2PCLK
        BF3703_write_cmos_sensor(0x15,0x00);	//HSYNC->HREF
        BF3703_write_cmos_sensor(0x3a,0x00);	// PCLK 0xYUYV

        BF3703_write_cmos_sensor(0x13, 0x07);	// Turn ON AEC/AGC/AWB
        udelay(30000);


    }
#elif defined(CONFIG_3630KUNLUN_WUDANG)
    if(type==_MAIN_SENSOR_)
    {
        reg.addr = 0x300a;
        reg.data = 0x00;
        ov5640_read_reg(&reg);

        printf("Product HIGH id = %x\n",reg.data);

        reg.addr = 0x300b;
        reg.data = 0x00;
        ov5640_read_reg(&reg);

        printf("Product LOW id = %x\n",reg.data);

        ov5640_write_seq((struct camreg*)ov5640_preview_basic_cfg);

    }else {

        reg.addr = 0x04;
        reg.data = 0x00;
        HI253_read_reg(&reg);

        printf("Product id = %x\n",reg.data);
        printf("Product id = %x\n",HI253_read_cmos_sensor(0x04));

        HI253_yuv_sensor_initial_setting( );
        DEBUG_INFO("sensorHI253_yuv_sensor_initial_setting\n");
        udelay(1000);
        HI253_Preview_Setting( );
        udelay(1000);
        //HI253_Set_Mirror_Flip(1);
        DEBUG_INFO("HI253_Preview_Setting\n");
    }
#else
    reg.addr = 0x300a;
    reg.data = 0x00;
    ov2655_read_reg(&reg);
    printf("Product id = %x\n",reg.data);

    ov2655_write_seq((struct camreg*)ov2655_common_svgabase);
    if(format == OUTPUT_YUV422)
        ov2655_write_seq((struct camreg*)ov2655_yuv422);
    else
        ov2655_write_seq((struct camreg*)ov2655_rgb565);

    //ov2655_write_seq((struct camreg*)ov2655_mirror);
    ov2655_write_seq((struct camreg*)ov2655_flip);
    //ov2655_read_seq((struct camreg*)ov2655_common_svgabase);

#endif

    return 0;

}


static void rotation_90(const u16 *src,u16 *dst,u32 width,u32 height)
{
    u32 i = 0,j = 0;
    for(i = 0;i < height;i++)
        for(j = 0;j < width;j++)
            *(dst + j*height + i) = *(src + i*width + j);
}

/**
 * ispccdc_busy - Gets busy state of the CCDC.
 **/
static int ispccdc_busy(void)
{
    return __raw_readl(CCDC_PCR) & CCDC_PCR_BUSY;
}

void handle_isp_int_poll(void)
{
    int irq_status = __raw_readl(ISP_IRQ0STATUS);
    static u32 frames = 0;
    unsigned int cur_frame = 0;
    unsigned int wait = 100000;

    while(1){
        wait = 0x1 << 30;
        while(ispccdc_busy() && wait--)udelay(1);
        if(wait < 0){
            printf("CCDC busy\n");
            return;
        }
        irq_status = __raw_readl(ISP_IRQ0STATUS);
        if(irq_status & ISP_IRQ0ENABLE_CCDC_VD1_IRQ){
            __raw_writel(ISP_IRQ0ENABLE_CCDC_VD1_IRQ,ISP_IRQ0STATUS);
#ifndef USE_RESIZER
            cur_frame = frames++;
            frames %= MAX_BUFFERS;
            __raw_writel(CAMERA_BUFFERS[frames],CCDC_SDR_ADDR);
            DEBUG_INFO("%d frames\n",frames);
            yuv422_2_rgb565((unsigned char*)(CAMERA_BUFFERS[cur_frame]),(unsigned char*)test_lcd,LCD_HEIGH * LCD_WIDTH * 2);
#endif
        }
        if(irq_status & ISP_IRQ0ENABLE_RSZ_DONE_IRQ){
            __raw_writel(ISP_IRQ0ENABLE_RSZ_DONE_IRQ,ISP_IRQ0STATUS);
            cur_frame = frames++;
            frames %= MAX_BUFFERS;
            ispresizer_set_outaddr(CAMERA_BUFFERS[frames]);

            DEBUG_INFO("%d frames\n",frames);

            if(cur_frame == 0){
                yuv422_2_rgb565((unsigned char*)CAMERA_BUFFERS[cur_frame],(unsigned char*)IMAGE_SAVE_RGB565,LCD_HEIGH * LCD_WIDTH * 2);
                rotation_90((unsigned short*)IMAGE_SAVE_RGB565,
                        (unsigned short*)test_lcd,LCD_HEIGH,LCD_WIDTH);
            }
            ispresizer_enable(1);
            DEBUG_INFO("%s:received interrupt ISP_IRQ0ENABLE_RSZ_DONE_IRQ\n",__FUNCTION__);
        }
        if(irq_status & ISP_IRQ0ENABLE_CCDC_VD0_IRQ){
            __raw_writel(ISP_IRQ0ENABLE_CCDC_VD0_IRQ,ISP_IRQ0STATUS);
            DEBUG_INFO("%s:received interrupt ISP_IRQ0ENABLE_CCDC_VD0_IRQ\n",__FUNCTION__);
        }
        if(irq_status & ISP_IRQ0ENABLE_HS_VS_IRQ){
            __raw_writel(ISP_IRQ0ENABLE_HS_VS_IRQ,ISP_IRQ0STATUS);
            DEBUG_INFO("%s:received %d frames\n",__FUNCTION__,frames);
        }
        if(irq_status & ISP_IRQ0ENABLE_MMU_ERR_IRQ){
            __raw_writel(ISP_IRQ0ENABLE_MMU_ERR_IRQ,ISP_IRQ0STATUS);
            DEBUG_INFO("%s:received interrupt ISP_IRQ0ENABLE_MMU_ERR_IRQ\n",__FUNCTION__);
        }
        if(irq_status & ISP_IRQ0ENABLE_OCP_ERR_IRQ){
            __raw_writel(ISP_IRQ0ENABLE_OCP_ERR_IRQ,ISP_IRQ0STATUS);
            DEBUG_INFO("%s:received interrupt ISP_IRQ0ENABLE_OCP_ERR_IRQ\n",__FUNCTION__);
        }
        if(irq_status & ISP_IRQ0ENABLE_CCDC_ERR_IRQ){
            __raw_writel(ISP_IRQ0ENABLE_CCDC_ERR_IRQ,ISP_IRQ0STATUS);
            DEBUG_INFO("%s:received interrupt ISP_IRQ0ENABLE_CCDC_ERR_IRQ\n",__FUNCTION__);
        }
        if(irq_status & ISP_IRQ0ENABLE_OVF_IRQ){
            __raw_writel(ISP_IRQ0ENABLE_OVF_IRQ,ISP_IRQ0STATUS);
            DEBUG_INFO("%s:received interrupt ISP_IRQ0ENABLE_OVF_IRQ\n",__FUNCTION__);
        }
        if(irq_status & ISP_IRQ0ENABLE_CBUFF_IRQ){
            __raw_writel(ISP_IRQ0ENABLE_CBUFF_IRQ,ISP_IRQ0STATUS);
            DEBUG_INFO("%s:received interrupt ISP_IRQ0ENABLE_CBUFF_IRQ\n",__FUNCTION__);
        }
    }
}


/*enable isp ccdc*/
static inline void ispccdc_enable(void)
{
    __raw_writel(0x1 << 0,CCDC_PCR);
}

int do_camtest ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{

    if(argc != 3){
        printf("usage:cam <poweron/enable/on type>\n");
        return -1;
    }
    if(strncmp(argv[1],"poweron",sizeof("poweron") - 1) == 0){
        if(strncmp(argv[2],"main",sizeof("main") - 1)== 0){
            sensor_poweron(_MAIN_SENSOR_);
            sensor_enable_clocks(_MAIN_SENSOR_);
            sensor_reset(_MAIN_SENSOR_);
            init_sensor(OUTPUT_YUV422,_MAIN_SENSOR_);
        }else{
            printf("initial sub sensor\n");
            sensor_poweron(_SUB_SENSOR_);
            sensor_enable_clocks(_SUB_SENSOR_);
            sensor_reset(_SUB_SENSOR_);
            init_sensor(OUTPUT_YUV422,_SUB_SENSOR_);
        }
    }else if(strncmp(argv[1],"enable",sizeof("enable")- 1) == 0){
        //enable CCDC
        init_isp(OUTPUT_YUV422);
        ispccdc_enable();
#if defined(USE_RESIZER)
        //enable resizer
        ispresizer_enable(1);
#endif
        handle_isp_int_poll();
    }else if(strncmp(argv[1],"on",sizeof("on") - 1) == 0){
        if(strncmp(argv[2],"sub",sizeof("sub") - 1)== 0){
            printf("initial sub sensor\n");
            sensor_poweron(_SUB_SENSOR_);
            sensor_enable_clocks(_SUB_SENSOR_);

            #if !defined(CONFIG_3630KUNLUN_WUDANG)
                sensor_reset(_SUB_SENSOR_);
            #endif

            init_isp(OUTPUT_YUV422);
            init_sensor(OUTPUT_YUV422,_SUB_SENSOR_);
        }else{
            printf("default main sensor\n");
            sensor_poweron(_MAIN_SENSOR_);
            sensor_enable_clocks(_MAIN_SENSOR_);

            #if !defined(CONFIG_3630KUNLUN_WUDANG)
                sensor_reset(_MAIN_SENSOR_);
            #endif

            init_isp(OUTPUT_YUV422);
            init_sensor(OUTPUT_YUV422,_MAIN_SENSOR_);
        }
        //enable CCDC
        ispccdc_enable();
#if defined(USE_RESIZER)
        //enable resizer
        ispresizer_enable(1);
#endif
        handle_isp_int_poll();
    }
    return 0;

}


U_BOOT_CMD(
        cam,	CFG_MAXARGS,	1,	do_camtest,
        "cam - test camera\n",
        "[<option>]\n"
        "    - with <option> argument: camera <option> \n"
        );
#endif
