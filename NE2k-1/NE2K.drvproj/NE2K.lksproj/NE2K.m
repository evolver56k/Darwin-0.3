/* NE2K.m created by root on Thu 07-May-1998 */

#define MACH_USER_API	1

#import <driverkit/generalFuncs.h>
#import <driverkit/IONetbufQueue.h>
#import <driverkit/i386/PCI.h>
#import <driverkit/i386/IOPCIDeviceDescription.h>
#import <driverkit/i386/IOPCIDirectDevice.h>



#import "ne2k_reg.h"
#import "ne2k_funcs.h"
#import "NE2K.h"


#import <kernserv/kern_server_types.h>
#import <kernserv/clock_timer.h>
#import <kernserv/prototypes.h>


//#define PINGPONG	1


#define NE_TXTIMEOUT	3000


//8k + 8k if 16 bit
#define NESM_START_PAGE	0x40
//16k .. end
#define NESM_STOP_PAGE	0x80


#define	MAX_SERVICE	12

#define	TX_2X_PAGES	12
#define	TX_1X_PAGES	6
#define	ETHER_ADDR_LEN	6

#ifdef PINGPONG
    #define	TX_PAGES	TX_2X_PAGES
#else
    #define	TX_PAGES	TX_1X_PAGES
#endif

struct e8390_pkt_hdr {
    unsigned char	status;	/* status */
    unsigned char	next;	/* pointer to next packet */
    unsigned short	count;	/* header + packet length in bytes */
};



#define	HARD_RESET	outb(base + NE_RESET, inb(base + NE_RESET))
#define	NS_TIMEOUT	2*1000*1000







static inline unsigned int	update_crc(unsigned char byte, unsigned int current_crc);

static inline void copyAddr(enet_addr_t *from, enet_addr_t* to);
static inline void zeroAddr(enet_addr_t *addr);
static inline int compareAddr(enet_addr_t *a1, enet_addr_t *a2);





@implementation NE2K:IOEthernet


+ (BOOL)probe:(IODeviceDescription *)devDesc
{
    NE2K	*driver = [self alloc];
    
    IOLog("NE2000 Generic Driver v0.9.1b\n");
    IOLog("by Gabor Sebestyen\n\n");

    return [driver initFromDeviceDescription:devDesc] != nil;
}


- initFromDeviceDescription:(IODeviceDescription *)devDesc
{
    IOEISADeviceDescription *deviceDescription = (IOEISADeviceDescription *)devDesc;

    IOPCIConfigSpace		config;
    int				i;
    unsigned char		*_ptr;
    ns_time_t			start_time, current;
    unsigned char		SA_prom[32];	/* misc */
    BOOL			SA_doubled;






    if ([IODirectDevice getPCIConfigSpace: &config withDeviceDescription: devDesc] != IO_R_SUCCESS) {
        IOLog("Can't get config space...\n");
        [self free];
        return nil;
    }

    if (config.VendorID == 0x10ec && config.DeviceID == 0x8029) {
        IOLog("Vendor: RealTek RTL-8029\n");
        vendor = 1;
    } else if (config.VendorID == 0x1050 && config.DeviceID == 0x0940) {
        IOLog("Vendor: Winbond 89C940\n");
	vendor = 2;
    } else if (config.VendorID == 0x11f6 && config.DeviceID == 0x1401) {
        IOLog("Vendor: Compex RL2000\n");
	vendor = 3;
    } else if (config.VendorID == 0x8e2e && config.DeviceID == 0x3000) {
        IOLog("Vendor: KTI ET32P2\n");
	vendor = 4;
    } else if (config.VendorID == 0x4a14 && config.DeviceID == 0x5000) {
        IOLog("Vendor: NetVin NV5000SC\n");
	vendor = 5;
    } else if (config.VendorID == 0x1106 && config.DeviceID == 0x0926) {
        IOLog("Vendor: Via 82C926\n");
	vendor = 6;
    } else {
        IOLog("Vendor: unknown (V:0x%x; D:0x%x)...\n", (unsigned int)config.VendorID, (unsigned int)config.DeviceID);
	vendor = 0;
    }

    irq = (int)config.InterruptLine;
    base = ((IOEISAPortAddress)config.BaseAddress[0]) & 0xfffe;
    port.start = base;
    port.size = 0x20;
    
    
    IOLog("BASE: 0x%x; IRQ: %d\n", base, irq);


    [deviceDescription setInterruptList: &irq	num:1];
    [deviceDescription setPortRangeList: &port	num: 1];

    
    if ([super initFromDeviceDescription:devDesc] == nil)
        return nil;

    
    debug = 0;    

    /*
     * Reset NE2000
     */
    if (debug) {
        IOLog("Resetting ...\n");
    }
    HARD_RESET;


    IOGetTimestamp(&start_time);
    //rst
    while (NICGetISR(base) & 0x80 == 0) {
        IOGetTimestamp(&current);
        if (current - start_time > NS_TIMEOUT) {
            IOLog("Card failure (no reset ack)!..\n");
            [self free];
            return nil;
        }
    }



    
    /*
     * INIT DEVICE
     */
    if (debug) {
        IOLog("Init card ...\n");
    }

    NICSetCR(base, 0x21);	//no dma, stop, page0
    NICSetDCR(base, 0x49);	//word16, lb - normal mode

    NICSetRBCR(base, 0);	//Remote byte count reg

    NICSetIMR(base, 0);		//Mask completion
    NICSetISR(base, 0xFF);	//Mask completion

    NICSetRCR(base, 0x20);	//monitor

    NICSetTCR(base, 0x02);	//int. loopback

    NICSetRBCR(base, 0x20);	//Remote byte count reg
    NICSetRSAR(base, 0);	//Remote start address

    NICSetCR(base, 0x8 | 0x2);	//remote read, start


    /* read SA_prom */
    /* TODO ... */
    SA_doubled = YES;
    for (i=0; i<32; i+=2) {
        SA_prom[i] = NICGetCharData(base);
        SA_prom[i+1] = NICGetCharData(base);
        if (SA_prom[i] != SA_prom[i+1]) {
            SA_doubled = NO;
        }
    }
    if (SA_doubled == YES) {
        for (i=0; i<16; i++) {
            SA_prom[i] = SA_prom[i+i];
        }
    }



    NICSetDCR(base, 0x49);	//lb - normal mode, word16


    /*
     * Set pages
     */
    tx_start_page = NESM_START_PAGE;
    stop_page = NESM_STOP_PAGE;
    rx_start_page = tx_start_page + TX_PAGES;




    /* get phys address */
/*
    NICSetCR(base, 0x61);	//nodma, page1, stop
    NICGetPAR(base, (unsigned char *)&myAddress);
    NICSetCR(base, 0x21);	//nodma, page0, stop
*/
    _ptr = (unsigned char *)&myAddress;
    for (i=0; i<6; i++) {
        _ptr[i] = SA_prom[i];
    }

    /* set multicast list */
    multiMode = NO;
    promMode = NO;

    mar_cnt = 0;	//list is empty
    for (i=0; i<MAR_MAX; i++) {
        mar_list[i].valid = NO;
        zeroAddr(&(mar_list[i].addr));
    }



    
    word16 = YES;	//16bit mode

    txqueue	= 0;
    
    [self resetAndEnable:NO];	
    transmitQueue = [[IONetbufQueue alloc] initWithMaxCount:32];

    network = [super attachToNetworkWithAddress:myAddress];

    IOLog("Device inited...\n");
    return self;
}

- free
{
    [transmitQueue free];
    [self _NS8390Init: NO];

    return [super free];
}





- (IOReturn)enableAllInterrupts
{
    NICSetIMR(base, 0x3f);	//All irqs!
    return [super enableAllInterrupts];
}

- (void)disableAllInterrupts
{
    NICSetIMR(base, 0x0);	//None irqs!
    [super disableAllInterrupts];
}




- (BOOL)resetAndEnable:(BOOL)enable
{
    [self disableAllInterrupts];


    [self _NS8390Reset];
    [self _NS8390Init: enable];        

    
    irqlock	= NO;
    //getStationAddress(&myAddress, base);	???

    if (enable && [self enableAllInterrupts] != IO_R_SUCCESS) {
        IOLog("NE2K: resetAndEnable: intterupts not allowed ..\n");
        [self setRunning:NO];
        return NO;
    }

    [self setRunning:enable];
    return YES;
}





/* transmit timeout Occurred */

- (void)timeoutOccurred
{
    netbuf_t	pkt = NULL;

    if (debug) {
        IOLog("TX/TO\n");
    }

    transmitActive = NO;

    if ([self isRunning]) {
        if ([self resetAndEnable:YES]) {
            if (pkt = [transmitQueue dequeue])
                [self transmit:pkt];
        }

    }
    /*
     * Value of [self isRunning] may have been modified by
     * resetAndEnable:
     */
    if (![self isRunning]) {	
        /*
         * Free any packets in the queue since we're not running.
         */

        if ([transmitQueue count]) {
            while (pkt = [transmitQueue dequeue])
                nb_free(pkt);
        }

    }
}






/* General Interrupt Head */

- (void)interruptOccurred
{
    int		nr_serviced = 0;
    unsigned	char	ni;

    NICSetCR(base, 0x20);	//no dma, page0


    while ( (ni = NICGetISR(base)) && ++nr_serviced < MAX_SERVICE) {
        //Ack. all incoming interrupts ...
        //NICSetISR(base, ni);
        
        if ([self isRunning] == NO) {
            IOLog("NE2K: Interrupts from stopped card!\n");
            break;
        }

        //Receive buffer exhausted
        if (ni & 0x10 /* _r.isr.ovw */) {
            //ei_rx_overrun
            [self _receiveOverrun];
        } else if (ni & 0x5 /* _r.isr.prx || _r.isr.rxe */) {
            [self _receiveInterruptOccurred];
        }
	
        if (ni & 0x2 /* _r.isr.ptx */) {
            [self _transmitInterruptOccurred];
        } else if (ni & 0x8 /* _r.isr.txe */) {
            [self _transmitError];
        }

        //counters set
        if (ni & 0x20 /*_r.isr.cnt */) {
/*
            rx_frame_errors	+= NICGetCNTR0(base);
            rx_crc_errors	+= NICGetCNTR1(base);
            rx_missed_errors	+= NICGetCNTR0(base);
 */
            (void)NICGetCNTR0(base);
            (void)NICGetCNTR1(base);
            (void)NICGetCNTR2(base);
        }

        /* ignore any RDC interrupts that make back to here */
        if (ni & 0x40 /* _r.isr.rdc */) {
            NICSetISR(base, 0x40);	//rdc = 0x40
        }

        NICSetCR(base, 0x22);		//nodma, page0, start
    }

    if (ni && debug==YES) {
        IOLog("NE2K: too much work at interrupt.\n");
    }

    //DriverKit gave me this advice:
    [self enableAllInterrupts];
}



/* from interrupt handler */

-(void) _transmitError
{
    RCNV	_r;
    BOOL	tx_was_aborted;

    _r.data	= NICGetTSR(base);
    if (_r.tsr.abort) {
        tx_was_aborted = YES;
    } else {
        tx_was_aborted = NO;
    }

    if (debug) {
        IOLog("NE2K: Transmitter error (%2x): ", _r.data);
        if (_r.tsr.abort)
            IOLog("excess-collision ");
        if (_r.tsr.nd)
            IOLog("non-deferral ");
        if (_r.tsr.crs)
            IOLog("lost-carrier ");
        if (_r.tsr.under)
            IOLog("FIFO-underrun ");
        if (_r.tsr.cdh)
            IOLog("lost-heartbeat ");
	
        IOLog("\n");
    }


    NICSetISR(base, 0x8);	//tx_err ack.

    if (tx_was_aborted == YES) {
        [self _transmitInterruptOccurred];
    } else {
        //Bang ...
        [network incrementOutputErrors];
        //TODO: gathering crs, cfh, owc errors (error++)
    }

}




-(void) _transmitInterruptOccurred
{
    RCNV		status;
    netbuf_t		pkt;

    status.data = NICGetTSR(base);

    [self clearTimeout];


    NICSetISR(base, 0x02);	//Ack. transmitter, no error

    /* ping-pong */

    /*
     * There are two Tx buffers, see wich one finished, and trigger
     * the send of another one if it exists.
     */

#ifdef PINGPONG
    txqueue--;
    if (tx1 < 0) {
        if (lasttx != 1 && lasttx != -1) {
            IOLog("NE2K: bogus last_tx_buffer %d, tx1=%d.\n", lasttx, tx1);
        }
        tx1=0;
        tbusy = NO;
        if (tx2 > 0) {
            transmitActive = YES;
            [self _NS8390TriggerSend: tx2 startPage: tx_start_page + 6];
            [self setRelativeTimeout:NE_TXTIMEOUT];

            tx2 = -1;
            lasttx = 2;
        } else {
            lasttx = 20;
            transmitActive = NO;
        }
    } else if (tx2 < 0) {
        if (lasttx != 2 && lasttx != -2) {
            IOLog("NE2K: bogus last_tx_buffer %d, tx2=%d.\n", lasttx, tx2);
        }
        tx2=0;
        tbusy = NO;
        if (tx1 > 0) {
            transmitActive = YES;
            [self _NS8390TriggerSend: tx1 startPage: tx_start_page];
            [self setRelativeTimeout:NE_TXTIMEOUT];

            tx1 = -1;
            lasttx = 1;
        } else {
            lasttx = 10;
            transmitActive = NO;
        }
    } else if (pkt = [transmitQueue dequeue]) {
        [self transmit: pkt];
    }
#else /* PINGPONG */

    transmitActive = NO;
    tbusy = NO;

    if (pkt = [transmitQueue dequeue]) {
        [self transmit: pkt];
    }

#endif /* PINGPONG */



    /* Minimize Tx latency: update the statistics after we restart TXing. */

//    if (status.tsr.col)
        //collisions ++
    if (status.tsr.ptx)
        [network incrementOutputPackets];
    else {
        [network incrementOutputErrors];
        //Here are a lot of errors go....
    }
     
}



-(void) _receiveInterruptOccurred
{
    unsigned char	rxing_page, this_frame, next_frame;
    unsigned short	current_offset;
    int			rx_pkt_count = 0;
    struct e8390_pkt_hdr	rx_frame;
    int			num_rx_pages = stop_page - rx_start_page;

    netbuf_t		pkt;


    while (++rx_pkt_count < 10) {
        int pkt_len, pkt_stat;
        
        /* Get the rx page (incoming paket pointer) */
        NICSetCR(base, 0x20 | 0x40);	//page1, no dma
        rxing_page = NICGetCURR(base);
	NICSetCR(base, 0x20);		//page0, no dma

        /* Remove one frame from the ring. Boundary is always a page behind */
        this_frame = NICGetBNRY(base) + 1;
        if (this_frame >= stop_page) {
            this_frame = rx_start_page;
        }


        /* Someday we'll omit the previous, if we never get this message */

        if (debug && this_frame != current_page) {
            IOLog("NE2K: Mismatched readpage pointers %2x vs %2x.\n", this_frame, current_page);
        }


        if (this_frame	 == rxing_page)	/* Read all the frames? */
            break;			/* Done for now */


        current_offset = this_frame << 8;
        [self _get8390hdr: &rx_frame ringPage: this_frame];

        pkt_len = rx_frame.count - sizeof(struct e8390_pkt_hdr);
        pkt_stat = rx_frame.status;

        next_frame = this_frame + 1 + ((pkt_len+4)>>8);

        /* Check for bogosity warned by 3c503 book: the status byte is never
            written. This happened a lot during testing! This code should be cleaned up someday */
        if (rx_frame.next != next_frame
            && rx_frame.next != next_frame + 1
            && rx_frame.next != next_frame - num_rx_pages
            && rx_frame.next != next_frame + 1 - num_rx_pages) {
            current_page = rxing_page;
            NICSetBNRY(base, current_page - 1);

            [network incrementInputErrors];
            continue;
        }

        if (pkt_len < 60 || pkt_len > 1518) {
            if (debug) {
                IOLog("NE2K: bogus packet size:%d, status=%2x npg=%2x.\n", rx_frame.count, rx_frame.status, rx_frame.next);
                [network incrementInputErrors];
                //rx_length_errors++;
            }
        } else if ((pkt_stat & 0x0f) == 0x01 /* RSR RXOK = good packet */) {
            pkt	= nb_alloc(pkt_len);	/* Allocate a new empty packet */

            if (pkt == NULL) {
                if (debug) {
                    IOLog("NE2K: Couldn't allocate new packet for size: %d.\n", pkt_len);
                }
            } else {
                [self _blockInput: pkt_len buffer: nb_map(pkt) offset: current_offset + sizeof(rx_frame)];

                //Simply handle new packet ..
/*
                if(promMode == NO && [super isUnwantedMulticastPacket:(ether_header_t *)nb_map(pkt)]) {
                    IOLog("Unwanted packet...\n");
                    nb_free(pkt);
                } else {
                    [network handleInputPacket:pkt extra:0];
                } 
 */
                [network handleInputPacket:pkt extra:0];
            }
        } else {
            if (debug) {
                IOLog("NE2K: bogus packet: status=%2x nxpg=%2x size=%d.\n", rx_frame.status, rx_frame.next, rx_frame.count);
            }
            [network incrementInputErrors];
            /* NB: The NIC counts CRC, frame and missed errors */
            if (pkt_stat & 0x08 /* RSR FO - FIFO overrun */ ) {
                //rx_fifo_errors++
            }
        }


        
        next_frame = rx_frame.next;

        /* This should never happen: it's here for avoiding bad clones. */
        if (next_frame >= stop_page) {
            IOLog("NE2K: next frame inconsistency, %2x.\n", next_frame);
            next_frame = rx_start_page;
        }


        current_page = next_frame;
        NICSetBNRY(base, next_frame - 1);

    }

    
    /* We used to also ENISR_OVER here, but that would sometimes mask a real overrun, leaving the 8390 in a stopped state withe rec'vr off .*/
    NICSetISR(base, 0x1 | 0x4);	 /* ISR RX + ISR RX_ERR ack. */

}






/*
 * We have a receive overrun: we have to kick the 8390 to get it started
 * again. problem is that you have kick it exactly as NS prescribes in
 * the updated datasheets, or "the NIC may act in an unpredictable manner."
 * This includes causing "the NIC to defer indefinitely when it is stopped
 * on a busy network." Ugh.
 */

-(void) _receiveOverrun
{
    unsigned char	was_txing, must_resend = 0;
    unsigned	char _d;
    
    must_resend = NO;

    /*
     * Record whether a TX was in progress and then issue the stop command.
     */

    //_r.data = NICGetCR(base);
    _d = NICGetCR(base);
    was_txing = _d & 0x4;	/* cr.trans */

    NICSetCR(base, 0x21);	//no dma, stop, page0;

    if (debug) {
        IOLog("NE2K: Receiver overrun.\n");
    }
    //rx_over_errors++;
    [network incrementInputErrors];

    /*
     * Wait a full Tx time (1.2ms) + some guard time, NS says 1.6ms total.
     * Early datasheets said to poll the reset bit, but now they say that
     * it "is not a reliable indicator and subsequently should be ignored."
     * we wait at least 10ms.
     */
    IOSleep(10);	/* 10 millisecs */

    /*
     * Reset RBCR[01] back to zero as per magic incatation
     */
    NICSetRBCR(base, 0);

    /*
     * See if any Tx was interrupted or not. According to NS, this
     * step is vital, and skipping will cause no end of havoc.
     */
    if (was_txing) {
        _d = NICGetISR(base);
        /* Transmit error */
        if ((_d & 0xa) == 0 /* isr.txe + isr.ptx */) {
            must_resend = 1;
        }
    }

    /*
     * Have to enter loopback mode  an then restart the NIC before
     * you are allowed to slurp packets up off the ring.
     */
    NICSetTCR(base, 0x2);	//Transmitter OFF (internal loopback)
    NICSetCR(base, 0x22);	//nodma, page0, start

    /*
     * Clear the Rx ring of all the debris, and ack the interrupt.
     */
    [self _receiveInterruptOccurred];

    NICSetISR(base, 0x10);	//OVW - overrun ...

    /*
     * Leave loopback mode, and resend any packet that got stopped.
     */
    NICSetTCR(base, 0x0);	//TxCONFIG - Normal transmit mode ...
    if (must_resend) {
        NICSetCR(base, 0x26);	//no dma, trans, start, page0
    }

}





/* multicast addresses */

-(void) addMulticastAddress:(enet_addr_t *) address
{
    int i;

    if (mar_cnt == MAR_MAX) {
        IOLog("NE2K: multicast address list is full!\n");
        return;
    }

    for (i=0; i<MAR_MAX; i++) {
        if (mar_list[i].valid == NO) {
            mar_list[i].valid = YES;
            copyAddr(address, &(mar_list[i].addr));
            mar_cnt++;

            [self _setMulticastList];
            break;
        }
    }
}


-(void) removeMulticastAddress:(enet_addr_t *) address
{
    int i;

    if (mar_cnt == 0) {
        //list is empty!
        return;
    }

    for (i=0; i<MAR_MAX; i++) {
        if (mar_list[i].valid == YES && compareAddr(address, &(mar_list[i].addr))) {
            mar_list[i].valid = NO;
            zeroAddr(&(mar_list[i].addr));
            mar_cnt--;

            [self _setMulticastList];
            break;
        }
    }
}







- (BOOL)enablePromiscuousMode
{
    if (promMode == NO) {
        promMode = YES;
        [self _setMulticastList];
    }

    return YES;
}

- (void)disablePromiscuousMode
{
    if (promMode == YES) {
        promMode = NO;
        [self _setMulticastList];
    }
}

- (BOOL)enableMulticastMode
{

    if (multiMode == NO) {
        multiMode = YES;
        [self _setMulticastList];
    }
    
    return YES;
}

- (void)disableMulticastMode
{
    if (multiMode == YES) {
        multiMode = NO;
        [self _setMulticastList];
    }
}





- (void)transmit:(netbuf_t)pkt
{
    int		send_length, output_page;
    int		pkt_len;

    if (tbusy == YES) {
        [transmitQueue enqueue:pkt];
    } else {
        /* Mask interrupts from the ethercard. */

        //Disable irq
        NICSetIMR(base, 0x00);
        [self disableInterrupt: irq];
        irqlock = YES;

        pkt_len = nb_size(pkt);
        send_length = pkt_len > 60 ? pkt_len : 60;	//Linux: ETH_ZLEN = 60

        /* PING_PONG */
#ifdef PINGPONG
	/*
         * We have two Tx slots available for use. Find the first free
         * slot, and then perform sanity checks. With two Tx bufs,
         * you get very close to transmitting back-to-back packets. With
         * only one Tx buf, the transmitter sits idle while you reload the
         * card, leaving a substantial gap between each transmitted paket.
         */

        if (tx1 == 0) {
            output_page	= tx_start_page;
            tx1		= send_length;
        } else if (tx2 == 0) {
            output_page	= tx_start_page + TX_1X_PAGES;
            tx2		= send_length;
        } else {
            irqlock	= NO;
            tbusy	= YES;
            NICSetIMR(base, 0x3f);	//All irqs!
            [self enableInterrupt: irq];

            //No ping-pong queue, queue packet ...
            [transmitQueue enqueue: pkt];
            return;
        }


	/* Okay, now upload the packet and trigger a send if the transmitter
	 * isn't already sending. If it is busy, the interrupt handler will
	 * trigger the send later, upon receiving a Tx done interrupt.
         */

        [self _blockOutput: nb_size(pkt) buffer: nb_map(pkt) start: output_page];
        if (transmitActive == NO) {
            transmitActive = YES;
            [self _NS8390TriggerSend: send_length startPage: output_page];

            //start transmit timeout ...
            [self setRelativeTimeout:NE_TXTIMEOUT];

            if (output_page == tx_start_page) {
                tx1	= -1;
                lasttx	= -1;
            } else {
                tx2	= -1;
                lasttx	= -2;
            }
        } else
            txqueue++;

        tbusy = (tx1 && tx2 ? YES : NO);
#else /* PINGPONG */
        [self _blockOutput: pkt_len buffer: nb_map(pkt) start: tx_start_page];
        transmitActive = YES;
        [self _NS8390TriggerSend: send_length startPage: tx_start_page];

        //start transmit timeout ...
        [self setRelativeTimeout:NE_TXTIMEOUT];
        tbusy = YES;
        
#endif

 
        /* Turn 8390 interrupts back on. */
        [self enableInterrupt: irq];
        irqlock	= NO;
        NICSetIMR(base, 0x3f);	//All irqs!

        /* free packet */
        nb_free(pkt);
    }
}









/* private methods */





/* Trigger a transmit start, assuming the length is valid */

-(void) _NS8390TriggerSend:(unsigned int) len startPage:(int)start_page
{
    RCNV	_r;
    
    NICSetCR(base, 0x20);		//nodma, page0

    _r.data = NICGetCR(base);
    if (_r.cr.txp == 1) {
        IOLog("NE2K._NS8390TriggerSend:: called with the transmitter busy.\n");
        return;
    }

    NICSetTPSR(base, start_page);
    NICSetTBCR(base, len);

    NICSetCR(base, 0x26);	//no dma, trans, start
}





/* base level */

-(void) _get8390hdr:(struct e8390_pkt_hdr *) hdr ringPage:(int) ring_page
{
    /* this shouldn't happen. If it does, it's the last thing you'll see */
    if (dmaing == YES) {
        IOLog("NE2K: DMAing conflict in _get8390hdr:ringPage:!\n");
        return;
    }


    dmaing = YES;
    NICSetCR(base, 0x22);	//no dma, start, page0


    
    NICSetRBCR(base, sizeof(struct e8390_pkt_hdr));	//Remote byte count reg
    NICSetRSAR(base, ring_page << 8);			//Remote start address: on page boundary

    NICSetCR(base, 0x08 | 0x02);	//remote read, start

    //Get datas
    NICGetShortDatas(base, (void *)hdr, 2);

    NICSetISR(base, 0x40);	/* Ack intr. */

    
    dmaing = NO;

}





-(void) _blockInput:(int) count buffer:(char *) buf offset:(int) ring_offset
{
    int		li;

    /* this shouldn't happen. If it does, it's the last thing you'll see */
    if (dmaing == YES) {
        IOLog("NE2K: DMAing conflict in _blockInput:buffer:offset:\n");
        return;
    }


    dmaing = YES;
    NICSetCR(base, 0x22);	//no dma, page0. start


    NICSetRBCR(base, count);		//Remote byte count reg
    NICSetRSAR(base, ring_offset);	//Remote start address: on page boundary

    NICSetCR(base, 0x08 | 0x02);	//remote read, start


    li = count >> 1;

    NICGetShortDatas(base, (void *)buf, li);

    if (count & 1) {
	buf[count-1] = NICGetCharData(base);
    }

    //TODO long-mode ...
    NICSetISR(base, 0x40);	/* Ack intr. */

    
    dmaing = NO;

}





-(void) _blockOutput:(int) count buffer:(char *) buf start:(int) start_page
{
    ns_time_t		start_time;
    ns_time_t		current;
    int			li;

    
    /* On little-endian it's safe to round the count up for word writes */
    if (count & 1)
        count++;


    /* this shouldn't happen. If it does, it's the last thing you'll see */
    if (dmaing == YES) {
        IOLog("NE2K: DMAing conflict in _blockOutput:buffer:start:\n");
        return;
    }


    dmaing = YES;

    NICSetCR(base, 0x22);	//no dma, start

    //NE_RW_BUGFIX
    //NICSetRBCR(base, 0x42);
    //NICSetRSAR(base, 0x42);
    //NICSetCR(base, 0x8 | 0x2);	//remote read, start
    //NE_RW_BUGFIX ...
    
    NICSetISR(base, 0x40);	//remote dma completed

    NICSetRBCR(base, count);		//Remote byte count reg
    NICSetRSAR(base, start_page << 8);	//Remote start address: on page boundary

    NICSetCR(base, 0x12);	//remote write, start


    li = count >> 1;

    NICSetShortDatas(base, (void *)buf, li);
    
    IOGetTimestamp(&start_time);
    //rdc
    while (NICGetISR(base) & 0x40 == 0) {
        IOGetTimestamp(&current);
        if (current - start_time > NS_TIMEOUT) {
            IOLog("NE2K: Timeout waiting for Tx RDC\n");
            [self _NS8390Reset];
            [self _NS8390Init: YES];

            break;
        }
    }
    NICSetISR(base, 0x40);	/* Ack intr. */

    dmaing = NO;

}





/*
 * Hard reset the card. This used to pause for the same period that a
 * 8390 reset command required, but that shouldn't be necessary.
 */


-(void) _NS8390Reset
{
    ns_time_t	start_time;
    ns_time_t	current;

    HARD_RESET;

    transmitActive = NO;
    dmaing = NO;

    IOGetTimestamp(&start_time);
    //rst
    while (NICGetISR(base) & 0x80 == 0) {
        IOGetTimestamp(&current);
        if (current - start_time > NS_TIMEOUT) {
            IOLog("NE2K: Reset did not complete.\n");
            break;
        }
    }


    NICSetISR(base, 0x80);

}


/* This page of functions should be 8390 generic */
/* Follow National Semi's recommendations for initializing the "NIC" */

-(void) _NS8390Init:(BOOL) startp
{
    NICSetCR(base, 0x21);		//nodma, page0, stop
    NICSetDCR(base, 0x49);		//word mode - preset word16!
    /* Clear the remote byte count registers. */
    NICSetRBCR(base, 0);
    /* Set to monitor and loopback mode -- this is vital! */
    NICSetRCR(base, 0x20);	//Rx OFF
    NICSetTCR(base, 0x02);	//Tx OFF
    /* Set the transmit page and receive ring */
    tx1 = 0;
    tx2 = 0;
    NICSetTPSR(base, tx_start_page);		//tx
    NICSetPSTART(base, rx_start_page);	//rx
    NICSetBNRY(base, stop_page-1);
    current_page = rx_start_page;
    NICSetPSTOP(base, stop_page);		//tx
    /* Clear the pending interrupts and masks */
    NICSetISR(base, 0xFF);
    NICSetIMR(base, 0x00);
    
    /* Copy the station address  */
    //cli
    NICSetCR(base, 0x61);	//nodma, page1, stop
    NICSetPAR(base, (unsigned char *)&myAddress);
    NICSetCURR(base, rx_start_page);
    NICSetCR(base, 0x21);	//nodma, page0, stop

    tbusy = NO;
    transmitActive	= NO;

    //interrupt = 0;

    tx1		= 0;
    tx2 	= 0;
    lasttx	= 0;	//post ins..

    if (startp == YES) {
        NICSetISR(base, 0xff);
        NICSetIMR(base, 0x3f);
        NICSetCR(base, 0x22);	//no dma, page0, start
        NICSetTCR(base, 0x0);	//transmit mode normal
        /* 3c503 TechMan says rxconfig only after the NIC is started */
        NICSetRCR(base, 0x4);	//rx on
        [self _setMulticastList];
    }
    
}






-(void) _setMulticastList
{
    int			i, g;
    unsigned int	crc;
    unsigned char	*_ptr;
    
    RCNV		_r;
    

    if (promMode == NO && multiMode == NO) {    
        for (i=0; i<8; i++)
            mcfilter[i] = 0x00;

        if (mar_cnt > 0) {
            //Make mc bits
            for (i=0; i<MAR_MAX; i++) {
                if (mar_list[i].valid == YES) {
                    crc = 0xffffffff;
                    _ptr = (unsigned char *)&mar_list[i].addr;
                    for (g=0; g<6; g++) {
                        crc = update_crc(_ptr[i], crc);
                    }
                    /*
                     * The 8390 uses the 6 most significant bits of the
                     * CRC to index the multicast table.
                     */ 
                    mcfilter[crc>>29] |= (1<<((crc>>26)&7));
                }
            }
        }

    } else {
        for (i=0; i<8; i++)
            mcfilter[i] = 0xff;
    }

    if ([self isRunning] == YES)
        NICSetRCR(base, 0x4);

    //cli
    NICSetCR(base, 0x20 | 0x40);//page1, no dma

    NICSetMAR(base, mcfilter);	//set filter

    NICSetCR(base, 0x20);	//no dma, pae0


    _r.data = 0;
    _r.rcr.broad = 1;	//RxCONFIG, broadcast enabled
    
    if (promMode == YES) {
	NICSetRCR(base, 0x4 | 0x8 | 0x10);
    } else if (multiMode == YES) {
        NICSetRCR(base, 0x4 | 0x8);
    } else {
        NICSetRCR(base, 0x4);
    }

    NICSetCR(base, 0x22);	//no dma, page0, start
}



@end


static inline unsigned int	update_crc(unsigned char byte, unsigned int current_crc)
{
    int			bit;
    unsigned char	ah = 0;


    for (bit=0; bit<8; bit++) {
        unsigned char	carry = (current_crc >> 31);
        current_crc <<= 1;
        ah = ((ah<<1) | carry) ^ byte;
        if (ah&1)
            current_crc ^= 0x04C11DB7;	/* crc polynomial */
        ah >>= 1;
        byte >>= 1;
    }

    return current_crc;
}


/* multicast ops */

static inline void copyAddr(enet_addr_t *from, enet_addr_t* to) {
    IOCopyMemory((void *) from, (void *)to, sizeof(enet_addr_t), sizeof(char));    
}

static inline void zeroAddr(enet_addr_t *addr) {
    addr->ether_addr_octet[0] = 0;
    addr->ether_addr_octet[1] = 0;
    addr->ether_addr_octet[2] = 0;
    addr->ether_addr_octet[3] = 0;
    addr->ether_addr_octet[4] = 0;
    addr->ether_addr_octet[5] = 0;
}


static inline int compareAddr(enet_addr_t *a1, enet_addr_t *a2) {
    return (a1->ether_addr_octet[0] == a2->ether_addr_octet[0] &&
            a1->ether_addr_octet[1] == a2->ether_addr_octet[1] &&
            a1->ether_addr_octet[2] == a2->ether_addr_octet[2] &&
            a1->ether_addr_octet[3] == a2->ether_addr_octet[3] &&
            a1->ether_addr_octet[4] == a2->ether_addr_octet[4] &&
            a1->ether_addr_octet[5] == a2->ether_addr_octet[5]);
}



