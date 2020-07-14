/*
 * Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
 *
 * @APPLE_LICENSE_HEADER_START@
 * 
 * "Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
 * Reserved.  This file contains Original Code and/or Modifications of
 * Original Code as defined in and that are subject to the Apple Public
 * Source License Version 1.0 (the 'License').  You may not use this file
 * except in compliance with the License.  Please obtain a copy of the
 * License at http://www.apple.com/publicsource and read it before using
 * this file.
 * 
 * The Original Code and all software distributed under the License are
 * distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE OR NON-INFRINGEMENT.  Please see the
 * License for the specific language governing rights and limitations
 * under the License."
 * 
 * @APPLE_LICENSE_HEADER_END@
 */
/*
	File:		HFSBtreesPriv.h

	Contains:	On disk B-tree structures for HFS and HFS Plus volumes.

	Version:	HFS Plus 1.0

	Copyright:	© 1997 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				Don Brady

		Other Contact:		Mark Day

		Technology:			File Systems

	Writers:

		(djb)	Don Brady

	Change History (most recent first):

	   <CS1>	 4/28/97	djb		first checked in
	  <HFS2>	  3/3/97	djb		Added kBTreeHeaderUserBytes constant.
	  <HFS1>	 2/19/97	djb		first checked in
*/

#ifndef	__HFSBTREESPRIV__
#define __HFSBTREESPRIV__

#if TARGET_OS_MAC
#include <Types.h>
#elif TARGET_OS_RHAPSODY
#ifndef __MACOSTYPES__
#include "MacOSTypes.h"
#endif
#endif 	/* TARGET_OS_MAC */

//// Node Descriptor - at beginning of each node

#if defined(powerc) || defined (__powerc)
	#pragma options align=mac68k
#endif
struct BTNodeDescriptor {
	UInt32						fLink;				// forward link
	UInt32						bLink;				// backward link
	SInt8						type;				// node type
	UInt8						height;				// node height
	UInt16						numRecords;			// number of records
	UInt16						reserved;			// reserved
};

typedef struct BTNodeDescriptor BTNodeDescriptor;

#if defined(powerc) || defined (__powerc)
	#pragma options align=reset
#endif


//// Node Types

typedef enum {
	kLeafNode		= -1,
	kIndexNode		=  0,
	kHeaderNode		=  1,
	kMapNode		=  2
} NodeType;


//// Header Node Structure (BTNodeDescriptor + Header Record)
#if defined(powerc) || defined (__powerc)
	#pragma options align=mac68k
#endif
typedef struct {
	BTNodeDescriptor		node;
	UInt16					treeDepth;
	UInt32					rootNode;
	UInt32					leafRecords;
	UInt32					firstLeafNode;
	UInt32					lastLeafNode;
	UInt16					nodeSize;
	UInt16					maxKeyLength;
	UInt32					totalNodes;
	UInt32					freeNodes;
	UInt16					reserved1;
	UInt32					clumpSize;		// misalligned
	UInt8					btreeType;
	UInt8					reserved2;
	UInt32					attributes;		// long aligned again
	UInt32					reserved3[16];
} HeaderRec, *HeaderPtr;
#if defined(powerc) || defined (__powerc)
	#pragma options align=reset
#endif


typedef enum {
	kBTBadCloseMask				= 0x00000001,
	kBTBigKeysMask				= 0x00000002,
	kBTVariableIndexKeysMask	= 0x00000004
}	BTreeAttributes;


enum {
	kBTreeHeaderUserBytes = 128

};


#endif //__HFSBTREESPRIV__
