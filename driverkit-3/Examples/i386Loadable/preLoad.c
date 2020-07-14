/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.1 (the "License").  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License.
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
#import <driverkit/generalFuncs.h>
#import <libc.h>

int main(int argc, char **argv)
{
	int instance;
	int len;
	int arg;
	
	if(argc < 2) {
		exit(1);
	}
	len = strlen(argv[argc-1]);
	instance = atoi(&argv[argc-1][len-1]);
	IOLog("NullDriver PreLoad instance %d\n", instance);
	for(arg=1; arg<argc; arg++) {
		IOLog("...argv[%d] = %s\n", arg, argv[arg]);
	}
	if((argc == 3) && (strcmp(argv[1], "abort") == 0)) {
		exit(1);
	}
	else {
		exit(0);
	}
}	