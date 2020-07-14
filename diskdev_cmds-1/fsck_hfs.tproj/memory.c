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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef __TYPES__
typedef char *Ptr;
typedef Ptr *Handle;
typedef short OSErr;
#endif

#ifndef __MEMORY__
typedef long Size;
#endif

#ifdef MEMTEST
#define NewHandle xNewHandle
#define NewHandleSys xNewHandleSys
#define NewHandleClear xNewHandleClear
#define NewHandleSysClear xNewHandleSysClear
#define NewPtr xNewPtr
#define NewPtrSys xNewPtrSys
#define NewPtrClear xNewPtrClear
#define NewPtrSysClear xNewPtrSysClear
#define GetPtrSize xGetPtrSize
#define SetPtrSize xSetPtrSize
#define RecoverHandle xRecoverHandle
#define RecoverHandleSys xRecoverHandleSys
#define NewEmptyHandle xNewEmptyHandle
#define NewEmptyHandleSys xNewEmptyHandleSys
#define DisposePtr xDisposePtr
#define DisposeHandle xDisposeHandle
#define SetHandleSize xSetHandleSize
#define GetHandleSize xGetHandleSize
#define ReallocateHandle xReallocateHandle
#define EmptyHandle xEmptyHandle
#define PtrAndHand xPtrAndHand
#endif

#ifdef __GNUC__
#define pascal
#endif

extern pascal Handle NewHandle(Size byteCount);
extern pascal Handle NewHandleSys(Size byteCount);
extern pascal Handle NewHandleClear(Size byteCount);
extern pascal Handle NewHandleSysClear(Size byteCount);
extern pascal Ptr NewPtr(Size byteCount);
extern pascal Ptr NewPtrSys(Size byteCount);
extern pascal Ptr NewPtrClear(Size byteCount);
extern pascal Ptr NewPtrSysClear(Size byteCount);
extern pascal Size GetPtrSize(Ptr p);
extern pascal void SetPtrSize(Ptr p, Size newSize);
extern pascal Handle RecoverHandle(Ptr p);
extern pascal Handle RecoverHandleSys(Ptr p);
extern pascal Handle NewEmptyHandle(void);
extern pascal Handle NewEmptyHandleSys(void);
extern pascal void DisposePtr(Ptr p);
extern pascal void DisposeHandle(Handle h);
extern pascal void SetHandleSize(Handle h, Size newSize);
extern pascal Size GetHandleSize(Handle h);
extern pascal void ReallocateHandle(Handle h, Size byteCount);
extern pascal void EmptyHandle(Handle h);
extern pascal OSErr PtrAndHand(const void *ptr1, Handle hand2, long size);

pascal Handle NewHandle(Size byteCount)
{
	Handle h;
	Ptr p = NULL;

	if (byteCount < 0)
		return NULL;
		
	if (!(h = malloc(sizeof(Ptr) + sizeof(Size))))
		return NULL;
		
	if (byteCount)
		if (!(p = calloc(1, byteCount)))
		{
			free(h);
			return NULL;
		}
	
	*h = p;
	
	*((Size *)(h + 1)) = byteCount;	
	
	return h;
}

pascal Handle NewHandleSys(Size byteCount)
{
	return NewHandle(byteCount);
}

pascal Handle NewHandleClear(Size byteCount)
{
	return NewHandle(byteCount);
}

pascal Handle NewHandleSysClear(Size byteCount)
{
	return NewHandle(byteCount);
}

pascal Ptr NewPtr(Size byteCount)
{
	Ptr p;

	if (byteCount < 0)
		return NULL;
		
	if (!(p = calloc(1, byteCount + sizeof(Size))))
		return NULL;

	*((Size *)p) = byteCount;

	return p + sizeof(Size);
}

pascal Ptr NewPtrSys(Size byteCount)
{
	return NewPtr(byteCount);
}

pascal Ptr NewPtrClear(Size byteCount)
{
	return NewPtr(byteCount);
}

pascal Ptr NewPtrSysClear(Size byteCount)
{
	return NewPtr(byteCount);
}

pascal Size GetPtrSize(Ptr p)
{
	return *((Size *)(p - sizeof(Size)));
}

pascal void SetPtrSize(Ptr p, Size newSize)
{
}

pascal Handle NewEmptyHandle(void)
{
	return NewHandle(0);
}

pascal Handle NewEmptyHandleSys(void)
{
	return NewHandle(0);
}

pascal void DisposePtr(Ptr p)
{
	if (p)
		free(p - sizeof(Size));
}

pascal void DisposeHandle(Handle h)
{
	if (h)
	{
		if (*h)
			free(*h);
		free(h);
	}
}

pascal void SetHandleSize(Handle h, Size newSize)
{
	Ptr p = NULL;

	if (!h || newSize < 0)
		return;

	if ((p = realloc(*h, newSize)))
	{
		*h = p;
		*((Size *)(h + 1)) = newSize;
	}
}

pascal Size GetHandleSize(Handle h)
{
	return h ? *((Size *)(h + 1)) : 0;
}

pascal void ReallocateHandle(Handle h, Size byteCount)
{
	if (h)
	{
		EmptyHandle(h);

		if (byteCount < 1)
			return;
		
		*h = calloc(1, byteCount);
	
		*((Size *)(h + 1)) = byteCount;
	}
}

pascal void EmptyHandle(Handle h)
{
	if (h)
	{
		if (*h)
		{
			free(*h);			
			*h = NULL;	
			*((Size *)(h + 1)) = 0;	
		}
	}
}

pascal OSErr PtrAndHand(const void *ptr1, Handle hand2, long size)
{
	Ptr p = NULL;
	Size old_size = 0;

	if (!hand2)
		return -109;
	
	if (!ptr1 || size < 1)
		return 0;
		
	old_size = *((Size *)(hand2 + 1));

	if (!(p = realloc(*hand2, size + old_size)))
		return -108;

	*hand2 = p;
	*((Size *)(hand2 + 1)) = size + old_size;
	
	memcpy(*hand2 + old_size, ptr1, size);
	
	return 0;
}


#ifdef MEMTEST

void main(void)
{
	char p[] = "Hello World";

	Handle h1 = NewHandle(16);
	Size n1 = GetHandleSize(h1);

	Handle h2 = NewEmptyHandle();
	Size n2 = GetHandleSize(h2);

	Handle h3 = NewHandle(64);
	Size n3 = GetHandleSize(h3);

	Handle h4 = NewHandle(16);
	Size n4 = GetHandleSize(h4);

	Ptr p1 = NewPtr(32);
	Size n5 = GetPtrSize(p1);

	DisposeHandle(h1);
	DisposeHandle(h2);
	EmptyHandle(h3);
	DisposeHandle(h3);
	
	PtrAndHand(p,h4,sizeof(p));
	SetHandleSize(h4,64-6);
	DisposeHandle(h4);
	
	DisposePtr(p1);
}

#endif