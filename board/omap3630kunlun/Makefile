#
# (C) Copyright 2009 Texas Instruments.
#
# See file CREDITS for list of people who contributed to this
# project.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston,
# MA 02111-1307 USA
#

include $(TOPDIR)/config.mk

LIB	= lib$(BOARD).a

# UGlee, original, Sun Oct  7 11:58:53 CST 2012
# OBJS	:= \
	omap3630kunlun.o \
	sys_info.o \
	lcd.o \
	backlight.o \
	backlight_pwm.o \
	adc_calibration.o \
	camera.o \
	ispresizer.o \
	kunlun_mouse.o\
	gpio.o\
	lg_lcd.o

# UGlee, twl5030_pwm1 added, 		Sun Oct  7 11:58:24 CST 2012
# UGlee, omap_dss added, lcd.o removed	Sun Oct  7 17:46:59 CST 2012
OBJS	:= \
	omap3630kunlun.o \
	omap_dss.o \
	sys_info.o \
	backlight.o \
	backlight_pwm.o \
	twl5030_pwm1.o \
	adc_calibration.o \
	camera.o \
	ispresizer.o \
	kunlun_mouse.o\
	gpio.o\
	lg_lcd.o


$(LIB):	$(OBJS)
	$(AR) crv $@ $^

clean:
	rm -f $(OBJS)

distclean:	clean
	rm -f $(LIB) core *.bak .depend

#########################################################################

.depend:	Makefile $(OBJS:.o=.c)
		$(CC) -M $(CPPFLAGS) $(OBJS:.o=.c) > $@

-include .depend

#########################################################################
