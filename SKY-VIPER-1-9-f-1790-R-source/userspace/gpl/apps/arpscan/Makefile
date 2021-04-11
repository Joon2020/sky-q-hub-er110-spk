#
# In most cases, you only need to modify this first section.
#
EXE = arpscan
OBJS = arp-scan.o error.o hash.o utils.o wrappers.o link-packet-socket.o mt19937ar.o

	#obstack.o strlcat.o strlcpy.o utils.o wrappers.o link-packet-socket.o \
#OBJS = arp-scan.o error.o getopt1.o getopt.o hash.o inet_aton.o \
#	link-bpf.o link-dlpi.o link-packet-socket.o mt19937ar.o \
#	obstack.o strlcat.o strlcpy.o utils.o wrappers.o

LIBS =-lpcap

all dynamic install: $(EXE) generic_exe_install

clean: generic_clean
	rm -f $(INSTALL_DIR)/bin/$(EXE)

#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

include $(BUILD_DIR)/make.common


#
# Private apps and libs are allowed to include header files from the
# private and public directories.
#
# WARNING: Do not modify this section unless you understand the
# license implications of what you are doing.
#
ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BUILD_DIR)/userspace/public/libs/libpcap \
                         -I$(BUILD_DIR)/userspace/public/libs/libpcap/pcap

ALLOWED_LIB_DIRS := /lib:/lib/private:/lib/public

#
# Implicit rule will make the .c into a .o
# Implicit rule is $(CC) -c $(CPPFLAGS) $(CFLAGS)
# See Section 10.2 of Gnu Make manual
# 
$(EXE): $(OBJS)
	$(CC) -o $@ $(OBJS) -Wl,-rpath,$(CMS_LIB_RPATH) $(CMS_LIB_PATH) -DSKY -L$(BUILD_DIR)/userspace/public/libs/libpcap $(LIBS)


#
# Include the rule for making dependency files.
# The '-' in front of the second include suppresses
# error messages when make cannot find the .d files.
# It will just regenerate them.
# See Section 4.14 of Gnu Make.
#

include $(BUILD_DIR)/make.deprules

-include $(OBJS:.o=.d)


