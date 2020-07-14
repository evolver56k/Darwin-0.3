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
/*	NXBundle.h
	Copyright 1990, 1991, NeXT, Inc.
	Utilities for dynamic loading and internationalization.
	IPC, November 1990
*/

#ifndef _OBJC_NXBUNDLE_H_
#define _OBJC_NXBUNDLE_H_

#import "Object.h"

@interface NXBundle: Object {
@private
    char	*_directory;
    Class	_principalClass;
    BOOL	_codeLoaded;
    BOOL	_reserved1, _reserved2, _reserved3;
    int		_bundleVersion;
}

- initForDirectory:(const char *)path;
+ mainBundle;
+ bundleForClass:class;
- (const char *)directory;
- classNamed:(const char *)className;
- principalClass;
- setVersion:(int)version;
- (int)version;
- free;
+ (BOOL)getPath:(char *)path forResource:(const char *)name
         ofType:(const char *)ext inDirectory: (const char *)bundlePath
    withVersion: (int)version;
- (BOOL)getPath:(char *)path forResource:(const char *)name ofType:(const char *)ext;
+ setSystemLanguages:(const char * const *)languages;
@end

#define NXLocalString(key, value, comment) \
    NXLoadLocalStringFromTableInBundle(NULL, nil, key, value)
#define NXLocalStringFromTable(table, key, value, comment) \
    NXLoadLocalStringFromTableInBundle(table, nil, key, value)
#define NXLocalStringFromTableInBundle(table, bundle, key, value, comment) \
    NXLoadLocalStringFromTableInBundle(table, bundle, key, value)

#define NXLocalizedString(key, value, comment) \
    NXLoadLocalizedStringFromTableInBundle(NULL, nil, key, value)
#define NXLocalizedStringFromTable(table, key, value, comment) \
    NXLoadLocalizedStringFromTableInBundle(table, nil, key, value)
#define NXLocalizedStringFromTableInBundle(table, bundle, key, value, comment) \
    NXLoadLocalizedStringFromTableInBundle(table, bundle, key, value)

extern const char *NXLoadLocalStringFromTableInBundle(const char *table, NXBundle *bundle, const char *key, const char *value);
extern const char *NXLoadLocalizedStringFromTableInBundle(const char *table, NXBundle *bundle, const char *key, const char *value);

#endif /* _OBJC_NXBUNDLE_H_ */
