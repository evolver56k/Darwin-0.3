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
 * Copyright (c) 1989 NeXT, Inc.
 *
 * HISTORY
 * 04-Apr-89  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */

#ifndef _KERN_LOADER_TYPES_
#define _KERN_LOADER_TYPES_

#import <mach/port.h>
#import <mach/boolean.h>

typedef char *printf_data_t;
typedef char server_name_t[256];
typedef server_name_t server_reloc_t;
typedef server_name_t *server_name_array_t;
typedef server_name_t *port_name_string_array_t;
typedef boolean_t *boolean_array_t;

enum server_state {
	Zombie = 0,
	Allocating = 1,
	Allocated = 2,
	Loading = 3,
	Loaded = 4,
	Unloading = 5,
	Deallocated = 6,
	Detached = 7,
};

typedef enum server_state server_state_t;

#define temp_server_state(s)	(   (s) == Allocating			\
				 || (s) == Loading			\
				 || (s) == Unloading)

static inline const char *server_state_string(server_state_t state)
{
	static const char * const strings[] = {
		"Zombie",
		"Allocating",
		"Allocated",
		"Loading",
		"Loaded",
		"Unloading",
		"Deallocated",
		"Detached",
	};
	return strings[state];
};

/*
 * Return error codes.
 */
#define KERN_LOADER_NO_PERMISSION	101
#define KERN_LOADER_UNKNOWN_SERVER	102
#define KERN_LOADER_SERVER_LOADED	103
#define KERN_LOADER_SERVER_UNLOADED	104
#define KERN_LOADER_NEED_SERVER_NAME	105
#define KERN_LOADER_SERVER_EXISTS	106
#define KERN_LOADER_PORT_EXISTS		107
#define KERN_LOADER_SERVER_WONT_LINK	108
#define KERN_LOADER_SERVER_WONT_LOAD	109
#define KERN_LOADER_CANT_OPEN_SERVER	110
#define KERN_LOADER_BAD_RELOCATABLE	111
#define KERN_LOADER_MEM_ALLOC_PROBLEM	112
#define KERN_LOADER_SERVER_DELETED	113
#define KERN_LOADER_WONT_UNLOAD		114
#define KERN_LOADER_DETACHED		115

/*
 * Our name (for netname lookup)
 */
#define KERN_LOADER_NAME "server_loader"

/*
 * Get port for communication with kern_loader.
 */
extern kern_return_t kern_loader_look_up(port_t *kern_loader_port);

#endif _KERN_LOADER_TYPES_
