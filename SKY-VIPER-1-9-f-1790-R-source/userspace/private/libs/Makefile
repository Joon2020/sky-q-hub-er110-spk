#***********************************************************************
#
#  Copyright (c) 2006  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/


#
# You do not need to modify this Makefile to add an app.
# Simply create a .autodetect in the subdir and this Makefile will enter
# that directory during make and make clean.  (Auto detected directories
# are listed in the AUTODETECT_SUBDIRS variable.  Type make show_subdirs)
#
HARDCODED_SUBDIRS := cms_core cms_dal cms_cli atmctl xdslctl bcmssl \
           omci mocactl \
           vlanctl gponif ethswctl cmfapi spuctl pwrctl omcipm_drv gponctl \
           nanoxml fcctl mdk212 eponsdk iqctl eponctl snoopctl gmacctl fapctl rdpactl

SKY_MALDIR := skymal

SUBDIRS = $(SKY_MALDIR) $(HARDCODED_SUBDIRS) $(AUTODETECT_SUBDIRS) $(MODSW_SUBDIRS)


# === this is the default target ===
# Need to add a level of indirection here so that AUTODETECT_SUBDIRS and
# MODSW_SUBDIRS can be populated in make.common/make.modsw before we build.
#
all : make_all_subdirs


#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

# automatically set BCM_BASE_BUILD_DIR so that the modsw dirs can compile
BCM_BASE_BUILD_DIR := $(BUILD_DIR)
export BCM_BASE_BUILD_DIR


include $(BUILD_DIR)/make.common

ifneq ($(strip $(BUILD_VODSL)),)
include $(BUILD_DIR)/make.voice
endif

export BUILD_DIR


#
# We need to declare these directory names as phony targets,
# otherwise make will think the directories themselves are
# the targets and will never execute the cmds.
# See GNU Make, 4.6 
#
.PHONY : $(SUBDIRS)
make_all_subdirs : $(SUBDIRS)


clean: generic_subdir_clean

modsw_clean: generic_modsw_base_clean


##############################################################
# 
# Rules for inidividual libs
# libs can be compiled or not depending on the profile (which
# is modified via make menuconfig)
#
##############################################################


$(AUTODETECT_SUBDIRS) $(MODSW_SUBDIRS): 
	$(MAKE) -C $@

### TESTING ifneq ($(strip $(BUILD_BRCM_CMS)),)
ifneq ($(strip $(BUILD_BRCM_CMS)),)

# always build this lib
cms_core:
	$(MAKE) -C $@

# always build this lib
cms_dal:
	$(MAKE) -C $@

# always build this lib
nanoxml:
	$(MAKE) -C $@


# Always build this lib, but if CLI_CMD and CLI_MENU is not defined
# it will be a very small lib (should be a zero length lib)
cms_cli:
	$(MAKE) -C $@

### TESTING ifneq ($(strip $(BUILD_BRCM_CMS)),)
endif


ifneq ($(strip $(BUILD_XTMCTL)),)
atmctl:
	$(MAKE) -C atmctl $(BUILD_XTMCTL)
else
atmctl:
	@echo "skipping atmctl (not configured)"
endif


ifneq ($(strip $(BUILD_XDSLCTL)),)
xdslctl:
	$(MAKE) -C xdslctl $(BUILD_XDSLCTL)
else
xdslctl:
	@echo "skipping xdslctl (not configured)"
endif


ifneq ($(strip $(BUILD_TR69C_BCM_SSL)),)
bcmssl: sanity_check
	(tar xkjf bcmssl.tar.bz2 2> /dev/null || true)
	$(MAKE) -C bcmssl
else
bcmssl: sanity_check
	@echo "skipping bcmssl (not configured)"
endif

ifneq ($(strip $(BUILD_OMCI)),)
omci: omcipm_drv
	$(MAKE) -C $@
omcipm_drv:
	$(MAKE) -C $@
else
omci:
	@echo "skipping omci (not configured)"
endif

ifneq ($(strip $(BUILD_GPONCTL)),)
gponctl:
	$(MAKE) -C $@
else
gponctl:
	@echo "skipping gponctl (not configured)"
endif

ifneq ($(strip $(BUILD_MoCACTL)),)
mocactl:
	$(MAKE) -C $@
else
mocactl:
	@echo "skipping mocactl (not configured)"
endif


ifneq ($(strip $(BUILD_VLANCTL)),)
vlanctl: 
	$(MAKE) -C $@
else
vlanctl:
	@echo "skipping vlanctl (not configured)"
endif

ifneq ($(strip $(BUILD_GPONIF)),)
gponif: 
	$(MAKE) -C $@
else
gponif:
	@echo "skipping gponif (not configured)"
endif

ifneq ($(strip $(BUILD_ETHSWCTL)),)
ethswctl: 
	$(MAKE) -C $@
else
ethswctl:
	@echo "skipping ethswctl (not configured)"
endif

ifneq ($(strip $(BUILD_CMFAPI)),)
cmfapi: 
	$(MAKE) -C $@
else
cmfapi:
	@echo "skipping cmfapi (not configured)"
endif

ifneq ($(strip $(BUILD_SPUCTL)),)
spuctl:
	$(MAKE) -C spuctl $(BUILD_SPUCTL)
else
spuctl:
	@echo "skipping spuctl (not configured)"
endif

ifneq ($(strip $(BUILD_PWRCTL)),)
pwrctl:
	$(MAKE) -C pwrctl $(BUILD_PWRCTL)
else
pwrctl:
	@echo "skipping pwrctl (not configured)"
endif

ifneq ($(strip $(BUILD_FCCTL)),)
fcctl: 
	$(MAKE) -C $@
else
fcctl:
	@echo "skipping fcctl (not configured)"
endif

ifneq ($(strip $(BUILD_EPON_SDK)),)
eponsdk: mdk212
	$(MAKE) -C $@
else
eponsdk:
	@echo "Skipping epon sdk "
endif

# Switch MDK
mdk212: sanity_check
ifneq ($(strip $(BUILD_SWMDK)),)
	if [ ! -e mdk212/RELEASE ]; then \
         echo untaring original mdk; \
	  (tar xkfj mdk212/$(ORIGINAL_MDK) 2> /dev/null || true); \
	fi
	$(MAKE) -j 1 -C $@
else
	@echo "Skipping switch mdk "
endif

# Ingress QoS
ifneq ($(strip $(BUILD_IQCTL)),)
iqctl: 
	$(MAKE) -C $@
else
iqctl:
	@echo "skipping iqctl (not configured)"
endif

# EPONCTL Library
ifneq ($(strip $(BUILD_EPONCTL)),)
eponctl: 
	$(MAKE) -C $@
else
eponctl:
	@echo "skipping eponctl (not configured)"
endif

# snoopctl Library
ifneq ($(strip $(BUILD_MCAST_SNOOP)),)
snoopctl: 
	echo Doing Make
	$(MAKE) -C snoopctl
else
snoopctl:
	@echo "skipping snoopctl (not configured)"
endif

# GMAC
ifneq ($(strip $(BUILD_GMACCTL)),)
gmacctl: 
	$(MAKE) -C $@
else
gmacctl:
	@echo "skipping gmacctl (not configured)"
endif

# FAP
ifneq ($(strip $(BUILD_FAPCTL)),)
fapctl: 
	$(MAKE) -C $@
else
fapctl:
	@echo "skipping fapctl (not configured)"
endif

ifneq ($(strip $(BUILD_RDPACTL)),)
rdpactl: 
	$(MAKE) -C $@
else
rdpactl:
	@echo "skipping rdpactl (not configured)"
endif

ifneq ($(strip $(BUILD_SKY_MESH_SUPPORT)),)
skymal: 
	$(MAKE) -C $@
else
skymal:
	@echo "skipping SKY MESH (not configured)"
endif




