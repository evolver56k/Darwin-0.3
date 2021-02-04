/*---------------------------------------------------------------------------*\
*	                                                                      *
*	Copyright (c) 2005-2012 by Jens Heise                                 *
*	                                                                      *
*	created: 08/22/2005	              last change: 08/02/2012         *
*									      *
*			Version: 1.5					      *
*	                                                                      *
\*---------------------------------------------------------------------------*/

#import "VMXNet.h"
#import "VMXNet_ddm.h"

#include "vm_version.h"
#include "vm_basic_types.h"
#include "vmnet_def.h"
#include "vmxnet_def.h"
#include "vmxnet2_def.h"
#include "vm_device_version.h"
#include "vmxnetInt.h"
#include "net.h"

				/* Data structure used when determining what
				   hardware the driver supports.             */
typedef struct ChipID {
    uint16	vendorID;
    uint16	deviceID;
    uint16	type;
} ChipID;


ChipID chipID[] = { 
    {PCI_VENDOR_ID_VMWARE, PCI_DEVICE_ID_VMWARE_NET, VMXNET_CHIP},
    {PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_VLANCE, LANCE_CHIP}
};
#define NUM_CHIPS (sizeof chipID / sizeof chipID[0])

				/* NeXT out-functions have the arguments in
				   another order than linux so switch them.  */
#define outb(a,b)	outb((b),(a))
#define outw(a,b)	outw((b),(a))
#define outl(a,b)	outl((b),(a))

				/* Redefine linux locking function in a NeXT
				   usable way.                               */
#define spin_lock_irqsave(lock, pri)	{pri = spl6(); simple_lock(*lock);}
#define spin_unlock_irqrestore(lock, pri)	{splx(pri);		\
						 simple_unlock(*lock);}

#define spin_lock(lock)		{simple_lock(*lock);}
#define spin_unlock(lock)	{simple_unlock(*lock);}


@interface VMXNetKernelServerInstance : Object
+ (kern_server_t *)kernelServerInstance;
@end


@interface VMXNet (Private)
- open;
- initRing;
- checkTxQueue;
- close;
@end

static inline void copyAddr(enet_addr_t *from, enet_addr_t* to);
static inline void zeroAddr(enet_addr_t *addr);
static inline int compareAddr(enet_addr_t *a1, enet_addr_t *a2);

@implementation VMXNet

/*--------------------------------- ()probe: --------------------------------*\
*									      *
*	Probe for the device by trying to initialize one                      *
*									      *
\*---------------------------------------------------------------------------*/

+ (BOOL)probe:devDesc
{
    VMXNet	*dev=[self alloc];
    
    if (!dev || ![dev initFromDeviceDescription:devDesc])
	return NO;
    
    return YES;
} /* ()probe: */



/*------------------------ initFromDeviceDescription: -----------------------*\
*									      *
*	Initializes a new device and performs the basic setup.                *
*									      *
\*---------------------------------------------------------------------------*/

- initFromDeviceDescription:devDesc
{
    IOPCIConfigSpace	configSpace;
    IOReturn		irtn;
    unsigned long	regLong;
    int			idx;
    IORange		portRange;
    unsigned int	irq_line;
    unsigned int	numRxBuffers;
    unsigned int	numTxBuffers;
    Bool		morphed=FALSE;
    int			i;
    unsigned int	driverDataSize;
    unsigned char	*enaddr=(unsigned char *)&myAddress;
    
    [self setName:"VMXNet"];
    
    mar_cnt = 0;	//list is empty
    for (i=0; i<MAR_MAX; i++)
    {
        mar_list[i].valid = NO;
        zeroAddr(&(mar_list[i].addr));
    } /* for */

				/* Read the PCI configuration of the device. */
    bzero(&configSpace, sizeof(IOPCIConfigSpace));
    if (irtn = [IODirectDevice getPCIConfigSpace:&configSpace
			withDeviceDescription:devDesc])
    {
	IOLog("%s: Can\'t get configSpace (%s); ABORTING\n", 
		    [self name], [IODirectDevice stringFromReturn:irtn]);
	return nil;
    } /* if */
    
				/* Loop through all known chip sets.         */
    for (idx=0; idx<NUM_CHIPS; idx++)
    {
	unsigned int	reqIOAddr;
	unsigned int	reqIOSize;
				/* VMware's version of the magic number      */
	unsigned int	low_vmware_version;
	
				/* Check if the device matches the current
				   entry if not continue.                    */
	if (chipID[idx].vendorID != configSpace.VendorID || 
	    chipID[idx].deviceID != configSpace.DeviceID)
	    continue;
	
	irq_line = configSpace.InterruptLine;
	ioaddr = configSpace.BaseAddress[0] & 0xfffffffc;

				/* Found adapter, adjust ioaddr to match the
				   adapter we found.                         */
	 if (chipID[idx].type == VMXNET_CHIP)
	 {
	    reqIOAddr = ioaddr;
	    reqIOSize = VMXNET_CHIP_IO_RESV_SIZE;
	 } /* if */
	 else 
	 {
	    
				/* Since this is a vlance adapter we can only
				   use it if its I/0 space is big enough for
				   the adapter to be capable of morphing.
				   This is the first requirement for this
				   adapter to potentially be morphable. The
				   layout of a morphable LANCE adapter is
				   
				   I/O space:
				   
				   |------------------| 
				   | LANCE IO PORTS   |
				   |------------------| 
				   | MORPH PORT       |
				   |------------------| 
				   | VMXNET IO PORTS  |
				   |------------------|
				   
				   VLance has 8 ports of size 4 bytes, the
				   morph port is 4 bytes, and Vmxnet has 10
				   ports of size 4 bytes.
				   
				   We shift up the ioaddr with the size of
				   the LANCE I/O space since we want to
				   access the vmxnet ports. We also shift the
				   ioaddr up by the MORPH_PORT_SIZE so other
				   port access can be independent of whether
				   we are Vmxnet or a morphed VLance. This
				   means that when we want to access the
				   MORPH port we need to subtract the size
				   from ioaddr to get to it.		     */

	    ioaddr += LANCE_CHIP_IO_RESV_SIZE + MORPH_PORT_SIZE;
	    reqIOAddr = ioaddr - MORPH_PORT_SIZE;
	    reqIOSize = MORPH_PORT_SIZE + VMXNET_CHIP_IO_RESV_SIZE;
	 } /* if..else */
	 
	portRange.start = reqIOAddr;
	portRange.size = reqIOSize;
	
				/* Morph the underlying hardware if we found
				   a VLance adapter.                         */
	if (chipID[idx].type == LANCE_CHIP)
	{
	    uint16	magic;
	    
	    IOLog("%s: Morphing adapter\n", [self name]);

				/* Read morph port to verify that we can
				   morph the adapter.                        */
	    magic = inw(ioaddr - MORPH_PORT_SIZE);
	    if (magic != LANCE_CHIP && magic != VMXNET_CHIP) 
	    {
		IOLog("%s: Invalid magic, read: 0x%08X\n", [self name], magic);
		continue;
	    } /* if */
	    
				/* Morph adapter.                            */
	    outw(VMXNET_CHIP, ioaddr - MORPH_PORT_SIZE);
	    
				/* Verify that we morphed correctly.         */
	    magic = inw(ioaddr - MORPH_PORT_SIZE);
	    if (magic != VMXNET_CHIP)
	    {
				/* Morph back to LANCE hw.                   */
		outw(LANCE_CHIP, ioaddr - MORPH_PORT_SIZE);
		IOLog("%s: Couldn't morph adapter. Invalid magic, read: "
			"0x%08X\n", [self name], magic);
		continue;
	    } /* if */
	    morphed = TRUE;
	} /* if */
	IOLog("%s: Found vmxnet/PCI at 0x%x, irq %d.\n", 
			[self name], ioaddr, irq_line);

	low_vmware_version = inl(ioaddr + VMXNET_LOW_VERSION);
	if ((low_vmware_version & 0xffff0000) != (VMXNET2_MAGIC & 0xffff0000))
	{
				/* Morph back to LANCE hw.                   */
	    if (morphed)
		outw(LANCE_CHIP, ioaddr - MORPH_PORT_SIZE);
    
	    IOLog("%s: Driver version 0x%08X doesn't match %s version "
			"0x%08X\n", [self name], VMXNET2_MAGIC, 
			PRODUCT_GENERIC_NAME, low_vmware_version);
	    continue;
	} /* if */
	else
	{
				/* The low version looked OK so get the high
				   version and make sure that our version is
				   supported.                                */
	    unsigned int	high_vmware_version;
	    
	    high_vmware_version=inl(ioaddr + VMXNET_HIGH_VERSION);
	    if ((VMXNET2_MAGIC < low_vmware_version) ||
		(VMXNET2_MAGIC > high_vmware_version)) 
	    {
				/* Morph back to LANCE hw.                   */
		if (morphed) 
		    outw(LANCE_CHIP, ioaddr - MORPH_PORT_SIZE);
		IOLog("%s: Driver version 0x%08x doesn't match %s version "
			"0x%08x, 0x%08x\n", [self name], VMXNET2_MAGIC, 
			PRODUCT_GENERIC_NAME, low_vmware_version,
			high_vmware_version);
		continue;
	    } /* if */
	    else break;
	} /* if..else */
    } /* for */
    
				/* No device found, abort now.               */
    if (idx >= NUM_CHIPS)
    {
	IOLog("%s: No matching device found!\n", [self name]);
	return [self free];
    } /* if */

				/* Register the required portrange.          */
    irtn = [devDesc setPortRangeList:&portRange num:1];
    if(irtn) 
    {
	IOLog("%s: Can\'t set portRangeList to port 0x%x (%s)\n", 
		    [self name], portRange.start, 
		    [IODirectDevice stringFromReturn:irtn]);
	return [self free];
    } /* if */

				/* Register the interrupt.                   */
    irtn = [devDesc setInterruptList:&(irq_line) num:1];
    if(irtn) 
    {
	IOLog("%s: Can\'t set interruptList to IRQ %d (%s)\n", [self name], 
		irq_line, [IODirectDevice stringFromReturn:irtn]);
	return [self free];
    } /* if */
	
				/* Enable busmastering.                      */
    if((irtn = [IODirectDevice getPCIConfigData:&regLong atRegister:0x04 
    			withDeviceDescription:devDesc]) || 
    	(irtn = [IODirectDevice setPCIConfigData:(regLong|0x0004) 
			atRegister:0x04 withDeviceDescription:devDesc]))
    {
	IOLog("%s: Can\'t enable busmastering (%s)\n", [self name], 
		[IODirectDevice stringFromReturn:irtn]);
	return [self free];
    } /* if */

				/* Get the transmission buffer sizes.        */
    outl(VMXNET_CMD_GET_NUM_RX_BUFFERS, ioaddr + VMXNET_COMMAND_ADDR);
    numRxBuffers = inl(ioaddr + VMXNET_COMMAND_ADDR);
    if (numRxBuffers == 0 || numRxBuffers > VMXNET2_MAX_NUM_RX_BUFFERS)
	numRxBuffers = VMXNET2_DEFAULT_NUM_RX_BUFFERS;

    outl(VMXNET_CMD_GET_NUM_TX_BUFFERS, ioaddr + VMXNET_COMMAND_ADDR);
    numTxBuffers = inl(ioaddr + VMXNET_COMMAND_ADDR);
    if (numTxBuffers == 0 || numTxBuffers > VMXNET2_MAX_NUM_TX_BUFFERS)
	numTxBuffers = VMXNET2_DEFAULT_NUM_TX_BUFFERS;

				/* Calculate the required size of the shared
				   area (numRxBuffers+1 for dummy rxRing2).  */
    driverDataSize = sizeof(Vmxnet2_DriverData) 
    		+ (numRxBuffers + 1) * sizeof(Vmxnet2_RxRingEntry)
    		+ numTxBuffers * sizeof(Vmxnet2_TxRingEntry);

    IOLog("%s (vmxnet): numRxBuffers=(%d*%ld) numTxBuffers=(%d*%ld) "
    		"driverDataSize=%d\n", [self name], 
                numRxBuffers, sizeof(Vmxnet2_RxRingEntry), 
                numTxBuffers, sizeof(Vmxnet2_TxRingEntry),
                driverDataSize);

				/* Allocate the private structures and
				   buffers.                                  */
    lp = (Vmxnet_Private *)IOMalloc(sizeof(Vmxnet_Private));
    if (lp == NULL)
    {
				/* Morph back to LANCE hw.                   */
	if (morphed)
	    outw(LANCE_CHIP, ioaddr - MORPH_PORT_SIZE);
	IOLog("%s: Unable to allocate memory for private extension\n", 
			[self name]);
	return [self free];
    } /* if */
    
    bzero(lp, sizeof(Vmxnet_Private));
    lp->dd = (Vmxnet2_DriverData *)
    		(((unsigned long)IOMallocLow(driverDataSize + 15) + 15) & ~15);
    bzero(lp->dd, driverDataSize);
    lp->txLock = simple_lock_alloc();
    simple_lock_init(lp->txLock);
    lp->numRxBuffers = numRxBuffers;
    lp->numTxBuffers = numTxBuffers;
				/* So that the vmkernel can check it is
				   compatible                                */
    lp->dd->magic = VMXNET2_MAGIC;
    lp->dd->length = driverDataSize;
    
				/* The port is needed in the interrupt
				   handler.                                  */
    lp->ioaddr = ioaddr;
    lp->name = VMXNET_CHIP_NAME;
				/* Store whether we are morphed so we can
				   figure out how to clean up when we unload.*/
    lp->morphed = morphed;

    outl(VMXNET_CMD_GET_FEATURES, ioaddr + VMXNET_COMMAND_ADDR);
    lp->features = inl(ioaddr + VMXNET_COMMAND_ADDR);

    outl(VMXNET_CMD_GET_CAPABILITIES, ioaddr + VMXNET_COMMAND_ADDR);
    lp->capabilities = inl(ioaddr + VMXNET_COMMAND_ADDR);

				/* Get our own MAC address                   */
    if (lp->capabilities & VMNET_CAP_VMXNET_APROM)
    {
	for (i = 0; i < NUM_EN_ADDR_BYTES; i++)
	    enaddr[i] = inb(ioaddr + VMXNET_APROM_ADDR + i);
	for (i = 0; i < NUM_EN_ADDR_BYTES; i++)
	    outb(enaddr[i], ioaddr + VMXNET_MAC_ADDR + i);
    } /* if */
    else 
    {
				/* Be backwards compatible and use the MAC
				   address register to get MAC address.      */
	for (i = 0; i < NUM_EN_ADDR_BYTES; i++)
	    enaddr[i] = inb(ioaddr + VMXNET_MAC_ADDR + i);
    } /* if..else */

				/* Initialize our super class after setting
				   the port range and interrupts.            */
    if ([super initFromDeviceDescription:devDesc] == nil)
	return nil;
    
    [self resetAndEnable:NO];

				/* Initialize the transmit queue and attach
				   to the network.                           */
    transmitQueue = [[IONetbufQueue alloc] initWithMaxCount:32];
    network = [super attachToNetworkWithAddress:myAddress];

    return self;
} /* initFromDeviceDescription: */



/*----------------------------------- free ----------------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- free
{
    if (lp != NULL)
    {
	if (lp->devOpen)
	    [self close];
	    
// 	if (lp->dd)
// 	    IOFreeLow(lp->dd, lp->dd->length+15);
	
	if (lp->morphed) 
	{
	    uint16	magic;
	    
				/* Read morph port to verify that we can
				   morph the adapter.                        */
	    magic = inw(ioaddr - MORPH_PORT_SIZE);
	    if (magic != VMXNET_CHIP)
	    {
		IOLog("%s: Adapter not morphed. read magic: 0x%08X\n", 
				[self name], magic);
	    } /* if */
	
				/* Morph adapter back to LANCE.              */
	    outw(LANCE_CHIP, ioaddr - MORPH_PORT_SIZE);
	    
				/* Verify that we unmorphed correctly.       */
	    magic = inw(ioaddr - MORPH_PORT_SIZE);
	    if (magic != LANCE_CHIP)
	    {
		IOLog("%s: Couldn't unmorph adapter. Invalid magic, read: "
			"0x%08X\n", [self name], magic);
	    } /* if */
	} /* if */
    
	IOFree(lp, sizeof(Vmxnet_Private));
	lp = NULL;
    } /* if */

    [transmitQueue free];

    return [super free];
} /* free */



/*---------------------------- ()resetAndEnable: ----------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (BOOL)resetAndEnable:(BOOL)enable
{
    [self disableAllInterrupts];

    if (lp == NULL)
    {
	[self setRunning:NO];
	return NO;
    } /* if */
    
    if (lp->devOpen)
	[self close];
    [self open];
    
    if (enable && [self enableAllInterrupts] != IO_R_SUCCESS)
    {
	[self setRunning:NO];
    	return NO;
    } /* if */
	
    [self setRunning:enable];
    return YES;
} /* ()resetAndEnable: */



/*------------------------------ receivePackets -----------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- receivePackets
{
    Vmxnet2_DriverData	*dd=lp->dd;

    while (1) 
    {
	short		pkt_len;
	netbuf_t	skb;
	Vmxnet2_RxRingEntry	*rre;
	
	rre = &lp->rxRing[dd->rxDriverNext];
	if (rre->ownership != VMXNET2_OWNERSHIP_DRIVER)
	    break;
	pkt_len = rre->actualLength;

	if (pkt_len < (ETHERMINPACKET - ETHERCRC) || pkt_len > ETHERMAXPACKET)
	{
	    if (pkt_len != 0)
		IOLog("%s: Runt pkt (%d bytes)!\n", [self name], pkt_len);
	    [network incrementInputErrors];
	} /* if */
	else
	{
	    skb = nb_alloc(pkt_len);
	    if (skb != NULL)
	    {
		IOCopyMemory(rre->driverData, nb_map(skb), 
						pkt_len, sizeof(short));
	    	[network handleInputPacket:skb extra:0];
	    } /* if */
	    else IOLog("%s: Memory squeeze, dropping packet.\n", [self name]);
	} /* if..else */

	rre->ownership = VMXNET2_OWNERSHIP_NIC;
	VMXNET_INC(dd->rxDriverNext, dd->rxRingLength);
    } /* while */

    return self;
} /* receivePackets */



/*---------------------------- ()timeoutOccurred ----------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (void)timeoutOccurred
{
    netbuf_t	pkt=NULL;

    IOLog("%s: timeout occurred\n", [self name]);
    if ([self isRunning])
    {
				/* Restart transfer and transmit the next
				   packet in the queue.                      */
	transmitStopped = NO;
	if ([transmitQueue count])
	{
	    pkt = [transmitQueue dequeue];
	    [self transmit:pkt];
	} /* if */
    } /* if */
    
    if (![self isRunning] && [transmitQueue count])
    {
				/* Free any packets in the queue since we're
				   not running.                              */
	while (pkt = [transmitQueue dequeue])
	    nb_free(pkt);
    } /* if */
    
    return;
} /* ()timeoutOccurred */



/*--------------------------- ()interruptOccurred ---------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (void)interruptOccurred
{
    [self receivePackets];

    if (lp->numTxPending > 0)
    {
	spin_lock(&lp->txLock);
	[self checkTxQueue];
	spin_unlock(&lp->txLock);
    } /* if */

    if (transmitStopped && !lp->dd->txStopped)
	transmitStopped = NO;
    
    [self enableAllInterrupts];
    
    return;
} /* ()interruptOccurred */



/*-------------------------------- clearInt() -------------------------------*\
*									      *
*	Clear the interrupt state on the card before calling original         *
*	NeXT-handler.                                                         *
*									      *
\*---------------------------------------------------------------------------*/

static void clearInt(void *identity, void *state, 
                         unsigned int arg)
{
    Vmxnet_Private	*lp=(Vmxnet_Private*)arg;
    
				/* Clear the interrupt and forward it to the
				   IOThread.                                 */
    outl(VMXNET_CMD_INTR_ACK, lp->ioaddr + VMXNET_COMMAND_ADDR);

    IOSendInterrupt(identity, state, IO_DEVICE_INTERRUPT_MSG);
    
    IOEnableInterrupt(identity);
    return;
} /* clearInt() */



/*---------------- ()getHandler:level:argument:forInterrupt: ----------------*\
*									      *
*	Replace the NeXT-handler by our own or it will not work. This is      *
*	because it takes too long for the interrupt to be cleared in          *
*	interruptOccurred.                                                    *
*									      *
\*---------------------------------------------------------------------------*/

- (BOOL)getHandler:(IOInterruptHandler *)handler level:(unsigned int *)ipl argument:(unsigned int *)arg forInterrupt:(unsigned int)localInterrupt
{
    *handler = clearInt;
    *ipl = IPLDEVICE;
    *arg = (unsigned int)lp;
    
    return YES;
} /* ()getHandler:level:argument:forInterrupt: */



/*------------------------------ ()doTransmit:: -----------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (Vmxnet_TxStatus)doTransmit:(netbuf_t)skb :(Bool)isCOS
{
    Vmxnet_TxStatus	status=VMXNET_DEFER_TRANSMIT;
    Vmxnet2_DriverData	*dd=lp->dd;
    unsigned long	flags;
    Vmxnet2_TxRingEntry	*xre;

    spin_lock_irqsave(&lp->txLock, flags);

    xre = &lp->txRing[dd->txDriverNext];
    xre->flags = 0;
    xre->flags &= ~VMXNET2_TX_HW_XSUM;

				/* No more empty slots in the ring buffer.
				   Queue further packets.                    */
    if (xre->sg.length != 0)
    {
	dd->txStopped = TRUE;
	transmitStopped = YES;
	status = VMXNET_STOP_TRANSMIT;
	if (dd->debugLevel > 0)
	    IOLog("%s: Stopping transmit\n", [self name]);
    } /* if */
    else if (nb_size(skb) <= PKT_BUF_SZ)
    {
	if (isCOS)
	{
	    xre->sg.addrType = NET_SG_MACH_ADDR;
	    xre->flags = 0;
	} /* if */
	else
	{
	    xre->sg.addrType = NET_SG_PHYS_ADDR;
	    xre->flags |= VMXNET2_TX_CAN_KEEP;	 
	} /* if..else */
	
	xre->sg.length = 1;
	xre->sg.sg[0].length = le16_to_cpu(nb_size(skb));
	IOCopyMemory(nb_map(skb), xre->driverData, nb_size(skb),sizeof(short));
	nb_free(skb);
	
	if (lp->numTxPending > dd->txRingLength - 5)
	{
	    if (dd->debugLevel > 0)
		IOLog("%s: Queue low\n", [self name]);

	    xre->flags |= VMXNET2_TX_RING_LOW;
	    status = VMXNET_CALL_TRANSMIT;
	} /* if */
	xre->ownership = VMXNET2_OWNERSHIP_NIC;

	VMXNET_INC(dd->txDriverNext, dd->txRingLength);

	dd->txNumDeferred++;
	if (isCOS || (dd->txNumDeferred >= dd->txClusterLength))
	{
	    dd->txNumDeferred = 0;
	    status = VMXNET_CALL_TRANSMIT;
	} /* if */
	lp->numTxPending++;
    } /* if */
    else
    {
	IOLog("%s: tx_pkt too long: %d\n", [self name], nb_size(skb));
	nb_free(skb);
    } /* if..else */
    [self checkTxQueue];

    spin_unlock_irqrestore(&lp->txLock, flags);

    return status;
} /* ()doTransmit:: */



/*------------------------------- ()transmit: -------------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (void)transmit:(netbuf_t)pkt
{
    Vmxnet_TxStatus	xs=VMXNET_DEFER_TRANSMIT;
    
    if (transmitStopped)
	[transmitQueue enqueue:pkt];
    else
    {
	do
	{
	    xs = [self doTransmit:pkt :FALSE];
	    switch (xs)
	    {
		case VMXNET_CALL_TRANSMIT:
		    inl(ioaddr + VMXNET_TX_ADDR);
		    [self setRelativeTimeout:3000];
		    break;
		case VMXNET_DEFER_TRANSMIT:
		    break;
		case VMXNET_STOP_TRANSMIT:
		    [transmitQueue enqueue:pkt];
		    break;
	    } /* switch */
	} while (xs == VMXNET_DEFER_TRANSMIT && 
	    	(pkt = [transmitQueue dequeue]));
    } /* if..else */
    
    return;
} /* ()transmit: */



/*------------------------- ()enablePromiscuousMode -------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (BOOL)enablePromiscuousMode
{
    lp->dd->ifflags = VMXNET_IFF_PROMISC;
    outl(VMXNET_CMD_UPDATE_IFF, ioaddr + VMXNET_COMMAND_ADDR);

    return YES;
} /* ()enablePromiscuousMode */



/*------------------------- ()disablePromiscuousMode ------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (void)disablePromiscuousMode
{
    lp->dd->ifflags = VMXNET_IFF_BROADCAST | VMXNET_IFF_MULTICAST;
    outl(VMXNET_CMD_UPDATE_IFF, ioaddr + VMXNET_COMMAND_ADDR);

    return;
} /* ()disablePromiscuousMode */



/*--------------------------- ()setMulticastList: ---------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (void)setMulticastList:(BOOL)clearList
{
    volatile unsigned short	*mcast_table=(unsigned short *)lp->dd->LADRF;
    char	*addrs;
    int		i;
    int		j;
    int		bit;
    int		byte;
    unsigned int	crc;
    unsigned int	poly=CRC_POLYNOMIAL_LE;

    /* clear the multicast filter */
    lp->dd->LADRF[0] = 0;
    lp->dd->LADRF[1] = 0;

    if (!clearList)
    {
				/* Add addresses                             */
	for (i = 0; i < mar_cnt; i++)
	{
	    addrs = (char*)&mar_list[i].addr;
	    
				/* multicast address?                        */
	    if (!(*addrs & 1))
		continue;
    
	    crc = 0xffffffff;
	    for (byte = 0; byte < 6; byte++)
	    {
		for (bit = *addrs++, j = 0; j < 8; j++, bit >>= 1)
		{
		    int	test;
    
		    test = ((bit ^ crc) & 0x01);
		    crc >>= 1;
    
		    if (test)
			crc = crc ^ poly;
		} /* for */
	    } /* for */
	    crc = crc >> 26;
	    mcast_table [crc >> 4] |= 1 << (crc & 0xf);
	} /* for */
    } /* if */
    outl(VMXNET_CMD_UPDATE_LADRF, ioaddr + VMXNET_COMMAND_ADDR);	       
    
    return;
} /* ()setMulticastList */



/*-------------------------- ()enableMulticastMode --------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (BOOL)enableMulticastMode
{
    lp->dd->ifflags = VMXNET_IFF_BROADCAST | VMXNET_IFF_MULTICAST;
    [self setMulticastList:NO];
    
    outl(VMXNET_CMD_UPDATE_IFF, ioaddr + VMXNET_COMMAND_ADDR);

    return YES;
} /* ()enableMulticastMode */



/*-------------------------- ()disableMulticastMode -------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (void)disableMulticastMode
{
    lp->dd->ifflags = VMXNET_IFF_BROADCAST | VMXNET_IFF_MULTICAST;
    [self setMulticastList:YES];
    
    outl(VMXNET_CMD_UPDATE_IFF, ioaddr + VMXNET_COMMAND_ADDR);


    return;
} /* ()disableMulticastMode */



/*-------------------------- ()addMulticastAddress: -------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (void)addMulticastAddress:(enet_addr_t *)address
{
    int i;

    if (mar_cnt == MAR_MAX)
    {
        IOLog("%s: multicast address list is full!\n", [self name]);
        return;
    }/* if */

    for (i=0; i<MAR_MAX; i++)
    {
        if (mar_list[i].valid == NO) 
	{
            mar_list[i].valid = YES;
            copyAddr(address, &(mar_list[i].addr));
            mar_cnt++;

            [self setMulticastList:NO];
            break;
        }
    } /* for */

    return;
} /* ()addMulticastAddress: */



/*------------------------ ()removeMulticastAddress: ------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- (void)removeMulticastAddress:(enet_addr_t *)address
{
    int i;

    if (mar_cnt == 0)
        return;

    for (i=0; i<MAR_MAX; i++)
    {
        if (mar_list[i].valid == YES && 
	    compareAddr(address, &(mar_list[i].addr)))
	{
            mar_list[i].valid = NO;
            zeroAddr(&(mar_list[i].addr));
            mar_cnt--;

            [self setMulticastList:NO];
            break;
        }/* if */
    } /* for */
    
    return;
} /* ()removeMulticastAddress: */



@end



@implementation VMXNet (Private)
/*---------------------------- IOAlignedMalloc() ----------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

static void *IOAlignedMalloc(int numBytes)
{
    void	*buf=IOMalloc(numBytes);

    if ((buf+numBytes) > (void*)((int)buf & ~(4096-1))+4096)
	IOLog("buffer crosses page boundary\n");

//     kern_return_t	ret;
//     
//     ret = vm_allocate(task_self(), (vm_address_t*)&buf, numBytes, TRUE);
// 
// 				/* Wire the memory down.                     */
//     if (ret==KERN_SUCCESS)
//     { 
// 	if (kern_serv_wire_range([VMXNetKernelServerInstance 
// 							kernelServerInstance],
// 			         (vm_address_t)buf, numBytes)!=KERN_SUCCESS)
// 	{
// 	    IOLog("Failed to wire down memory\n");
// 	    vm_deallocate(task_self(), (vm_address_t)buf, numBytes);
// 	    buf = NULL;
// 	} /* if */
//     } /* if */
//     else
//     {
// 	IOLog("vm_allocate failed: %d\n", ret);
// 	buf = NULL;
//     } /* if */
    
    return buf;
} /* IOAlignedMalloc() */



/*----------------------------- IOAlignedFree() -----------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

static void IOAlignedFree(void *var, int numBytes)
{
//     kern_serv_unwire_range([VMXNetKernelServerInstance kernelServerInstance],
// 			   (vm_address_t)var, numBytes);
//     vm_deallocate(task_self(), (vm_address_t)var, numBytes);
IOFree(var, numBytes);
    return;
} /* IOAlignedFree() */



/*----------------------------------- open ----------------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- open
{
    unsigned int	paddr;
    IOReturn		irtn;
    
    IOLog("%s: opening hardware\n", [self name]);
    if (![self initRing])
    {
	IOLog("%s: Error allocating ring buffers\n", [self name]);
	return nil;
    } /* if */

    irtn = IOPhysicalFromVirtual(IOVmTaskSelf(), (vm_address_t)lp->dd, &paddr);
    if (irtn = IO_R_SUCCESS)
	IOLog("%s: Error getting paddr: %d\n", [self name], irtn);

    outl(paddr, ioaddr + VMXNET_INIT_ADDR);
    outl(lp->dd->length, ioaddr + VMXNET_INIT_LENGTH);

    lp->dd->debugLevel = 0;
    lp->dd->txStopped = FALSE;
    lp->devOpen = TRUE;
    
    return self;
} /* open */



/*--------------------------------- initRing --------------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- initRing
{
    Vmxnet2_DriverData	*dd=lp->dd;
    int		i;
    int32	offset;
    IOReturn	irtn;

    offset = sizeof(*dd);

    dd->rxRingLength = lp->numRxBuffers;
    dd->rxRingOffset = offset;
    lp->rxRing = (Vmxnet2_RxRingEntry *)((uint32)dd + offset);
    offset += lp->numRxBuffers * sizeof(Vmxnet2_RxRingEntry);
    
    // dummy rxRing2
    dd->rxRingLength2 = 1;
    dd->rxRingOffset2 = offset;
    offset += sizeof(Vmxnet2_RxRingEntry);
    
    dd->txRingLength = lp->numTxBuffers;
    dd->txRingOffset = offset;
    lp->txRing = (Vmxnet2_TxRingEntry *)((uint32)dd + offset);
    offset += lp->numTxBuffers * sizeof(Vmxnet2_TxRingEntry);
    
    IOLog("%s: vmxnet_init_ring: offset=%d length=%d\n", [self name], 
    		offset, dd->length);
		
    for (i = 0; i < lp->numRxBuffers; i++)
    {
	lp->rxSkbuff[i] = IOAlignedMalloc(PKT_BUF_SZ);
	if (lp->rxSkbuff[i] == NULL)
	{
				/* there is not much we can do at this point */
	    IOLog("%s: vmxnet_init_ring IOMalloc failed.\n", [self name]);
	    return nil;
	} /* if */
	lp->rxRing[i].driverData = lp->rxSkbuff[i];
	irtn = IOPhysicalFromVirtual(IOVmTaskSelf(), 
					(vm_address_t)lp->rxRing[i].driverData,
					&(lp->rxRing[i].paddr));
	if (irtn = IO_R_SUCCESS)
	    IOLog("%s: Error getting paddr: %d\n", [self name], irtn);

	lp->rxRing[i].bufferLength = le16_to_cpu(PKT_BUF_SZ);
	lp->rxRing[i].actualLength = 0;
	lp->rxRing[i].ownership = VMXNET2_OWNERSHIP_NIC;
    } /* if */
    
				/* dummy rxRing2 tacked on to the end, with a
				   single unusable entry                     */
    lp->rxRing[i].paddr = 0;
    lp->rxRing[i].bufferLength = 0;
    lp->rxRing[i].actualLength = 0;
    lp->rxRing[i].driverData = 0;
    lp->rxRing[i].ownership = VMXNET2_OWNERSHIP_DRIVER;

    dd->rxDriverNext = 0;
    
    for (i = 0; i < lp->numTxBuffers; i++)
    {
	lp->txRing[i].driverData = IOAlignedMalloc(PKT_BUF_SZ);
	if (lp->txRing[i].driverData == NULL)
	{
				/* there is not much we can do at this point */
	    IOLog("%s: vmxnet_init_ring IOMalloc failed.\n", [self name]);
	    return nil;
	} /* if */
	irtn = IOPhysicalFromVirtual(IOVmTaskSelf(), 
					(vm_address_t)lp->txRing[i].driverData,								    
					&(lp->txRing[i].sg.sg[0].addrLow));
	if (irtn = IO_R_SUCCESS)
	    IOLog("%s: Error getting paddr: %d\n", [self name], irtn);
	    
	lp->txRing[i].sg.length = 0;
	lp->txRing[i].sg.sg[0].length = 0;
	lp->txRing[i].sg.sg[0].addrHi = 0;
	lp->txRing[i].ownership = VMXNET2_OWNERSHIP_DRIVER;
    } /* for */

    dd->txDriverCur = dd->txDriverNext = 0;
    dd->savedRxNICNext = dd->savedRxNICNext2 = dd->savedTxNICNext = 0;
    dd->txStopped = FALSE;

    return self;
} /* initRing */



/*------------------------------- checkTxQueue ------------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- checkTxQueue
{
    Vmxnet2_DriverData	*dd=lp->dd;
    
    while (1)
    {
	Vmxnet2_TxRingEntry *xre = &lp->txRing[dd->txDriverCur];
	if (xre->ownership != VMXNET2_OWNERSHIP_DRIVER || 
	    xre->sg.length == 0) 
	{
	    break;
	} /* if */

				/* Update the statistics.                    */
	[network incrementOutputPackets];
	xre->sg.length = 0;
	lp->numTxPending--;

				/* Clear the transmission timeout            */
	if (lp->numTxPending <= 0)
	    [self clearTimeout];

	if (dd->debugLevel > 0)
	{
	    IOLog("%s: check_tx_queue: returned packet, numTxPending %d "
	    	"next %d cur %d\n", [self name], 
		lp->numTxPending, dd->txDriverNext, dd->txDriverCur);
	}

	VMXNET_INC(dd->txDriverCur, dd->txRingLength);
	dd->txStopped = FALSE;

				/* Restart transmitting packets.             */
	transmitStopped = NO;
    } /* while */

    return self;
} /* checkTxQueue */



/*---------------------------------- close ----------------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

- close
{
    int			i;
    unsigned long	flags;
    
    IOLog("%s: closing hardware\n", [self name]);
    [self disableAllInterrupts];
    
    lp->devOpen = FALSE;
    spin_lock_irqsave(&lp->txLock, flags);
    
    if (lp->numTxPending > 0)
    {
	//Wait absurdly long (2sec) for all the pending packets to be returned.
	IOLog("%s: vmxnet_close: Pending tx = %d\n", 
			[self name], lp->numTxPending); 
			
	for (i = 0; i < 200 && lp->numTxPending > 0; i++)
	{
	    outl(VMXNET_CMD_CHECK_TX_DONE, ioaddr + VMXNET_COMMAND_ADDR);
	    IODelay(10000);
	    [self checkTxQueue];
	} /* for */

	// This is possiblly caused by a faulty physical driver. 
	// Will go ahead and free these skb's anyways (possibly dangerous,
	// but seems to work in practice)
	if (lp->numTxPending > 0)
	{
	    IOLog("%s (vmxnet_close): failed to finish all pending tx.\n"
		"This virtual machine may be in an inconsistent state.\n",
		[self name]);
	    lp->numTxPending = 0;
	} /* if */
    } /* if */
    spin_unlock_irqrestore(&lp->txLock, flags);

				/* Set driver data in HW address to NULL     */
    outl(0, ioaddr + VMXNET_INIT_ADDR);

				/* Free the ring buffers                     */
    for (i = 0; i < lp->dd->txRingLength; i++)
    {
	if (lp->txRing[i].driverData != NULL)
	{
	    IOAlignedFree(lp->txRing[i].driverData, PKT_BUF_SZ);
	    lp->txRing[i].driverData = NULL;
	} /* if */
	lp->txRing[i].sg.length = 0;
	lp->txRing[i].sg.sg[0].addrLow = 0;
    } /* for */

    for (i = 0; i < lp->numRxBuffers; i++)
    {
	if (lp->rxSkbuff[i] != NULL)
	{
	    IOAlignedFree(lp->rxSkbuff[i], PKT_BUF_SZ);
	    lp->rxSkbuff[i] = NULL;
	} /* if */
	lp->rxRing[i].driverData = NULL;
    } /* for */
    
    return self;
} /* close */



@end


/*-------------------------------- copyAddr() -------------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

static inline void copyAddr(enet_addr_t *from, enet_addr_t* to)
{
    IOCopyMemory((void*)from, (void*)to, sizeof(enet_addr_t), sizeof(char));
} /* copyAddr() */



/*-------------------------------- zeroAddr() -------------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

static inline void zeroAddr(enet_addr_t *addr)
{
    addr->ether_addr_octet[0] = 0;
    addr->ether_addr_octet[1] = 0;
    addr->ether_addr_octet[2] = 0;
    addr->ether_addr_octet[3] = 0;
    addr->ether_addr_octet[4] = 0;
    addr->ether_addr_octet[5] = 0;
} /* zeroAddr() */



/*------------------------------ compareAddr() ------------------------------*\
*									      *
*	«Text»
*									      *
\*---------------------------------------------------------------------------*/

static inline int compareAddr(enet_addr_t *a1, enet_addr_t *a2)
{
    return (a1->ether_addr_octet[0] == a2->ether_addr_octet[0] &&
            a1->ether_addr_octet[1] == a2->ether_addr_octet[1] &&
            a1->ether_addr_octet[2] == a2->ether_addr_octet[2] &&
            a1->ether_addr_octet[3] == a2->ether_addr_octet[3] &&
            a1->ether_addr_octet[4] == a2->ether_addr_octet[4] &&
            a1->ether_addr_octet[5] == a2->ether_addr_octet[5]);
} /* compareAddr() */


