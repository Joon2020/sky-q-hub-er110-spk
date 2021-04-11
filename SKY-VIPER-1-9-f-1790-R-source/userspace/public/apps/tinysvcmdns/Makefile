#
# Makefile for tinysvcmdns
#

EXE = tinysvcmdns
OBJS = mdns.o mdnsd.o main.o
LIBS = -lpthread 

all dynamic install: $(EXE) generic_exe_install

clean: generic_clean
	rm -f $(INSTALL_DIR)/bin/$(EXE) 

CUSTOM_CFLAGS += -Wall -pedantic -std=gnu99
CUSTOM_CFLAGS += -O2 -DNDEBUG

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common

#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
# 
$(EXE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)


