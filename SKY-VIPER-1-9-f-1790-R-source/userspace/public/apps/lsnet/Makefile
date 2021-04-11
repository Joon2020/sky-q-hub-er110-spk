#
# In most cases, you only need to modify this first section.
#
EXE = lsnet
OBJS = main.o
SUBDIRS := lib

LIB_LOCAL_PATH = -Wl,-allow-shlib-undefined -L./lib/libskynetlink -L./lib/libskyipr2util

LIBS = -lcms_msg $(CMS_COMMON_LIBS) -lskynetlink -lskyipr2util -lpthread
#LIBS = -lcms_msg $(CMS_COMMON_LIBS) $(CMS_CORE_LIBS) -lnetlink -lpthread

all dynamic install: $(STATIC_CHECK) gen-libs $(EXE) generic_exe_install

clean: generic_clean
	rm -f $(INSTALL_DIR)/bin/$(EXE) *.o *.d

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

export $(BUILD_DIR)

include $(BUILD_DIR)/make.common


#
# Private apps and libs are allowed to include header files from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BUILD_DIR)/userspace/public/include  \
                         -I$(BUILD_DIR)/userspace/public/include/$(OALDIR)
#                         -I$(BUILD_DIR)/userspace/private/include  \
#                         -I$(BUILD_DIR)/userspace/private/include/$(OALDIR)



#
# Private apps and libs are allowed to link with libraries from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_LIB_DIRS := /lib:/lib/private:/lib/public:./lib


# ssk.c needs cms_boardioctl.h, which needs these headers
CFLAGS += -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD)  \
          -I$(INC_BRCMDRIVER_PRIV_PATH)/$(BRCM_BOARD) \
          -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD)


gen-libs:
	$(MAKE) -C lib

generic_clean:
	$(MAKE) clean -C lib 

#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
# 
$(EXE): $(OBJS)
	$(CC) -o $@ $(OBJS) -Wl,-rpath,$(CMS_LIB_RPATH) $(CMS_LIB_PATH) -L$(BUILD_DIR)/userspace/public/apps/lsnet/lib $(LIBS)
#	$(CC) -o $@ $(OBJS) $(LDFLAGS) $(ALLOWED_LIB_DIRS)$(LIBS)


splint:
	splint *.c \
	$(CFLAGS) \
	$(CPPFLAGS) \
	-E \
	-DSPLINT_CHECK \
	-I/opt/toolchains/uclibc-crosstools-gcc-4.4.2-1/usr/include -I$(BUILD_DIR)/dev_tools/splint $(ALLOWED_INCLUDE_PATHS) \
	-sysdirs /opt/toolchains/uclibc-crosstools-gcc-4.4.2-1/usr/include:$(BUILD_DIR)/userspace/public/include \
	-f $(BUILD_DIR)/dev_tools/splint/splint_config.txt \
	-compdestroy \
	-usedef \
	-compmempass \
	+voidabstract

#
# Include the rule for making dependency files.
# The '-' in front of the second include suppresses
# error messages when make cannot find the .d files.
# It will just regenerate them.
# See Section 4.14 of Gnu Make.
#

include $(BUILD_DIR)/make.deprules

-include $(OBJS:.o=.d)


