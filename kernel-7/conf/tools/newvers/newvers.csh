#!/bin/sh -

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
# Mach Operating System
# Copyright (c) 1990 Carnegie-Mellon University
# Copyright (c) 1989 Carnegie-Mellon University
# All rights reserved.  The CMU software License Agreement specifies
# the terms and conditions for use and redistribution.
#

#
# newvers.sh	copyright major minor variant edit patch
#

major="$1"; minor="$2"; variant="$3"
v="${major}.${minor}" d=`pwd` h="rcbuilder" t=`date` w=`whoami`
if [ -z "$d" -o -z "$h" -o -z "$t" ]; then
    exit 1
fi
CONFIG=`expr "$d" : '.*/\([^/]*\)$'`
d=`expr "$d" : '.*/\([^/]*/[^/]*/[^/]*\)$'`
(
  /bin/echo "int  version_major      = ${major};" ;
  /bin/echo "int  version_minor      = ${minor};" ;
  /bin/echo "char version_variant[]  = \"${variant}\";" ;
  /bin/echo "char version[] = \"Kernel Release ${v}:\\n${t}; $w\\nCopyright (c) 1988-1995,1997-1999 Apple Computer, Inc. All Rights Reserved.\\n\\n\";" ;
  /bin/echo "char osrelease[] = \"${major}.${minor}\";" ;
  /bin/echo "char ostype[] = \"Darwin\";" ;
) > vers.c
if [ -s vers.suffix -o ! -f vers.suffix ]; then
    rm -f vers.suffix
    echo ".${variant}.${CONFIG}" > vers.suffix
fi
exit 0
