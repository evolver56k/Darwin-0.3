/* **********************************************************
 * Copyright 2000 VMware, Inc.  All rights reserved. -- VMware Confidential
 * **********************************************************/

/*
 * net_sg.h --
 *
 *	Network packet scatter gather structure.
 */


#ifndef _NET_SG_H
#define _NET_SG_H

#define NET_SG_DEFAULT_LENGTH	16

/*
 * A single scatter-gather element for a network packet.
 * The address is split into low and high to save space.
 * If we make it 64 bits then Windows pads things out such that
 * we lose a lot of space for each scatter gather array.
 * This adds up when you have embedded scatter-gather 
 * arrays for transmit and receive ring buffers.
 */
typedef struct NetSG_Elem {
   uint32 	addrLow;
   uint16	addrHi;
   uint16	length;
} NetSG_Elem;

typedef enum NetSG_AddrType {
   NET_SG_MACH_ADDR,
   NET_SG_PHYS_ADDR,
   NET_SG_VIRT_ADDR,
} NetSG_AddrType;

typedef struct NetSG_Array {
   uint16	addrType;
   uint16	length;
   NetSG_Elem	sg[NET_SG_DEFAULT_LENGTH];
} NetSG_Array;

#define NET_SG_SIZE(len) (sizeof(NetSG_Array) + (len - NET_SG_DEFAULT_LENGTH) * sizeof(NetSG_Elem))

#define NET_SG_MAKE_PA(elem) (((PA)elem.addrHi << 32) | (PA)elem.addrLow)

#endif
