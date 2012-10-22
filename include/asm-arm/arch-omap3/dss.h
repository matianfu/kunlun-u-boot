/*
 * =====================================================================================
 *
 *       Filename:  dss.h
 *
 *    Description:  omap3630 display subsystem
 *
 *        Version:  1.0
 *        Created:  04/21/2010 05:34:22 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Daqing Li (daqli), daqli@via-telecom.com
 *        Company:  VIA TELECOM
 *
 * =====================================================================================
 */
#include <config.h>


#define DSS_REVISION            0x000
#define DSS_SYSCONFIG           0x010
#define DSS_SYSSTATUS           0x014
#define DSS_CONTROL         	0x040


/* physical memory map definitions */
    /*  display subsystem */
#define DSS_REG_BASE            0x48050000
#define DSS_REG_SIZE            0x00001000
    /*  DSS */
#define DSS_REG_OFFSET          0x00000000
    /*  display controller */
#define DISPC_REG_OFFSET        0x00000400
    /*  remote framebuffer interface */
#define RFBI_REG_OFFSET         0x00000800
    /*  video encoder */
#define VENC_REG_OFFSET         0x00000C00

// DSI
#define DSI_REG_BASE            0x4804FC00
#define DSI_REG_OFFSET		0x00000000
#define DSI_CTRL                0x0040


/*  display controller register offsets */
#define DISPC_REVISION          0x000
#define DISPC_SYSCONFIG         0x010
#define DISPC_SYSSTATUS         0x014
#define DISPC_IRQSTATUS         0x018
#define DISPC_IRQENABLE         0x01C
#define DISPC_CONTROL           0x040
#define DISPC_CONFIG            0x044
#define DISPC_CAPABLE           0x048
#define DISPC_DEFAULT_COLOR0    0x04C
#define DISPC_DEFAULT_COLOR1    0x050
#define DISPC_TRANS_COLOR0      0x054
#define DISPC_TRANS_COLOR1      0x058
#define DISPC_LINE_STATUS       0x05C
#define DISPC_LINE_NUMBER       0x060
#define DISPC_TIMING_H          0x064
#define DISPC_TIMING_V          0x068
#define DISPC_POL_FREQ          0x06C
#define DISPC_DIVISOR           0x070
#define DISPC_GLOBAL_ALPHA      0x074
#define DISPC_SIZE_DIG          0x078
#define DISPC_SIZE_LCD          0x07C
#define DISPC_GFX_BA0           	0x080
#define DISPC_GFX_BA1           	0x084
#define DISPC_GFX_POSITION      	0x088
#define DISPC_GFX_SIZE          	0x08C
#define DISPC_GFX_ATTRIBUTES        	0x0A0
#define DISPC_GFX_FIFO_THRESHOLD    	0x0A4
#define DISPC_GFX_FIFO_SIZE     	0x0A8
#define DISPC_GFX_ROW_INC       	0x0AC
#define DISPC_GFX_PIXEL_INC     	0x0B0
#define DISPC_GFX_WINDOW_SKIP       	0x0B4
#define DISPC_GFX_TABLE_BA      	0x0B8

// DISPC_POL_FREQ
#define IVS     (1 << 12)
#define IHS     (1 << 13)
#define IPC     (1 << 14)
#define IEO     (1 << 15)
#define RF      (1 << 16)

#if defined(CONFIG_LCD_ILI9327)
#define LCD_HEIGH   400
#define LCD_WIDTH   240
#define HSW         (10)
#define HFP         (10)
#define HBP         (20)
#define VSW         (2)
#define VFP         (4)
#define VBP         (2)
#define POL_FREQ_VALUE  (IVS | IHS | IPC)

#elif defined(CONFIG_LCD_RM68041)
#define LCD_HEIGH   480
#define LCD_WIDTH   320
#define HSW         (10)
#define HFP         (10)
#define HBP         (20)
#define VSW         (2)
#define VFP         (8)
#define VBP         (8)
#define POL_FREQ_VALUE  (IVS | IHS | RF)

/* Haire N710E. */
#elif defined(CONFIG_LCD_RM68041_N710E)
#define LCD_HEIGH   480
#define LCD_WIDTH   320
#define HSW         (10)
#define HFP         (10)
#define HBP         (20)
#define VSW         (2)
#define VFP         (8)
#define VBP         (8)
#define POL_FREQ_VALUE  (IVS | IHS | IPC)

#elif defined(CONFIG_LCD_R61529)
#define LCD_HEIGH   480
#define LCD_WIDTH   320
#define HSW         (10)
#define HFP         (200)
#define HBP         (10)
#define VSW         (2)
#define VFP         (4)
#define VBP         (2)
#define POL_FREQ_VALUE  (IVS | IHS | IEO | RF)

#elif defined(CONFIG_LCD_NT35510)
#define LCD_HEIGH   800
#define LCD_WIDTH   480
#define HSW         (10)
#define HFP         (10)
#define HBP         (20)
#define VSW         (2)
#define VFP         (10)
#define VBP         (5)
#define POL_FREQ_VALUE  (IVS | IHS | IPC)
#endif

/* vibrate */
/* moved to misc.h **/
// void vib_ctrl(int value);

/* initialize lcd*/
void kunlun_lcd_init(void);

/* display logo*/
void kunlun_lcd_display_logo(void);

/*draw characters at position of (x,y) screen */
void omap3_lcd_drawchars(u16 x, u16 y, u8 *str, int count);

/*clear screen*/
void omap3_lcd_clear (void);
