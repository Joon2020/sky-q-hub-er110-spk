CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

ALLOWED_INCLUDE_PATHS := -I.\
                         -I$(BUILD_DIR)/userspace/public/apps/lsnet \
                         -I$(BUILD_DIR)/userspace/public/apps/lsnet/include \
                         -I$(BUILD_DIR)/userspace/public/apps/lsnet/lib \
                         -I$(BUILD_DIR)/kernel/linux/include \
                         -I$(BUILD_DIR)/kernel/linux/arch/mips/include \
                         -I$(TOOLCHAIN)/include

UTILOBJ=utils.o rt_names.o ll_types.o ll_proto.o ll_addr.o inet_proto.o

#NLOBJ=ll_map.o libnetlink.o
NLOBJ=ll_map.o libnetlink.o

all: libskynetlink.a libskyipr2util.a
#all: libnetlink.a 

libskynetlink.a: $(NLOBJ)
	$(AR) rcs $@ $(NLOBJ)

#libnetlink.o: libnetlink.c
#	$(CC) -c -Wall $(ALLOWED_INCLUDE_PATHS) libnetlink.c

#ll_map.o: ll_map.c
#	$(CC) -c -Wall $(ALLOWED_INCLUDE_PATHS) ll_map.c

libskyipr2util.a: $(UTILOBJ) $(ADDLIB)
	$(AR) rcs $@ $(UTILOBJ) $(ADDLIB)

%.o: %.c
	$(CC) $(ALLOWED_INCLUDE_PATHS) -c $< -o $@
	

clean:
	rm -f $(NLOBJ) $(UTILOBJ) $(ADDLIB) libskynetlink.a libskyipr2util.a
