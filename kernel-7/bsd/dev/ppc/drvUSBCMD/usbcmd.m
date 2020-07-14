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

/*********

usbcmd.m
This is a preliminary driver for the CMD PCI USB card on my
PowerSurge 8500.

	Common USB terms:  TD is Transfer Descriptor, ED is Endpoint Descriptor.
* Naga Pappireddi 12-17-98 Radar bug# 2293217   Cleanup of code in GetErrataBits function to read the registry properties
*                           this cleanup took care of LED problem on Yosemite 

*********/


#import <kern/clock.h>
#import <machdep/ppc/interrupts.h>
#import <kernserv/prototypes.h>
#import <kernserv/clock_timer.h>
#import <kernserv/ns_timer.h>
#import <sys/time.h>
#import <sys/callout.h>
#import <machdep/ppc/proc_reg.h>
#import <driverkit/generalFuncs.h>
#import <driverkit/kernelDriver.h>
#import <driverkit/interruptMsg.h>
#import <driverkit/ppc/IODBDMA.h>  //for ReadSwap32() only
#import <driverkit/align.h>  //for IOAlign() only
#import "usbcmd.h"

extern unsigned char usb_2_adb_keymap[];  // 3/19/99 Canadian changes
extern id ApplePMUId;	//in adb.m (remove this when adb becomes an indirect driver)(q8q)
extern id usb_UIM_object; //in bsd/dev/ppc/EventSrcPCKeyboard.m
extern unsigned int usb_kbd_vendor_id, usb_kbd_product_id; //in uslExpert.c  

int     OHCIUIMInitialize(unsigned long ioBase);
void pollRootHubSim(void);
IOThreadFunc usb_thread(void *);
void IOSync(void)
{
eieio();
}
extern OHCIUIMDataPtr                          pOHCIUIMData;
extern void kprintf(const char *, ...);
extern void bcopy(void *, void *, int);
extern void usb_intr(int intr, void *ssp, void *dev);
#define printf usb_donone 
#define kprintf usb_donone
void usb_donone(char *x,...);


//OSStatus OHCIUIMInterruptHandler (InterruptSetMember member, void *refCon, UInt32 interruptCount);
extern int kdp_flag;
int usb_port_configured[2] = {0,0};

IOPropertyTable *usb_propTable;
IOPCIDevice *devUsb;
AppleUSBCMD *selfUsb;
int  cmd_pci_errata;  //global needed to fix errata for Caps Lock LED support

unsigned int
usb_swap_word(unsigned int  input)
{
    int lo, hi;  

    lo = 0x00ff & input;
    lo = lo << 8;       
    hi = input >> 8;
    return (hi | lo);     
}

void enable_intr()
{
    [selfUsb enableAllInterrupts];
}
     
void ReadConfigVal(UInt32 ctl, UInt32 *val)
{
    [devUsb configReadLong:ctl value:val];
}
       
void WriteConfigVal(UInt32 ctl, UInt32 val)
{
    [devUsb configWriteLong:ctl value:val];
}

@implementation AppleUSBCMD

// **********************************************************************************
// probe
//
// 
//
// **********************************************************************************
+ (Boolean) probe : deviceDescription
{
  id dev;
//kdp_flag=2;
kprintf("usb probe\n");
    usb_propTable = [deviceDescription propertyTable]; /* For use with RegistryPropertyGet */

	if ( (dev = [ self alloc ]) == nil ) {
		return NO;
	}

	if ([dev initFromDeviceDescription:deviceDescription] == nil) {
		return NO;
	}
	
    usb_UIM_object = dev;

kprintf("naga:Successful probe of USB\n");
	return YES;
}


// **********************************************************************************
// initFromDeviceDescription
//
// 
//- initFromDeviceDescription:(IODeviceDescription *)deviceDescription
//- initFromDeviceDescription:(IOPCIDevice *)deviceDescription
//
// **********************************************************************************
- initFromDeviceDescription:(IODeviceDescription *) devDesc 
{
	IORange *	ioRange;
	int			numRanges;
        int irq;
	unsigned long		PCICommandReg;
	IOPCIDevice *deviceDescription = (IOPCIDevice *)devDesc;
	char	*tbuf;


        devUsb = (IOPCIDevice *)devDesc;
        selfUsb=self;
	if ( [super initFromDeviceDescription:deviceDescription] == nil ) 
        {
		[self free];
		return nil;
	}

  	[self setDeviceKind:"USB-CMD Subsystem"];
  	[self setLocation:NULL];
  	[self setName:"USBCMD"];  

	tbuf = [devDesc nodeName];
//CMD card is "pci1095,670"  
	cmd_pci_errata = 0;
	if (!strcmp(tbuf, "pci1095,670")) //if match Open Firmware string
	{
		cmd_pci_errata = 1;
	}


	ioRange = [deviceDescription memoryRangeList];
	//ioRange->start is 0x80800000 for my PCI USB card
	kprintf("USB Start of mem is %x\n", ioRange->start);

        numRanges = [deviceDescription numMemoryRanges];
	kprintf("USB numRanges is %d\n", numRanges);
    // This causes crash [self isPCIPresent];

       if (numRanges < 1 )
       {
          IOLog( "USB-CMD: Incorrect deviceDescription - 1\n\r");
          return nil;
       }
       ioBase = 0;
       [self mapMemoryRange: 0 to:(vm_address_t *)&ioBase findSpace:YES cache:IO_CacheOff];
       kprintf("USB ioBase is %x\n", ioBase); //Should be 3519000 on PowerSurge (Heathrow)

    /*
     * BUS MASTER, MEM I/O Space, MEM WR & INV
     */

    [deviceDescription configWriteLong:0x04 value:0x16];
#ifdef OMIT
    [deviceDescription configReadLong:0x04 value:&PCICommandReg];
    PCICommandReg |= (4 | 1);  // Master & Memory
    PCICommandReg &= ~2;       // Not I/O
    [deviceDescription configWriteLong:0x04 value:PCICommandReg];
#endif

OHCIUIMInitialize( ioBase);
USBServicesInitialise(nil);

if ([self startIOThread] != IO_R_SUCCESS)
{       
    [self free];
    kprintf("USB thread error \n");
    return nil;
}
 port = IOConvertPort([self interruptPort],IO_KernelIOTask,IO_Kernel);
	kprintf("USB port is %x\n", port);
	[self registerDevice];
        irq = [devDesc interrupt];
        {
             IOThread tid;
             tid = IOForkThread(usb_thread, (void *)0);
        }
	return self;
}

// **********************************************************************************
// free
//
// 
//
// **********************************************************************************
- free
{
	return [ super free ];
}



- (void)interruptOccurred
{
void dump_regs(UInt32 *);
extern int usb_iodone;
void wait_usbio();
void print_usbDesc();
OHCIUIMInterruptHandler (0, NULL , 0);
[self enableAllInterrupts];
}


//New method added 12/22/98 by A.W. to support international USB keyboards
//NOTE: ADB keyboard keymaps are in /System/Library/Keyboards/*.keyboard
// Apple Extended Keyboard, North American version has ID of 16+2 = 18
- (unsigned int) getADBKeyboardID
{
//kprintf("USB: in usbcmd.m, vendor is %x, product is %x\n", usb_kbd_vendor_id, usb_kbd_product_id);

	if (usb_kbd_vendor_id == 0x05ac)  //Apple Corp. vendor ID
	{
		switch (usb_kbd_product_id)
		{
			case 0x0201: //North American iMac keyboard
				return 18;
			case 0x0203: //38=Japanese.  34 is adjustable JIS
				return 34;
			case 0x0202: //Arabic
				//3/19/99 A.W. last-minute changes for Canadian keyboard
				//Swap two ISO keys around
				usb_2_adb_keymap[0x35] = 0x0a;  //Cosmo key18 swaps with key74, 0a is ADB keycode
				usb_2_adb_keymap[0x64] = 0x32; 
				return 21; //21 is Apple ISO Extended Keyboard
			default:
				return 18;
		}
	}

	if (usb_kbd_vendor_id == 0x045e)  //Microsoft ID
	{
		if (usb_kbd_product_id == 0x000b)	//Natural USB+PS/2 keyboard
			return 18;
	}	
	//By default return Apple Extended ADB Keyboard ID
	return 18;
}


@end

IOThreadFunc usb_thread(void *ignore)
{
   extern void Get_Config_Info(void);
   extern void usbAddBus(void);
   extern char ports_connected[2];
   extern USBIdleTask(void);

   kprintf("Usb Thread Started\n");
   usbAddBus();
kprintf("***************    usbAddBus complete\n");
   for(;;)
   {
       USBIdleTask();

       IOSleep(1);   // Milliseconds 
   }
}
/*  Moved this function from ohciuim.c to here 
* Naga Pappireddi 12-17-98 Radar bug# 2293217   Cleanup of code in GetErrataBits function to read the registry properties
*                           this cleanup took care of LED problem on Yosemite
*/

OSStatus RegistryPropertyGet(const RegEntryID *entryID, const RegPropertyName *propertyName, void *propertyValue, RegPropertyValueSize *propertySize)
{   
void *prop_value;

prop_value = (void *)propertyValue;
[usb_propTable getProperty:propertyName flags:0 value:(void **) &prop_value length:propertySize];

return 0;       
}

