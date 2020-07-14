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

#import "pmu.h"
#import "pmupriv.h"
#import "pmumisc.h"
#import "pmutables.h"
#import <kern/clock.h>
#import <kernserv/prototypes.h>
#import <kernserv/clock_timer.h>
#import <kernserv/ns_timer.h>
#import <bsd/sys/time.h>
#import <sys/callout.h>
#import <machdep/ppc/proc_reg.h>
#import <machdep/ppc/powermac.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>

// extern to let us fix up the boot time.
extern void set_boot_time(void);

void gotInterruptCause(id, UInt32, UInt32, UInt8 *);
void timer_expired(port_t mach_port);

extern void kprintf(const char *, ...);
extern void bcopy(void *, void *, int);
extern msg_send_from_kernel(msg_header_t *, int, int);

extern id ApplePMUId;

@implementation ApplePMU

// **********************************************************************************
// probe
//
// 
//
// **********************************************************************************
+ (Boolean) probe : deviceDescription
{
  id dev;
kprintf("PMU probe\n");
  if ( (dev = [ self alloc ]) == nil ) {
    return NO;
  }
  
  if ([dev initFromDeviceDescription:deviceDescription] == nil) {
    return NO;
  }
  
  ApplePMUId = dev;
  
  set_boot_time();

  return YES;
}




// **********************************************************************************
// initFromDeviceDescription
//
// 
//
// **********************************************************************************
- initFromDeviceDescription:(IODeviceDescription *)deviceDescription
{
  VIAAddress	  physicalAddress;
  IORange         *ioRange;
  PMUmachMessage  theRequest;
  unsigned int    *oldIRQs, newIRQs[2], tmpIRQ;
  
  
  if ( [super initFromDeviceDescription:deviceDescription] == nil ) {
    [self free];
    return nil;
  }
  
  [self setDeviceKind:"PM Subsystem"];
  [self setLocation:NULL];
  [self setName:"PMU"];
  
  ioRange = [deviceDescription memoryRangeList];
  physicalAddress = (VIAAddress)ioRange->start;
  
  VIA1_shift		= physicalAddress + 0x1400; // initialize VIA addresses
  VIA1_auxillaryControl = physicalAddress + 0x1600;
  VIA1_interruptFlag	= physicalAddress + 0x1A00;
  VIA1_interruptEnable	= physicalAddress + 0x1C00;
// VIA2_dataB		= physicalAddress + 0x2000;		// 5300
// PMreq		= 1 << M2Req;
// PMack		= 1 << M2Ack;
  VIA2_dataB		= physicalAddress + 0x0000; // Hooper uses VIA 1 instead
  PMreq			= 1 << HooperReq;           // and different bits in it as well
  PMack			= 1 << HooperAck;
  // initialize other variables
  ADBclient = NULL;
  RTCclient = NULL;
  debugging = FALSE;
  queueHead = NULL;
  queueTail = NULL;
  PGE_ISR_state = kPMUidle;
  pollList = 0;
  autopollOn = FALSE;
  adb_reading = FALSE;
  PMU_int_pending = FALSE;

  adb_read_timeout = 100000000;

  [self AcknowledgePMUInterrupt];                 		// turn off any pending PGE interrupt
  [self EnablePMUInterrupt];              			// enable PGE interrupts

  // This is a still sleazy hack...
  oldIRQs = [deviceDescription interruptList];

  if (IsSawtooth()) {
    // On Sawtooth it is ExtInt1...
    tmpIRQ = 47 ^ 0x18;
  } else {
    // This is another sleazy hack.
    //The second irq is two lower in the via table.
    tmpIRQ = ((*oldIRQs ^ 0x18) + 2) ^ 0x18;
  }

  newIRQs[0] = *oldIRQs;
  newIRQs[1] = tmpIRQ;

  [deviceDescription setInterruptList:newIRQs num:2];

  [self enableAllInterrupts];

  if ([self startIOThread] != IO_R_SUCCESS) {
    [self free];
    return nil;
  }

  port = IOConvertPort([self interruptPort],IO_KernelIOTask,IO_Kernel);
  
  theRequest.msgBody.pmCommand = kPMUSetModem1SecInt;             // tell PGE why it may interrupt
  theRequest.msgBody.pmFlag = FALSE;
  theRequest.msgBody.pmSLength1 = 1;
  theRequest.msgBody.pmSBuffer1[0] = kPMUMD2Int | kPMUbrightnessInt | kPMUADBint;
  theRequest.msgBody.pmSLength2 = 0;
  theRequest.msgBody.pmCallback = NULL;
  
  theRequest.msgHeader.msg_simple = TRUE;
  theRequest.msgHeader.msg_type = MSG_TYPE_NORMAL;
  theRequest.msgHeader.msg_remote_port = port;
  theRequest.msgHeader.msg_local_port = PORT_NULL;
  theRequest.msgHeader.msg_size = sizeof(PMUmachMessage);
  msg_send_from_kernel(&theRequest.msgHeader, MSG_OPTION_NONE, 0);
  
  theRequest.msgBody.pmCommand = kPMUreadINT;                     // read any pending interrupt from PGE
  theRequest.msgBody.pmFlag = FALSE;
  theRequest.msgBody.pmSLength1 = 0;                              // just to clear it
  theRequest.msgBody.pmSLength2 = 0;
  theRequest.msgBody.pmRBuffer = &interruptState[0];
  theRequest.msgBody.pmCallback = NULL;
  
  theRequest.msgHeader.msg_simple = TRUE;
  theRequest.msgHeader.msg_type = MSG_TYPE_NORMAL;
  theRequest.msgHeader.msg_remote_port = port;
  theRequest.msgHeader.msg_local_port = PORT_NULL;
  theRequest.msgHeader.msg_size = sizeof(PMUmachMessage);
  msg_send_from_kernel(&theRequest.msgHeader, MSG_OPTION_NONE, 0);

  [self registerDevice];

  return self;
}


// **********************************************************************************
// free
//
// 
//
// **********************************************************************************
- free
{
return [ super free ];
}





// **********************************************************************************
// poll_device
//
// System interrupts are disabled, but we are still operating the PMU for mini-
// monitor keyboard input.  We are called here in a loop to service the PMU.
//
// **********************************************************************************
- (void)poll_device
{
  if ( *VIA1_interruptFlag & 0x04 ) {   // is shift register done? ( ifSR )
    [self interruptOccurred];         	// yes, handle it
    return;
  }
  if ( *VIA1_interruptFlag & 0x10 ) { 	// is PMU requesting service? ( ifCB1 )
    *VIA1_interruptFlag = 0x10;       	// yes, clear interrupt ( ifCB1 )
    PGE_ISR_state = kPMUidle;     	// and handle it
    [self interruptOccurredAt:1];
  }
}


// **********************************************************************************
// receiveMsg
//
// 
//
// **********************************************************************************
- (void)receiveMsg
{
PMUmachMessage * toQueue;
IOReturn result;
if ( (PGE_ISR_state == kPMUidle) && !adb_reading ) {
	localMachMessage.msgHeader.msg_size = sizeof(PMUmachMessage);
        localMachMessage.msgHeader.msg_local_port = [self interruptPort];
	result = msg_receive(&localMachMessage.msgHeader, (msg_option_t)RCV_TIMEOUT, 0);
	if ( result == RCV_SUCCESS ) {
		[self StartPMUTransmission:&localMachMessage.msgBody];
		}
	}
else {
	toQueue = (PMUmachMessage*)kalloc(sizeof(PMUmachMessage));
        toQueue->msgHeader.msg_size = sizeof(PMUmachMessage);
        toQueue->msgHeader.msg_local_port = [self interruptPort];
        result = msg_receive(&toQueue->msgHeader, (msg_option_t)RCV_TIMEOUT, 0);
        if ( result == RCV_SUCCESS ) {
		toQueue->msgBody.prev = queueTail;
		toQueue->msgBody.next = NULL;
		if ( queueTail != NULL ) {
			queueTail->msgBody.next = toQueue;
			}
		else {
			queueHead = toQueue;
			}
		queueTail = toQueue;
		}
	}
}


// **********************************************************************************
// timeoutOccurred
//
// Our adb-read timer has expired after sending an adb-read command to the PMU.
// This means there is no such addressed device on the ADB bus.
// We call back to the ADB driver with a zero-characters-received response and
// dequeue our command queue and carry on.
// **********************************************************************************
- (void)timeoutOccurred
{
adb_reading = FALSE;
if ( clientRequest->pmCallback != NULL ) {		// Make the client callback
	clientRequest->pmCallback(clientRequest->pmId, clientRequest->pmRefNum, 0, NULL);
	}						// with zero received-length
[self CheckRequestQueue];
}


// ****************************************************************************
//      CheckRequestQueue
//      Called at interrupt time when current request is complete.  We may start
//      another request here if one is in queue, or we may re-enable PMU interrupts
//      (they were turned off in PMUStartIO) and return.
// ****************************************************************************
- (void)CheckRequestQueue
{
PMUmachMessage * nextRequest;

if ( queueHead == NULL ) {		                // is queue empty?
        [self EnablePMUInterrupt];                      // yes, enable interrupt and return
        }
else {
        nextRequest = queueHead;                        // no, dequeue first command
        queueHead = nextRequest->msgBody.next;
	if ( queueHead == NULL ) {
		queueTail = NULL;
		}
	bcopy (&nextRequest->msgBody, &localMachMessage.msgBody, sizeof(PMURequest));	// copy it
        kfree(nextRequest, sizeof(PMUmachMessage));				     	// free its memory
        [self StartPMUTransmission:&localMachMessage.msgBody];				// and send it to the PMU
        }
}


// **********************************************************************************
// registerForADBAutopoll
//
// The ADB driver is calling to tell us that it is prepared to receive
// "unsolicited" ADB autopoll data.  The parameter tells who to call
// when we get some.
//
// **********************************************************************************
- (void)registerForADBAutopoll	:(pmADBinput_func)InputHandler
				:(id)caller
{
ADBclient = InputHandler;
ADBid = caller;
}


// **********************************************************************************
// ADBWrite
//
// **********************************************************************************
- (PMUStatus)ADBWrite	:(UInt32)DevAddr
			:(UInt32)DevReg
			:(UInt32)ByteCount
			:(UInt8*)Buffer
			:(UInt32)RefNum
			:(id)Id
			:(pmCallback_func)Callback
{
PMUmachMessage	request;
msg_return_t	return_code;

if ( 	(ByteCount == 0) ||
	(Buffer == NULL) ||
	(ByteCount > (MISC_LENGTH-3) ) ) {
		return kPMUParameterError;
		}

request.msgBody.pmCommand = kPMUpMgrADB;
request.msgBody.pmFlag = TRUE;			// this op solicits input from PGE
request.msgBody.pmSLength1 = 3;
request.msgBody.pmSBuffer2 = Buffer;
request.msgBody.pmSLength2 = ByteCount;
request.msgBody.pmRBuffer = NULL;
request.msgBody.pmCallback = Callback;
request.msgBody.pmId = Id;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmSBuffer1[0] = kPMUWriteADB | (DevAddr << kPMUADBAddressField) | (DevReg);
if ( autopollOn ) {
	request.msgBody.pmSBuffer1[1] = 2;
	}
else {
	request.msgBody.pmSBuffer1[1] = 0;
	}
request.msgBody.pmSBuffer1[2] = ByteCount;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(PMUmachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
	return kPMUNoError;
	}
else {
	return kPMUIOError;
	}
}


// **********************************************************************************
// ADBRead
//
// **********************************************************************************
- (PMUStatus)ADBRead	:(UInt32)DevAddr
			:(UInt32)DevReg
			:(UInt32)RefNum
                        :(id)Id
			:(pmCallback_func)Callback
{
PMUmachMessage	request;
msg_return_t	return_code;

request.msgBody.pmCommand = kPMUpMgrADB;
request.msgBody.pmFlag = TRUE;			// this op solicits input from PGE
request.msgBody.pmSLength1 = 3;
request.msgBody.pmSBuffer2 = NULL;
request.msgBody.pmSLength2 = 0;
request.msgBody.pmRBuffer = NULL;
request.msgBody.pmCallback = Callback;
request.msgBody.pmId = Id;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmSBuffer1[0] = kPMUReadADB | (DevAddr << kPMUADBAddressField) | (DevReg);
if ( autopollOn ) {
	request.msgBody.pmSBuffer1[1] = 2;
	}
else {
	request.msgBody.pmSBuffer1[1] = 0;
	}
request.msgBody.pmSBuffer1[2] = 0;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(PMUmachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
	return kPMUNoError;
	}
else {
	return kPMUIOError;
	}
}


// **********************************************************************************
// ADBReset
//
// **********************************************************************************
- (PMUStatus)ADBReset	:(UInt32)RefNum
                        :(id)Id
			:(pmCallback_func)Callback
{
PMUmachMessage	request;
msg_return_t	return_code;

request.msgBody.pmCommand = kPMUpMgrADB;
request.msgBody.pmFlag = TRUE;		// this op solicits input from PGE
request.msgBody.pmSLength1 = 3;
request.msgBody.pmSBuffer2 = NULL;
request.msgBody.pmSLength2 = 0;
request.msgBody.pmRBuffer = NULL;
request.msgBody.pmCallback = Callback;
request.msgBody.pmId = Id;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmSBuffer1[0] = kPMUResetADBBus;
request.msgBody.pmSBuffer1[1] = 0;
request.msgBody.pmSBuffer1[2] = 0;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(PMUmachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
	return kPMUNoError;
	}
else {
	return kPMUIOError;
	}
}


// **********************************************************************************
// ADBFlush
//
// **********************************************************************************
- (PMUStatus)ADBFlush	:(UInt32)DevAddr
			:(UInt32)RefNum
                        :(id)Id
			:(pmCallback_func)Callback
{
PMUmachMessage	request;
msg_return_t	return_code;

request.msgBody.pmCommand = kPMUpMgrADB;
request.msgBody.pmFlag = TRUE;
request.msgBody.pmSLength1 = 3;
request.msgBody.pmSBuffer2 = NULL;
request.msgBody.pmSLength2 = 0;
request.msgBody.pmRBuffer = NULL;
request.msgBody.pmId = Id;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmCallback = Callback;
request.msgBody.pmSBuffer1[0] = kPMUFlushADB | (DevAddr << kPMUADBAddressField);
if ( autopollOn ) {
	request.msgBody.pmSBuffer1[1] = 2;
	}
else {
	request.msgBody.pmSBuffer1[1] = 0;
	}
request.msgBody.pmSBuffer1[2] = 0;
		
request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(PMUmachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
	return kPMUNoError;
	}
else {
	return kPMUIOError;
	}
}


// **********************************************************************************
// ADBSetPollList
//
// **********************************************************************************
- (PMUStatus)ADBSetPollList	:(UInt32)PollBitField
				:(UInt32)RefNum
	                        :(id)Id
				:(pmCallback_func)Callback
{
PMUmachMessage	request;
msg_return_t	return_code;

pollList = PollBitField;				// remember the new poll list

if ( autopollOn ) {					// if PMU is currently autopolling,
	request.msgBody.pmCommand = kPMUpMgrADB;	// give it the new list
	request.msgBody.pmFlag = FALSE;
	request.msgBody.pmSLength1 = 4;
	request.msgBody.pmSBuffer2 = NULL;
	request.msgBody.pmSLength2 = 0;
	request.msgBody.pmRBuffer = NULL;
	request.msgBody.pmRefNum = RefNum;
	request.msgBody.pmId = Id;
	request.msgBody.pmCallback = Callback;
	request.msgBody.pmSBuffer1[0] = 0;
	request.msgBody.pmSBuffer1[1] = 0x86;
	request.msgBody.pmSBuffer1[2] = (UInt8)(PollBitField >> 8);
	request.msgBody.pmSBuffer1[3] = (UInt8)(PollBitField & 0xff);

	request.msgHeader.msg_simple = TRUE;
	request.msgHeader.msg_type = MSG_TYPE_NORMAL;
	request.msgHeader.msg_remote_port = port;
	request.msgHeader.msg_local_port = PORT_NULL;
	request.msgHeader.msg_size = sizeof(PMUmachMessage);
	return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

	if ( return_code == SEND_SUCCESS ) {
		return kPMUNoError;
		}
	else {
		return kPMUIOError;
		}
	}
else {							// we'll do it later
	if ( Callback != NULL ) {			// but make the client callback now
		Callback(Id, RefNum, 0, NULL);
		}
	}
return kPMUNoError;
}


// **********************************************************************************
// ADBSetFileServerMode()
//
// **********************************************************************************
- (PMUStatus)ADBSetFileServerMode       :(UInt32)RefNum
                                		:(id)Id
                                		:(pmCallback_func)Callback

{
return kPMUNotSupported;
}


// **********************************************************************************
// ADBPollEnable
//
// **********************************************************************************
- (PMUStatus)ADBPollEnable      :(UInt32)RefNum
                                :(id)Id
                                :(pmCallback_func)Callback

{
PMUmachMessage  request;
msg_return_t    return_code;


request.msgBody.pmCommand = kPMUpMgrADB;        // give it the list we have
request.msgBody.pmFlag = FALSE;
request.msgBody.pmSLength1 = 4;
request.msgBody.pmSBuffer2 = NULL;
request.msgBody.pmSLength2 = 0;
request.msgBody.pmRBuffer = NULL;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;
request.msgBody.pmSBuffer1[0] = 0;
request.msgBody.pmSBuffer1[1] = 0x86;
request.msgBody.pmSBuffer1[2] = (UInt8)(pollList >> 8);
request.msgBody.pmSBuffer1[3] = (UInt8)(pollList & 0xff);

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(PMUmachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
	autopollOn = TRUE;
        return kPMUNoError;
        }
else {
        return kPMUIOError;
        }
}


// **********************************************************************************
// ADBPollDisable
//
// **********************************************************************************
- (PMUStatus)ADBPollDisable	:(UInt32)RefNum
	                        :(id)Id
				:(pmCallback_func)Callback
{
PMUmachMessage	request;
msg_return_t	return_code;

request.msgBody.pmCommand = kPMUpMgrADBoff;
request.msgBody.pmFlag = FALSE;
request.msgBody.pmFlag = TRUE;
request.msgBody.pmSLength1 = 0;
request.msgBody.pmSBuffer2 = NULL;
request.msgBody.pmSLength2 = 0;
request.msgBody.pmRBuffer = NULL;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(PMUmachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
	autopollOn = FALSE;
	return kPMUNoError;
	}
else {
	return kPMUIOError;
	}
}


// **********************************************************************************
// ADBSetPollRate
//
// **********************************************************************************
- (PMUStatus)ADBSetPollRate     :(UInt32)NewRate
				:(UInt32)RefNum
                                :(id)Id
                                :(pmCallback_func)Callback
{
return kPMUNotSupported;
}


// **********************************************************************************
// ADBGetPollRate
//
// **********************************************************************************
- (PMUStatus)ADBGetPollRate     :(UInt32 *)CurrentRate
                                :(UInt32)RefNum
                                :(id)Id
                                :(pmCallback_func)Callback
{
return kPMUNotSupported;
}


// **********************************************************************************
// ADBSetAlternateKeyboard
//
// **********************************************************************************
- (PMUStatus)ADBSetAlternateKeyboard	:(UInt32)DevAddr
                                	:(UInt32)RefNum
                                	:(id)Id
                                	:(pmCallback_func)Callback
{
return kPMUNotSupported;
}


// **********************************************************************************
// ADBinput
//
// The PGE has interrupted with ADB data.  We package this up and send
// it to our ADB client, if there is one, either as the result to its previous
// read command, or as autopoll data.
//
// **********************************************************************************
- (void)ADBinput:(UInt32)theLength:(UInt8 *)theInput
{
if ( theInput[0] & kPMUautopoll ) {				// autopoll data?
	if ( ADBclient != NULL ) {
	  ADBclient(ADBid, 0, (theInput[1]>>4)&0x0F, theLength-2, theInput+2);
	  // yes, call adb input handler
	}
	return;
	}
if ( adb_reading ) {						// no, expecting adb input?
	if ( clientRequest->pmSBuffer1[0] == theInput[1] ) {	// yes, is it our input?
		ns_untimeout((func)timer_expired,(void *)port);	// yes, turn off our timer
		if ( clientRequest->pmCallback != NULL ) {	// Make the client callback
			clientRequest->pmCallback(clientRequest->pmId, clientRequest->pmRefNum, theLength-2, theInput+2);
			}
		adb_reading = FALSE;
		return;
		}
	}
kprintf("unexpected adb input: %02d %02x %02x %02x %02x\n", theLength, interruptState[0], interruptState[1], interruptState[2], interruptState[3]);
}


// **********************************************************************************
// registerForClockTicks
//
// The RTC driver is calling to tell us that it is prepared to receive clock
// ticks every second.  The parameter block tells who to call when we get one.
//
// **********************************************************************************
- (void)registerForClockTicks	:(pmCallback_func)TickHandler
				:(id)caller
{
RTCclient = TickHandler;
RTCid = caller;
}


// **********************************************************************************
// setRealTimeClock
//
// The RTC driver is calling to set the real time clock.  We translate this into
// a PMU command and enqueue it to our command queue.
//
// **********************************************************************************
- (PMUStatus)setRealTimeClock	:(UInt8 *)newTime
				:(UInt32)RefNum
	                        :(id)Id
                                :(pmCallback_func)Callback
{
PMUmachMessage  request;
msg_return_t    return_code;

if ( newTime == NULL ) {
	return kPMUParameterError;
	}

request.msgBody.pmCommand = kPMUtimeWrite;
request.msgBody.pmFlag = FALSE;
request.msgBody.pmSLength1 = 0;
request.msgBody.pmSBuffer2 = newTime;
request.msgBody.pmSLength2 = 4;
request.msgBody.pmRBuffer = NULL;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(PMUmachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
        return kPMUNoError;
        }
else {
        return kPMUIOError;
        }
}


// **********************************************************************************
// getRealTimeClock
//
// The RTC driver is calling to read the real time clock.  We translate this into
// a PMU command and enqueue it to our command queue.
//
// **********************************************************************************
- (PMUStatus)getRealTimeClock	:(UInt8 *)currentTime
                                :(UInt32)RefNum
	                        :(id)Id
                                :(pmCallback_func)Callback
{
PMUmachMessage  request;
msg_return_t    return_code;

if ( currentTime == NULL ) {
        return kPMUParameterError;
        }

request.msgBody.pmCommand = kPMUtimeRead;
request.msgBody.pmFlag = FALSE;
request.msgBody.pmSLength1 = 0;
request.msgBody.pmSBuffer2 = NULL;
request.msgBody.pmSLength2 = 0;
request.msgBody.pmRBuffer = currentTime;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(PMUmachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
        return kPMUNoError;
        }
else {
        return kPMUIOError;
        }
}


// **********************************************************************************
// readNVRAM
//
// The NVRAM driver is calling to read part of the NVRAM.  We translate this into
// single-byte PMU commands and enqueue them to our command queue.
//
// **********************************************************************************
- (PMUStatus) readNVRAM :(UInt32)Offset
                        :(UInt32)Length
			:(UInt8 *)Buffer
                        :(UInt32)RefNum
                        :(id)Id
                        :(pmCallback_func)Callback
{
PMUmachMessage  request;
msg_return_t    return_code;
int		i;
UInt8 *		client_buffer = Buffer;
UInt32		our_offset = Offset;

if ( (Buffer == NULL) ||
	(Length == 0) ||
	(Length > 8192) ||
	(Offset > 8192) ||
	((Length + Offset) > 8192) ) {
        return kPMUParameterError;
        }

for ( i = 0; i < (Length - 1); i++ ) {			// read all but the last byte
	request.msgBody.pmCommand = kPMUNVRAMRead;
	request.msgBody.pmFlag = FALSE;
	request.msgBody.pmSLength1 = 2;
	request.msgBody.pmSBuffer2 = NULL;
	request.msgBody.pmSLength2 = 0;
	request.msgBody.pmRBuffer = client_buffer++;
	request.msgBody.pmCallback = NULL;
	request.msgBody.pmSBuffer1[0] = our_offset >> 8;
	request.msgBody.pmSBuffer1[1] = our_offset++;

	request.msgHeader.msg_simple = TRUE;
	request.msgHeader.msg_type = MSG_TYPE_NORMAL;
	request.msgHeader.msg_remote_port = port;
	request.msgHeader.msg_local_port = PORT_NULL;
	request.msgHeader.msg_size = sizeof(PMUmachMessage);
	return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

	if ( return_code != SEND_SUCCESS ) {
        	return kPMUIOError;
        	}
	}

request.msgBody.pmCommand = kPMUNVRAMRead;			// now read last byte
request.msgBody.pmFlag = FALSE;
request.msgBody.pmSLength1 = 2;
request.msgBody.pmSBuffer2 = NULL;
request.msgBody.pmSLength2 = 0;
request.msgBody.pmRBuffer = client_buffer;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;
request.msgBody.pmSBuffer1[0] = our_offset >> 8;
request.msgBody.pmSBuffer1[1] = our_offset;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(PMUmachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
 	return kPMUNoError;
      	}
else {
        return kPMUIOError;
        }
}


// **********************************************************************************
// writeNVRAM
//
// The NVRAM driver is calling to write part of the NVRAM.  We translate this into
// single-byte PMU commands and enqueue them to our command queue.
//
// **********************************************************************************
- (PMUStatus) writeNVRAM:(UInt32)Offset
                        :(UInt32)Length
			:(UInt8 *)Buffer
                        :(UInt32)RefNum
                        :(id)Id
                        :(pmCallback_func)Callback
{
PMUmachMessage  request;
msg_return_t    return_code;
int		i;
UInt32		our_offset = Offset;
UInt8 *		client_buffer = Buffer;

if ( (Buffer == NULL) ||
        (Length == 0) ||
        (Length > 8192) ||
        (Offset > 8192) ||
        ((Length + Offset) > 8192) ) {
        return kPMUParameterError;
        }

for ( i = 0; i < (Length - 1); i++ ) {			// write all but the last byte
        request.msgBody.pmCommand = kPMUNVRAMWrite;
	request.msgBody.pmFlag = FALSE;
        request.msgBody.pmSLength1 = 3;
        request.msgBody.pmSBuffer2 = NULL;
        request.msgBody.pmSLength2 = 0;
        request.msgBody.pmRBuffer = NULL;
        request.msgBody.pmCallback = NULL;
	request.msgBody.pmSBuffer1[0] = our_offset >> 8;
	request.msgBody.pmSBuffer1[1] = our_offset++;
	request.msgBody.pmSBuffer1[2] = *client_buffer++;

	request.msgHeader.msg_simple = TRUE;
	request.msgHeader.msg_type = MSG_TYPE_NORMAL;
	request.msgHeader.msg_remote_port = port;
	request.msgHeader.msg_local_port = PORT_NULL;
	request.msgHeader.msg_size = sizeof(PMUmachMessage);
	return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

        if ( return_code != SEND_SUCCESS ) {
                return kPMUIOError;
                }
	}

request.msgBody.pmCommand = kPMUNVRAMWrite;		// write the last byte
request.msgBody.pmFlag = FALSE;
request.msgBody.pmSLength1 = 3;
request.msgBody.pmSBuffer2 = NULL;
request.msgBody.pmSLength2 = 0;
request.msgBody.pmRBuffer = NULL;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;
request.msgBody.pmSBuffer1[0] = our_offset >> 8;
request.msgBody.pmSBuffer1[1] = our_offset;
request.msgBody.pmSBuffer1[2] = *client_buffer;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(PMUmachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
        return kPMUNoError;
        }
else {
        return kPMUIOError;
        }
}


// **********************************************************************************
// registerForPowerInterrupts
//
// Some driver is calling to say it is prepared to receive "unsolicited" power-system
// interrups (e.g. battery low).  The parameter block says who to call when we get one.
//
// **********************************************************************************
- (void)registerForPowerInterrupts	:(pmCallback_func)buttonHandler
					:(id)caller
{
PWRclient = buttonHandler;
PWRid = caller;
}


// **********************************************************************************
// sendMiscCommand
//
// Some driver is calling to send some miscellaneous command.  We copy this into a
// PMU command and enqueue it to our command queue.
//
// **********************************************************************************
- (PMUStatus)sendMiscCommand	:(UInt32)Command
				:(UInt32)SLength
				:(UInt8 *)SBuffer
				:(UInt8 *)RBuffer
				:(UInt32)RefNum
	                        :(id)Id
				:(pmCallback_func)Callback
{
PMUmachMessage  request;
msg_return_t    return_code;
SInt32		rsp_length;
SInt32		send_length;

rsp_length = rspLengthTable[Command];                         	// get cmd and response lengths from table
send_length = cmdLengthTable[Command];

if ( ((SLength != 0) && (SBuffer == NULL)) ||          		// validate pointers
	((rsp_length != 0) && (RBuffer == NULL)) ) {
		return kPMUParameterError;
		}
if ( (Command != kPMUdownloadFlash) &&
	((send_length != -1) && (send_length != SLength)) ) {
                return kPMUParameterError;
                }

if ( send_length > MISC_LENGTH ) {
	return kPMUParameterError;
	}

request.msgBody.pmCommand = Command;
request.msgBody.pmFlag = FALSE;
request.msgBody.pmSLength1 = 0;
request.msgBody.pmSBuffer2 = SBuffer;
request.msgBody.pmSLength2 = SLength;
request.msgBody.pmRBuffer = RBuffer;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(PMUmachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
        return kPMUNoError;
        }
else {
        return kPMUIOError;
        }
}


// **********************************************************************************
// StartPMUTransmission
//
// Transmission of the command byte is started.  The transaction will be
// completed by the Shift Register Interrupt Service Routine.
// **********************************************************************************
- (void)StartPMUTransmission:(PMURequest *)plugInMessage
{
if ( !debugging ) {
	clientRequest = plugInMessage;
	firstChar = plugInMessage->pmCommand;		// get command byte
	charCountS1 = plugInMessage->pmSLength1;	// get caller's length counters
	charCountS2 = plugInMessage->pmSLength2;
	dataPointer1 = plugInMessage->pmSBuffer1;	// and transmit data pointers
	dataPointer2 = plugInMessage->pmSBuffer2;
	dataPointer = plugInMessage->pmRBuffer;		// set up read pointer for data bytes
	charCountR = rspLengthTable[firstChar];		// get response length from table
	charCountR2 = charCountR;

				// figure out what happens after command byte transmission
	if ( cmdLengthTable[firstChar] < 0 ) {	// will we be sending a length byte next?
		PGE_ISR_state = kPMUxmtLen;	// yes
		}
	else {					// no, will we be sending data next?
		if ( cmdLengthTable[firstChar] > 0 ) {
			PGE_ISR_state = kPMUxmtData;			// yes
			}
		else {							// no, will we be receiving a length byte next?
			if ( charCountR < 0 ) {
				PGE_ISR_state = kPMUreadLen_cmd;	// yes
				}
			else {						// no, will we be receiving data next?
				if ( charCountR > 0 ) {
					PGE_ISR_state = kPMUreadData;	// yes
					}
				else {
					PGE_ISR_state = kPMUdone;	// no, this is a single-byte transaction
					}
				}
			}
		}
								// ready to start the command byte
	*VIA1_auxillaryControl |= 0x1C;				// set shift register to output
	*VIA1_shift = firstChar;				// give it the byte (this clears any pending SR interrupt)
//	*VIA1_interruptEnable = 0x84;				// enable SR interrupt
	*VIA2_dataB &= ~PMreq;					// assert /REQ
	return;
	
	}
else {
	UInt32		i;
	
	*VIA1_interruptEnable = 0x04;							// disable SR interrupt

	firstChar = plugInMessage->pmCommand;						// get command byte
	charCountS1 = plugInMessage->pmSLength1;					// get caller's length counters
	charCountS2 = plugInMessage->pmSLength2;
	dataPointer1 = plugInMessage->pmSBuffer1;					// and transmit data pointers
	dataPointer2 = plugInMessage->pmSBuffer2;
	
	charCountR = rspLengthTable[firstChar];						// get response length from table
	charCountR2 = charCountR;
	
	[self SendPMUByte:firstChar];							// send command byte

	if ( cmdLengthTable[firstChar] < 0 ) {						// should we send a length byte?
		[self SendPMUByte:(UInt8)(charCountS1 + charCountS2)];			// yes, do it
		}

	for ( i = 0; i < charCountS1; i++ ) {						// send data bytes
		[self SendPMUByte:*dataPointer1++];
		}

	for ( i = 0; i < charCountS2; i++ ) {						// send more data bytes
		[self SendPMUByte:*dataPointer2++];
		}
/* charCountR ==	0:	no reply at all
				1:	only a reply byte will be sent by the PGE
				<0: a length byte and a reply will be sent
				>1: a reply will be sent, but no length byte
					 (length is charCount - 1)
*/
	if ( charCountR ) {								// receive the reply byte
		if ( charCountR == 1 ) {		
			[self ReadPMUByte:plugInMessage->pmRBuffer];
			}
		else {
			if ( charCountR < 0 ) {						// receive the length byte
				[self ReadPMUByte:&receivedByte];
				charCountR = receivedByte;
				}
			else {
				charCountR--;
				}
			dataPointer = plugInMessage->pmRBuffer;
			for ( i = 0; i < charCountR; i++ ) {
				[self ReadPMUByte:dataPointer++];			// receive the rest of the reply
				}
			}
		}

	if ( plugInMessage->pmCallback != NULL ) {				// Make the client callback
		plugInMessage->pmCallback(plugInMessage->pmId, plugInMessage->pmRefNum, charCountR, plugInMessage->pmRBuffer);
		}
	return;
	}
}



// ****************************************************************************
//	interruptOccurred
//	The shift register has finished shifting in a byte from PG&E or finished
//	shifting out a byte to PG&E.  Here we continue the transaction by starting
//	the i/o of the next byte, or we finish the transaction by calling the
//	client's callback function.
//	Both the VIA interrupt flag register and the interrupt enable registers
//	have been cleared by the ohare ISR.
// ****************************************************************************

- (void)interruptOccurred
{
*VIA2_dataB |= PMreq;					// deassert /REQ line
							// what state are we in?
switch ( PGE_ISR_state ) {
							// We are processing a PMU interrupt.  We are reading the response
							// to the kPMUreadINT command, and a byte has arrived.
	case kPMUrcvData_int:
		*dataPointer++ = *VIA1_shift;		// read the data byte
		charCountR2--;
		if ( charCountR2 > 0 ) {				// is there more to read?
			while ( !(*VIA2_dataB & PMack) ) {
				}
			*VIA2_dataB &= ~PMreq;				// yes, assert /REQ
//			*VIA1_interruptEnable = 0x84;			// enable SR interrupt
			return;						// next interrupt will be next data byte
			}
		if ( interruptState[0] & kPMUADBint ) {					// no, what kind of interrupt was it?
			[self ADBinput: (UInt32)charCountR: &interruptState[0]];	// ADB
			}
		else {
			if ( interruptState[0] & kPMUbattInt ) {
				kprintf("battery PGE interrupt\n");
				}
			else {
				if ( interruptState[0] & kPMUoneSecInt ) {
//					kprintf("one-second PGE interrupt\n");
					if ( RTCclient != NULL ) {			// one-second interrupt
        					RTCclient(RTCid,0,0,0);
        					}
					}
				else {
					if ( interruptState[0] & kPMUenvironmentInt ) {
						kprintf("environment interrupt\n");
						}
					else {
						if ( interruptState[0] & kPMUbrightnessInt ) {
							kprintf("brightness button PGE interrupt\n");
							}
						else {
							kprintf("machine-dependent PGE interrupt\n");
							}
						}
					}
				}
			}
		PGE_ISR_state = kPMUidle;					// set the state
		if ( !PMU_int_pending ) {					// is PMU requesting service again?
//		if ( !(*VIA1_interruptFlag & 0x10) ) {				// is PMU requesting service again? ( ifCB1 )
//			if ( queueHead == (PMUmachMessage*)0 ) {		// no, queue empty?
//				*VIA1_interruptEnable = 0x90;			// yes, enable PMU interrupts ( ifCB1 )
//				return;						// and we are completely idle
//				}
			[self CheckRequestQueue];				// no, start next queued transaction
			}
		else {
//			*VIA1_interruptFlag = 0x10;		// PMU wants service, acknowledge VIA interrupt ( ifCB1 )	
			PMU_int_pending = FALSE;
			*VIA1_auxillaryControl |= 0x1C;				// set shift register to output
			*VIA1_shift = kPMUreadINT;				// give it the command byte
			*VIA2_dataB &= ~PMreq;					// assert /REQ
//			*VIA1_interruptEnable = 0x84;				// enable SR interrupt
			PGE_ISR_state = kPMUreadLen_int;			// set the state
			dataPointer = &interruptState[0];			// set up read pointer for data bytes
			return;					// next interrupt is command byte transmission complete
			}
		return;
		
							// We are processing a PMU interrupt.
							// We have finished transmitting the kPMUreadINT command byte, and
							// according to our table, we will be getting a response and a
							// length byte for it.  Finish the transmit handshake and set up
	case kPMUreadLen_int:				// a receive for the length byte.
		receivedByte = *VIA1_shift;					// read shift reg to turn off SR int
		PGE_ISR_state = kPMUrcvLen_int;
		*VIA1_auxillaryControl &= 0xEF;					// set shift register to input
		while ( !(*VIA2_dataB & PMack) ) {
			}
		*VIA2_dataB &= ~PMreq;						// assert /REQ
//		*VIA1_interruptEnable = 0x84;					// enable SR interrupt
		return;								// next interrupt will be the length byte

							// We are processing a PMU interrupt.
	case kPMUrcvLen_int:				// The length byte has arrived.  Read it and start data read
		charCountR = *VIA1_shift;		// read it

		charCountR2 = charCountR;
		PGE_ISR_state = kPMUrcvData_int;
		while ( !(*VIA2_dataB & PMack) ) {
			}
		*VIA2_dataB &= ~PMreq;						// assert /REQ
//		*VIA1_interruptEnable = 0x84;					// enable SR interrupt
		return;								// next interrupt will be the first data byte
			
							// We are doing a command transaction.  The command byte transmission
	case kPMUxmtLen:				// has completed.  Start length byte transmission
		PGE_ISR_state = kPMUxmtData;
		while ( !(*VIA2_dataB & PMack) ) {
			}
		*VIA1_shift = (UInt8)(charCountS1 + charCountS2);		// give it the length byte
		*VIA2_dataB &= ~PMreq;						// assert /REQ
//		*VIA1_interruptEnable = 0x84;					// enable SR interrupt
		return;								// next interrupt start sending data
		
							// We are doing a command transaction.  A byte transmission has completed.
	case kPMUxmtData:				// Continue data byte transmission
		while ( !(*VIA2_dataB & PMack) ) {
			}
		if ( charCountS1 ) {
			*VIA1_shift = *dataPointer1++;				// give it the next data byte from buffer 1
			*VIA2_dataB &= ~PMreq;					// assert /REQ 
			if ( --charCountS1 + charCountS2 ) {
//				*VIA1_interruptEnable = 0x84;			// enable SR interrupt
				return;						// next interrupt do another byte
				}
			}
		else {
			if ( charCountS2 ) {
				*VIA1_shift = *dataPointer2++;		// buffer 1 empty, give it the next byte from buffer 2
				*VIA2_dataB &= ~PMreq;			// assert /REQ
				if ( --charCountS2 ) {
//					*VIA1_interruptEnable = 0x84;	// enable SR interrupt
					return;				// next interrupt do another byte
					}
				}
			}
										// sending last byte, what's next?
		if ( charCountR < 0 ) {
			PGE_ISR_state = kPMUreadLen_cmd;			// we will receive a length byte
			}
		else {
			if ( charCountR > 0 ) {
				PGE_ISR_state = kPMUreadData;			// we will receive constant-length data
				}
			else {
				PGE_ISR_state = kPMUdone;			// nothing, we're done
				}
			}
//		*VIA1_interruptEnable = 0x84;					// enable SR interrupt
		return;
		
							// We have finished the transmission part of a command transaction, and
							// according to our table, we will be getting a response and a
							// length byte for it.  Finish the transmit handshake and set up
	case kPMUreadLen_cmd:				// a receive for the length byte.
		receivedByte = *VIA1_shift;					// read shift reg to turn off SR int
		PGE_ISR_state = kPMUrcvLen_cmd;
		*VIA1_auxillaryControl &= 0xEF;					// set shift register to input
		while ( !(*VIA2_dataB & PMack) ) {
			}
		*VIA2_dataB &= ~PMreq;						// assert /REQ
//		*VIA1_interruptEnable = 0x84;					// enable SR interrupt
		return;								// next interrupt will be the length byte

	case kPMUrcvLen_cmd:				// the length byte has arrived, read it and start data read
		charCountR = *VIA1_shift;				// read it
		charCountR2 = charCountR;
		PGE_ISR_state = kPMUrcvData_cmd;
		if ( !(*VIA2_dataB & PMack) )
			if ( ![self WaitForAckHi] ) {
				return;						// make sure ACK is high
				}
		*VIA2_dataB &= ~PMreq;						// assert /REQ
//		*VIA1_interruptEnable = 0x84;					// enable SR interrupt
		return;								// next interrupt will be the first data byte
	
		
							// We have finished the transmission part of a command transaction, and
							// according to our table, we will be getting a response but not a
							// length byte for it.  Finish the transmit handshake and set up
	case kPMUreadData:				// a receive for the first data byte.
		if ( charCountR > 1 ) {
			charCountR2--;						// make constant (byte count + 1) into byte count
			charCountR--;
			}
//		receivedByte = *VIA1_shift;					// read shift reg to turn off SR int
		PGE_ISR_state = kPMUrcvData_cmd;
		*VIA1_auxillaryControl &= 0xEF;					// set shift register to input
		if ( !(*VIA2_dataB & PMack) )
			if ( ![self WaitForAckHi] ) {
				return;						// make sure ACK is high
				}
		*VIA2_dataB &= ~PMreq;						// assert /REQ
//		*VIA1_interruptEnable = 0x84;					// enable SR interrupt
		return;								// next interrupt will be the first data character
		
								// We are reading the response in a command transaction, and
	case kPMUrcvData_cmd:					// a data byte has arrived
		*dataPointer++ = *VIA1_shift;					// read the data byte
		charCountR2--;
		if ( charCountR2 > 0 ) {					// is there more to read?
			if ( !(*VIA2_dataB & PMack) )
				if ( ![self WaitForAckHi] ) {
					return;					// yes, make sure ACK is high
					}
			*VIA2_dataB &= ~PMreq;					// assert /REQ
			return;							// next interrupt will be next data byte
			}
		if ( clientRequest->pmCallback != NULL ) {			// no, make the client callback
			clientRequest->pmCallback(clientRequest->pmId, clientRequest->pmRefNum, charCountR, clientRequest->pmRBuffer);
			}
		PGE_ISR_state = kPMUidle;						// set the state
		if ( !PMU_int_pending ) {						// is PMU now requesting service?
//		if ( !(*VIA1_interruptFlag & 0x10) ) {					// is PMU now requesting service? (ifCB1)
//			if ( queueHead == (PMUmachMessage*)0 ) {			// no, queue empty?
//				*VIA1_interruptEnable = 0x90;				// yes, enable PMU interrupts ( ifCB1 )
//				return;							// and we are completely idle
//				}
			[self CheckRequestQueue];					// no, start next queued transaction
			}
		else {
//			*VIA1_interruptFlag = 0x10;		// PMU wants service, acknowledge VIA interrupt ( ifCB1 )	
			PMU_int_pending = FALSE;
			*VIA1_auxillaryControl |= 0x1C;					// set shift register to output
			*VIA1_shift = kPMUreadINT;					// give it the command byte
			*VIA2_dataB &= ~PMreq;						// assert /REQ
//			*VIA1_interruptEnable = 0x84;					// enable SR interrupt
			PGE_ISR_state = kPMUreadLen_int;				// set the state
			dataPointer = &interruptState[0];				// set up read pointer for data bytes
			return;					// next interrupt is command byte transmission complete
			}
		return;

	case kPMUdone:						// this was the last xmt SR interrupt of a command transaction
//		receivedByte = *VIA1_shift;					// read shift reg to turn off SR int

		if ( clientRequest->pmFlag ) {					// does this command cause input?
               		PGE_ISR_state = kPMUidle;                               // yes, set the state
			adb_reading = TRUE;					// don't do callback now
//			who_to_call = clientRequest->pmCallback;		// do it after the read completes
//			theirId = clientRequest->pmId;
//			theirRefNum = clientRequest->pmRefNum;
			ns_timeout((func)timer_expired,(void *)port,adb_read_timeout,CALLOUT_PRI_SOFTINT0);	// start timer
			if ( !PMU_int_pending ) {					// is PMU now requesting service?
//			if ( !(*VIA1_interruptFlag & 0x10) ) {				// is PMU now requesting service? (ifCB1)
//				*VIA1_interruptEnable = 0x90;				// yes, enable PMU interrupts ( ifCB1 )
				return;							// and we are completely idle
				}
			else {
//				*VIA1_interruptFlag = 0x10;		// PMU wants service, acknowledge VIA interrupt ( ifCB1 )	
				PMU_int_pending = FALSE;
				*VIA1_auxillaryControl |= 0x1C;					// set shift register to output
				*VIA1_shift = kPMUreadINT;					// give it the command byte
				*VIA2_dataB &= ~PMreq;						// assert /REQ
//				*VIA1_interruptEnable = 0x84;					// enable SR interrupt
				PGE_ISR_state = kPMUreadLen_int;				// set the state
				dataPointer = &interruptState[0];				// set up read pointer for data bytes
				return;					// next interrupt is command byte transmission complete
				}
			}
										// not an adb read
		if ( clientRequest->pmCallback != NULL ) {				// Make the client callback
			clientRequest->pmCallback(clientRequest->pmId, clientRequest->pmRefNum, 0, NULL);
			}
		if ( !PMU_int_pending ) {						// is PMU now requesting service?
//		if ( !(*VIA1_interruptFlag & 0x10) ) {					// is PMU now requesting service? (ifCB1)
//			if ( queueHead == (PMUmachMessage*)0  ) {			// no, queue empty?
				PGE_ISR_state = kPMUidle;
//				*VIA1_interruptEnable = 0x90;				// yes, enable PMU interrupts ( ifCB1 )
//				return;							// and we are completely idle
//				}
			[self CheckRequestQueue];					// no, start next queued transaction
			}
		else {
			*VIA1_interruptFlag = 0x10;		// PMU wants service, acknowledge VIA interrupt ( ifCB1 )	
			PMU_int_pending = FALSE;
			*VIA1_auxillaryControl |= 0x1C;					// set shift register to output
			*VIA1_shift = kPMUreadINT;					// give it the command byte
			*VIA2_dataB &= ~PMreq;						// assert /REQ
//			*VIA1_interruptEnable = 0x84;					// enable SR interrupt
			PGE_ISR_state = kPMUreadLen_int;				// set the state
			dataPointer = &interruptState[0];				// set up read pointer for data bytes
			return;					// next interrupt is command byte transmission complete
			}
		return;
	}
return;
}

// ****************************************************************************
//	interruptOccurredAt
//	PGE has interrupted.  Send the ReadInt command to find out why.
//	When the command byte is sent, the Shift Register will interrupt.
//	If we are mid-transaction when we find out about the interrupt,
//	set a flag and find out why later.
//
// ****************************************************************************

- (void)interruptOccurredAt:(int)localInterrupt
{
if ( PGE_ISR_state != kPMUidle ) {
	PMU_int_pending = TRUE;
	return;
	}
if ( !debugging ) {
								// make sure ACK is high
//	*VIA1_interruptFlag = 0x10;				// acknowledge VIA interrupt ( ifCB1 )
//	*VIA1_interruptEnable = 0x10;				// and disable it entirely ( ifCB1 )
	while ( !(*VIA2_dataB & PMack) ) {
		}
	*VIA1_auxillaryControl |= 0x1C;				// set shift register to output
	*VIA1_shift = kPMUreadINT;				// give it the command byte
	*VIA2_dataB &= ~PMreq;					// assert /REQ
//	*VIA1_interruptEnable = 0x84;				// enable SR interrupt
	PGE_ISR_state = kPMUreadLen_int;			// set the state
	dataPointer = &interruptState[0];			// set up read pointer for data bytes
	return;							// return till character transmission completes
	}
else {
	PMURequest	getInterruptState;			// debug mode PMU interrupt handler

//	[self AcknowledgePMUInterrupt];				// turn off VIA interrupt
	*VIA1_interruptEnable = 0x04;				// disable SR interrupt
	
	getInterruptState.pmCommand = kPMUreadINT;		// find out cause of interrupt from PGE
	getInterruptState.pmFlag = FALSE;
	getInterruptState.pmSLength1 = 0;
	getInterruptState.pmSLength2 = 0;
	getInterruptState.pmRBuffer = &interruptState[0];
	getInterruptState.pmCallback = gotInterruptCause;
	getInterruptState.pmId = self;
	
	[self StartPMUTransmission:&getInterruptState];
	}
}

	
// ****************************************************************************
// gotInterruptCause
// 
// Called by the debug-mode PMU interrupt handler as the Callback function
// after sending the kPMUreadInt command and receiving its response
// ****************************************************************************
void gotInterruptCause(id PMUdriver, UInt32 unused, UInt32 length, UInt8 * data)
{
UInt8 interruptSource;

interruptSource = *data;
	
if ( interruptSource & kPMUADBint ) {
	[PMUdriver ADBinput: length: data];
	}
else {
	if ( interruptSource & kPMUbattInt ) {
		IOLog("battery PGE interrupt");
		}
	else {
		if ( interruptSource & kPMUoneSecInt ) {
			IOLog("one-second PGE interrupt");
			}
		else {
			if ( interruptSource & kPMUenvironmentInt ) {
				IOLog("environment interrupt");
				}
			else {
				if ( interruptSource & kPMUbrightnessInt ) {
					IOLog("brightness button PGE interrupt");
					}
				else {
					IOLog("machine-dependent PGE interrupt");
					}
				}
			}
		}
	}
}


// ****************************************************************************
// SendPMUByte
// ****************************************************************************
- (void)SendPMUByte:(UInt8)theByte
{
*VIA1_auxillaryControl |= 0x1C;         // set shift register to output
eieio();
*VIA1_shift = theByte;                  // give it the byte
eieio();
*VIA2_dataB &= ~PMreq;   		// assert /REQ
eieio();
if ( [self WaitForAckLo] ) {            // ack now low
        *VIA2_dataB |= PMreq;    	// deassert /REQ line
        eieio();
        if ( ! [self WaitForAckHi] ) {
                return;
                }
        }
else {
        *VIA2_dataB |= PMreq;		// deassert /REQ line
        eieio();
        return;
        }
return;
}


// ****************************************************************************
// ReadPMUByte
// ****************************************************************************
- (void)ReadPMUByte:(UInt8 *)theByte
{
*VIA1_auxillaryControl |= 0x0C;                 // set shift register to input
*VIA1_auxillaryControl &= ~0x10;
*theByte = *VIA1_shift;                         // read a byte to reset shift reg
eieio();
*VIA2_dataB &= ~PMreq;           		// assert /REQ
eieio();
if ( [self WaitForAckLo] ) {                    // ack now low
        *VIA2_dataB |= PMreq;    		// deassert /REQ line
        eieio();
        if ( [self WaitForAckHi] ) {            // wait for /ACK high
                *theByte = *VIA1_shift;         // got it, read the byte
                eieio();
                }
        else {
                return;
                }
        }
else {
        *VIA2_dataB |= PMreq;    		// deassert /REQ line
        eieio();
        return;
        }
return;
}


// ****************************************************************************
// WaitForAckLo
// ****************************************************************************
- (Boolean)WaitForAckLo
{
struct timeval startTime;
struct timeval currentTime;
ns_time_t x;

// wait up to 32 milliseconds for Ack signal from PG&E to go low

IOGetTimestamp(&x);
ns_time_to_timeval(x, &startTime);		        		// get current time

while ( TRUE ) {
        if ( !(*VIA2_dataB & PMack) ) {
                return ( TRUE );					// ack is low, return
                }
	IOGetTimestamp(&x);
        ns_time_to_timeval(x, &currentTime);
	if ( startTime.tv_usec > currentTime.tv_usec ) {
		currentTime.tv_usec += 1000000;				// clock has wrapped, adjust it
		}
	if ( currentTime.tv_usec > (startTime.tv_usec + 32000) ) {	// has 32 ms elapsed?
                return ( FALSE );					// yes, return
                }
        }
}


// ****************************************************************************
// WaitForAckHi
// ****************************************************************************
- (Boolean)WaitForAckHi
{
struct timeval startTime;
struct timeval currentTime;
ns_time_t x;

// wait up to 32 milliseconds for Ack signal from PG&E to go high

IOGetTimestamp(&x);
ns_time_to_timeval(x, &startTime);		        		// get current time

while ( TRUE ) {
	if ( *VIA2_dataB & PMack ) {
		return ( TRUE );					// ack is high, return
		}
	IOGetTimestamp(&x);
        ns_time_to_timeval(x, &currentTime);
        if ( startTime.tv_usec > currentTime.tv_usec ) {
                currentTime.tv_usec += 1000000;                         // clock has wrapped, adjust it
                }
        if ( currentTime.tv_usec > (startTime.tv_usec + 32000) ) {      // has 32 ms elapsed?
                return ( FALSE );                                       // yes, return
                }
        }
}


// ****************************************************************************
// GetPMUInterruptState
// ****************************************************************************
- (UInt8)GetPMUInterruptState
{				// return current state of CB1 int enable
return (*VIA1_interruptEnable & (1<<ifCB1));
}


// ****************************************************************************
// RestorePMUInterrupt
// ****************************************************************************
- (void)RestorePMUInterrupt:(UInt8)savedValue
{
if ( savedValue ) {		// restore VIA interrupt state
	*VIA1_interruptEnable = savedValue | 0x80;
	}
eieio();
}


// ****************************************************************************
// DisablePMUInterrupt
// ****************************************************************************
- (void)DisablePMUInterrupt
{
*VIA1_interruptEnable = 1<<ifCB1;
eieio();
}


// ****************************************************************************
// EnablePMUInterrupt
// ****************************************************************************
- (void)EnablePMUInterrupt
{
*VIA1_interruptEnable = (1<<ifCB1) | 0x80;
eieio();
}


// ****************************************************************************
// AcknowledgePMUInterrupt
// ****************************************************************************
- (void)AcknowledgePMUInterrupt
{
*VIA1_interruptFlag = 1<<ifCB1;
eieio();
}


// ****************************************************************************
// GetSRInterruptState
// ****************************************************************************
- (UInt8)GetSRInterruptState
{					// return current state of SR int enable
return (*VIA1_interruptEnable & (1<<ifSR));
}


// ****************************************************************************
// RestoreSRInterrupt
// ****************************************************************************
- (void)RestoreSRInterrupt:(UInt8)savedValue
{
if ( savedValue ) {			// restore SR interrupt state
	*VIA1_interruptEnable = savedValue | 0x80;
	eieio();
	}
}


// ****************************************************************************
// DisableSRInterrupt
// ****************************************************************************
- (void)DisableSRInterrupt
{
*VIA1_interruptEnable = 1<<ifSR;
}


// ****************************************************************************
// EnableSRInterrupt
// ****************************************************************************
- (void)EnableSRInterrupt
{
*VIA1_interruptEnable = (1<<ifSR) | 0x80;
}



// ****************************************************************************
// timer_expired
//
// Our adb-read timer has expired, so we have to notify our i/o thread by
// enqueuing a Timeout message to its interrupt port.
// ****************************************************************************
void timer_expired(port_t mach_port)
{
PMUmachMessage	request;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_id = IO_TIMEOUT_MSG;
request.msgHeader.msg_remote_port = mach_port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(msg_header_t);
msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

}


@end
