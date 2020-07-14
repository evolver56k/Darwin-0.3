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
# gentoc.sh
# - generate a table of contents file
#
# HISTORY
#
# 6-Aug-96      Dieter Siegmund (dieter@next)
#      Created.
#

[ "$1" = "" ] && {
    echo "Usage: `basename $0` <driver source root>"
    exit 1
}

INPUT_ROOT=$1
DRIVER_INFO_FILE=$INPUT_ROOT/DriverInfo
DIR_NAME=`expr "$0" : '\(.*\)/.*'`

[ -d "$INPUT_ROOT" ] || {
    echo "$0: driver input root directory '$INPUT_ROOT' does not exist"
    exit 1
}

# source in the common version retrieval code
if [ ",$DIR_NAME" = "," ]
then
	. getvers.sh
else
	. ${DIR_NAME}/getvers.sh
fi

#
# generate Table of Contents
#
cat <<EOT
{\rtf0\ansi{\fonttbl\f0\fswiss Helvetica;}
\paperw7900
\margl0
\margr0
\pard\tx360
EOT

for i in ${INPUT_ROOT}/English.lproj/*.strings
do
    LONG_NAME=`sed -n 's/.*"Long Name"[       ]*=[    ]*"\([^"]*\)";/\1/p' $i`
    LONG_NAME="$LONG_NAME (v$DRIVER_VERSION)"
    f=`basename $i | sed 's/.strings//'`
    [ "$f" = "Localizable" ] && {
	f=Default
    }
    RTFD_NAME=`fgrep 'Help File' ${INPUT_ROOT}/${f}.table | sed 's/.*=[ 	]*"\(.*\)".*/\1/'`
    [ "$RTFD_NAME" = "" ] && {
	echo "$0: \"Help File\" is not defined in $f.table - continuing" 1>&2 
	continue
    }
    [ -d "${INPUT_ROOT}/English.lproj/DriverHelp/${RTFD_NAME}" ] || {
	echo "$0: Help File in '${f}.table': '$RTFD_NAME' does not exist" 1>&2 
	exit 1
    }
    sed "s%<<LONG_NAME>>%$LONG_NAME%g; s%<<RTFD_NAME>>%$RTFD_NAME%g" <<EOT
{{\NeXTHelpLink \markername ;\linkFilename <<RTFD_NAME>>;\linkMarkername;}
,}\pard\tx360\f0\b\i0\ulnone\fs24\fc0\cf0 	<<LONG_NAME>>\\

EOT
done

cat <<EOT
}
EOT

exit 0
