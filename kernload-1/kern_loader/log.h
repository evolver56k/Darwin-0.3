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
 * Copyright (c) 1990 NeXT, Inc.
 *
 * HISTORY
 * 13-Apr-90  Gregg Kellogg (gk) at NeXT
 *	Created.
 *
 */ 

#ifndef	_KERNLOAD_LOG_
#define	_KERNLOAD_LOG_
#import <streams/streams.h>

/*
 * Kern loader message logging interface.
 */
void klinit(const char *server_name);
void kllog(int priority, const char *message, ...);
void kllog_stream(int priority, NXStream *stream);
void kladdport(port_name_t port);
void klremoveport(port_name_t port);

#endif	_KERNLOAD_LOG_

