/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*	NXPropertyList.h
	Basic protocol for property lists
  	Copyright 1991, NeXT, Inc.
	Bertrand, August 1991
*/

#import "NXString.h"
#import "hashtable.h"
#import "maptable.h"
#import "List.h"
 
/********	The basic property list protocol		********/

@protocol NXPropertyList
- (unsigned)count;
- (BOOL)member:(NXString *)key;
- get:(NXString *)key;
    /* returns nil or value */
- insert:(NXString *)key value:value;
    /* returns nil or previous value; 
    caller is responsible for freing previous value */
- remove:(NXString *)key;
    /* returns nil or previous value; 
    caller is responsible for freing previous value */
- empty;
- (NXMapState)initEnumeration;
    /* To enumerate use something like:
	NXMapState	state = [self initEnumeration];
        NXString	*key;
	id		value;
	while ([self enumerate:&state key:&key value:&value]) {
		...
	}
    */
- (BOOL)enumerate:(NXMapState *)state key:(NXString **)refKey value:(id *)refValue;
    
@end

/********	A class implementing it		********/

@interface NXPropertyList:Object <NXPropertyList> {
    NXMapTable	*table;
}
@end

/********	Basic ASCII read/write of property lists	********/

@interface NXPropertyList (Basic_IO)
- initFromStream:(NXStream *)stream;
- initFromPath:(NXString *)path;
    /* all init methods must be called just after alloc; 
	they may return nil in case of inexistant file or syntax error;  
	Only syntax errors are logged on console */

- (void)writeToStream:(NXStream *)stream;
- (BOOL)writeToPath:(NXString *)path safely:(BOOL)safe;
    /* uses a temporary ~ file, and then atomically moves the file */
- (BOOL)writeToPath:(NXString *)path;
    /* All basic write function do indenting;
	BOOL returned indicates success */
    
@end

/********	A list that really frees its elements	********/

@interface NXCleanList:List
- free;
    /* sends free to each of its objects */
@end

/********	Fancy ASCII read/write of property lists	********/

typedef struct _NXPropertyListReadContext {
    unsigned	line; 		/* for error messages */
    NXMutableString *buffer;	/* for efficiency */
    id	keyFactory;		/* e.g. [NXCollectedString class] */
    id	stringValueFactory;	/* e.g. [NXReadOnlyString class] */
    id	listValueFactory;	/* e.g. [List class]; nil OK */
    id	propertyListValueFactory; /* e.g. [NXPropertyList class]; nil OK */
    BOOL	noValueIsSame;	/* if = missing, either key or nil */
    NXZone	*zone;		/* zone used for reading; may be NULL */
    NXHashTable	*uniquingTable;	/* used to unique strings */
} NXPropertyListReadContext;

typedef struct _NXPropertyListWriteContext {
    unsigned	indentDelta;		/* 0 => dont add white spaces */
    unsigned	indent;			/* current number of spaces */
    BOOL	topLevelBrackets;	/* the outer {} */
    const char	*pairSeparator;		/* after each pair (after the ';') */
    // not an NXString because bug cc!
} NXPropertyListWriteContext;

@protocol NXPropertyListFancyIO
- initFromStream:(NXStream *)stream context:(NXPropertyListReadContext *)context;
- (void)writeToStream:(NXStream *)stream context:(NXPropertyListWriteContext *)context;
@end

@interface NXPropertyList (Fancy_IO)
- initFromStream:(NXStream *)stream context:(NXPropertyListReadContext *)context; //?? use protocol when possible in ObjC
- (void)writeToStream:(NXStream *)stream context:(NXPropertyListWriteContext *)context; //?? use protocol when possible in ObjC
- initFromPath:(NXString *)path context:(NXPropertyListReadContext *)context;
@end

@interface NXCleanList (Fancy_IO)
- initFromStream:(NXStream *)stream context:(NXPropertyListReadContext *)context; //?? use protocol when possible in ObjC
- (void)writeToStream:(NXStream *)stream context:(NXPropertyListWriteContext *)context; //?? use protocol when possible in ObjC
@end

@interface NXString (Fancy_IO)
- initFromStream:(NXStream *)stream context:(NXPropertyListReadContext *)context; //?? use protocol when possible in ObjC
- (void)writeToStream:(NXStream *)stream context:(NXPropertyListWriteContext *)context; //?? use protocol when possible in ObjC
@end

