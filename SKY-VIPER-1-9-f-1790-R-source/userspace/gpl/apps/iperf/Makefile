
all:	build

CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))
include $(BUILD_DIR)/make.common

CXX := $(CC)
CFLAGS := -lstlport
CXXFLAGS := -lstlport
LDFLAGS := -lstlport -L$(USERSPACE_PUBLIC_LIBS_DIR)/stlport/lib

export CXX CFLAGS CXXFLAGS LDFLAGS CC USERSPACE_PUBLIC_LIBS_DIR

build:
	(tar xkfz iperf-3.0.11.tar.gz 2> /dev/null || true)
	if [ ! -e ./iperf-3.0.11/Makefile ];then \
	cd ./iperf-3.0.11; ./configure --host=mips-linux-uclibc --target=mips-linux-uclibc ac_cv_func_malloc_0_nonnull=yes; \
	fi
	echo "configure is done"
	$(MAKE) -C ./iperf-3.0.11 srcdir=./ builddir=./
	mkdir -p $(INSTALL_DIR)/lib/gpl
	install -m 755 ./iperf-3.0.11/src/.libs/libiperf.so.0.0.0 $(INSTALL_DIR)/lib/gpl/libiperf.so.0
	install -m 755 ./iperf-3.0.11/src/.libs/iperf3 $(INSTALL_DIR)/bin/iperf3

clean:
	if [ -f ./iperf-3.0.11/Makefile ];then $(MAKE) -C ./iperf-3.0.11 clean srcdir=./ builddir=./;fi;

