#***********************************************************************
#
#  Copyright (c) 2012  Broadcom Corporation
#  All Rights Reserved
#
#***********************************************************************/

all dynamic install: conditional_build

ifeq ($(V),1)
  $(info building wireless-tools verbosely)
  Q =
else
  Q = @
endif

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

APP = wireless_tools.29

BUFFER_HACK := $(shell grep -q IW_SCAN_BUFFER_HACK ${KERNEL_DIR}/include/linux/wireless.h && echo BUFFER_HACK)
ifeq (${BUFFER_HACK}, BUFFER_HACK)
FLAGS += BUFFER_HACK=y
else
FLAGS += BUFFER_HACK=n
endif 

.PHONY: conditional_build 

ifneq ($(strip $/(BUILD_WIRELESS_TOOLS)),)

conditional_build: 
	@echo "Untarring original $(APP) source";
	$(Q)tar xkjf $(APP).tar.bz2 2> /dev/null || true;
	$(Q) $(MAKE) -C $(APP) all TOOLPREFIX=${CROSS_COMPILE} PREFIX=${INSTALL_DIR}  install-dynamic install-bin

else

conditional_build:
	@echo "skipping $(APP) (not configured)"

endif

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:


# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: 
	$(Q) $(MAKE) -C $(APP) clean


