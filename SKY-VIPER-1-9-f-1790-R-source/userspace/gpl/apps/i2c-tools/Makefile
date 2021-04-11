# I2C tools for Linux
#
# Copyright (C) 2007-2012  Jean Delvare <jdelvare@suse.de>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

DESTDIR	= $(INSTALL_DIR)
prefix	= 
bindir	= $(prefix)/bin
sbindir	= $(prefix)/sbin

INSTALL		:= cp
INSTALL_PROGRAM	:= $(INSTALL) 
RM		:= rm -f

CFLAGS	?= -O2
# When debugging, use the following instead
#CFLAGS	:= -O -g
CFLAGS	+= -Wall

KERNELVERSION	:= $(shell uname -r)

.PHONY: all strip clean install uninstall

all:

EXTRA	:=
#EXTRA	+= py-smbus
SRCDIRS	:= tools $(EXTRA)
include $(SRCDIRS:%=%/Module.mk)
