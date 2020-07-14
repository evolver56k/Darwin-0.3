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

/* 	Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved. 
 *
 * PPCMouse.m - PPC mouse driver.
 * 
 *
 * HISTORY
 * 14-Aug-92    Joe Pasqua at NeXT
 *      Created. 
 * 08-Feb-94	
 *	Modified for hppa (PS2)
 * 11-April-97   Simon Douglas
 *      Munged into ADB version. Doesn't depend on PPCKeyboard.
 */
 
// TO DO:
//   Make this an indirect client of ADB. ADB should probe to create multiple
//   event sources.
// Notes:
// * To find things that need to be fixed, search for FIX, to find questions
//   to be resolved, search for ASK, to find stuff that still needs to be
//   done, search for TO DO.
//

#define MACH_USER_API	1
#undef	KERNEL_PRIVATE

#import <driverkit/driverServer.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/interruptMsg.h>
#import <mach/mach_interface.h>
#import <bsd/dev/ppc/PPCMouse.h>
#import <bsd/dev/ppc/PCPointerDefs.h>
#import <bsd/dev/ppc/PPCKeyboard.h>
#import <bsd/dev/ppc/PPCKeyboardPriv.h>
#import <driverkit/ppc/directDevice.h>
#import <driverkit/ppc/driverTypes.h>

#include <mach_kdb.h>
#include <mach/mach_types.h>
#include <sys/tty.h>
#include <sys/conf.h>
#include <machdep/ppc/mach_param.h>
#include <machine/machspl.h>
#import <kern/thread_call.h>

#include "busses.h"
#include "adb.h"
#import  <bsd/dev/ppc/IOADBBus.h>

int set_handler(int device, int handler);

static PointerEvent event;

PPCMouse *_my_Mouse;

extern unsigned int wserver_on;

/* Forward declarations */
static void mouse_intr();

#define msg_printf printf


@implementation PPCMouse

static id  adb_driver;// pointer to adb indirect driver

+ (BOOL)probe:(IODeviceDescription *)deviceDescription
{
  BOOL return_code;

  adb_driver = [deviceDescription directDevice];

  return_code = [super probe: deviceDescription];

  return return_code;
}

+ (IODeviceStyle) deviceStyle
{
  return IO_IndirectDevice;
}

static Protocol *protocols[] = {
      @protocol(ADBprotocol),
      nil
};

+ (Protocol **)requiredProtocols
{
      return protocols;
}

//
// BEGIN:	Implementation of internal support methods
//

void mouse_adbhandler(int number, unsigned char *buffer, int count, void * ssp);

struct ADBDeviceData
{
    unsigned int	type;
    unsigned int	resolution;
    unsigned int	buttonCount;
};
typedef struct ADBDeviceData ADBDeviceData;
static ADBDeviceData deviceInfo[ADB_DEVICE_COUNT];

boolean_t   mouse_initted = FALSE;

- (void) mouseInit
{
    int i, ret;
    IOADBDeviceInfo  table[IO_ADB_MAX_DEVICE];
    unsigned char    buffer[8];
    int              length;

    resolution = 200;						// what event source sees
    [adb_driver adb_register_handler:ADB_DEV_MOUSE :mouse_adbhandler];

    [adb_driver GetTable:table :&i];

    for (i = 0; i < ADB_DEVICE_COUNT; i++) {

	int thisDeviceResolution, thisDeviceType, thisButtonCount;

        if ((table[i].flags & ADB_FLAGS_PRESENT) == 0)
            continue;
	if (table[i].originalAddress != ADB_DEV_MOUSE)
            continue;

	thisDeviceType = 2;
	thisDeviceResolution = 200;
	thisButtonCount = 1;

	ret = set_handler(i, 4);
	if( ret == ADB_RET_OK) {
	    adb_request_t   readreg;

	    thisDeviceType = 4;
	
	    [adb_driver readADBDeviceRegister:i :1 :buffer :&length];
	    if ( length == 8 ) {
	      thisDeviceResolution = (buffer[4] << 8) | buffer[5];
	      thisButtonCount = buffer[7];
	    }
	} else {
	    ret = set_handler(i, 2);
	    if( ret != ADB_RET_OK) {
		thisDeviceType = 1;
		thisDeviceResolution = 100;
	    }
	}
	deviceInfo[ i ].type 		= thisDeviceType;
	deviceInfo[ i ].resolution	= thisDeviceResolution;
	deviceInfo[ i ].buttonCount	= thisButtonCount;
   }
}


static void mouse_intr()
{
	[_my_Mouse interruptHandler];
}

void 
mouse_adbhandler(int number, unsigned char *buffer, int count, void * ssp)
{
    int i;

    /* If window server is not up no further processing is needed */
    if (wserver_on == 0) return;

    if ( buffer[0] & 0x80 ) event.b0 = TRUE;
    else event.b0 = FALSE;
    if ( buffer[1] & 0x80 ) event.b1 = TRUE;
    else event.b1 = FALSE;
    event.b2 = TRUE;
    event.b3 = TRUE;
    event.dx = buffer[1] & 0x7F;
    event.dy = buffer[0] & 0x7F;
    if ( count > 2 ) {                           // 3 or 4-button mouse
      if ( buffer[2] & 0x80 ) event.b2 = TRUE;
      else event.b2 = FALSE;
      if ( buffer[2] & 0x08 ) event.b3 = TRUE;
      else event.b3 = FALSE;
				// get higher order bits from subsequent bytes
      for ( i = 2; i < count; i++ ) {
         event.dy = event.dy + ((buffer[i] & 0x70) << ((3*i)-3));
         event.dx = event.dx + ((buffer[i] & 0x07) << ((3*i)+1));
      }
    }
				// sign-extend
    if ( event.dx & (1 << (count*3)) ) event.dx -= 1 << ((count*3)+1);
    if ( event.dy & (1 << (count*3)) ) event.dy -= 1 << ((count*3)+1);
   
    event.buttonCount = deviceInfo[ number ].buttonCount;
    
#if	DRVRKITINTS
	IOSendInterrupt(identity, state, IO_DEVICE_INTERRUPT_MSG);
#else
	thread_call_func((thread_call_func_t)mouse_intr,0, TRUE);
#endif	DRVRKITINTS

}

- (BOOL)mouseInit:deviceDescription
// Description:	Initialize the mouse object.
{
    if( mouse_initted == FALSE)
    {
	mouse_initted = TRUE;
    
	[self setDeviceKind:"PPCMouse"];
    
#ifdef	DRVRKITINTS
	[self enableAllInterrupts];
#endif	DRVRKITINTS
    
	[self mouseInit];
	
	_my_Mouse = self;
    }


    return YES;	
}

int
set_handler(int device, int handler)
{
      unsigned long   retval;
      unsigned char   value[8];
      int             length;

      retval = [adb_driver readADBDeviceRegister:device :3 :value :&length];

      if (retval != ADB_RET_OK)
	return retval;

      value[0] &= 0xF0;
      value[1] = handler;

      retval = [adb_driver writeADBDeviceRegister:device :3 :value :length];

      if (retval != ADB_RET_OK)
	return retval;

      retval = [adb_driver readADBDeviceRegister:device :3 :value :&length];

      if (retval != ADB_RET_OK)
	return retval;

      if (value[1] != handler)
	retval = ADB_RET_UNEXPECTED_RESULT;

      return( retval );
}


//
// END:		Implementation of internal support methods
//

//
// BEGIN:	Implementation of EXPORTED methods
//
- free
{
    return [super free];
}


- (BOOL) getHandler:(IOPPCInterruptHandler *)handler
              level:(unsigned int *) ipl
	   argument:(unsigned int *) arg
       forInterrupt:(unsigned int) localInterrupt
{
#ifdef	DRVRKITINTS
    *handler = PPCMouseIntHandler;
    *ipl = 3;
    *arg = 0xdeadbeef;
    return YES;
#endif	DRVRKITINTS
   return NO;
}


- (void)interruptHandler
{
    IOGetTimestamp(&event.timeStamp);
    if (target != nil)
    {
	[target dispatchPointerEvent:&event];
    }
}


- (int)getResolution
{
    return resolution;
}
//
// END:		Implementation of EXPORTED methods
//
@end
