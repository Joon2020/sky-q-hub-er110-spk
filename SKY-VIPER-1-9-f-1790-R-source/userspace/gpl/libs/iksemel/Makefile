#***********************************************************************
#
#  Copyright SKY 2014
#  All Rights Reserved
#
#***********************************************************************/

all dynamic install: conditional_build

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

APP = iksemel-1.4-REPACK-1
APP_BASE = libiksemel

.PHONY: check_untar_patch_configure conditional_build check_versions

ifneq ($(strip $(BUILD_LIBIKSEMEL)),)


conditional_build:
	@if [ ! -e $(APP)/config.log ]; then \
	    echo "Untarring original $(APP) source"; \
	    tar xf $(APP).tar.gz 2> /dev/null || true; \
	    cd $(APP); \
	    echo "Patching from file in $(CURR_DIR) to $(APP)"; \
	    patch -p0 < $(CURR_DIR)/iksemel-getaddrinfo.patch;\
	    echo "Building $(APP) to $(INSTALL_DIR)/lib/gpl"; \
#	    export PKG_CONFIG_LIBDIR=$(INSTALL_DIR)/lib/gpl; \
#	    export PKG_CONFIG_PATH=$(INSTALL_DIR)/lib/gpl/pkgconfig; \
	    ./configure --host=mips-linux-uclibc \
	    --prefix=$(INSTALL_DIR) \
	    --sbindir=$(INSTALL_DIR)/bin \
	    --libdir=$(INSTALL_DIR)/lib/gpl \
#            OPENSSL_CFLAGS=-I$(BUILD_DIR)/userspace/public/libs/openssl/include \
#            OPENSSL_LIBS=-L$(INSTALL_DIR)/lib/public/\ -lssl\ -lcrypto\
#	    --with-openssl \
	    LDFLAGS=-L$(INSTALL_DIR)/lib/gpl \
	    CFLAGS=-s\ -Os\ -DSKY=1\ -Wno-strict-aliasing || exit 1; \
	fi; \
	cd $(APP); \
	$(MAKE) -j1; \
#	$(MAKE) -j1 install; \
	mkdir -p $(INSTALL_DIR)/lib/gpl; \
	cp src/.libs/libiksemel.so.3 $(INSTALL_DIR)/lib/gpl/libiksemel.so.3; \
#	cp src/.libs/libiksemel.so $(INSTALL_DIR)/lib/gpl/libiksemel.so; \
	echo "Done building $(APP_BASE)"
else
conditional_build:
	@echo "skipping $(APP) (not configured)"
endif

# NOTE: make clean from within app does not do a proper job, so wiping out
# entire directory to ensure consistency.
clean:
	if [ -d $(APP) ]; then \
		cd $(APP); \
		$(MAKE) clean; \
		$(MAKE) uninstall; \
	fi; \

# The next line is a hint to our release scripts
# GLOBAL_RELEASE_SCRIPT_CALL_DISTCLEAN
distclean: clean



