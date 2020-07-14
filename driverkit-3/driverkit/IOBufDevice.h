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
/* 	Copyright (c) 1991 NeXT Computer, Inc.  All rights reserved. 
 *
 * IOBufDevice.h - Buffered Device abstract superclass. 
 *
 * HISTORY
 * 13-Jan-93	John Seamons (jks) at NeXT
 *	Ported to i386.
 *
 * 23-Jun-92	Mike DeMoney (mike@next.com)
 *      Major hacks... 
 *
 * 16-Jul-91    Doug Mitchell at NeXT
 *      Created.
 */
 
#import <driverkit/IODirectDevice.h>
#import	<mach/mach_types.h>

#define	MAX_OWNER_NAME_LEN	80

typedef	char *		DeviceStateName;
typedef	unsigned	DeviceStateValue;

typedef	void *		Tag;
typedef	char		OwnerName[MAX_OWNER_NAME_LEN+1];

/*
 * BufferParameters -- information that characterizes data buffer
 * requirements of the device.  User of the network device is expected
 * to create buffers meeting these requirements.
 *
 * minAlignment		-- both the start address and end address
 *			   of the buffer must be aligned to this value.
 *			   MUST be a power of 2.  (NOTE: the buffer
 *			   end address alignment restriction does
 *			   not require the end of data itself to be
 *			   to be aligned, data may end at any point
 *			   within the buffer.)
 *
 * minBufferLength	-- the minimum buffer length that the driver
 *			   will accept.  (NOTE: this only limits the
 *			   actual buffer size, the data within the
 *			   buffer and the corresponding data length
 *			   specified in the transmitBuffer requests
 *			   may be of shorter length.)
 *			   
 *
 * privatePrefix	-- initial prefix of buffer that is for the
 *			   private use of the device driver.  The user
 *			   of the device driver should begin placing
 *			   data at this offset in the buffer.
 *
 * maxTransferUnit	-- the largest amount of data that the device
 *			   driver can accept as a "record" for transmission
 *			   or that the device driver will ever return
 *			   as a received "record".
 *
 * transmitFragmentOk	-- if TRUE, the device driver allows a single
 *			   transmit "record" to be fragmented across
 *			   multiple transmitData requests (the final
 *			   transmitData request must indicate "eor").
 *			   if FALSE, the entire transmit record must
 *			   be presented in a single transmitData request.
 *
 * receiveFragmentOk	-- if TRUE, the device driver can receive a
 *			   single received record into multiple
 *			   posted receive buffers of smaller length,
 *			   the final buffer will be marked "eor" by
 *			   the device driver.
 *			   if FALSE, the device driver will place
 *			   the received record into a single posted
 *			   receive buffer, the buffer will be marked
 *			   "eor".  Normally in this case, all receive
 *			   buffers should be large enough to receive a
 *			   "max Transfer Unit" record to avoid lost data.
 *
 * Buffer layout
 *
 *	bufferBaseAddress ->	+-------------------------------+
 *	(must be aligned	| driver "private prefix data"	|
 *	as per minAlignment)	| 				|
 *				| (device driver will typically	|
 *				| size this area to a multiple	|
 *	bufferBaseAddress	| of minAlignment)		|
 *	+ privatePrefix ->	+-------------------------------+
 *				| data area			|
 *				| transmit data or receive data	|
 *				| (devices not supporting	|
 *				| fragments will typically	|
 *				| require this to the data	|
 *				| area to be at least		|
 *				| maxTransferUnit bytes long)	|
 *	bufferEndAddress ->	+-------------------------------+
 *	(must be aligned
 *	as per minAlignment,
 *	must be greater than
 *	or equal to
 *	bufferBaseAddress
 *	+ minBufferLength)
 */

/*
 * Synchronous and asynchronous methods
 *
 * Methods in this interface are classified as either synchronous or
 * asynchronous.  Synchronous methods complete there action before
 * returning; asynchronous methods simply queue the request for action,
 * this request may get processed after the method returns.  Actions
 * requested by methods are always accomplished in the same order as the
 * method invocations.
 *
 * Invoking any synchronous method forces all previously queued asynchronous
 * methods to complete before processing the synchronous method.  This is
 * useful to guarantee that when it is necessary to insure that an async
 * method has completed.
 */

typedef	struct	{
	unsigned	minAlignment;
	unsigned	minBufferLength;
	unsigned	privatePrefix;
	unsigned	maxTransferUnit;
	BOOL		transmitFragmentOk;
	BOOL		receiveFragmentOk;
} BufferParameters;

/*
 * IOBufDevice supports devices that have multiple "subunits" (e.g.
 * DUART's).  Each subunit may be individually acquired.
 */
#define	MAX_IOBUFDEVICE_UNITS	4	// FIXME: Should be dynamic...

typedef struct {
	id		callbackId;
	Tag		deviceTag;
	OwnerName	ownerName;
} UnitInfo;

@interface IOBufDevice:IODirectDevice
{
@protected
	UnitInfo unitInfo[MAX_IOBUFDEVICE_UNITS];
}

/*
 * Acquiring and releasing underlying device
 *
 * Subunits of an IOBufDevice are exclusive access.
 *
 * After successful acquire:
 *	1. device will be disabled for receive, transmit and state changes.
 *	2. no receive or transmit buffers will be queued
 *	3. device will be initialized to a device-specific state
 *
 * When acquiring the device, the caller passes two pieces of information:
 *	1. a callbackId, this is an id representing a caller provided
 *	   object.  This object will be messaged as per the
 *	   NetDeviceCallback protocol (defined below).
 *	2. a deviceTag, this is opaque to the device driver, the device
 *	   simply passes the deviceTag on all future callbacks.
 *
 * If the device is not currently acquired by another device, the
 * acquisition will succeed.  If the device is already acquired, the
 * current owner will receive a callback indicating that the device
 * is desired.  The current owner will then receive a callback indicating
 * that ownership is being requested.  This callback will pass a character
 * string indicating the "name" of the requester.  If the previous owner
 * releases the device before returning from this callback, the new
 * requestor will be granted the device.  Should the prevous owner not
 * release the device, the new requestor will receive an error.
 * NOTE: This is currently unimplemented.
 *
 * Buffers queued with the device at the time of a release request
 * will be abandon by the device, the caller should issue flush
 * requests before release if it requires the buffers to be returned.
 */
- (IOReturn)acquireUnit				: (unsigned) unitNumber
				for		: (OwnerName) ownerName
				callbackId	: (id) callbackId
				deviceTag	: (Tag) deviceTag;
- (IOReturn)releaseUnit				: (unsigned) unitNumber;
- (IOReturn)ownerForUnit			: (unsigned) unitNumber
				isNamed		: (OwnerName) ownerName;

/*
 * Standard IODevice methods
 */
- (const char *)stringFromReturn		: (IOReturn) rtn;
- (int)errnoFromReturn 				: (IOReturn) rtn;
@end

/*
 * Methods to be implemented by subclass.  These methods specify
 * the common functionality of all IOBufDevice drivers.
 */
@protocol IOBufDeviceSubclass

/*
 * Device initialization
 *
 * Device is initialized to device-specific state.
 *
 * initializeUnit may be called to re-initialize the device as necessary.
 * NOTE: the device is forced to a device-specific state.  Any buffers
 * queued are flushed.  Receive, transmit, and state changes are disabled.
 * No state is watched.  The acquisition state of the device is not altered.
 * After initializeUnit completes, the device should be totally quiescent:
 * interrupts should be disabled and there should be no possibility of
 * callbacks being generated.  InitializeUnit is invoked by the
 * IOBufDevice superclass when processing an acquireUnit command.
 *
 * shutdownUnit puts the device into totally quiescent state.  It is
 * invoked by the IOBufDevice superclass when processing a releaseUnit 
 * command.  It is a programming error to use any method other than
 * releaseUnit or initializeUnit immediately after having done a
 * shutdownUnit.  releaseUnit performs a shutdownUnit.
 *
 * These methods are synchronous.
 *
 * FIXME: Get rid of shutdownUnit if at all possible, hopefully,
 * initializeUnit will make the device quiescent.
 */
- (IOReturn)initializeUnit			: (unsigned) unitNumber;

- (IOReturn)shutdownUnit			: (unsigned) unitNumber;

/*
 * Device enable and disable
 *
 * Devices may be enabled or disabled for:
 *	1. transmit
 *	2. receive
 *	3. state changes (device-specific)
 *
 * Disabling a device prohibits the device from performing and/or reacting
 * to function disabled.  Input and state changes are ignored while disabled.
 * No callbacks will be generated after the disable action is complete
 * (callbacks may occur during the processing of the disable).  Since
 * setting of an enable is asynchronous, it may at times be necessary
 * to follow the set of the enable by a synchronous method to guarantee
 * that all callback have completed.
 *
 * Unless transmit (receive) buffers are flushed after disabling
 * the transmitter (the receiver) the behavior of the device on
 * re-enable is device specific.
 *
 * Enabling the receiver before having posted receive buffers may risk
 * overruns for certain devices.  Likewise, enabling the transmitter before
 * queuing transmit buffers may risk underrun.
 *
 * When state changes are enabled, immediate callbacks will be generated
 * indicating the current state.  These callbacks pass a flag indicating
 * that this is initial state, rather than a true state transition.
 * Since setting the state change enable is asynchronous, it may be
 * necessary to invoke a synchronous method to guarantee that
 * all initial callbacks have completed.
 *
 * State change callbacks may be enabled and disabled with the method
 * setStateEnable:; individual states may be added or deleted from
 * the list of state variables tracked with setWatchEnable:enable:forUnit:.
 *
 * These methods are ASYNCHRONOUS.
 */
- (IOReturn)setRxEnable				: (BOOL) enableFlag
				forUnit		: (unsigned) unitNumber;

- (IOReturn)setTxEnable				: (BOOL) enableFlag
				forUnit		: (unsigned) unitNumber;

- (IOReturn)setStateEnable			: (BOOL) enableFlag
				forUnit		: (unsigned) unitNumber;

- (IOReturn)setWatchState			: (DeviceStateName) stateName
				enable		: (BOOL) newEnable
				forUnit		: (unsigned) unitNumber;

/*
 * These methods are synchronous.
 */
- (BOOL)rxEnableForUnit				: (unsigned) unitNumber;
- (BOOL)txEnableForUnit				: (unsigned) unitNumber;
- (BOOL)stateEnableForUnit			: (unsigned) unitNumber;
- (IOReturn)watchState				: (DeviceStateName) stateName
				enablePtr	: (BOOL *) enableP
				forUnit		: (unsigned) unitNumber;

/*
 * Transmitting and receiving
 *
 * Transmit and receive buffers are "given" to the underlying device by
 * these methods.  The device indicates completion or failure of the
 * operation by a later appropriate callback.  The callback indicates a
 * particular buffer by returning the bufferTag.
 *
 * Transmit buffers also may specify a buffer as the "end of record";
 * interpretation of this flag is device-specific.
 *
 * The format of data within a transmit buffer is device specific.
 * Certain devices may require that device specific headers be present
 * in the data.  E.g. an Ethernet driver may require that the first bytes
 * of data be the Ethernet source addr, dest addr, and type.
 *
 * Receive buffer addresses returned on callback may not be identical
 * to the buffer address passed in with postReceiveBuffer (but the
 * returned buffer will lie entirely within the original buffers
 * range.  If the original buffer address is required by the caller,
 * it should be determined via the bufferTag.
 *
 * NOTE: These methods may not fail.  The driver must always be prepared
 * to queue transmit or receive buffers.
 *
 * In all cases, dataLength and maxDataLength refer ONLY to the data
 * portion and do not include the privatePrefix.
 *
 * The vmTask parameter is either a task port (user space) or
 * a vm_map_t (kernel space) and indicates the address space
 * containing the buffer.
 *
 * These methods are ASYNCHRONOUS.
 */
- (IOReturn)transmitBuffer			: (void *) buffer
				dataLength	: (size_t) dataLength
				eor		: (BOOL) eor
				bufferTag	: (Tag) bufferTag
				vmTask		: (vm_task_t) vmTask
				forUnit		: (unsigned) unitNumber;

- (IOReturn)postReceiveBuffer			: (void *) buffer
				maxDataLength	: (size_t) maxDataLength
				bufferTag	: (Tag) bufferTag
				vmTask		: (vm_task_t) vmTask
				forUnit		: (unsigned) unitNumber;

/*
 * Buffer management
 *
 * Buffers given to the underlying device may be forceably retrieved by
 * the following flush methods -- pending actions are aborted (flushed
 * transmit buffers may not have been transmitted, receive buffers may
 * be empty).  Invoking a flush method will cause the underlying device
 * to immediately return the buffers via callbacks.
 *
 * User of the underlying device should invoke the method bufferParameters
 * and create buffers for transmission and reception that meet the
 * requirements returned.
 *
 * These methods are asynchronous.
 */
- (IOReturn)flushTransmitBuffersForUnit		: (unsigned) unitNumber;
- (IOReturn)flushReceiveBuffersForUnit		: (unsigned) unitNumber;

/*
 * These methods are synchronous
 */
- (IOReturn)waitTransmitBuffersDoneForUnit	: (unsigned) unitNumber;
- (BufferParameters)bufferParameters;

/*
 * Device parameter management
 *
 * Inherits {get,set}Parameter{Int,Char} from IODevice class
 *
 * The set methods are asynchronous, the get methods synchronous.
 */

/*
 * "Commanded" control functions
 *
 * Commands are entirely device-specific.
 */
@end

@protocol IOBufDeviceCallback
/*
 * The methods described here are the callback methods that IOBufDevice
 * subclass driver will invoke in the object registered as the callback
 * object.
 *
 * These methods must be implemented by any client of a IOBufDevice.
 *
 * NOTE: It is illegal to invoke synchronous methods from a callback
 * method.  Doing so will result in an error return.
 *
 * Since the client of this class must implement these methods, it
 * doesn't make sense to characterize these methods as sync or async.
 */

/*
 * Buffer management.
 *
 * Transmit buffers are returned because the data was transmitted
 * (with or without error) or the transmitter was flushed.  NOTE: Transmit
 * buffers are guaranteed to be returned in the same order in which
 * they were posted to the device.
 *
 * Similarly, receive buffers are returned either because data was
 * received or the receiver was flushed.  The dataPtr returned may
 * point to any location within the originally posted buffer
 * (including within the privatePrefix!).
 *
 * Returned receive buffers may be marked by the device as "end of record",
 * the meaning of this flag is device-specific.
 *
 * Format of the returned receive data is device-specific and may include
 * media headers, etc.
 */
- (void)transmitDoneForUnit			: (unsigned) unitNumber
				bufferTag	: (Tag) bufferTag
				deviceTag	: (Tag) deviceTag
				bytesSent	: (size_t) bytesSent
				error		: (IOReturn) error;

- (void)receiveDoneForUnit			: (unsigned) unitNumber
				bufferTag	: (Tag) bufferTag
				deviceTag	: (Tag) deviceTag
				dataPtr		: (void *) dataPtr
				bytesReceived	: (size_t) bytesReceived
				eor		: (BOOL) eor
				error		: (IOReturn) error;

- (void)transmitError				: (IOReturn) error
				forUnit		: (unsigned) unitNumber
				deviceTag	: (Tag) deviceTag;

- (void)receiveError				: (IOReturn) error
				forUnit		: (unsigned) unitNumber
				deviceTag	: (Tag) deviceTag;

/*
 * Watch'ed state change notification
 *
 * isInitial denotes that this callback indicates initial state as
 * opposed to a state change.  "initial" callbacks are made
 * whenever stateEnable transitions from FALSE to TRUE.
 */
- (void)watchChangeForUnit			: (unsigned) unitNumber
				watchState	: (DeviceStateName) stateName
				newValue	: (DeviceStateValue) stateValue
				initialChange	: (BOOL) isInitial
				deviceTag	: (Tag) deviceTag;

/*
 * FIXME:  Define callback from ownership request.
 */
@end

/*
 * IOReturn values specific to IOBufDevice
 */
#define	IO_R_FLUSHED	(-800)		/* buffer was flushed */
#define	IO_R_NOT_OWNER	(-801)		/* not owner */

