EXE = skydhcpc
dynamic: generic_exe_install

clean:
	rm -f skydhcpc


skydhcpc:
	cp skydhcpc_$(PROFILE)_save skydhcpc

CURR_DIR := $(shell pwd)
BUILD_DIR := $(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR := $(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common
