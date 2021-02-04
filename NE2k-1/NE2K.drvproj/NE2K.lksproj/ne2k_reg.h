
/*
 * NE2000 / NIC Registers
 */

#ifndef NE2K_REG
#define NE2K_REG	1



/* CR - Command Register */

#define	NIC_CR_OFFSET	0x00
#define NIC_CR_PGREAD	0x00
#define NIC_CR_PGWRITE	0x00


typedef struct {
    unsigned char	stp	:1,	/* stop device */
                        sta	:1,	/* start device */
                        txp	:1,	/* begin packet xmt */
                        rd	:3,	/* x */
                        psel	:2;	/* register page select */
} NIC_CR;



/* ISR - Interrupt Status Register */

#define	NIC_ISR_OFFSET	0x07
#define NIC_ISR_PGREAD	0x00
#define NIC_ISR_PGWRITE	0x00


typedef struct {
    unsigned char	prx	:1,	/* packet recvd */
                        ptx	:1,	/* packet xmtd */
                        rxe	:1,	/* packet recvd w/error */
                        txe	:1,	/* packet xmt error */
                        ovw	:1,	/* recv ring overwrite warning */
                        cnt	:1,	/* counter overflow warning */
                        rdc	:1,	/* set when remote DMA completed */
                        rst	:1;	/* device stopped */
} NIC_ISR;



/* IMR - Interrupt Mask Register */

#define	NIC_IMR_OFFSET	0x0F
#define NIC_IMR_PGREAD	0x02
#define NIC_IMR_PGWRITE	0x00



typedef struct {
    unsigned char	prx	:1,	/* packet recvd */
                        ptx	:1,	/* packet xmtd */
                        rxe	:1,	/* packet recvd w/error */
                        txe	:1,	/* packet xmt error */
                        ovw	:1,	/* recv ring overwrite warning */
                        cnt	:1,	/* counter overflow warning */
                        rdc	:1,	/* set when remote DMA completed */
                        rst	:1;	/* device stopped */
} NIC_IMR;



/* DCR - Data Configuration Register */

#define	NIC_DCR_OFFSET	0x0E
#define NIC_DCR_PGREAD	0x02
#define NIC_DCR_PGWRITE	0x00


typedef struct {
    unsigned char	wts	:1,	/* word transfer select 0=byte/1=word */
                        bos	:1,	/* byte order select 0=Little Endian/1=Big Endian */
                        las	:1,	/* 0! */
                        ls	:1,	/* loopback select 0=loopback mode/1=normal mode */
                        arm	:1,	/* auto-init remote 0=send packet command not exec/1=exec */
                        ft	:2,	/* fifo treshold select bits */
				:1;	/* 1 */
} NIC_DCR;



/* TCR - Transmit Configuration Register */

#define	NIC_TCR_OFFSET	0x0D
#define NIC_TCR_PGREAD	0x02
#define NIC_TCR_PGWRITE	0x00



typedef struct {
    unsigned char	crc	:1,	/* no CRC generation */
                        lb	:2,	/* loopback mode */
			atd	:1,	/* auto transmit */
    			ofst	:1,	/* collision offset enable */
				:3;
} NIC_TCR;



/* TSR - Transmit Status Register */

#define	NIC_TSR_OFFSET	0x04
#define NIC_TSR_PGREAD	0x00


typedef struct {
    unsigned char	ptx	:1,	/* packet xmtd on wire */
                        nd	:1,	/* 1 */
                        col	:1,	/* xmtd with collisions */
                        abort	:1,	/* not xmtd due to excess. coll. */
                        crs	:1,	/* packet xmtd but carrier was lost */
                        under	:1,	/* 0 */
                        cdh	:1,	/* cd heartbeat detected */
                        owc	:1;	/* out of win. collision occurred */
} NIC_TSR;



/* RCR - Receive Configuration Register (rcon) */

#define	NIC_RCR_OFFSET	0x0C
#define NIC_RCR_PGREAD	0x02
#define NIC_RCR_PGWRITE	0x00


typedef struct {
    unsigned char	sep	:1,	/* save error packets */
                        runts	:1,	/* save runt packets */
                        broad	:1,	/* receive broadcast packets */
                        group	:1,	/* receive *all* multicast packets [MULTICAST] */
                        prom	:1,	/* receive all packets [PROMISCUOUS] */
                        mon	:1,	/* monitor network */
                                :2;
} NIC_RCR;



/* RSR - Receive Status Register */

#define	NIC_RSR_OFFSET	0x0C
#define NIC_RSR_PGREAD	0x00


typedef struct {
    unsigned char	prx	:1,	/* packet recvd w/o error */
                        crc	:1,	/* packet recvd w/crc error */
                        fae	:1,	/* packet recvd w/framing error */
                        over	:1,	/* 0 */
                        mpa	:1,	/* missed packet occurred */
                        group	:1,	/* packet recvd is bcast or mcast */
                        dis	:1,	/* receiver is in mon mode */
                        dfr	:1;	/* jabber condition on wire */
} NIC_RSR;



/* Current Local DMA Registers */

#define	NIC_CLDA0_OFFSET	0x01
#define	NIC_CLDA1_OFFSET	0x02
#define NIC_CLDA_PGREAD		0x00



/* Page Start Register */

#define	NIC_PSTART_OFFSET	0x01
#define NIC_PSTART_PGREAD	0x02
#define NIC_PSTART_PGWRITE	0x00



/* Page Stop Register */

#define	NIC_PSTOP_OFFSET	0x02
#define NIC_PSTOP_PGREAD	0x02
#define NIC_PSTOP_PGWRITE	0x00



/* Boundary Register */

#define	NIC_BNRY_OFFSET		0x03
#define NIC_BNRY_PGREAD		0x00
#define NIC_BNRY_PGWRITE	0x00



/* Transmit Page Start Register */

#define NIC_TPSR_OFFSET		0x04
#define NIC_TPSR_PGWRITE	0x00

/* Transmit Byte Count Registers */

#define	NIC_TBCR0_OFFSET 	0x05
#define	NIC_TBCR1_OFFSET 	0x06
#define NIC_TBCR_PGWRITE	0x00




/* Number of Collisions Register */

#define	NIC_NCR_OFFSET	0x05
#define NIC_NCR_PGREAD	0x00



/* FIFO Register */

#define	NIC_FIFO_OFFSET	0x06
#define NIC_FIFO_PGREAD	0x00



/* Current Remote DMA Registers */

#define	NIC_CRDA0_OFFSET	0x08
#define	NIC_CRDA1_OFFSET	0x09
#define NIC_CRDA_PGREAD		0x00



/* Remote Start Address Registers */

#define	NIC_RSAR0_OFFSET	0x08
#define	NIC_RSAR1_OFFSET	0x09
#define NIC_RSAR_PGWRITE       	0x00



/* Remote Byte Count Registers */

#define	NIC_RBCR0_OFFSET	0x0A
#define	NIC_RBCR1_OFFSET	0x0B
#define NIC_RBCR_PGWRITE       	0x00



/* Frame Aligment Error Tally Counter Register */

#define	NIC_CNTR0_OFFSET	0x0D
#define NIC_CNTR0_PGREAD	0x00



/* CRC Error Tally Counter Register */

#define	NIC_CNTR1_OFFSET	0x0E
#define NIC_CNTR1_PGREAD	0x00



/* Missed Packet Tally Counter Register */

#define	NIC_CNTR2_OFFSET	0x0F
#define NIC_CNTR2_PGREAD	0x00



/* Physical Address Register */

#define	NIC_PAR0_OFFSET	0x01
#define	NIC_PAR1_OFFSET	0x02
#define	NIC_PAR2_OFFSET	0x03
#define	NIC_PAR3_OFFSET	0x04
#define	NIC_PAR4_OFFSET	0x05
#define	NIC_PAR5_OFFSET	0x06
#define NIC_PAR_PGREAD	0x01
#define NIC_PAR_PGWRITE	0x01



/* Current Page Register */

#define	NIC_CURR_OFFSET		0x07
#define NIC_CURR_PGREAD		0x01
#define NIC_CURR_PGWRITE	0x01



/* Multicast Address Register */

#define	NIC_MAR0_OFFSET	0x08
#define	NIC_MAR1_OFFSET	0x09
#define	NIC_MAR2_OFFSET	0x0A
#define	NIC_MAR3_OFFSET	0x0B
#define	NIC_MAR4_OFFSET	0x0C
#define	NIC_MAR5_OFFSET	0x0D
#define	NIC_MAR6_OFFSET	0x0E
#define	NIC_MAR7_OFFSET	0x0F
#define NIC_MAR_PGREAD	0x01
#define NIC_MAR_PGWRITE	0x01


/*
 * NE offsets
 */

#define	NE_CMD		0x00
#define	NE_DATAPORT	0x10
#define	NE_RESET	0x1f
#define	NE_IO_EXTENT	0x20


/* joker */

typedef  union {
    NIC_CR		cr;
    NIC_ISR		isr;
    NIC_IMR		imr;
    NIC_DCR		dcr;
    NIC_TCR		tcr;
    NIC_TSR		tsr;
    NIC_RCR		rcr;
    NIC_RSR	rsr;
    unsigned char	data;
} RCNV;


#endif
