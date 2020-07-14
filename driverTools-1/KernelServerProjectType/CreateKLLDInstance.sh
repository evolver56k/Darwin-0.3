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

cmdName=`basename $0`

usage()
{
    >&2 cat << EOusage
Usage: $cmdName [-C] <SERVER_NAME>
   where '-C' indicates the generation of a C only server.
EOusage
    exit 1
}

# Routine that echos a comment and bombs
die()
{
    code=$1; shift
    >&2 echo "$cmdName: $*"
    exit $code;
}

useObjC=1

for arg
do
    case $arg in
    -C)
        useObjC=0
        ;;
    -M)
        useObjC=1
        ;;
    *)
        serverName=$arg
        ;;
    esac
done

[ -n "$serverName" ] || usage

if test $useObjC -eq 1
then
cat << EOI
#import <driverkit/IODevice.h>
#import <kernserv/kern_server_types.h>

kern_server_t ${serverName}_instance;

@interface ${serverName}KernelServerInstance : Object
{
}
+ (kern_server_t *) kernelServerInstance;
@end

@implementation ${serverName}KernelServerInstance

+ (kern_server_t *) kernelServerInstance
{
    return &${serverName}_instance;
}
@end

@interface ${serverName}Version : IODevice
{
}
+ (int) driverKitVersionFor${serverName};
@end

@implementation ${serverName}Version

+ (int) driverKitVersionFor${serverName}
{
    return IO_DRIVERKIT_VERSION;
}
@end

EOI

else

cat << EOI
#import <kernserv/kern_server_types.h>

kern_server_t ${serverName}_instance;

EOI

fi
