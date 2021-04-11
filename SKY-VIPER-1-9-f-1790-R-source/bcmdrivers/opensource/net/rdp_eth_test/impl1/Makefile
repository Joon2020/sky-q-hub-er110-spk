# File: bcmdrivers/broadcom/net/rdpa/Makefile
#
# Makefile for the BCM968500 ETH driver
#

#Lilac BSP shared base
LILAC_BSP := $(SHARED_DIR)/opensource/bcm968500

#carrent module path
CURRENT_MODULE_PATH := $(BRCMDRIVERS_DIR)/opensource/net/rdp_eth_test/$(LN_NAME)

#module SRC files
# 1. from   $(LILAC_BSP)/drv
rdp_eth_test_drv_files := runner_data_path.c eth_data_path.c
	
# 2. from   $(LILAC_BSP)/rnr 
rdp_eth_test_rnr_files :=  bl_lilac_rdd.c		\
						   bl_rnr_fw_a.c       \
						   bl_rnr_fw_b.c       \
						   bl_rnr_fw_c.c       \
						   bl_rnr_fw_d.c

#DAN: probably temporary solution , but currently i can not figure out more elegant solution	
# We need 
 # a) add to module files from shared directory different locations 
 # b) we need leave the shared directory clean e.g. place all *.o to the module directory
#so we create link for each file using $(HOSTTOOLS_DIR)/make_links.sh script. The script will create link if it not exists or update it if source file was updated 


all_src := $(rdp_eth_test_drv_files) $(rdp_eth_test_rnr_files) 

#create file links 	
PHONY: makelinks

makelinks:
	$(HOSTTOOLS_DIR)/make_links.sh $(LILAC_BSP)/drv $(src) $(rdp_eth_test_drv_files)
	$(HOSTTOOLS_DIR)/make_links.sh $(LILAC_BSP)/rnr $(src) $(rdp_eth_test_rnr_files)

$(addprefix $(src)/,$(all_src)): makelinks

 
rdp_eth_test-objs	 := $(all_src:.c=.o)
					
rdp_eth_test-objs	 += lilac_eth.o 
           
obj-$(CONFIG_BCM_RDPETH_TEST) += rdp_eth_test.o

EXTRA_CFLAGS += -I. -I$(INC_BRCMSHARED_PUB_PATH)/bcm968500/drv -I$(INC_BRCMSHARED_PUB_PATH)/bcm968500/rnr
EXTRA_CFLAGS += -I$(INC_BRCMSHARED_PUB_PATH)/bcm963xx

EXTRA_CFLAGS += -Werror -Wfatal-errors


clean-files := $(addprefix $(src)/,$(all_src))

-include $(TOPDIR)/Rules.make

