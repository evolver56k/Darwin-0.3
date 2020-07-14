/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */

NAME = include

SUBDIRS = arpa protocols

SOURCES = Makefile

FASTCP = /usr/lib/fastcp
RM     = /bin/rm
ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif
ECHO   = echo

PUBLIC_HEADERS = \
	a.out.h ar.h assert.h bitstring.h ctype.h db.h dirent.h disktab.h err.h \
	fnmatch.h fstab.h fts.h glob.h grp.h kvm.h limits.h locale.h math.h \
	memory.h mpool.h ndbm.h netdb.h nlist.h paths.h pwd.h ranlib.h regex.h \
	regexp.h resolv.h rune.h runetype.h setjmp.h sgtty.h signal.h stab.h \
	stddef.h stdio.h stdlib.h string.h strings.h struct.h sysexits.h \
	tar.h time.h ttyent.h tzfile.h unistd.h utime.h utmp.h vis.h 

all::

recurse::
	@echo '====' make $(TARGET) for: $(SUBDIRS) '===='
	@for i in ${SUBDIRS};  \
	do  \
		echo == make $(TARGET) for $$i ==;  \
		(cd $$i; ${MAKE} $(TARGET)  \
			OBJROOT="$$ABSOROOT/$$i" \
			SYMROOT="$$ABSSROOT/$$i" \
			DSTROOT="$(DSTROOT)" \
			SRCROOT="$(SRCROOT)/$$i" \
			RC_CFLAGS="$(RC_CFLAGS)" \
			RC_ARCHS="$(RC_ARCHS)" \
			ARGON_PUBLIC_HEADER="$(ARGON_PUBLIC_HEADER)/$$i" \
			ARGON_PRIVATE_HEADER="$(ARGON_PRIVATE_HEADER)/$$i" \
			|| exit 1);  \
	done
	@echo '====' make $(TARGET) for $(SUBDIRS) done '===='

installhdrs::
	$(MKDIRS) $(ARGON_PUBLIC_HEADER)
	@$(FASTCP) $(PUBLIC_HEADERS) $(ARGON_PUBLIC_HEADER)
	@$(MAKE) recurse "TARGET = $@"

install::

installsrc::

clean:
	@(cd $(ARGON_PUBLIC_HEADER); $(RM) -f $(PUBLIC_HEADERS))
	@$(MAKE) recurse "TARGET = $@"

copy::	DEST
	@$(MAKE) installsrc "SRCROOT = $(DEST)"

$(DSTROOT):;$(MKDIRS) -m 755 -o root -g wheel $@

$(SYMROOT):
	@$(MKDIRS) $(SYMROOT)

$(OBJROOT):
	@$(MKDIRS) $(OBJROOT)

SRCROOT DEST:
	@if [ -n "${$@}" ]; then exit 0; \
	else $(ECHO) Must define $@; exit 1; fi

	
