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
// localecho.m
// Takes a key and prints the value for that key using the ROOT and LANGUAGE
// environment variables.

#import <stdio.h>
#import <stdlib.h>
#import <sys/param.h>
#import <Foundation/Foundation.h>

char *progName;

void usage() {
    fprintf(stderr, "\tUsage: %s [-q] key\n", progName);
    exit(1);
}

int main(int argc, char **argv) {
    NSString *value;
    char *language = NULL, *root = NULL;
    NSString *langDir;
    NSString *key = nil;
    int question = 0;
    NSBundle *langBundle;
    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    if (!(language = getenv("LANGUAGE"))) {
	language = "English";
    }
    if (!(root = getenv("ROOT"))) {
	root = "";
    }

    progName = *argv++;

    if (*argv) {
	if (!strcmp(*argv, "-q") ) {
	    question = 1;
	    argv++;
	}
    } else {
	usage();
    }
    if (!(*argv)) {
    	usage();
    }	
    else key = [NSString stringWithCString: (*argv)]; 	
    
    
    //sprintf(langDir, "%s/NextCD/CDIS/%s.lproj", root, language);
    langDir = [NSString stringWithFormat: @"%s/System/Installation/CDIS/%s.lproj", root, language];
    if (!(langBundle = [[NSBundle alloc] initWithPath: langDir])) {
    	fprintf(stderr, "Could not load table for %s.\n", language);
	exit(2);
    }
    value = NSLocalizedStringFromTableInBundle(key, NULL, langBundle, NULL);
    [langBundle release];
    printf("%s", [value cString]);
    if (!question) {
	printf("\n");
    }

    [pool release];

    return(0);
}