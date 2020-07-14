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

/*
	File:		uslPipeState.c

	Contains:	xxx put contents here xxx

	Version:	xxx put version here xxx

	Copyright:	© 1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(BT)	Barry Twycross

	Change History (most recent first):

	  <USB3>	 8/13/98	BT		Add multibus support
	  <USB2>	 7/28/98	BT		Fix spurious errors
	  <USB1>	 4/26/98	BT		first checked in
*/

#include "../USB.h"
#include "../USBpriv.h"

#include "uslpriv.h"
#include "../uimpriv.h"


void uslSetPipeStall(USBPipeRef ref)
{
pipe *thisPipe;
OSStatus err = noErr;
	err = findPipe(ref, &thisPipe);
	if(thisPipe != nil)
	{
		if(thisPipe->state == kUSBActive)
		{
			thisPipe->state = kUSBStalled;
		}
		else if(thisPipe->state == kUSBIdle)
		{
			thisPipe->state = kUSBIdleStalled;
		}
	}

}

OSStatus USBAbortPipeByReference(USBReference refIn)
{
pipe *thisPipe;
USBReference ref;
OSStatus err = noErr;

	do{
			/* Is this a device ref ? */
		ref = getPipeZero(refIn);
		if(ref == 0)
		{
				/* No, is it a pipe ref? */
			ref = refIn;
		}
			/* Find the pipe struct */
		err = findPipe(ref, &thisPipe);
		if(thisPipe == nil)
		{
			break;
		}
		
		err = UIMAbortEndpoint(thisPipe->bus, thisPipe->devAddress, thisPipe->endPt, thisPipe->direction);
	}while(0);

	return(err);
}

OSStatus USBClearPipeStallByReference(USBPipeRef ref)
{
pipe *thisPipe;
OSStatus err = noErr;

	do{
			/* Note this can't be a device ref, they're automatically cleared */
			/* Find the pipe struct */
		err = findPipe(ref, &thisPipe);
		if(err != kUSBPipeStalledError)
		{
			break;
		}
				
		err = UIMClearEndPointStall(thisPipe->bus, thisPipe->devAddress, thisPipe->endPt, thisPipe->direction);
		if(err == noErr)
		{
			if(thisPipe->state == kUSBIdleStalled)
			{
				thisPipe->state = kUSBIdle;
			}
			else
			{
				thisPipe->state = kUSBActive;
			}
		}	
	}while(0);

	return(err);
}

OSStatus USBSetPipeIdleByReference(USBPipeRef ref)
{
pipe *thisPipe;
OSStatus err = noErr;

	do{
			/* Note this can't be a device ref, they're automatically cleared */
			/* Find the pipe struct */
		err = findPipe(ref, &thisPipe);
		if(thisPipe == nil)
		{
			break;
		}
		
		err = noErr;
		
		if(thisPipe->state == kUSBStalled)
		{
			thisPipe->state = kUSBIdleStalled;
		}
		else
		{
			thisPipe->state = kUSBIdle;
		}
	}while(0);

	return(err);
}

OSStatus USBSetPipeActiveByReference(USBPipeRef ref)
{
pipe *thisPipe;
OSStatus err = noErr;

	do{
			/* Note this can't be a device ref, they're automatically cleared */
			/* Find the pipe struct */
		err = findPipe(ref, &thisPipe);
		if(thisPipe == nil)
		{
			break;
		}

		if(thisPipe->state == kUSBIdleStalled)
		{
			thisPipe->state = kUSBStalled;
			// do nothing, err is stalled err.
		}
		else if(thisPipe->state == kUSBStalled)
		{
			// do nothing, err is stalled err.
		}
		else if(thisPipe->state == kUSBIdle)
		{
			thisPipe->state = kUSBActive;
			err = noErr;
		}

	}while(0);

	return(err);
}



OSStatus USBResetPipeByReference(USBReference refIn)
{
pipe *thisPipe;
USBReference ref;
OSStatus err = noErr;

	do{
			/* Is this a device ref ? */
		ref = getPipeZero(refIn);
		if(ref == 0)
		{
				/* No, is it a pipe ref? */
			ref = refIn;
		}
			/* Find the pipe struct */
		err = findPipe(ref, &thisPipe);
		if(thisPipe == nil)
		{
			break;
		}
		if(err == kUSBPipeStalledError)
		{
			err = USBClearPipeStallByReference(ref);
		}
		else
		{
			err = noErr;
		}
		if(err == noErr)
		{
			thisPipe->state = kUSBActive;
		}

	}while(0);

	return(err);
}

OSStatus USBClosePipeByReference(USBPipeRef ref)
{
pipe *thisPipe;
OSStatus err = noErr;

	do{
			/* Note this can't be a device ref, they're automatically cleared */
			/* Find the pipe struct */
		err = findPipe(ref, &thisPipe);
		if(thisPipe == nil)
		{
			break;
		}
		
		err = uslClosePipe(recoverPipeIdx(ref));
	}while(0);

	return(err);
}


OSStatus USBGetPipeStatusByReference(USBReference refIn, USBPipeState *state)
{
pipe *thisPipe;
USBReference ref;
OSStatus err = noErr;

	do{
			/* Is this a device ref ? */
		ref = getPipeZero(refIn);
		if(ref == 0)
		{
				/* No, is it a pipe ref? */
			ref = refIn;
		}
			/* Find the pipe struct */
		err = findPipe(ref, &thisPipe);
		if(thisPipe == nil)
		{
			break;
		}
		
		if(thisPipe->state == kUSBIdleStalled)
		{
			*state = kUSBStalled;
		}
		else
		{
			*state = thisPipe->state;
		}
		
	}while(0);

	return(err);
}



