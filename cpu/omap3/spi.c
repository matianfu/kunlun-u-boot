/*
 * =====================================================================================
 *
 *       Filename:  spi.c
 *
 *    Description:  OMAP3630 SPI controller
 *
 *        Version:  1.0
 *        Created:  04/06/2010 03:58:16 PM
 *       Revision:  none
 *       Compiler:  gcc
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
#include <lcd.h>
#include <i2c.h>
#include <asm/io.h>
#include <command.h>
#include <asm/arch/omap3430.h>
#include <asm/arch/cpu.h>

#define OMAP2_MCSPI_MAX_FREQ    48000000

#define MCSPI1_CHXCONF(x) (0x4809802C + 0x14 * (x))
#define MCSPI2_CHXCONF(x) (0x4809A02C + 0x14 * (x))
#define MCSPI3_CHXCONF(x) (0x480B802C + 0x14 * (x))
#define MCSPI4_CHXCONF(x) (0x480BA02C + 0x14 * (x))

#define MCSPI1_CHXCTRL(x) (0x48098034 + 0x14 * (x))
#define MCSPI2_CHXCTRL(x) (0x4809A034 + 0x14 * (x))
#define MCSPI3_CHXCTRL(x) (0x480B8034 + 0x14 * (x))
#define MCSPI4_CHXCTRL(x) (0x480BA034 + 0x14 * (x))

#define MCSPI1_CHXSTAT(x) (0x48098030 + 0x14 * (x))
#define MCSPI2_CHXSTAT(x) (0x4809A030 + 0x14 * (x))
#define MCSPI3_CHXSTAT(x) (0x480B8030 + 0x14 * (x))
#define MCSPI4_CHXSTAT(x) (0x480BA030 + 0x14 * (x))

#define MCSPI1_TXX(x) (0x48098038 + 0x14 * (x))
#define MCSPI2_TXX(x) (0x4809A038 + 0x14 * (x))
#define MCSPI3_TXX(x) (0x480B8038 + 0x14 * (x))
#define MCSPI4_TXX(x) (0x480BA038 + 0x14 * (x))

#define MCSPI1_RXX(x) (0x4809803C + 0x14 * (x))
#define MCSPI3_RXX(x) (0x480B803C + 0x14 * (x))
#define MCSPI1_MODULCTRL    0x48098028
#define MCSPI1_IRQSTATUS    0x48098018
#define MCSPI3_MODULCTRL    0x480B8028
#define MCSPI3_IRQSTATUS    0x480B8018

#ifdef  DEBUG
#define DEBUG_INFO(fmt,args...)  printf(fmt,##args)
#else
#define DEBUG_INFO(fmt,args...)
#endif

static void omap3_mcspi_enable (int num);
static void omap3_mcspi_disable (int num);

struct SPI_CONTROLER{
    int word_len; /*word length,use bits*/
};

struct SPI_CONTROLER spi_controller[3];

static void omap3_mcspi_force_cs(int num,int cs_active)
{
	u32 l;
	if(num == 1){
	    l = __raw_readl(MCSPI1_CHXCONF(0));
	    l &= ~(1 << 20);
	    if(cs_active){
            l |= 1 << 20;
	    }
        __raw_writel(l,MCSPI1_CHXCONF(0));
	}
	else if(num == 3){
	    l = __raw_readl(MCSPI3_CHXCONF(0));
	    l &= ~(1 << 20);
	    if(cs_active){
            l |= 1 << 20;
	    }
        __raw_writel(l,MCSPI3_CHXCONF(0));
	}

}

void omap3_mcspi_startbit(int num,int high)
{
	u32 l;
	if(num == 1){
	    l = __raw_readl(MCSPI1_CHXCONF(0));
	    l &= ~(1 << 24);
	    if(high){
            l |= 1 << 24;
	    }
        __raw_writel(l,MCSPI1_CHXCONF(0));
	}
}

/*
 * ===  FUNCTION  ======================================================================
 *         Name:  omap3_mcspi_enable_clocks
 *  Description:  enable mcspi #number module FCLK and ICLK
 *  Parameters: @number:mcspi module number(1:4)
 *  Return       : 0 if success,or -1
 * =====================================================================================
 */
static int omap3_mcspi_enable_clocks (int number)
{
    u32 fclk,iclk;
    if(number < 1 || number > 4){
        return -1;
    }
    fclk = __raw_readl(CM_FCLKEN1_CORE);
    iclk = __raw_readl(CM_ICLKEN1_CORE);

    if(fclk & (1 << (17 + number))){
        DEBUG_INFO("mcspi[%d] FCLK is enabled already\n",number);
    }else{
        fclk |= 1 << (17 + number);
        __raw_writel(fclk,CM_FCLKEN1_CORE);
    }

    if(iclk & (1 << (17 + number))){
        DEBUG_INFO("mcspi[%d] ICLK is enabled already\n",number);
    }else{
        iclk |= 1 << (17 + number);
        __raw_writel(fclk,CM_ICLKEN1_CORE);
    }

    return 0;
}		/* -----  end of function omap3_mcspi_enable_clocks  ----- */




/*
 * ===  FUNCTION  ======================================================================
 *         Name:  omap3_mcspi_setup_transfer
 *  Description:  setup MCSPI controller registers and be ready for SPI transfer
 *  Parameters: @number:mcspi module number(1:4)
 *                      @word_len: SPI word length
 *                      @maxfreq:   required SPI clock frequency
 *  Return       : 0 if success,or -1
 * =====================================================================================
 */
int omap3_mcspi_setup_transfer (int num,int word_len,int maxfreq)
{
    /*ILI9327 requires SCL frequency no faster than (50/3)M(write),(10/3)M(read)*/

    u32 div = 0;
    u32 chxconf = 0;

    if(num < 1 || num > 4){
        return -1;
    }
    omap3_mcspi_enable_clocks (num);
    switch(num){
        case 1:
            chxconf = __raw_readl(MCSPI1_CHXCONF(0));
            break;
        case 2:
            chxconf = __raw_readl(MCSPI2_CHXCONF(0));
            break;
        case 3:
            chxconf = __raw_readl(MCSPI3_CHXCONF(0));
            break;
        case 4:
            chxconf = __raw_readl(MCSPI4_CHXCONF(0));
            break;
        default:
            return -1;
    }

	if(maxfreq){
		while (div <= 12 && (OMAP2_MCSPI_MAX_FREQ / (1 << div)) > maxfreq)
			div++;
	}else{
		div = 12;
	}
	chxconf &= ~(0xF << 2);
	chxconf |= (div & 0xF) << 2;
    /*clock granularity of power of 2*/
	chxconf &= ~(1 << 29);
	/*SPI word long*/
	chxconf &= ~(0x1F << 7);
	chxconf |= ((word_len & 0x1F) - 1) << 7;
	if(num == 1){
		/*spim_clk is held low during the active state */
	    chxconf |= 1 << 1;
	    /*PHA = 1,EPOL = 1*/
	    chxconf |= 1 << 0 | 1 << 6;
	    /*transimit mode only*/
	    chxconf &= ~(0x3 << 12);
	    chxconf |= 2 << 12;
	    /*spim_simo is selected for transmission*/
	    chxconf &= ~(1 << 18);
	    chxconf &= ~(1 << 17);
	    /*start bit enable*/
	    chxconf |= 1 << 23;
	    /*no transmission on spim_somi*/
	    chxconf |= 1 << 16;
	    /*not used FIFO*/
	    chxconf &= ~(3 << 27);
	}
	else if(num == 3){
		/*spim_clk is held low during the active state */
	    chxconf &= ~(1 << 1);
	    chxconf |= 1 << 6;
		 chxconf &= ~(1 << 0);
	    /*transimit mode only*/
	    chxconf &= ~(0x3 << 12);
	    chxconf |= 2 << 12;
	    /*spim_simo is selected for transmission*/
	    chxconf &= ~(1 << 18);
	    chxconf &= ~(1 << 17);
#if defined(CONFIG_LCD_NT35510)
		/*start bit Disable*/
	   chxconf &= ~(1 << 23);//mask by zwzhu for nt35510
#else
	/*start bit enable*/
	   chxconf |= 1 << 23;//mask by zwzhu for nt35510

#endif
	    /*no transmission on spim_somi*/
	    chxconf |= 1 << 16;
	    /*not used FIFO*/
	    chxconf &= ~(3 << 27);
	}

     switch(num){
        case 1:
            __raw_writel(chxconf,MCSPI1_CHXCONF(0));
            break;
        case 2:
            __raw_writel(chxconf,MCSPI2_CHXCONF(0));
            break;
        case 3:
            __raw_writel(chxconf,MCSPI3_CHXCONF(0));
            break;
        case 4:
            __raw_writel(chxconf,MCSPI4_CHXCONF(0));
            break;
    }
	DEBUG_INFO("MCSPI_CONF:%#08x\n",chxconf);
    spi_controller[num].word_len = word_len;
    if(num == 1){
	    omap3_mcspi_enable (1);
	}
	else if(num == 3){
	    omap3_mcspi_enable (3);
	}

    return 0;
}		/* -----  end of function omap3_mcspi_setup_transfer  ----- */


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  omap3_mcspi_read
 *  Description:  read a word from spi device
 *  Parameters: @num:mcspi module number(1:4)
 * =====================================================================================
 */
unsigned int omap3_mcspi_read (int num)
{
    return 0;
}		/* -----  end of function omap3_mcspi_read  ----- */




/*
 * ===  FUNCTION  ======================================================================
 *         Name:  omap3_mcspi_send_sync
 *  Description: send words synchronously to spi device
 *  Parameters: @num:mcspi module number(1:4)
 *                      @buf: data to be sent
 *                      @len: data length to be sent
 * =====================================================================================
 */
int omap3_mcspi_read_sync(int num, unsigned char *buf,int len)
{
    if(!buf || len <= 0)
        return -1;
    u8 data8 = 0;
    u16 data16 = 0;
	u32 data32 = 0;
    /*support McSPI1 only now*/
    if(num == 1){
        omap3_mcspi_force_cs(num,1);
        /*wait for TX is empty*/
        while(!(__raw_readl(MCSPI1_CHXSTAT(0)) & (1 << 1)));
        if(spi_controller[num].word_len <= 8){
            while(len > 0){
                data8 = *(u8*)buf;
                buf += sizeof(data8);
                __raw_writel(data8,MCSPI1_TXX(0));
                len -= sizeof(data8);
                /*wait for TX is empty*/
                while(!(__raw_readl(MCSPI1_CHXSTAT(0)) & (1 << 1)));
                DEBUG_INFO("transmit %#x OK \n",data8);
            }
        }else if(spi_controller[num].word_len <= 16){
            while(len > 0){
                data16 = *(u16*)buf;
                buf += sizeof(data16);
                __raw_writel(data16,MCSPI1_TXX(0));
                len -= sizeof(data16);
                /*wait for TX is empty*/
                while(!(__raw_readl(MCSPI1_CHXSTAT(0)) & (1 << 1)));
                DEBUG_INFO("transmit %#x OK \n",data16);
            }
		//while(!(__raw_readl(MCSPI1_CHXSTAT(0)) & (1 << 0)));
		data32 = __raw_readl(MCSPI1_RXX(0));
		DEBUG_INFO("data32 = 0x%x\n",data32);

        }else if(spi_controller[num].word_len <= 32){

        }else{
            printf("can't support word length %d\n",spi_controller[num].word_len);
            omap3_mcspi_force_cs(num,0);
            return -1;
        }
        omap3_mcspi_force_cs(num,0);
        return 0;
    }
    else if(num == 3){
        omap3_mcspi_force_cs(num,1);
        /*wait for TX is empty*/
        while(!(__raw_readl(MCSPI3_CHXSTAT(0)) & (1 << 1)));
        if(spi_controller[num].word_len <= 8){
            while(len > 0){
                data8 = *(u8*)buf;
                buf += sizeof(data8);
                __raw_writel(data8,MCSPI3_TXX(0));
                len -= sizeof(data8);
                /*wait for TX is empty*/
                while(!(__raw_readl(MCSPI3_CHXSTAT(0)) & (1 << 1)));
                DEBUG_INFO("transmit %#x OK \n",data8);
            }
        }else if(spi_controller[num].word_len <= 16){
            while(len > 0){
                data16 = *(u16*)buf;
                buf += sizeof(data16);
                __raw_writel(data16,MCSPI3_TXX(0));
                len -= sizeof(data16);
                /*wait for TX is empty*/
                while(!(__raw_readl(MCSPI3_CHXSTAT(0)) & (1 << 1)));
                DEBUG_INFO("transmit %#x OK \n",data16);
            }
		//while(!(__raw_readl(MCSPI1_CHXSTAT(0)) & (1 << 0)));
		data32 = __raw_readl(MCSPI3_RXX(0));
		DEBUG_INFO("data32 = 0x%x\n",data32);

        }else if(spi_controller[num].word_len <= 32){

        }else{
            printf("can't support word length %d\n",spi_controller[num].word_len);
            omap3_mcspi_force_cs(num,0);
            return -1;
        }
        omap3_mcspi_force_cs(num,0);
        return 0;
    }
    return 0;
}		/* -----  end of function omap3_mcspi_write  ----- */


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  omap3_mcspi_send_sync
 *  Description: send words synchronously to spi device
 *  Parameters: @num:mcspi module number(1:4)
 *                      @buf: data to be sent
 *                      @len: data length to be sent
 * =====================================================================================
 */
int omap3_mcspi_send_sync(int num, unsigned char *buf,int len)
{
    if(!buf || len <= 0)
        return -1;
    u8 data8 = 0;
    u16 data16 = 0;
    /*support McSPI1 only now*/
    if(num == 1){
        omap3_mcspi_force_cs(num,1);
        /*wait for TX is empty*/
        while(!(__raw_readl(MCSPI1_CHXSTAT(0)) & (1 << 1)));
        if(spi_controller[num].word_len <= 8){
            while(len > 0){
                data8 = *(u8*)buf;
                buf += sizeof(data8);
                __raw_writel(data8,MCSPI1_TXX(0));
                len -= sizeof(data8);
                /*wait for TX is empty*/
                while(!(__raw_readl(MCSPI1_CHXSTAT(0)) & (1 << 1)));
                DEBUG_INFO("transmit %#x OK \n",data8);
            }
        }else if(spi_controller[num].word_len <= 16){
            while(len > 0){
                data16 = *(u16*)buf;
                buf += sizeof(data16);
                __raw_writel(data16,MCSPI1_TXX(0));
                len -= sizeof(data16);
                /*wait for TX is empty*/
                while(!(__raw_readl(MCSPI1_CHXSTAT(0)) & (1 << 1)));
                DEBUG_INFO("transmit %#x OK \n",data16);
            }
        }else if(spi_controller[num].word_len <= 32){

        }else{
            printf("can't support word length %d\n",spi_controller[num].word_len);
            omap3_mcspi_force_cs(num,0);
            return -1;
        }
        omap3_mcspi_force_cs(num,0);
        return 0;
    }
    else  if(num == 3){
        omap3_mcspi_force_cs(num,1);
        /*wait for TX is empty*/
        while(!(__raw_readl(MCSPI3_CHXSTAT(0)) & (1 << 1)));
        if(spi_controller[num].word_len <= 8){
            while(len > 0){
                data8 = *(u8*)buf;
                buf += sizeof(data8);
                __raw_writel(data8,MCSPI3_TXX(0));
                len -= sizeof(data8);
                /*wait for TX is empty*/
                while(!(__raw_readl(MCSPI3_CHXSTAT(0)) & (1 << 1)));
                DEBUG_INFO("transmit %#x OK \n",data8);
            }
        }else if(spi_controller[num].word_len <= 16){
            while(len > 0){
                data16 = *(u16*)buf;
                buf += sizeof(data16);
                __raw_writel(data16,MCSPI3_TXX(0));
                len -= sizeof(data16);
                /*wait for TX is empty*/
                while(!(__raw_readl(MCSPI3_CHXSTAT(0)) & (1 << 1)));
                DEBUG_INFO("transmit %#x OK \n",data16);
            }
        }else if(spi_controller[num].word_len <= 32){

        }else{
            printf("can't support word length %d\n",spi_controller[num].word_len);
            omap3_mcspi_force_cs(num,0);
            return -1;
        }
        omap3_mcspi_force_cs(num,0);
        return 0;
    }
    return 0;
}		/* -----  end of function omap3_mcspi_write  ----- */


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  omap3_mcspi_enable
 *  Description:  enable the clock and the chanel 0 of mcspi module #num
 * =====================================================================================
 */
static void omap3_mcspi_enable (int num)
{
    u32 modulctrl = 0;
    u32 chxctrl = 0;
    /*support McSPI1 only at present*/
    if(num == 1){
        modulctrl = __raw_readl(MCSPI1_MODULCTRL);
        /*master generates clock,use single mode*/
	    modulctrl &= ~(1 << 3);
	    modulctrl &= ~(1 << 2);
	    modulctrl |= 1 << 0;

        __raw_writel(modulctrl,MCSPI1_MODULCTRL);
        chxctrl = __raw_readl(MCSPI1_CHXCTRL(0));
        chxctrl |= (1 << 0);
        __raw_writel(chxctrl,MCSPI1_CHXCTRL(0));
    }
    else if(num == 3){
        modulctrl = __raw_readl(MCSPI3_MODULCTRL);
        /*master generates clock,use single mode*/
	    modulctrl &= ~(1 << 3);
	    modulctrl &= ~(1 << 2);
	    modulctrl |= 1 << 0;

        __raw_writel(modulctrl,MCSPI3_MODULCTRL);
        chxctrl = __raw_readl(MCSPI3_CHXCTRL(0));
        chxctrl |= (1 << 0);
        __raw_writel(chxctrl,MCSPI3_CHXCTRL(0));
    }
    return ;
}		/* -----  end of function omap3_mcspi_enable  ----- */


/*
 * ===  FUNCTION  ======================================================================
 *         Name:  omap3_mcspi_disable
 *  Description:  disable the clock and the chanel 0 of mcspi module #num
 * =====================================================================================
 */
static void omap3_mcspi_disable (int num)
{
    u32 modulctrl = 0;
    u32 chxctrl = 0;
    /*support McSPI1 only at present*/
    if(num == 1){
        modulctrl = (1 << 2);
        __raw_writel(modulctrl,MCSPI1_MODULCTRL);
        chxctrl = __raw_readl(MCSPI1_CHXCTRL(0));
        chxctrl &= ~(1 << 0);
        __raw_writel(chxctrl,MCSPI1_CHXCTRL(0));
    }
    else if(num ==3)
    {
        modulctrl = (1 << 2);
        __raw_writel(modulctrl,MCSPI3_MODULCTRL);
        chxctrl = __raw_readl(MCSPI3_CHXCTRL(0));
        chxctrl &= ~(1 << 0);
        __raw_writel(chxctrl,MCSPI3_CHXCTRL(0));
    }
    return ;
}		/* -----  end of function omap3_mcspi_enable  ----- */

static int do_spi( cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    int port,channel;
    u32 data;
    if(argc != 5 && argc != 4){
        printf("usage:spi port channel r/w [data]\n");
        return -1;
    }

    port = simple_strtoul(argv[1],NULL,10);
    channel = simple_strtoul(argv[2],NULL,10);
    if(port == 1 && channel == 0){
        omap3_mcspi_setup_transfer(port,9,50000);
        if(argv[3][0] == 'w'){
            data = simple_strtoul(argv[4],NULL,16);
            printf("data = %#x\n",data);
            omap3_mcspi_send_sync(port,(u8*)&data,2);
        }
    }
    return 0;

}

U_BOOT_CMD(
        spi,	CFG_MAXARGS,	1,	do_spi,
        "spi - spi interface test\n"
        "usage:spi port channel r/w [data]\n",
);
