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
#ifndef HFSSUPPORT_H
#define HFSSUPPORT_H

#include "SecondaryLoader.h"

int fPStringsArePreciselyEqual (StringPtr string1, StringPtr string2);
void fGetFileExtents (ExtentsArray *extentsTreeExtentsP, UInt32 fileID,
					  ExtentsArray *extentsBufferP);
int fReadHFSNode (UInt32 nodeNumber, ExtentsArray *extentsBufferP,
				  BTreeNode *nodeBufferP);
UInt32 fFindLeafNode (ExtentsArray *extentsBufferP, CatalogKey *keyToFindP,
					  int (*compareFunction) (CatalogKey *key1, CatalogKey *key2),
					  BTreeNode *nodeBufferP, int *nRecordsP);
int fFileIDCompare (CatalogKey *key1, CatalogKey *key2);
int fExtentsIDCompare (CatalogKey *key1, CatalogKey *key2);
int fFileNameCompare (CatalogKey *key1, CatalogKey *key2);
void *fFindBTreeRecord (BTreeNode *nodeBufferP, int recordNumber);
FileRecord *fFindFileRecord (UInt32 node, BTreeNode *nodeBufferP,
							 ExtentsArray *catalogExtentsP, CatalogKey *leafKeyP);

#endif
