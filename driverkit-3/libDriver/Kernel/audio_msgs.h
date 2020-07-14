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
 * audio_msgs.h
 *
 * Copyright (c) 1992, NeXT Computer, Inc.  All rights reserved.
 *
 *      RPC interface to audio server.
 *
 * HISTORY
 *      10/29/92/mtm    Original coding.
 */

#import <mach/mach_types.h>
#import <mach/message.h>

#define AUDIO_SERVER_NAME "audio0"

#define AUDIO_MSG_GET_PORTS	0
#define	AUDIO_MSG_RET_PORTS	1

typedef struct {
    msg_header_t	header;
    msg_type_t		privType;
    port_t		priv;
    msg_type_t		resetType;
    boolean_t		reset;
} audio_get_ports_t;

typedef struct {
    msg_header_t	header;
    msg_type_t		portsType;
    port_t		inputPort;
    port_t		outputPort;
    port_t		sndPort;
} audio_ret_ports_t;
