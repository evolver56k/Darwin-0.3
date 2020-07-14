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
 * Copyright (c) 1995-1996 NeXT Software, Inc.
 *
 * Hardware definitions and data structures for the DECchip 2104x
 *
 * HISTORY
 *
 * 26-Apr-95	Rakesh Dubey (rdubey) at NeXT 
 *	Created.
 * 07-Feb-96	Dieter Siegmund (dieter) at NeXT
 *	Crank down the size of netbufs we allocate to eliminate
 *	confusion.
 */

#define DEC_21X40_ALIGNMENT		4
#define DEC_21X40_DESCRIPTOR_ALIGNMENT	16	/* only long-word is needed */

#define TRANSMIT_QUEUE_SIZE		64

					// 1514 + 4 + 2(align to multiple of 4)
#define NETWORK_BUFSIZE			(ETHERMAXPACKET + ETHERCRC + 2)
#define NETWORK_BUFALIGNMENT		32

#define DEC_21X40_RX_RING_LENGTH	32
#define DEC_21X40_TX_RING_LENGTH	16

#define DEC_21X40_DESC_OWNED_BY_HOST	0
#define DEC_21X40_DESC_OWNED_BY_CTRL	1

#define DEC_21X40_REGISTER_SPACE	128

#define PCI_CBIO_OFFSET			0x10
#define PCI_CFCS_OFFSET			0x04

/*
 * Command and Status Register Offsets. 
 */
#define DEC_21X40_CSR0			0x00
#define DEC_21X40_CSR1			0x08
#define DEC_21X40_CSR2			0x10
#define DEC_21X40_CSR3			0x18
#define DEC_21X40_CSR4			0x20
#define DEC_21X40_CSR5			0x28
#define DEC_21X40_CSR6			0x30
#define DEC_21X40_CSR7			0x38
#define DEC_21X40_CSR8			0x40
#define DEC_21X40_CSR9			0x48
#define DEC_21X40_CSR10			0x50
#define DEC_21X40_CSR11			0x58
#define DEC_21X40_CSR12			0x60
#define DEC_21X40_CSR13			0x68
#define DEC_21X40_CSR14			0x70
#define DEC_21X40_CSR15			0x78

/*
 * Command and Status Register Definitions. 
 */
typedef struct csr0Struct {
    unsigned int
	    rsvd	:11,
            dbo         :1,             /* descriptor byte ordering */
	    tap		:3,		/* transmit auto polling */
	    das		:1,		/* diagnostic address space */
	    cal		:2,		/* cache alignment */
	    pbl		:6,		/* programmable burst length */
	    ble		:1,		/* big/little endian */
	    dsl		:5,		/* descriptor skip length */
	    bar		:1,		/* bus arbitration */
	    swr		:1;		/* software reset */
} busModeRegister_t;

typedef struct csr1Struct {
    unsigned int
	    rsvd	:31,
	    tpd		:1;		/* transmit poll demand */
} transmitPollDemand_t;

typedef struct csr2Struct{
    unsigned int
	    rsvd	:31,
	    rpd		:1;		/* receive poll demand */
} receivePollDemand_t;

typedef struct csr3Struct {
    unsigned int
	    rdlba	:32;		/* receive desc. list base address */
} receiveListBaseAddress_t;

typedef struct csr4Struct {
    unsigned int
	    tdlba	:32;		/* transmit desc. list base address */
} transmitListBaseAddress_t;

typedef struct csr5Struct {
    unsigned int
	    rsvd3	:6,
	    eb		:3,		/* error bits */
	    ts		:3,		/* transmission process state */
	    rs		:3,		/* receive process state */
	    nis		:1,		/* normal interrupt summary */
	    ais		:1,		/* abnormal interrupt summary */
	    rsvd2  	:1,
	    se		:1,		/* system error */
	    lnf		:1,		/* link fail */
	    fd		:1,		/* full-duplex short frame received */
	    at  	:1,		/* AUI/TP pin */
	    rwt		:1,		/* receive watchdog timeout */
	    rps		:1,		/* receive process stopped */
	    ru		:1,		/* receive buffer unavailable */
	    ri		:1,		/* receive interrupt */
	    unf		:1,		/* transmit underflow */
	    rsvd	:1,
	    tjt		:1,		/* transmit jabber time-out */
	    tu		:1,		/* transmit buffer unavailable */
	    tps		:1,		/* transmit process stopped */
	    ti		:1;		/* transmit interrupt */
} statusRegister_t;

typedef struct csr5Struct_41 {
    unsigned int
	    rsvd3	:6,
	    eb		:3,		/* error bits */
	    ts		:3,		/* transmission process state */
	    rs		:3,		/* receive process state */
	    nis		:1,		/* normal interrupt summary */
	    ais		:1,		/* abnormal interrupt summary */
            er  	:1,             /* early receive */
	    se		:1,		/* system error */
	    lnf		:1,		/* link fail */
	    tm		:1,		/* full-duplex short frame received */
	    rsvd1	:1,		/* AUI/TP pin */
	    rwt		:1,		/* receive watchdog timeout */
	    rps		:1,		/* receive process stopped */
	    ru		:1,		/* receive buffer unavailable */
	    ri		:1,		/* receive interrupt */
	    unf		:1,		/* transmit underflow */
	    lnp 	:1,             /* link pass */
	    tjt		:1,		/* transmit jabber time-out */
	    tu		:1,		/* transmit buffer unavailable */
	    tps		:1,		/* transmit process stopped */
	    ti		:1;		/* transmit interrupt */
} statusRegister_41_t;

typedef struct csr6Struct {
    unsigned int
            sc          :1,
	    rsvd	:13,
	    ca		:1,		/* capture effect enable */
	    rsvd2      	:1,
	    tr		:2,		/* threshold control bits */
	    st		:1,		/* start/stop transmission command */
	    fc		:1,		/* force collision mode */
	    om		:2,		/* operating mode */
	    fd		:1,		/* full-duplex mode */
	    fkd		:1,		/* flakey oscillator disable */
	    pm		:1,		/* pass all multicast */
	    pr		:1,		/* promiscuous mode */
	    sb		:1,		/* start/stop backoff counter */
	    inf		:1,		/* inverse filtering */
	    pb		:1,		/* pass bad frames */
	    ho		:1,		/* hash-only filtering mode */
	    sr		:1,		/* start/stop receive */
	    hp		:1;		/* hash/perfect receive filt. mode */
} operationModeRegister_t;

typedef struct csr7Struct {
    unsigned int
	    rsvd3	:15,
	    nim		:1,		/* normal interrupt summary mask */
	    aim		:1,		/* abnormal interrupt summary mask */
	    rsvd2	:1,
	    sem		:1,		/* system error mask */
	    lfm		:1,		/* link fail mask */
	    fdm		:1,		/* full-duplex mask  */
	    atm		:1,		/* AUI/TP switch mask */
	    rwm		:1,		/* receive watchdog timeout mask */
	    rsm		:1,		/* receive stopped mask */
	    rum		:1,		/* receive buffer unavail. mask */
	    rim		:1,		/* receive interrupt mask */
	    unm		:1,		/* underflow interrupt mask */
	    rsvd1	:1,
	    tjm		:1,		/* transmit jabber time-out mask */
	    tum		:1,		/* transmit buffer unavail. mask */
	    tsm		:1,		/* transmit stopped mask */
	    tim		:1;		/* transmit interrupt mask */
} interruptMaskRegister_t;

typedef struct csr8Struct {
    unsigned int
	    rsvd	:15,
	    mfo		:1,		/* missed frame overflow */
	    mfc		:16;		/* missed frame counter */
} missedFrameCounter_t;


/*
 * This is CSR9 for DECchip21040. 
 */

typedef struct csr90Struct {
    unsigned int
	    dtnv	:1,		/* data not valid */
	    rsvd	:23,
	    dt		:8;		/* data */
} ethernetROMRegister_t;

/*
 * This is CSR9 for DECchip21041. 
 */
typedef struct csr91Struct {
    unsigned int
            rsvd        :16,
	    mod 	:1,             /* mode select */
	    sro		:1,		/* read operation */
	    swo		:1,		/* write opration */
	    sbr 	:1,
	    ssr		:1,		/* select serial ROM */
	    sreg	:1,		/* select external register */
	    rsvd1	:6,
	    sdo		:1,		/* data out */
	    sdi		:1,		/* data in */
	    sclk	:1,		/* serial clock */
	    scs		:1;		/* chip select */
//	    mdo		:1,		/* MII management data out */
//	    mmd		:2,		/* MII management mode */
//	    mdi		:1,		/* MII management data in */
//	    rsvd4	:12;
} serialROMRegister_t;

/*
 * CSR 10 is reserved so we omit it. 
 */
struct csr10Struct {
    unsigned int
	    rsvd	:32;
};

typedef struct csr11Struct {
    unsigned int
	    rsvd	:15,
	    CON 	:1,
	    fdacv	:16;		/* full-duplex auto-conf value */
} fullDuplexRegister_t;

/*
 * Serial Interface Attachment CSRs. 
 */ 

typedef struct csr12Struct {
    unsigned int
	    rsvd	:24,
	    dao		:1,		/* PLL all one */
	    daz		:1,		/* PLL all zero */
	    dsp		:1,		/* PLL self-test pass */
	    dsd		:1,		/* PLL self-test done */
	    aps		:1,		/* auto polarity state */
	    lkf		:1,		/* link fail status */
	    ncr		:1,		/* network connection error */
	    paui	:1;		/* pin AUI_TP detection */
} siaStatusRegister_t;

typedef struct csr12Struct_41 {
    unsigned int
	    lpc		:16,		/* link-partner's link code word */
	    lpn		:1,		/* link-partner negotiable */
	    ans		:3,		/* auto-negotiate arbitratione state */
	    anr_fds	:1,		/* auto-negotiation restart/ */
					/* full-duplex selected */
	    nsn		:1,		/* non-stable NLPs detected */
	    nra		:1,		/* non-selected port activity */
	    sra		:1,		/* selected port activity */
	    dao		:1,		/* PLL all one */
	    daz		:1,		/* PLL all zero */
	    dsp		:1,		/* PLL self-test pass */
	    dsd		:1,		/* PLL self-test done */
	    aps		:1,		/* auto polarity state */
	    lkf		:1,		/* link fail status */
	    ncr		:1,		/* network connection error */
	    rsvd	:1;
} siaStatusRegister_41_t;

typedef struct csr13Struct {
    unsigned int
	    rsvd	:16,
	    oe57	:1,		/* output enable 5 6 7 */
	    oe24	:1,		/* output enable 2 4 */
	    oe13	:1,		/* outut enable 1 3 */
	    ie		:1,		/* input enable */
	    sel		:4,		/* ext. port output multiplx. select */
	    ase		:1,		/* APLL start enable */
	    sim		:1,		/* serial interface input multiplx. */
	    eni		:1,		/* encoder input multiplexer */
	    edp		:1,		/* SIA PLL external input enable */
	    aui		:1,		/* 10Base-T or AUI */
	    cac		:1,		/* CSR auto configuration */
	    ps		:1,		/* pin AUI/TP selection */
	    srl		:1;		/* SIA reset */
} siaConnectivityRegister_t;

typedef struct csr14Struct {
    unsigned int
	    rsvd2	:17,
	    spp		:1,		/* set polarity plus */
	    ape		:1,		/* auto polarity enable */
	    lte		:1,		/* link test enable */
	    sqe		:1,		/* signal quality gen. en. */
	    cld		:1,		/* collision detect enable */
	    csq		:1,		/* collision squelch enable */
	    rsq		:1,		/* receive squelch enable */
	    rsvd1	:2,
	    cpen	:2,		/* compensation enable */
	    lse		:1,		/* link pulse send enable */
	    dren	:1,		/* driver enable */
	    lbk		:1,		/* loopback enable */
	    ecen	:1;		/* encoder enable */
} siaTransmitReceiveRegister_t;

typedef struct csr14Struct_41 {
    unsigned int
	    rsvd	:16,
	    tas		:1,		/* 10BASE-T/AUI autosensing enable */
	    spp		:1,		/* set polarity plus */
	    ape		:1,		/* auto polarity enable */
	    lte		:1,		/* link test enable */
	    sqe		:1,		/* signal quality gen. en. */
	    cld		:1,		/* collision detect enable */
	    csq		:1,		/* collision squelch enable */
	    rsq		:1,		/* receive squelch enable */
	    ane		:1,		/* auto-negotiation enable */
	    hde		:1,		/* half-duplex enable */
	    cpen	:2,		/* compensation enable */
	    lse		:1,		/* link pulse send enable */
	    dren	:1,		/* driver enable */
	    lbk		:1,		/* loopback enable */
	    ecen	:1;		/* encoder enable */
} siaTransmitReceiveRegister_41_t;

typedef struct csr15Struct {
    unsigned int
	    rsvd4	:18,
	    frl		:1,		/* force receive low */
	    dpst	:1,		/* PLL self-test start */
	    rsvd3	:1,
	    flf		:1,		/* force link fail */
	    fusq	:1,		/* force unsquelch */
	    tsck	:1,		/* test clock */
	    rsvd2	:2,
	    rwr		:1,		/* receive watchdog release */
	    rwd		:1,		/* receive watchdog disable */
	    rsvd1	:1,
	    jck		:1,		/* jabber clock */
	    huj		:1,		/* host unjab */
	    jbd		:1;		/* jabber disable */
} siaGeneralRegister_t;

/*
 * This is done for convenience in accessing registers. 
 */
typedef union {
    struct csr0Struct	csr0;
    struct csr1Struct	csr1;
    struct csr2Struct	csr2;
    struct csr3Struct	csr3;
    struct csr4Struct	csr4;
    struct csr5Struct	csr5;
    struct csr5Struct_41	csr5_41;
    struct csr6Struct	csr6;
    struct csr7Struct	csr7;
    struct csr8Struct	csr8;
    struct csr90Struct	csr90;
    struct csr91Struct	csr91;
    struct csr10Struct	csr10;
    struct csr11Struct	csr11;
    struct csr12Struct	csr12;
    struct csr12Struct_41	csr12_41;
    struct csr13Struct	csr13;
    struct csr14Struct	csr14;
    struct csr15Struct	csr15;
    unsigned int	data;
} csrRegUnion;

/*  
 * Descriptor structures
 */

typedef union	{
    struct _tdes0	{
	unsigned int	
	    own		:1,		/* ownership (1 == 21X40) */
	    rsvd2	:15,
	    es		:1,		/* error summary */
	    to		:1,		/* transmit jabber timeout */
	    rsvd1	:2,
	    lo		:1,		/* loss of carrier */
	    nc		:1,		/* no carrier */
	    lc		:1,		/* late collision */
	    ec		:1,		/* excessive collisions */
	    hf		:1,		/* heartbeat fail */
	    cc		:4,		/* collision count */
	    lf		:1,		/* link fail */
	    uf		:1,		/* underflow error */
	    de		:1;		/* deferred */
    } reg;
    unsigned int data;
} tdes0;

typedef union	{
    struct	{
	unsigned int	
	    ic		:1,			/* interrupt on completion */
	    ls		:1,			/* last segment */
	    fs		:1,			/* first segment */
	    ft1		:1,			/* filtering type */
	    set		:1,			/* setup packet */
	    ac		:1,			/* add CRC disable */
	    ter		:1,			/* transmit end of ring */
	    tch		:1,			/* second address chained */
	    dpd		:1,			/* disabled padding */
	    fto		:1,			/* filtering type */
	    byteCountBuffer2	:11,		/* buffer 2 size */
	    byteCountBuffer1	:11;		/* buffer 1 size */
    } reg;
    unsigned int data;
} tdes1;

typedef struct _txDescriptorStruct	{
	tdes0 status;
	tdes1 control;
	unsigned int bufferAddress1;
	unsigned int bufferAddress2;
} txDescriptorStruct;

typedef union	{
    struct	{
	unsigned int	
	    own		:1,		/* ownership (1 == 21X40) */
	    fl		:15,		/* frame length */
	    es		:1,		/* error summary */
	    le		:1,		/* length error */
	    dt		:2,		/* data type */
	    rf		:1,		/* runt frame */
	    mf		:1,		/* multicast frame */
	    fs		:1,		/* first descriptor */
	    ls		:1,		/* last descriptor */
	    tl		:1,		/* frame too long */
	    cs		:1,		/* collision seen */
	    ft		:1,		/* frame type */
	    rj		:1,		/* receive watchdog */
	    rsvd	:1,
	    db		:1,		/* dribbling bit */
	    ce		:1,		/* CRC error */
	    of		:1;		/* overflow */
    } reg;
    unsigned int data;
} rdes0;

typedef union	{
    struct	{
	unsigned int	
	    rsvd2	:6,
	    rer		:1,			/* receive end of ring */
	    rch		:1,			/* second address chained */
	    rsvd1	:2,
	    byteCountBuffer2	:11,		/* buffer 2 size */
	    byteCountBuffer1	:11;		/* buffer 1 size */
    } reg;
    unsigned int data;
} rdes1;

typedef struct _rxDescriptorStruct	{
	rdes0 status;
	rdes1 control;
	unsigned int bufferAddress1;
	unsigned int bufferAddress2;
} rxDescriptorStruct;

#define DEC21040_VENDOR_DEVICE_ID		0x00021011
#define DEC21041_VENDOR_DEVICE_ID		0x00141011

#define DEC_21X40_SETUP_PERFECT_ENTRIES		16
#define DEC_21X40_SETUP_PERFECT_MCAST_ENTRIES	14

typedef struct {
	unsigned long physicalAddress[DEC_21X40_SETUP_PERFECT_ENTRIES][3];
} setupBuffer_t;

#ifdef undef
void *
IOMallocPage(int page_size, void * * actual_ptr,
				 int * actual_size)
{
    void * mem_ptr;
    
    if (page_size != PAGE_SIZE)
	return (NULL);
    
    *actual_size = PAGE_SIZE * 2;
    mem_ptr = IOMalloc(PAGE_SIZE * 2);
    if (mem_ptr == NULL)
	return (NULL);
    *actual_ptr = mem_ptr;
    return ((void *)round_page(mem_ptr));
}
#endif undef

