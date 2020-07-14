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
/*
 * malloc() timing test using uxpr.
 */

#import <bsd/libc.h> 
#import <driverkit/debugging.h>

static void usage(char **argv);

int num_loops=10;
char *foobar;

int main(int argc, char **argv)
{
	int arg;
	int loop;
	int malloc_size;
	char s[100];
	
	if(argc < 1)
		usage(argv);
	for(arg=1; arg<argc; arg++) {
		switch(argv[arg][0]) {
		    case 'n':
		    	num_loops = atoi(&argv[arg][2]);
			break;
		    default:
		    	usage(argv);
		}
	}
	IOInitDDM(1000, "malloc");
	IOAddDDMEntry("%s: starting\n", (int)argv[0], 2,3,4,5);
	for(loop=1; loop<=num_loops; loop++) {
		malloc_size = 1 << loop;
		foobar = malloc(malloc_size);
		IOAddDDMEntry("malloc'd %d bytes\n", malloc_size, 2,3,4,5);		
	}
	printf("Done; enter CR to quit: ");
	gets(s);
	exit(0);
}

static void usage(char **argv)
{
	printf("usage: %s [n=num_loops]\n", argv[0]);
	exit(1);
}

