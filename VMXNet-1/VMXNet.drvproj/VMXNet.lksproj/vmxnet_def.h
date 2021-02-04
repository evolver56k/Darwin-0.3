/* **********************************************************
 * Copyright 1999 VMware, Inc.  All rights reserved. 
 * -- VMware Confidential
 * **********************************************************/

#ifndef _VMXNET_DEF_H_
#define _VMXNET_DEF_H_

#include "net_sg.h"
#include "vmnet_def.h"


/*
 *   Vmxnet I/O ports, used by both the vmxnet driver and 
 *   the device emulation code.
 */

#define VMXNET_INIT_ADDR		0x00
#define VMXNET_INIT_LENGTH		0x04
#define VMXNET_TX_ADDR		        0x08
#define VMXNET_COMMAND_ADDR		0x0c
#define VMXNET_MAC_ADDR			0x10
#define VMXNET_LOW_VERSION		0x18
#define VMXNET_HIGH_VERSION		0x1c
#define VMXNET_STATUS_ADDR		0x20
#define VMXNET_TOE_INIT_ADDR            0x24
#define VMXNET_APROM_ADDR               0x28

/*
 * Vmxnet command register values.
 */
#define VMXNET_CMD_INTR_ACK		0x0001
#define VMXNET_CMD_UPDATE_LADRF		0x0002
#define VMXNET_CMD_UPDATE_IFF		0x0004
#define VMXNET_CMD_UNUSED 1		0x0008
#define VMXNET_CMD_UNUSED_2		0x0010
#define VMXNET_CMD_INTR_DISABLE  	0x0020
#define VMXNET_CMD_INTR_ENABLE   	0x0040
#define VMXNET_CMD_UNUSED_3		0x0080
#define VMXNET_CMD_CHECK_TX_DONE	0x0100
#define VMXNET_CMD_GET_NUM_RX_BUFFERS	0x0200
#define VMXNET_CMD_GET_NUM_TX_BUFFERS	0x0400
#define VMXNET_CMD_PIN_TX_BUFFERS	0x0800
#define VMXNET_CMD_GET_CAPABILITIES	0x1000
#define VMXNET_CMD_GET_FEATURES		0x2000

/*
 * Vmxnet status register values.
 */
#define VMXNET_STATUS_CONNECTED		0x0001
#define VMXNET_STATUS_ENABLED		0x0002
#define VMXNET_STATUS_TX_PINNED         0x0004

/*
 * Values for the interface flags.
 */
#define VMXNET_IFF_PROMISC		0x01
#define VMXNET_IFF_BROADCAST		0x02
#define VMXNET_IFF_MULTICAST		0x04
#define VMXNET_IFF_DIRECTED             0x08

/*
 * Length of the multicast address filter.
 */
#define VMXNET_MAX_LADRF		2

/*
 * Size of Vmxnet APROM. 
 */
#define VMXNET_APROM_SIZE 6

/*
 * An invalid ring index.
 */
#define VMXNET_INVALID_RING_INDEX	-1

/*
 * Maximum number of pages that the vmxnet emulation is willing to
 * share with the driver.
 */
#define VMXNET_MAX_SHARED_PAGES		4

/*
 * Features that are implemented by the driver.  These are driver
 * specific so not all features will be listed here.  In addition not all
 * drivers have to pay attention to these feature flags.
 *
 *  VMXNET_FEATURE_ZERO_COPY_TX 	The driver won't do any copies as long as
 *					the packet length is > 
 *					Vmxnet_DriverData.minTxPhysLength.
 * 
 *  VMXNET_FEATURE_TSO                  The driver will use the TSO capabilities
 *                                      of the underlying hardware if available 
 *                                      and enabled.
 */
#define VMXNET_FEATURE_ZERO_COPY_TX		0x01
#define VMXNET_FEATURE_TSO		        0x02

/*
 * Define the set of capabilities required by each feature above
 */
#define VMXNET_FEATURE_ZERO_COPY_TX_CAPS        VMXNET_CAP_SG
#define VMXNET_FEATURE_TSO_CAPS                 VMXNET_CAP_TSO
#define VMXNET_HIGHEST_FEATURE_BIT              VMXNET_FEATURE_TSO

#define VMXNET_INC(val, max)     \
   val++;                        \
   if (val == max) {   		 \
      val = 0;                   \
   }

/*
 * code that just wants to switch on the different versions of the
 * guest<->implementation protocol can cast driver data to this.
 */
typedef uint32 Vmxnet_DDMagic;

#endif /* _VMXNET_DEF_H_ */


