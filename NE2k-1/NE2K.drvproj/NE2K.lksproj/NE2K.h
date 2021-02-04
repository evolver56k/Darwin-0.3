/* NE2K.h created by root on Thu 07-May-1998 */


#import <driverkit/IOEthernet.h>
#import <driverkit/i386/directDevice.h>


/* multicast entry */

#define	MAR_MAX	32	//??

struct mar_entry {
    BOOL	valid;
    enet_addr_t	addr;
};



@interface NE2K:IOEthernet
{
    IOEISAPortAddress	base;		/* port base 			     */
    IORange		port;		/* port base and extent		     */
    int			irq;		/* interrupt			     */
    enet_addr_t		myAddress;	/* local copy of ethernet address    */
    IONetwork		*network;	/* handle to kernel network object   */
    id			transmitQueue;	/* transmit queue */
    
/* NS8390 headers */

    struct mar_entry	mar_list[MAR_MAX];	/* multicast addres list */
    int			mar_cnt;	/* multicast address list count */
    unsigned char	mcfilter[8];	/* multicast filter */

    BOOL		tbusy;		/* ? */
    BOOL		word16;		/* 16bit mode */
    BOOL		transmitActive;		/* Transmit active */
    BOOL		irqlock;	/* 8390's intrs disabled when YES */
    BOOL		dmaing;		/* Remote DMA active */
    unsigned char	tx_start_page;
    unsigned char	rx_start_page;
    unsigned char	stop_page;
    unsigned char	current_page;	/* Read pointer in buffer */
    unsigned char	txqueue;	/* Tx Packet buffer queue length */
    short		tx1, tx2;	/* Packet lengths for ping-pong tx. */
    short		lasttx;		/* Alpha version consistency check */

/* own headers */
    int			vendor;		/* NE2000 vendor id */
    int			debug;		/* debug level flag; 0=off .. */

    BOOL		promMode;
    BOOL		multiMode;
}


+ (BOOL)probe:(IODeviceDescription *)devDesc;

- initFromDeviceDescription:(IODeviceDescription *)devDesc;
- free;

- (IOReturn)enableAllInterrupts;
- (void)disableAllInterrupts;
- (BOOL)resetAndEnable:(BOOL)enable;
- (void)timeoutOccurred;
- (void)interruptOccurred;


-(void) _transmitError;
-(void) _transmitInterruptOccurred;
-(void) _receiveInterruptOccurred;
-(void) _receiveOverrun;

-(void) addMulticastAddress:(enet_addr_t *) address;
-(void) removeMulticastAddress:(enet_addr_t *) address;

- (BOOL)enablePromiscuousMode;
- (void)disablePromiscuousMode;
- (BOOL)enableMulticastMode;
- (void)disableMulticastMode;

- (void)transmit:(netbuf_t)pkt;

-(void) _NS8390TriggerSend:(unsigned int) len startPage:(int)start_page;
-(void) _get8390hdr:(struct e8390_pkt_hdr *) hdr ringPage:(int) ring_page;
-(void) _blockInput:(int) count buffer:(char *) buf offset:(int) ring_offset;
-(void) _blockOutput:(int) count buffer:(char *) buf start:(int) start_page;
-(void) _NS8390Reset;
-(void) _NS8390Init:(BOOL) startp;

-(void) _setMulticastList;

@end
