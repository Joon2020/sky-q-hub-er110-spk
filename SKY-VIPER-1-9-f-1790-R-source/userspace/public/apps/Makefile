#***********************************************************************
#
#  Copyright (c) 2006  Broadcom Corporation
#  All Rights Reserved
#
#  <:label-BRCM:2006:DUAL/GPL:standard
#  
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License, version 2, as published by
#  the Free Software Foundation (the "GPL").
#  
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#  
#  
#  A copy of the GPL is available at http://www.broadcom.com/licenses/GPLv2.php, or by
#  writing to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
#  Boston, MA 02111-1307, USA.
#  
#  :>
#
#***********************************************************************/


#
# You do not need to modify this Makefile to add an app.
# Simply create a .autodetect in the subdir and this Makefile will enter
# that directory during make and make clean.  (Auto detected directories
# are listed in the AUTODETECT_SUBDIRS variable.  Type make show_subdirs)
#
HARDCODED_SUBDIRS := dhcpv6 sshd ledctl dhrystone tcpdump \
                     sendpackets tinysvcmdns

SUBDIRS = $(HARDCODED_SUBDIRS) $(AUTODETECT_SUBDIRS) $(MODSW_SUBDIRS)


# === this is the default target ===
# Need to add a level of indirection here so that AUTODETECT_SUBDIRS and
# MODSW_SUBDIRS can be populated in make.common/make.modsw before we build.
#
all : make_all_subdirs


#
# Set our CommEngine directory (by splitting the pwd into two words
# at /userspace and taking the first word only).
# Then include the common defines under CommEngine.
# You do not need to modify this part.
#
CURR_DIR := $(shell pwd)
BUILD_DIR:=$(subst /userspace, /userspace,$(CURR_DIR))
BUILD_DIR:=$(word 1, $(BUILD_DIR))

# automatically set BCM_BASE_BUILD_DIR so that the modsw dirs can compile
BCM_BASE_BUILD_DIR := $(BUILD_DIR)
export BCM_BASE_BUILD_DIR


include $(BUILD_DIR)/make.common

ifneq ($(strip $(BUILD_VODSL)),)
include $(BUILD_DIR)/make.voice
endif

export BUILD_DIR
export CFLAGS


#
# We need to declare these directory names as phony targets,
# otherwise make will think the directories themselves are
# the targets and will never execute the cmds.
# See GNU Make, 4.6 
#
.PHONY : $(SUBDIRS)
make_all_subdirs : $(SUBDIRS) pppd


clean: generic_subdir_clean
	$(MAKE) -C ppp/pppoe clean

modsw_clean: generic_modsw_base_clean


##############################################################
# 
# Rules for inidividual apps
# apps can be compiled or not depending on the profile (which
# is modified via make menuconfig)
#
##############################################################


$(AUTODETECT_SUBDIRS) $(MODSW_SUBDIRS): 
	$(MAKE) -C $@


ifneq ($(strip $(BUILD_SSHD)),)
sshd:
	(tar xkjf sshd.tar.bz2 2> /dev/null || true)
	$(MAKE) -C sshd
else
sshd:
	@echo "skipping sshd (not configured)"
endif

ifneq ($(strip $(BUILD_IPV6)),)
dhcpv6:
	(tar xkjf dhcpv6.tar.bz2 2> /dev/null || true)
	$(MAKE) -j1 -C dhcpv6
else
dhcpv6:
	@echo "skipping dhcpv6 (not configured)"
endif


ifneq ($(strip $(BUILD_PPPD)),)
pppd:
	$(MAKE) -C ppp/pppoe $(BUILD_PPPD)
else
pppd:
	@echo "skipping pppd (not configured)"
endif

ifneq ($(strip $(BUILD_LEDCTL)),)
ledctl:
	$(MAKE) -C $@
else
ledctl:
	@echo "skipping $@ (not configured)"
endif

ifneq ($(strip $(BENCHMARKS_SMP)),)
dhrystone:
	$(MAKE) -C $@
else
dhrystone:
	@echo "skipping dhrystone (not configured)"
endif

ifneq ($(strip $(BUILD_SENDPACKETS)),)
sendpackets:
	$(MAKE) -C sendpackets install
else
sendpackets:
	@echo "skipping sendpackets (not configured)"
endif

ifneq ($(strip $(BUILD_TCPDUMP)),)
tcpdump:
	ln -sf tcpdump tcpdump-4.1.1
	(tar xkz  -f tcpdump.tar.gz 2> /dev/null || true)
	rm -f tcpdump-4.1.1
	$(MAKE) -C tcpdump install
else
tcpdump:
	@echo "skipping tcpdump (not configured)"
endif

ifneq ($(strip $(BUILD_SKYLSNET)),)
lsnet:
	$(MAKE) -C $@
else
lsnet:
	@echo "skipping lsnet (not configured)"
endif
ifneq ($(strip $(BUILD_TINYSVCMDNS)),)
tinysvcmdns:
	$(MAKE) -C $@
else
tinysvcmdns:
	@echo "skipping tinysvcmdns (not configured)"
endif
