/*---------------------------------------------------------------------------*\
*	                                                                      *
*	Copyright (c) 2005 by Jens Heise                                      *
*	                                                                      *
*	created: 08/22/2005	              last change: 09/06/2005         *
*									      *
*			Version: 1.00					      *
*	                                                                      *
\*---------------------------------------------------------------------------*/
#import <driverkit/align.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/IONetbufQueue.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/kernelDriver.h>
#import <kernserv/prototypes.h>
#import <kernserv/kern_server_types.h>
#import <driverkit/i386/kernelDriver.h>
#import <driverkit/i386/ioPorts.h>
#import <driverkit/i386/IOPCIDeviceDescription.h>
#import <driverkit/i386/IOPCIDirectDevice.h>
#import <driverkit/i386/PCI.h>
#import <kernserv/i386/spl.h>
#import <driverkit/i386/directDevice.h>
#import <driverkit/IOEthernet.h>

#include "compat.h"
#include "vm_basic_types.h"
#include "vmxnet2_def.h"
#include "vmxnetInt.h"


				/* multicast entry                           */
#define	MAR_MAX		32

struct mar_entry {
    BOOL	valid;
    enet_addr_t	addr;
};

typedef unsigned short 	IOPortAddress;

@interface VMXNet:IOEthernet
{
    IOPortAddress	ioaddr;	/* port base 				     */
    enet_addr_t	myAddress;	/* local copy of ethernet address	     */
    IONetwork	*network;	/* handle to kernel network object	     */

    struct Vmxnet_Private	*lp;
    
				/* multicast addres list                     */
    struct mar_entry	mar_list[MAR_MAX];
    int		mar_cnt;	/* multicast address list count		     */

    id		transmitQueue;	/* queue for outgoing packets	 	     */
    BOOL	transmitStopped;/* transmits have been stopped		     */
}

+ (BOOL)probe:devDesc;

- initFromDeviceDescription:devDesc;
- free;

- (BOOL)resetAndEnable:(BOOL)enable;
- (void)timeoutOccurred;
- (void)interruptOccurred;
- (BOOL)getHandler:(IOInterruptHandler *)handler level:(unsigned int *)ipl argument:(unsigned int *)arg forInterrupt:(unsigned int)localInterrupt;

- (void)transmit:(netbuf_t)pkt;

- (BOOL)enablePromiscuousMode;
- (void)disablePromiscuousMode;

- (BOOL)enableMulticastMode;
- (void)disableMulticastMode;
- (void)addMulticastAddress:(enet_addr_t *)address;
- (void)removeMulticastAddress:(enet_addr_t *)address;

@end
