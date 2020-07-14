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
# veredit.sh
# - change all the versions in the .table files to 
#   the one stored in the DriverInfo file
# - sed the Help files to replace <<OS_NAME>> and <<OS_RELEASE>> with
#   correct values
# - sed the TableOfContents.rtf to replace vX.XX with the right release
#   (if TableOfContents.rtf exists)
#
# HISTORY
#
# 1997-10-9	Godfrey van der Linden (gvdl@apple.com)
#	Rhapsodied it.
#
# 6-Aug-96      Dieter Siegmund (dieter@next)
#      Created.
#

cmdName=`basename $0`
if test $# -ne 2
then
    echo "Usage: $cmdName <source root> <output dir>" 1>&2
    exit 1
fi

sourceRoot=$1
outputRoot=$2
tmpFile=/tmp/$$.veredit
dirName=`expr $0 : '\(.*\)/.*'`

if test ! -d "$sourceRoot"
then
    echo "$cmdName: directory '$sourceRoot' does not exist" 1>&2
    exit 2
fi

if test ! -d "$outputRoot"
then
    echo "$cmdName: directory '$outputRoot' does not exist" 1>&2
    exit 3
fi

# source in the common version retrieval code
DRIVER_INFO_FILE=$sourceRoot/DriverInfo
if test -z "$dirName"
then
	. getvers.sh
else
	. ${dirName}/getvers.sh
fi

trap 'rm -f $tmpFile; exit 1' 1 2 15

# update versions in table files
for table in $outputRoot/*.table
do
    thisVersion='^[       ]*"Version"[       ]*=[    ]*"\([0-9].[0-9][0-9]\)".*'
    thisVersion=`sed -n "s/$thisVersion/\1/p" $table`
    if test "$thisVersion" != "$DRIVER_VERSION"
    then
	if egrep '^[       ]*"Version"' $table 2>&1 >/dev/null
        then # Version string already there, edit it
	    sed "s/^[       ]*\"Version\".*/\"Version\" = \"$DRIVER_VERSION\";/" $table > $tmpFile
        else # 
	    cat $table > $tmpFile
	    echo "\"Version\" = \"$DRIVER_VERSION\";" >> $tmpFile
        fi
	rm -f $table
	mv $tmpFile $table
    fi
done

helpDir=$outputRoot/English.lproj/Help
toc=$helpDir/TableOfContents.rtf
if [ -d "$helpDir" ] 
then
    chmod +w $helpDir
    # update version in Table Of Contents file, if it exists
    if [ -f "$toc" ] 
    then
	sed "s/vX\.XX/v$DRIVER_VERSION/g" $toc > $tmpFile
	rm -f $toc
	mv $tmpFile $toc
    fi

    # update each of the *rtfd/TXT.rtf with the correct OS name
    for i in $helpDir/*.rtfd
    do
	chmod +w $i
	sed "s/<<OS_NAME>>/$OS_NAME/g; s/<<OS_RELEASE>>/$OS_RELEASE/g" $i/TXT.rtf > $tmpFile
	rm -f $i/TXT.rtf
	mv $tmpFile $i/TXT.rtf
    done
fi

rm -f $tmpFile

exit 0
