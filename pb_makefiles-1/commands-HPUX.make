##
# Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
#
# @APPLE_LICENSE_HEADER_START@
# 
# "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
# Reserved.  This file contains Original Code and/or Modifications of
# Original Code as defined in and that are subject to the Apple Public
# Source License Version 1.0 (the 'License').  You may not use this file
# except in compliance with the License.  Please obtain a copy of the
# License at http://www.apple.com/publicsource and read it before using
# this file.
# 
# The Original Code and all software distributed under the License are
# distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
# INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
# License for the specific language governing rights and limitations
# under the License."
# 
# @APPLE_LICENSE_HEADER_END@
##
#
# commands-HPUX.make
#
# commands needed by the makefiles on HPUX
#

NEXTLIB_BIN = $(NEXT_ROOT)$(SYSTEM_LIBRARY_EXECUTABLES_DIR)
NEXTDEV_BIN = $(NEXT_ROOT)$(SYSTEM_DEVELOPER_DIR)/Executables
HPDEV_BIN = /usr/ccs/bin
LOCAL_BIN = $(NEXT_ROOT)/local/bin

ECHO    = /bin/echo
NULL = /dev/null

CD = cd
RM = /bin/rm
LN = /bin/ln 
SYMLINK = /bin/ln -s
CP = /bin/cp
MV = /bin/mv
FASTCP = $(NEXTDEV_BIN)/fastcp
TAR	= $(NEXTLIB_BIN)/gnutar
MKDIRS	= /usr/bin/mkdir -p
CAT = /bin/cat
TAIL = /usr/bin/tail
TOUCH   = /bin/touch
FIND = /bin/find
GREP = /bin/grep
STRIP  = $(HPDEV_BIN)/strip
CHGRP   = /bin/chgrp -h
CHMOD   = /bin/chmod
CHOWN   = /bin/chown -h

ARCH_CMD = $(NEXTDEV_BIN)/arch
VERS_STRING = $(LOCAL_BIN)/vers_string
MERGEINFO = $(NEXTDEV_BIN)/mergeInfo
OFILE_LIST_TOOL = $(NEXTDEV_BIN)/ofileListTool 
FRAMEWORK_TOOL = $(NEXTDEV_BIN)/frameworkFlags
NEWER = $(MAKEFILEDIR)/newer
DOTDOTIFY = $(MAKEFILEDIR)/dotdotify
CLONEHDRS = $(MAKEFILEDIR)/clonehdrs
MIG = $(NEXTDEV_BIN)/mig
MSGWRAP = $(NEXTDEV_BIN)/msgwrap
PSWRAP = $(NEXTDEV_BIN)/pswrap
RPCGEN = /bin/rpcgen
LEX = $(HPDEV_BIN)/lex
YACC = $(HPDEV_BIN)/yacc
SED	= /bin/sed
TR = /bin/tr
CC	= $(NEXTDEV_BIN)/gcc
LD = $(CC)
AR = /bin/ar
RANLIB = $(HPDEV_BIN)/ranlib
LIBTOOL = $(NEXTDEV_BIN)/libtool

JAVATOOL = $(NEXTDEV_BIN)/javatool
BRIDGET = $(NEXTDEV_BIN)/bridget
GENFORCELOAD = $(NEXTDEV_BIN)/genforceload
GENFRAMEWORKSPLIST = $(NEXTDEV_BIN)/genframeworksplist
PLISTREAD = $(NEXTDEV_BIN)/plistread
BUILDFILTER = $(NEXTDEV_BIN)/BuildFilter
