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
# This had better be run with a bourne shell equivalent or bad things
# is going to happen.  However I refuse to specify the shell on the grounds
# that shell may move around in the future.
#

# Attempt to catch out the case when we don't have a bourne shell
set -e

# Establish the environment
ARCH_CMD=${ARCH_CMD:-/usr/bin/arch}
BASENAME=${BASENAME:-/usr/bin/basename}
CAT=${CAT:-/bin/cat}
CHMOD=${CHMOD:-/bin/chmod}
ECHO=${ECHO:-echo}
FASTCP=${FASTCP:-/usr/lib/fastcp}
FILE=${FILE:-/usr/bin/file}
FIND=${FIND:-/usr/bin/find}
LIPO=${LIPO:-/bin/lipo}
MV=${MV:-/bin/mv}
RM=${RM:-/bin/rm}

#
# Establish the commands that have moved around between 4.2 mach and Rhapsody
#
if [ -z "$MKDIRS" ]
then
    if [-x /bin/mkdirs ]
    then
        MKDIRS=/bin/mkdirs
    else
        MKDIRS="/bin/mkdir -p"
    fi
fi

if [ -z "$CHOWN" ]
then
    if [ -x /usr/sbin/chown ]
    then
        CHOWN=/usr/sbin/chown
    else
        CHOWN=/etc/chown
    fi
fi

if [ -z "$CHGRP" ]
then
    if [ -x /usr/bin/chgrp ]
    then
        CHGRP=/usr/bin/chgrp
    else
        CHGRP=/bin/chgrp
    fi
fi

if [ -z "$SED" ]
then
    if [ -x /usr/bin/sed ]
    then
        SED=/usr/bin/sed
    else
        SED=/bin/sed
    fi
fi

set +e

cmdName=`eval ${BASENAME} $0`

usage()
{
    >&2 ${CAT} << EOusage
Usage: $cmdName -driver <driver directory>
    -archs <architecure list> [-binaries <binary list>
    -perms <install mode> -owner <install owner> -group <install group>]

This script will pickup and use the following environment variables
    ARCH_CMD BASENAME CAT CHMOD CHGRP CHOWN ECHO
    FASTCP FILE FIND LIPO MKDIRS MV RM SED
EOusage
    exit 1
}

# Routine that echos a comment and bombs
die()
{
    code=$1; shift
    >&2 ${ECHO} "$cmdName: $*"
    exit $code;
}

for arg
do
    case $arg in
    -driver)
        argCmd='driverPath="$driverPath '
        ;;
    -archs)
        argCmd='archList="$archList '
        ;;
    -binaries)
        argCmd='binList="$binList '
        ;;
    -perms)
        argCmd='installPerms="$installPerms '
        ;;
    -owner)
        argCmd='installOwner="$installOwner '
        ;;
    -group)
        argCmd='installGroup="$installGroup '
        ;;
    *)
        [ -n "$argCmd" ] || usage
	eval "$argCmd$arg\""
        ;;
    esac
done

# Validate arguments
eval "set $driverPath";
[ $# -eq 1 -a -n "$driverPath" -a -d "$1" ] \
    || die 1 Must specify a target driver directory

eval "set $installPerms"; [ $# -le 1 ] || usage
eval "set $installOwner"; [ $# -le 1 ] || usage
eval "set $installGroup"; [ $# -le 1 ] || usage

curDir=`pwd`
cd $driverPath/.. || die 2 "Couldn't change to $driverPath/.. directory"
driverName=`eval "${BASENAME} $driverPath"`

[ -n "$archList" ]  || archList=`${ARCH_CMD}`
eval "set $archList";
if [ $# -eq 1 ]
then
    #
    # Only have to move the current Driver to the architecture
    # specific directory.
    #
    targetArch=$1
    ${ECHO} Moving $driverName to $targetArch
    eval "${MKDIRS} $targetArch"
    ${CHMOD} +w $driverName || die 2 "Couldn't chmod to $driverName"
    ${MV} $driverName $targetArch/$driverName \
	 || die 2 "Couldn't move $driverName to $targetArch/$driverName"
    ${CHMOD} -w $targetArch/$driverName
    exit 0
fi


#
# We know we are multi architecture now
#
doInstall()
{
    [ -z "$installPerms" ] \
        || ( ${CHMOD} -R ugo-w $1 && ${CHMOD} -R $installPerms $1 ) || :
    [ -z "$installGroup" ] \
        || ${CHGRP} -R $installGroup $1 || :
    [ -z "$installOwner" ] \
        || ${CHOWN} -R $installOwner $1 || :
}

findArchForFamilyInFile()
{
    family=$1; shift
    file=$1;   shift

    ${FILE} $file | ${SED} -n "s/^.*(for architecture \(.*\)):.* $family\$/\1/p"

    unset family
    unset file
}

#
# Force a halt if we have any problems at all in this the thinning section
#
set -e
eval "set $binList"; numBins=$#
for arch in $archList
do
    ${ECHO} -n "Thinning $arch/$driverName...    "
    eval "${MKDIRS} $arch"
    eval "${FASTCP} $driverName $arch"
    if [ $numBins -ne 0 ]
    then (
        cd $arch/$driverName
        for bin in $binList
	do
            subArch=`findArchForFamilyInFile $arch $bin`
	    [ -n "$subArch" ] || die 3 "Can't find target arch in $bin"
            eval "${LIPO} $bin -thin $subArch -o $bin.$arch"
            eval "${MV} -f $bin.$arch $bin"
        done
	doInstall .
    ) fi
done
set +e

${ECHO} Removing $driverPath
${CHMOD} +w `${FIND} $driverPath -type d -print`
${RM} -rf $driverPath || ${ECHO} "$cmdName Warning: Couldn't remove $driverPath"
