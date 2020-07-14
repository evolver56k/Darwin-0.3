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
/* -*- mode:C++; tab-width: 4 -*- */

#include <Types.h>
#include <KernelParams.h>

#include "SecondaryLoaderOptions.h"
#include <SecondaryLoader.h>
#include <memory.h>

#ifndef DUMP_TREE
#define DUMP_TREE	0
#endif


extern char *gKernelBSSAppendixP;
extern boot_args *gKernelParamsP;


static void traverseSubTree (CICell root)
{
	CICell node;
	DeviceTreeNode *thisNodeP;
	DeviceTreeNodeProperty prop;
	char previous[kPropNameLength];

//	printf ("subtree %X\n", root);

	// Reserve space for this node's descriptor and remember where it is
	thisNodeP = VCALL(AppendBSSData) (nil, sizeof (*thisNodeP));

	// Emit the property list for this node
	previous[0] = 0;					// So we get the first one OK

	while (VCALL (GetNextProperty) (root, previous, prop.name)) {
		long remaining;
		CICell size;
		DeviceTreeNodeProperty *thisPropP;

		thisPropP = VCALL(AppendBSSData) (&prop, sizeof (prop));
		remaining = KERNEL_LEN - (UInt32) gKernelBSSAppendixP;
		size = VCALL (GetProperty) (root, prop.name, gKernelBSSAppendixP, remaining);

#if 0
		if (strcmp (prop.name, "name") == 0)
			printf ("name=\"%s\"\n", gKernelBSSAppendixP);
		else
			printf ("    prop=\"%s\"\n", prop.name);
#endif

		if (remaining < size) VCALL (FatalError) ("Device Tree prop too big!");
		thisPropP->length = size; 		// Fixup size in our property descriptor
		gKernelBSSAppendixP += (size + 3) & -4;	// Allocate enough to round to longword boundary
		++thisNodeP->nProperties;		// Accumulate count of properties we find
		strcpy (previous, prop.name);	// Remember this one's name so we can move on
	}

	// Enumerate all direct children of this node
	for (node = VCALL (GetChildPHandle) (root);
		 node != 0;
		 node = VCALL (GetPeerPHandle) (node))
	{
		traverseSubTree (node);
		++thisNodeP->nChildren;			// Accumulate count of children we find
	}
}


#if DUMP_TREE
static int indent = 0;
static char *cursorP;

static void dumpTree ()
{
	DeviceTreeNode *nodeP = (DeviceTreeNode *) cursorP;
	int k;

	if (nodeP->nProperties == 0) return;	// End of the list of nodes
	cursorP = (char *) (nodeP + 1);

	// Dump properties (only name for now)
	for (k = 0; k < nodeP->nProperties; ++k) {
		DeviceTreeNodeProperty *propP = (DeviceTreeNodeProperty *) cursorP;

		cursorP += sizeof (*propP) + ((propP->length + 3) & -4);

		if (strcmp (propP->name, "name") == 0) {
			int n;

			// I know printf can do all of this in one call but my printf is lame
			for (n = 0; n < indent; ++n) printf ("  ");
			printf ("%s\n", propP + 1);
		}
	}

	// Dump child nodes
	++indent;
	for (k = 0; k < nodeP->nChildren; ++k) dumpTree ();
	--indent;
}
#endif


void GetDeviceTree ()
{
	static unsigned long zero = 0;
	char *deviceTreeBase = gKernelBSSAppendixP;

	traverseSubTree (VCALL(GetPeerPHandle) (0));
	VCALL(AppendBSSData) (&zero, sizeof (zero)); // Terminate structure

	gKernelParamsP->deviceTreeP = deviceTreeBase;
	gKernelParamsP->deviceTreeLength = gKernelBSSAppendixP - deviceTreeBase;
	printf ("DeviceTree base=0x%08X, length=%d bytes\n",
			gKernelParamsP->deviceTreeP,
			gKernelParamsP->deviceTreeLength);

#if DUMP_TREE
	cursorP = gKernelParamsP->deviceTreeP;
	printf ("Device Tree Dump:\n");
	dumpTree ();
#endif
}
