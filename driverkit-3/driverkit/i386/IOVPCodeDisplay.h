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
/* CONFIDENTIAL
 * Copyright (c) 1993 by NeXT Computer, Inc as an unpublished work.
 * All rights reserved.
 *
 * IOVPCodeDisplay.h -- interface for the vpcode video display driver.
 *
 * 22 July 1993		Derek B Clegg
 * 	Created.
 */

#ifndef IOVPCODEDISPLAY_H__
#define IOVPCODEDISPLAY_H__

#import <driverkit/IOFrameBufferDisplay.h>
#import <driverkit/i386/vpCode.h>

#define VP_VERIFY_MODE			0
#define VP_INITIALIZE_MODE		1
#define VP_ENABLE_LINEAR_FRAMEBUFFER	2
#define VP_RESET_VGA			3
#define VP_SET_DEFAULT_MODE		4
#define VP_GET_DISPLAY_INFO		5
#define VP_GET_PIXEL_ENCODING		6
#define VP_SET_TRANSFER_TABLE		7
#define VP_TRANSFER_TABLE		8

@interface IOVPCodeDisplay:IOFrameBufferDisplay
{
    /* The transfer tables for this mode. */
    unsigned char *redTransferTable;
    unsigned char *greenTransferTable;
    unsigned char *blueTransferTable;

    /* The number of entries in the transfer table. */
    int transferTableCount;

    /* The current screen brightness. */
    int brightnessLevel;
@private
    unsigned int *_vpCode;
    unsigned int _vpCodeCount;
    unsigned int _vpCodeReceived;
    unsigned long _videoRamAddress;
    unsigned long _videoRamSize;
    BOOL _debug;

    /* Reserved for future expansion. */
    int _IOVPCodeDisplay_reserved[8];
}
- initFromDeviceDescription: deviceDescription;
- (void)enterLinearMode;
- (void)revertToVGAMode;
- setBrightness:(int)level token:(int)t;
- setTransferTable:(const unsigned int *)table count:(int)count;
- getVPCodeFilename:(char *)parameterArray count:(unsigned int *)count;
- runVPCode:(unsigned int)entryPoint withRegs:(VPInstruction *)initialRegs;
@end

#endif	/* IOVPCODEDISPLAY_H__ */
