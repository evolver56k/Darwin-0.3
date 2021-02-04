/* **********************************************************
 * Copyright 2004 VMware, Inc.  All rights reserved. 
 * -- VMware Confidential
 * **********************************************************/

#ifndef _VMXNET2_DEF_H_
#define _VMXNET2_DEF_H_

#include "net_sg.h"
#include "vmxnet_def.h"


/*
 * Magic number that identifies this version of the vmxnet protocol.
 */
#define VMXNET2_MAGIC			0xbabe864f

#define VMXNET2_MAX_NUM_RX_BUFFERS		128
#define VMXNET2_DEFAULT_NUM_RX_BUFFERS	        100

#define VMXNET2_MAX_NUM_TX_BUFFERS		128
#define VMXNET2_DEFAULT_NUM_TX_BUFFERS	        100

enum {
   VMXNET2_OWNERSHIP_DRIVER,
   VMXNET2_OWNERSHIP_DRIVER_PENDING,
   VMXNET2_OWNERSHIP_NIC,
   VMXNET2_OWNERSHIP_NIC_PENDING,
};

#define VMXNET2_SG_DEFAULT_LENGTH	6

typedef struct Vmxnet2_SG_Array {
   uint16	addrType;
   uint16	length;
   NetSG_Elem	sg[VMXNET2_SG_DEFAULT_LENGTH];
} Vmxnet2_SG_Array;

typedef struct Vmxnet2_RxRingEntry {
   uint32		paddr;		/* Physical address of the packet data. */
   uint32		paddrPad;	/* Address pad to make it 64 bit long. */

   uint32		bufferLength;	/* The length of the data at paddr. */
   uint32		actualLength;	/* The actual length of the received data. */
   uint16               ownership;	/* Who owns the packet. */
   uint16		flags;		/* Flags as defined below. */
   void		       *driverData;	/* Driver specific data */
} Vmxnet2_RxRingEntry;

/*
 * Vmxnet2_RxRingEntry flags:
 * 
 * VMXNET2_RX_HW_XSUM_OK       The hardware verified the TCP/UDP checksum.
 *
 */
#define VMXNET2_RX_HW_XSUM_OK  0x01

typedef struct Vmxnet2_TxRingEntry {
   uint16		flags;		/* Flags as defined below. */
   uint16 	        ownership;	/* Who owns this packet. */
   void 	       *driverData;	/* Driver specific data. */
   uint32               tsoMss;         /* TSO pkt MSS */
   Vmxnet2_SG_Array	sg;		/* Packet data. */
} Vmxnet2_TxRingEntry;

/*
 * Vmxnet2_TxRingEntry flags:
 *
 *   VMXNET2_TX_CAN_KEEP	The implementation can return the tx ring entry 
 *				to the driver when it is ready as opposed to 
 *				before the transmit call from the driver completes.
 *   VMXNET2_TX_RING_LOW	The driver's transmit ring buffer is low on free
 *				slots.
 *   VMXNET2_TX_HW_XSUM         The hardware should perform the TCP/UDP checksum
 *   VMXNET2_TX_TSO             The hardware should do TCP segmentation.
 *   VMXNET2_TX_PINNED_BUFFER   The driver used one of the preallocated vmkernel
 *                              buffers *and* it has been pinned with Net_PinTxBuffers.
 */
#define VMXNET2_TX_CAN_KEEP	     0x0001
#define VMXNET2_TX_RING_LOW	     0x0002
#define VMXNET2_TX_HW_XSUM           0x0004
#define VMXNET2_TX_TSO	             0x0008
#define VMXNET2_TX_PINNED_BUFFER     0x0010

/*
 * Structure used by implementations.  This structure allows the inline
 * functions below to be used.
 */
typedef struct Vmxnet2_ImplData {
   Vmxnet2_RxRingEntry    *rxRingPtr;
   uint32		   rxNICNext; 
   uint32		   rxRingLength;
   Vmxnet2_RxRingEntry    *rxRingPtr2;
   uint32		   rxNICNext2; 
   uint32		   rxRingLength2;
   Vmxnet2_TxRingEntry    *txRingPtr;   
   uint32		   txRingLength;
   uint32		   txNICNext;
   struct PhysMem_Token	  *ddPhysMemToken;
} Vmxnet2_ImplData;

typedef struct Vmxnet2_DriverStats {
   uint32	transmits;	   /* # of times that the drivers transmit function */
				   /*   is called. The driver could transmit more */
				   /*   than one packet per call. */
   uint32	pktsTransmitted;   /* # of packets transmitted. */
   uint32	noCopyTransmits;   /* # of packets that are transmitted without */
				   /*   copying any data. */
   uint32	copyTransmits;	   /* # of packets that are transmittted by copying */
				   /*   the data into a buffer. */
   uint32	maxTxsPending;	   /* Max # of transmits outstanding. */
   uint32	txStopped;	   /* # of times that transmits got stopped because */
				   /*   the tx ring was full. */
   uint32	txRingOverflow;	   /* # of times that transmits got deferred bc */
				   /*   the tx ring was full.  This must be >= */
				   /*   txStopped since there will be one */
				   /*   txStopped when the ring fills up and then */
				   /*   one txsRingOverflow for each packet that */
				   /*   that gets deferred until there is space. */
   uint32	interrupts;	   /* # of times interrupted. */
   uint32	pktsReceived;	   /* # of packets received. */
   uint32	rxBuffersLow;	   /* # of times that the driver was low on */
				   /*   receive buffers. */
} Vmxnet2_DriverStats;

/*
 * Shared data structure between the vm, the vmm, and the vmkernel.
 * This structure was originally arranged to try to group common data 
 * on 32-byte cache lines, but bit rot and the fact that we no longer
 * run on many CPUs with that cacheline size killed that optimization.
 * vmxnet3 should target 128 byte sizes and alignments to optimize for
 * the 64 byte cacheline pairs on P4.
 */
typedef struct Vmxnet2_DriverData {
   /*
    * Magic must be first.
    */
   Vmxnet_DDMagic       magic;

   /*
    * Receive fields. 
    */
   uint32		rxRingLength;		/* Length of the receive ring. */
   uint32		rxDriverNext;		/* Index of the next packet that will */
						/*   be filled in by the impl */

   uint32		rxRingLength2;	        /* Length of the 2nd receive ring. */
   uint32		rxDriverNext2;	        /* Index of the next packet that will */
						/*   be filled in by the impl */

   uint32		notUsed1;               /* was "irq" */

   /*
    * Interface flags and multicast filter.
    */
   uint32		ifflags;
   uint32		LADRF[VMXNET_MAX_LADRF];

   /*
    * Transmit fields
    */
   uint32               txDontClusterSize;      /* All packets <= this will be transmitted */
                                                /* immediately, regardless of clustering */
                                                /* settings [was fill[1]] */
   uint32		txRingLength;		/* Length of the transmit ring. */
   uint32		txDriverCur;		/* Index of the next packet to be */
						/*   returned by the implementation.*/
   uint32		txDriverNext;		/* Index of the entry in the ring */
						/*   buffer to use for the next packet.*/
   uint32		txStopped;  		/* The driver has stopped transmitting */
						/*   because its ring buffer is full.*/
   uint32		txClusterLength;	/* Maximum number of packets to */
						/*   put in the ring buffer before */
						/*   asking the implementation to */
						/*   transmit the packets in the buffer.*/
   uint32		txNumDeferred;          /* Number of packets that have been */
						/*   queued in the ring buffer since */
						/*   the last time the implementation */
						/*   was asked to transmit. */
   uint32		notUsed3;               /* This field is deprecated but still used */
                                                /* as minXmitPhysLength on the escher branch. */
                                                /* It cannot be used for other purposes */
                                                /* until escher vms no longer are allowed */
                                                /* to install this driver. */

   /*
    * Extra fields for future expansion.
    */
   uint32		extra[6];

   /*
    * The following fields are used to save the NICNext indexes part
    * of implData in the vmkernel when disconnecting the adapter, we
    * need them when we reconnect.  This mechanism is used for
    * checkpointing as well.
    */
   uint32               savedRxNICNext;
   uint32               savedRxNICNext2;
   uint32               savedTxNICNext;

   /*
    * Fields used during initialization or debugging.
    */
   uint32		length;
   uint32		rxRingOffset;
   uint32		rxRingOffset2;
   uint32		txRingOffset;   
   uint32		debugLevel;
   uint32		txBufferPhysStart;
   uint32		txBufferPhysLength;
   uint32		txPktMaxSize;

   /*
    * Driver statistics.
    */
   Vmxnet2_DriverStats	stats;
} Vmxnet2_DriverData;

#endif

