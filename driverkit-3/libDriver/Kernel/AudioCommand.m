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

#import "AudioCommand.h"
#import <driverkit/IOAudioPrivate.h>		// AD_CMD_MSG_SYNCHRONOUS

#import <mach/message.h>		// msg_header_t
#import <machkit/NXLock.h>
#import <driverkit/kernelDriver.h>
 
/*
 * FIXME: This is from <kern/ipc_basics.h>, which can't be
 * imported because of a conflict with <mach/mach_types.h>
 */
extern msg_return_t msg_send_from_kernel();

@implementation AudioCommand

- initPort:(port_t)port
{
    [super init];

    interLock = [[NXConditionLock alloc] initWith:AUDIO_COMMAND_IDLE];
    driverPort_kern = port;
    return self;
}

- free
{
    [interLock free];
    return [super free];
}

- (ADCommand) command
{
    return (command);
}

- (void)done:(int)_ret
{
    if ([interLock condition] == AUDIO_COMMAND_BUSY) {
	[interLock lock];
	ret = _ret;
	[interLock unlockWith:AUDIO_COMMAND_DONE];
    }

}

- (int)send:(ADCommand)_command
{
    msg_header_t	msg = { 0 };
    int			result = 0;
 
 
     [interLock lockWhen:AUDIO_COMMAND_IDLE];
        
    command = _command;
    
    msg.msg_size = sizeof (msg);
    msg.msg_remote_port = driverPort_kern;
    msg.msg_id =  AD_CMD_MSG_SYNCHRONOUS;
    
    [interLock unlockWith:AUDIO_COMMAND_BUSY];
    
    result = msg_send_from_kernel(&msg, SEND_TIMEOUT, 1000);
    if (result == SEND_SUCCESS) {
	[interLock lockWhen:AUDIO_COMMAND_DONE];
	result = ret;
    }
    else
    	[interLock lock];
    
    [interLock unlockWith:AUDIO_COMMAND_IDLE];
    
    return (result);

}

@end
