/* **********************************************************
 * Copyright 1998 VMware, Inc.  All rights reserved. -- VMware Confidential
 * **********************************************************/

/************************************************************
 *
 *   net.h
 *
 *   This file should contain all network global defines.
 *   No vlance/vmxnet/vnet/vmknet specific stuff should be
 *   put here only defines used/usable by all network code.
 *   --gustav
 *
 ************************************************************/

#ifndef _NET_H_
#define _NET_H_

#define MAX_ETHERNET_CARDS      4 

#define ETHERNET_MTU         1518
#define ETH_MIN_FRAME_LEN      60


#define ETHER_ADDR_LEN          6  /* length of MAC address */
#define ETH_HEADER_LEN	       14  /* length of Ethernet header */
#define IP_ADDR_LEN	        4  /* length of IPv4 address */
#define IP_HEADER_LEN	       20  /* minimum length of IPv4 header */

#define ETHER_MAX_QUEUED_PACKET 1600


/* 
 * State's that a NIC can be in currently we only use this 
 * in VLance but if we implement/emulate new adapters that
 * we also want to be able to morph a new corresponding 
 * state should be added.
 */

#define LANCE_CHIP  0x2934
#define VMXNET_CHIP 0x4392

/* 
 * Size of reserved IO space needed by the LANCE adapter and
 * the VMXNET adapter. If you add more ports to Vmxnet than
 * there is reserved space you must bump VMXNET_CHIP_IO_RESV_SIZE.
 * The sizes must be powers of 2.
 */

#define LANCE_CHIP_IO_RESV_SIZE  0x20 
#define VMXNET_CHIP_IO_RESV_SIZE 0x40

#define MORPH_PORT_SIZE 4

#endif //_NET_H_

