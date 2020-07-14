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

VERSDIR=../..
OBJROOT=$(VERSDIR)/BUILD
SYMROOT=$(VERSDIR)/BUILD

DIR=KERNEL/conf/tools
INSTALL_DIR=doconf
SUBDIR=	newvers $(INSTALL_DIR)
DSTROOT=$(VERSDIR)/BUILD

#
# Include file for directory makefiles
#
findfile MakeInc.dir

#
# Get a usable version of doconf for the installhdrs target.
#
pre_install:	ALWAYS OBJROOT
	@CWD=`pwd`;							     \
	cd ${OBJROOT}; OBJROOT=`pwd`; cd $$CWD;				     \
	cd ${VERSDIR}; VERSDIR=`pwd`; cd $$CWD;				     \
	echo '#############' make $@ for $(DIR) '###############';  	     \
	make install SUBDIR=$(INSTALL_DIR)				     \
		OBJROOT=$(OBJROOT)					     \
		SYMROOT=$(SYMROOT)					     \
		DSTROOT=$(DSTROOT)

