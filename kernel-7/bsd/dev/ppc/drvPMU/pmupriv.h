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
#import <kernserv/ns_timer.h>
#import <driverkit/IODirectDevice.h>


        typedef volatile unsigned char  *VIAAddress;	// This is an address on the bus

// **********************************************************************************
//
// VIA definitions
//
// **********************************************************************************

enum {				 	// port B
						// M2 uses VIA2
	M2Req		= 2,			      	// Power manager handshake request
	M2Ack		= 1,				// Power manager handshake acknowledge
	      					// Hooper uses VIA1
	HooperReq	= 4,			      	// request
	HooperAck	= 3				// acknowledge
};

enum {					// IFR/IER
	ifCA2 = 0,				// CA2 interrupt
	ifCA1 = 1,				// CA1 interrupt
	ifSR  = 2,				// SR shift register done
	ifCB2 = 3,				// CB2 interrupt
	ifCB1 = 4,				// CB1 interrupt
	ifT2  = 5,				// T2 timer2 interrupt
	ifT1  = 6,				// T1 timer1 interrupt
	ifIRQ = 7				// any interrupt
};

// **********************************************************************************
// bits in response to kPMUReadInt command
// **********************************************************************************

enum {
	kPMUMD0Int 		= 0x01,   // interrupt type 0 (machine-specific)
	kPMUMD1Int 		= 0x02,   // interrupt type 1 (machine-specific)
	kPMUMD2Int 		= 0x04,   // interrupt type 2 (machine-specific)
	kPMUbrightnessInt 	= 0x08,   // brightness button has been pressed, value changed
	kPMUADBint 		= 0x10,   // ADB
	kPMUbattInt 		= 0x20,   // battery
	kPMUenvironmentInt 	= 0x40,   // environment
	kPMUoneSecInt 		= 0x80    // one second interrupt
};

enum {					// when kPMUADBint is set
	kPMUautopoll		= 0x04		// input is autopoll data
};

// **********************************************************************************
// states of the ISR
// **********************************************************************************

enum {
	kPMUidle,
	kPMUxmtLen,
	kPMUxmtData,
	kPMUreadLen_cmd,
	kPMUrcvLen_cmd,
	kPMUreadData,
	kPMUrcvData_cmd,
	kPMUdone,
	kPMUreadLen_int,
	kPMUrcvLen_int,
	kPMUrcvData_int
};


enum {
	kPMUADBAddressField = 4
};

enum {
	kPMUResetADBBus	= 0x00,
	kPMUFlushADB	= 0x01,
	kPMUWriteADB	= 0x08,
	kPMUReadADB	= 0x0C,
	kPMURWMaskADB	= 0x0C
};

#define MISC_LENGTH 8

struct PMURequest {
	struct PMUmachMessage *	next;
	struct PMUmachMessage *	prev;
	UInt32			pmCommand;
	UInt32			pmSLength1;
	UInt8 * 		pmSBuffer2;
	UInt32			pmSLength2;
	UInt8 *			pmRBuffer;
	Boolean			pmFlag;
	pmCallback_func		pmCallback;
	id			pmId;
	UInt32			pmRefNum;
	UInt8			pmSBuffer1[MISC_LENGTH];
};

typedef struct PMURequest PMURequest;

struct PMUmachMessage {
	msg_header_t	msgHeader;
	msg_type_t	msgType;
	PMURequest	msgBody;
};

typedef struct PMUmachMessage PMUmachMessage;


@interface ApplePMU : IODirectDevice <ADBservice, RTCservice, NVRAMservice, PowerService>
{
VIAAddress		VIA1_shift;		// pointers to VIA registers
VIAAddress		VIA1_auxillaryControl;
VIAAddress		VIA1_interruptFlag;
VIAAddress		VIA1_interruptEnable;
VIAAddress		VIA2_dataB;
UInt8			PMreq;		      	// req bit
UInt8			PMack;			// ack bit
UInt8			savedPGEintEn;
UInt8			savedSRintEn;
pmADBinput_func		ADBclient;		// Input handler in ADB client
id			ADBid;			// id of ADB client
pmCallback_func		RTCclient;		// Tick handler in RTC client
id			RTCid;			// id of RTC client
pmCallback_func		PWRclient;		// Button handler in Power Management client
id			PWRid;			// id of Power Management client
Boolean			debugging;		// TRUE to avoid COP timeout during debugging
PMURequest *		clientRequest;
PMUmachMessage		localMachMessage;
UInt8			firstChar;
UInt32			charCountS1;
UInt32			charCountS2;
UInt8 *			dataPointer1;
UInt8 *			dataPointer2;
UInt8 *			dataPointer;
SInt32			charCountR;
SInt32			charCountR2;
UInt32			PGE_ISR_state;
UInt8			receivedByte;
UInt8			interruptState[12];
PMUmachMessage *       	queueHead;		// our command queue
PMUmachMessage *	queueTail;
port_t			port;			// our interrupt port
UInt32			pollList;		// ADB autopoll device bitmap
Boolean			autopollOn;		// TRUE: PMU is autopolling
Boolean			adb_reading;		// TRUE: we have a register read outstanding
pmCallback_func		who_to_call;		// ADB client's callback for solicited input
id			theirId;		// ADB client's id for input callback
UInt32			theirRefNum;		// ADB client's refnum for input callback
ns_time_t		adb_read_timeout;	// timeout on read to absent adb device
Boolean			PMU_int_pending;	// TRUE: PMU has requested service
}


+ (Boolean)probe : (IODeviceDescription *)deviceDescription;	// initialize the PMU driver

- initFromDeviceDescription : (IODeviceDescription *)deviceDescription;

- free;                                         		// shut the driver down

- (void)interruptOccurred;

- (void)interruptOccurredAt:(int)localInterrupt;

- (void)timeoutOccurred;

- (void)receiveMsg;



								// ADB protocol
- (void)registerForADBAutopoll	:(pmADBinput_func)inputHandler
				:(id)caller;

- (PMUStatus)ADBWrite	:(UInt32)DevAddr
			:(UInt32)DevReg
			:(UInt32)ByteCount
			:(UInt8*)Buffer
			:(UInt32)RefNum
                        :(id)Id
			:(pmCallback_func)Callback;

- (PMUStatus)ADBRead	:(UInt32)DevAddr
			:(UInt32)DevReg
			:(UInt32)RefNum
                        :(id)Id
			:(pmCallback_func)Callback;

- (PMUStatus)ADBReset	:(UInt32)Refnum
                        :(id)Id
			:(pmCallback_func)Callback;

- (PMUStatus)ADBFlush	:(UInt32)DevAddr
			:(UInt32)RefNum
                        :(id)Id
			:(pmCallback_func)Callback;

- (PMUStatus)ADBSetPollList	:(UInt32)PollBitField
				:(UInt32)RefNum
                        	:(id)Id
				:(pmCallback_func)Callback;

- (PMUStatus)ADBPollDisable	:(UInt32)RefNum
                	        :(id)Id
				:(pmCallback_func)Callback;

- (PMUStatus)ADBPollEnable      :(UInt32)RefNum
				:(id)Id
 				:(pmCallback_func)Callback;

- (PMUStatus)ADBSetPollRate	:(UInt32)newRate
			        :(UInt32)RefNum
				:(id)Id
				:(pmCallback_func)Callback;

- (PMUStatus)ADBGetPollRate	:(UInt32 *)currentRate
				:(UInt32)RefNum
				:(id)Id
				:(pmCallback_func)Callback;

- (PMUStatus)ADBSetAlternateKeyboard	:(UInt32)DevAddr
					:(UInt32)RefNum
					:(id)Id
					:(pmCallback_func)Callback;

- (void)poll_device;


								// RTC protocol
- (void)registerForClockTicks	:(pmCallback_func)tickHandler
				:(id)caller;

- (PMUStatus)setRealTimeClock	:(UInt8 *)newTime
			 	:(UInt32)RefNum
        	                :(id)Id
                                :(pmCallback_func)Callback;

- (PMUStatus)getRealTimeClock	:(UInt8 *)currentTime
				:(UInt32)RefNum
	                        :(id)Id
				:(pmCallback_func)Callback;


								// NVRAM protocol
- (PMUStatus) readNVRAM :(UInt32)Offset
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


								// Power Management protocol
- (void)registerForPowerInterrupts	:(pmCallback_func)buttonHandler
					:(id)caller;


								// Misc protocol
- (PMUStatus)sendMiscCommand    :(UInt32)Command
                                :(UInt32)SLength
                                :(UInt8 *)SBuffer
                                :(UInt8 *)RBuffer
                                :(UInt32)RefNum
	                        :(id)Id
                                :(pmCallback_func)Callback;




								// private methods

- (void)ADBinput:(UInt32)theLength:(UInt8 *)theInput;

- (void)StartPMUTransmission:(PMURequest *)plugInMessage;

- (void)SendPMUByte:(UInt8)theByte;

- (void)ReadPMUByte:(UInt8*)theByte;

- (Boolean)WaitForAckLo;

- (Boolean)WaitForAckHi;

- (UInt8)GetPMUInterruptState;

- (void)RestorePMUInterrupt:(UInt8)savedPGEintEn;

- (void)DisablePMUInterrupt;

- (void)EnablePMUInterrupt;

- (void)AcknowledgePMUInterrupt;

- (UInt8)GetSRInterruptState;

- (void)RestoreSRInterrupt:(UInt8)savedSRintEn;

- (void)DisableSRInterrupt;

- (void)EnableSRInterrupt;

- (void)CheckRequestQueue;

@end










