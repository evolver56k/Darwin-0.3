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
 This API returns the various standard system directories where apps, resources, etc get installed.
 Because queries can return multiple directories, the API is in the form of an enumeration.
 The directories are returned in search path order; that is, the first place to look is returned first.
 This API may return directories that do not exist yet.
 If NSUserDomain is included in a query, then the results will contain "~" to refer to the user's directory.
 NEXT_ROOT is prepended as necessary to the returned values.
 Some calls might return no directories!

 Typical usage:
 
    char path[MAXPATHLEN];
    NSSearchPathEnumerationState state = NSStartSearchPathEnumeration(dir, domainMask);
    while (state = NSGetNextSearchPathEnumeration(state, path)) {
        // Handle path
    }


 Remaining issues:
 On Windows, treatment of /Local is goofy. Ideally this should not exposed to the outside world at all.
 On Windows, should directories such as "Program Files" or "Windows" be included in these paths?
*/

// Directories

typedef enum {
    NSApplicationDirectory = 1,		// supported applications (Applications)
    NSDemoApplicationDirectory,		// unsupported applications, demonstration versions (Demos)
    NSDeveloperApplicationDirectory,	// developer applications (Developer/Applications)
    NSAdminApplicationDirectory,	// system and network administration applications (Administration)
    NSLibraryDirectory, 		// various user-visible documentation, support, and configuration files, resources (Library)
    NSDeveloperDirectory,		// developer resources (Developer)
    NSUserDirectory,			// user home directories (Users)
    NSDocumentationDirectory,		// documentation (Documentation)
    NSAllApplicationsDirectory = 100,	// all directories where applications can occur (ie Applications, Demos, Administration, Developer/Applications)
    NSAllLibrariesDirectory = 101	// all directories where resources can occur (Library, Developer)
} NSSearchPathDirectory;

// Domains

typedef enum {
   NSUserDomainMask = 1,	// user's home directory --- place to install user's personal items (~)
   NSLocalDomainMask = 2,	// local to the current machine --- place to install items available to everyone on this machine (/Local)
   NSNetworkDomainMask = 4, 	// publically available location in the local area network --- place to install items available on the network (/Network)
   NSSystemDomainMask = 8,	// provided by Apple, unmodifiable (/System)
   NSAllDomainsMask = 0x0ffff	// all domains: all of the above and more, future items
} NSSearchPathDomainMask;

typedef unsigned int NSSearchPathEnumerationState;

/* Enumeration
 Call NSStartSearchPathEnumeration() once, then call NSGetNextSearchPathEnumeration() one or more times with the returned state.
 The return value of NSGetNextSearchPathEnumeration() should be used as the state next time around.
 When NSGetNextSearchPathEnumeration() returns 0, you're done.
*/
extern NSSearchPathEnumerationState NSStartSearchPathEnumeration(NSSearchPathDirectory dir, NSSearchPathDomainMask domainMask);

extern NSSearchPathEnumerationState NSGetNextSearchPathEnumeration(NSSearchPathEnumerationState state, char *path);




