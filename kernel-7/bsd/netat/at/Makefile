#
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
#

# COMPONENT_NAME: AppleTalk
#
# FUNCTIONS:
#
# ORIGINS: Apple
#
# Header files in this directory that need to be accessible to both user
#  and kernel components are listed below.
#

.include <${MAKETOP}bos/kernext/Kernext.mk>


# all files in this dir should be listed here and will be installed,
# if a new header file needs to be added for export but not
# installation, put it in atallk/h and add to export list

INCLUDES = \
		adsp.h\
		appletalk.h\
		asp_errno.h\
		asp_if.h\
		at_atp.h\
		at_lap.h\
		at_pap.h\
		atp.h\
		ddp.h\
		elap.h\
		ep.h\
		nbp.h\
		pap.h\
		zip.h\
		aurp.h

IDIR		= /usr/include/at/
ILIST		= ${INCLUDES}
IMODE		= 644

EXPDIR		= /usr/include/at/

.include <${RULES_MK}>
