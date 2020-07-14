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
 * Copyright 1993 NeXT, Inc.
 * All rights reserved.
 */
 
#import "libsaio.h"
#import "drivers.h"


/*
 * Multiple-choice question, CDIS-style.
 * Localizable.
 */

static char *prompt = "\n---> ";

int chooseSimple( char **strings, int nstrings, int min, int max )
{
    char buf[80];
    register int num;
    
    for (num = 0; num < nstrings; num++) {
	localPrintf(strings[num]);
    }
    printf(prompt);
    gets(buf,sizeof(buf));
    num = atoi(buf);
    if (num < min || num > max) {
	return -1;
    }
    return num;
}

/*
 * Choices are assumed to be already localized.
 */

#define SCREEN_SIZE 7

/*
 * New: choose only "interesting" drivers
 * (which have their "interesting" bit set)
 * Two quit messages; will return -1 for first quit,
 * -2 for second quit.
 */

int chooseDriverFromList(
    char *title,
    char *message,
    struct driver_info *drivers,
    int nstrings,
    char *footMessage,
    char *additionalMessage,
    char *quit1Message,
    char *quit2Message
)
{
    char buf[80];
    int i, tail, head, choice, more, quit1, quit2, num, nchoices;
    int screenSize = quit2Message ? SCREEN_SIZE - 1 : SCREEN_SIZE;

    head = tail = 0;
    for (i=nchoices=0; i<nstrings; i++) {
	if (drivers[i].flags & DRIVER_FLAG_INTERESTING)
	    nchoices++;
    }
    while (1) {
	clearScreen();
	if (message) {
	    localPrintf(message);
	    printf("\n");
	}
	more = quit1 = quit2 = 0;
	for(i = head, choice = 0; i < nstrings; i++) {
	    if (drivers[i].name == 0)
		break;
	    if (!(drivers[i].flags & DRIVER_FLAG_INTERESTING))
		continue;
	    printf("%d. %s",
		++choice, drivers[i].name);
	    if (drivers[i].locationTag)
		printf(" (%s)\n", drivers[i].locationTag);
	    else
		printf("\n");
	    if (choice == screenSize)
		break;
	}
	tail = ++i;
	printf("\n");
	localPrintf(footMessage);

	if (nchoices > screenSize) {
	    localPrintf(additionalMessage, more = ++choice);
	}
	if (quit1Message && *quit1Message)
	    localPrintf(quit1Message, quit1 = ++choice);
	if (quit2Message && *quit2Message)
	    localPrintf(quit2Message, quit2 = ++choice);
	printf(prompt);
	gets(buf,sizeof(buf));
	num = atoi(buf);
	if (num < 1 || num > choice) {
//	    localPrintf("Your reply must be between %d and %d.  "
//	    		"Press <Return> to continue.\n",1,choice);
//	    gets(buf,sizeof(buf));
	    continue;
	}
	if (num == more) {
	    head = tail;
	    if (head >= nstrings)
		head = 0;
	} else if (num == quit1) {
	    return -1;
	} else if (num == quit2) {
	    return -2;
	} else {
	    for (i=head; i < nstrings; i++)
		if (drivers[i].flags & DRIVER_FLAG_INTERESTING)
		    if (--num == 0)
			break;
	    return i + 1;
	}
    }
}


void clearScreen()
{
    register int i;
    for (i=0; i < 25; i++)
	putchar('\n');
}
