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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Device independent abstract superclass for Ethernet.
 *
 * HISTORY
 *
 * 25 September 1992 David E. Bohman at NeXT
 *	Created.
 *
 *  4 April 1993 Joel D. Greenblatt at NeXT
 *	Integrated multicast & promiscuous mode support.
 */

#define MACH_USER_API	1

#import <mach/mach_types.h>
#import <mach/message.h>

#import <machkit/NXLock.h>

#import <driverkit/generalFuncs.h>
#import <driverkit/interruptMsg.h>

#import <sys/errno.h>
#import <sys/socket.h>
#ifndef NEXT
#define NEXT 1
#endif
#import <sys/sockio.h>

#import <driverkit/IOEthernet.h>
#import <driverkit/IOEthernetPrivate.h>
#import	<netinet/in_var.h>
#import	<netinet/if_ether.h>

#import <net/bpf.h>

#define min(a, b)	(((a) < (b)) ? (a) : (b))

/*
 * Allowable values for _net_buf_type
 */
enum {
	_NET_BUF_TYPE_NETBUF,	// netbuf
	_NET_BUF_TYPE_MBUF,		// mbuf
};

static void
IOEthernetTimeout(IOEthernet	*device);

static char	IOEthernetDeviceName[] = "en",
		IOEthernetDeviceType[] = "Ethernet";
static int	IOEthernetDeviceCount = 0;


@interface DriverCmd:Object
{
@private
    port_t		_driverPort_kern;
    id			_interLock;
#define DONE		1
#define BUSY		2
#define IDLE		3
    int			_oper;
#define RESET_ON		1
#define RESET_OFF		2
#define TERMINATE		4
#define PROMISC_ENABLE		5
#define	PROMISC_DISABLE 	6
#define	MULTICAST_ENABLE 	7 
#define	MULTICAST_DISABLE	8 
    int			_ret;
    void *		_param;
}
@end

@implementation DriverCmd

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

- (void *)param
{
    return (_param);
}

- (void)done:(int)ret
{
    if ([_interLock condition] == BUSY) {
	[_interLock lock];
	_ret = ret;
	[_interLock unlockWith:DONE];
    }
}

- (int)send:(int)oper withParam:(void *)param

{
    msg_header_t	msg = { 0 };
    int			result;
    
    [_interLock lockWhen:IDLE];
        
    _oper = oper;
    _param = param;
    
    msg.msg_size = sizeof (msg);
    msg.msg_remote_port = _driverPort_kern;
    msg.msg_id =  IO_COMMAND_MSG;
    
    [_interLock unlockWith:BUSY];
    
    result = msg_send_from_kernel(&msg, MSG_OPTION_NONE, 0);
    if (result == SEND_SUCCESS) {
	[_interLock lockWhen:DONE];
	result = _ret;
    }
    else
    	[_interLock lock];
    
    [_interLock unlockWith:IDLE];
    
    return (result);
}

- (int)send:(int)oper
{
    return( [self send:oper withParam:0]);
}

@end

@implementation IOEthernet

- initFromDeviceDescription:(IODeviceDescription *)devDesc
{
    char	devName[8];
    int		unit;
    
    if ([super initFromDeviceDescription:devDesc] == nil)
    	return nil;
	
    if ([self startIOThread] != IO_R_SUCCESS) {
    	[self free]; 
	return nil;
    }

    if( (en_arpcom = IOMalloc( sizeof( *en_arpcom))) == NULL) {
        [self free];
        return nil;
    }
    bzero( en_arpcom, sizeof( *en_arpcom));

    _driverCmd = [[DriverCmd alloc] initPort:[self interruptPort]];

    queue_init(&_multicastQueue);

    if ([self respondsTo:@selector(mbufsPlease)]) {
		_net_buf_type = _NET_BUF_TYPE_MBUF;
		printf("IOEthernet: driver registered to use mbufs\n");
    }
    else {
		_net_buf_type = _NET_BUF_TYPE_NETBUF;
    }

    unit = IOEthernetDeviceCount++;
    
    sprintf(devName, "%s%d", IOEthernetDeviceName,unit);
    [self setName:devName];
    [self setDeviceKind:IOEthernetDeviceType];
    [self setUnit:unit];
    
    [self registerDevice];
    
    return self;
}

- free
{
    enetMulti_t		*multiAddr;

    [self clearTimeout];
    if (_driverCmd) {
	[_driverCmd send:TERMINATE];   
	[_driverCmd free];
    }
    if (_netif)
    	[_netif free];

    if( en_arpcom)
        IOFree( en_arpcom, sizeof( *en_arpcom));

    return [super free];
}

- (BOOL)isRunning
{
    return (_isRunning);
}

- (void)setRunning:(BOOL)running
{
    _isRunning = running;
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
    	(void)ns_untimeout((func)IOEthernetTimeout, self);

    IOGetTimestamp(&_absTimeout);
    
    _absTimeout += ((ns_time_t)timeout * (1000 * 1000));
    
    ns_abstimeout(
	(func)IOEthernetTimeout, self, _absTimeout, CALLOUT_PRI_THREAD);
}

- (void)clearTimeout
{
    if (_absTimeout > 0) {
	(void)ns_untimeout((func)IOEthernetTimeout, self);
	_absTimeout = 0;
    }
}

//
// Multicast Methods
//

/*
 * Determine if specified packet passes thru multicast filter. Returns
 * YES if packet should be dropped (because it is an unregistered multicast
 * packet), else returns NO.
 * Must be called from command thread.
 */
- (BOOL)isUnwantedMulticastPacket:(ether_header_t *)header
{
	int i;
	enetMulti_t *multi;
	BOOL	isBroadcastPacket = YES;

    if ((header->ether_dhost[EA_GROUP_BYTE] & EA_GROUP_BIT) &&
	    !_promiscEnabled) {
		
		
	for (i = 0 ; i < NUM_EN_ADDR_BYTES ; ++i) {
		if (header->ether_dhost[i] != 0xff) {
			isBroadcastPacket = NO;
			break;
		}
	}

  	/*
  	 * Always accept the all-ones broadcast.
  	 */
	if (isBroadcastPacket)  
  	    return NO;
			
	/*
	 * Is this address registered as a multicast address?
	 *
	 * CAREFUL - this assumes that the bytes in
	 * the packet are in the same format as the
	 * bytes in an enet_addr_t...
	 */
	multi = [self searchMulti:(enet_addr_t *)&header->ether_dhost];
	if (multi == NULL) {
	    /*
	     * We don't want this.
	     */
	    return YES;
	} 
    }     	 
    
    /*
     * Not a group packet (or is a registered multicast pckt),
     * must be for us.
     */
    return NO;
}


/* 
 * Determine if the outgoing packet should be received by the current 
 * interface (either because it's a broadcast packet or a multicast 
 * packet for which we are enabled); if so, send a copy of the packet 
 * up the pipe.
 */
/*
 * DWS 7/25/97
 *
 * In BSD 4.4, the layer above already takes care of looping back packets:
 * see bsd/net/if_ethersubr.c.
 */
- (void)performLoopback : (netbuf_t)pkt
{
}

#import <sys/param.h>
#import <sys/mbuf.h>

//
// Implement the IONetworkDeviceMethods protocol.
//
// NOTE: Stop the unneeded copies and proc calls.
// Also: we want to "guarantee" that the media header is still
//	 in the mbuf; the new code makes it clear.
//
-(int) inputPacket: (netbuf_t)pkt  extra:(void *)extra
{
	struct ether_header *eh;
	struct ifnet *ifp;
	struct mbuf *m;
	
	ifp = (struct ifnet *)&en_arpcom->ac_if;
	ifp->if_ipackets += 1;
	m = (struct mbuf *)pkt;
	m->m_pkthdr.rcvif = ifp;

	if (ifp->if_bpf)
		BPF_MTAP(ifp->if_bpf, m);

	eh = (struct ether_header *)m->m_data;
	m->m_len -= sizeof (struct ether_header);
	m->m_data += sizeof (struct ether_header);

    if (_net_buf_type == _NET_BUF_TYPE_NETBUF) { 
		/* guaranteed one-piece packet */
		m->m_pkthdr.len = m->m_len;
    }
    else {
		m->m_pkthdr.len -= sizeof(struct ether_header);
    }

	ether_input(ifp, eh, (struct mbuf *)pkt);
	
	return 0;
}

- (int)finishInitialization
{
    return [_driverCmd send:RESET_ON];
}

- (int)outputPacket:(netbuf_t)pkt address:(void *)addrs
{
    ether_header_t	*header;
    enet_addr_t		*ea;
    int			length;

    if (!_isRunning) {
    	nb_free(pkt);
	return 0;
    }
    
    /*
     * Setup the Ethernet header
     */
    header = (ether_header_t *)nb_map(pkt);

    /*
     * Insert the destination address
     */
    ea = (enet_addr_t *)header->ether_dhost;
    *ea = *(enet_addr_t *)addrs;

    /*
     * Insert our source address
     */  
    ea = (enet_addr_t *)header->ether_shost;
    *ea = _ethernetAddress;

    /*
     * Insure that the packet meets minimum length requirements	
     */
    length = nb_size(pkt);
    if (length < (ETHERMINPACKET - ETHERCRC))
    	nb_grow_bot(pkt, ETHERMINPACKET - ETHERCRC - length);
	
    [self transmit:pkt];
    
    return (0);
}

- (netbuf_t)allocateNetbuf
{
    return (nb_alloc(ETHERMAXPACKET));
}


- (int)performCommand:(const char *)command data:(void *)data
{
    return (0);
}
- (id)getDriverCmd
{
	return(_driverCmd);
}

- (struct ifnet *)allocateIfnet
{
	return(&en_arpcom->ac_if);
}

//
// Methods which implement the
// IOEthernet(DriverInterface) category.
//
- (BOOL)resetAndEnable:(BOOL)enable
{
    return (TRUE);
}

IOEthernet_ioctl(struct ifnet * ifp,u_long cmd, caddr_t data)
{
	struct arpcom *			ar;
	id 				device = ifp->if_private;
	id 				dcmd;
	unsigned 			error = 0;
	struct ifaddr *			ifa = (struct ifaddr *)data;
	struct ifreq *			ifr = (struct ifreq *)data;
	unsigned 			ioctl_command;
	void *				ioctl_data;
	int 				s;
// R E M E M B E R
// sockaddr_in comes from bsd/netinet/in.h
	struct sockaddr_in * 		sin;

	sin = (struct sockaddr_in *)(&((struct ifreq *)data)->ifr_addr);

	ar = (struct arpcom *)ifp;

	dcmd = [device getDriverCmd];
	switch (cmd) {
	  case SIOCAUTOADDR:
	    error = in_bootp(ifp, sin, 
			     &((struct IOEthernet *)device)->_ethernetAddress);
	    break;

	case SIOCSIFADDR:
#if NeXT
		ifp->if_flags |= (IFF_UP | IFF_RUNNING);
#else
		ifp->if_flags |= IFF_UP;
#endif
		switch (ifa->ifa_addr->sa_family) {
		case AF_INET:
			[dcmd send:RESET_ON];
			/*
			 * See if another station has *our* IP address.
			 * i.e.: There is an address conflict! If a
			 * conflict exists, a message is sent to the
			 * console.
			 */
			if (IA_SIN(ifa)->sin_addr.s_addr != 0) { /* don't bother for 0.0.0.0 */
			    ar->ac_ipaddr = IA_SIN(ifa)->sin_addr;
			    arpwhohas(ar, &IA_SIN(ifa)->sin_addr);
			}
			break;
		default:
			[dcmd send:RESET_ON];
			break;
		}
		break;

	case SIOCSIFFLAGS:
		/*
		 * If interface is marked down and it is running, then stop it
		 */
		if ((ifp->if_flags & IFF_UP) == 0 &&
		    (ifp->if_flags & IFF_RUNNING) != 0) {
			/*
			 * If interface is marked down and it is running, then
			 * stop it.
			 */
			ifp->if_flags &= ~IFF_RUNNING;
		} else if ((ifp->if_flags & IFF_UP) != 0 &&
		    	   (ifp->if_flags & IFF_RUNNING) == 0) {
			/*
			 * If interface is marked up and it is stopped, then
			 * start it.
			 */
			[dcmd send:RESET_ON];
			ifp->if_flags |= IFF_RUNNING;
		} else {
			/*
			 * Reset the interface to pick up changes in any other
			 * flags that affect hardware registers.
			 */
			[dcmd send:RESET_ON];
		}
		/*
		 * Set or clear promiscuous mode as appropriate
		 */
		if (ifp->if_flags & IFF_RUNNING) {
			BOOL isProm = ((struct IOEthernet *)device)->_promiscEnabled;
			if (isProm && !(ifp->if_flags & IFF_PROMISC))
				[dcmd send:PROMISC_DISABLE];
			else if (!isProm && (ifp->if_flags & IFF_PROMISC))
				[dcmd send:PROMISC_ENABLE];

		}
		break;

	case SIOCADDMULTI:
	    error = ether_addmulti(ifr, ar);
	    if (error == ENETRESET) {
		error = 0;
		[device enableMulticast:
		 (enet_addr_t *)ar->ac_multiaddrs->enm_addrlo];
	    }
	    break;
	case SIOCDELMULTI:	   
	    { 
		struct ether_addr enaddr[2]; /* 0 - addrlo, 1 - addrhi */

		error = ether_delmulti(ifr, ar, enaddr);
		if (error == ENETRESET) {
		    error = 0;
		    [device disableMulticast:enaddr]; /* XXX assume addrlo == addrhi */
		}
	    }
	    break;

	default:
	    error = EINVAL;
	    break;
    }
    return (error);
//	return [device performCommand:cmd data:data];
	

}

#include <sys/mbuf.h>

static int
IOEthernet_transmitPacket(
    struct ifnet        *ifp
)
{
    void		*buf_p;
    id			device = ifp->if_private;
    IOEthernet  *eth = (IOEthernet *)device;
    struct mbuf		*m, *mp;
    int			mLen;
    netbuf_t      	pkt;
    int			pktSize;
	
    if (device == nil) {
	return -1;
    }

    IF_DEQUEUE(&(ifp->if_snd), m);

    if (m == 0)
        return 0;
	
	if ((m->m_flags & M_PKTHDR) == 0) {
		IOLog("IOEthernet: M_PKTHDR flag not set (%04x)\n", m->m_flags);
		m_freem(m);
		return -1;
	}

    if (ifp->if_bpf)
	BPF_MTAP(ifp->if_bpf, m);

    if (eth->_net_buf_type == _NET_BUF_TYPE_MBUF)
		[device transmit:(netbuf_t)m];
    else {
	/* old compat interface uses netbufs */
    pkt = [device allocateNetbuf]; /* call the driver's allocation routine */
    if (pkt == 0) {
	m_freem(m);
        return 0;
    }

    mLen = m->m_pkthdr.len;
    buf_p = nb_map(pkt);
    pktSize = nb_size(pkt);
    if (pktSize < mLen) {
	IOLog("IOEthernet: %s driver packet size too small (%d < %d)\n",
	      [device name], pktSize, mLen);
	m_freem(m);
	nb_free(pkt);
	return 0;
    }

    { /* copy the mbuf to the netbuf */
	int 		len = mLen;
	struct mbuf * 	m_pkt;

	for (mp = m; mp; mp = mp->m_next) {
            if (mp->m_len == 0)
		continue;
            bcopy(mtod(mp, caddr_t), buf_p, min(len, mp->m_len));
            buf_p += mp->m_len;
            len -= mp->m_len;
	}
	m_pkt = (struct mbuf *)pkt;
	m_pkt->m_len = mLen;
	m_pkt->m_pkthdr.len = mLen;
    }
    m_freem(m);
    [device transmit:pkt];
    }

    return 0;
}


static void
null_func()
{
}

- (IONetwork *)attachToNetworkWithAddress:(enet_addr_t)addrs
{
    struct ifnet *ifp;

    _ethernetAddress = addrs;
    bcopy(&addrs,en_arpcom->ac_enaddr,6);
 
    _netif = [[IONetwork alloc] initForNetworkDevice:self
		    name:IOEthernetDeviceName unit:[self unit]
		    type:IFTYPE_ETHERNET
		    maxTransferUnit:ETHERMTU
		    flags:0];
	ifp = [_netif  getIONetworkIfnet];
	ifp->if_unit = [self unit];
	ifp->if_name = "en";

	/*
	 *
	 * Fix the start and WD functions
	 */
	ifp->if_output = ether_output;
	ifp->if_ioctl = IOEthernet_ioctl;
	ifp->if_start = IOEthernet_transmitPacket;
	ifp->if_watchdog = (void *) null_func;

	ifp->if_flags =
	    IFF_BROADCAST | IFF_SIMPLEX | IFF_NOTRAILERS | IFF_MULTICAST;
	bpfattach(&ifp->if_bpf, ifp, DLT_EN10MB, sizeof(struct ether_header));
	if_attach(ifp);
	ether_ifattach(ifp);
		    
    [self registerAsDebuggerDevice];
        
    IOLog("%s: Ethernet address %02x:%02x:%02x:%02x:%02x:%02x\n",
    	[self name],
	_ethernetAddress.ether_addr_octet[0],
	_ethernetAddress.ether_addr_octet[1],
	_ethernetAddress.ether_addr_octet[2],
	_ethernetAddress.ether_addr_octet[3],
	_ethernetAddress.ether_addr_octet[4],
	_ethernetAddress.ether_addr_octet[5]);
    
    return _netif;
}

- (void)transmit:(netbuf_t)pkt
{
    nb_free(pkt);
}

- (void)receivePacket:(void *)pkt
		length:(unsigned int *)pkt_len
		timeout:(unsigned int)timeout
{
    *pkt_len = 0;
}
		
- (void)sendPacket:(void *)pkt
		length:(unsigned int)pkt_len
{
}

- (BOOL)enablePromiscuousMode
{
    return YES;
}

- (void)disablePromiscuousMode
{
}

- (void)addMulticastAddress:(enet_addr_t *)addr
{
}

- (void)removeMulticastAddress:(enet_addr_t *)addr
{

}

- (BOOL)enableMulticastMode
{
    return YES;
}
- (void)disableMulticastMode
{
}

- property_IODeviceClass:(char *)classes length:(unsigned int *)maxLen
{
    [_netif property_IODeviceClass:classes length:maxLen];
    strcat( classes, " "IOClassEthernet);
    return( self);
}

@end

@implementation IOEthernet(PrivateMethods)

//
// Used to handle a synchronous
// command sent by another thread
// to the driver thread.
//
- (void)commandRequestOccurred
{
    int		oper = [_driverCmd oper];
    int		result = 0;
    enet_addr_t * multiAddr;

    switch (oper) {
    
    case RESET_ON:
    	if (!_isRunning) {
	    if (![self resetAndEnable:TRUE])
		result = EIO;
	}
	[_driverCmd done:result];
	break;
	
    case RESET_OFF:
    	[self resetAndEnable:FALSE];
	[_driverCmd done:result];
	break;
    
    case TERMINATE:
        while (!queue_empty(&_multicastQueue)) {
            enetMulti_t		*multiAddr;
            queue_remove_first(&_multicastQueue,multiAddr,enetMulti_t *,link);
            IOFree(multiAddr,sizeof(*multiAddr));
        }
    	[_driverCmd done:result];
	IOExitThread();
	break;
   
    case PROMISC_ENABLE:
	if ([self enablePromiscuousMode]) 
	    _promiscEnabled = TRUE;
	else {
	    _promiscEnabled = FALSE;
	    result = 1;
	}
    	[_driverCmd done:result];
	break;
    
    case PROMISC_DISABLE:
	[self disablePromiscuousMode];
	_promiscEnabled = FALSE;
   	[_driverCmd done:result];
	break;

    case MULTICAST_ENABLE:
        multiAddr = (enet_addr_t *) [_driverCmd param];
#ifdef DEBUG
	printf("%s: enabling multicast address %x:%x:%x:%x:%x:%x\n", 
	       [self name],
	       multiAddr->ether_addr_octet[0],
	       multiAddr->ether_addr_octet[1],
	       multiAddr->ether_addr_octet[2],
	       multiAddr->ether_addr_octet[3],
	       multiAddr->ether_addr_octet[4],
	       multiAddr->ether_addr_octet[5]);
#endif DEBUG
        [self enqueueMulticast:multiAddr];
	[self addMulticastAddress:multiAddr];
	[self enableMulticastMode];
   	[_driverCmd done:result];
	break;

    case MULTICAST_DISABLE:
        multiAddr = (enet_addr_t *) [_driverCmd param];
#ifdef DEBUG
	printf("%s: removing multicast address %x:%x:%x:%x:%x:%x\n", 
	       [self name],
	       multiAddr->ether_addr_octet[0],
	       multiAddr->ether_addr_octet[1],
	       multiAddr->ether_addr_octet[2],
	       multiAddr->ether_addr_octet[3],
	       multiAddr->ether_addr_octet[4],
	       multiAddr->ether_addr_octet[5]);
#endif DEBUG
        [self dequeueMulticast:multiAddr];
	[self removeMulticastAddress:multiAddr];
	if (queue_empty(&_multicastQueue)) {
	    [self disableMulticastMode];
	}
   	[_driverCmd done:result];
	break;
    }

}

- (queue_head_t *)multicastQueue
{
    return &_multicastQueue;
}

/*
 * Add a multicast address to the class queue.
 * Invoked from our i/o thread
 */
- (void)enqueueMulticast:(enet_addr_t *)addrs
{
    enetMulti_t *multi;

    /*
     * Paranoia check to make sure group bit is on.
     */
    if (!(addrs->ether_addr_octet[EA_GROUP_BYTE] & EA_GROUP_BIT)) {
	return;
    }

    multi = [self searchMulti:addrs];
    if (multi) {
    	/*
	 * Just bump the reference count since the hardware should
	 * already be configured.
    	 */
	if (++multi->refCount < 0)
	    multi->refCount--;
	return;
    }
    
    /*
     * New entry, so queue it
     */
    multi = IOMalloc(sizeof(*multi));
    multi->address = *addrs;	
    multi->refCount = 1;
    queue_enter(&_multicastQueue, multi, enetMulti_t *, link);
}

/*
 * Pass multicast address to the command thread.
 */

- (void)enableMulticast:(enet_addr_t *)addrs
{
    [_driverCmd send:MULTICAST_ENABLE withParam:addrs];
}

/*
 * Disable a multicast address.
 */
- (void)dequeueMulticast:(enet_addr_t *)addrs
{
    enetMulti_t *multi;

    multi = [self searchMulti:addrs];
    if (multi) {
	/*
	 * Found it, now remove it if there are no remaining
	 * references.
	 */
	if (multi->refCount > 0)
	    multi->refCount--;
	if (multi->refCount <= 0) {
	    queue_remove(&_multicastQueue, multi, enetMulti_t *, link);
	    IOFree(multi, sizeof(*multi));
	}
    }
}

- (void)disableMulticast:(enet_addr_t *)addrs
{
    [_driverCmd send:MULTICAST_DISABLE withParam:addrs];
}

/*
 * See if a given enet_addr_t is present in _multicastQueue. If so, return
 * the enetMulti_t of the entry in _multicastQueue; else return NULL.
 * Must be called from command thread.
 */
- (enetMulti_t *)searchMulti : (enet_addr_t *)addrs
{
    enetMulti_t *multi;
    int i;

    queue_iterate(&_multicastQueue,multi,enetMulti_t *,link) {

	if (memcmp(&multi->address.ether_addr_octet[0], 
			&addrs->ether_addr_octet[0], NUM_EN_ADDR_BYTES) == 0)
			return multi;
	}

    return NULL;
}

@end

static void
IOEthernetTimeout(IOEthernet *device)
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
