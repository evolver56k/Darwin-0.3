##
# Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
#
# @APPLE_LICENSE_HEADER_START@
# 
# Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
# Reserved.  This file contains Original Code and/or Modifications of
# Original Code as defined in and that are subject to the Apple Public
# Source License Version 1.1 (the "License").  You may not use this file
# except in compliance with the License.  Please obtain a copy of the
# License at http://www.apple.com/publicsource and read it before using
# this file.
# 
# The Original Code and all software distributed under the License are
# distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
# INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
# License for the specific language governing rights and limitations
# under the License.
# 
# @APPLE_LICENSE_HEADER_END@
##

#
# Makefile include file for shell scripts
# Copyright NeXT, Inc.  1989, 1990.  All rights reserved.
#

#
# Standard targets
#
ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

.DEFTARGET:	all

all:

remake:	clean all

install: DSTROOT $(DSTDIRS) $(OBJROOT) all installhdrs
	-$(RM) $(RMFLAGS) $(OBJROOT)/$(PROGRAM).VERS
	sed -e "s/#PROGRAM.*/#`vers_string $(PROGRAM)`/" \
	    <$(PROGRAM).csh >$(OBJROOT)/$(PROGRAM).VERS
	install $(IFLAGS) $(OBJROOT)/$(PROGRAM).VERS \
		$(DSTROOT)$(BINDIR)/$(PROGRAM)
	-$(RM) $(RMFLAGS) $(OBJROOT)/$(PROGRAM).VERS

reinstall: clean install

clean:	ALWAYS
	-$(RM) $(RMFLAGS) $(OBJROOT)/$(PROGRAM).VERS

clobber: clean
	-$(RM) $(RMFLAGS) $(GARBAGE)
	
print:	ALWAYS
	for i in $(PRINTFILES); \
	do \
		expand -$(TABSIZE) $$i >/tmp/$$i; \
	done; \
	cd /tmp; \
	enscript $(ENSCRIPTFLAGS) $(PRINTFILES); \
	$(RM) $(RMFLAGS) $(PRINTFILES); \
	touch PrintDate.$(USER)

update:	PrintDate.$(USER)

tags vgrind:

#
# Internal targets
#
PrintDate.$(USER): $(PRINTFILES)
	for i in $?; \
	do \
		expand -$(TABSIZE) $$i >/tmp/$$i; \
	done; \
	cd /tmp; \
	enscript $(ENSCRIPTFLAGS) $(PRINTFILES); \
	$(RM) $(RMFLAGS) $(PRINTFILES); \
	touch PrintDate.$(USER)

DSTROOT:
	@if [ -n "$($@)" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi

$(DSTDIRS) $(OBJROOT):
	$(MKDIRS) $@

ALWAYS:
