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

/* TypeDefs for the basic data bytes. */

typedef unsigned char		byte, *bytePtr;
typedef char			int8;
typedef unsigned char		boolean;

typedef unsigned short		word;
typedef short 			int16;

typedef unsigned int 		dword;
typedef int 			int32;

#define BYTE_AT(x)		(*((byte PTR)(x)))
#define WORD_AT(x)		(*((word PTR)(x)))
#define DWORD_AT(x)		(*((dword PTR)(x)))

#define high(x)			((byte)((x) >> 8))
#define low(x)			((byte)(x))
#define hlword(h, l)		(((byte)(l)) | (((byte)(h)) << 8))

#define offsetof(typ,id)	(size_t)&(((typ*)0)->id)


/* 
 * On a Mac, there is no need to byte-swap data on the network, so 
 * these macros do nothing 
 */

#define netw(x)		x
#define netdw(x)	x


typedef struct
{
  at_net           network;       /* network number */
  byte           nodeid;        /* node number    */
  byte           socket;        /* socket number  */
} AddrBlk, *AddrBlkPtr;

typedef union
{
   at_inet_t     a;
} AddrUnion, *AddrUnionPtr;



/* End Portab.h */
