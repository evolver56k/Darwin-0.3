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
/*
    NXStringTable.h
    Copyright 1990 NeXT, Inc.
	
    DEFINED AS: A common class
    HEADER FILES: objc/NXStringTable.h

*/

#ifndef _OBJC_NXSTRINGTABLE_H_
#define _OBJC_NXSTRINGTABLE_H_

#import "HashTable.h"

#define MAX_NXSTRINGTABLE_LENGTH	1024

@interface NXStringTable: HashTable

- init;
- free;
    
- (const char *)valueForStringKey:(const char *)aString;
    
- readFromStream:(NXStream *)stream;
- readFromFile:(const char *)fileName;

- writeToStream:(NXStream *)stream;
- writeToFile:(const char *)fileName;

/*
 * The following new... methods are now obsolete.  They remain in this 
 * interface file for backward compatibility only.  Use Object's alloc method 
 * and the init method defined in this class to create an NXStringTable and
 * the readFrom... methods to fill it with data.
 */

+ new;
+ newFromStream:(NXStream *)stream;
+ newFromFile:(const char *)fileName;

@end

static inline const char *NXSTR(NXStringTable *table, const char *key) {
    return [table valueForStringKey:key];
}

#endif /* _OBJC_NXSTRINGTABLE_H_ */
