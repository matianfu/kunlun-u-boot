/*
 * =====================================================================================
 *
 *       Filename:  omap_dss.c
 *
 *    Description:  OMAP3630 Display  subsystem
 *
 *        Version:  1.0
 *        Created:  2009
 *
 *	   Author:  matianfu@actnova.com
 *	  company:  ACTNOVA
 *         Author:  Daqing Li (daqli), daqli@via-telecom.com
 *        Company:  VIA TELECOM
 *
 * =====================================================================================
 */

#include <config.h>

#if (defined(CONFIG_LCD_ILI9327)\
    || defined(CONFIG_LCD_RM68041)\
    || defined(CONFIG_LCD_RM68041_N710E)\
    || defined(CONFIG_LCD_NT35510)\
	|| defined(CONFIG_LCD_R61529))

//#define CONFIG_LCD_ILI9327_DEBUG 1

#ifdef CONFIG_LCD_ILI9327_DEBUG
#define DEBUG_INFO(fmt,args...)  printf(fmt,##args)
#else
#define DEBUG_INFO(fmt,args...)
#endif
#include <common.h>
#include <version.h>
#include <stdarg.h>
#include <linux/types.h>
#include <devices.h>
#include <i2c.h>
#include <asm/io.h>
#include <command.h>
#include <twl4030.h>
#include <asm/arch/omap3430.h>
#include <asm/arch/cpu.h>
#include <asm/arch/dss.h>
#include <video_font.h>
#include <bmp_logo.h>


#define LCD_CLOCK_ALWON 1

#define LCD_ON 1
#define LCD_OFF 0

#define VAUX1_DEV_GRP (TWL4030_BASEADD_PM_RECIEVER + 0x17)
#define VAUX1_DEDICATED (TWL4030_BASEADD_PM_RECIEVER + 0x1A)

#define VAUX2_DEV_GRP (TWL4030_BASEADD_PM_RECIEVER + 0x1b)
#define VAUX2_DEDICATED (TWL4030_BASEADD_PM_RECIEVER + 0x1e)
#define VPLL2_DEV_GRP (TWL4030_BASEADD_PM_RECIEVER + 0x33)
#define VPLL2_DEDICATED (TWL4030_BASEADD_PM_RECIEVER + 0x36)

#define RFBI_TRIGGER_ITE 0
#define RFBI_TRIGGER_TE 1
#define RFBI_TRIGGER_SYNC 2

#define FLD_MASK(start, end)	(((1 << (start - end + 1)) - 1) << (end))
#define FLD_VAL(val, start, end) (((val) << end) & FLD_MASK(start, end))

#define LCD_COLOR_FG    0x001F
#define LCD_COLOR_BG    0xFFFF

#define LCD_COR_RED     (0x1F << 11)
#define LCD_COR_GREEN   (0x3F << 5)
#define LCD_COR_BLUE    (0x1F)
#define LCD_COR_WHITE   (0xFFFF)
#define LCD_COR_BLACK   (0x0000);

#if defined(CONFIG_BACKLIGHT_CAT3648) || defined(CONFIG_BACKLIGHT_CAT3637)||defined(CONFIG_BACKLIGHT_TWL5030_PWM)
extern void backlight_on (void);
extern void backlight_off(void);
#endif

extern void sr32(u32 addr, u32 start_bit, u32 num_bits, u32 value);

static void omap34xx_dss_clocks_on(void);
static void omap3_disp_config_dss(void);
static void omap3_disp_config_dispc(int is_rfbi);
static void omap3_lcd_poweron(void);

extern int omap3_mcspi_setup_transfer (int num,int word_len,int maxfreq);
extern int omap3_mcspi_send_sync(int num, u8 *buf,int len);
extern void omap3_mcspi_startbit(int num,int high);

//extern unsigned short logo_picture[];

struct omap_video_timings {
	/* Unit: pixels */
	u16 x_res;
	/* Unit: pixels */
	u16 y_res;
	/* Unit: KHz */
	u32 pixel_clock;
	/* Unit: pixel clocks */
	u16 hsw;	/* Horizontal synchronization pulse width */
	/* Unit: pixel clocks */
	u16 hfp;	/* Horizontal front porch */
	/* Unit: pixel clocks */
	u16 hbp;	/* Horizontal back porch */
	/* Unit: line clocks */
	u16 vsw;	/* Vertical synchronization pulse width */
	/* Unit: line clocks */
	u16 vfp;	/* Vertical front porch */
	/* Unit: line clocks */
	u16 vbp;	/* Vertical back porch */
};

/*
 * DSS register I/O routines
 */
static __inline__ u32 dss_reg_in(u32 offset)
{
    return  __raw_readl(DSS_REG_BASE + DSS_REG_OFFSET + offset);
}
static __inline__ u32 dss_reg_out(u32 offset, u32 val)
{
    __raw_writel(val,DSS_REG_BASE + DSS_REG_OFFSET + offset);
    return val;
}
static __inline__ u32 dss_reg_merge(u32 offset, u32 val, u32 mask)
{
    u32 addr = DSS_REG_BASE + DSS_REG_OFFSET + offset;
    u32 new_val = (__raw_readl(addr) & ~mask) | (val & mask);

    __raw_writel(new_val, addr);
    return new_val;
}

/*
 * Display controller register I/O routines
 */
static __inline__ u32 dispc_reg_in(u32 offset)
{
    return __raw_readl(DSS_REG_BASE + DISPC_REG_OFFSET + offset);
}

static __inline__ u32 dispc_reg_out(u32 offset, u32 val)
{
    __raw_writel(val, DSS_REG_BASE + DISPC_REG_OFFSET + offset);
    return val;
}
static __inline__ u32 dispc_reg_merge(u32 offset, u32 val, u32 mask)
{
    u32 addr = DSS_REG_BASE + DISPC_REG_OFFSET + offset;
    u32 new_val = (__raw_readl(addr) & ~mask) | (val & mask);

    __raw_writel(new_val, addr);
    return new_val;
}

/*
 * DSI register Access
 */
static __inline__ u32 dsi_reg_in(u32 offset)
{
    return __raw_readl(DSI_REG_BASE + DSI_REG_OFFSET + offset);
}

static __inline__ u32 dsi_reg_out(u32 offset, u32 val)
{
    __raw_writel(val, DSI_REG_BASE + DSI_REG_OFFSET + offset);
    return val;
}


/*
 * set golcd bit
 */
static void omap3_disp_golcd(void)
{
    u32 dispc_ctrl;
    dispc_ctrl = dispc_reg_in(DISPC_CONTROL);
    dispc_ctrl |= 1 << 5; /** golcd bit **/
    dispc_reg_out(DISPC_CONTROL,dispc_ctrl);
 }


/** configure graphics DMA **/
/** base: pointer to pixel buffer **/
static int omap3_disp_config_gfxdma(void *base)
{
    u32 gfx_attr = dispc_reg_in(DISPC_GFX_ATTRIBUTES);
    dispc_reg_out(DISPC_GFX_BA0,(u32)base);
    //(x,y) of GFX window
    dispc_reg_out(DISPC_GFX_POSITION,0);
    //size of GFX window
    dispc_reg_out(DISPC_GFX_SIZE,(LCD_WIDTH - 1) | (LCD_HEIGH - 1) << 16);
    DEBUG_INFO("GFX_FIFO_THRESHOLD = %#08x\n",dispc_reg_in(DISPC_GFX_FIFO_THRESHOLD));
    DEBUG_INFO("GFX_FIFO_SIZE_STATUS = %#08x\n",dispc_reg_in(DISPC_GFX_FIFO_SIZE));
    DEBUG_INFO("GFX_ROW_INC = %#08x\n",dispc_reg_in(DISPC_GFX_ROW_INC));
    DEBUG_INFO("GFX_PIXEL_INC = %#08x\n",dispc_reg_in(DISPC_GFX_PIXEL_INC));
    DEBUG_INFO("GFX_ATTRIBUTES = %#08x\n",gfx_attr);

    //GFX format:RGB16
    gfx_attr |= 0x6 << 1;
    dispc_reg_out(DISPC_GFX_ATTRIBUTES,gfx_attr);
    omap3_disp_golcd();
    return 0;

}

/*enable DISPC and DMA output*/
static void omap3_disp_dispc_enable(void)
{
    dispc_reg_merge(DISPC_CONTROL,0x1,0x1);
}


/*enable  Graphics DMA output*/
static void omap3_disp_gfxdma_enable(void)
{
    dispc_reg_merge(DISPC_GFX_ATTRIBUTES,0x1,0x1);
    omap3_disp_golcd();
}


/*enable  Graphics DMA output*/
static void omap3_disp_dispc_disable(void)
{
    dispc_reg_merge(DISPC_CONTROL,0x0,0x1);
}


#ifdef CONFIG_LCD_ILI9327_DEBUG

/* display ALL clocks frequency related DSS*/
static void omap3_dss_print_clocks(void)
{
    u32 sys_clk = 0;
    u32 osc_sys_clk = 26000000;
    u32 dpll4_alwon_fclk = 0;
    u32 dss1_alwon_fclk = 0;
    u32 cm_clksel_pll = 0;
    u32 lcd,pcd,dispc_divisor,dispc_pixel_clk = 0;
    printf("the source clock of sys_clk is %dHz\n",osc_sys_clk);
    switch(__raw_readl(PRM_CLKSEL) & 0x7){
        case 0:
            sys_clk = 12000000;
            break;
        case 1:
            sys_clk = 13000000;
            break;
        case 2:
            sys_clk = 19200000;
            break;
        case 3:
            sys_clk = 26000000;
            break;
        case 4:
            sys_clk = 38400000;
            break;
        case 5:
            sys_clk = 16800000;
            break;
   }
   printf("sys_clk is expected %d Hz\n",sys_clk);
   sys_clk = osc_sys_clk / ((__raw_readl(PRM_CLKSRC_CTRL) >> 6) & 0x3);
   printf("sys_clk is actually %d Hz\n",sys_clk);

   if((__raw_readl(CM_IDLEST_CKGEN) >> 11) & 1){
       printf("dss1_alwon_fclk is active\n");
   }else{
       printf("dss1_alwon_fclk is inactive\n");
   }

   if((__raw_readl(CM_IDLEST_CKGEN) >> 1) & 1){
       printf("DPLL4 is locked\n");
   }else{
       printf("DPLL4 is bypassed\n");
   }

   if((__raw_readl(PRM_CLKSRC_CTRL) >> 8) & 1){
       dpll4_alwon_fclk = sys_clk * 2 / 13;/*sys_clk / 6.5*/
   }else{
       dpll4_alwon_fclk = sys_clk;
   }
   printf("dpll4_alwon_clk is %d Hz\n",dpll4_alwon_fclk);

   cm_clksel_pll = __raw_readl(CM_CLKSEL2_PLL);
   printf("M:%d,N:%d\n",((cm_clksel_pll >> 8)&0xFFF),(cm_clksel_pll & 0x7F));

   dss1_alwon_fclk = dpll4_alwon_fclk / ((cm_clksel_pll & 0x7F) + 1) * ((cm_clksel_pll >> 8) & 0xFFF) * 2;

   dss1_alwon_fclk /= (__raw_readl(CM_CLKSEL_DSS) & 0x1F);
   printf("dss1_alwon_fclk is %d Hz\n",dss1_alwon_fclk);

   printf("the source of DISPC_FCLK is ");
   if(dss_reg_in(DSS_CONTROL) & 0x1){
       printf("DSI1_PLL_FCLK\n");
       return;
   }else{
       printf("DSS1_ALWON_FCLK\n");
   }
   dispc_divisor = dispc_reg_in(DISPC_DIVISOR);
   lcd =  (dispc_divisor >> 16)& 0xFF;
   pcd = dispc_divisor & 0xFF;

   dispc_pixel_clk = dss1_alwon_fclk / lcd / pcd;
   printf("dispc_pixel_clk is %d Hz\n",dispc_pixel_clk);

   printf("DISPC_CONTROL=%#08x\n",dispc_reg_in(DISPC_CONTROL));

}

#endif

/*Description:enable LDO output voltage = 1.8v
  VPLL2 belongs to all the device groups
  */
static void omap3_lcd_poweron(void)
{
    uchar buf[1];

    /*enable VPLL2 (1.8 v) output*/
    buf[0] = 0x05;
    i2c_write(TWL4030_CHIP_PM_RECEIVER,VPLL2_DEDICATED,sizeof(buf),buf,sizeof(buf));
    buf[0] = 0xE0;
    i2c_write(TWL4030_CHIP_PM_RECEIVER,VPLL2_DEV_GRP,sizeof(buf),buf,sizeof(buf));

#if defined(CONFIG_3630KUNLUN_WUDANG)

	    /*enable VAUX2 (2.8v) output*/
    buf[0] = 0x09;
    i2c_write(TWL4030_CHIP_PM_RECEIVER,VAUX2_DEDICATED,sizeof(buf),buf,sizeof(buf));
    buf[0] = 0xE0;
    i2c_write(TWL4030_CHIP_PM_RECEIVER,VAUX2_DEV_GRP,sizeof(buf),buf,sizeof(buf));
#else
    /*enable VAUX1 (3.0v) output*/
    buf[0] = 0x07;
    i2c_write(TWL4030_CHIP_PM_RECEIVER,VAUX1_DEDICATED,sizeof(buf),buf,sizeof(buf));
    buf[0] = 0xE0;
    i2c_write(TWL4030_CHIP_PM_RECEIVER,VAUX1_DEV_GRP,sizeof(buf),buf,sizeof(buf));
#endif

    udelay(400000);
}

/* turn on DSS clock */
static void omap34xx_dss_clocks_on()
{
    u32 clksel_dss = __raw_readl(CM_CLKSEL_DSS);
    u32 iclken_dss = __raw_readl(CM_ICLKEN_DSS);
    u32 fclken_dss = __raw_readl(CM_FCLKEN_DSS);
    u32 clken_pll  = __raw_readl(CM_CLKEN_PLL);
    u32 autoidle_pll = __raw_readl(CM_AUTOIDLE_PLL);

    DEBUG_INFO("CM_CLKSEL_DSS:%#08x\n",clksel_dss);
    DEBUG_INFO("CM_ICLKEN_DSS:%#08x\n",iclken_dss);
    DEBUG_INFO("CM_FCLKEN_DSS:%#08x\n",fclken_dss);
    DEBUG_INFO("CM_CLKEN_PLL:%#08x\n",clken_pll);
    DEBUG_INFO("CM_AUTOIDLE_PLL:%#08x\n",autoidle_pll);

    //power up DPLL4_M3/M4_CLK HSDIVIDER path(DSS1 & TV)
    clken_pll &= ~(1<<29 | 1 << 28);
    //enable DPLL4 in lock mode & automatic recalibration
    clken_pll |= 0x7 << 16 | 0x1 << 19;
    __raw_writel(clken_pll,CM_CLKEN_PLL);

    //disable DPLL4 auto control
    autoidle_pll &= ~(0x7 << 3);
    __raw_writel(autoidle_pll,CM_AUTOIDLE_PLL);

    //DSS1_ALWON_FCLK = DPLL4/9,DSS_TV_FCLK = DPLL4/16
    #if defined(CONFIG_LCD_NT35510)
    //output pclk=26mhz
	clksel_dss &= ~0x0000003f;
	clksel_dss |=0x00000005;
	__raw_writel(clksel_dss,CM_CLKSEL_DSS);
	#else
	clksel_dss |= 0x00001009;
	__raw_writel(clksel_dss,CM_CLKSEL_DSS);
	#endif
    //DSS_L3/L4_ICLK are enabled
    __raw_writel(0x1,CM_ICLKEN_DSS);

    //DSS_ICLK is unrelated to the domain state transition
    __raw_writel(0x0,CM_AUTOIDLE_DSS);

    //enable DSS_TV_FCLK,DSS1/2_ALWON_FCLK
    fclken_dss |= 0x1 | 0x1 << 1 | 0x1 << 2;
    __raw_writel(fclken_dss,CM_FCLKEN_DSS);

    DEBUG_INFO("%s:exit\n",__FUNCTION__,clksel_dss);

}

/*Description:set DSS global registers
  */
void omap3_disp_config_dss(void)
{
    u32 idle_dss;
    u32 rev;
    int wait;

    DEBUG_INFO("Entering %s\n",__FUNCTION__);
    /* turn on DSS clock */
    //omap34xx_ll_config_disp_clocks(0);
    /*reset dss module*/
    idle_dss = 1 << 1;
    idle_dss = dss_reg_out(DSS_SYSCONFIG,idle_dss);

    wait = 1 << 28;
    while(!(dss_reg_in(DSS_SYSSTATUS) & 1) && --wait){}

    if(wait <= 0){DEBUG_INFO("%s:reset dss error\n",__FUNCTION__);return;}

    rev = dss_reg_in(DSS_REVISION);
    DEBUG_INFO( "OMAP Display hardware version %d.%d,expected 2.0\n", \
                (rev & (15 << 4)) >> 4, \
                (rev & (15 << 0)) >> 0);


#ifdef LCD_CLOCK_ALWON
    /*OCP clock is free-running*/
    idle_dss = dss_reg_in(DSS_SYSCONFIG) ;
    idle_dss &= ~(1 << 0);
    dss_reg_out(DSS_SYSCONFIG, idle_dss);
#else
    /* Set auto idle for Display subsystem */
    idle_dss = dss_reg_in(DSS_SYSCONFIG) | DSS_SYSCONFIG_AUTOIDLE;
#endif

    DEBUG_INFO("%s exit!\n",__FUNCTION__);

}

static void omap3_disp_set_lcd_timings(struct omap_video_timings t)
{
	u32 timing_h, timing_v,size_lcd;

    timing_h = FLD_VAL(t.hsw-1, 7, 0) | FLD_VAL(t.hfp-1, 19, 8) |
			FLD_VAL(t.hbp-1, 31, 20);

	timing_v = FLD_VAL(t.vsw-1, 7, 0) | FLD_VAL(t.vfp, 19, 8) |
			FLD_VAL(t.vbp, 31, 20);

	dispc_reg_out(DISPC_TIMING_H, timing_h);
	dispc_reg_out(DISPC_TIMING_V, timing_v);
	dispc_reg_out(DISPC_LINE_NUMBER,t.y_res);

    size_lcd  = FLD_VAL(t.y_res - 1, 26, 16) | FLD_VAL(t.x_res - 1, 10, 0);
	dispc_reg_out(DISPC_SIZE_LCD, size_lcd);

}



/*Description:set display controller
  */
static void omap3_disp_config_dispc(int is_rfbi)
{

    u32 dispc_sysc;
    u32 temp;
    int wait;
    struct omap_video_timings timings;
    DEBUG_INFO("Entering %s\n",__FUNCTION__);

    /*reset DISPC*/
    dispc_reg_out(DISPC_SYSCONFIG,1 << 1);

    wait = 1 << 28;
    while(!(dispc_reg_in(DISPC_SYSSTATUS) & 1) && --wait){}

    if(wait <= 0){DEBUG_INFO("%s:reset dispc error\n",__FUNCTION__);return;}


#ifdef LCD_CLOCK_ALWON
    /*all clocks are always on*/
    dispc_sysc= dispc_reg_in(DISPC_SYSCONFIG);
    dispc_sysc &= ~((3 << 12) | (3 << 3));
    dispc_sysc |= (1 << 12) | (1 << 3) | (3 << 8) | (1 << 2);
    dispc_reg_out(DISPC_SYSCONFIG,dispc_sysc);

    /*function clock gated disabled*/
    temp = dispc_reg_in(DISPC_CONFIG);
    temp &= ~(1 << 9);
    dispc_reg_out(DISPC_CONFIG, temp);
#else
    /* Enable smart standby/idle, autoidle and wakeup */
    dispc_sysc = dispc_reg_in(DISPC_SYSCONFIG);
    dispc_sysc &= ~((3 << 12) | (3 << 3));
    dispc_sysc |= (2 << 12) | (2 << 3) | (1 << 2) | (1 << 0);
    dispc_reg_out(DISPC_SYSCONFIG,dispc_sysc);

    /* Set functional clock autogating */
    temp = dispc_reg_in(DISPC_CONFIG);
    temp |= 1 << 9;
    dispc_reg_out(DISPC_CONFIG, temp);
#endif
    /*config timings*/
    if(!is_rfbi){
        timings.x_res = LCD_WIDTH;
        timings.y_res = LCD_HEIGH;
        timings.hsw = HSW;
        timings.hfp = HFP;
        timings.hbp = HBP;
        timings.vsw = VSW;
        timings.vfp = VFP;
        timings.vbp = VBP;
        omap3_disp_set_lcd_timings(timings);
    }

    /*reset all the status of module internal events*/
    dispc_reg_out(DISPC_IRQSTATUS,0x7FFF);

    /*all interrupts are not masked*/
    dispc_reg_out(DISPC_IRQENABLE,0x1ffff);

#if defined(CONFIG_LCD_ILI9327)
    /* Set logic clock to fck, pixel clock to fck/2 for now */
    dispc_reg_out(DISPC_DIVISOR, 2  << 16 | 4  << 0);
#elif defined(CONFIG_LCD_R61529) || defined(CONFIG_LCD_RM68041) \
    || defined(CONFIG_LCD_RM68041_N710E)
    dispc_reg_out(DISPC_DIVISOR, 1  << 16 | 6  << 0);
#else
    dispc_reg_out(DISPC_DIVISOR, 2  << 16 | 4  << 0);
#endif
    /*VSYNC,HSYNC are low active,data are driven on data lines on falling edge of pixel clock*/

    dispc_reg_out(DISPC_POL_FREQ, POL_FREQ_VALUE);

    /*frame data only loaded every frame*/
    temp = 2 << 1;
    dispc_reg_out(DISPC_CONFIG,temp);

    temp = dispc_reg_in(DISPC_CONTROL);
    /* Enable RFBI, GPIO0/1 */
    temp &= ~((1 << 11) | (1 << 15) | (1 << 16));
    /* Stall mode,RFBI En: GPIO0/1=10  RFBI Dis: GPIO0/1=11,TFT,16 bits*/
    if(is_rfbi)
        temp |= 1 << 11 | 1 << 15 | 1 << 3 | 1 << 8;
    else
        temp |=  1 << 15 | 1 << 16 |1 << 3 | 1 << 8;

    dispc_reg_out(DISPC_CONTROL,temp);
    omap3_disp_golcd();

    DEBUG_INFO("%s exit!\n",__FUNCTION__);

}
#ifdef CONFIG_LCD_NT35510
#define NT35510_READ 1
#define NT35510_WRITE 0

#define NT35510_DATA 1
#define NT35510_CMD 0

#define NT35510_HIGH 1
#define NT35510_LOW 0
#define SPI1 (1)
#define SPI3 (3)
#define NT35510_SPI_CHANEL SPI3
static void spi_send_cmd_nt35510(u16 cmd)
{
    u8 cmd_high=0,cmd_low=0,ctl_byte=0;
    u16 temp_data;
    cmd_high = cmd>>8;
    cmd_low = cmd&0xFF;
    //first transiction
    ctl_byte = (NT35510_WRITE<<7)|(NT35510_CMD<<6)|(NT35510_HIGH<<5);
    temp_data = (ctl_byte<<8)|cmd_high;
    omap3_mcspi_send_sync(NT35510_SPI_CHANEL, &temp_data,1);


     udelay(1000);
    //second transiction
    ctl_byte = (NT35510_WRITE<<7)|(NT35510_CMD<<6)|(NT35510_LOW<<5);
    temp_data = (ctl_byte<<8)|cmd_low;

    omap3_mcspi_send_sync(NT35510_SPI_CHANEL, &temp_data,1);

    udelay(1000);



}

static void spi_send_data_nt35510(u8 data)
{
    u8 ctl_byte=0;
    u16 temp_data=0;
    ctl_byte = (NT35510_WRITE<<7)|(NT35510_DATA<<6)|(NT35510_LOW<<5);
    temp_data = (ctl_byte<<8)|data;
    omap3_mcspi_send_sync(NT35510_SPI_CHANEL, &temp_data,1);
    udelay(1000);

}

static void spi_read_data_nt35510(u16 cmd , u8 *data)
{
    u8 cmd_high=0,cmd_low=0,ctl_byte=0;
    u16 temp_data=0;
    cmd_high = cmd>>8;
    cmd_low = cmd&0xFF;
    //first transiction
    ctl_byte = (NT35510_WRITE<<7)|(NT35510_CMD<<6)|(NT35510_HIGH<<5);
    temp_data = (ctl_byte<<8)|cmd_high;
    omap3_mcspi_send_sync(NT35510_SPI_CHANEL, &temp_data,1);
     udelay(1000);
    //second transiction
    ctl_byte = (NT35510_WRITE<<7)|(NT35510_CMD<<6)|(NT35510_LOW<<5);
    temp_data = (ctl_byte<<8)|cmd_low;
    omap3_mcspi_send_sync(NT35510_SPI_CHANEL, &temp_data,1);
    udelay(1000);

    ctl_byte = (NT35510_READ<<7)|(NT35510_DATA<<6)|(NT35510_LOW<<5);
    temp_data = (ctl_byte<<8)|0xff;
    omap3_mcspi_read_sync(NT35510_SPI_CHANEL,&temp_data,1);
    udelay(1000);

}
/*
void NT35510_Window_Set(u16 sax, u16 say, u16 eax, u16 eay)
{

    //for NT35510

    spi_send_cmd_nt35510(0x2A00);
    spi_send_data_nt35510(((sax+0x0000)>>8)&0x0003);
    spi_send_cmd_nt35510(0x2A01);
    spi_send_data_nt35510((sax+0x0000)&0x00FF);
    spi_send_cmd_nt35510(0x2A02);
    spi_send_data_nt35510(((eax+0x0000)>>8)&0x0003);
    spi_send_cmd_nt35510(0x2A03);
    spi_send_data_nt35510((eax+0x0000)&0x00FF);
    spi_send_cmd_nt35510(0x2B00);
    spi_send_data_nt35510(((say+0x0000)>>8)&0x0003);
    spi_send_cmd_nt35510(0x2B01);
    spi_send_data_nt35510((say+0x0000)&0x00FF);
    spi_send_cmd_nt35510(0x2B02);
    spi_send_data_nt35510(((eay+0x0000)>>8)&0x0003);
    spi_send_cmd_nt35510(0x2B03);
    spi_send_data_nt35510((eay+0x0000)&0x00FF);
}
*/
#endif
static void spi_send_cmd(u8 cmd)
{
#if 0
    u16 temp;
    temp = cmd;
    temp |= 0x0000;
    omap3_mcspi_send_sync(1, (u8*)&temp,2);
#endif
    omap3_mcspi_startbit(1,0);
    omap3_mcspi_send_sync(1, &cmd,1);
}

static void spi_send_data(u8 data)
{
#if 0
    u16 temp;
    temp = data;
    temp |= 0x0100;
    omap3_mcspi_send_sync(1, (u8*)&temp,2);
#endif
    omap3_mcspi_startbit(1,1);
    omap3_mcspi_send_sync(1, &data,1);
}

#ifdef CONFIG_LCD_ILI9327
static void initialise_lcd_panael(void)
{
    spi_send_cmd(0xE9);
    spi_send_data(0x20);
    spi_send_cmd(0x11); //Exit Sleep
    udelay(100);
    spi_send_cmd(0xD1);
    spi_send_data(0x00);
    spi_send_data(0x79);
    spi_send_data(0x1E);
    spi_send_cmd(0xD0);
    spi_send_data(0x05);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_cmd(0x36);
    spi_send_data(0x08);
    spi_send_cmd(0x3A);
    spi_send_data(0x55); //for 16 bit
    spi_send_cmd(0xC1);
    spi_send_data(0x10);
    spi_send_data(0x10);
    spi_send_data(0x02);
    spi_send_data(0x02);
    spi_send_cmd(0xC0); //Set Default Gamma
    spi_send_data(0x00);
    spi_send_data(0x35);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x01);
    spi_send_data(0x02);
    spi_send_cmd(0xC5); //Set frame rate
    spi_send_data(0x02);
    spi_send_cmd(0xD2);
    spi_send_data(0x01);
    spi_send_data(0x22);

    spi_send_cmd(0xC8);  //Set Gamma
    spi_send_data(0x02);
    spi_send_data(0x77);
    spi_send_data(0x47);
    spi_send_data(0x09);
    spi_send_data(0x09);
    spi_send_data(0x00);
    spi_send_data(0x03);
    spi_send_data(0x00);
    spi_send_data(0x57);
    spi_send_data(0x50);
    spi_send_data(0x00);
    spi_send_data(0x10);
    spi_send_data(0x08);
    spi_send_data(0x80);
    spi_send_data(0x00);

    spi_send_cmd(0XC9);    //Set Gamma
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);

    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);

    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);

    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    //4
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);

    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    //8
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    //12
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    //16
    spi_send_data(0x0D);
    spi_send_data(0x09);
    spi_send_data(0x09);
    spi_send_data(0x09);

    spi_send_data(0x09);
    spi_send_data(0x09);
    spi_send_data(0x09);
    spi_send_data(0x0B);

    spi_send_data(0x0B);
    spi_send_data(0x0D);
    spi_send_data(0x0D);
    spi_send_data(0x0E);

    spi_send_data(0x0E);
    spi_send_data(0x0E);
    spi_send_data(0x0E);
    spi_send_data(0x00);
    //16
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);

    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);

    spi_send_data(0x00);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    //20
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    //24
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    spi_send_data(0x04);
    //28
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    spi_send_data(0x04);
    //32
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    spi_send_data(0x04);
    //36
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    spi_send_data(0x04);
    //40
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    spi_send_data(0x04);
    //40
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    spi_send_data(0x04);
    //44
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    spi_send_data(0x04);
    //48
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    spi_send_data(0x04);
    //52
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    spi_send_data(0x04);
    //56
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    spi_send_data(0x04);
    //60
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x01);

    spi_send_data(0x01);
    spi_send_data(0x01);
    spi_send_data(0x04);
    spi_send_data(0x04);
    spi_send_cmd(0xEA); //Enable 3 Gamma
    spi_send_data(0x00);
    spi_send_data(0xC0);

    spi_send_cmd(0xB4);
    spi_send_data(0x10);

    spi_send_cmd(0x36); //set address mode
    spi_send_data(0x88);

    spi_send_cmd(0x2B); //set page address area
    spi_send_data(0x00);
    spi_send_data(0x20);
    spi_send_data(0x01);
    spi_send_data(0xAF);

    spi_send_cmd(0x29); //display on
    spi_send_cmd(0x2C); //Write data to GRAM
}
#elif defined(CONFIG_LCD_RM68041)
static void initialise_lcd_panael(void)
{
    //rm68041
    spi_send_cmd(0x11);
    udelay(20000);
    spi_send_cmd(0xB4);
    spi_send_data(0x10);

    spi_send_cmd(0xD0);
    spi_send_data(0x07);//02
    spi_send_data(0x41);
    spi_send_data(0x1D);//13

    spi_send_cmd(0xD1);
    spi_send_data(0x00);//00
    spi_send_data(0x0e);//0X28
    spi_send_data(0x0e);//19

    spi_send_cmd(0xD2);
    spi_send_data(0x01);
    spi_send_data(0x11);


    spi_send_cmd(0xC0);
    spi_send_data(0x00);
    spi_send_data(0x3B);
    spi_send_data(0x00);
    spi_send_data(0x02);//12
    spi_send_data(0x11);//01

    spi_send_cmd(0xC1);
    spi_send_data(0x10);
    spi_send_data(0x13);
    spi_send_data(0x88);

    spi_send_cmd(0xC5);
    spi_send_data(0x02);

    spi_send_cmd(0xC6);
    spi_send_data(0x03);

    spi_send_cmd(0xC8);
    spi_send_data(0x02);
    spi_send_data(0x46);
    spi_send_data(0x14);
    spi_send_data(0x31);
    spi_send_data(0x0A);
    spi_send_data(0x04);
    spi_send_data(0x37);
    spi_send_data(0x24);
    spi_send_data(0x57);
    spi_send_data(0x13);
    spi_send_data(0x06);
    spi_send_data(0x0C);

    spi_send_cmd(0xF3);
    spi_send_data(0x24);
    spi_send_data(0x1A);

    spi_send_cmd(0xF6);
    spi_send_data(0x80);

    spi_send_cmd(0xF7);
    spi_send_data(0x80);

    spi_send_cmd(0x36);
    spi_send_data(0x0A);
    //spi_send_data(0x08);

    spi_send_cmd(0x3A);
    spi_send_data(0x66);

    spi_send_cmd(0x2A);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x01);
    spi_send_data(0x3F);

    spi_send_cmd(0x2B);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x01);
    spi_send_data(0xDF);

    udelay(120000);
    spi_send_cmd(0x29);

    //udelay(120000);
    //spi_send_cmd(0x2C);
}
#elif defined(CONFIG_LCD_RM68041_N710E)
static void initialise_lcd_panael(void)
{
    //rm68041
	spi_send_cmd(0x11);
	udelay(120*1000);

	spi_send_cmd(0xB4);
	spi_send_data(0x10);//RGB=10  MPU=00

	spi_send_cmd(0xD0);//Power_Setting (D0h)
	spi_send_data(0x07);
	spi_send_data(0x42);
	spi_send_data(0x1C);

	spi_send_cmd(0xD1);//VCOM Control (D1h)
	spi_send_data(0x00);
	spi_send_data(0x30);//37 110427
	spi_send_data(0x1C);//1E 110427

	spi_send_cmd(0xD2);//Power_Setting for Normal Mode
	spi_send_data(0x01);
	spi_send_data(0x11);

	spi_send_cmd(0xC0);//Panel Driving Setting (C0h)
	spi_send_data(0x00);//10
	spi_send_data(0x3B);
	spi_send_data(0x00);
	spi_send_data(0x12);
	spi_send_data(0x01);

	spi_send_cmd(0xC1);
	spi_send_data(0x10);
	spi_send_data(0x16);
	spi_send_data(0x88);
	spi_send_cmd(0xC5);
	spi_send_data(0x02);
#if 1
	spi_send_cmd(0xB3);//Frame Memory Access and Interface Setting (B3h)
	spi_send_data(0x00);
	spi_send_data(0x00);
	spi_send_data(0x00);
	spi_send_data(0x20);
#endif
	spi_send_cmd(0xC5);//Frame Rate and Inversion Control (C5h)
	spi_send_data(0x03);//frame=75hz


	//spi_send_cmd(0xC6);//RGB SIGNAL SETTTING
	//spi_send_data(0x00);

	spi_send_cmd(0xC8);
	spi_send_data(0x02);
	spi_send_data(0x75);
	spi_send_data(0x77);
	spi_send_data(0x05);
	spi_send_data(0x0C);
	spi_send_data(0x00);
	spi_send_data(0x03);
	spi_send_data(0x20);
	spi_send_data(0x57);
	spi_send_data(0x53);
	spi_send_data(0x00);
	spi_send_data(0x0C);

	spi_send_cmd(0xF3);
	spi_send_data(0x40);
	spi_send_data(0x0A);
	spi_send_cmd(0xF6);
	spi_send_data(0x80);
	spi_send_cmd(0xF7);
	spi_send_data(0x80);

	spi_send_cmd(0x36);
	spi_send_data(0x0A);

	spi_send_cmd(0x3A);
	spi_send_data(0x55);

	spi_send_cmd(0x2A);
	spi_send_data(0x00);
	spi_send_data(0x00);
	spi_send_data(0x01);
	spi_send_data(0x3F);
	spi_send_cmd(0x2B);
	spi_send_data(0x00);
	spi_send_data(0x00);
	spi_send_data(0x01);
	spi_send_data(0xDF);
	//spi_send_cmd(0x21);
	//udelay(120);
	spi_send_cmd(0x29);
	udelay(120*1000);
	spi_send_cmd(0x21);
}
#elif defined(CONFIG_LCD_R61529)
static void initialise_lcd_panael(void)
{
    //AFTER 2010 11 17 3.47 lgd HVGA r61529
    spi_send_cmd(0xB0);
    spi_send_data(0x04);

    spi_send_cmd(0x36);
    spi_send_data(0x00);

    spi_send_cmd(0x3A);
    spi_send_data(0x66);

    spi_send_cmd(0xB4);
    spi_send_data(0x00);

    spi_send_cmd(0xC0);
    spi_send_data(0x03);//0013
    spi_send_data(0xDF);//480
    spi_send_data(0x40);
    spi_send_data(0x12);
    spi_send_data(0x00);
    spi_send_data(0x01);
    spi_send_data(0x00);
    spi_send_data(0x43);

    spi_send_cmd(0xC1);//frame frequency
    spi_send_data(0x05);//BCn,DIVn[1:0
    spi_send_data(0x28);//RTNn[4:0]
    spi_send_data(0x04);// BPn[7:0]
    spi_send_data(0x04);// FPn[7:0]
    spi_send_data(0x00);

    spi_send_cmd(0xC4);
    spi_send_data(0x64);//54
    spi_send_data(0x00);
    spi_send_data(0x08);
    spi_send_data(0x08);

    spi_send_cmd(0xC8);//Gamma
    spi_send_data(0x06);
    spi_send_data(0x0c);
    spi_send_data(0x16);
    spi_send_data(0x24);//26
    spi_send_data(0x30);//32
    spi_send_data(0x48);
    spi_send_data(0x3d);
    spi_send_data(0x28);
    spi_send_data(0x20);
    spi_send_data(0x14);
    spi_send_data(0x0c);
    spi_send_data(0x04);

    spi_send_data(0x06);
    spi_send_data(0x0c);
    spi_send_data(0x16);
    spi_send_data(0x24);
    spi_send_data(0x30);
    spi_send_data(0x48);
    spi_send_data(0x3d);
    spi_send_data(0x28);
    spi_send_data(0x20);
    spi_send_data(0x14);
    spi_send_data(0x0c);
    spi_send_data(0x04);

    spi_send_cmd(0xC9);//Gamma
    spi_send_data(0x06);
    spi_send_data(0x0c);
    spi_send_data(0x16);
    spi_send_data(0x24);//26
    spi_send_data(0x30);//32
    spi_send_data(0x48);
    spi_send_data(0x3d);
    spi_send_data(0x28);
    spi_send_data(0x20);
    spi_send_data(0x14);
    spi_send_data(0x0c);
    spi_send_data(0x04);

    spi_send_data(0x06);
    spi_send_data(0x0c);
    spi_send_data(0x16);
    spi_send_data(0x24);
    spi_send_data(0x30);
    spi_send_data(0x48);
    spi_send_data(0x3d);
    spi_send_data(0x28);
    spi_send_data(0x20);
    spi_send_data(0x14);
    spi_send_data(0x0c);
    spi_send_data(0x04);

    spi_send_cmd(0xCA);//Gamma
    spi_send_data(0x06);
    spi_send_data(0x0c);
    spi_send_data(0x16);
    spi_send_data(0x24);//26
    spi_send_data(0x30);//32
    spi_send_data(0x48);
    spi_send_data(0x3d);
    spi_send_data(0x28);
    spi_send_data(0x20);
    spi_send_data(0x14);
    spi_send_data(0x0c);
    spi_send_data(0x04);

    spi_send_data(0x06);
    spi_send_data(0x0c);
    spi_send_data(0x16);
    spi_send_data(0x24);
    spi_send_data(0x30);
    spi_send_data(0x48);
    spi_send_data(0x3d);
    spi_send_data(0x28);
    spi_send_data(0x20);
    spi_send_data(0x14);
    spi_send_data(0x0c);
    spi_send_data(0x04);

    spi_send_cmd(0xD0);
    spi_send_data(0x95);
    spi_send_data(0x06);
    spi_send_data(0x08);
    spi_send_data(0x10);
    spi_send_data(0x3c);

    spi_send_cmd(0xD1);
    spi_send_data(0x02);
    spi_send_data(0x2c);
    spi_send_data(0x2c);
    spi_send_data(0x3c);

    spi_send_cmd(0xE1);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);
    spi_send_data(0x00);

    spi_send_cmd(0xE2);
    spi_send_data(0x80);

    spi_send_cmd(0x11);

    udelay(120 * 1000);

    spi_send_cmd(0x29);

}
#elif defined(CONFIG_LCD_NT35510)
static void initialise_lcd_panael(void)
{

    udelay(50000);

    spi_send_cmd_nt35510(0xF000);spi_send_data_nt35510(0x55);
    spi_send_cmd_nt35510(0xF001);spi_send_data_nt35510(0xAA);
    spi_send_cmd_nt35510(0xF002);spi_send_data_nt35510(0x52);
    spi_send_cmd_nt35510(0xF003);spi_send_data_nt35510(0x08);
    spi_send_cmd_nt35510(0xF004);spi_send_data_nt35510(0x01);

    spi_send_cmd_nt35510(0xBC01);spi_send_data_nt35510(0xA8);
    spi_send_cmd_nt35510(0xBC02);spi_send_data_nt35510(0x10);
    spi_send_cmd_nt35510(0xBD01);spi_send_data_nt35510(0xA8);
    spi_send_cmd_nt35510(0xBD02);spi_send_data_nt35510(0x10);

    spi_send_cmd_nt35510(0xBE01);spi_send_data_nt35510(0x6C);
    //R+
    spi_send_cmd_nt35510(0xD100);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD101);spi_send_data_nt35510(0x5D);
    spi_send_cmd_nt35510(0xD102);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD103);spi_send_data_nt35510(0x69);
    spi_send_cmd_nt35510(0xD104);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD105);spi_send_data_nt35510(0x7F);
    spi_send_cmd_nt35510(0xD106);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD107);spi_send_data_nt35510(0x92);
    spi_send_cmd_nt35510(0xD108);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD109);spi_send_data_nt35510(0xA3);
    spi_send_cmd_nt35510(0xD10A);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD10B);spi_send_data_nt35510(0xBF);
    spi_send_cmd_nt35510(0xD10C);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD10D);spi_send_data_nt35510(0xD8);
    spi_send_cmd_nt35510(0xD10E);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD10F);spi_send_data_nt35510(0xFE);
    spi_send_cmd_nt35510(0xD110);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD111);spi_send_data_nt35510(0x1D);
    spi_send_cmd_nt35510(0xD112);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD113);spi_send_data_nt35510(0x4E);
    spi_send_cmd_nt35510(0xD114);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD115);spi_send_data_nt35510(0x73);
    spi_send_cmd_nt35510(0xD116);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD117);spi_send_data_nt35510(0xAD);
    spi_send_cmd_nt35510(0xD118);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD119);spi_send_data_nt35510(0xDC);
    spi_send_cmd_nt35510(0xD11A);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD11B);spi_send_data_nt35510(0xDD);
    spi_send_cmd_nt35510(0xD11C);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD11D);spi_send_data_nt35510(0x05);
    spi_send_cmd_nt35510(0xD11E);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD11F);spi_send_data_nt35510(0x2D);
    spi_send_cmd_nt35510(0xD120);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD121);spi_send_data_nt35510(0x43);
    spi_send_cmd_nt35510(0xD122);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD123);spi_send_data_nt35510(0x60);
    spi_send_cmd_nt35510(0xD124);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD125);spi_send_data_nt35510(0x79);
    spi_send_cmd_nt35510(0xD126);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD127);spi_send_data_nt35510(0xA5);
    spi_send_cmd_nt35510(0xD128);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD129);spi_send_data_nt35510(0xCE);
    spi_send_cmd_nt35510(0xD12A);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD12B);spi_send_data_nt35510(0x0F);
    spi_send_cmd_nt35510(0xD12C);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD12D);spi_send_data_nt35510(0x49);
    spi_send_cmd_nt35510(0xD12E);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD12F);spi_send_data_nt35510(0x83);
    spi_send_cmd_nt35510(0xD130);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD131);spi_send_data_nt35510(0xC7);
    spi_send_cmd_nt35510(0xD132);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD133);spi_send_data_nt35510(0xCC);
    //G+
    spi_send_cmd_nt35510(0xD200);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD201);spi_send_data_nt35510(0x5D);
    spi_send_cmd_nt35510(0xD202);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD203);spi_send_data_nt35510(0x69);
    spi_send_cmd_nt35510(0xD204);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD205);spi_send_data_nt35510(0x7F);
    spi_send_cmd_nt35510(0xD206);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD207);spi_send_data_nt35510(0x92);
    spi_send_cmd_nt35510(0xD208);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD209);spi_send_data_nt35510(0xA3);
    spi_send_cmd_nt35510(0xD20A);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD20B);spi_send_data_nt35510(0xBF);
    spi_send_cmd_nt35510(0xD20C);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD20D);spi_send_data_nt35510(0xD8);
    spi_send_cmd_nt35510(0xD20E);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD20F);spi_send_data_nt35510(0xFE);
    spi_send_cmd_nt35510(0xD210);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD211);spi_send_data_nt35510(0x1D);
    spi_send_cmd_nt35510(0xD212);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD213);spi_send_data_nt35510(0x4E);
    spi_send_cmd_nt35510(0xD214);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD215);spi_send_data_nt35510(0x73);
    spi_send_cmd_nt35510(0xD216);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD217);spi_send_data_nt35510(0xAD);
    spi_send_cmd_nt35510(0xD218);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD219);spi_send_data_nt35510(0xDC);
    spi_send_cmd_nt35510(0xD21A);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD21B);spi_send_data_nt35510(0xDD);
    spi_send_cmd_nt35510(0xD21C);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD21D);spi_send_data_nt35510(0x05);
    spi_send_cmd_nt35510(0xD21E);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD21F);spi_send_data_nt35510(0x2D);
    spi_send_cmd_nt35510(0xD220);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD221);spi_send_data_nt35510(0x43);
    spi_send_cmd_nt35510(0xD222);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD223);spi_send_data_nt35510(0x60);
    spi_send_cmd_nt35510(0xD224);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD225);spi_send_data_nt35510(0x79);
    spi_send_cmd_nt35510(0xD226);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD227);spi_send_data_nt35510(0xA5);
    spi_send_cmd_nt35510(0xD228);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD229);spi_send_data_nt35510(0xCE);
    spi_send_cmd_nt35510(0xD22A);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD22B);spi_send_data_nt35510(0x0F);
    spi_send_cmd_nt35510(0xD22C);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD22D);spi_send_data_nt35510(0x49);
    spi_send_cmd_nt35510(0xD22E);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD22F);spi_send_data_nt35510(0x83);
    spi_send_cmd_nt35510(0xD230);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD231);spi_send_data_nt35510(0xC7);
    spi_send_cmd_nt35510(0xD232);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD233);spi_send_data_nt35510(0xCC);
    //B+
    spi_send_cmd_nt35510(0xD300);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD301);spi_send_data_nt35510(0x5D);
    spi_send_cmd_nt35510(0xD302);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD303);spi_send_data_nt35510(0x69);
    spi_send_cmd_nt35510(0xD304);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD305);spi_send_data_nt35510(0x7F);
    spi_send_cmd_nt35510(0xD306);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD307);spi_send_data_nt35510(0x92);
    spi_send_cmd_nt35510(0xD308);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD309);spi_send_data_nt35510(0xA3);
    spi_send_cmd_nt35510(0xD30A);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD30B);spi_send_data_nt35510(0xBF);
    spi_send_cmd_nt35510(0xD30C);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD30D);spi_send_data_nt35510(0xD8);
    spi_send_cmd_nt35510(0xD30E);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD30F);spi_send_data_nt35510(0xFE);
    spi_send_cmd_nt35510(0xD310);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD311);spi_send_data_nt35510(0x1D);
    spi_send_cmd_nt35510(0xD312);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD313);spi_send_data_nt35510(0x4E);
    spi_send_cmd_nt35510(0xD314);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD315);spi_send_data_nt35510(0x73);
    spi_send_cmd_nt35510(0xD316);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD317);spi_send_data_nt35510(0xAD);
    spi_send_cmd_nt35510(0xD318);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD319);spi_send_data_nt35510(0xDC);
    spi_send_cmd_nt35510(0xD31A);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD31B);spi_send_data_nt35510(0xDD);
    spi_send_cmd_nt35510(0xD31C);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD31D);spi_send_data_nt35510(0x05);
    spi_send_cmd_nt35510(0xD31E);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD31F);spi_send_data_nt35510(0x2D);
    spi_send_cmd_nt35510(0xD320);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD321);spi_send_data_nt35510(0x43);
    spi_send_cmd_nt35510(0xD322);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD323);spi_send_data_nt35510(0x60);
    spi_send_cmd_nt35510(0xD324);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD325);spi_send_data_nt35510(0x79);
    spi_send_cmd_nt35510(0xD326);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD327);spi_send_data_nt35510(0xA5);
    spi_send_cmd_nt35510(0xD328);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD329);spi_send_data_nt35510(0xCE);
    spi_send_cmd_nt35510(0xD32A);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD32B);spi_send_data_nt35510(0x0F);
    spi_send_cmd_nt35510(0xD32C);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD32D);spi_send_data_nt35510(0x49);
    spi_send_cmd_nt35510(0xD32E);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD32F);spi_send_data_nt35510(0x83);
    spi_send_cmd_nt35510(0xD330);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD331);spi_send_data_nt35510(0xC7);
    spi_send_cmd_nt35510(0xD332);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD333);spi_send_data_nt35510(0xCC);
    //R-
    spi_send_cmd_nt35510(0xD400);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD401);spi_send_data_nt35510(0x5D);
    spi_send_cmd_nt35510(0xD402);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD403);spi_send_data_nt35510(0x69);
    spi_send_cmd_nt35510(0xD404);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD405);spi_send_data_nt35510(0x7F);
    spi_send_cmd_nt35510(0xD406);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD407);spi_send_data_nt35510(0x92);
    spi_send_cmd_nt35510(0xD408);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD409);spi_send_data_nt35510(0xA3);
    spi_send_cmd_nt35510(0xD40A);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD40B);spi_send_data_nt35510(0xBF);
    spi_send_cmd_nt35510(0xD40C);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD40D);spi_send_data_nt35510(0xD8);
    spi_send_cmd_nt35510(0xD40E);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD40F);spi_send_data_nt35510(0xFE);
    spi_send_cmd_nt35510(0xD410);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD411);spi_send_data_nt35510(0x1D);
    spi_send_cmd_nt35510(0xD412);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD413);spi_send_data_nt35510(0x4E);
    spi_send_cmd_nt35510(0xD414);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD415);spi_send_data_nt35510(0x73);
    spi_send_cmd_nt35510(0xD416);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD417);spi_send_data_nt35510(0xAD);
    spi_send_cmd_nt35510(0xD418);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD419);spi_send_data_nt35510(0xDC);
    spi_send_cmd_nt35510(0xD41A);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD41B);spi_send_data_nt35510(0xDD);
    spi_send_cmd_nt35510(0xD41C);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD41D);spi_send_data_nt35510(0x05);
    spi_send_cmd_nt35510(0xD41E);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD41F);spi_send_data_nt35510(0x2D);
    spi_send_cmd_nt35510(0xD420);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD421);spi_send_data_nt35510(0x43);
    spi_send_cmd_nt35510(0xD422);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD423);spi_send_data_nt35510(0x60);
    spi_send_cmd_nt35510(0xD424);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD425);spi_send_data_nt35510(0x79);
    spi_send_cmd_nt35510(0xD426);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD427);spi_send_data_nt35510(0xA5);
    spi_send_cmd_nt35510(0xD428);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD429);spi_send_data_nt35510(0xCE);
    spi_send_cmd_nt35510(0xD42A);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD42B);spi_send_data_nt35510(0x0F);
    spi_send_cmd_nt35510(0xD42C);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD42D);spi_send_data_nt35510(0x49);
    spi_send_cmd_nt35510(0xD42E);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD42F);spi_send_data_nt35510(0x83);
    spi_send_cmd_nt35510(0xD430);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD431);spi_send_data_nt35510(0xC7);
    spi_send_cmd_nt35510(0xD432);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD433);spi_send_data_nt35510(0xCC);
    //G-
    spi_send_cmd_nt35510(0xD500);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD501);spi_send_data_nt35510(0x5D);
    spi_send_cmd_nt35510(0xD502);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD503);spi_send_data_nt35510(0x69);
    spi_send_cmd_nt35510(0xD504);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD505);spi_send_data_nt35510(0x7F);
    spi_send_cmd_nt35510(0xD506);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD507);spi_send_data_nt35510(0x92);
    spi_send_cmd_nt35510(0xD508);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD509);spi_send_data_nt35510(0xA3);
    spi_send_cmd_nt35510(0xD50A);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD50B);spi_send_data_nt35510(0xBF);
    spi_send_cmd_nt35510(0xD50C);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD50D);spi_send_data_nt35510(0xD8);
    spi_send_cmd_nt35510(0xD50E);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD50F);spi_send_data_nt35510(0xFE);
    spi_send_cmd_nt35510(0xD510);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD511);spi_send_data_nt35510(0x1D);
    spi_send_cmd_nt35510(0xD512);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD513);spi_send_data_nt35510(0x4E);
    spi_send_cmd_nt35510(0xD514);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD515);spi_send_data_nt35510(0x73);
    spi_send_cmd_nt35510(0xD516);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD517);spi_send_data_nt35510(0xAD);
    spi_send_cmd_nt35510(0xD518);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD519);spi_send_data_nt35510(0xDC);
    spi_send_cmd_nt35510(0xD51A);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD51B);spi_send_data_nt35510(0xDD);
    spi_send_cmd_nt35510(0xD51C);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD51D);spi_send_data_nt35510(0x05);
    spi_send_cmd_nt35510(0xD51E);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD51F);spi_send_data_nt35510(0x2D);
    spi_send_cmd_nt35510(0xD520);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD521);spi_send_data_nt35510(0x43);
    spi_send_cmd_nt35510(0xD522);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD523);spi_send_data_nt35510(0x60);
    spi_send_cmd_nt35510(0xD524);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD525);spi_send_data_nt35510(0x79);
    spi_send_cmd_nt35510(0xD526);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD527);spi_send_data_nt35510(0xA5);
    spi_send_cmd_nt35510(0xD528);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD529);spi_send_data_nt35510(0xCE);
    spi_send_cmd_nt35510(0xD52A);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD52B);spi_send_data_nt35510(0x0F);
    spi_send_cmd_nt35510(0xD52C);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD52D);spi_send_data_nt35510(0x49);
    spi_send_cmd_nt35510(0xD52E);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD52F);spi_send_data_nt35510(0x83);
    spi_send_cmd_nt35510(0xD530);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD531);spi_send_data_nt35510(0xC7);
    spi_send_cmd_nt35510(0xD532);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD533);spi_send_data_nt35510(0xCC);
    //B-
    spi_send_cmd_nt35510(0xD600);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD601);spi_send_data_nt35510(0x5D);
    spi_send_cmd_nt35510(0xD602);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD603);spi_send_data_nt35510(0x69);
    spi_send_cmd_nt35510(0xD604);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD605);spi_send_data_nt35510(0x7F);
    spi_send_cmd_nt35510(0xD606);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD607);spi_send_data_nt35510(0x92);
    spi_send_cmd_nt35510(0xD608);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD609);spi_send_data_nt35510(0xA3);
    spi_send_cmd_nt35510(0xD60A);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD60B);spi_send_data_nt35510(0xBF);
    spi_send_cmd_nt35510(0xD60C);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD60D);spi_send_data_nt35510(0xD8);
    spi_send_cmd_nt35510(0xD60E);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0xD60F);spi_send_data_nt35510(0xFE);
    spi_send_cmd_nt35510(0xD610);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD611);spi_send_data_nt35510(0x1D);
    spi_send_cmd_nt35510(0xD612);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD613);spi_send_data_nt35510(0x4E);
    spi_send_cmd_nt35510(0xD614);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD615);spi_send_data_nt35510(0x73);
    spi_send_cmd_nt35510(0xD616);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD617);spi_send_data_nt35510(0xAD);
    spi_send_cmd_nt35510(0xD618);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD619);spi_send_data_nt35510(0xDC);
    spi_send_cmd_nt35510(0xD61A);spi_send_data_nt35510(0x01);
    spi_send_cmd_nt35510(0xD61B);spi_send_data_nt35510(0xDD);
    spi_send_cmd_nt35510(0xD61C);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD61D);spi_send_data_nt35510(0x05);
    spi_send_cmd_nt35510(0xD61E);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD61F);spi_send_data_nt35510(0x2D);
    spi_send_cmd_nt35510(0xD620);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD621);spi_send_data_nt35510(0x43);
    spi_send_cmd_nt35510(0xD622);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD623);spi_send_data_nt35510(0x60);
    spi_send_cmd_nt35510(0xD624);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD625);spi_send_data_nt35510(0x79);
    spi_send_cmd_nt35510(0xD626);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD627);spi_send_data_nt35510(0xA5);
    spi_send_cmd_nt35510(0xD628);spi_send_data_nt35510(0x02);
    spi_send_cmd_nt35510(0xD629);spi_send_data_nt35510(0xCE);
    spi_send_cmd_nt35510(0xD62A);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD62B);spi_send_data_nt35510(0x0F);
    spi_send_cmd_nt35510(0xD62C);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD62D);spi_send_data_nt35510(0x49);
    spi_send_cmd_nt35510(0xD62E);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD62F);spi_send_data_nt35510(0x83);
    spi_send_cmd_nt35510(0xD630);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD631);spi_send_data_nt35510(0xC7);
    spi_send_cmd_nt35510(0xD632);spi_send_data_nt35510(0x03);
    spi_send_cmd_nt35510(0xD633);spi_send_data_nt35510(0xCC);

    spi_send_cmd_nt35510(0xB000);spi_send_data_nt35510(0x12);
    spi_send_cmd_nt35510(0xB001);spi_send_data_nt35510(0x12);
    spi_send_cmd_nt35510(0xB002);spi_send_data_nt35510(0x12);

    spi_send_cmd_nt35510(0xB100);spi_send_data_nt35510(0x0A);
    spi_send_cmd_nt35510(0xB101);spi_send_data_nt35510(0x0A);
    spi_send_cmd_nt35510(0xB102);spi_send_data_nt35510(0x0A);

    spi_send_cmd_nt35510(0xBA00);spi_send_data_nt35510(0x24);
    spi_send_cmd_nt35510(0xBA01);spi_send_data_nt35510(0x24);
    spi_send_cmd_nt35510(0xBA02);spi_send_data_nt35510(0x24);

    spi_send_cmd_nt35510(0xB900);spi_send_data_nt35510(0x34);
    spi_send_cmd_nt35510(0xB901);spi_send_data_nt35510(0x34);
    spi_send_cmd_nt35510(0xB902);spi_send_data_nt35510(0x34);

    spi_send_cmd_nt35510(0xF000);spi_send_data_nt35510(0x55);
    spi_send_cmd_nt35510(0xF001);spi_send_data_nt35510(0xAA);
    spi_send_cmd_nt35510(0xF002);spi_send_data_nt35510(0x52);
    spi_send_cmd_nt35510(0xF003);spi_send_data_nt35510(0x08);
    spi_send_cmd_nt35510(0xF004);spi_send_data_nt35510(0x00);

    spi_send_cmd_nt35510(0xB100);spi_send_data_nt35510(0xCC);

    spi_send_cmd_nt35510(0xBC00);spi_send_data_nt35510(0x05);
    spi_send_cmd_nt35510(0xBC01);spi_send_data_nt35510(0x05);
    spi_send_cmd_nt35510(0xBC02);spi_send_data_nt35510(0x05);

    spi_send_cmd_nt35510(0xB800);spi_send_data_nt35510(0x01);

    spi_send_cmd_nt35510(0xB700);spi_send_data_nt35510(0x55);
    spi_send_cmd_nt35510(0xB701);spi_send_data_nt35510(0x55);

    spi_send_cmd_nt35510(0xBD02);spi_send_data_nt35510(0x07);
    spi_send_cmd_nt35510(0xBD03);spi_send_data_nt35510(0x31);
    spi_send_cmd_nt35510(0xBE02);spi_send_data_nt35510(0x07);
    spi_send_cmd_nt35510(0xBE03);spi_send_data_nt35510(0x31);
    spi_send_cmd_nt35510(0xBF02);spi_send_data_nt35510(0x07);
    spi_send_cmd_nt35510(0xBF03);spi_send_data_nt35510(0x31);

    spi_send_cmd_nt35510(0xFF00);spi_send_data_nt35510(0xAA);
    spi_send_cmd_nt35510(0xFF01);spi_send_data_nt35510(0x55);
    spi_send_cmd_nt35510(0xFF02);spi_send_data_nt35510(0x25);
    spi_send_cmd_nt35510(0xFF03);spi_send_data_nt35510(0x01);

    //spi_send_cmd_nt35510(0xF304);spi_send_data_nt35510(0x11);
    //spi_send_cmd_nt35510(0xF306);spi_send_data_nt35510(0x10);
    //spi_send_cmd_nt35510(0xF408);spi_send_data_nt35510(0x00);

    spi_send_cmd_nt35510(0x3500);spi_send_data_nt35510(0x00);
    spi_send_cmd_nt35510(0x3A00);spi_send_data_nt35510(0x50);

    //SLEEP OUT
    spi_send_cmd_nt35510(0x1100);
    udelay(120000);

    //DISPLY ON
    spi_send_cmd_nt35510(0x2900);
    udelay(120000);

}
#endif

static void power_lcd_backlight(int level)
{
    DEBUG_INFO("Entering %s\n",__FUNCTION__);

    switch (level) {
        case LCD_OFF:
#if defined(CONFIG_BACKLIGHT_CAT3648) || defined(CONFIG_BACKLIGHT_CAT3637)||defined(CONFIG_BACKLIGHT_TWL5030_PWM)
            backlight_off();
#endif
            break;
        default:
#if defined(CONFIG_BACKLIGHT_CAT3648) || defined(CONFIG_BACKLIGHT_CAT3637)||defined(CONFIG_BACKLIGHT_TWL5030_PWM)
            backlight_on();
#endif
            break;
    }
    DEBUG_INFO("Entering %s\n",__FUNCTION__);
}


void enable_backlight(void)
{
    /* If already enabled, return*/
    static int  lcd_in_use = 0;
    if (lcd_in_use)
        return;
    power_lcd_backlight(LCD_ON);
    lcd_in_use = 1;
}

u16 test_lcd[LCD_HEIGH][LCD_WIDTH];

static void lcd_reset(void)
{
#if defined(CONFIG_3630KUNLUN_WUDANG)

    gpio_t *gpio6_base = (gpio_t *)OMAP34XX_GPIO6_BASE;

    //set GPIO176 as output pin
    /*set GPIO176 high level,LCD_RST inactive */
    set_gpio_dataout(176,1);
    udelay(1000);
    /*set GPIO176 low level,LCD_RST active */
    set_gpio_dataout(176,0);
    /*wait for 20ms*/
    udelay(20000);
    /*set GPIO176 high level,LCD_RST inactive */
    set_gpio_dataout(176,1);
    udelay(50000);
    DEBUG_INFO("LCD_RST signal is expected to hold low for 20ms \n");
#else
    gpio_t *gpio3_base = (gpio_t *)OMAP34XX_GPIO3_BASE;
    //set GPIO93 as output pin
    /*set GPIO93 high level,LCD_RST inactive */
    set_gpio_dataout(93,1);
    udelay(1000);
    /*set GPIO93 low level,LCD_RST active */
    set_gpio_dataout(93,0);
    /*wait for 20ms*/
    udelay(20000);
    /*set GPIO93 high level,LCD_RST inactive */
    set_gpio_dataout(93,1);
    udelay(50000);
    DEBUG_INFO("LCD_RST signal is expected to hold low for 20ms \n");
#endif
}

/*configure LCD controller & panel before displaying*/
void kunlun_lcd_init(void)
{
    u32 utemp = 0;

    gpio_enable(3);
    gpio_enable(5);
#if defined(CONFIG_3630KUNLUN_WUDANG)
	gpio_enable(6);
#endif
    omap3_lcd_poweron();
    omap34xx_dss_clocks_on();
    omap3_disp_config_dss();
    omap3_disp_config_dispc(0);
    omap3_disp_config_gfxdma((void*)test_lcd);
#if defined(CONFIG_3630KUNLUN_WUDANG)
    omap3_mcspi_setup_transfer(NT35510_SPI_CHANEL,16,50000);
#else
    omap3_mcspi_setup_transfer(1,8,50000);
#endif
    lcd_reset();

    omap3_disp_gfxdma_enable();
    omap3_disp_dispc_enable();

    initialise_lcd_panael();

    enable_backlight();
#ifdef CONFIG_LCD_ILI9327_DEBUG
    omap3_dss_print_clocks();
#endif
}

#if 0
/*display logo picture*/
void kunlun_lcd_display_logo(void)
{
    int i,j;
    int bmp_top = 0,bmp_left = 0;
    int bmp_pos = 0;
    u16 pixel = 0;
    /*display logo on the center area of panel*/
    bmp_left = (LCD_WIDTH - LOGO_WIDTH) / 2;
    bmp_top = (LCD_HEIGH - LOGO_HEIGH) / 2;
    for(j = 0; j < LCD_HEIGH;j++)
        for(i = 0; i < LCD_WIDTH;i++){
            if(i >= bmp_left && i < bmp_left + LOGO_WIDTH &&
               j >= bmp_top && j < bmp_top + LOGO_HEIGH){
                pixel = *(logo_picture + bmp_pos++);
                test_lcd[j][i] = pixel;
            }else{
                test_lcd[j][i] = LOGO_BACK_CLOLR;
            }
        }

}
#endif

/*display logo picture*/
void kunlun_lcd_display_logo(void)
{
    int i,j;
    int bmp_top = 0,bmp_left = 0;
    int bmp_pos = 0;
    u16 pixel = 0;
    /*display logo on the center area of panel*/
    bmp_left = (LCD_WIDTH - BMP_LOGO_WIDTH) / 2;
    bmp_top = (LCD_HEIGH - BMP_LOGO_HEIGHT) / 2;
    omap3_disp_dispc_enable();
    for(j = 0; j < LCD_HEIGH;j++)
        for(i = 0; i < LCD_WIDTH;i++){
            if(i >= bmp_left && i < bmp_left + BMP_LOGO_WIDTH &&
               j >= bmp_top && j < bmp_top + BMP_LOGO_HEIGHT){
                if(BMP_LOGO_BPP == 8){
			pixel = bmp_logo_palette[bmp_logo_bitmap[bmp_pos++] - BMP_LOGO_OFFSET];
                   test_lcd[j][i] = pixel;
                }else{
                    test_lcd[j][i] = bmp_logo_bitmap[bmp_pos++];
                }
            }else{
                /*use the first pixel as default background color*/
                if(BMP_LOGO_BPP == 8){
                    test_lcd[j][i] = bmp_logo_palette[bmp_logo_bitmap[0] ];
                }else{
                    test_lcd[j][i] = bmp_logo_bitmap[0];
                }
            }
        }

    omap3_disp_dispc_disable();
        
}


/*draw characters at position of (x,y) screen */
void omap3_lcd_drawchars (u16 x, u16 y, u8 *str, int count)
{
    u16 *dest;
    int row;

    if(x + count * VIDEO_FONT_WIDTH > LCD_WIDTH
        || y + VIDEO_FONT_HEIGHT > LCD_HEIGH){
        printf("out of screen\n");
        return;
    }

    omap3_disp_dispc_enable();

    dest = &test_lcd[y][x];

	for (row=0;  row < VIDEO_FONT_HEIGHT;  ++row, dest += LCD_WIDTH)  {
		u8 *s = str;
		u16 *d = dest;
		int i;

		for (i=0; i<count; ++i) {
			unsigned char c, bits;

			c = *s++;
			bits = video_fontdata[c * VIDEO_FONT_HEIGHT + row];

			for (c=0; c<8; ++c) {
				*d++ = (bits & 0x80) ?
						LCD_COLOR_FG : LCD_COLOR_BG;
				bits <<= 1;
			}
		}
	}

    omap3_disp_dispc_disable();
}

/*clear screen*/
void omap3_lcd_clear (void)
{
    omap3_disp_dispc_enable();
    memset(test_lcd,0xFF,sizeof(test_lcd));
    omap3_disp_dispc_disable();
}
void omap3_lcd_color (u8 color)
{
    omap3_disp_dispc_enable();
    memset(test_lcd,color,sizeof(test_lcd));
    omap3_disp_dispc_disable();
}
static int do_lcdtest ( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int i = 0;
    int j = 0;
    u16 cor = 0;
    if(argc > 3){
        printf("usage:lcd <poweron/reset/initdpi/initlcd/red/green/blue/logo>\n");
        return -1;
    }
    if(strncmp(argv[1],"poweron",sizeof("poweron")) == 0){
        gpio_enable(3);
        gpio_enable(5);
        omap3_lcd_poweron();
        DEBUG_INFO("VPLL2 output is expected to 1.8V\n");
    }else if(strncmp(argv[1],"reset",sizeof("reset")) == 0){
        lcd_reset();
    }else if(strncmp(argv[1],"initdpi",7) == 0){
        DEBUG_INFO("dss init\n");
        omap34xx_dss_clocks_on();
        omap3_disp_config_dss();
        omap3_disp_config_dispc(0);
        omap3_disp_config_gfxdma((void*)test_lcd);
        omap3_disp_gfxdma_enable();
        omap3_disp_dispc_enable();
#ifdef CONFIG_LCD_ILI9327_DEBUG
        omap3_dss_print_clocks();
#endif
    }else if(strncmp(argv[1],"initlcd",7) == 0){
#if defined(CONFIG_3630KUNLUN_WUDANG)
        omap3_mcspi_setup_transfer(NT35510_SPI_CHANEL,16,50000);
#else
        omap3_mcspi_setup_transfer(1,8,50000);
#endif
        initialise_lcd_panael();
    }else if(strncmp(argv[1],"red",3) == 0){
        omap3_disp_dispc_enable();
        for(i = 0; i < LCD_WIDTH * LCD_HEIGH; i++){
            *((u16*)test_lcd + i) = (0x1F << 11);
        }
        omap3_disp_dispc_disable();
    }else if(strncmp(argv[1],"green",5) == 0){
        omap3_disp_dispc_enable();
        for(i = 0; i < LCD_WIDTH * LCD_HEIGH; i++){
            *((u16*)test_lcd + i) = (0x3F << 5);
        }
        omap3_disp_dispc_disable();
    }else if(strncmp(argv[1],"blue",4) == 0){
        omap3_disp_dispc_enable();
        for(i = 0; i < LCD_WIDTH * LCD_HEIGH; i++){
            *((u16*)test_lcd + i) = 0x1F;
        } 
        omap3_disp_dispc_disable();
    }else if(strncmp(argv[1],"black",3) == 0){
        omap3_disp_dispc_enable();
        for(i = 0; i < LCD_WIDTH * LCD_HEIGH; i++){
            *((u16*)test_lcd + i) = 0;
        }
        omap3_disp_dispc_disable();
    }else if(strncmp(argv[1],"white",3) == 0){
        omap3_disp_dispc_enable();
        for(i = 0; i < LCD_WIDTH * LCD_HEIGH; i++){
            *((u16*)test_lcd + i) = 0xFFFF;
        }
        omap3_disp_dispc_disable();
    }else if(strncmp(argv[1],"frame_blue",10) == 0){

        omap3_disp_dispc_enable();
        /*white screen*/
        memset((void*)test_lcd,0xFF,sizeof(test_lcd));
        for(i = 0;i < LCD_HEIGH;i++){
            for(j = 0;j < LCD_WIDTH;j++){
                if(i == 0 || i == LCD_HEIGH - 1 ||
                    j == 0 || j == LCD_WIDTH - 1){
                    *((u16*)test_lcd + i * LCD_WIDTH + j) = 0x1F;
                }
            }
        }
        omap3_disp_dispc_disable();
    }else if(strncmp(argv[1],"frame_red",9) == 0){
        omap3_disp_dispc_enable();
        /*white screen*/
        memset((void*)test_lcd,0xFF,sizeof(test_lcd));
        for(i = 0;i < LCD_HEIGH;i++){
            for(j = 0;j < LCD_WIDTH;j++){
                if(i == 0 || i == LCD_HEIGH - 1 ||
                    j == 0 || j == LCD_WIDTH - 1){
                    *((u16*)test_lcd + i * LCD_WIDTH + j) = 0x1F << 11;
                }
            }
        }
        omap3_disp_dispc_disable();
    }else if(strncmp(argv[1],"logo",2) == 0){
        kunlun_lcd_display_logo();
    }else if(strncmp(argv[1],"val",3) == 0){
        if(argv[2])
        {
            omap3_disp_dispc_enable();
            cor = (u16)simple_strtoul(argv[2], NULL, 16);

            for(i = 0; i < LCD_WIDTH * LCD_HEIGH; i++){
                *((u16*)test_lcd + i) = cor;
            }
            omap3_disp_dispc_disable();

            printf("lcd test color value:0x%08x\n", cor);
        }
        else
        {
            printf("please enter value with value like 0x3F.\n");
        }
    }else if(strncmp(argv[1],"bit",3) == 0){
        if(argv[2])
        {
            omap3_disp_dispc_enable();
            cor = (u16)simple_strtoul(argv[2], NULL, 10);

            for(i = 0; i < LCD_WIDTH * LCD_HEIGH; i++){
                *((u16*)test_lcd + i) = (1 << cor);
            }
            omap3_disp_dispc_disable();
            printf("lcd test color value:0x%08x\n", 1 << cor);
        }
        else
        {
            printf("please enter value with value 0 - 15.\n");
        }
    }else if(strncmp(argv[1],"test",4) == 0){
        omap3_disp_dispc_enable();
        for(i = 0; i < LCD_HEIGH; i++)
        {
            for(j = 0; j < LCD_WIDTH; j++)
            {
                if(i > LCD_HEIGH / 2)
                {
                    if(j < LCD_WIDTH / 2)
                    {
                        *((u16*)test_lcd + i * LCD_WIDTH + j) = 0x1F << 11;
                    }
                    else
                    {
                        *((u16*)test_lcd + i * LCD_WIDTH + j) = 0x00;
                    }
                }
                else
                {
                    if(j < LCD_WIDTH / 2)
                    {
                        *((u16*)test_lcd + i * LCD_WIDTH + j) = 0x1F;
                    }
                    else
                    {
                        *((u16*)test_lcd + i * LCD_WIDTH + j) = 0xFFFF;
                    }
                }
            }
        }
        omap3_disp_dispc_disable();
    }else if(strncmp(argv[1],"spi",3) == 0){
        spi_send_cmd(0xB4);
        spi_send_data(0x11);
    }else{
        kunlun_lcd_init();
        kunlun_lcd_display_logo();
    }
    return 0;

}


/************************************************************************/
/************************************************************************/
U_BOOT_CMD(
        lcd,	CFG_MAXARGS,	1,	do_lcdtest,
        "lcd - test lcd\n",
        "[<option>]\n"
        "    - with <option> argument: lcd <option> on TFT LCD\n"
        "    - without arguments: display logo\n"
        "<option>:\n"
        "    ---poweron\n"
        "    ---initdpi\n"
        "    ---reset\n"
        "    ---initlcd\n"
        "    ---red/blue/green\n"
        "    ---logo\n"
        "    ---spi\n"
        "    ---test\n"
        "    ---val [0x00 - 0xFFFF]\n"
        "    ---bit [0 - 15]\n"
        );

#endif
