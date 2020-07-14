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

#import <mach/std_types.h>
#import <driverkit/IODirectDevice.h>

#ifndef __APPLE_TYPES_DEFINED__
#define __APPLE_TYPES_DEFINED__ 1

        typedef unsigned int    UInt32;			// A 32-bit unsigned integer
        typedef unsigned char   UInt8;			// A "byte-sized" integer
	typedef signed char	SInt8;
        typedef signed int      SInt32;			// A 32-bit signed integer
        typedef boolean_t       Boolean;		// TRUE/FALSE value (YES/NO in NeXT)
	typedef signed int	OSErr;
	typedef signed int	OSStatus;

#endif /* __APPLE_TYPES_DEFINED__ */


typedef void (*pmCallback_func)(id obj_id, UInt32 refNum, UInt32 length, UInt8 * buffer);
typedef void (*pmADBinput_func)(id obj_id, UInt32 refNum, UInt32 devNum, UInt32 length, UInt8 * buffer);

typedef OSStatus PMUStatus;

enum {
        kPMUNoError             = 0,
        kPMUInitError           = 1,    // PMU failed to initialize
        kPMUParameterError      = 2,    // Bad parameters
	kPMUNotSupported	= 3,	// PMU don't do that (Cuda does, though)
        kPMUIOError             = 4     // Nonspecific I/O failure
	};


// **********************************************************************************
//
// exported protocols
//
// **********************************************************************************

@protocol ADBservice

- (void)registerForADBAutopoll	:(pmADBinput_func)InputHandler
				:(id)caller;

- (PMUStatus)ADBWrite   :(UInt32)DevAddr
			:(UInt32)DevReg
			:(UInt32)ByteCount
			:(UInt8*)Buffer
			:(UInt32)RefNum
                        :(id)Id
			:(pmCallback_func)Callback;

- (PMUStatus)ADBRead    :(UInt32)DevAddr
			:(UInt32)DevReg
			:(UInt32)RefNum
                        :(id)Id
			:(pmCallback_func)Callback;

- (PMUStatus)ADBReset   :(UInt32)Refnum
                        :(id)Id
			:(pmCallback_func)Callback;

- (PMUStatus)ADBFlush   :(UInt32)DevAddr
			:(UInt32)RefNum
                        :(id)Id
			:(pmCallback_func)Callback;

- (PMUStatus)ADBSetPollList     :(UInt32)PollBitField
				:(UInt32)RefNum
                        	:(id)Id
				:(pmCallback_func)Callback;

- (PMUStatus)ADBPollDisable     :(UInt32)RefNum
                	        :(id)Id
				:(pmCallback_func)Callback;

- (PMUStatus)ADBSetFileServerMode:(UInt32)RefNum
				:(id)Id
 				:(pmCallback_func)Callback;



- (PMUStatus)ADBPollEnable      :(UInt32)RefNum
				:(id)Id
				:(pmCallback_func)Callback;

- (PMUStatus)ADBSetPollRate     :(UInt32)newRate
				:(UInt32)RefNum
				:(id)Id
				:(pmCallback_func)Callback;

- (PMUStatus)ADBGetPollRate     :(UInt32 *)currentRate
				:(UInt32)RefNum
				:(id)Id
				:(pmCallback_func)Callback;

- (PMUStatus)ADBSetAlternateKeyboard    :(UInt32)DevAddr
					:(UInt32)RefNum
					:(id)Id
					:(pmCallback_func)Callback;
- (void)poll_device;

@end



@protocol RTCservice

- (void) registerForClockTicks	:(pmCallback_func)tickHandler
				:(id)caller;

- (PMUStatus)setRealTimeClock   :(UInt8 *)newTime
				:(UInt32)RefNum
        	                :(id)Id
				:(pmCallback_func)Callback;

- (PMUStatus)getRealTimeClock   :(UInt8 *)currentTime
				:(UInt32)RefNum
	                        :(id)Id
				:(pmCallback_func)Callback;

@end



@protocol NVRAMservice

- (PMUStatus) readNVRAM	:(UInt32)Offset
			:(UInt32)Length
			:(UInt8 *)Buffer
			:(UInt32)RefNum
                        :(id)Id
			:(pmCallback_func)Callback;

- (PMUStatus) writeNVRAM:(UInt32)Offset
			:(UInt32)Length
			:(UInt8 *)Buffer
			:(UInt32)RefNum
                        :(id)Id
			:(pmCallback_func)Callback;

@end



@protocol PowerService

- (void) registerForPowerInterrupts	:(pmCallback_func)buttonHandler
					:(id)caller;

@end















