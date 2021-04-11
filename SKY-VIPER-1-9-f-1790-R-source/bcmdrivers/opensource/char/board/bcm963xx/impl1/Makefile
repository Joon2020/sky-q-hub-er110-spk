# Makefile for the bcm963xx board drivers
#


obj-y := board.o bcm63xx_flash.o bcm63xx_led.o bcm63xx_gpio.o

#for SKY MESH build 1eee1905_module, to recived push button notification in userspace
ifneq ($(strip $(BUILD_SKY_MESH_SUPPORT)),)
obj-y += ieee1905_module.o    
endif

ifneq ($(strip $(BUILD_IEEE1905)),)
obj-y += ieee1905_module.o    
endif

ifeq ($(strip $(BRCM_CHIP)),6816)
obj-y += spidevices.o
endif

ifeq ($(strip $(BRCM_CHIP)),6818)

ifneq ($(strip $(BUILD_6802_MOCA)),)
    obj-y += bbsi/bbsi.o    
    obj-y += bbsi/bbsiExecSeq.o   
    obj-y += moca/6802/init.o   
    
else
    obj-y += spidevices.o
endif

endif

ifeq ($(strip $(BRCM_CHIP)),63268)
ifneq ($(strip $(BUILD_6802_MOCA)),)
    obj-y += bbsi/bbsi.o    
    obj-y += bbsi/bbsiExecSeq.o   
    obj-y += moca/6802/init.o   
    
else    
obj-y += spidevices.o
endif

endif

ifneq ($(strip $(CONFIG_BCM_CPLD1)),)
    obj-y += bcmcpld1.o
	EXTRA_CFLAGS += -DCONFIG_BCM_CPLD1
endif
ifeq ($(strip ${WP_SERIAL_DATA}),1)
EXTRA_CFLAGS += -DENABLE_SERIALISATION_WP
endif
ifneq ($(strip ${ENABLE_REPAIR_SW}),)
	EXTRA_CFLAGS+= -DENABLE_REPAIR_SW
endif

EXTRA_CFLAGS += -I. -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)
ifeq ($(strip $(BRCM_CHIP)),6838)
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/pmc
endif
EXTRA_CFLAGS += -Werror -Wfatal-errors
EXTRA_CFLAGS += -g

ifneq ($(strip $(CONFIG_BCM_WLAN)),)
	EXTRA_CFLAGS += -DWIRELESS
endif

EXTRA_CFLAGS += $(SKY_CFLAGS)
-include $(TOPDIR)/Rules.make

clean:
	rm -f core *.o *.a *.s

