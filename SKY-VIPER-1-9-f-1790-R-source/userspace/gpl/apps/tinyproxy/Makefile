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
          -DHAVE_STDDEF_H
LDFLAGS = -Wl,-allow-shlib-undefined
                   
ifeq ($(strip $(BUILD_IPV6)),y)
	CFLAGS += -DHAVE_NETINET_IN_H
	CFLAGS += $(SKY_CFLAGS)
endif

#CFLAGS += -DBUILD_STATIC
CFLAGS += -DFILTER_ENABLE
CFLAGS += -DTRANSPARENT_PROXY
CFLAGS += -DNDEBUG
INCLUDES = ./
OBJS = log.o main.o vector.o heap.o conf.o utils.o stats.o anonymous.o daemon.o sock.o child.o html-error.o acl.o hashmap.o connect-ports.o http-message.o network.o reqs.o conns.o buffer.o text.o filter.o transparent-proxy.o

LIBS =
CFLAGS+=-Wall

#OBJS = tinyproxy.o
# $(LDFLAGS) $(LIBS) 

all: tinyproxy

install: tinyproxy
	install -m 755 tinyproxy $(INSTALL_DIR)/bin
	install -m 755 etc/tinyproxy.conf $(INSTALL_DIR)/etc
	#install -m 777 etc/filter $(INSTALL_DIR)/etc  
	#$(STRIP) $(INSTALL_DIR)/bin/tinyproxy

dynamic: all install

static: tinyproxy.a

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<

tinyproxy: $(OBJS)
	$(CC) -o tinyproxy $(OBJS) $(LDFLAGS)

tinyproxy.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

clean:
	rm -f tinyproxy *.o  tinyproxy.a
