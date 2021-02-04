/* **********************************************************
 * Copyright 1998 VMware, Inc.  All rights reserved. -- VMware Confidential
 * **********************************************************/

#ifndef __VMXNETINT_H__
#define __VMXNETINT_H__

#include "net_dist.h"

#define VMXNET_CHIP_NAME "vmxnet ether"

#define CRC_POLYNOMIAL_LE 0xedb88320UL  /* Ethernet CRC, little endian */

#define PKT_BUF_SZ			1536

typedef enum Vmxnet_TxStatus {
   VMXNET_CALL_TRANSMIT,
   VMXNET_DEFER_TRANSMIT,
   VMXNET_STOP_TRANSMIT
} Vmxnet_TxStatus;

#define le16_to_cpu(x) ((unsigned short)(x))
#define le32_to_cpu(x) ((unsigned int)(x))

/*
 * Private data area, pointed to by priv field of our struct net_device.
 * dd field is shared with the lower layer.
 */
typedef struct Vmxnet_Private {
    Vmxnet2_DriverData	*dd;
    const char		*name;
    netbuf_t		rxSkbuff[VMXNET2_MAX_NUM_RX_BUFFERS];
    spinlock_t		txLock;
    int			numTxPending;
    unsigned int	numRxBuffers;
    unsigned int	numTxBuffers;
    Vmxnet2_RxRingEntry	*rxRing;
    Vmxnet2_TxRingEntry	*txRing;
    
    Bool		devOpen;
    Net_PortID		portID;
    
    uint32		capabilities;
    uint32		features;
    
    Bool		morphed;	// Indicates whether adapter is morphed
    unsigned short	ioaddr;
} Vmxnet_Private;

#endif /* __VMXNETINT_H__ */
