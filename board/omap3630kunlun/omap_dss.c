/*
 * =====================================================================================
 *
 *       Filename:  lcd.c
 *
 *    Description:  OMAP3630 Display  subsystem
 *
 *        Version:  1.0
 *        Created:  2009
 *
 *         Author:  Daqing Li (daqli), daqli@via-telecom.com
 *        Company:  VIA TELECOM
 *
 * =====================================================================================
 */

#include <config.h>


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
#include <asm/arch/mux.h>

#include "lg_lcd.h"

#define LCD_CLOCK_ALWON 1
#define DEBUG_INFO(fmt,args...)  printf(fmt,##args)

/** timings for kindle fire **/
#if 0
static struct omap_video_timings otter1_panel_timings = {
	.x_res          = 1024,
	.y_res          = 600,
	.pixel_clock    = 46000, /* in kHz */
	.hfp            = 160,   /* HFP fix 160 */
	.hsw            = 10,    /* HSW = 1~140 */
	.hbp            = 150,   /* HSW + HBP = 160 */
	.vfp            = 12,    /* VFP fix 12 */
	.vsw            = 3,     /* VSW = 1~20 */
	.vbp            = 20,    /* VSW + VBP = 23 */
};
#endif

/** define LCD width and height **/
#ifdef	LCD_WIDTH
#undef	LCD_WIDTH
#endif
#define	LCD_WIDTH	(1024)

#ifdef	LCD_HEIGH
#undef	LCD_HEIGH	
#endif
#define	LCD_HEIGH	(600)
/*
   timings.hsw = HSW;
   timings.hfp = HFP;
   timings.hbp = HBP;
   timings.vsw = VSW;
   timings.vfp = VFP;
   timings.vbp = VBP;
 */

#ifdef HSW
#undef HSW
#endif
#define HSW	(10)

#ifdef HFP
#undef HFP
#endif
#define HFP	(160)

#ifdef HBP
#undef HBP
#endif
#define HBP	(150) /** mismatch with datasheet, 160 **/

#ifdef VSW
#undef VSW
#endif
#define VSW	(3)

#ifdef VFP
#undef VFP
#endif
#define VFP	(12)

#ifdef VBP
#undef VBP
#endif
#define VBP	(20) /** mismatch with datasheet, 23 **/

#define LCD_ON 1
#define LCD_OFF 0

#define FLD_MASK(start, end)	(((1 << (start - end + 1)) - 1) << (end))
#define FLD_VAL(val, start, end) (((val) << end) & FLD_MASK(start, end))

#define LCD_COLOR_FG    0x001F
#define LCD_COLOR_BG    0xFFFF

#define LCD_COR_RED     (0x1F << 11)
#define LCD_COR_GREEN   (0x3F << 5)
#define LCD_COR_BLUE    (0x1F)
#define LCD_COR_WHITE   (0xFFFF)
#define LCD_COR_BLACK   (0x0000);

static void omap34xx_dss_clocks_on(void);
static void omap3_disp_config_dss(void);
static void omap3_disp_config_dispc(int is_rfbi);

static void omap3_lcd_poweron(void){}

/** this two function are needed from outside (nand, fastboot) 
  recover them after test **/
void kunlun_lcd_display_logo(void){}
void omap3_lcd_drawchars (u16 x, u16 y, u8 *str, int count){}


static void initialise_lcd_panael(void) {

	lg_3wire_init(
			CONTROL_PADCONF_McSPI1_CS0, 174,
			CONTROL_PADCONF_McSPI1_CLK, 171,
			CONTROL_PADCONF_McSPI1_SIMO, 172,
			0, 0);

	lg_3wire_init_regs(1); // normal white

}

extern void twl5030_pwm1_init(void);

static void enable_backlight(void) {

	twl5030_pwm1_init();
}


/*
 *	timings 
 */
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
 * dsi register access
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
 * setup golcd bit in DISPC_CONTROL register
 */
static void omap3_disp_golcd(void)
{
	u32 dispc_ctrl;
	dispc_ctrl = dispc_reg_in(DISPC_CONTROL);
	dispc_ctrl |= 1 << 5;
	dispc_reg_out(DISPC_CONTROL,dispc_ctrl);
}

/* 
 * configure graphics DMA
 * 
 * DISPC_GFX_ATTRIBUTES - pixel format
 * DISPC_GFX_BA0
 * DISPC_GFX_POSITION
 * DISPC_GRX_SIZE
 */
static int omap3_disp_config_gfxdma(void *base)
{
	u32 gfx_attr = dispc_reg_in(DISPC_GFX_ATTRIBUTES);

	// set frame buffer base
	dispc_reg_out(DISPC_GFX_BA0,(u32)base);

	// set gfx position, 0, 0
	dispc_reg_out(DISPC_GFX_POSITION,0);

	//size of GFX window; value should minus 1, see datasheet
	dispc_reg_out(DISPC_GFX_SIZE,(LCD_WIDTH - 1) | (LCD_HEIGH - 1) << 16);

	DEBUG_INFO("GFX_FIFO_THRESHOLD = %#08x\n",dispc_reg_in(DISPC_GFX_FIFO_THRESHOLD));
	DEBUG_INFO("GFX_FIFO_SIZE_STATUS = %#08x\n",dispc_reg_in(DISPC_GFX_FIFO_SIZE));
	DEBUG_INFO("GFX_ROW_INC = %#08x\n",dispc_reg_in(DISPC_GFX_ROW_INC));
	DEBUG_INFO("GFX_PIXEL_INC = %#08x\n",dispc_reg_in(DISPC_GFX_PIXEL_INC));
	DEBUG_INFO("GFX_ATTRIBUTES = %#08x\n",gfx_attr);

	// GFX format:RGB16
	// gfx_attr |= 0x6 << 1;

	// change to RGB 24 unpacked
	gfx_attr &= ~(0x1F << 1);	/** clear 5 bits **/
	gfx_attr |= 0x8 << 1;		/** set to 0x8, RGB 24 unpacked format, _RGB **/
	dispc_reg_out(DISPC_GFX_ATTRIBUTES,gfx_attr);

	// taking effect
	omap3_disp_golcd();
	return 0;
}

/* 
 * enable DISPC and DMA output
 */
static void omap3_disp_dispc_enable(void)
{
	dispc_reg_merge(DISPC_CONTROL,0x1,0x1);
}


/* 
 * enable  Graphics DMA output
 */
static void omap3_disp_gfxdma_enable(void)
{
	dispc_reg_merge(DISPC_GFX_ATTRIBUTES,0x1,0x1);
	omap3_disp_golcd();
}


/* 
 * enable  Graphics DMA output
 */
static void omap3_disp_dispc_disable(void)
{
	dispc_reg_merge(DISPC_CONTROL,0x0,0x1);
}

/* display ALL clocks frequency related DSS*/
static void omap3_dss_print_clocks(void)
{
	u32 sys_clk = 0;
	u32 osc_sys_clk = 26000000;
	u32 dpll4_alwon_fclk = 0;
	u32 dss1_alwon_fclk = 0;
	u32 cm_clksel_pll = 0;
	u32 lcd, pcd, dispc_divisor, dispc_fclk, dispc_pclk = 0;
	u32 m, n, clkout_m4x2;

	printf("omap3_dss_print_clocks ------------------------------begin-----\n");
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
	m = ((cm_clksel_pll >> 8)&0xFFF);
	n = (cm_clksel_pll & 0x7F);
	printf("m:%d, n:%d\n",m, n);

	/**********************************************************************

	see TRM pg308

	The formula is 

	CLKOUTX2 = (Fref x 2 x M)/(N+1)
	CLKOUT = CLKOUTX2/2

	In our case, the Fref is dpll4_alwon_fclk, and 
	dss1_alwon_fclk is provided by CLKOUT_M4X2

	**********************************************************************/

	// dss1_alwon_fclk = dpll4_alwon_fclk / ((cm_clksel_pll & 0x7F) + 1) * ((cm_clksel_pll >> 8) & 0xFFF) * 2;
	clkout_m4x2 = dpll4_alwon_fclk / (n + 1) * m * 2;
	printf("clkout_m4x2 is %d Hz\n", clkout_m4x2);

	dss1_alwon_fclk = clkout_m4x2/(__raw_readl(CM_CLKSEL_DSS) & 0x1F);
	printf("dss1_alwon_fclk is %d Hz\n",dss1_alwon_fclk);

	printf("the source of DISPC_FCLK is ");
	if(dss_reg_in(DSS_CONTROL) & 0x1){
		printf("DSI1_PLL_FCLK\n");
		return;
	}
	else{
		printf("DSS1_ALWON_FCLK\n");
	}

	/** set Figure 7-16 on page 1736, TRM **/
	dispc_divisor = dispc_reg_in(DISPC_DIVISOR);
	lcd = (dispc_divisor >> 16) & 0xFF;
	pcd = dispc_divisor & 0xFF;

	dispc_fclk = dss1_alwon_fclk / lcd;
	printf("lcd divisor is %d, dispc_fclk is %dHz.\n", lcd, dispc_fclk);

	dispc_pclk = dispc_fclk / pcd;
	printf("pcd divisor is %d, dispc_pclk (pixel clock) is %dHz.\n", pcd, dispc_pclk);

	printf("DISPC_CONTROL register = %#08x\n",dispc_reg_in(DISPC_CONTROL));
	printf("omap3_dss_print_clocks ------------------------------end-------\n");
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

	// clear bit 29, power up DPLL4_M4_CLK HSDIVIDER path (DSS1)
	// clear bit 28, power up DPLL4_M3_CLK HSDIVIDER path (TV)
	clken_pll &= ~(1 << 29 | 1 << 28);
	// set bit 16~18, EN_PERIPH_DPLL=7, enables the DPLL4 in lock mode
	// set bit 29, EN_PERIPH_DPLL_DRIFTGUARD=1, enables DPLL4 automatic calibration
	clken_pll |= 0x7 << 16 | 0x1 << 19;
	__raw_writel(clken_pll,CM_CLKEN_PLL);

	// clear bit 3~5, AUTO_PERIPH_DPLL=0, disable DPLL4 auto control
	autoidle_pll &= ~(0x7 << 3);
	__raw_writel(autoidle_pll,CM_AUTOIDLE_PLL);

	//DSS1_ALWON_FCLK = DPLL4/9,DSS_TV_FCLK = DPLL4/16
#if 0
#if defined(CONFIG_LCD_NT35510)
	//output pclk=26mhz
	clksel_dss &= ~0x0000003f;
	clksel_dss |= 0x00000005;
	__raw_writel(clksel_dss,CM_CLKSEL_DSS);
#else
	clksel_dss |= 0x00001009;
	__raw_writel(clksel_dss,CM_CLKSEL_DSS);
#endif
#endif
	/** clear CLKSEL_DSS1 **/
	clksel_dss &= ~0x3f;	

	/** divider set to 9, 1728/9 = 192MHz */
	clksel_dss |= 0x09;

	/** set CLKSEL_DSS1 **/
	__raw_writel(clksel_dss, CM_CLKSEL_DSS);

	/** DSS_L3/L4_ICLK are enabled **/
	__raw_writel(0x1,CM_ICLKEN_DSS);

	/** DSS_ICLK is unrelated to the domain state transition **/
	__raw_writel(0x0,CM_AUTOIDLE_DSS);

	/** enable DSS_TV_FCLK,DSS1/2_ALWON_FCLK **/
	fclken_dss |= 0x1 | 0x1 << 1 | 0x1 << 2;
	__raw_writel(fclken_dss,CM_FCLKEN_DSS);

	DEBUG_INFO("%s:exit\n",__FUNCTION__,clksel_dss);
}

/*
 *	Description:set DSS global registers
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



/** Description:set display controller **/
static void omap3_disp_config_dispc(int is_rfbi)
{

	u32 dispc_sysc;
	u32 temp;
	u32 pol_freq_value;
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

#if 0
#if defined(CONFIG_LCD_ILI9327)
	/* Set logic clock to fck, pixel clock to fck/2 for now */
	dispc_reg_out(DISPC_DIVISOR, 2  << 16 | 4  << 0);
#elif defined(CONFIG_LCD_R61529) || defined(CONFIG_LCD_RM68041) \
	|| defined(CONFIG_LCD_RM68041_N710E)
	dispc_reg_out(DISPC_DIVISOR, 1  << 16 | 6  << 0);
#else
	dispc_reg_out(DISPC_DIVISOR, 2  << 16 | 4  << 0);
#endif
#endif

	/** divider for logic clock and pixel clock **/
	/** 192 / 2 = 96MHz (logic) and 192 / 2 / 2 = 48MHz (pixel) **/
	dispc_reg_out(DISPC_DIVISOR, (2 << 16) | (2 << 0));

	/*VSYNC,HSYNC are low active,data are driven on data lines on falling edge of pixel clock*/
	// dispc_reg_out(DISPC_POL_FREQ, POL_FREQ_VALUE);
	pol_freq_value = IHS | IVS;
	dispc_reg_out(DISPC_POL_FREQ, pol_freq_value);

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
		temp |=  1 << 15 | 1 << 16 |1 << 3 | 1 << 8 | 1 << 9; /** bit 8/9 0x03 -> 24bit line */

	dispc_reg_out(DISPC_CONTROL,temp);
	omap3_disp_golcd();

	DEBUG_INFO("%s exit!\n",__FUNCTION__);

}


u32 test_lcd[LCD_HEIGH][LCD_WIDTH];

static void init_buffer() {
	int y, x;

	for (y = 0; y < LCD_HEIGH; y++) {
		for (x = 0; x < LCD_WIDTH; x++) {
			if (y <200) {
				test_lcd[y][x] = 0x00FF0000;
			}
			else if (y < 400) {
				test_lcd[y][x] = 0x0000FF00;
			}
			else {
				test_lcd[y][x] = 0x000000FF;
			}
		}
	}
}

/*configure LCD controller & panel before displaying*/
void kunlun_lcd_init(void) {

	/** init display data **/
	init_buffer();	

	/** step 1: lcd power on, board specific **/
	omap3_lcd_poweron();

	/** 3-wire initialise panel, and don't forget re-mux dss_acbias **/
	initialise_lcd_panael();

	udelay(5000);

	// This cannot be skipped, DE is neccesary for LVDS
	// MUX_VAL(CP(DSS_ACBIAS),     (IEN  | PTD | DIS | M4)) /*DSS_ACBIAS, GPIO_69 */
	__raw_writew((IEN | PTD | DIS | M0), OMAP34XX_CTRL_BASE + CONTROL_PADCONF_DSS_ACBIAS);
	udelay(5000);

	/** dss clocks, clkset seems to be uncertain **/
	omap34xx_dss_clocks_on();

	/** not carefully examined **/
	omap3_disp_config_dss();

	/** config timings, active high/low, rising/falling edge trigger **/
	omap3_disp_config_dispc(0);

	/** config frame buffer **/
	omap3_disp_config_gfxdma((void*)test_lcd);

	// no reset for lg lcd
	// lcd_reset();

	omap3_disp_gfxdma_enable();
	omap3_disp_dispc_enable();

	// initialise_lcd_panael();
	udelay(500000);

	enable_backlight();

	// print clock info
	omap3_dss_print_clocks();
}

// not used for now

#if 0
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
#endif

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

#if 0
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
