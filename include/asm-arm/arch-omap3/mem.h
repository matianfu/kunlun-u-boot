/*
 * (C) Copyright 2006
 * Texas Instruments, <www.ti.com>
 * Richard Woodruff <r-woodruff2@ti.com>
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

#ifndef _OMAP34XX_MEM_H_
#define _OMAP34XX_MEM_H_

#define SDRC_CS0_OSET    0x0
#define SDRC_CS1_OSET    0x30  /* mirror CS1 regs appear offset 0x30 from CS0 */

#ifndef __ASSEMBLY__

typedef enum {
	STACKED		= 0,
	IP_DDR		= 1,
	COMBO_DDR	= 2,
	IP_SDR	 	= 3,
} mem_t;

#endif

/* set the 343x-SDRC incoming address convention */
#if defined(SDRC_B_R_C)
#define B_ALL	(0 << 6)	/* bank-row-column */
#elif defined(SDRC_B1_R_B0_C)
#define B_ALL	(1 << 6)	/* bank1-row-bank0-column */
#elif defined(SDRC_R_B_C)
#define B_ALL	(2 << 6)	/* row-bank-column */
#endif

/* Future memory combinations based on past */
#define SDP_SDRC_MDCFG_MONO_DDR    0x0
#define SDP_COMBO_MDCFG_0_DDR      0x0
#define SDP_SDRC_MDCFG_0_SDR       0x0

/* Slower full frequency range default timings for x32 operation*/
#define SDP_SDRC_SHARING		0x00000100
#define SDP_SDRC_MR_0_SDR		0x00000031

#ifdef CONFIG_3430ZEBU
#define SDP_SDRC_MDCFG_0_DDR	(0x02582019|B_ALL) /* Infin ddr module */
#elif CONFIG_3430SDP
#define SDP_SDRC_MDCFG_0_DDR	(0x02584019|B_ALL) /* Infin ddr module */
#define SDP_SDRC_MDCFG_0_DDR_2G	(0x03588019|B_ALL) /* QIMONDA ddr module */
#elif CONFIG_OMAP3_BEAGLE
#define SDP_SDRC_MDCFG_0_DDR	(0x00D04019|B_ALL) /* Samsung MCP ddr module */
#elif defined(CONFIG_3430ZOOM2_512M) ||\
			defined(CONFIG_3630ZOOM3) || defined(CONFIG_3630SDP)
#define SDP_SDRC_MDCFG_0_DDR	(0x03588099)	 /* Hynix MCP ddr module */
#elif defined(CONFIG_3630SDP_1G) || defined(CONFIG_3630ZOOM3_1G)
/*
 * LOCKSTATUS 	 - Set to 0b0;
 * RASWIDTH	 - Set to 0x3    For the row address a0-a13
 * CASWIDTH	 - Set to 0x6	 For the column address a0-a9,a11
 * ADDRMUXLEGACY - Set to 0b1;	 Enabled the Legacy Addresssing
 * RAMSIZE	 - Set to 0x100; Set this to 512MB for each CS
 * BANKALLOCATION - Set to 0x2;  Row-bank-column
 * B32NOT16	 - Set to 0b1;	 External SDRAM device is x32 bit.
 * DEEPPD	 - Set to 0b1;	 The memory supports deep-power-down mode
 * DDRTYPE	 - Set to 0b0;	 Mobile DDR
 * RAMTYPE	 - Set to 0x1;	 DDR-SDRAM (double data rate)
 */
#define SDP_SDRC_MDCFG_0_DDR	(0x03690099)	 /* Hynix MCP ddr module */
#elif defined(CONFIG_3630KUNLUN_KL9C) || defined(CONFIG_3630KUNLUN_JT)
#define SDP_SDRC_MDCFG_0_DDR	(0x03588099)	 /* Toshiba MCP ddr module */
#elif defined(CONFIG_3630KUNLUN_WUDANG)
#define SDP_SDRC_MDCFG_0_DDR	(0x03588099)	 /* Hynix TWO CS , each one is 256M MCP ddr module */
#elif defined(CONFIG_K29C_MEM)  //2011.5.9 jhy Flash for N8K
#define SDP_SDRC_MDCFG_0_DDR    (0x03588099)     /* Toshiba MCP ddr module */
#elif defined(CONFIG_3630KUNLUN_P2) || defined(CONFIG_3630PUMA_V1)
#define SDP_SDRC_MDCFG_0_DDR	(0x03588099)	 /* Hynix 256M MCP ddr module */
#else
#define SDP_SDRC_MDCFG_0_DDR	(0x02584099)	 /* Micron MCP ddr module */
#endif

#define SDP_SDRC_MR_0_DDR		0x00000032

/* optimized timings good for current shipping parts */
#define SDP_3430_SDRC_RFR_CTRL_100MHz   0x0002da01
#define SDP_3430_SDRC_RFR_CTRL_133MHz   0x0003de01 /* 7.8us/7.5ns - 50=0x3de */
#define SDP_3430_SDRC_RFR_CTRL_165MHz   0x0004e201 /* 7.8us/6ns - 50=0x4e2 */
#define ZOOM3_3630_RFR_CTRL_200MHz	0x0005e601

#define DLL_OFFSET              0
#define DLL_WRITEDDRCLKX2DIS    1
#define DLL_ENADLL              1
#define DLL_LOCKDLL             0
#define DLL_DLLPHASE_72         0
#define DLL_DLLPHASE_90         1

#define SDP_SDRC_DLLAB_CTRL ((DLL_ENADLL << 3) | \
    (DLL_LOCKDLL << 2) | (DLL_DLLPHASE_90 << 1))

#if defined(CONFIG_3430LABRADOR) || defined(CONFIG_BEAGLE_REV2)
/* Micron part of 3430 Labrador (133MHz optimized) ~ 7.5ns
 *	TDAL = Twr/Tck + Trp/tck = 15/7.5 + 22.5/7.5 = 2 + 3 = 5
 *	TDPL =  15/7.5   = 2
 *	TRRD =  15/7.5   = 2
 *	TRCD =  22.5/7.5 = 3
 *	TRP  =  22.5/7.5 = 3
 *	TRAS =  45/7.5   = 6
 *	TRC  =  75/7.5   = 10
 *	TRFC =  125/7.5  = 16.6->17
 *   ACTIMB
 *	TWTR =  1
 *	TCKE =  1
 *	TXSR =  138/7.5  = 18.3->19
 *	TXP  =  25/7.5   = 3.3->4
 */
#define TDAL_133   5
#define TDPL_133   2
#define TRRD_133   2
#define TRCD_133   3
#define TRP_133    3
#define TRAS_133   6
#define TRC_133    10
#define TRFC_133   17
#define V_ACTIMA_133 ((TRFC_133 << 27) | (TRC_133 << 22) | (TRAS_133 << 18) \
		|(TRP_133 << 15) | (TRCD_133 << 12) |(TRRD_133 << 9) |(TDPL_133 << 6) \
		| (TDAL_133))

#define TWTR_133   1
#define TCKE_133   1
#define TXSR_133   19
#define TXP_133    4
#define V_ACTIMB_133 ((TWTR_133 << 16) | (TCKE_133 << 12) | (TXP_133 << 8) \
		| (TXSR_133 << 0))
#else
/* Infineon part of 3430SDP (133MHz optimized) ~ 7.5ns
 *	TDAL = Twr/Tck + Trp/tck = 15/7.5 + 22.5/7.5 = 2 + 3 = 5
 *	TDPL = 15/7.5	= 2
 *	TRRD = 15/2.5	= 2
 *	TRCD = 22.5/7.5	= 3
 *	TRP = 22.5/7.5	= 3
 *	TRAS = 45/7.5	= 6
 *	TRC = 65/7.5	= 8.6->9
 *	TRFC = 75/7.5	= 10
 *   ACTIMB
 *	TWTR = 1
 *	TCKE = 2
 *	TXP =  = 12/7.5=1.6=2
 *	XSR = 120/7.5 = 16
 */
#define TDAL_133   5
#define TDPL_133   2
#define TRRD_133   2
#define TRCD_133   3
#define TRP_133    3
#define TRAS_133   6
#define TRC_133    9
#define TRFC_133  10
#define V_ACTIMA_133 ((TRFC_133 << 27) | (TRC_133 << 22) | (TRAS_133 << 18) \
		|(TRP_133 << 15) | (TRCD_133 << 12) |(TRRD_133 << 9) |(TDPL_133 << 6) \
		| (TDAL_133))

#define TWTR_133   1
#define TCKE_133   2
#define TXP_133    2
#define XSR_133   16
#define V_ACTIMB_133 ((TCKE_133 << 12) | (XSR_133 << 0)) | \
				(TXP_133 << 8) | (TWTR_133 << 16)
#endif
#define V_ACTIMA_100 V_ACTIMA_133
#define V_ACTIMB_100 V_ACTIMB_133

#if defined(CONFIG_3430LABRADOR) || defined(CONFIG_BEAGLE_REV2) || defined(CONFIG_3630KUNLUN)
/* Micron part of 3430 Labrador (166MHz optimized) 6.02ns
 *     ACTIMA
 *        -TDAL = Twr/Tck + Trp/tck = 15/6 + 18/6 = 2.5 + 3 = 5.5 -> 6
 *        -TDPL (Twr) = 15/6       = 2.5 -> 3
 *        -TRRD = 12/6     = 2
 *        -TRCD = 18/6     = 3
 *        -TRP = 18/6      = 3
 *        -TRAS = 42/6     = 7
 *        -TRC = 60/6      = 10
 *        -TRFC = 125/6    = 20.8 -> 21
 *        ACTIMB
 *        -TWTR = 1
 *        -TCKE = 1
 *        -TXP = 25/6 = 5
 *        -XSR = 138/6 = 23
 */
#define TDAL_165   6
#define TDPL_165   3
#define TRRD_165   2
#define TRCD_165   3
#define TRP_165    3
#define TRAS_165   7
#define TRC_165   10
#define TRFC_165  21
#define V_ACTIMA_165 ((TRFC_165 << 27) | (TRC_165 << 22) | (TRAS_165 << 18) \
		                | (TRP_165 << 15) | (TRCD_165 << 12) |(TRRD_165 << 9) | \
		                (TDPL_165 << 6) | (TDAL_165))

#define TWTR_165   1
#define TCKE_165   1
#define TXP_165    5
#define XSR_165    23
#define V_ACTIMB_165 ((TCKE_165 << 12) | (XSR_165 << 0)) | \
	                                (TXP_165 << 8) | (TWTR_165 << 16)

/* HY part of 3430 Labrador (200MHz optimized) 6.02ns
 *     ACTIMA
 *        -TDAL = Twr/Tck + Trp/tck = 15/5 + 15/5 = 3 + 3 =  6
 *        -TDPL (Twr) = 15/6       = 3
 *        -TRRD = 10/5     = 2
 *        -TRCD = 20/5     = 4
 *        -TRP = 15/5      = 3
 *        -TRAS = 40/5     = 8
 *        -TRC = 55/5      = 11
 *        -TRFC = 90/5    = 18
 *        ACTIMB
 *        -TWTR = 2
 *        -TCKE = 5
 *        -TXP = 25/6 = 5
 *        -XSR = 140/5 = 28
 */
#define TDAL_200   6
#define TDPL_200   3
#define TRRD_200   2
#define TRCD_200   4
#define TRP_200    3
#define TRAS_200   8
#define TRC_200   11
#define TRFC_200  18
#define V_ACTIMA_200 ((TRFC_200 << 27) | (TRC_200 << 22) | (TRAS_200 << 18) \
                                | (TRP_200 << 15) | (TRCD_200 << 12) |(TRRD_200 << 9) | \
                                (TDPL_200 << 6) | (TDAL_200))

#define TWTR_200   2
#define TCKE_200   1
#define TXP_200    5
#define XSR_200    28
#define V_ACTIMB_200 ((TCKE_200 << 12) | (XSR_200 << 0)) | \
                                        (TXP_200 << 8) | (TWTR_200 << 16)

#elif defined(CONFIG_3430ZOOM2_512M) ||\
		defined(CONFIG_3630ZOOM3) || defined(CONFIG_3630SDP)\
	|| defined(CONFIG_3630SDP_1G) || defined(CONFIG_3630ZOOM3_1G)

/* Hynix part of 3430 Zoom2 (166MHz optimized) 6.02ns
 *     ACTIMA
 *        -TDAL = Twr/Tck + Trp/tck = 15/6 + 18/6 = 2.5 + 3 = 5.5 -> 6
 *        -TDPL (Twr) = 15/6       = 2.5 -> 3
 *        -TRRD = 12/6     = 2
 *        -TRCD = 18/6     = 3
 *        -TRP = 18/6      = 3
 *        -TRAS = 42/6     = 7
 *        -TRC = 60/6      = 10
 *        -TRFC = 97.5/6    = 17
 *     ACTIMB
 *        -TWTR = 1
 *        -TCKE = 1
 *        -TXP = 1+1
 *        -XSR = 140/6 = 24
 */
#define TDAL_165	6
#define TDPL_165	3
#define TRRD_165	2
#define TRCD_165	3
#define TRP_165	3
#define TRAS_165	7
#define TRC_165	10
#define TRFC_165	21
#define V_ACTIMA_165 ((TRFC_165 << 27) | (TRC_165 << 22) | (TRAS_165 << 18) |\
		      (TRP_165 << 15) | (TRCD_165 << 12) | (TRRD_165 << 9) | \
		      (TDPL_165 << 6) | (TDAL_165))

#define TWTR_165	1
#define TCKE_165	1
#define TXP_165	2
#define XSR_165	24
#define V_ACTIMB_165 (((TCKE_165 << 12) | (XSR_165 << 0)) |	\
		      (TXP_165 << 8) | (TWTR_165 << 16))

/* TODO : Cleanup magic */
#define V_ACTIMA_200 0xa2e1b4c6
#define V_ACTIMB_200 0x0002131c

#else /* sdp3430 */
/* Infineon part of 3430SDP (166MHz optimized) 6.02ns
 *   ACTIMA
 *	TDAL = Twr/Tck + Trp/tck = 15/6 + 18/6 = 2.5 + 3 = 5.5 -> 6
 *	TDPL (Twr) = 15/6	= 2.5 -> 3
 *	TRRD = 12/6	= 2
 *	TRCD = 18/6	= 3
 *	TRP = 18/6	= 3
 *	TRAS = 42/6	= 7
 *	TRC = 60/6	= 10
 *	TRFC = 72/6	= 12
 *   ACTIMB
 *	TCKE = 2
 *	XSR = 120/6 = 20
 */
#define TDAL_165   6
#define TDPL_165   3
#define TRRD_165   2
#define TRCD_165   3
#define TRP_165    3
#define TRAS_165   7
#define TRC_165   10
#define TRFC_165  12
#define V_ACTIMA_165 ((TRFC_165 << 27) | (TRC_165 << 22) | (TRAS_165 << 18) \
		| (TRP_165 << 15) | (TRCD_165 << 12) |(TRRD_165 << 9) | \
		(TDPL_165 << 6) | (TDAL_165))

#define TWTR_165   1
#define TCKE_165   2
#define TXP_165    2
#define XSR_165    20
#define V_ACTIMB_165 ((TCKE_165 << 12) | (XSR_165 << 0)) | \
				(TXP_165 << 8) | (TWTR_165 << 16)

#endif

/* New and compatability speed defines */
#if defined(PRCM_CLK_CFG2_200MHZ) || defined(PRCM_CONFIG_II) || defined(PRCM_CONFIG_5B)
# define L3_100MHZ   /* Use with <= 100MHz SDRAM */
#elif defined (PRCM_CLK_CFG2_266MHZ) || defined(PRCM_CONFIG_III) || defined(PRCM_CONFIG_5A)
# define L3_133MHZ    /* Use with <= 133MHz SDRAM*/
#elif  defined(PRCM_CLK_CFG2_332MHZ) || defined(PRCM_CONFIG_I) || defined(PRCM_CONFIG_2)
# define L3_165MHZ    /* Use with <= 165MHz SDRAM (L3=166 on 3430) */
#elif defined(PRCM_CLK_CFG2_400MHZ)
# define L3_200MHZ    /* Use with <= 200MHz SDRAM (L3=200 on 3630) */
#else
#error "Undefined bus speed"
#endif


#if defined(CONFIG_DDR_H8KDS0UN0MER_4EM)
/* Hynix H8KDS0UN0MER-4EM (DDR400, just used as 166)(166MHz optimized) 6.02ns
 *   ACTIMA
 *	TDAL = Twr/Tck + Trp/tck = 15/6 + 15/6 = 2.5 + 2.5 = 3 + 3 -> 6
 *	TDPL (Twr) = 15/6	= 2.5 -> 3
 *	TRRD = 10/6	= 2
 *	TRCD = 20/6	= 4
 *	TRP = 15/6	= 3
 *	TRAS = 40/6	= 7
 *	TRC = (TRAS + TRP) = 7 + 3 = 10
 *	TRFC = 90/6	= 15
 *   ACTIMB
 *	TCKE = 1
 *	XSR = 140/6 = 24
 */
#undef  TDAL_165
#undef  TDPL_165
#undef  TRRD_165
#undef  TRCD_165
#undef  TRP_165
#undef  TRAS_165
#undef  TRC_165
#undef  TRFC_165
#undef  TWTR_165
#undef  TCKE_165
#undef  TXP_165
#undef  XSR_165

#undef  TDAL_200
#undef  TDPL_200
#undef  TRRD_200
#undef  TRCD_200
#undef  TRP_200
#undef  TRAS_200
#undef  TRC_200
#undef  TRFC_200
#undef  TWTR_200
#undef  TCKE_200
#undef  TXP_200
#undef  XSR_200

/* All plus 1 */
#define TDAL_165   7
#define TDPL_165   4
#define TRRD_165   3
#define TRCD_165   5
#define TRP_165    4
#define TRAS_165   8
#define TRC_165   11
#define TRFC_165  16
#undef V_ACTIMA_165
#define V_ACTIMA_165 ((TRFC_165 << 27) | (TRC_165 << 22) | (TRAS_165 << 18) \
		| (TRP_165 << 15) | (TRCD_165 << 12) |(TRRD_165 << 9) | \
		(TDPL_165 << 6) | (TDAL_165))

#define TWTR_165   3
#define TCKE_165   2
#define TXP_165    3    //for test, maybe 1 is enough
#define XSR_165    25
#undef V_ACTIMB_165
#define V_ACTIMB_165 ((TCKE_165 << 12) | (XSR_165 << 0)) | \
				(TXP_165 << 8) | (TWTR_165 << 16)

/* Hynix H8KDS0UN0MER-4EM (DDR400)(200MHz optimized) 5ns
 *     ACTIMA
 *        -TDAL = Twr/Tck + Trp/tck = 15/5 + 15/5 = 3 + 3 =  6
 *        -TDPL (Twr) = 15/6       = 3
 *        -TRRD = 10/5     = 2
 *        -TRCD = 20/5     = 4
 *        -TRP = 15/5      = 3
 *        -TRAS = 40/5     = 8
 *        -TRC = TRAS + TRP  = 8 + 3 = 11
 *        -TRFC = 90/5    = 18
 *        ACTIMB
 *        -TWTR = 2
 *        -TCKE = 2 (maybe 1 is enough)
 *        -TXP = 2  (maybe 1 is enough)
 *        -XSR = 140/5 = 28
 */
/* All Plus 1 */
#define TDAL_200   7
#define TDPL_200   4
#define TRRD_200   3
#define TRCD_200   5
#define TRP_200    4
#define TRAS_200   9
#define TRC_200   12
#define TRFC_200  19
#define V_ACTIMA_200 ((TRFC_200 << 27) | (TRC_200 << 22) | (TRAS_200 << 18) \
                                | (TRP_200 << 15) | (TRCD_200 << 12) |(TRRD_200 << 9) | \
                                (TDPL_200 << 6) | (TDAL_200))

#define TWTR_200   3
#define TCKE_200   3
#define TXP_200    3
#define XSR_200    29
#define V_ACTIMB_200 ((TCKE_200 << 12) | (XSR_200 << 0)) | \
                                        (TXP_200 << 8) | (TWTR_200 << 16)
#endif

#if defined(L3_100MHZ)
# define SDP_SDRC_ACTIM_CTRLA_0     V_ACTIMA_100
# define SDP_SDRC_ACTIM_CTRLB_0     V_ACTIMB_100
# define SDP_SDRC_RFR_CTRL          SDP_3430_SDRC_RFR_CTRL_100MHz
#elif defined(L3_133MHZ)
# define SDP_SDRC_ACTIM_CTRLA_0     V_ACTIMA_133
# define SDP_SDRC_ACTIM_CTRLB_0     V_ACTIMB_133
# define SDP_SDRC_RFR_CTRL          SDP_3430_SDRC_RFR_CTRL_133MHz
#elif  defined(L3_165MHZ)
# define SDP_SDRC_ACTIM_CTRLA_0     V_ACTIMA_165
# define SDP_SDRC_ACTIM_CTRLB_0     V_ACTIMB_165
# define SDP_SDRC_RFR_CTRL          SDP_3430_SDRC_RFR_CTRL_165MHz
#elif defined(L3_200MHZ)
# define SDP_SDRC_ACTIM_CTRLA_0     V_ACTIMA_200
# define SDP_SDRC_ACTIM_CTRLB_0     V_ACTIMB_200
# define SDP_SDRC_RFR_CTRL          ZOOM3_3630_RFR_CTRL_200MHz
#endif

/*
 * GPMC settings -
 * Definitions is as per the following format
 * # define <PART>_GPMC_CONFIG<x> <value>
 * Where:
 * PART is the part name e.g. STNOR - Intel Strata Flash
 * x is GPMC config registers from 1 to 6 (there will be 6 macros)
 * Value is corresponding value
 *
 * For every valid PRCM configuration there should be only one definition of
 * the same. if values are independent of the board, this definition will be
 * present in this file if values are dependent on the board, then this should
 * go into corresponding mem-boardName.h file
 *
 * Currently valid part Names are (PART):
 * STNOR - Intel Strata Flash
 * SMNAND - Samsung NAND
 * M_NAND - Micron Large page x16 NAND
 * MPDB - H4 MPDB board
 * SBNOR - Sibley NOR
 * ONNAND - Samsung One NAND
 *
 * include/configs/file.h contains the defn - for all CS we are interested
 * #define OMAP34XX_GPMC_CSx PART
 * #define OMAP34XX_GPMC_CSx_SIZE Size
 * #define OMAP34XX_GPMC_CSx_MAP Map
 * Where:
 * x - CS number
 * PART - Part Name as defined above
 * SIZE - how big is the mapping to be
 *   GPMC_SIZE_128M - 0x8
 *   GPMC_SIZE_64M  - 0xC
 *   GPMC_SIZE_32M  - 0xE
 *   GPMC_SIZE_16M  - 0xF
 * MAP  - Map this CS to which address(GPMC address space)- Absolute address
 *   >>24 before being used.
 */
#define GPMC_SIZE_256M	0x0
#define GPMC_SIZE_128M  0x8
#define GPMC_SIZE_64M   0xC
#define GPMC_SIZE_32M   0xE
#define GPMC_SIZE_16M   0xF

#if defined(L3_100MHZ)
# define SMNAND_GPMC_CONFIG1 0x0
# define SMNAND_GPMC_CONFIG2 0x00141400
# define SMNAND_GPMC_CONFIG3 0x00141400
# define SMNAND_GPMC_CONFIG4 0x0F010F01
# define SMNAND_GPMC_CONFIG5 0x010C1414
# define SMNAND_GPMC_CONFIG6 0x00000A80

# define M_NAND_GPMC_CONFIG1 0x00001800
# define M_NAND_GPMC_CONFIG2 0x00141400
# define M_NAND_GPMC_CONFIG3 0x00141400
# define M_NAND_GPMC_CONFIG4 0x0F010F01
# define M_NAND_GPMC_CONFIG5 0x010C1414
# define M_NAND_GPMC_CONFIG6 0x1f0f0A80

# define STNOR_GPMC_CONFIG1  0x3
# define STNOR_GPMC_CONFIG2  0x000f0f01
# define STNOR_GPMC_CONFIG3  0x00050502
# define STNOR_GPMC_CONFIG4  0x0C060C06
# define STNOR_GPMC_CONFIG5  0x01131F1F
# define STNOR_GPMC_CONFIG6  0x1F0F0000

# define MPDB_GPMC_CONFIG1   0x00011000
# define MPDB_GPMC_CONFIG2   0x001F1F00
# define MPDB_GPMC_CONFIG3   0x00080802
# define MPDB_GPMC_CONFIG4   0x1C091C09
# define MPDB_GPMC_CONFIG5   0x031A1F1F
# define MPDB_GPMC_CONFIG6   0x000003C2
#endif

#if defined(L3_133MHZ)
# define SMNAND_GPMC_CONFIG1 0x00000800
# define SMNAND_GPMC_CONFIG2 0x00141400
# define SMNAND_GPMC_CONFIG3 0x00141400
# define SMNAND_GPMC_CONFIG4 0x0F010F01
# define SMNAND_GPMC_CONFIG5 0x010C1414
# define SMNAND_GPMC_CONFIG6 0x1F0F0A80
# define SMNAND_GPMC_CONFIG7 0x00000C44

# define M_NAND_GPMC_CONFIG1 0x00001800 /* might reuse smnand, with |= 1000 */
# define M_NAND_GPMC_CONFIG2 0x00141400
# define M_NAND_GPMC_CONFIG3 0x00141400
# define M_NAND_GPMC_CONFIG4 0x0F010F01
# define M_NAND_GPMC_CONFIG5 0x010C1414
# define M_NAND_GPMC_CONFIG6 0x1F0F0A80
# define M_NAND_GPMC_CONFIG7 0x00000C44

# define STNOR_GPMC_CONFIG1  0x1203
# define STNOR_GPMC_CONFIG2  0x00151501
# define STNOR_GPMC_CONFIG3  0x00060602
# define STNOR_GPMC_CONFIG4  0x10081008
# define STNOR_GPMC_CONFIG5  0x01131F1F
# define STNOR_GPMC_CONFIG6  0x1F0F04c4

# define SIBNOR_GPMC_CONFIG1  0x1200
# define SIBNOR_GPMC_CONFIG2  0x001f1f00
# define SIBNOR_GPMC_CONFIG3  0x00080802
# define SIBNOR_GPMC_CONFIG4  0x1C091C09
# define SIBNOR_GPMC_CONFIG5  0x01131F1F
# define SIBNOR_GPMC_CONFIG6  0x1F0F03C2

/* ES1 SDP and ES1 chip Debug FPGA */
# define MPDB_GPMC_CONFIG1  0x00011000
# define MPDB_GPMC_CONFIG2  0x001f1f01
# define MPDB_GPMC_CONFIG3  0x00080803
# define MPDB_GPMC_CONFIG4  0x1C091C09
# define MPDB_GPMC_CONFIG5  0x041f1F1F
# define MPDB_GPMC_CONFIG6  0x000004C4

/* ES2 SDP and ES2 chip Debug FPGA */
# define SDPV2_MPDB_GPMC_CONFIG1  0x00611200
# define SDPV2_MPDB_GPMC_CONFIG2  0x001F1F01
# define SDPV2_MPDB_GPMC_CONFIG3  0x00080803
# define SDPV2_MPDB_GPMC_CONFIG4  0x1D091D09
# define SDPV2_MPDB_GPMC_CONFIG5  0x041D1F1F
# define SDPV2_MPDB_GPMC_CONFIG6  0x1D0904C4

# define LAB_ENET_GPMC_CONFIG1  0x00611000
# define LAB_ENET_GPMC_CONFIG2  0x001F1F01
# define LAB_ENET_GPMC_CONFIG3  0x00080803
# define LAB_ENET_GPMC_CONFIG4  0x1D091D09
# define LAB_ENET_GPMC_CONFIG5  0x041D1F1F
# define LAB_ENET_GPMC_CONFIG6  0x1D0904C4

# define P2_GPMC_CONFIG1  0x0
# define P2_GPMC_CONFIG2  0x0
# define P2_GPMC_CONFIG3  0x0
# define P2_GPMC_CONFIG4  0x0
# define P2_GPMC_CONFIG5  0x0
# define P2_GPMC_CONFIG6  0x0

# define ONENAND_GPMC_CONFIG1 0x00001200
# define ONENAND_GPMC_CONFIG2 0x000c0c01
# define ONENAND_GPMC_CONFIG3 0x00030301
# define ONENAND_GPMC_CONFIG4 0x0c040c04
# define ONENAND_GPMC_CONFIG5 0x010C1010
# define ONENAND_GPMC_CONFIG6 0x1F060000

#endif /* endif L3_133MHZ */

#if defined (L3_165MHZ)
# define SMNAND_GPMC_CONFIG1 0x00000800
# define SMNAND_GPMC_CONFIG2 0x00060600
# define SMNAND_GPMC_CONFIG3 0x00060401
# define SMNAND_GPMC_CONFIG4 0x05010801
# define SMNAND_GPMC_CONFIG5 0x00090B0B
# define SMNAND_GPMC_CONFIG6 0x050001C0
# define SMNAND_GPMC_CONFIG7 0x00000C44

#if defined(CONFIG_3630KUNLUN_N710E)
/* Nand timing for Haier N710E. */
/* As same as 200MHz According to haier N8K series,should copy to new packet*/
# define M_NAND_GPMC_CONFIG1 0x00001800
# define M_NAND_GPMC_CONFIG2 0x00141400
# define M_NAND_GPMC_CONFIG3 0x00141400
# define M_NAND_GPMC_CONFIG4 0x0F010F01
# define M_NAND_GPMC_CONFIG5 0x010C1414
# define M_NAND_GPMC_CONFIG6 0x1F0F0A80
# define M_NAND_GPMC_CONFIG7 0x00000C44

/* Default. */
#else
/* As same as 200MHz */
# define M_NAND_GPMC_CONFIG1 0x00001800
# define M_NAND_GPMC_CONFIG2 0x00181800
# define M_NAND_GPMC_CONFIG3 0x00181800
# define M_NAND_GPMC_CONFIG4 0x12031203     //for hynix, OEONTIME should 2, plus 1
# define M_NAND_GPMC_CONFIG5 0x020f1818
# define M_NAND_GPMC_CONFIG6 0x1F0F0F80     //maybe 0x000F0F80 can work also
# define M_NAND_GPMC_CONFIG7 0x00000C44
#endif /* Define CONFIG_3630KUNLUN_N710E. */

# define STNOR_GPMC_CONFIG1  0x3
# define STNOR_GPMC_CONFIG2  0x00151501
# define STNOR_GPMC_CONFIG3  0x00060602
# define STNOR_GPMC_CONFIG4  0x11091109
# define STNOR_GPMC_CONFIG5  0x01141F1F
# define STNOR_GPMC_CONFIG6  0x1F0F04c4

# define SIBNOR_GPMC_CONFIG1  0x1200
# define SIBNOR_GPMC_CONFIG2  0x001f1f00
# define SIBNOR_GPMC_CONFIG3  0x00080802
# define SIBNOR_GPMC_CONFIG4  0x1C091C09
# define SIBNOR_GPMC_CONFIG5  0x01131F1F
# define SIBNOR_GPMC_CONFIG6  0x1F0F03C2

# define SDPV2_MPDB_GPMC_CONFIG1  0x00611200
# define SDPV2_MPDB_GPMC_CONFIG2  0x001F1F01
# define SDPV2_MPDB_GPMC_CONFIG3  0x00080803
# define SDPV2_MPDB_GPMC_CONFIG4  0x1D091D09
# define SDPV2_MPDB_GPMC_CONFIG5  0x041D1F1F
# define SDPV2_MPDB_GPMC_CONFIG6  0x1D0904C4

# define MPDB_GPMC_CONFIG1  0x00011000
# define MPDB_GPMC_CONFIG2  0x001f1f01
# define MPDB_GPMC_CONFIG3  0x00080803
# define MPDB_GPMC_CONFIG4  0x1c0b1c0a
# define MPDB_GPMC_CONFIG5  0x041f1F1F
# define MPDB_GPMC_CONFIG6  0x1F0F04C4

# define LAB_ENET_GPMC_CONFIG1  0x00611000
# define LAB_ENET_GPMC_CONFIG2  0x001F1F01
# define LAB_ENET_GPMC_CONFIG3  0x00080803
# define LAB_ENET_GPMC_CONFIG4  0x1D091D09
# define LAB_ENET_GPMC_CONFIG5  0x041D1F1F
# define LAB_ENET_GPMC_CONFIG6  0x1D0904C4

# define P2_GPMC_CONFIG1  0x0
# define P2_GPMC_CONFIG2  0x0
# define P2_GPMC_CONFIG3  0x0
# define P2_GPMC_CONFIG4  0x0
# define P2_GPMC_CONFIG5  0x0
# define P2_GPMC_CONFIG6  0x0

# define ONENAND_GPMC_CONFIG1 0x00001200
# define ONENAND_GPMC_CONFIG2 0x000F0F01
# define ONENAND_GPMC_CONFIG3 0x00030301
# define ONENAND_GPMC_CONFIG4 0x0F040F04
# define ONENAND_GPMC_CONFIG5 0x010F1010
# define ONENAND_GPMC_CONFIG6 0x1F060000
#endif /* L3_165MHZ */

#if defined(L3_200MHZ)
# define SMNAND_GPMC_CONFIG1 0x00000800
# define SMNAND_GPMC_CONFIG2 0x00060600
# define SMNAND_GPMC_CONFIG3 0x00060401
# define SMNAND_GPMC_CONFIG4 0x05010801
# define SMNAND_GPMC_CONFIG5 0x00090B0B
# define SMNAND_GPMC_CONFIG6 0x050001C0
# define SMNAND_GPMC_CONFIG7 0x00000C44

#if defined (CONFIG_3630KUNLUN_N710E)
/* Nand timing for Haier N710E. */

/* As same as 200MHz According to haier N8K series,should copy to new packet*/
# define M_NAND_GPMC_CONFIG1 0x00001800
# define M_NAND_GPMC_CONFIG2 0x00181800
# define M_NAND_GPMC_CONFIG3 0x00181800
# define M_NAND_GPMC_CONFIG4 0x12021202
# define M_NAND_GPMC_CONFIG5 0x020f1818
# define M_NAND_GPMC_CONFIG6 0x00000c80
# define M_NAND_GPMC_CONFIG7 0x00000870

#else /* Default. */
/* As same as 200MHz */
# define M_NAND_GPMC_CONFIG1 0x00001800
# define M_NAND_GPMC_CONFIG2 0x00181800
# define M_NAND_GPMC_CONFIG3 0x00181800
# define M_NAND_GPMC_CONFIG4 0x12031203     //for hynix, OEONTIME should 2, plus 1
# define M_NAND_GPMC_CONFIG5 0x020f1818
# define M_NAND_GPMC_CONFIG6 0x1F0F0F80     //maybe 0x000F0F80 can work also
# define M_NAND_GPMC_CONFIG7 0x00000C44
#endif /* Define CONFIG_3630KUNLUN_N710E. */

# define STNOR_GPMC_CONFIG1  0x3
# define STNOR_GPMC_CONFIG2  0x00151501
# define STNOR_GPMC_CONFIG3  0x00060602
# define STNOR_GPMC_CONFIG4  0x11091109
# define STNOR_GPMC_CONFIG5  0x01141F1F
# define STNOR_GPMC_CONFIG6  0x1F0F04c4

# define SIBNOR_GPMC_CONFIG1  0x1210
# define SIBNOR_GPMC_CONFIG2  0x00131300
# define SIBNOR_GPMC_CONFIG3  0x00050501
# define SIBNOR_GPMC_CONFIG4  0x11061106
# define SIBNOR_GPMC_CONFIG5  0x010c1313
# define SIBNOR_GPMC_CONFIG6  0x130902c2

# define SDPV2_MPDB_GPMC_CONFIG1  0x00611200
# define SDPV2_MPDB_GPMC_CONFIG2  0x001F1F01
# define SDPV2_MPDB_GPMC_CONFIG3  0x00080803
# define SDPV2_MPDB_GPMC_CONFIG4  0x1D091D09
# define SDPV2_MPDB_GPMC_CONFIG5  0x041D1F1F
# define SDPV2_MPDB_GPMC_CONFIG6  0x1D0904C4

# define MPDB_GPMC_CONFIG1  0x00011000
# define MPDB_GPMC_CONFIG2  0x001f1f01
# define MPDB_GPMC_CONFIG3  0x00080803
# define MPDB_GPMC_CONFIG4  0x1c0b1c0a
# define MPDB_GPMC_CONFIG5  0x041f1F1F
# define MPDB_GPMC_CONFIG6  0x1F0F04C4

# define LAB_ENET_GPMC_CONFIG1  0x00611000
# define LAB_ENET_GPMC_CONFIG2  0x001F1F01
# define LAB_ENET_GPMC_CONFIG3  0x00080803
# define LAB_ENET_GPMC_CONFIG4  0x1D091D09
# define LAB_ENET_GPMC_CONFIG5  0x041D1F1F
# define LAB_ENET_GPMC_CONFIG6  0x1D0904C4

# define P2_GPMC_CONFIG1  0x0
# define P2_GPMC_CONFIG2  0x0
# define P2_GPMC_CONFIG3  0x0
# define P2_GPMC_CONFIG4  0x0
# define P2_GPMC_CONFIG5  0x0
# define P2_GPMC_CONFIG6  0x0

# define ONENAND_GPMC_CONFIG1 0x00001200
# define ONENAND_GPMC_CONFIG2 0x000F0F01
# define ONENAND_GPMC_CONFIG3 0x00030301
# define ONENAND_GPMC_CONFIG4 0x0F040F04
# define ONENAND_GPMC_CONFIG5 0x010F1010
# define ONENAND_GPMC_CONFIG6 0x1F060000

#endif /* L3_200MHZ */

/* max number of GPMC Chip Selects */
#define GPMC_MAX_CS	8
/* max number of GPMC regs */
#define GPMC_MAX_REG	7

#define PISMO1_NOR	1
#define PISMO1_NAND	2
#define PISMO2_CS0	3
#define PISMO2_CS1	4
#define PISMO1_ONENAND	5
#define DBG_MPDB	6
#define PISMO2_NAND_CS0 7
#define PISMO2_NAND_CS1 8

/* make it readable for the gpmc_init */
#define PISMO1_NOR_BASE		FLASH_BASE
#define PISMO1_NAND_BASE	NAND_BASE
#define PISMO2_CS0_BASE		PISMO2_MAP1
#define PISMO1_ONEN_BASE	ONENAND_MAP
#define DBG_MPDB_BASE		DEBUG_BASE

#endif /* endif _OMAP34XX_MEM_H_ */
