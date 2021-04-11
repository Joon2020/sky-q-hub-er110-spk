#
# <:copyright-BRCM:2012:DUAL/GPL:standard
# 
#    Copyright (c) 2012 Broadcom Corporation
#    All Rights Reserved
# 
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2, as published by
# the Free Software Foundation (the "GPL").
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# 
# A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
# writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.
# 
# :> 
#

#
# Makefile for the Godenrod board common
#

# remove optimization for debug 
#EXTRA_CFLAGS += -O0 -g

BRD = brd
DPD = dpd
DRV = drv
OS  = os
REGS = registers
RNR = rnr
SOC = soc

BRD_OBJ  := $(BRD)/bl_lilac_brd.o               

DPD_OBJ  := $(DPD)/bl_lilac_drv_mac.o           \
            $(DPD)/bl_lilac_drv_serdes.o		\
            $(DPD)/hwapi_lilac_mac.o
            

DRV_OBJ  := $(DRV)/bl_lilac_bat.o              \
            $(DRV)/bl_lilac_i2c.o              \
            $(DRV)/bl_lilac_otpm.o             \
            $(DRV)/bl_lilac_rst.o              \
            $(DRV)/bl_lilac_vpb_bridge.o       \
            $(DRV)/bl_lilac_ic.o               \
            $(DRV)/bl_lilac_pin_muxing.o       \
            $(DRV)/bl_lilac_spi.o              \
            $(DRV)/bl_lilac_gpio.o             \
            $(DRV)/bl_lilac_nand.o             \
            $(DRV)/bl_lilac_wd.o               \
            $(DRV)/bl_lilac_timer.o            

SOC_OBJ  := $(SOC)/bl_lilac_soc.o

REGS_OBJ := $(REGS)/bl_lilac_mips_blocks_arrays.o \
            $(REGS)/bl_lilac_net_if_arrays.o      \
            $(REGS)/bl_lilac_pcie_arrays.o        \
            $(REGS)/bl_lilac_serdes_arrays.o

NET_OBJ	 := $(DRV)/bl_lilac_eth_common.o 
           
obj-y	 := $(REGS_OBJ)   \
            $(BRD_OBJ)    \
            $(DRV_OBJ)    \
            $(SOC_OBJ)    \
            $(DPD_OBJ)    \
            $(NET_OBJ)	
			
obj-$(CONFIG_PCI)	 += $(DRV)/bl_lilac_pci.o
obj-$(CONFIG_USB)	 += usb/bl_lilac_usb_hc.o  
 
EXTRA_CFLAGS 	+= -I$(INC_BRCMSHARED_PUB_PATH)/bcm963xx
EXTRA_CFLAGS 	+= -I$(INC_RDPA_PATH)  -I$(INC_BDMF_PATH)/framework

-include $(TOPDIR)/Rules.make

