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


#ifndef __APPLE_TYPES_DEFINED__
#define __APPLE_TYPES_DEFINED__ 1

    typedef unsigned int    UInt32;         // A 32-bit unsigned integer
    typedef unsigned char   UInt8;          // A "byte-sized" integer
    typedef signed char SInt8;
    typedef signed int      SInt32;         // A 32-bit signed integer
    typedef boolean_t       Boolean;        // TRUE/FALSE value (YES/NO in NeXT)
    typedef signed int  OSErr;
    typedef signed int  OSStatus;

#endif /* __APPLE_TYPES_DEFINED__ */



struct adb_packet {
    unsigned char   a_header[8];
    int     a_hcount;
    unsigned char   a_buffer[32];
    int     a_bcount;
    int     a_bsize;
};

typedef struct adb_packet adb_packet_t;


struct adb_request {
    simple_lock_data_t	a_lock;
    int         a_result;   /* Result of ADB command */
    int         a_flags;    /* used internally */
    adb_packet_t        a_cmd;      /* Command packet */
    adb_packet_t        a_reply;    /* Reply packet */
    void            (*a_done)(struct adb_request *);
    struct adb_request  *a_next;
};

typedef struct adb_request adb_request_t;

/*
 * ADB Packet Types
 */

#define ADB_PACKET_ADB      0
#define ADB_PACKET_PSEUDO   1
#define ADB_PACKET_ERROR    2
#define ADB_PACKET_TIMER    3
#define ADB_PACKET_POWER    4
#define ADB_PACKET_MACIIC   5

/*
 * ADB Device Commands 
 */

#define ADB_ADBCMD_RESET_BUS    0x00
#define ADB_ADBCMD_FLUSH_ADB    0x01
#define ADB_ADBCMD_WRITE_ADB    0x08
#define ADB_ADBCMD_READ_ADB 0x0c

/*
 * ADB Pseudo Commands
 */

#define ADB_PSEUDOCMD_WARM_START        0x00
#define ADB_PSEUDOCMD_START_STOP_AUTO_POLL  0x01
#define ADB_PSEUDOCMD_GET_6805_ADDRESS      0x02
#define ADB_PSEUDOCMD_GET_REAL_TIME     0x03
#define ADB_PSEUDOCMD_GET_PRAM          0x07
#define ADB_PSEUDOCMD_SET_6805_ADDRESS      0x08
#define ADB_PSEUDOCMD_SET_REAL_TIME     0x09
#define ADB_PSEUDOCMD_POWER_DOWN        0x0a
#define ADB_PSEUDOCMD_SET_POWER_UPTIME      0x0b
#define ADB_PSEUDOCMD_SET_PRAM          0x0c
#define ADB_PSEUDOCMD_MONO_STABLE_RESET     0x0d
#define ADB_PSEUDOCMD_SEND_DFAC         0x0e
#define ADB_PSEUDOCMD_BATTERY_SWAP_SENSE    0x10
#define ADB_PSEUDOCMD_RESTART_SYSTEM        0x11
#define ADB_PSEUDOCMD_SET_IPL_LEVEL     0x12
#define ADB_PSEUDOCMD_FILE_SERVER_FLAG      0x13
#define ADB_PSEUDOCMD_SET_AUTO_RATE     0x14
#define ADB_PSEUDOCMD_GET_AUTO_RATE     0x16
#define ADB_PSEUDOCMD_SET_DEVICE_LIST       0x19
#define ADB_PSEUDOCMD_GET_DEVICE_LIST       0x1a
#define ADB_PSEUDOCMD_SET_ONE_SECOND_MODE   0x1b
#define ADB_PSEUDOCMD_SET_POWER_MESSAGES    0x21
#define ADB_PSEUDOCMD_GET_SET_IIC       0x22
#define ADB_PSEUDOCMD_ENABLE_DISABLE_WAKEUP 0x23
#define ADB_PSEUDOCMD_TIMER_TICKLE      0x24
#define ADB_PSEUDOCMD_COMBINED_FORMAT_IIC   0X25


struct CudaRequest {
	struct CudaMachMessage *	next;
	struct CudaMachMessage *	prev;
	adb_packet_t        		a_cmd;      /* Command packet */
	adb_packet_t        		a_reply;    /* Reply packet */
	pmCallback_func			pmCallback;
	id				pmId;
	UInt32				pmRefNum;
};

typedef struct CudaRequest CudaRequest;

struct CudaMachMessage {
	msg_header_t	msgHeader;
	msg_type_t	msgType;
	CudaRequest	msgBody;
};

typedef struct CudaMachMessage CudaMachMessage;


@interface AppleCuda : IODirectDevice <ADBservice, RTCservice>
{

VIAAddress      VIA1_shift;     // pointers to VIA registers
VIAAddress      VIA1_auxillaryControl;
VIAAddress      VIA1_interruptFlag;
VIAAddress      VIA1_interruptEnable;
VIAAddress      VIA2_dataB;
UInt8           PMack;          // ack bit

pmADBinput_func		ADBclient;		// Input handler in ADB client
id			ADBid;			// id of ADB client
pmCallback_func		RTCclient;		// Tick handler in RTC client
id			RTCid;			// id of RTC client
pmCallback_func     PWRclient;      // Button handler in Power Management client
id          PWRid;          // id of Power Management client

CudaRequest *		clientRequest;
CudaMachMessage		localMachMessage;
UInt8           	firstChar;
UInt32          	charCountS1;
UInt8 *         	dataPointer1;

CudaMachMessage *       queueHead;		// our command queue
CudaMachMessage *	queueTail;
port_t			port;			// our interrupt port
pmCallback_func		who_to_call;		// ADB client's callback for solicited input
id			theirId;		// ADB client's id for input callback
UInt32			theirRefNum;		// ADB client's refnum for input callback
ns_time_t		adb_read_timeout;	// timeout on read to absent adb device
UInt8		auto_power_on;

}


+ (Boolean)probe : (IODeviceDescription *)deviceDescription;	// initialize the driver

- initFromDeviceDescription : (IODeviceDescription *)deviceDescription;

- free;                                         		// shut the driver down

- (void)interruptOccurred;

- (void)timeoutOccurred;

- (void)receiveMsg;



								// ADB protocol
- (void)registerForADBAutopoll	:(pmCallback_func)inputHandler
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


- (PMUStatus)ADBSetFileServerMode:(UInt32)RefNum
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

- (PMUStatus)setPowerupTime	:(UInt8 *)newTime
			 	:(UInt32)RefNum
        	    :(id)Id
                :(pmCallback_func)Callback;


- (PMUStatus)CudaMisc		:(UInt8 *)output
				:(UInt32)length
				:(UInt32)RefNum
	                        :(id)Id
				:(pmCallback_func)Callback;

								// private methods

// - (void)ADBinput:(UInt32)theLength:(UInt8 *)theInput;

- (void)StartCudaTransmission:(CudaRequest *)plugInMessage;

- (void)EnableCudaInterrupt;

- (void)CheckRequestQueue;

-(void) cuda_error;
-(void) cuda_idle;
-(void) cuda_collision;
-(void) cuda_receive_last_byte;
-(void) cuda_receive_data;
-(void) cuda_unexpected_attention;
-(void) cuda_expected_attention;
-(void) cuda_transmit_data;
-(void) cuda_process_response;
-(void) poll_device;


@end










