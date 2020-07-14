#!/bin/sh
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
# Copyright (c) 1996 NeXT Software, Inc.  All rights reserved. 
#
# getvers.sh
# - source'r must define DRIVER_INFO_FILE root before source'ing
# - sets DRIVER_VERSION OS_NAME OS_RELEASE
#
# HISTORY
#
# 1997-10-9	Godfrey van der Linden (gvdl@apple.com)
#	Rhapsodied it.
# 6-Aug-96      Dieter Siegmund (dieter@next)
#      Created.
#

if test ! -f "$DRIVER_INFO_FILE"
then
    echo "$cmdName: driver info file '$DRIVER_INFO_FILE' does not exist" 1>&2
    exit 1
fi

# retrieve the version and set the os name/release
. $DRIVER_INFO_FILE


cpp="/usr/bin/cc -E -arch `arch`"
cFile=/tmp/getvers.$$.m

trap 'rm -f $cFile' 0 1 2 15

cat > $cFile << EOI
#import <driverkit/IODevice.h>
iokit_getver=IO_DRIVERKIT_VERSION;
EOI

eval `($cpp $cFile 2> /dev/null) | fgrep iokit_getver | tr -d ' '`
eval DRIVER_VERSION='$DRIVER_VERSION_'$iokit_getver

rm -f $cFile
trap 0 1 2 15

if test -z "$DRIVER_VERSION"
then
    DRIVER_VERSION=$DEFAULT_DRIVER_VERSION
fi

if test -z "$DRIVER_VERSION"
then
    echo "$cmdName: '$DRIVER_INFO_FILE' contains no version string" 1>&2
    exit 1
fi

major=`expr "$iokit_getver" : '\(.\).*'`;
minor=`expr "$iokit_getver" : '.\(.\).*'`
OS_RELEASE="$major.$minor"
case "$OS_RELEASE" in
    3*)	OS_NAME="NEXTSTEP" ;;
    4*)	OS_NAME="OPENSTEP for Mach" ;;
    *)	OS_NAME="Rhapsody" ;;
esac
