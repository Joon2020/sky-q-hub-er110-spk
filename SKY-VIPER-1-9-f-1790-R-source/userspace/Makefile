#***********************************************************************
#
#  Copyright (c) 2007  Broadcom Corporation
#  All Rights Reserved
#  No portions of this material may be reproduced in any form without the
#  written permission of:
#          Broadcom Corporation
#          5300 California Avenue
#          Irvine, California 92617
#  All information contained in this document is Broadcom Corporation
#  company private, proprietary, and trade secret.
#
#***********************************************************************/

#
# You probably don't need to modify anything in this Makefile.
#

APPS:=public-apps private-apps gpl-apps
LIBS:=public-libs private-libs gpl-libs
DIRS:=public private gpl

.PHONY : $(APPS) $(LIBS) $(DIRS) public-include

all    : $(APPS) $(LIBS) 


$(APPS) : $(LIBS)
$(LIBS) : public-include

public-include:
	$(MAKE) -C public/include

private-libs:
	$(MAKE) -C private/libs

public-libs:
	$(MAKE) -C public/libs

gpl-libs:
	$(MAKE) -C gpl/libs

gpl-apps:
	$(MAKE) -C gpl/apps

public-apps:
	$(MAKE) -C public/apps

private-apps:
	$(MAKE) -C private/apps


gpl: gpl-libs gpl-apps

private: private-libs private-apps

public: public-libs public-apps public-include


#
# Cleaning rules
#
clean: 
	@for dir in $(DIRS); do \
		for apporlib in libs apps; do \
			$(MAKE) -C $$dir/$$apporlib clean; \
		done \
	done
	@$(MAKE) -C public/include clean