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


/*********

cuda.m

This file is a combination of the old cuda.c and the new pmu.m for PGE.
Cuda is based on a Motorola MC68HC05E1.

NOTES:

	"WARNING" look for this string during integration.
	cuda interrupts are cleared by reading the shift register.
	cuda has a TIP (transaction in progress) signal which the PGE seems to lack.
	PGE seems to require a "length" byte in command packets while cuda does not.
	Signals:
		TIP is asserted when 0.  System asserts this when transaction is in progress.
		BYTEACK is asserted when 0.  When data is passed from the system to Cuda,
			a toggle indicates to Cuda that the VIA data register is full.  When
			data is passed from Cuda to the system, a toggle indicates that the
			system has read the data from the VIA and the VIA data register
			is now empty.  When no transaction in progress (note TIP), this
			line is deasserted.
		TREQ is asserted when 0.  When asserted by Cuda, this indicates to the system
			that Cuda is making a Transaction REQUEST.  During a transaction which
			is initiated by Cuda (as opposed to one initiated by system), this line
			is negated prior to generating a VIA interrupt for the last byte
			of data transacted.  When no transaction is in progress (note TIP),
			this line is negated.


*********/


#include <sys/param.h>  //for sysctl
#include <sys/proc.h>   //for sysctl also (but not documented)

#import <kern/clock.h>
#import <kernserv/prototypes.h>
#import <kernserv/clock_timer.h>
#import <kernserv/ns_timer.h>
#import <sys/time.h>
#import <sys/callout.h>
#import <machdep/ppc/proc_reg.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>

#import <bsd/dev/ppc/drvPMU/pmu.h>
#import <bsd/dev/ppc/drvPMU/pmupriv.h>
#import <bsd/dev/ppc/drvPMU/pmumisc.h>
#include <sys/sysctl.h>  //for debug2, debug3, debug4 in sysctl
#import "via6522.h"
#import "cuda_hdw.h"
#import "cuda.h"        //must come after pmu definitions

//typedefs from cuda.c:
//
//  CudaInterruptState - internal 
//
//#define kNS_TIMEOUT_AUTOPOLL  9000000000ULL
#define kNS_TIMEOUT_AUTOPOLL  4000000000ULL

enum CudaInterruptState
{
    CUDA_STATE_INTERRUPT_LIMBO  = -1,       //
    CUDA_STATE_IDLE         = 0,        //
    CUDA_STATE_ATTN_EXPECTED    = 1,        //
    CUDA_STATE_TRANSMIT_EXPECTED    = 2,        //
    CUDA_STATE_RECEIVE_EXPECTED = 3         //
};

typedef enum CudaInterruptState CudaInterruptState;


//
//  CudaTransactionFlag - internal to cuda2.m, used to be in cuda.c
//

enum CudaTransactionFlag
{
    CUDA_TS_NO_REQUEST  = 0x0000,
    CUDA_TS_SYNC_RESPONSE   = 0x0001,
    CUDA_TS_ASYNC_RESPONSE  = 0x0002
};

typedef enum CudaTransactionFlag CudaTransactionFlag;


void gotInterruptCause(id, UInt32, UInt32, UInt8 *);
void cuda_timer_autopoll(port_t mach_port);
void timer_expired(port_t mach_port);  //WARNING... not needed by Cuda?  Was not in cuda.c

extern id ApplePMUId;	//in adb.m (remove this when adb becomes an indirect driver)(q8q)

extern void kprintf(const char *, ...);
extern void bcopy(void *, void *, int);
// extern to let us fix up the boot time.
extern void set_boot_time(void);
extern msg_send_from_kernel(msg_header_t *, int, int);

// Variables copied from cuda.c:
static long     cuda_state_transition_delay_ticks;
volatile CudaInterruptState cuda_interrupt_state;
volatile CudaTransactionFlag    cuda_transaction_state;
static port_t   glob_port;

//WARNING Hack to make pmutables.h work
extern SInt8 *cmdLengthTable;   
extern SInt8 *rspLengthTable;


CudaRequest   *cuda_request = NULL, *cuda_collided = NULL;
adb_packet_t    cuda_unsolicited;
adb_packet_t    *cuda_current_response = NULL;

int     cuda_transfer_count = 0;
boolean_t   cuda_is_header_transfer = FALSE;
boolean_t   cuda_is_packet_type = FALSE;
boolean_t   cuda_initted = FALSE;
boolean_t	debugging = FALSE;
boolean_t	adb_reading = FALSE;

static UInt8 	*return_buff_pointer = NULL;
static boolean_t bImmediate_buff_needed = FALSE;

// for Debug
pmADBinput_func		cuda_debug_client;		// Input handler in ADB client
static VIAAddress	stat_aux_control;
static boolean_t	p = FALSE;
int	que_cuda_count;
int	cuda_glob_dbug_freeze; //Can only be asserted through debugging tool
int	cuda_glob_data1;
int	cuda_glob_data2;
int cuda_freeze_counter;  //This will become "static" after debugging phase
int cuda_freeze_prevcount;  

//sysctl debugging tool 
struct ctldebug debug2 = { "cuda_glob_dbug_freeze", &cuda_glob_dbug_freeze };
//struct ctldebug debug3 = { "cuda_glob_data1", &cuda_glob_data1 };
//struct ctldebug debug4 = { "cuda_glob_data2", &cuda_glob_data2 };


@implementation AppleCuda

// **********************************************************************************
// probe
//
// 
//
// **********************************************************************************
+ (Boolean) probe : deviceDescription
{
  id dev;

    if (cuda_initted)
        return 1;

    cuda_initted = TRUE;
	que_cuda_count = 0;
	cuda_glob_dbug_freeze = 0;  // 0.  Only external debugger can change it now.
	cuda_freeze_counter = 1;  //2 vars needed to recover from BYTEACK hang
	cuda_freeze_prevcount = 0;  

	if ( (dev = [ self alloc ]) == nil ) {
		return NO;
	}

	if ([dev initFromDeviceDescription:deviceDescription] == nil) {
		return NO;
	}
	
	// tell ADB about us.  This is a global variable accessible in adb.m
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
volatile unsigned char valcuda;
VIAAddress	physicalAddress;
IORange *	ioRange;
unsigned char	*cuda_temp_reg;

struct timeval	x = {0,1000000};	// one second timeout on adb reads

auto_power_on = 1;	//default is to enable auto power on feature (after electrical power failure)

if ( [super initFromDeviceDescription:deviceDescription] == nil ) {
	[self free];
	return nil;
	}

  [self setDeviceKind:"Cuda Subsystem"];
  [self setLocation:NULL];
  [self setName:"Cuda"];  


ioRange = [deviceDescription memoryRangeList];

physicalAddress = (VIAAddress)ioRange->start;
stat_aux_control = physicalAddress;

//above works perfectly physicalAddress = (unsigned char*)POWERMAC_IO(PCI_VIA_BASE_PHYS);


//CUDA_REPLACE


VIA1_shift		= physicalAddress + 0x1400;	// initialize VIA addresses
VIA1_auxillaryControl 	= physicalAddress + 0x1600;

VIA1_interruptFlag	= physicalAddress + 0x1A00;
VIA1_interruptEnable	= physicalAddress + 0x1C00;
VIA2_dataB		= physicalAddress + 0x0000;		// Hooper uses VIA 1 instead


PMack			= kCudaAssertByteAcknowledge;	//in cuda_hdw.h is 0x10



    cuda_state_transition_delay_ticks = nsec_to_processor_clock_ticks(200);
	//This is just 2 ticks
    // Set the direction of the cuda signals.  ByteACk and TIP are output and
    // TREQ is an input

	cuda_temp_reg = physicalAddress + 0x0400;	//dataDirectionB
    *cuda_temp_reg |= (kCudaByteAcknowledgeMask | kCudaTransferInProgressMask);
    *cuda_temp_reg &= ~kCudaTransferRequestMask;

    *VIA1_auxillaryControl 	= ( *VIA1_auxillaryControl 	| kCudaTransferMode) &
                                             kCudaSystemRecieve;

    // Clear any posible cuda interupt.
    if ( *VIA1_shift );

    // Initialize the internal data.

    cuda_interrupt_state    = CUDA_STATE_IDLE;
    cuda_transaction_state  = CUDA_TS_NO_REQUEST;
    cuda_is_header_transfer = FALSE;
    cuda_is_packet_type = FALSE;
    cuda_transfer_count = 0;    
    cuda_current_response   = NULL;     
    
    // Terminate transaction and set idle state
    // cuda_neg_tip_and_byteack();
	*VIA2_dataB |= kCudaNegateByteAcknowledge | kCudaNegateTransferInProgress;
	eieio();
    
    // we want to delay 4 mS for ADB reset to complete
    
    delay(4000);
    
    // Clear pending interrupt if any...
    //(void)cuda_read_data();
	valcuda = *VIA1_shift; eieio();
    
    // Issue a Sync Transaction, ByteAck asserted while TIP is negated.
    // cuda_assert_byte_ack();
	*VIA2_dataB &= kCudaAssertByteAcknowledge; eieio();


    // Wait for the Sync acknowledgement, cuda to assert TREQ
    // cuda_wait_for_transfer_request_assert();
	while ( (*VIA2_dataB & kCudaTransferRequestMask) != 0 ) 
	{
		eieio();
	}
	eieio();

    // Wait for the Sync acknowledgement interrupt.
    // cuda_wait_for_interrupt();
	while ( (*VIA1_interruptFlag & kCudaInterruptMask) == 0 ) 
	{
		eieio();
	}
	eieio();


    // Clear pending interrupt
    //(void)cuda_read_data();
	valcuda = *VIA1_shift; eieio();

    // Terminate the sync cycle by Negating ByteAck
    // cuda_neg_byte_ack();
	*VIA2_dataB |= kCudaNegateByteAcknowledge; eieio();

    // Wait for the Sync termination acknowledgement, cuda negates TREQ.
    // cuda_wait_for_transfer_request_neg();
	while ( (*VIA2_dataB & kCudaTransferRequestMask) == 0 ) 
	{
		eieio();
	}
	eieio();


    // Wait for the Sync termination acknowledgement interrupt.
    // cuda_wait_for_interrupt();
	while ( (*VIA1_interruptFlag & kCudaInterruptMask) == 0 ) 
	{
		eieio();
	}
	eieio();


    // Terminate transaction and set idle state, TIP negate and ByteAck negate.
    // cuda_neg_transfer_in_progress();
	*VIA2_dataB |= kCudaNegateTransferInProgress; eieio();

    // Clear pending interrupt, if there is one...
    //(void)cuda_read_data();
	valcuda = *VIA1_shift; eieio();





ADBclient = NULL;
RTCclient = NULL;
debugging = FALSE;
queueHead = NULL;
queueTail = NULL;
adb_reading = FALSE;
adb_read_timeout = timeval_to_ns_time(&x);

//CUDA_REPLACE
// A.W. This call seems to have no counterpart in Cuda
// 	Pending interrupts are cleared with cuda_read_data()
	*VIA1_interruptEnable = kCudaInterruptDisable;eieio(); // turn off any pending interrupt
valcuda = *VIA1_shift; eieio();

[self EnableCudaInterrupt];   // enable PGE interrupts. CUDA_REPLACE already

[self enableAllInterrupts];

//WARNING... this was moved here in new pmu.m
	if ([self startIOThread] != IO_R_SUCCESS) {
        [self free];
        return nil;
	}
	port = IOConvertPort([self interruptPort],IO_KernelIOTask,IO_Kernel);
	glob_port = port;  //needed for non-ObjC functions
	[self registerDevice];


//A.W. I don't think Cuda needs to exclude any interrupts
    //I need to set up a 5-second timer which will make sure that the Auto Poll
    // feature of Cuda is never accidentally disabled by anybody
    ns_timeout((func)cuda_timer_autopoll,(void *)port, kNS_TIMEOUT_AUTOPOLL ,CALLOUT_PRI_SOFTINT0);  // start timer


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
// receiveMsg
//
// 
//
// **********************************************************************************


- (void)receiveMsg
{
	CudaMachMessage * toQueue;
	IOReturn result;
	spl_t	intstate;
	//unsigned char	interruptState;

    //interruptState =  (*VIA2_dataB & kCudaInterruptStateMask) | (*VIA1_auxillaryControl & kCudaDirectionMask) ;
    //if ( (interruptState == kCudaIdleState) && !adb_reading ) 

    if  (cuda_interrupt_state == CUDA_STATE_IDLE)
    {
	
if (p) kprintf("cuda2.m::receiveMsg will Xmit now... \n");
		//localMachMessage is class variable in cuda.h 
	    localMachMessage.msgHeader.msg_size = sizeof(CudaMachMessage);
        localMachMessage.msgHeader.msg_local_port = [self interruptPort];
		result = msg_receive(&localMachMessage.msgHeader, (msg_option_t)RCV_TIMEOUT, 0);

#ifdef OMIT
		if ( localMachMessage.msgBody.pmCallback == NULL ) 
		{
			kprintf("ADAM: receiveMsg no callback\n");
		}
#endif

	    if ( result == RCV_SUCCESS ) {
			//Try Simon's suggestion
			intstate = spltty();
		    [self StartCudaTransmission:&localMachMessage.msgBody];
			splx(intstate);
		}
	}
    else 
    {
if (p) kprintf("cuda2.m::receiveMsg will enqueue instead \n");

#ifdef DEBUG
		if (cuda_glob_dbug_freeze == 1)	
		{
			kprintf("cuda: Queued\n");
		}
#endif

	    toQueue = (CudaMachMessage*)kalloc(sizeof(CudaMachMessage));
        toQueue->msgHeader.msg_size = sizeof(CudaMachMessage);
        toQueue->msgHeader.msg_local_port = [self interruptPort];
        result = msg_receive(&toQueue->msgHeader, (msg_option_t)RCV_TIMEOUT, 0);
        if ( result == RCV_SUCCESS ) {
		    toQueue->msgBody.prev = queueTail;
		    toQueue->msgBody.next = NULL;
		    if ( queueTail != NULL ) {
			    queueTail->msgBody.next = toQueue;
			}
			else 
			{
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
	CudaMachMessage * nextRequest;

	if ( queueHead == NULL ) {		           // is queue empty?
if (p) kprintf("cuda2.m::CheckRequestQueue empty \n");
        [self EnableCudaInterrupt];            // yes, enable interrupt and return
    }
	else 
	{
		que_cuda_count--;
if (p) kprintf("cuda2.m::CheckRequestQueue will dequeue \n");
        nextRequest = queueHead;                        // no, dequeue first command
        queueHead = nextRequest->msgBody.next;
	    if ( queueHead == NULL ) {
		    queueTail = NULL;
		}
		bcopy (&nextRequest->msgBody, &localMachMessage.msgBody, sizeof(CudaRequest));	// copy it
        kfree(nextRequest, sizeof(CudaMachMessage));				     	// free its memory
        [self StartCudaTransmission:&localMachMessage.msgBody];				// and send it to the Cuda
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
- (void)registerForADBAutopoll	:(pmCallback_func)InputHandler
				:(id)caller
{
if (p) kprintf("registerForADBAutopoll: %08x   caller: %08x\n", InputHandler, caller);
  ADBclient = InputHandler;
  cuda_debug_client = ADBclient;		// Debug use only
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
	CudaMachMessage	request;
	msg_return_t	return_code;

if (p) kprintf("ADBWrite\n");
	
	if (ByteCount > 7)
	{
		kprintf("bsd-dev-ppc-drvCuda-cuda.m: ADBWrite may fail\n");
	}

	request.msgBody.a_cmd.a_header[0] = ADB_PACKET_ADB;
	request.msgBody.a_cmd.a_header[1] = ADB_ADBCMD_WRITE_ADB | DevAddr << 4 | DevReg;
	request.msgBody.a_cmd.a_header[2] = Buffer[0];
	request.msgBody.a_cmd.a_header[3] = Buffer[1];
	request.msgBody.a_cmd.a_header[4] = Buffer[2]; //necessary?
	request.msgBody.a_cmd.a_header[5] = Buffer[3];
	request.msgBody.a_cmd.a_header[6] = Buffer[4];
	request.msgBody.a_cmd.a_header[7] = Buffer[5];
	request.msgBody.a_cmd.a_header[8] = Buffer[6];

	//request.msgBody.a_cmd.a_hcount = 4; 
	request.msgBody.a_cmd.a_hcount = ByteCount + 2;
	//Dave added unlimited-length adb_writereg()

	request.msgBody.a_cmd.a_bcount = 0;
	request.msgBody.pmCallback = Callback;

#ifdef OMIT
	if (Callback == NULL)
	{
		kprintf("Cuda: ADBWrite callback is NULL\n");
	}
#endif

	request.msgBody.pmId = Id;
	request.msgBody.pmRefNum = RefNum;

	request.msgHeader.msg_simple = TRUE;
	request.msgHeader.msg_type = MSG_TYPE_NORMAL;
	request.msgHeader.msg_remote_port = port;
	request.msgHeader.msg_local_port = PORT_NULL;
	request.msgHeader.msg_size = sizeof(CudaMachMessage);
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
CudaMachMessage	request;
msg_return_t	return_code;

request.msgBody.a_cmd.a_header[0] = ADB_PACKET_ADB;
request.msgBody.a_cmd.a_header[1] = ADB_ADBCMD_READ_ADB | DevAddr << 4 | DevReg;
request.msgBody.a_cmd.a_hcount = 2;
//request.msgBody.a_cmd.a_bsize = sizeof(request.msgBody.a_cmd.a_buffer);  
request.msgBody.a_reply.a_bsize = 8; //It's 8 in adb.h
//WARNING... try out line above 3/4/98 TitanL kernel 
 
request.msgBody.a_cmd.a_bcount = 0;
request.msgBody.pmCallback = Callback;

#ifdef OMIT
	if (Callback == NULL)
	{
		kprintf("Cuda: ADBRead callback is NULL\n");
	}
#endif

request.msgBody.pmId = Id;
request.msgBody.pmRefNum = RefNum;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(CudaMachMessage);
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
	CudaMachMessage	request;
	msg_return_t	return_code;

kprintf("CUDA: ADBReset\n");
	request.msgBody.a_cmd.a_header[0] = ADB_PACKET_ADB;
	request.msgBody.a_cmd.a_header[1] = ADB_ADBCMD_RESET_BUS;
	request.msgBody.a_cmd.a_hcount = 2;
	request.msgBody.a_cmd.a_bcount = 0;
	request.msgBody.pmCallback = Callback;
	request.msgBody.pmId = Id;
	request.msgBody.pmRefNum = RefNum;

	request.msgHeader.msg_simple = TRUE;
	request.msgHeader.msg_type = MSG_TYPE_NORMAL;
	request.msgHeader.msg_remote_port = port;
	request.msgHeader.msg_local_port = PORT_NULL;
	request.msgHeader.msg_size = sizeof(CudaMachMessage);
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
CudaMachMessage	request;
msg_return_t	return_code;

if (p) kprintf("ADBFlush\n");
request.msgBody.a_cmd.a_header[0] = ADB_PACKET_ADB;
request.msgBody.a_cmd.a_header[1] = ADB_ADBCMD_FLUSH_ADB | (DevAddr << 4);
request.msgBody.a_cmd.a_hcount = 2;
request.msgBody.a_cmd.a_bcount = 0;
request.msgBody.pmId = Id;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmCallback = Callback;
		
request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(CudaMachMessage);
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
//  
// **********************************************************************************
- (PMUStatus)ADBSetPollList	:(UInt32)PollBitField
				:(UInt32)RefNum
	            :(id)Id
				:(pmCallback_func)Callback
{
CudaMachMessage	request;
msg_return_t	return_code;

if (p) kprintf("ADBSetPollList\n");
request.msgBody.a_cmd.a_header[0] = ADB_PACKET_PSEUDO;
request.msgBody.a_cmd.a_header[1] = ADB_PSEUDOCMD_SET_DEVICE_LIST;
request.msgBody.a_cmd.a_header[2] = (PollBitField >> 8) & 0xFF;
request.msgBody.a_cmd.a_header[3] = PollBitField & 0xFF;
request.msgBody.a_cmd.a_hcount = 4;
request.msgBody.a_cmd.a_bcount = 0;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

#ifdef OMIT
	if (Callback == NULL)
	{
		kprintf("Cuda: ADBSetPollList callback is NULL\n");
	}
#endif


request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(CudaMachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
	return kPMUNoError;
	}
else {
	return kPMUIOError;
	}
}




// **********************************************************************************
// ADBSetFileServerMode()
//
// **********************************************************************************
- (PMUStatus)ADBSetFileServerMode       :(UInt32)RefNum
                                		:(id)Id
                                		:(pmCallback_func)Callback

{
	CudaMachMessage  request;
	msg_return_t    return_code;

	request.msgBody.a_cmd.a_header[0] = ADB_PACKET_PSEUDO;
	request.msgBody.a_cmd.a_header[1] = ADB_PSEUDOCMD_FILE_SERVER_FLAG;
	request.msgBody.a_cmd.a_header[2] = TRUE;
	request.msgBody.a_cmd.a_hcount = 3;
	request.msgBody.a_cmd.a_bcount = 0;
	request.msgBody.pmRefNum = RefNum;
	request.msgBody.pmId = Id;
	request.msgBody.pmCallback = Callback;

	request.msgHeader.msg_simple = TRUE;
	request.msgHeader.msg_type = MSG_TYPE_NORMAL;
	request.msgHeader.msg_remote_port = port;
	request.msgHeader.msg_local_port = PORT_NULL;
	request.msgHeader.msg_size = sizeof(CudaMachMessage);
	return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

	if ( return_code == SEND_SUCCESS ) {
        return kPMUNoError;
        }
	else {
        return kPMUIOError;
        }
}


// **********************************************************************************
// ADBPollEnable
//
// **********************************************************************************
- (PMUStatus)ADBPollEnable      :(UInt32)RefNum
                                :(id)Id
                                :(pmCallback_func)Callback

{
CudaMachMessage  request;
msg_return_t    return_code;

if (p) kprintf("ADBPollEnable\n");
request.msgBody.a_cmd.a_header[0] = ADB_PACKET_PSEUDO;
request.msgBody.a_cmd.a_header[1] = ADB_PSEUDOCMD_START_STOP_AUTO_POLL;
request.msgBody.a_cmd.a_header[2] = TRUE;
request.msgBody.a_cmd.a_hcount = 3;
request.msgBody.a_cmd.a_bcount = 0;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(CudaMachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
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
CudaMachMessage	request;
msg_return_t	return_code;

if (p) kprintf("ADBPollDisable\n");
request.msgBody.a_cmd.a_header[0] = ADB_PACKET_PSEUDO;
request.msgBody.a_cmd.a_header[1] = ADB_PSEUDOCMD_START_STOP_AUTO_POLL;
request.msgBody.a_cmd.a_header[2] = FALSE;
request.msgBody.a_cmd.a_hcount = 3;
request.msgBody.a_cmd.a_bcount = 0;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(CudaMachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
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
CudaMachMessage	request;
msg_return_t	return_code;

if (p) kprintf("ADBSetPollRate\n");
request.msgBody.a_cmd.a_header[0] = ADB_PACKET_PSEUDO;
request.msgBody.a_cmd.a_header[1] = ADB_PSEUDOCMD_SET_AUTO_RATE;
request.msgBody.a_cmd.a_header[2] = NewRate;
request.msgBody.a_cmd.a_hcount = 3;
request.msgBody.a_cmd.a_bcount = 0;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(CudaMachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
	return kPMUNoError;
	}
else {
	return kPMUIOError;
	}
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
// Removed since cuda_process_response() does the same thing
// **********************************************************************************


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
CudaMachMessage  request;
msg_return_t    return_code;

if (p) kprintf("ADBSetRealTimeClock\n");
if ( newTime == NULL ) {
	return kPMUParameterError;
	}

request.msgBody.a_cmd.a_header[0] = ADB_PACKET_PSEUDO;
request.msgBody.a_cmd.a_header[1] = ADB_PSEUDOCMD_SET_REAL_TIME;
request.msgBody.a_cmd.a_hcount = 2;
request.msgBody.a_cmd.a_buffer[0] = newTime[0];
request.msgBody.a_cmd.a_buffer[1] = newTime[1];
request.msgBody.a_cmd.a_buffer[2] = newTime[2];
request.msgBody.a_cmd.a_buffer[3] = newTime[3];
request.msgBody.a_cmd.a_bcount = 4;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(CudaMachMessage);
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
// Called from PowerSurgeMB.m
//
// **********************************************************************************
- (PMUStatus)getRealTimeClock	:(UInt8 *)currentTime
                                :(UInt32)RefNum
	                            :(id)Id
                                :(pmCallback_func)Callback
{
CudaMachMessage  request;
msg_return_t    return_code;

if (p) kprintf("ADBGetRealTimeClock\n");

request.msgBody.a_cmd.a_header[0] = ADB_PACKET_PSEUDO;
request.msgBody.a_cmd.a_header[1] = ADB_PSEUDOCMD_GET_REAL_TIME;
request.msgBody.a_cmd.a_hcount = 2;
request.msgBody.a_cmd.a_bcount = 0;  //A.W. added 2/27.  Is it enough?  Response Specs:
									 //  Attention Byte
									 //  Response Type = PSEUDO
									 //  Response Flag
									 //  Command (was)
									 //  Data (Real Time Clock MSB)
									 //  Data 
									 //  Data 
									 //  Data (RTC LSB)
									 //  Idle Byte

//The a_cmd.a_buffer is filled in by cuda_transmit_data()... but I need receive_data
//Called from PowerSurgeMB.m
/*** following commented out because it doesn't look right
request.msgBody.a_cmd.a_buffer[0] = currentTime[0];
request.msgBody.a_cmd.a_buffer[1] = currentTime[1];
request.msgBody.a_cmd.a_buffer[2] = currentTime[2];
request.msgBody.a_cmd.a_buffer[3] = currentTime[3];
******/
bImmediate_buff_needed = TRUE;
return_buff_pointer = currentTime;
request.msgBody.a_reply.a_bsize = 8; //It's 8 in adb.h

request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(CudaMachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
        return kPMUNoError;
        }
else {
        return kPMUIOError;
        }
}



// **********************************************************************************
// setPowerupTime
// This sets the future time (in seconds) when Cuda will power-up the system again
// If the power is already on then nothing happens.  TRICKLE SENSE must be asserted.
//
// **********************************************************************************
- (PMUStatus)setPowerupTime	:(UInt8 *)newTime
				:(UInt32)RefNum
	            :(id)Id
                :(pmCallback_func)Callback
{
CudaMachMessage  request;
msg_return_t    return_code;

	if ( newTime == NULL ) {
		return kPMUParameterError;
	}

	if ( auto_power_on == 0) {
        return kPMUNoError;
	}

	request.msgBody.a_cmd.a_header[0] = ADB_PACKET_PSEUDO;
	request.msgBody.a_cmd.a_header[1] = ADB_PSEUDOCMD_SET_POWER_UPTIME;
	request.msgBody.a_cmd.a_hcount = 2;
	request.msgBody.a_cmd.a_buffer[0] = newTime[0];
	request.msgBody.a_cmd.a_buffer[1] = newTime[1];
	request.msgBody.a_cmd.a_buffer[2] = newTime[2];
	request.msgBody.a_cmd.a_buffer[3] = newTime[3];
	request.msgBody.a_cmd.a_bcount = 4;
	request.msgBody.pmRefNum = RefNum;
	request.msgBody.pmId = Id;
	request.msgBody.pmCallback = Callback;

	request.msgHeader.msg_simple = TRUE;
	request.msgHeader.msg_type = MSG_TYPE_NORMAL;
	request.msgHeader.msg_remote_port = port;
	request.msgHeader.msg_local_port = PORT_NULL;
	request.msgHeader.msg_size = sizeof(CudaMachMessage);
	return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

	if ( return_code == SEND_SUCCESS ) {
        return kPMUNoError;
        }
	else {
        return kPMUIOError;
        }
}


// **********************************************************************************
// CudaMisc
//
//
// **********************************************************************************
- (PMUStatus)CudaMisc		:(UInt8 *)output
				:(UInt32)length
				:(UInt32)RefNum
	            :(id)Id
				:(pmCallback_func)Callback
{
CudaMachMessage  request;
msg_return_t    return_code;
int		i;

for ( i = 0; i < length; i++ ) {
	//I should check for length < 8 but I trust callers in adb.m and PowerSurgeMB.m
	request.msgBody.a_cmd.a_header[i] = output[i];
	}

request.msgBody.a_cmd.a_hcount = length;
request.msgBody.a_cmd.a_bcount = 0;
//Don't need to worry about how big the caller's buffer is because
// cuda's adb_packet_t buffer is 8 bytes, and it's the caller who
// must copy the bytes in its callback function
request.msgBody.a_reply.a_bsize = 8; //It's 8 in adb.h for adb_packet_type
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(CudaMachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
        return kPMUNoError;
        }
else {
        return kPMUIOError;
        }

}



//This function calls back the ADB client (such as Mouse driver).
-(void) cuda_process_response
{

	if ( cuda_transaction_state == CUDA_TS_SYNC_RESPONSE ) 
	{
		if ( cuda_request->pmCallback != NULL ) {	// make the client callback

if (p) kprintf("cuda_process_response: pmId =  %08x, a_bcount = %08x, a_buffer = %08x\n",
cuda_request->pmId, cuda_request->a_reply.a_bcount, cuda_request->a_reply.a_buffer);


			cuda_request->pmCallback ( cuda_request->pmId,
				cuda_request->pmRefNum,
				cuda_request->a_reply.a_bcount, 
				cuda_request->a_reply.a_buffer  );
		}
		else
		{
			//It's OK to not have a callback since some callers don't care
			// about synchronous completion
			//kprintf("WARNING: Cuda process_response() has no callback\n");
		}
		return_buff_pointer = NULL; //for getRealTimeClock only.  a_buffer[]
									//above is ignored by PowerSurgeMB.m
		bImmediate_buff_needed = FALSE;
//kprintf("cuda.m: cuda_process_response end \n");
	}

	else 
	{
		if ( cuda_transaction_state == CUDA_TS_ASYNC_RESPONSE ) 
		{
			if ( ADBclient != NULL ) 
			{		// call the client input handler
				cuda_freeze_counter++;

if (p) {
kprintf("cuda2.m::cuda_process_response ASYNC \n");
kprintf("cuda2.m::ASYNC count is %d\n", cuda_current_response->a_bcount);
kprintf("cuda2.m::ASYNC buffer 0 is %x\n", cuda_current_response->a_buffer[0]);
kprintf("cuda2.m::ASYNC buffer 1 is %x\n", cuda_current_response->a_buffer[1]);
kprintf("cuda2.m::ASYNC buffer 2 is %x\n", cuda_current_response->a_buffer[2]);
kprintf("cuda2.m::ASYNC buffer 3 is %x\n", cuda_current_response->a_buffer[3]);
kprintf("cuda2.m::ASYNC buffer 4 is %x\n", cuda_current_response->a_buffer[4]);
kprintf("cuda2.m::ASYNC buffer 5 is %x\n", cuda_current_response->a_buffer[5]);
kprintf("cuda2.m::ASYNC header 0 is %x\n", cuda_current_response->a_header[0]);
kprintf("cuda2.m::ASYNC header 1 is %x\n", cuda_current_response->a_header[1]);
kprintf("cuda2.m::ASYNC header 2 is %x\n", cuda_current_response->a_header[2]);
}

if (p) kprintf("cuda_process_response: ADBclient = %08x, ADBid = %08x\n", ADBclient, ADBid);

#ifdef DEBUG
		cuda_glob_data1 = cuda_current_response->a_buffer[0];
		cuda_glob_data2 = cuda_current_response->a_buffer[1];
		if (cuda_glob_dbug_freeze == 1)	
		{
			//kprintf("%x %x \n", cuda_glob_data1, cuda_glob_data2);
		}
#endif
				//ADBClient is the global variable that clients registered with
 ADBclient(ADBid,
	   0,
	   (cuda_current_response->a_header[2] >> 4) & 0xf,
	   cuda_current_response->a_bcount,
	   &cuda_current_response->a_buffer[0]);
			}
		}
	}
}
	

// **********************************************************************************
// readNVRAM
//
// The NVRAM driver is calling to read part of the NVRAM.  We translate this into
// a PMU command and enqueue it to our command queue.
//
// **********************************************************************************
- (PMUStatus) readNVRAM :(UInt32)Offset
                        :(UInt32)Length
			:(UInt8 *)Buffer
                        :(UInt32)RefNum
                        :(id)Id
                        :(pmCallback_func)Callback
{
	return kPMUNotSupported;
}


// **********************************************************************************
// writeNVRAM
//
// The NVRAM driver is calling to write part of the NVRAM.  We translate this into
// a PMU command and enqueue it to our command queue.
//
// **********************************************************************************
- (PMUStatus) writeNVRAM:(UInt32)Offset
                        :(UInt32)Length
			:(UInt8 *)Buffer
                        :(UInt32)RefNum
                        :(id)Id
                        :(pmCallback_func)Callback
{
	return kPMUNotSupported;
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
// A.W. Not completed yet for Cuda commands!!
// **********************************************************************************
- (PMUStatus)sendMiscCommand	:(UInt32)Command
				:(UInt32)SLength
				:(UInt8 *)SBuffer
				:(UInt8 *)RBuffer
				:(UInt32)RefNum
	                        :(id)Id
				:(pmCallback_func)Callback
{
CudaMachMessage  request;
msg_return_t    return_code;
spl_t		interruptState;


	if (Command == kPMUPmgrPWRoff)
	{
		//kprintf("\nCuda received power-down message\n");
		request.msgBody.a_cmd.a_header[1] = ADB_PSEUDOCMD_POWER_DOWN;
		auto_power_on = 0;	//console operator really wants to shut off power
	}
	else if (Command == kPMUresetCPU)
	{
		request.msgBody.a_cmd.a_header[1] = ADB_PSEUDOCMD_RESTART_SYSTEM;
		//kprintf("\nCuda received reset message\n");
	}
	else
	{
		return kPMUParameterError;
	}

	//A.W. 3/17/98 I found out that reboots cannot happen from within
	// the mini-monitor because interrupts are turned off, which means
	// Cuda cannot get the subsequent command packet bytes to reboot
	// However, Simon says that next 2 lines are not necessary
	// WARNING The current file is checked out from trunk, don't check in
	interruptState = spltty();
	splx(interruptState);
//request.msgBody.a_reply.a_bsize = 0; //we don't expect any replies for two commands above
request.msgBody.a_reply.a_bsize = 8; //reset fails WARNING
request.msgBody.a_cmd.a_header[0] = ADB_PACKET_PSEUDO;
request.msgBody.a_cmd.a_hcount = 2;
request.msgBody.a_cmd.a_bcount = 0;
request.msgBody.pmRefNum = RefNum;
request.msgBody.pmId = Id;
request.msgBody.pmCallback = Callback;

request.msgHeader.msg_simple = TRUE;
request.msgHeader.msg_type = MSG_TYPE_NORMAL;
request.msgHeader.msg_remote_port = port;
request.msgHeader.msg_local_port = PORT_NULL;
request.msgHeader.msg_size = sizeof(CudaMachMessage);
return_code = msg_send_from_kernel(&request.msgHeader, MSG_OPTION_NONE, 0);

if ( return_code == SEND_SUCCESS ) {
        return kPMUNoError;
        }
else {
        return kPMUIOError;
        }
}


// **********************************************************************************
// StartCudaTransmission
//
// Called with PMU interrupts disabled. q8q
// Transmission of the command byte is started.  The transaction will be
// completed by the Shift Register Interrupt Service Routine.
// **********************************************************************************
- (void)StartCudaTransmission:(CudaRequest *)plugInMessage;
{


	if (( *VIA2_dataB & kCudaTransferRequestMask) == 0)
	{
		//Simon says there's a collision here
    	cuda_interrupt_state = CUDA_STATE_ATTN_EXPECTED;
    	cuda_transaction_state = CUDA_TS_ASYNC_RESPONSE;
		[self cuda_queue: plugInMessage];
		return;
	}

	//MAJOR CHANGE 8/6/98... 2 lines were at beginning, IDLE didn't make sense there
	if ( cuda_interrupt_state == CUDA_STATE_IDLE)
	{
	clientRequest = plugInMessage;
	firstChar = plugInMessage->a_cmd.a_header[0];	//Should be ADB_PACKET or PSEUDO
	charCountS1 = plugInMessage->a_cmd.a_hcount;

	dataPointer1 = &plugInMessage->a_cmd.a_header[0];

    cuda_request = plugInMessage;
    cuda_request->a_reply.a_bcount = 0;


	// cuda_set_data_direction_to_output();	//On cuda value is 0x10;
	*VIA1_auxillaryControl |= kCudaSystemSend; eieio();

    // ADBPollDisable is handled how?
	*VIA1_shift = firstChar;	// give it the byte (this clears any pending SR interrupt)

    // Set up the transfer state info here.

    cuda_is_header_transfer = TRUE;
    cuda_transfer_count = 1;



	// *VIA2_dataB &= ~kCudaNegateTransferRequest;	// yes, assert /REQ
	//I think I actually need to assert TIP here, not REQ.
	//eieio();


    // cuda_neg_byte_ack(); 
	*VIA2_dataB |= kCudaNegateByteAcknowledge; eieio();

    // cuda_assert_transfer_in_progress();
	*VIA2_dataB	 &= kCudaAssertTransferInProgress; eieio();
    
    // The next state is going to be a transmit state, if there is
    // no collision.  This is a requested response but call it sync.
    
    cuda_interrupt_state = CUDA_STATE_TRANSMIT_EXPECTED;

	//INVESTIGATE:  we get here even if there was a collision and the
	// collision packet was put onto the queue.  In that case it means
	// the transaction was ASYNC?
	// This entire function is protected by splt so no interrupts should
	// be happening at all.
    cuda_transaction_state = CUDA_TS_SYNC_RESPONSE;

	return;
	
	}
}



//This ISR is copied from cuda.c.  The nice thing about Cuda is that at any
// time we can determine state of hardware by examining 3 bits.
- (void)interruptOccurred
{

    unsigned char   interruptState;

	*VIA2_dataB |= kCudaNegateTransferRequest;	// deassert /REQ 

    // Get the relevant signal in determining the cause of the interrupt:
    // the shift direction, the transfer request line and the transfer
    // request line.

    //interruptState = cuda_get_interrupt_state();
    interruptState =  (*VIA2_dataB & kCudaInterruptStateMask) | (*VIA1_auxillaryControl & kCudaDirectionMask) ;
	
#ifdef DEBUG
	if ( cuda_glob_dbug_freeze == 1)	
	{
		printf(" %d: ", interruptState);
	}
#endif

	//no use DELAY(12);	//12 microsecond delay in Ray Montagne's Cuda Manager for MacOS

    switch ( interruptState ) {

    case kCudaReceiveByte:
if (p) kprintf("CUDA ** INTERRUPT ** RECEIVE *** \n");
		[self cuda_receive_data];
        break;

    case kCudaReceiveLastByte:
if (p) kprintf("CUDA ** INTERRUPT ** RECEIVE LAST BYTE *** \n");
		[self cuda_receive_last_byte];
        break;

    case kCudaTransmitByte:
if (p) kprintf("CUDA ** INTERRUPT ** TRANSMIT *** \n");
		[self cuda_transmit_data];
        break;

    case kCudaUnexpectedAttention:
if (p) kprintf("CUDA ** INTERRUPT ** Unexpected Attention *** \n");
		[self cuda_unexpected_attention];
        break;

    case kCudaExpectedAttention:
if (p) kprintf("CUDA ** INTERRUPT ** EXPECTED ATTENTION  *** \n");
		[self cuda_expected_attention];
        break;

    case kCudaIdleState:
if (p) kprintf("CUDA ** INTERRUPT ** IDLE *** \n");
		[self cuda_idle];
        break;

    case kCudaCollision:
if (p) kprintf("Cuda *** INTERRUPT *** ADB collision\n");
		[self cuda_collision];
        break;

    // Unknown interrupt, clear it and leave
    default:
if (p) kprintf("Cuda *** INTERRUPT *** ERROR\n");
		[self cuda_error];
        break;
    }
}


// ****************************************************************************
//	interruptOccurredAt
//	PGE has interrupted.  Send the ReadInt command to find out why.
//	When the command byte is sent, the Shift Register will interrupt.
//	If we are mid-transaction when we find out about the interrupt,
//	set a flag and find out why later.
//
//  A.W. Removed 2/11/98 because cuda only has 1 interrupt
//
// ****************************************************************************


	
// ****************************************************************************
// gotInterruptCause
// 
// Called by the debug-mode PMU interrupt handler as the Callback function
// after sending the kPMUreadInt command and receiving its response
// A.W. only called by interruptOccurredAt above, so remove as well
// ****************************************************************************


// ****************************************************************************
// EnableCudaInterrupt  used to be EnablePMUInterrupt
// ****************************************************************************
- (void)EnableCudaInterrupt
{

	// cuda_enable_interrupt();	// from cuda_hdw.h
	*VIA1_interruptEnable = kCudaInterruptEnable; 
	eieio();

}


#ifdef OMIT
// ****************************************************************************
// timer_expired
//
// Our adb-read timer has expired, so we have to notify our i/o thread by
// enqueuing a Timeout message to its interrupt port.
// This was only called by ADBinput() in pmu.m, but cuda doesn't have ADBinput()
// so this method is useless for now.
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

#endif



// ****************************************************************************
// cuda_timer_autopoll
//
// Automatic scheduling of command to make sure auto-polling is always available
// In worst case, I may want to reset Cuda here.
// ****************************************************************************
void cuda_timer_autopoll(port_t mach_port)
{  
    CudaMachMessage request;
	extern int	kdp_flag;
	static int	old_kdp_flag;
	static int 	cuda_TREQ_history;
	VIAAddress	temp_register, dataB;
	unsigned char	interruptState;
	//unsigned int    newTime;

#ifdef DEBUG
	//[ApplePMUId ADBPollEnable :0:0: NULL];  //stress this one
	//newTime = 0;
	//[ApplePMUId getRealTimeClock :(UInt8 *)&newTime : 0: 0: NULL];

	//*stat_aux_control &= kCudaSystemRecieve; eieio();
    ns_timeout((func)cuda_timer_autopoll,(void *)glob_port, kNS_TIMEOUT_AUTOPOLL,CALLOUT_PRI_SOFTINT0);  // start timer again for another 4 seconds
	//Can only be set through debugging tool
	switch (cuda_glob_dbug_freeze )	
	{
		case 0:
			old_kdp_flag = kdp_flag;
			break;
		case 1:
			kdp_flag |= 0x02;
			temp_register = stat_aux_control + 0x1600;  //auxillary
			dataB = stat_aux_control ;
    		interruptState =  (*dataB & kCudaInterruptStateMask) | (*temp_register & kCudaDirectionMask) ;
			printf("CUDA: 4-second periodic, interruptState is %x\n", interruptState);
			kprintf("CUDA: 4-second periodic, interruptState is %x\n", interruptState);

#ifdef OMIT_CUDA_DBUG
			//kprintf("istate = %x ", interruptState);
			//temp_register = stat_aux_control + 0x1A00;  
			//kprintf("ir flag = %x ", *temp_register);
			//eieio();
			temp_register	= stat_aux_control + 0x1600; //auxillary
			kprintf("aux= %x ", *temp_register);
			eieio();
			temp_register	= stat_aux_control;//dataB
			kprintf("dataB= %x ", *temp_register);
			eieio();
			temp_register	= stat_aux_control + 0x1400;	// SHIFT
			kprintf("shift= %x ", *temp_register);
			eieio();
			temp_register	= stat_aux_control + 0x1c00;	// Interrupt Enable
			kprintf("IE= %x \n", *temp_register);
			eieio();
#endif

			//put printf here so it doesn't interfere with other debugging sessions
			break;
		case 2:  
			//May disabling Cuda debugging kprintf, but preserve "4" for debugging
			kdp_flag = old_kdp_flag;
			break;
		case 3:
			cuda_glob_dbug_freeze = 1; 
    		cuda_interrupt_state = CUDA_STATE_IDLE;  //Force state change
			[ApplePMUId EnableCudaInterrupt];   
			[ApplePMUId enableAllInterrupts]; 
			[ApplePMUId ADBReset :0:0: NULL]; 
			//mouse will be in low-res mode
			break;
		case 4:
			cuda_glob_dbug_freeze = 1;
			[ApplePMUId ADBPollEnable :0:0: NULL];
			//No side effects seen for now, but gets queued when cuda freezes
			break;
		case 5:
			cuda_glob_dbug_freeze = 1;
			[ApplePMUId ADBFlush :2:0:0: NULL]; //keyboard is 2
			//No side effects seen for now, but gets queued when cuda freezes
			break;
		case 6:
			cuda_glob_dbug_freeze = 1;
			//change data direction to input
			temp_register = stat_aux_control + 0x1600;
			*temp_register &= kCudaSystemRecieve; eieio();
			break;
		case 7:
			cuda_glob_dbug_freeze = 1;
			temp_register	= stat_aux_control + 0x1400;	// SHIFT
			interruptState = *temp_register; eieio(); //dummy read
			//Simon suggests toggling the byteack too
			temp_register	= stat_aux_control;//dataB
    		*temp_register ^= kCudaByteAcknowledgeMask; eieio();
			break;
		case 8:
    		cuda_interrupt_state = CUDA_STATE_IDLE;  //Force state change
			temp_register	= stat_aux_control;//dataB
			*temp_register &= kCudaAssertTransferInProgress;
			*temp_register |= kCudaNegateByteAcknowledge ;
			eieio();
			cuda_glob_dbug_freeze = 1;
			break;
		case 9:
			cuda_glob_dbug_freeze = 1;
			cuda_current_response->a_buffer[0] = 0;
			cuda_current_response->a_buffer[1] = 0xff;
 			cuda_debug_client(0, 0, 2, 2, &cuda_current_response->a_buffer[0]);
			break;
		case 10:
			cuda_glob_dbug_freeze = 1;
			call_kdp();
			break;
		case 11:
			cuda_glob_dbug_freeze = 1; 
    		cuda_interrupt_state = CUDA_STATE_IDLE;  //Force state change
			[ApplePMUId EnableCudaInterrupt];   
			[ApplePMUId enableAllInterrupts]; 
			[ApplePMUId ADBPollEnable :0 :0: NULL];    
			break;
		case 12:
			cuda_glob_dbug_freeze = 1;
			temp_register	= stat_aux_control + 0x1400;	// SHIFT
			interruptState = *temp_register; eieio(); //dummy read
			break;
		case 20: //Negate ByteAcknowledge
			temp_register	= stat_aux_control;//dataB
    		*temp_register |= kCudaNegateByteAcknowledge; eieio();
			eieio();
			cuda_glob_dbug_freeze = 1;
			break;
		case 21:
			//Set ByteAcknowledge
			temp_register	= stat_aux_control;//dataB
    		*temp_register &= kCudaAssertByteAcknowledge; eieio();
			eieio();
			cuda_glob_dbug_freeze = 1;
			break;
		case 22:
			//Set TIP
			temp_register = stat_aux_control; //dataB
    		*temp_register &= kCudaTransferInProgressMask;
			eieio();
			cuda_glob_dbug_freeze = 1;
			break;
		case 23:
			//Negate TIP
			temp_register = stat_aux_control; //dataB
    		*temp_register |= kCudaNegateTransferInProgress;
			eieio();
			cuda_glob_dbug_freeze = 1;
			break;
		case 24:
			//data direction to input
			temp_register	= stat_aux_control + 0x1600; //auxillary
    		*temp_register &= kCudaSystemRecieve;
			eieio();
			cuda_glob_dbug_freeze = 1;
			break;
		case 25:
			//data direction to output
			temp_register	= stat_aux_control + 0x1600; //auxillary
    		*temp_register |= kCudaSystemSend;
			eieio();
			cuda_glob_dbug_freeze = 1;
			break;
		case 30: //general reset
			{
	temp_register = stat_aux_control + 0x0400;	//dataDirectionB
    *temp_register |= (kCudaByteAcknowledgeMask | kCudaTransferInProgressMask);
    *temp_register &= ~kCudaTransferRequestMask;

	temp_register	= stat_aux_control + 0x1600; //auxillary
    *temp_register 	= ( *temp_register 	| kCudaTransferMode) & kCudaSystemRecieve;

    // Clear any posible cuda interupt.
	temp_register = stat_aux_control + 0x1400;	//shift
    if ( *temp_register );
	eieio();

    // Initialize the internal data.

    cuda_interrupt_state    = CUDA_STATE_IDLE;
    cuda_transaction_state  = CUDA_TS_NO_REQUEST;
    cuda_is_header_transfer = FALSE;
    cuda_is_packet_type = FALSE;
    cuda_transfer_count = 0;    
    cuda_current_response   = NULL;     
    
    // Terminate transaction and set idle state
    // cuda_neg_tip_and_byteack();
	temp_register = stat_aux_control; //dataB
	*temp_register |= kCudaNegateByteAcknowledge | kCudaNegateTransferInProgress;
	eieio();
    
    // we want to delay 4 mS for ADB reset to complete
    
    delay(4000);
    
    // Clear pending interrupt if any...
    //(void)cuda_read_data();
	temp_register = stat_aux_control + 0x1400; //Shift
    if ( *temp_register ); eieio();
    
    // Issue a Sync Transaction, ByteAck asserted while TIP is negated.
    // cuda_assert_byte_ack();
	temp_register = stat_aux_control; //dataB
	*temp_register &= kCudaAssertByteAcknowledge; eieio();


    // Wait for the Sync acknowledgement, cuda to assert TREQ
    // cuda_wait_for_transfer_request_assert();
	while ( (*temp_register & kCudaTransferRequestMask) != 0 ) 
	{
		eieio();
	}
	eieio();

    // Wait for the Sync acknowledgement interrupt.
    // cuda_wait_for_interrupt();
	temp_register = stat_aux_control + 0x1A00; //VIA interrupt flag
	while ( (*temp_register & kCudaInterruptMask) == 0 ) 
	{
		eieio();
	}
	eieio();


    // Clear pending interrupt
    //(void)cuda_read_data();
	temp_register = stat_aux_control + 0x1400;	//shift
    if ( *temp_register );
	eieio();

    // Terminate the sync cycle by Negating ByteAck
    // cuda_neg_byte_ack();
	temp_register = stat_aux_control; //dataB
	*temp_register |= kCudaNegateByteAcknowledge; eieio();

    // Wait for the Sync termination acknowledgement, cuda negates TREQ.
    // cuda_wait_for_transfer_request_neg();
	while ( (*temp_register & kCudaTransferRequestMask) == 0 ) 
	{
		eieio();
	}
	eieio();


    // Wait for the Sync termination acknowledgement interrupt.
    // cuda_wait_for_interrupt();
	temp_register = stat_aux_control + 0x1A00; //VIA interrupt flag
	while ( (*temp_register & kCudaInterruptMask) == 0 ) 
	{
		eieio();
	}
	eieio();


    // Terminate transaction and set idle state, TIP negate and ByteAck negate.
    // cuda_neg_transfer_in_progress();
	temp_register = stat_aux_control; //dataB
	*temp_register |= kCudaNegateTransferInProgress; eieio();

    // Clear pending interrupt, if there is one...
    //(void)cuda_read_data();
	temp_register = stat_aux_control + 0x1400;	//shift
    if ( *temp_register );
	eieio();




debugging = FALSE;
adb_reading = FALSE;

//CUDA_REPLACE
// A.W. This call seems to have no counterpart in Cuda
// 	Pending interrupts are cleared with cuda_read_data()
	temp_register = stat_aux_control + 0x1A00; //VIA interrupt flag
	*temp_register = kCudaInterruptDisable;eieio(); // turn off any pending interrupt

	temp_register = stat_aux_control + 0x1400;	//shift
    if ( *temp_register );
	eieio();

	[ApplePMUId EnableCudaInterrupt];   

	[ApplePMUId enableAllInterrupts];

			}
			break;

		case 40:
			//try AIX stuff
			temp_register = stat_aux_control; //dataB
    		*temp_register |= kCudaNegateTransferInProgress | kCudaNegateByteAcknowledge;
			eieio();
			[ApplePMUId EnableCudaInterrupt];   
			//[ApplePMUId enableAllInterrupts]; 
			[ApplePMUId ADBPollEnable :0 :0: NULL];    
			cuda_glob_dbug_freeze = 1;
			break;

		default:
	}

	// Sept 11, 1998 Add new code to fix ADB hang problem
	if ( cuda_freeze_counter == cuda_freeze_prevcount)
	//Even if user never touches keyboard, this test is FALSE first time
	{
		//if here then that means either user has not moved ADB device for
		// a long time or the system is hung
		//kprintf("HH: No ADB movement so far\n");
		//Now check TREQ (if in middle of transaction)
		temp_register = stat_aux_control; //dataB
		if ((*temp_register & kCudaTransferRequestMask) == 0 ) 
		{
			//kprintf("HH: TREQ is 0 (asserted), incrementing flag now\n");
			cuda_TREQ_history++;
		}
		else
		{
			cuda_TREQ_history = 0;
		}
		if ( cuda_TREQ_history == 2)  // 2 cycles already
		{
			//Executing this code at any time has proven to be safe and does
			// not affect the operation of the high-resolution pointing devices
			//kprintf("HH: In recovery code now \n");
			printf("\nCuda: recover from transaction hang\n");
			temp_register = stat_aux_control; //dataB
    		*temp_register |= kCudaNegateTransferInProgress | kCudaNegateByteAcknowledge;
			eieio();
			[ApplePMUId EnableCudaInterrupt];   
			[ApplePMUId ADBPollEnable :0 :0: NULL];    
			cuda_TREQ_history = 0;
		}
	}
	else
	{
		cuda_TREQ_history = 0;
	}

	cuda_freeze_prevcount = cuda_freeze_counter ;  //Get ready for next 4-sec timer
#endif  //DEBUG


}


// Cuda functions needed to help -interruptOccured:




//
//  TransmitCudaData
//  Executes at hardware interrupt level.
//

-(void) cuda_transmit_data
{
	volatile unsigned char valcuda;  //dummy, unused
    // Clear the pending interrupt by reading the shift register.


    if ( cuda_is_header_transfer ) {
        // There are more header bytes, write one out.
		// Initially cuda_transfer_count is 1, so for most Cuda commands
		// that means the ADB command itself, which contains Talk, Listen, etc.
        // cuda_write_data(cuda_request->a_cmd.a_header[cuda_transfer_count++]);
		*VIA1_shift = cuda_request->a_cmd.a_header[cuda_transfer_count++];

        // Toggle the handshake line.
		// a_hcount is number of bytes in command packet
        if ( cuda_transfer_count >= cuda_request->a_cmd.a_hcount ) {
            cuda_is_header_transfer = FALSE;
            cuda_transfer_count = 0;
        }

        // cuda_toggle_byte_ack();
    	*VIA2_dataB ^= kCudaByteAcknowledgeMask; eieio();


    } else if ( cuda_transfer_count < cuda_request->a_cmd.a_bcount ) {
//WARNING... find out why a_bcount above is examined during Cuda Transmit?


    // There are more command bytes, write one out and update the pointer
        // cuda_write_data(cuda_request->a_cmd.a_buffer[cuda_transfer_count++]);
		*VIA1_shift = cuda_request->a_cmd.a_buffer[cuda_transfer_count++];

        // Toggle the handshake line.
        // cuda_toggle_byte_ack();
    	*VIA2_dataB ^= kCudaByteAcknowledgeMask; eieio();
    } else {
        //(void)cuda_read_data();
		valcuda = *VIA1_shift; eieio();
        // There is no more command bytes, terminate the send transaction.
        // Cuda should send a expected attention interrupt soon.

        // cuda_neg_tip_and_byteack();
		*VIA2_dataB |= kCudaNegateByteAcknowledge | kCudaNegateTransferInProgress;
		eieio();

        // The next interrupt should be a expected attention interrupt.

        cuda_interrupt_state = CUDA_STATE_ATTN_EXPECTED;

/**** moved to cuda_idle() portion
		// call back the ADB layer if entire transmission packet is complete.
 		if ( clientRequest->pmCallback != NULL ) {
            clientRequest->pmCallback(clientRequest->pmId, clientRequest->pmRefNum, 0, NULL);

		}
*****/

    }
}

//
//  cuda_expected_attention
//  Executes at hardware interrupt level.
//

- (void)cuda_expected_attention
{
	volatile unsigned char valcuda;  //dummy, unused
    

	// Clear the pending interrupt by reading the shift register.
	// This first byte is the "attention" byte and contains no
	//    useful data, so ignore it.  Next byte should be response
	//    packet TYPE.
    //(void)cuda_read_data();
	valcuda = *VIA1_shift; eieio();

    // Allow the VIA to settle directions.. else the possibility of
    // data corruption.
    tick_delay(cuda_state_transition_delay_ticks);

    if ( cuda_transaction_state ==  CUDA_TS_SYNC_RESPONSE) {
        cuda_current_response = (adb_packet_t*)&cuda_request->a_reply;
    } else {
        cuda_unsolicited.a_hcount = 0;
        cuda_unsolicited.a_bcount = 0;
        cuda_unsolicited.a_bsize = sizeof(cuda_unsolicited.a_buffer);
        cuda_current_response = &cuda_unsolicited;
    }

    cuda_is_header_transfer = TRUE;
    cuda_is_packet_type = TRUE;
    cuda_transfer_count = 0;

    // Set the shift register direction to input.
    // cuda_set_data_direction_to_input();
	*VIA1_auxillaryControl &= kCudaSystemRecieve; eieio();

    // Start the response packet transaction.
    // cuda_assert_transfer_in_progress();
	*VIA2_dataB	&= kCudaAssertTransferInProgress; eieio();

    // The next interrupt should be a receive data interrupt.
    cuda_interrupt_state = CUDA_STATE_RECEIVE_EXPECTED;
}

//
//  cuda_unexpected_attention
//  Executes at hardware interrupt level.
//

-(void) cuda_unexpected_attention
{
	volatile unsigned char valcuda;  //dummy, unused

    // Clear the pending interrupt by reading the shift register.
	// This first byte is the "attention" byte and contains no
	//    useful data, so ignore it.  Next byte should be response
	//    packet TYPE.
    //(void)cuda_read_data();
	valcuda = *VIA1_shift; eieio();

    // Get ready for a unsolicited response.

    cuda_unsolicited.a_hcount = 0;
    cuda_unsolicited.a_bcount = 0;
    cuda_unsolicited.a_bsize = sizeof(cuda_unsolicited.a_buffer);
	//Try to fix jumping bug
	bzero( cuda_unsolicited.a_buffer, cuda_unsolicited.a_bsize);


    cuda_current_response = &cuda_unsolicited;

    cuda_is_header_transfer = TRUE;
    cuda_is_packet_type = TRUE;
    cuda_transfer_count = 0;


    // Start the response packet transaction, Transaction In Progress
    // cuda_assert_transfer_in_progress();
	*VIA2_dataB	&= kCudaAssertTransferInProgress; eieio();

    // The next interrupt should be a receive data interrupt and the next
    // response should be an async response.

    cuda_interrupt_state = CUDA_STATE_RECEIVE_EXPECTED;

    cuda_transaction_state = CUDA_TS_ASYNC_RESPONSE;
}

//
//  cuda_receive_data
//  Executes at hardware interrupt level.
//

-(void) cuda_receive_data
{

	// cuda_is_packet_type is set to TRUE when cuda_(+un)expected_attention() is called
	// The first part will be called just to read the packet type 
    if ( cuda_is_packet_type ) {
        unsigned char packetType;

        //packetType = cuda_read_data();
		packetType = *VIA1_shift; eieio();
        cuda_current_response->a_header[cuda_transfer_count++] = packetType;

        if ( packetType == ADB_PACKET_ERROR) {
            cuda_current_response->a_hcount = 4;
        } else {
            cuda_current_response->a_hcount = 3;
        }

        cuda_is_packet_type = FALSE;

        // cuda_toggle_byte_ack();
    	*VIA2_dataB ^= kCudaByteAcknowledgeMask; eieio();

    } else if ( cuda_is_header_transfer ) {  //set TRUE by cuda_(+un)expected_attention

		//This should be the Response Packet ADB Status byte (2nd byte)
        cuda_current_response->a_header[cuda_transfer_count++] =
				*VIA1_shift; eieio();
                // cuda_read_data();

        if (cuda_transfer_count >= cuda_current_response->a_hcount) {
            cuda_is_header_transfer = FALSE;
            cuda_transfer_count = 0;
        }

        // cuda_toggle_byte_ack();
    	*VIA2_dataB ^= kCudaByteAcknowledgeMask; eieio();

    } else if ( cuda_transfer_count < cuda_current_response->a_bsize ) {
        // Still room for more bytes. Get the byte and tell Cuda to continue.
        // Toggle the handshake line, ByteAck, to acknowledge receive.

		// a_buffer[] is the actual buffer going back to ADB.M Callback
        cuda_current_response->a_buffer[cuda_transfer_count] =
				*VIA1_shift; eieio();
                // cuda_read_data();

		if (bImmediate_buff_needed)  //just for getRealTimeClock 
		{
			if ( return_buff_pointer)
			{
				*(return_buff_pointer + cuda_transfer_count) =
        			cuda_current_response->a_buffer[cuda_transfer_count];
 
//kprintf("%x ", cuda_current_response->a_buffer[cuda_transfer_count]);
			}
		}

		cuda_transfer_count++;

        // cuda_toggle_byte_ack();
    	*VIA2_dataB ^= kCudaByteAcknowledgeMask; eieio();
    } else {
        // Cuda is still sending data but the buffer is full.
        // Normally should not get here.  The only exceptions are open ended
        // request such as  PRAM read...  In any event time to exit.

		// a_bcount is the actual count going back to ADB.M Callback
        cuda_current_response->a_bcount = cuda_transfer_count;

        // cuda_read_data();
		if (*VIA1_shift); eieio();

		//So far no hits here on kernel bootup 3/20/98

        [self cuda_process_response];  //Rewritten by D.S.
        // cuda_neg_tip_and_byteack();
	    *VIA2_dataB |= kCudaNegateByteAcknowledge | kCudaNegateTransferInProgress;
	    eieio();
    }
}

//
//  cuda_receive_last_byte
//  Executes at hardware interrupt level.
//

-(void) cuda_receive_last_byte
{
	volatile unsigned char valcuda;  //dummy, unused


    if ( cuda_is_header_transfer ) {
        cuda_current_response->a_header[cuda_transfer_count++]  =
			*VIA1_shift; eieio();
            // cuda_read_data();

        cuda_transfer_count = 0;
    } else if ( cuda_transfer_count < cuda_current_response->a_bsize ) {

        cuda_current_response->a_buffer[cuda_transfer_count] =
			*VIA1_shift; eieio();

		//Added 1/13/98 A.W. 
		if (bImmediate_buff_needed)  //just for getRealTimeClock 
		{
			if ( return_buff_pointer)
			{
				*(return_buff_pointer + cuda_transfer_count) =
        			cuda_current_response->a_buffer[cuda_transfer_count];
			}
		}
		cuda_transfer_count++;

            // cuda_read_data();
    } else {
        /* Overrun -- ignore data */
        //(void) cuda_read_data();
		valcuda = *VIA1_shift; eieio();
    }

    cuda_current_response->a_bcount = cuda_transfer_count;

//SD: acknowledge before response so polled mode can work from inside the adb handler

    // cuda_neg_tip_and_byteack();
    *VIA2_dataB |= kCudaNegateByteAcknowledge | kCudaNegateTransferInProgress;
    eieio();
if (p) kprintf("cuda.m: process response in LAST BYTE, count is %d\n", cuda_transfer_count);
    [self cuda_process_response];  //Rewritten by D.S.
}

/* 3/19/98 A.W. added to fix collision problems */
- (void) cuda_queue:(CudaRequest *)cuda_req
{
	CudaMachMessage * toQueue;
	IOReturn result;

	que_cuda_count++;  //for debugging only
	//For collisions, queue up the incoming request from Rhapsody system
	// and handle Cuda's urgent request first.
    toQueue = (CudaMachMessage*)kalloc(sizeof(CudaMachMessage));
    toQueue->msgHeader.msg_size = sizeof(CudaMachMessage);
    toQueue->msgHeader.msg_local_port = [self interruptPort];
	bcopy (cuda_req, &toQueue->msgBody, sizeof(CudaRequest));	// copy it
	toQueue->msgBody.prev = queueTail;
	toQueue->msgBody.next = NULL;
	if ( queueTail != NULL ) {
	   queueTail->msgBody.next = toQueue;
	}
	else 
	{
		queueHead = toQueue;
	}
	queueTail = toQueue;
}

//
//  cuda_collision
//  Executes at hardware interrupt level.
//

-(void) cuda_collision
{
	volatile unsigned char valcuda;  //dummy, unused


    // Clear the pending interrupt by reading the shift register.
    //(void)cuda_read_data();
	valcuda = *VIA1_shift; eieio();

    // Negate TIP to abort the send.  Cuda should send a second attention
    // interrupt to acknowledge the abort cycle.
    // cuda_neg_transfer_in_progress();
    *VIA2_dataB |= kCudaNegateTransferInProgress; eieio();

    // The next interrupt should be an expected attention and the next
    // response packet should be an async response.

    cuda_interrupt_state = CUDA_STATE_ATTN_EXPECTED;
    cuda_transaction_state = CUDA_TS_ASYNC_RESPONSE;

    /* queue the request */
    cuda_collided = cuda_request; //WARNING... unused now, don't use this variable
#ifdef OMIT
    cuda_request = NULL;
#endif
    cuda_is_header_transfer = FALSE;
    cuda_transfer_count = 0;

	[self cuda_queue: cuda_request];
}

//
//  cuda_idle
//  Executes at hardware interrupt level.
//  A.W. WARNING... what if this state is reached without a client callback available?
//		what happens with unsolicited cuda events?  Should they go to the ADBRegisterAutopoll
//		function?
//

-(void) cuda_idle
{
	volatile unsigned char valcuda;  //dummy, unused

    // Clear the pending interrupt by reading the shift register.
    //(void)cuda_read_data();
	valcuda = *VIA1_shift; eieio();

    // Set to the idle state.
    cuda_interrupt_state = CUDA_STATE_IDLE;

    // See if there are any pending requests.  There may be a pending request
    // if a collision has occured.  If so do the resend.

    //if (adb_polling)
    //return;     /* Prevent recursion */

	//if collision then that has already been queued above, so all we do
	// now is check for it.
	    
	[self CheckRequestQueue];

}



-(void) cuda_error
{
	unsigned char valcuda;

    kprintf("\nCUDA:{Error %d}\n", cuda_transaction_state);

    switch (cuda_transaction_state) {
    case    CUDA_STATE_IDLE:
        // cuda_neg_tip_and_byteack();
    	*VIA2_dataB |= kCudaNegateByteAcknowledge | kCudaNegateTransferInProgress;
    	eieio();
        break;

    case    CUDA_STATE_TRANSMIT_EXPECTED:
        if (cuda_is_header_transfer && cuda_transfer_count <= 1) {
            tick_delay(cuda_state_transition_delay_ticks);
            // cuda_neg_transfer_in_progress();
    		*VIA2_dataB |= kCudaNegateTransferInProgress; eieio();
            // cuda_set_data_direction_to_input();
			*VIA1_auxillaryControl &= kCudaSystemRecieve; eieio();
            //panic ("CUDA - TODO FORCE COMMAND BACK UP!\n");
        } else {
            cuda_transaction_state = CUDA_STATE_ATTN_EXPECTED;
            // cuda_neg_tip_and_byteack();
    	    *VIA2_dataB |= kCudaNegateByteAcknowledge | kCudaNegateTransferInProgress;
    	    eieio();
        }
        break;

    case    CUDA_STATE_ATTN_EXPECTED:
        // cuda_assert_transfer_in_progress();
		*VIA2_dataB	&= kCudaAssertTransferInProgress; eieio();

        tick_delay(cuda_state_transition_delay_ticks);
        // cuda_set_data_direction_to_input();
		*VIA1_auxillaryControl &= kCudaSystemRecieve; eieio();
        // cuda_neg_transfer_in_progress();
    	*VIA2_dataB |= kCudaNegateTransferInProgress; eieio();
        //panic("CUDA - TODO CHECK FOR TRANSACTION TYPE AND ERROR");
        break;

    case    CUDA_STATE_RECEIVE_EXPECTED:
        // cuda_neg_tip_and_byteack();
	    *VIA2_dataB |= kCudaNegateByteAcknowledge | kCudaNegateTransferInProgress;
	    eieio();
        //panic("Cuda - todo check for transaction type and error");
        break;

    default:
        // cuda_set_data_direction_to_input();
		*VIA1_auxillaryControl &= kCudaSystemRecieve; eieio();
		valcuda = *VIA1_shift; eieio();  //Ray Montagne .a indicates this may be useful
        // cuda_neg_tip_and_byteack();
	    *VIA2_dataB |= kCudaNegateByteAcknowledge | kCudaNegateTransferInProgress;
	    eieio();
        break;
    }
}

-(void) poll_device
{
    CudaRequest   *next;
    int interruptflag = *VIA1_interruptFlag & kCudaInterruptMask;
    eieio();

    if(interruptflag) {
    	[self interruptOccurred];
    	if( cuda_interrupt_state == CUDA_STATE_IDLE) {
			[self CheckRequestQueue];
    	}
    }
}


@end
