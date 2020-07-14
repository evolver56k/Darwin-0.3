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
#import <stdio.h>
#import <sys/errno.h>
#import <Foundation/Foundation.h>
#import "ProgressWriter.h"
#undef TRUE
#undef FALSE
#import <curses.h>

#define LocalString(key) \
	NSLocalizedStringFromTableInBundle(key, NULL, langBundle, NULL)

#define ELLIPSIS "..."

@interface UnixBTree (DirectoryCount)
- (unsigned long)realObjectCount;
@end

@implementation UnixBTree (DirectoryCount)
/* Expensive, but worth it */
- (unsigned long)realObjectCount {
    UnixObject *obj = [self rootObject];
    UnixTreeCursor curs = [self enumerateFromRoot:obj];
    unsigned long objCount = 0;
    [obj free];
    while ((obj = [self next:curs])) {
	objCount++;
	[obj free];
    }
    [self endEnumeration:curs];
    return objCount;
}
@end

@implementation ProgressWriter:NSObject

NSBundle *langBundle = nil;

+ (void) initialize {
    char *language = NULL, *root = NULL; //langDir[MAXPATHLEN];
    NSString *langDir;    

    language = getenv("LANGUAGE");
    if (language == NULL) {
	language = "English";
    }
    root = getenv("ROOT");
    if (root == NULL) {
	root = "";
    }

//    sprintf(langDir, "%s/NextCD/CDIS/%s.lproj", root, language);
    langDir = [NSString stringWithFormat: @"%s/System/Installation/CDIS/%s.lproj", root, language];

    if (!(langBundle = [[NSBundle alloc] initWithPath: langDir])) {
    	fprintf(stderr, "Could not load table for %s.\n", language);
    }
}

- init {
    [super init];
    return [self initWithBom:nil useThermometer:NO usefbshow:NO];
}

- initWithBom:(UnixBTree *)bom useThermometer:(BOOL)useThermo usefbshow:(BOOL)usefbshow {

    [super init];
    thermo = useThermo;
    fbshow = usefbshow;
    
    if ((bom != nil) && (thermo || fbshow)) {
        numObjects = [bom realObjectCount];
        objectsCopied = 0;
    }

    if ((bom != nil) && thermo) {
	installString = LocalString(@"Installing ");
        installStrLen = [installString length];  //strlen(installString);
	initscr();
	clear();
	realCols = COLS - 1;
	maxLen = realCols - installStrLen;
	incr = ratio = numObjects / realCols;
	row = 1;
	move(row, 0);
	addstr([installString cString]);
	move(row+1, 0);
	for (tickCol = 0; tickCol < realCols; tickCol++) {
	    addch('.');
	}
	tickCol = 0;
	refresh();
    }
    
    if ((bom != nil) && fbshow) {
        system("fbshow -B -I 'Copying base system.' -z 0");

    }
    return self;
}

- copyingStartedFor:(const char *)fsnode mode:(u_short)mode
{
    if (thermo) {
	const char *path = fsnode+1;
	int len = strlen(path);
	move(row, installStrLen);
	clrtoeol();
	if (len >= maxLen) {
	    path += (len - maxLen + strlen(ELLIPSIS) );
	    addstr(ELLIPSIS);
	}
	addstr(path);
	refresh();
    } else {
	switch (mode & S_IFMT) {
	    case S_IFREG:
		printf("copying file %s ... ", fsnode);
		fflush(stdout);
		break;
	    case S_IFLNK:
		printf("copying symlink %s ... ", fsnode);
		fflush(stdout);
		break;
	    case S_IFBLK:
	    case S_IFCHR:
		printf("copying device %s ... ", fsnode);
		fflush(stdout);
		break;
	    /*
	    case S_IFDIR:
		printf("copying directory %s ... ", fsnode);
 		fflush(stdout);
		break;
	    */
	    default:;
	}
    }
    return self;
}

- copyingFinishedFor:(const char*)fsnode fileDesc:(int)fd mode:(u_short)mode size:(size_t)size
{
    char	command[200];
    static long	pct = 0;
    long	oldpct = 0;

    objectsCopied++;
    if (thermo) {
	if (tickCol < realCols) {
            //objectsCopied++;
	    if (objectsCopied >= incr) {
		incr += ratio;
		move(row+1, tickCol++);
		addch('=');
		move(row+1, tickCol);
		addch('>');
		refresh();
	    }
	}
    } else if (fbshow) {
       oldpct = pct;
        pct = 100 * objectsCopied / numObjects;
        if (pct != oldpct)
        {
             sprintf(command,
                    "fbshow -B -I '%d files copied. %ld%% done.' -z %ld",
                    objectsCopied,
                    pct,
                    pct);
            system(command);
 	    printf("%d files copied. %ld%% done.\n", objectsCopied, pct);
	    fflush(stdout);
         }
    } else {
	if (((mode & S_IFMT) == S_IFREG) && (size == -1)) {
	    printf("hard-linked\n");
	    fflush(stdout);
	    return self;
	}
	switch (mode & S_IFMT) {
	    case S_IFREG:
	    case S_IFLNK:
	    case S_IFBLK:
	    case S_IFCHR:
	    //case S_IFDIR:
		printf("%ld bytes\n", size);
		fflush(stdout);
		break;
	    default:;
	}
    }


    return self;
}

- copyingSkippedFor:(const char*)fsnode
{
    if (thermo) {
	objectsSkipped++;
    } else {
	printf("SKIPPED");
	fflush(stdout);
    }
    return self;
}

- (BOOL)quitBecauseErrorCopying:(const char*)fsnode errno:(int)errno
{
    printf("%s: %s\n", fsnode, sys_errlist[errno]);
    fflush(stdout);
    return YES;
}

- fatalErrorCopying:(const char*)fsnode errno:(int)errno
{
    printf("%s: %s\n", fsnode, sys_errlist[errno]);
    fflush(stdout);
    return self;
}

- fatalError:(const char*)errMsg
{
    fprintf(stderr, "%s\n", errMsg);
    fflush(stderr);
    return self;
}

- (void) dealloc {
    if (thermo) {
	move(row, 0);
	clrtoeol();
	addstr([LocalString(@"Completed.") cString]);
	move(row+2, 0);
	refresh();
	endwin();
    }
    [super dealloc];
}
@end


