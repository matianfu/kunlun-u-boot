#ifndef LG_LCD_H
#define LG_LCD_H

#include <common.h>

/** provide padconf and gpio number to lg lcd module
    checkmux and print is not implemented yet, pass 0 **/
void lg_3wire_init(u32 pd_cs, int io_cs, u32 pd_scl, int io_scl, u32 pd_sda, int io_sda,
                       int checkmux, int print);

/** init all regs according to datasheet recommendations **/
/** default setting CABC off, turn it on in kernel if neccessary **/
/** pass 1 to enable normalblack or 0 for normal white **/
void lg_3wire_init_regs(int normalblack);


#endif


