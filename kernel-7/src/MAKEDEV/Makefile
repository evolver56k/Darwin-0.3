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
# Generic top-level makefile for shell scripts
# (c) NeXT, Inc.  1987, 1990.  All rights reserved.
#
# Who and where
#
PROGRAM= MAKEDEV
BINDIR=	/private/dev
DSTDIRS= $(DSTROOT)$(BINDIR)

#
# What to print and how
#
PRINTFILES= Makefile $(PROGRAM).csh
ENSCRIPTFLAGS=
TABSIZE=8

#
# What to remove on "clean"
#
GARBAGE= errs Make.out

#
# How to install it
#
IFLAGS= -c -m 555
ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

VERSDIR=../..
OBJROOT=$(VERSDIR)/BUILD
SYMROOT=$(VERSDIR)/BUILD

install:

#
# installhdrs
#
installhdrs:
	$(MKDIRS) $(DSTROOT)$(BINDIR) $(OBJROOT)
	-$(RM) $(RMFLAGS) $(OBJROOT)/$(PROGRAM).VERS
	sed -e "s/#PROGRAM.*/#`vers_string $(PROGRAM)`/" \
	<$(PROGRAM).csh >$(OBJROOT)/$(PROGRAM).VERS
	install $(IFLAGS) $(OBJROOT)/$(PROGRAM).VERS \
	$(DSTROOT)$(BINDIR)/$(PROGRAM)
	-$(RM) $(RMFLAGS) $(OBJROOT)/$(PROGRAM).VERS

#
# Include file for shell script makefiles
#
findfile MakeInc.csh
