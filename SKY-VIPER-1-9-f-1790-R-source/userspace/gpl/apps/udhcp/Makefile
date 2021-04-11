# udhcp Makefile

#CROSS_COMPILE=mips-linux-
LDFLAGS = -Wl,--allow-shlib-undefined

prefix=/usr
SBINDIR=/sbin
USRSBINDIR=${prefix}/sbin
USRBINDIR=${prefix}/bin
USRSHAREDIR=${prefix}/share

INSTALL = install

VER := 0.9.6


OBJS_SHARED = options.o socket.o packet.o pidfile.o
DHCPD_OBJS = dhcpd.o arpping.o files.o leases.o serverpacket.o static_leases.o bcmqos.o
DHCPC_OBJS = dhcpc.o clientpacket.o script.o 

ifeq ($(strip $(SKY_MODEL_EE120)), y)

# Build only DHCP client for the Extender

CFLAGS += -DSKY_EXTENDER

EXEC1 = udhcpc
OBJS1 = $(DHCPC_OBJS) $(OBJS_SHARED)

else

# Build DHCP server and dumpleases command

EXEC2 = udhcpd
OBJS2 = $(DHCPD_OBJS) $(OBJS_SHARED)

EXEC3 = dumpleases
OBJS3 = dumpleases.o

DAEMONS = udhcpd
COMMANDS = dumpleases

endif

CFLAGS += -s -W -Wall -Wstrict-prototypes -DVERSION='"$(VER)"' -I$(INC_BRCMDRIVER_PUB_PATH)/$(BRCM_BOARD) -I$(INC_BRCMSHARED_PUB_PATH)/$(BRCM_BOARD) 
CFLAGS += -DBRCM_CMS_BUILD -DCMS_LOG3 -I$(BUILD_DIR)/userspace/public/include -I$(BUILD_DIR)/userspace/public/include/linux
CFLAGS += -I$(TOOLCHAIN)/include

# treat all warnings as errors
#CFLAGS += -Werror -Wfatal-errors

LIBS=-Wl,-rpath, -L$(INSTALL_DIR)/lib -L$(INSTALL_DIR)/lib/public -lcms_util -lcms_msg

#
# Use between the following 2 lines depending on whether you want verbose debug or not.
#
#CFLAGS += -g -DVERBOSE
CFLAGS += -Os -fomit-frame-pointer -fno-strict-aliasing
CFLAGS += $(BRCM_WERROR_CFLAGS) 
CFLAGS += $(SKY_FLAGS)
CFLAGS += $(SKY_CFLAGS)

ifeq ($(strip $(BUILD_UDHCP)), static)
CFLAGS += -DBUILD_STATIC
endif

ifeq ($(strip $(BUILD_UDHCP_RELAY)), y)
CFLAGS += -DDHCP_RELAY
DHCPD_OBJS += relay.o
endif

# BRCM begin
# all: $(AR1)
all: $(EXEC1) $(EXEC2) $(EXEC3)
# BRCM end

$(OBJS1) $(OBJS2) $(OBJS3): *.h Makefile
$(EXEC1) $(EXEC2) $(EXEC3): Makefile

.c.o:
	$(CC) -c $(CFLAGS) $<

$(EXEC1): $(OBJS1)
	$(CC) $(LDFLAGS) $(OBJS1) -o $(EXEC1) $(LIBS)

$(EXEC2): $(OBJS2)
	$(CC) $(LDFLAGS) $(OBJS2) -o $(EXEC2) $(LIBS)

$(EXEC3): $(OBJS3)
	$(CC) $(LDFLAGS) $(OBJS3) -o $(EXEC3)

ifeq ($(strip $(SKY_MODEL_EE120)), y)
install: udhcpc
	install -m 755 udhcpc $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/udhcpc
	ln -sf udhcpc $(INSTALL_DIR)/bin/dhcpc
else
install: udhcpd
	install -m 755 udhcpd $(INSTALL_DIR)/bin
	$(STRIP) $(INSTALL_DIR)/bin/udhcpd
	ln -sf udhcpd $(INSTALL_DIR)/bin/dhcpc
	ln -sf udhcpd $(INSTALL_DIR)/bin/dhcpd
endif

dynamic: all install

clean:
	-rm -f udhcpc *.o core udhcpd dumpleases




