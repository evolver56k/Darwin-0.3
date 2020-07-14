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
/* 	Copyright (c) 1993 NeXT Computer, Inc.  All rights reserved. 
 *
 * TokenRing.m - TokenRing device-independent abstract superclass
 *
 *	The TokenRing superclass contains the device-independent functions
 *	that are common to most token-ring drivers.  The hardware-specific
 *	driver should be a subclass of TokenRing.
 *
 * HISTORY
 * 25-Jan-93 Joel Greenblatt at NeXT 
 *	created
 *      
 */

 
#define MACH_USER_API	1
 

#import <mach/mach_types.h>
#import <mach/message.h>
#import <machkit/NXLock.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/kernelDriver.h>
#import <netinet/in.h>
#import <kernserv/prototypes.h>
#import <net/tokensr.h>
#import <sys/errno.h>

#import <driverkit/IOTokenRing.h>

static void TokenRingTimeout(IOTokenRing *device);

static char	TRDeviceName[] = "tr";
static char	TRDeviceType[] = "TokenRing";
static int	TRDeviceCount = 0;

extern int ipforwarding; // flag in kernel to control ip lan-to-lan routing

/*
 * Attach virtual drivers
 */
extern void vtrip_config(int unit, int flags, int mtu, int tokpri);

#define IP_SRCROUTING		0x1	/* enable IP source routing */


/*
 * Configuration strings in Instance.table
 */
static const char ITRingSpeed[] = 		"Ring Speed";
static const char ITNodeAddress[] = 		"Node Address";
static const char ITGroupAddress[] = 		"Group Address";
static const char ITFunctionalAddress[] = 	"Functional Address";
static const char ITEarlyToken[] = 		"16Mb Early Token";
static const char ITAutoRecovery[] = 		"Auto Recovery";
static const char ITIPAttach[] = 		"IP Attach";
static const char ITIPMtu[] = 			"IP Mtu";
static const char ITIPTokenPriority[] = 	"IP Token Priority";
static const char ITIPSourceRouting[] = 	"IP 802.5 Source Routing";

static const char ITipFowarding[] = 		"IP LAN-to-LAN Routing";


@interface DriverCmdtr:Object
{
@private
    port_t	_driverPort_kern;
    id		_interLock;
#define OPDONE		1
#define BUSY		2
#define IDLE		3
    int		_oper;
#define RESET_ON	1
#define RESET_OFF	2
#define TERMINATE	4
    int		_ret;
}
@end

@implementation DriverCmdtr

- initPort:(port_t)port
{
    [super init];

    _interLock = [[NXConditionLock alloc] initWith:IDLE];
    _driverPort_kern = IOGetKernPort(port);
    
    return self;
}

 
- free
{
    [_interLock free];
    
    port_release(_driverPort_kern);
    
    return [super free];
}

- (int)oper
{
    return (_oper);
}

- (void)done:(int)ret
{
    if ([_interLock condition] == BUSY) {
	[_interLock lock];
	_ret = ret;
	[_interLock unlockWith:OPDONE];
    }
}

- (int)send:(int)oper
{
    msg_header_t	msg = { 0 };
    int			result;
    
    [_interLock lockWhen:IDLE];
        
    _oper = oper;
    
    msg.msg_size = sizeof (msg);
    msg.msg_remote_port = _driverPort_kern;
    msg.msg_id =  IO_COMMAND_MSG;
    
    [_interLock unlockWith:BUSY];
    
    result = msg_send_from_kernel(&msg, MSG_OPTION_NONE, 0);
    if (result == SEND_SUCCESS) {
	[_interLock lockWhen:OPDONE];
	result = _ret;
    }
    else
    	[_interLock lock];
    
    [_interLock unlockWith:IDLE];
    
    return (result);
}

@end

@implementation IOTokenRing

/*
 * Private Methods
 */
 
/*
 * Set maximum packet size based on ring speed.  Unfortunately, there may
 * be different interpretations for token-ring.  Subclass can override if it
 * disagrees, or must reduce due to implementation limitations.
 */
-(void)_set8025FrameSizes
{
    /*
     * The maximum allowable packet size is a function of ring speed 
     * (per IEEE 802.5 standard). 
     */
    if(_ringSpeed == 4) {
	_maxInfoFieldSize = MAC_INFO_4MB; 
    }   
    if(_ringSpeed == 16) {
	_maxInfoFieldSize = MAC_INFO_16MB;
    }
}

/*
 * Get info from Instance.table (some params are just saved for
 * optional subclass use).
 */
- (int)_getInstanceTable:(IODeviceDescription *)devDesc
{

    IOConfigTable 	*confTable;
    const char 	*s;

    confTable = [devDesc configTable];
    if(confTable == nil) {
	IOLog("%s: couldn't get Instance%d.table\n",
	    [self name], [self unit]);
	return 1;
    }

    /*
	* MAC layer parms ...
	*/

    s = [confTable valueForStringKey: ITRingSpeed];		// ring speed
    if (!strcmp(s, "4")) 
    	_ringSpeed = 4;
    else if (strcmp(s, "16") == 0) 
    	_ringSpeed = 16;
    else {
	IOLog("%s: invalid ring speed in Instance.table\n", [self name]);
	_ringSpeed = 16;
    }
    [confTable freeString:s];

    s = [confTable valueForStringKey: ITNodeAddress];	// Node address
#if 0
    /*
     * Unsupported for now.  The subclass is free to access the config
     * table directly to read the node address and use it.
     */
    if (strcmp(s, "")) {
	//_nodeAddress
	IOLog("%s: cannot override burned-in addr\n", [self name]);
    } 
    else 
    	IOLog("%s: will use burned-in node address\n", [self name]);
#endif
    [confTable freeString:s];

#if 0
    s = [confTable valueForStringKey: ITGroupAddress];	// Group addr
    if (strcmp(s, "")) {
	//_groupAddress	
	IOLog("%s: cannot set Group addr\n", [self name]);
    } 
    else 
    	IOLog("%s: no Group Addresses enabled\n", [self name]);
    [confTable freeString:s];

    s = [confTable valueForStringKey: ITFunctionalAddress];	// Func addr
    if (strcmp(s, "")) {
	//_functionalAddress
	IOLog("%s: cannot set Functional addr\n", [self name]);
    } 
    else 
	IOLog("%s: no Functional Addresses enabled\n", [self name]);
    [confTable freeString:s];
#endif 0

    s = [confTable valueForStringKey: ITEarlyToken];	// early token
    if (!strcmp(s, "YES")) 
    	_flags._isEarlyTokenEnabled = 1;
    else 
    	_flags._isEarlyTokenEnabled = 0;
    [confTable freeString:s];

    s = [confTable valueForStringKey: ITAutoRecovery];	// auto recov
    if (!strcmp(s, "YES")) 
    	_flags._shouldAutoRecover = 1;
    else 
    	_flags._shouldAutoRecover = 0;
    [confTable freeString:s];

#if 0
    /*
     * IP params ...
     */

    s = [confTable valueForStringKey: ITIPAttach];		// attach i/f
    if (!strcmp(s, "YES")) 
    	_flags._shouldAttachIP = 1;
    else 
    	_flags._shouldAttachIP = 0;
    [confTable freeString:s];

    s = [confTable valueForStringKey: ITIPMtu];		// mtu
    _ipMtu = strtol(s, 0, 10);
    [confTable freeString:s];

    s = [confTable valueForStringKey: ITIPTokenPriority];	// token pri
    if (!strcmp(s, "DEFAULT")) 
    	_ipTokenPriority = 8;
    else 
    	_ipTokenPriority = strtol(s, 0, 10);
    [confTable freeString:s];

    s = [confTable valueForStringKey: ITIPSourceRouting];	// source rtng
    if (!strcmp(s, "YES")) 
    	_flags._doesIPSourceRouting = 1;
    else 
    	_flags._doesIPSourceRouting = 0;
    [confTable freeString:s];

    /*
     * Allow fowarding of packets between interfaces in a multi-homed
     * host (a don't care for hosts with single i-faces).  Note that
     * if no param is specified we don't touch ipforwarding (as of
     * this writing this parameter is left out of our Default.table).
     */
    s = [confTable valueForStringKey: ITipFowarding]; 
    if (!strcmp(s, "YES")) 
    	ipforwarding = 1;
    if (!strcmp(s, "NO")) 
    	ipforwarding = 0;
    [confTable freeString:s];
#endif 0
  
    /*
     * Set these defaults here for now.  We'll support them in the instance
     * table eventually.
     */
    _flags._shouldAttachIP = 1;
    _ipMtu = 8100;
    _ipTokenPriority = 8;
    _flags._doesIPSourceRouting = 1;
    ipforwarding = 0;

    return 0;
}


/*
 * Public Methods
 */

/*
 * Normally invoked from subclass method of same name.
 */
- initFromDeviceDescription:(IODeviceDescription *)devDesc
{
    char	devName[8];
    int 	unit;
    
    if ([super initFromDeviceDescription:devDesc] == nil)
	return nil;

    if ([self startIOThread] != IO_R_SUCCESS) {
    	[self free]; 
	return nil;
    }
    
    unit = TRDeviceCount++;
  
    sprintf(devName, "%s%d", TRDeviceName, unit);
    [self setName:devName];
    [self setDeviceKind:TRDeviceType];
    [self setUnit:unit];

    if([self _getInstanceTable:devDesc]) {
	[self free]; 
	return nil;
    }

    _driverCmd = [[DriverCmdtr alloc] initPort:[self interruptPort]];

    [self _set8025FrameSizes];	
    
    return self;
}

- free
{
    [self clearTimeout];
    if (_driverCmd) {
	[_driverCmd send:TERMINATE];
	[_driverCmd free];
    }
    if (_netif)
    	[_netif free];
    return [super free];
}

/*
 * Subclass must invoke this method to attach our networking interfaces
 * (typically done after successful hardware init).
 */
- (IONetwork *)attachToNetworkWithAddress:(token_addr_t)addrs
{
    int 	vtripf = 0; 
    int		i;

    [self registerDevice];

    _nodeAddress = addrs;
    
    /*
     * Netif for "real" (hdw) driver interface.
     */
    _netif = [[IONetwork alloc] initForNetworkDevice:self
	name:TRDeviceName unit:[self unit]
	type:IFTYPE_TOKENRING
	maxTransferUnit:_maxInfoFieldSize   
	flags:0];

    /*
     * Netif for IP to get to us.
     */
    if(_flags._shouldAttachIP) {
	if(_flags._doesIPSourceRouting) vtripf |= IP_SRCROUTING;
	vtrip_config([self unit], vtripf, _ipMtu, _ipTokenPriority);
    }

    /*
     * Emit the node address
     */
     IOLog("%s: Token Ring Node address %02x:%02x:%02x:%02x:%02x:%02x\n",
	[self name],
    	_nodeAddress.ta_byte[0],_nodeAddress.ta_byte[1],
    	_nodeAddress.ta_byte[2],_nodeAddress.ta_byte[3],
    	_nodeAddress.ta_byte[4],_nodeAddress.ta_byte[5]);
    
#if 0
    /*
     * Emit the group address, if non-zero
     */
    for (i = 2 ; i < 4 ; ++i) {
    	if (_groupAddress.ta_byte[i]) {
	    IOLog("%s: Token Ring Group address "
		"%02x:%02x:%02x,%02x:%02x:%02x\n",
		[self name],
		_groupAddress.ta_byte[0], _groupAddress.ta_byte[1],
		_groupAddress.ta_byte[2], _groupAddress.ta_byte[3],
		_groupAddress.ta_byte[4], _groupAddress.ta_byte[5]);
	    break;
	}
    }
    
    /*
     * Emit the functional address, if non-zero
     */
    
    for (i = 2 ; i < 4 ; ++i) {
    	if (_functionalAddress.ta_byte[i]) {
	    IOLog("%s: Token Ring Functional address "
		"%02x:%02x:%02x,%02x:%02x:%02x\n",
		[self name],
		_functionalAddress.ta_byte[0], _functionalAddress.ta_byte[1],
		_functionalAddress.ta_byte[2], _functionalAddress.ta_byte[3],
		_functionalAddress.ta_byte[4], _functionalAddress.ta_byte[5]);
	    break;
	}
    }
#endif 0
    return _netif;
}

- (IONetwork *)networkInterface
{
    return _netif;
}

- (BOOL)isRunning
{
    return (BOOL)_flags._isRunning;
}

- (void)setRunning:(BOOL)running
{
    _flags._isRunning = running;
}

/*
 * Implement driver timeouts.
 */
 
- (unsigned int)relativeTimeout
{
    ns_time_t			timestamp;

    if (_absTimeout == 0)	
    	return (0);
    
    IOGetTimestamp(&timestamp);
    
    if (_absTimeout <= timestamp) {
	_absTimeout = 0; return (0);
    }
	
    return ((unsigned int) ((_absTimeout - timestamp) / (1000 * 1000)));
}

- (void)setRelativeTimeout:(unsigned int)timeout
{
    if (_absTimeout > 0)
    	(void)ns_untimeout((func)TokenRingTimeout, self);

    IOGetTimestamp(&_absTimeout);
    
    _absTimeout += ((ns_time_t)timeout * (1000 * 1000));
    
    ns_abstimeout((func)TokenRingTimeout, self, 
    	_absTimeout, CALLOUT_PRI_THREAD);
}

- (void)clearTimeout
{
    if (_absTimeout > 0) {
	(void)ns_untimeout((func)TokenRingTimeout, self);
	_absTimeout = 0;
    }
}

- (BOOL)earlyTokenEnabled
{
    return (BOOL)_flags._isEarlyTokenEnabled;
}

- (BOOL)shouldAutoRecover
{
    return (BOOL)_flags._shouldAutoRecover;
}

- (unsigned)ringSpeed
{
    return _ringSpeed;
} 

- (unsigned)maxInfoFieldSize
{
    return _maxInfoFieldSize;
}

- (void)setMaxInfoFieldSize:(unsigned)size
{
    //sanity check?
    _maxInfoFieldSize = size;
}

/*
 * Hardware Address Methods
 */

- (token_addr_t)nodeAddress
{
    return _nodeAddress;
}

#if 0

- (token_addr_t)groupAddress
{
    return _groupAddress;
}

- (token_addr_t)functionalAddress
{
    return _functionalAddress;
}

#endif 0

/*
 * Implement the IONetworkDeviceMethods protocol
 */
 
- (int)finishInitialization
{
    int	result = 0;

    if (!_flags._isRunning)
	result = [_driverCmd send:RESET_ON];

    return (result);
}

- (BOOL)resetAndEnable:(BOOL)enable
{
    return YES;
}

- (void)transmit:(netbuf_t)pkt
{
    nb_free(pkt);
}


/*
 * Token-ring transmit.  Invokes [self transmit] in subclass 
 * to do actual hardware-specific transmission.
 */
- (int)outputPacket :(netbuf_t)nb address:(void *)dest_addr
{

    int 		maclen;
    tokenHeader_t 	*th = (tokenHeader_t *)dest_addr;

    /* 
     * Bail out if our hardware is down.
     */
    if(!_flags._isRunning) {
	nb_free(nb);
	return TRINGDOWN;
    }

    /* 
     * Return error if data-field too large (shouldn't happen by
     * well behaved code, but we include for safety).
     */
    if(nb_size(nb) > _maxInfoFieldSize) {
	IOLog("%s: netOutput bad frame size=%d\n",
	    [self name], nb_size(nb));
	nb_free(nb);
	return TBADFSIZE; 
    }

    /*
     * Get the callers MAC header length (which is variable, due
     * to the source routing field).
     */
    maclen = get_8025_hdr_len(th); 
    if(maclen < 0) {
	IOLog("%s: bad mac header\n", [self name]);
	nb_free(nb);
	return TBADFSIZE;  
    }

    /*
     * Add the 802.5 MAC header to the net buffer.  Note that when
     * we allocate buffers we reserve MAC_HDR_MAX (the largest possible
     * MAC header).  Here we grow the buffer so that the actual MAC header
     * just fits.  The packet data is therefore contiguous (as opposed to
     * a gap between the unused portion of the MAC header and the data
     * field).  This saves the subclass from needing to deal with
     * fragmentation.  This is all great as long as the subclass
     * hardware only requires even-aligned data.  Otherwise the subclass
     * must implement its' own outputPacket method or copy the data
     * (sorry).
     */
    nb_grow_top(nb, maclen);  // grow buffer so that mac header just fits
    th = (tokenHeader_t *)nb_map(nb);
    bcopy(dest_addr, th, maclen); /* copy da to tx buf */
    th->ac &= 0xf0; /* mask all but token priority & mon bits */

    /*
     * Subclass does hdw-specific transmit.
     */
    [self transmit:nb];	 

    return 0;
}

/*
 * Netif get-buffer method.   
 *
 * This generic version doesn't insure buffer alignment or prevent page
 * spanning.  Subclass should override if needed (as does TokenExpress).
 */
- (netbuf_t)allocateNetbuf
{
    netbuf_t nb;

    nb = nb_alloc(_maxInfoFieldSize + MAC_HDR_MAX);
    if (nb)
    	nb_shrink_top(nb, MAC_HDR_MAX);  /* reserve space for max 802.5 hdr */
	    
    return nb;		
}

/*
 * Netif command method.
 */
- (int)performCommand:(const char *)command data:(void *)data
{
    int	rtn = 0;

    if (strcmp(command, IFCONTROL_SETFLAGS) == 0) 
	return(rtn);

    else if (strcmp(command, IFCONTROL_GETADDR) == 0) {
	bcopy((void *)&_nodeAddress, data, sizeof(_nodeAddress));
    } 
    
    else {
	rtn = EINVAL;
    }

    return (rtn);
}

@end

@implementation IOTokenRing(PrivateMethods)

//
// Used to handle a synchronous
// command sent by another thread
// to the driver thread.
//
- (void)commandRequestOccurred
{
    int		oper = [_driverCmd oper];
    int		result = 0;

    switch (oper) {
    
    case RESET_ON:
    	if (!_flags._isRunning) {
	    if (![self resetAndEnable:TRUE])
		result = EIO;
	}
	[_driverCmd done:result];
	break;
    
    case RESET_OFF:
    	[self resetAndEnable:FALSE];
	_flags._isRunning = FALSE;
	[_driverCmd done:result];
	break;

    case TERMINATE:
    	[_driverCmd done:result];
	IOExitThread();
	break;
    }
}

@end

static void
TokenRingTimeout(IOTokenRing *device)
{
    msg_header_t	msg = { 0 };
    
    if (device->_absTimeout > 0) {	
	device->_absTimeout = 0;
	
    
	msg.msg_size = sizeof (msg);
	msg.msg_remote_port = IOGetKernPort([device interruptPort]);
	msg.msg_id =  IO_TIMEOUT_MSG;
	
	msg_send_from_kernel(&msg, MSG_OPTION_NONE, 0);
    }
}

