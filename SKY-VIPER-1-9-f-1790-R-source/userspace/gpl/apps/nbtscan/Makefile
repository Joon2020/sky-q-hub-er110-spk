#
# build file 
#
#
CFLAGS = -s -Os -fomit-frame-pointer
CFLAGS += $(BRCM_WERROR_CFLAGS)
CFLAGS += -DHAVE_SIGNAL_H \
          -DHAVE_SYS_WAIT_H \
          -DHAVE_STDIO_H \
          -DHAVE_STDLIB_H \
          -DHAVE_CTYPE_H \
          -DHAVE_ERRNO_H \
          -DHAVE_SYSEXITS_H \
          -DHAVE_STRING_H \
          -DHAVE_GRP_H \
          -DHAVE_PWD_H \
          -DHAVE_SYS_STAT_H \
          -DHAVE_UNISTD_H \
          -DHAVE_STDARG_H \
          -DHAVE_SYSLOG_H \
          -DHAVE_ASSERT_H \
          -DHAVE_MALLOC_H \
          -DHAVE_SYS_MMAN_H \
          -DHAVE_ARPA_INET_H \
          -DHAVE_REGEX_H \
          -DHAVE_FCNTL_H \
          -DHAVE_SYS_SOCKET_H \
          -DHAVE_NETDB_H \
          -DHAVE_SYS_TYPES_H \
          -DHAVE_STDDEF_H \
          -Dmy_uint8_t=uint8_t \
          -Dmy_uint16_t=uint16_t \
          -Dmy_uint32_t=uint32_t 

LDFLAGS = -Wl,-allow-shlib-undefined

INCLUDES = ./
OBJS = nbtscan.o statusq.o range.o list.o

LIBS =
CFLAGS+=-Wall

all: nbtscan

install: nbtscan
	install -m 755 nbtscan $(INSTALL_DIR)/bin

dynamic: all install

static: nbtscan.a

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

nbtscan: $(OBJS)
	$(CC) -o nbtscan $(OBJS) $(LDFLAGS)

nbtscan.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

clean:
	-rm -f nbtscan *.o  nbtscan.a
