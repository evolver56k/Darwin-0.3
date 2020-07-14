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
	File:		OHCIUIM.c

	Contains:	OHCI implementation of USB Hal for Neptune Project

	Version:	xxx put version here xxx

	Written by:	Guillermo Gallegos

	Copyright:	© 1997-1998 by Apple Computer, Inc., all rights reserved.

	File Ownership:

		DRI:				xxx put dri here xxx

		Other Contact:		xxx put other contact here xxx

		Technology:			xxx put technology here xxx

	Writers:

		(TC)	Tom Clark
		(DRF)	Dave Falkenburg
		(DF)	David Ferguson
		(GG)	Guillermo Gallegos
		(BT)	Barry Twycross
		(CJK)	Craig Keithley

	Change History (most recent first):

	 <USB89>	11/20/98	GG		[2287626]  Fixed bug where I was instantiating the same callback
									twice, which was causing a crash with the Imation Drive.
	 <USB88>	11/18/98	GG		[2283557]  Changed the calculation of the frame number to
									account for use of bit 15 of the frame number register as a
									toggle overflow. Should fix bug where the framenumber was too
									high.
	 <USB87>	11/16/98	GG		Fixed off by one error in isochronous transfers and cleaned up
									finalize routines memory deallocations.
	 <USB86>	11/12/98	GG		Fixed memory leaks in descriptor allocation, PCI allocatio and
									pphysical to virtual allocations.
	 <USB85>	11/11/98	GG		Fixed how ischronous tranfers get the end of there buffer
									calculated for each TD.
	 <USB84>	 11/9/98	CJK		Back out some changes to rev 80.  Should make bulk work again.
	 <USB83>	 11/5/98	GG		Fix bug where Bulk didnt work due to bug introduced in 81.  Also
									cleaned up a little.
	 <USB82>	10/24/98	GG		Changed power on of root hub to call rootHubPower, instead of
									tripping the bit directly. Also added a function to find out
									what machine we are on.
	 <USB81>	10/22/98	GG		Fixed bug where pages needed would be mis calculated if the the
									buffer ended on the first byte of a page.
	 <USB80>	 9/14/98	GG		Fix Isoch bug where Isochronous was being disabled when Opti
									errata fix was being implemented.
	 <USB79>	  9/9/98	BT		Fix Isoc
	 <USB78>	  9/9/98	BT		Fix size of Isoc refcon
	 <USB77>	  9/5/98	GG		Some more Isoch fixes.
	 <USB76>	  9/4/98	GG		Fix page used calcualation and internal loop condition.
	 <USB75>	  9/3/98	GG		Add Isochronous Suppurt for Create, Abort, Delete endpoint and
									create transfers.  Also rolled in earlier branch fix to TOT, the
									off by 2 bug.
	 <USB74>	 8/12/98	GG		Initialize pointer in zero length bulk transfer condition.
	 <USB73>	 8/12/98	BT		Move root hub into UIM again.
	 <USB72>	 8/11/98	GG		Fixed the original Compiles and works with symbols, but compiles
									but not works without symbols.  Turns out to be an uninitialized
									pointer to the register set.
	 <USB71>	 8/11/98	GG		Commented out a for(j=0;j<0;j++) loop that the compiler did not
									know how to optimize, with symbols on it worked, symbols off it
									didnt.
	 <USB70>	 8/11/98	GG		stuff
	 <USB69>	  8/7/98	GG		Fixed Opti data corruption problem. Also fixed bug where an
									aborted transfer would return wrong value for pending data.
									Tweaked interface to USL to be consistent with direction
									parameter.
	 <USB68>	 7/28/98	GG		Merged to TOT: <USB62a2> 7/15/98 GG Fixed bug that was caused by
									previous checkin where transfer setup would try to use data
									outsided the preparememforio physical data block. <USB62a1>
									7/14/98 GG [2252282] <TC> Fixed Bulk to not generate short
									packets in the middle of a tranfer, both read and write. This
									was caused when the data buffer was not aligned in memory to max
									packet size. The fix was to check for unalignment and to
									generate 4k or less Transfer Descriptors.
	 <USB67>	 7/23/98	BT		fix fix
	 <USB66>	 7/23/98	BT		Fix Endpoint Delete to not hose bulk and control queues
	 <USB65>	 7/10/98	TC		Back out <USB63>
	 <USB64>	 6/30/98	BT		Tidy up
	 <USB63>	 6/30/98	BT		Move Root hub sim into UIM
	 <USB62>	 6/24/98	GG		Changed PeriodStart Register to static value instead of
									calculating it.
	 <USB61>	 6/23/98	DF		[2238756, 2243961]  always reset the endpoint virtual head when
									deleting TDs.
	 <USB60>	 6/15/98	DF		Turn off power when finializing the UIM
	 <USB59>	 6/14/98	DF		Remove unused variable that was causing a warning
	 <USB58>	 6/14/98	DF		Shutdown the OHCI controller when Finalize function is called.
	 <USB57>	 6/13/98	BT		Stop hanging is we don't see a start of frame interrupt.
	 <USB56>	  6/4/98	DF		turn off debugstrs and other clean up
	 <USB55>	  6/4/98	DRF		In UIMInitialize & UIMFinalize, add a Ptr to facilitate
									handing-off global state when performing replacement. NOTE:
									These routines still to be changed to do the additional work.
	 <USB54>	  6/3/98	GG		Changed UInt64 operations to use macros from Math64.h.  Made
									interrupts go to their requested period and randomly generated
									slot with given period.
	 <USB53>	  6/3/98	GG		Checked if UInt64 is a structure or integer.
	 <USB52>	  6/3/98	DF		[2240506]  clear skipped bit when aborting transfers, also
									slight tweek to the frame number stuff to work with the
									compilers used when building in SuperMario.
	 <USB51>	  6/2/98	GG		Add interfaces IsochEndpointCreate, and IsochTransfer.  Also
									added a getFrameCount function.
	 <USB50>	  6/2/98	DRF		Added params to UIMInitialize & UIMFinalize for replacement. Use
									them later
	 <USB49>	 5/16/98	DF		For now, always call CheckPointIO after getting the physical
									address of transfer buffers.  PrepareMemoryForIO and
									CheckpointIO are not implemented as advertised that's why we are
									having so much trouble with them.  The real solution is to carry
									a copy of the ioPreparation table with each active transfer
									descriptor.
	 <USB48>	 5/15/98	DF		make the bus operational following an unrecoverable error
									interrupt, fix TD leak in bulk transfers, turn back on the
									CheckpointIOs, misc cleanups.
	 <USB47>	 5/14/98	GG		Added BufferUnderrun Errata Fix for CMD KAB4 and KAB4 revA.
									Also increased memory usage and added some checks for out of
									memory cases.  Took out CheckpointIO usage for now.
	 <USB46>	 5/14/98	DF		Initialize the root hub status register
	 <USB45>	  5/8/98	DF		Make sure interrupt transfer descriptors use the endpoint for
									the data toggle carry.
	 <USB44>	  5/7/98	BT		Hack find interrupt endpoint so it never finds control endpoint
									for (0,0).
	 <USB43>	  5/5/98	GG		Change Buffersize from short to unsigned long in BulkTransfer.
	 <USB42>	  5/4/98	GG		Another fix for Bulk transfers in done Q processing.
	 <USB41>	  5/4/98	GG		Fixed Done Queue Processing to do error recovery.
	 <USB40>	  5/4/98	DF		fix errata test, and check the Bulk list when clearing endpoint
									stalls
	 <USB39>	  5/2/98	DF		fix bulk transfers, add errata checks, remove VooDoo, add
									CheckPointIO() calls when TDs are released.
	 <USB38>	 4/29/98	BT		Fix returned errors, hasck in FSMPS
	 <USB37>	 4/29/98	GG		Added calculation of FSMPS based on FMInterval.
	 <USB36>	 4/27/98	DF		Initialize the Control & Bulk Current Pointers
	 <USB35>	 4/24/98	GG		Added support for bulk transfers greater than 4k.  Added fix to
									support to clear an endpoint stall.
	 <USB34>	 4/22/98	DF		Mask off low-order interrupt bits when reading the hccaDoneHead.
									Also change to match pci class code instead of a specific
									vendor's chip.
	 <USB33>	 4/16/98	BT		Fix codewarrior incompatabilities
	 <USB32>	 4/14/98	DF		make ProcessDoneQueue do something useful
	 <USB31>	  4/9/98	BT		Use USB.h
		<30>	  4/8/98	GG		Modified OHCIUIMEndpointDelete to not be static.
		<29>	  4/7/98	GG		Added support for OF generic driver.  Added suypport for UHCI to
									OHCI mode.
		<28>	  4/7/98	GG		Added Abort and Delete Apis.
		<27>	 3/19/98	BT		Split UIM into UIM and root hub parts
		<26>	 3/18/98	BT		Add reset enable change to root hub.
		<25>	 3/11/98	BT		Root hub interrupt transaction simulation support
		<24>	 2/25/98	GG		Re enabled queue processing.
		<23>	 2/25/98	GG		Modified Interrupt transfer to call a separate troutine to find
									an enpoint.
		<22>	 2/24/98	GG		Calculated Periodic Start using Frame Interval.  Changed
									calculation of Interrupt tree.
		<21>	 2/23/98	GG		Added checks for task time.  Changed Interrupt heads to be an
									array of structs.  Changed the value of hcPeriodicStart register
									to spec static value.  Corrected interrupt tree structure.
		<20>	 2/20/98	GG		Fixed compile problem.
		<19>	 2/20/98	GG		Changed DelayFor to DelayForHardware, modifed
									InterruptInitialize.
		<18>	 2/20/98	GG		Changed to CMD from Opti.
		<17>	 2/20/98	GG		Enabled periodic endpoint processing.
		<16>	 2/20/98	GG		Added Interrupt Transfer Support.  Modified Interrupt Endpoint
									Creation.
		<15>	 2/19/98	GG		Added endpoint creation functonality.
	 <USB14>	 2/17/98	DF		Add HW Interrupt handling, and a few changes related to getting
									operation in a VM environment working.
		<13>	 2/13/98	GG		Comment out debug strings
		<12>	 2/13/98	GG		Comment out debug strings
		<11>	 2/12/98	GG		Added Rom in Ram memory offset support.  Overwrote #10 and 9
									Checkins.
	 <USB10>	 2/12/98	DF		Detect unrecoverable error at initialize time, and try to
									recover by resetting
	  <USB9>	 2/12/98	DF		Add more error checking on memory allocations
		 <8>	  2/2/98	BT		Add bulk stuff
		 <7>	 1/26/98	BT		Add unstall endpoint
		 <6>	12/19/97	BT		Temp hack, Make it a shared lib
		 <5>	12/18/97	BT		Add in my changes so this works with multiple endpoints
		 <4>	11/30/97	BT		Various mods to get perliminary USL running
		<3*>	11/20/97	GG		Add callback parameter to bulk and control creators.
		 <3>	11/20/97	GG		Initial check in has minimal support for Root Hub, Control and
									Bulk transfers.
		 <2>	11/20/97	GG		Filled in Contains Field.
		 <1>	11/20/97	GG		first checked in

*/

#import <mach/mach_types.h>
#import <architecture/arch_types.h>
#import <machdep/ppc/interrupts.h>
#import <kernserv/prototypes.h>
#import <kernserv/clock_timer.h>
#import <kernserv/ns_timer.h>
#import <sys/time.h>     
#import <sys/callout.h>
#include <bsd/sys/systm.h>
#import <sys/proc.h>    // prototypes for sleep,wakeup                  
#import <sys/fcntl.h>                                                   
#include <vm/vm_kern.h>
#import <driverkit/generalFuncs.h>
                
#include "driverservices.h"
#include "USB.h"
#include "USBpriv.h"
#include "OHCIUIM.h"     
#include "OHCIRootHub.h"
#include "pci.h"                                                        

#define  ProgressDebugStr(s)   kprintf(s)         /* SysDebugStr(s)    */
#define  MemoryDebugStr(s)    kprintf(s)          /* SysDebugStr(s)    */
#define  AssertDebugStr(s)   kprintf(s)                 /* SysDebugStr(s)    */
#define  FailureDebugStr(s)  kprintf(s)          /* SysDebugStr(s)    */

void IOSync(void);       
simple_lock_t usb_slock; 
char Desc_Buffer_Usb[54*4096];  //naga  
extern OHCIUIMDataPtr                           pOHCIUIMData;
#define KBD_UNIQUE_ADDRESS    10
#define MOUSE_UNIQUE_ADDRESS  0x61




static OSStatus OHCIUIMFinalizeOHCIUIMData(
	OHCIUIMDataPtr				pOHCIUIMData);

static OSStatus	OHCIUIMGetRegisterBaseAddress (
	RegEntryIDPtr				pOHCIRegEntryID,
	OHCIRegistersPtr			*ppOHCIRegisters);

UInt32 OHCIUIMCreatePhysicalAddress(
	UInt32						LogicalAddress,
	UInt32						count);
	
UInt32 OHCIUIMGetPhysicalAddress(
	UInt32						LogicalAddress,
	UInt32						count);

UInt32 OHCIUIMGetLogicalAddress (
	UInt32 				pPhysicalAddress);

OSStatus OHCIUIMAllocateBuffer();

OSStatus OHCIUIMDeallocateED (
	OHCIEndpointDescriptorPtr pED);
	
OSStatus OHCIUIMDeallocateTD (
	OHCIGeneralTransferDescriptorPtr pTD);
	
OSStatus OHCIUIMDeallocateITD (
	OHCIIsochTransferDescriptorPtr pTD);
	
OHCIEndpointDescriptorPtr OHCIUIMAllocateED();

OHCIGeneralTransferDescriptorPtr OHCIUIMAllocateTD();

OHCIIsochTransferDescriptorPtr OHCIUIMAllocateITD();

OSStatus OHCIUIMAllocateMemory (
	int 								num_of_TDs,
	int									num_of_EDs,
	int									num_of_ITDs);

OSStatus OHCIUIMInterruptFindED();

OSStatus OHCIUIMInterruptBandwidthCheck();

OSStatus OHCIUIMInterruptfits();

OSStatus OHCIUIMInterruptschedule();

OSStatus OHCIUIMIsochronousInitialize();
OSStatus OHCIUIMInterruptInitialize ();
OSStatus OHCIUIMBulkInitialize ();
OSStatus OHCIUIMControlInitialize ();

OSStatus OHCIUIMInstallInterruptHandler();

OSStatus OHCIUIMInterruptHandler (InterruptSetMember member, void *refCon, UInt32 interruptCount);

OSStatus DoDoneQueueProcessing(OHCIGeneralTransferDescriptorPtr pHCDoneTD, Boolean immediateFlag);

Boolean BandwidthAvailable(
	UInt32			pollingRate, 
	UInt32			reserveBandwidth,
	int				*offset);

OHCIEndpointDescriptorPtr FindEndpoint (
	short 						functionNumber, 
	short 						endpointNumber,
	short 						direction, 
	OHCIEndpointDescriptorPtr 	*pEDQueueBack, 
	UInt32 						*controlMask);


OHCIEndpointDescriptorPtr FindInterruptEndpoint(
	short 						functionNumber,
	short						endpointNumber,
	OHCIEndpointDescriptorPtr	*pEDBack);

OHCIEndpointDescriptorPtr FindBulkEndpoint (
	short 						functionNumber, 
	short						endpointNumber,
	short						direction,
	OHCIEndpointDescriptorPtr   *pEDBack);

OHCIEndpointDescriptorPtr FindControlEndpoint (
	short 						functionNumber, 
	short						endpointNumber, 
	OHCIEndpointDescriptorPtr   *pEDBack);

OHCIEndpointDescriptorPtr FindIsochronousEndpoint(
	short 						functionNumber, 
	short 						endpointNumber,
	short 						direction, 
	OHCIEndpointDescriptorPtr	*pEDBack);

static OSStatus RemoveTDs (OHCIEndpointDescriptorPtr pED);

static OSStatus RemoveAllTDs (OHCIEndpointDescriptorPtr pED);

static UInt32 GetErrataBits (RegEntryIDPtr);

void doCallback(
	OHCIGeneralTransferDescriptorPtr 			nextTD,
	UInt32 			transferStatus,
	UInt32 			bufferSizeRemaining);

OHCIEndpointDescriptorPtr AddEmptyEndPoint(
	UInt8 						functionAddress,
	UInt8						endpointNumber,
	UInt16						maxPacketSize,
	UInt8						speed,
	UInt8						direction,
	OHCIEndpointDescriptorPtr	pED,
	int							isoch);

OHCIIsochTransferDescriptorPtr QueueITD (
	OHCIIsochTransferDescriptorPtr 	pITD, 
	OHCIEndpointDescriptorPtr 		pED);

void SwapIsoc(OHCIIsochTransferDescriptorPtr pITD);

UInt32 findBufferRemaining (OHCIGeneralTransferDescriptorPtr pCurrentTD);

void DoOptiFix(OHCIEndpointDescriptorPtr pIsochHead);

void trivialSwap( UInt32 *thing);

int GetEDType(
	OHCIEndpointDescriptorPtr pED);

void ProcessCompletedITD (OHCIIsochTransferDescriptorPtr pITD);

OSStatus OptiLSHSFix();

Boolean AmIThisMachine(Str255 inMachineNameStr);

OSStatus OHCIResetRootHub(uslBusRef bus);
OSStatus OHCIRootHubPortEnable( uslBusRef bus, short port, Boolean on);

void Get_Config_Info(void);
void enable_intr();
void ReadConfigVal(UInt32 ctl, UInt32 *val);
void WriteConfigVal(UInt32 ctl, UInt32 val);

typedef struct{
        UInt8 len;
        UInt8 type;
        UInt16 totalLen;
        char rest_data[34];   /* = totalLen for mouse  naga needs change later */
        } configHeader;     
//configHeader usbConfDesc;

char usb_data_buffer[8];    /* General purpose buffer */
int usb_task_level = kKernelLevel;

void usb_iodone_wakeup(long lParam, OSStatus status, short s16Param);
int usb_iodone = 1;
char ports_connected[2] = {0,0};
usbControlBits usbConfig;       /* 8 bytes */
USBDeviceDescriptor  usbDesc;   /* 18 bytes */
struct usbConfDesc
{
     USBConfigurationDescriptor   header;
     USBInterfaceDescriptor       interface;
     USBEndPointDescriptor       endpoint;
     char   rest_data[29];         
}  usbConfDesc;
void usb_donone(char *x,...)
{
}
void fill_usbDesc()
{
int i;
for(i=0;i<18;++i)
{
    char *x = (char *)&usbDesc;
    *x++ = 0xbc;
}
}    
void print_usbDesc(char *usbDesc,int size)
{
int i;

for(i=1;i<=size;++i)
{
    kprintf("0x%02x ",(int) *usbDesc++);
        if(i%10 == 0)kprintf("\n");
}
kprintf("\n");
}

void print_td(OHCIGeneralTransferDescriptorPtr x)
{
   // kprintf("TD(0x%08x):w0=0x%08x,w1(cbp)=0x%08x,w2=0x%08x,w3=0x%08x\n",x,EndianSwap32Bit(x->dWord0),EndianSwap32Bit(x->dWord1),EndianSwap32Bit(x->dWord2),EndianSwap32Bit(x->dWord3));
}

void print_ed(OHCIEndpointDescriptorPtr x)
{
    //kprintf("ED(0x%08x):w0=0x%08x,w1(cbp)=0x%08x,w2=0x%08x,w3=0x%08x\n",x,EndianSwap32Bit(x->dWord0),EndianSwap32Bit(x->dWord1),EndianSwap32Bit(x->dWord2),EndianSwap32Bit(x->dWord3));
}
 
void usbAddBus()
{
 extern struct UIMPluginDispatchTable ThePluginDispatchTable;
  USBAddBus(0,&ThePluginDispatchTable,1);
}
UInt16 EndianSwap16Bit(UInt16 x)
{
   UInt16 byte1,byte2;
   byte1 = x & 0xFF;
   byte2 = (x>>8) & 0xFF;
   return (byte1<<8) | byte2;
}
UInt32 EndianSwap32Bit(UInt32 x)
{
   UInt32 byte1,byte2,byte3,byte4;
   UInt32 result;

   byte1 = x & 0xFF;
   byte2 = (x>>8) & 0xFF;
   byte3 = (x>>16) & 0xFF;
   byte4 = (x>>24) & 0xFF;
   result =  (byte1<<24) | (byte2<<16) | (byte3<<8) | byte4;
   return result;
}
void dump_regs(volatile UInt32 *x)
{
int i;
      printf("Regs:\n");
 for(i=0;i<23;++i)
{
      printf("offset %2x: 0x%x\n",4*i,EndianSwap32Bit(*x++));
IOSync();
}
printf("End Regs\n");
}

OSStatus CheckpointIO(IOPreparationID theIOPreparation, IOCheckpointOptions    options)
{
   return 0;
}
OSStatus DelayForHardware (AbsoluteTime                   absoluteTime) 
{
     int x = (int)((AbsoluteTime)1000*absoluteTime);
//     delay((int)((AbsoluteTime)1000*absoluteTime));
kprintf("***Delaying for %d microseconds***\n",x);
     delay(x);
   return 0;
}
AbsoluteTime DurationToAbsolute(Duration duration)   
{
   AbsoluteTime x = (AbsoluteTime)duration;    //naga
    return x;
}

AbsoluteTime UpTime(void)
{
       static long r=67;
       AbsoluteTime x ;
       r=  (r+12061999)%1389;  //Random number  better replace by std random fn
       x = (AbsoluteTime)r;
       return x;
}

ExecutionLevel CurrentExecutionLevel()
{
    return  usb_task_level;
}

LogicalAddress PoolAllocateResident(ByteCount byteSize, Boolean flag)
{
     return (LogicalAddress)kalloc(byteSize);
}
OSStatus PoolDeallocate(LogicalAddress address)
{
//naga    kfree(address,size??);
       return 0;
}

OSStatus QueueSecondaryInterruptHandler(SecondaryInterruptHandler2  theHandler, ExceptionHandler      exceptionHandler, void *p1,     void *p2)
{
   (*theHandler)(p1,p2);
   return 0;
}

OSStatus CallSecondaryInterruptHandler2(SecondaryInterruptHandler2  theHandler, ExceptionHandler exceptionHandler, void *p1, void *p2)
{
     (*theHandler)(p1,p2);
     return 0;
}



void BlockCopy(const void *src, void *tgt, Size len)
{
     bcopy(src,tgt,len);
}
void usb_BlockMoveData(void *src, void *tgt, UInt32 len)
{
     bcopy(src,tgt,len);
}
OSStatus CompareAndSwap(UInt32 old, UInt32 new, UInt32 *adr)
{
     simple_lock(usb_slock);
     if(*adr ==  old)
     {
          *adr = new;
          simple_unlock(usb_slock);
          return true;
     }
     simple_unlock(usb_slock);
     return false;
}

UInt16 Disable68kInterrupts(void)
{
return 0;
}
void Restore68kInterrupts(UInt16 a)
{
}

/* See Page 276 of Writing drivers for PCI cards */
SInt32 AddAtomic(SInt32 amount, SInt32 *value)
{
    simple_lock(usb_slock);
    *value += amount;
    simple_unlock(usb_slock);
    return (*value);
}

/* see pg. 271 */
AbsoluteTime AddAbsoluteToAbsolute(AbsoluteTime a, AbsoluteTime b)
{
    AbsoluteTime c;
      
    simple_lock(usb_slock);
    c = a+b;
    simple_unlock(usb_slock);
      return c;
}
void SysDebugStr(ConstStr255Param str)
{
kprintf("SysDebugStr: Not implemented yet\n");
}

OSStatus CancelTimer (TimerID theTimer, AbsoluteTime *timeRemaining)
{
kprintf("***CancelTimer not yet implemented***\n");
return 0;
}
/*
OSStatus SetInterruptTimer (const AbsoluteTime *expirationTime, SecondaryInterruptHandler2  handler, void *p1, TimerID *theTimer)
{
kprintf("*** SetInterruptTimer not yet implemented***\n");
IOSchduleFunc((IOThreadFunc)handler, p1,1)
return 0;
}
 */
OSStatus PrepareMemoryForIO(IOPreparationTable                      *ioPrep)
{
   return 0;
}

void BlockZero(void *ptr, int size)
{
      bzero(ptr,size);
}
OSStatus RegistryEntryIDDispose (RegEntryID  *foundEntry)
{
      kprintf("***RegistryEntryIDDispose:Not yet Implemented***\n");
return noErr;
}
    
OSStatus RegistryCStrEntryLookup(const RegEntryID *searchPointID, const RegCStrPathName *pathName, RegEntryID *foundEntry) 
{
      kprintf("***RegistryCStrEntryLookupisposePtr:Not yet Implemented***\n");
      return noErr;
}
    
Ptr usb_NewPtr(RegPropertyValueSize propertySize)	
{
     return (Ptr)kalloc(propertySize);
}
void usb_DisposePtr(Ptr modelNamePropertyStrPtr )
{
      kprintf("usb_DisposePtr:Not yet Implemented\n");
}
void c2pstr(Ptr modelNamePropertyStrPtr )
{
kprintf("c2pstr:Not yet Implemented\n");
}
OSStatus CompareString(Str255 inMachineNameStr, Str255 MacMachineName, char *ignore_null)
{
     return strcmp((char *)inMachineNameStr, (char *) MacMachineName);
}
OSStatus RegistryPropertyGetSize(const RegEntryID *entryID, const RegPropertyName *propertyName, RegPropertyValueSize *propertySize)
{
    kprintf("RegistryPropertyGetSize not implemented\n");
    return 0;
}
void USBDemoKeyIn(UInt32 refcon, void *theData)
{
      kprintf("USBDemoKeyIn:Not yet Implemented\n");
}


UInt64 usb_U64Add(UInt64 a, UInt64 b)
{
    return a+b;
}

UInt64 U64SetU(UInt32 a)
{
    return (UInt64) a;
}
UInt32 U32SetU(UInt64 a)
{
    return (UInt32) a;
}

/* Barry - Silly routine to stop unused paramter warnings */

static void parameterNotUsed(UInt32 notUsed)
{
	notUsed = 0;
}
void usb_iodone_wakeup(long lParam, OSStatus status, short s16Param)
{   
        usb_iodone = 1; 
}    
void wait_usbio()
{
#if 0
     while(usb_iodone == 0)
     {
          timeout((timeout_fcn_t)wakeup,&usb_iodone,(100*HZ+999)/1000);
          sleep((char *)&usb_iodone,PZERO);
     }
#endif
}   


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// UIM interface routines.
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

/* This is here so the UIM knows what its address is and can shunt those off to the sim */
Boolean OHCISetOurAddress(uslBusRef bus, UInt16 addr)
{
	pOHCIUIMData->rootHubFuncAddress = addr;
	bus = 0; // not used 
	return(false);
}


////////////////////////////////////////////////////////////////////////////////
//
// OHCIUIMInitialize
//
//   This is the UIM initialization procedure.
//


int OHCIUIMInitialize(unsigned long ioBase)
{
	RegEntryIDPtr				pUIMRegEntryID;
	OHCIRegistersPtr			pOHCIRegisters;
	UInt32						hccaAlignment;
	Ptr							pHCCAAllocation;
	UInt32						hcFmInterval;
	OSStatus					status = noErr;

        pOHCIUIMData = (OHCIUIMDataPtr)kalloc (sizeof (OHCIUIMData));
	if (pOHCIUIMData == NULL)
              return 1;
        BlockZero(pOHCIUIMData,sizeof(OHCIUIMData));
	pOHCIUIMData->errataBits = GetErrataBits(pUIMRegEntryID);
	pOHCIUIMData->pageSize = PAGE_SIZE;

	//Allocate TDs, EDs;need to find out real numbers to use, CPU specific.
	OHCIUIMAllocateMemory(1500,1500, 1300);
	//OHCIUIMAllocateMemory(40,40, 100);
        pOHCIUIMData->pOHCIRegisters = pOHCIRegisters = ioBase; //Should be 3519000 on PowerSurge
dump_regs((volatile UInt32 *)ioBase);
	// is this a goofy CMD chip that is using UHCI-style test mode?  If so, make it behave like an OHCI controller.
kprintf("errata bits=0x%x,mask=0x%x\n",pOHCIUIMData->errataBits,kErrataCMDDisableTestMode);
	if (pOHCIUIMData->errataBits & kErrataCMDDisableTestMode)
	{
		UInt32 registerVal;
kprintf("fixing errata bits for cmd\n");
		// see CMD 670 documentation, appendix A for rev KAB-4 parts
                ReadConfigVal(0x4C,&registerVal);
                    //[deviceDescription configReadLong:0x4C value:&registerVal];
		registerVal &= ~2;
                   // [deviceDescription configWriteLong:0x4C value:registerVal];
                WriteConfigVal(0x4C,registerVal);
	}
        WriteConfigVal(cwCommand,(cwCommandEnableBusMaster | cwCommandEnableMemorySpace));
             //[deviceDescription configWriteLong:cwCommand value:(cwCommandEnableBusMaster | cwCommandEnableMemorySpace)]; 
	pOHCIRegisters->hcControlCurrentED = pOHCIRegisters->hcControlHeadED = pOHCIRegisters->hcDoneHead = 0;
        IOSync();
	// Set up HCCA.
	if (status == noErr)
	{
		// Compute alignment of HCCA. zzz do for real 
		hccaAlignment = 256;
kprintf("settingup HCCA\n");
		// Allocate HCCA.  
		pHCCAAllocation = kalloc (2 * hccaAlignment);
		if (pHCCAAllocation != nil)
		{
			//Needs to be the physical Address
			pOHCIUIMData->pHCCAAllocation = pHCCAAllocation;
			pOHCIUIMData->pHCCA = (Ptr) (((UInt32) pHCCAAllocation + hccaAlignment) & ~(hccaAlignment - 1));
                        pOHCIRegisters->hcHCCA =  EndianSwap32Bit((UInt32)kvtophys((vm_offset_t) pOHCIUIMData->pHCCA));
                        IOSync();
kprintf("pHCCAAllocation=0x%x,aligned logical=0x%x,phy=0x%x\n",pHCCAAllocation,pOHCIUIMData->pHCCA,pOHCIRegisters->hcHCCA);
			/* Set the HC to write the donehead to the HCCA, and enable interrupts */
			pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIHcInterrupt_WDH);
                        IOSync();

		}
		else
		{
kprintf("kalloc failed!!\n");
			status = 1;  // memory full
		}

	}

	// Initialize the Root Hub registers
	if (status == noErr)
	{
		pOHCIRegisters->hcRhStatus = EndianSwap32Bit (/*kOHCIHcRhStatus_LPSC |*/kOHCIHcRhStatus_OCIC | kOHCIHcRhStatus_CRWE);
                OHCIResetRootHub(0);
                IOSync();
	}
		
	// set up Interrupt transfer tree
	if (status == noErr)
	{
		status = OHCIUIMIsochronousInitialize();
		status = OHCIUIMInterruptInitialize();
		status = OHCIUIMBulkInitialize();
		status = OHCIUIMControlInitialize();
		pOHCIUIMData->rootHubFuncAddress = 0;
	} 
		
kprintf("over interrupt transfer tree,status=%d\n",status);
	// Set up hcFmInterval.
	//zzz what would be a good number for FSMPS?
	if (status == noErr)
	{
		UInt32	hcFSMPS;
		hcFmInterval = 11999;
//		hcFSMPS = (((hcFmInterval & kOHCIHcFmInterval_FI)-kOHCIMax_OverHead) * 6)/7;
		hcFSMPS = 10100;	// Barry fix undefineds.
		hcFmInterval = (hcFmInterval & ~kOHCIHcFmInterval_FSMPS) |
					   (hcFSMPS << kOHCIHcFmInterval_FSMPSPhase);
		pOHCIRegisters->hcFmInterval = EndianSwap32Bit (hcFmInterval);
        IOSync();
	}

	// Set up OHCI periodic start to determine time to start processing periodic values, 
	// use hcfminterval and get 90% of that,  can use simple integer math, since frame interval
	// is no more than 14 bits.
	hcFmInterval = EndianSwap32Bit (pOHCIRegisters->hcFmInterval);
        IOSync();
	hcFmInterval &= kOHCIHcFmInterval_FIPhase;
	pOHCIRegisters->hcPeriodicStart = EndianSwap32Bit (10800);
        IOSync();
//	pOHCIRegisters->hcPeriodicStart = EndianSwap32Bit (( hcFmInterval*9)/10);
	
	// Set OHCI to operational state and enable processing of control list.
	if (status == noErr)
	{
kprintf("Setting up usb operational\n");
		pOHCIRegisters->hcControl = EndianSwapImm32Bit ((kOHCIFunctionalState_Operational << kOHCIHcControl_HCFSPhase) |
				 kOHCIHcControl_CLE | kOHCIHcControl_BLE | kOHCIHcControl_PLE | kOHCIHcControl_IE);
                 IOSync();
	}

	if (status == noErr)
	{
kprintf("enabling interrupts\n");
		pOHCIRegisters->hcInterruptEnable = EndianSwap32Bit (kOHCIHcInterrupt_MIE | kOHCIDefaultInterrupts);
        IOSync();
kprintf("After Enabling regs=:\n");
dump_regs((volatile UInt32 *)ioBase);
enable_intr();
	//naga	(*pOHCIUIMData->interruptEnabler)(pOHCIUIMData->interruptSetMember, nil);
	}
	
	if (pOHCIUIMData->errataBits & kErrataLSHSOpti)
		status = OptiLSHSFix();

	// Clean up on error.
	if (status != noErr)
	{
kprintf("OHCIUIMFinalize called!!!!\n");
		if (pOHCIUIMData != nil)
			OHCIUIMFinalizeOHCIUIMData (pOHCIUIMData);
	}
//naga
usb_slock = simple_lock_alloc();
simple_lock_init(usb_slock);
	return (status);
}

////////////////////////////////////////////////////////////////////////////////
//
// OHCIUIMFinalize
//
//   This is the UIM finalization procedure.
//


OSStatus	OHCIUIMFinalize(
	Boolean						beingReplaced,
	Ptr *						savedStatePtr)
{
	OHCIRegistersPtr			pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	OSStatus					status = noErr;


	if (!beingReplaced) {
		ProgressDebugStr ((ConstStr255Param) "OHCIUIMFinalize (shutting down HW)");

		// Disable All OHCI Interrupts
		(*pOHCIUIMData->interruptDisabler)(pOHCIUIMData->interruptSetMember, nil);
		pOHCIRegisters->hcInterruptDisable = kOHCIHcInterrupt_MIE;
/*naga
		// restore interrupt set
		status = InstallInterruptFunctions
					(pOHCIUIMData->interruptSetMember.setID, pOHCIUIMData->interruptSetMember.member,
					 pOHCIUIMData->oldInterruptRefCon, pOHCIUIMData->oldInterruptHandler,
					 pOHCIUIMData->interruptEnabler, pOHCIUIMData->interruptDisabler);
*/
		
		// Place the USB bus into the Reset State
		pOHCIRegisters->hcControl = EndianSwapImm32Bit ((kOHCIFunctionalState_Reset << kOHCIHcControl_HCFSPhase));
		
		// Take away the controllers ability be a bus master.
//naga		status = ExpMgrConfigWriteWord(&pOHCIUIMData->ohciRegEntryID,(LogicalAddress) cwCommand, cwCommandEnableMemorySpace);  // turn off busmastering,
WriteConfigVal(cwCommand,cwCommandEnableMemorySpace);
		// Clear all Processing Registers
		pOHCIRegisters->hcHCCA = nil;
		pOHCIRegisters->hcPeriodCurrentED = nil;
		pOHCIRegisters->hcControlHeadED = nil;
		pOHCIRegisters->hcControlCurrentED = nil;
		pOHCIRegisters->hcBulkHeadED = nil;
		pOHCIRegisters->hcBulkCurrentED = nil;
		pOHCIRegisters->hcDoneHead = nil;
		
		OHCIRootHubPower( 0, 0);	// turn off the global power zzzzz - check for per-port vs. Global power control
		
		status = OHCIUIMFinalizeOHCIUIMData ((OHCIUIMDataPtr) pOHCIUIMData);
	} else {
		ProgressDebugStr ((ConstStr255Param) "OHCIUIMFinalize (being replaced)");
		pOHCIRegisters->hcInterruptDisable = kOHCIHcInterrupt_MIE;
/*naga
		// restore interrupt set
		status = InstallInterruptFunctions
					(pOHCIUIMData->interruptSetMember.setID, pOHCIUIMData->interruptSetMember.member,
					 pOHCIUIMData->oldInterruptRefCon, pOHCIUIMData->oldInterruptHandler,
					 pOHCIUIMData->interruptEnabler, pOHCIUIMData->interruptDisabler);
*/
		*savedStatePtr = (Ptr) pOHCIUIMData;
	}

	ProgressDebugStr ((ConstStr255Param) "OHCIUIMFinalize Exit");


	return (status);
}
static OSStatus OHCIUIMFinalizeOHCIUIMData(
	OHCIUIMDataPtr				pOHCIUIMData)
{
	OSStatus					status = noErr;
	ProgressDebugStr ((ConstStr255Param) "OHCIUIMfinalize");

	if (pOHCIUIMData != nil)
	{
		// Deallocate HCCA.
		if (pOHCIUIMData->pHCCAAllocation != nil){
			PoolDeallocate (pOHCIUIMData->pHCCAAllocation);
			pOHCIUIMData->pHCCAAllocation = nil;
		}
		//nagaif (pOHCIUIMData->pDataAllocation)
		//naga	MemDeallocatePhysicallyContiguous(pOHCIUIMData->pDataAllocation);
		
		if (pOHCIUIMData->pPhysicalLogical)
			PoolDeallocate (pOHCIUIMData->pPhysicalLogical);
		
		// Deallocate UIM data record.
		PoolDeallocate ((Ptr) pOHCIUIMData);
		pOHCIUIMData = nil;
	}

	return (status);
}

// Barry set frame interrupt on. This will be disabled by the service routine, so this is one shot

void OHCISetFrameInterrupt(void)
{
#if 0
OHCIRegistersPtr			pOHCIRegisters;
	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	pOHCIRegisters->hcInterruptEnable = EndianSwap32Bit(kOHCIHcInterrupt_SF);
#endif
}



//	What needs to be done?
// 		Need to figure out an efficient way of keeping track of callbackroutines for when a transaction completes.
//		what to do with control transaction which are really three transactions, must call callback only after 
//		status transaction completes, but must also keep track of what happens to setup and bulk transactions.  
//		hmmmmmmmmmm...its guaranteed that they will finish inorder, but if setup or bulk fails will the HC disable
//		the endpoint until I clean up or will the it continue on til the next one.



OHCIEndpointDescriptorPtr AddEmptyEndPoint(
	UInt8 						functionAddress,
	UInt8						endpointNumber,
	UInt16						maxPacketSize,
	UInt8						speed,
	UInt8						direction,
	OHCIEndpointDescriptorPtr	pED,
	int							isoch)

{
	UInt32								myFunctionAddress, myEndPointNumber,myEndPointDirection,myMaxPacketSize, mySpeed, myIsoch;
	OHCIEndpointDescriptorPtr			pOHCIEndpointDescriptor;
	OHCIGeneralTransferDescriptorPtr	pOHCIGeneralTransferDescriptor;
	OHCIIsochTransferDescriptorPtr		pITD;
	
	pOHCIEndpointDescriptor = (OHCIEndpointDescriptorPtr) OHCIUIMAllocateED();
	myFunctionAddress = ((UInt32) functionAddress) << kOHCIEDControl_FAPhase;
	myEndPointNumber = ((UInt32) endpointNumber) << kOHCIEDControl_ENPhase;
	
	myEndPointDirection = ((UInt32) direction) << kOHCIEDControl_DPhase;
	
	mySpeed = ((UInt32) speed) << kOHCIEDControl_SPhase;

	myMaxPacketSize = ((UInt32) maxPacketSize) << kOHCIEDControl_MPSPhase;
	
	myIsoch = ((UInt32) isoch) << kOHCIEDControl_FPhase;
	pOHCIEndpointDescriptor->dWord0 = EndianSwap32Bit(myFunctionAddress | myEndPointNumber 
										| myEndPointDirection | myMaxPacketSize |mySpeed | myIsoch);
	if (isoch == kOHCIEDFormatGeneralTD) {
		pOHCIGeneralTransferDescriptor = OHCIUIMAllocateTD();

		/* These were previously nil */
		pOHCIEndpointDescriptor->dWord1 = EndianSwap32Bit( (UInt32) pOHCIGeneralTransferDescriptor->pPhysical);
		pOHCIEndpointDescriptor->dWord2 = EndianSwap32Bit( (UInt32) pOHCIGeneralTransferDescriptor->pPhysical);
		pOHCIEndpointDescriptor->pVirtualTailP = (UInt32) pOHCIGeneralTransferDescriptor;
		pOHCIEndpointDescriptor->pVirtualHeadP = (UInt32) pOHCIGeneralTransferDescriptor;

	} else {
		pITD = OHCIUIMAllocateITD();
		
		/* These were previously nil */
		pOHCIEndpointDescriptor->dWord1 = EndianSwap32Bit( (UInt32) pITD->pPhysical);
		pOHCIEndpointDescriptor->dWord2 = EndianSwap32Bit( (UInt32) pITD->pPhysical);
		pOHCIEndpointDescriptor->pVirtualTailP = (UInt32) pITD;
		pOHCIEndpointDescriptor->pVirtualHeadP = (UInt32) pITD;		
	
	}

	pOHCIEndpointDescriptor->dWord3 =  pED->dWord3;
	pOHCIEndpointDescriptor->pVirtualNext = pED->pVirtualNext;
	pED->pVirtualNext = (UInt32) pOHCIEndpointDescriptor;
	pED->dWord3 = EndianSwap32Bit (pOHCIEndpointDescriptor->pPhysical);


	return (pOHCIEndpointDescriptor);

}

OSStatus OHCIUIMControlEDCreate(
	UInt8 						functionAddress,
	UInt8						endpointNumber,
	UInt16						maxPacketSize,
	UInt8						speed)
{
	OHCIEndpointDescriptorPtr	pOHCIEndpointDescriptor, pED;
	OSStatus					status = noErr;
	
#if 0
		/* BT - Let the set address command do this */
	if ((pOHCIUIMData->rootHubFuncAddress == 0) && (functionAddress == 0))
	{
		return(status);
	}
	if (pOHCIUIMData->rootHubFuncAddress == 0){ 
		pOHCIUIMData->rootHubFuncAddress = functionAddress;
		return(status);
	}
#endif

kprintf("OHCIUIMControlEDCreate: fn=%d,ep=%d\n",functionAddress,endpointNumber);
	if (pOHCIUIMData->rootHubFuncAddress == functionAddress) 
		return (status);
		
	pED = (OHCIEndpointDescriptorPtr) pOHCIUIMData->pControlHead;
	if ((speed == kOHCIEDSpeedFull) && pOHCIUIMData->OptiOn)
		pED = (OHCIEndpointDescriptorPtr) pOHCIUIMData->pBulkHead;

	pOHCIEndpointDescriptor = AddEmptyEndPoint (
					functionAddress, endpointNumber, maxPacketSize, speed, kOHCIEDDirectionTD, pED, kOHCIEDFormatGeneralTD);

	if (pOHCIEndpointDescriptor == nil)
		return(-1);
	return (status);
}



OSStatus OHCIUIMControlTransfer(
	UInt32						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short 						functionNumber,
	short						endpointNumber,
	short						bufferSize,
	short						direction)
{
	OHCIRegistersPtr			pOHCIRegisters;
	OHCIGeneralTransferDescriptorPtr
								pOHCIGeneralTransferDescriptor,
								newOHCIGeneralTransferDescriptor;
	OSStatus					status = noErr;
	UInt32						myBufferRounding;
	UInt32						myDirection;
	UInt32						myToggle;
	UInt32						CBPPhysical;

	OHCIEndpointDescriptorPtr	pEDQueue, pEDDummy;


	
	if (pOHCIUIMData->rootHubFuncAddress == functionNumber) 
	{
//kprintf("OHCIUIMControlTransfer:Calling UIMSimulateRootHubStages\n");
		return(UIMSimulateRootHubStages(refcon, handler, CBP, bufferRounding, 
						endpointNumber, bufferSize, direction));
	}
//kprintf("Cntl Tx:roothubfuncaddr=0x%x,fnumb=0x%x;calling FindControlEndpoint\n",pOHCIUIMData->rootHubFuncAddress , functionNumber);
	if (direction == kUSBOut)
		direction = kOHCIGTDPIDOut;
	else if (direction == kUSBIn) 
		direction = kOHCIGTDPIDIn;
	else
		direction = kOHCIGTDPIDSetup;


	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	//search for endpoint descriptor

	pEDQueue = FindControlEndpoint(functionNumber, endpointNumber, &pEDDummy);
	if (pEDQueue == nil){
		kprintf("OHCIUIMControlTransfer: Couldnot find endpoint\n");
		status = kUSBNotFound;
		return(status);
	}
	myBufferRounding = (UInt32) bufferRounding << kOHCIBufferRoundingOffset;
	myDirection = (UInt32) direction << kOHCIDirectionOffset;
	myToggle = kOHCIBit25;	/* Take data toggle from TD */
	if(direction !=0)
	{	/* Setup uses Data 0, data status use Data1 */
		myToggle |= kOHCIBit24;	/* use Data1 */
	}

	// setup transfer descriptor
	newOHCIGeneralTransferDescriptor = OHCIUIMAllocateTD();
	if (newOHCIGeneralTransferDescriptor == nil)
        {
                kprintf("Kalloc TD failed\n");
		return (-1);
        }

	/* Last in queue is a dummy descriptor. Fill it in then add new dummy */
	pOHCIGeneralTransferDescriptor =  (OHCIGeneralTransferDescriptorPtr) pEDQueue->pVirtualTailP;
pOHCIGeneralTransferDescriptor->pEndpoint = (UInt32) pEDQueue;    //naga added for our reference
	pOHCIGeneralTransferDescriptor->dWord0 = EndianSwap32Bit (myBufferRounding | myDirection | myToggle);
//kprintf("dword0=0x%x,round=0x%x,dir=0x%x,toggle=0x%x\n",EndianSwap32Bit (pOHCIGeneralTransferDescriptor->dWord0),myBufferRounding,myDirection,myToggle);
	pOHCIGeneralTransferDescriptor->dWord2 = EndianSwap32Bit ((UInt32)newOHCIGeneralTransferDescriptor->pPhysical);
	pOHCIGeneralTransferDescriptor->pVirtualNext = (UInt32)newOHCIGeneralTransferDescriptor;

	if (bufferSize != 0)
	{
                CBPPhysical = kvtophys((vm_offset_t) CBP);
		pOHCIGeneralTransferDescriptor->dWord1 = EndianSwap32Bit (CBPPhysical);
		pOHCIGeneralTransferDescriptor->dWord3 = EndianSwap32Bit (CBPPhysical + bufferSize-1);
	}
	else
	{
		pOHCIGeneralTransferDescriptor->dWord1 = 0;
		pOHCIGeneralTransferDescriptor->dWord3 = 0;
	}
	pOHCIGeneralTransferDescriptor->CallBack = handler;
	pOHCIGeneralTransferDescriptor->refcon = refcon;

	/* Make new descriptor the tail */
	pEDQueue->dWord1 = pOHCIGeneralTransferDescriptor->dWord2;
	pEDQueue->pVirtualTailP = (UInt32) newOHCIGeneralTransferDescriptor;

print_ed(pEDQueue);
print_td(pOHCIGeneralTransferDescriptor);
	pOHCIRegisters->hcCommandStatus = EndianSwap32Bit (kOHCIHcCommandStatus_CLF);
        IOSync();
	return (status);
}
OSStatus OHCIUIMControlEDDelete(
	short 						functionNumber,
	short						endpointNumber)
{
	OSStatus					status = noErr;
	
	if (pOHCIUIMData->rootHubFuncAddress == functionNumber) 
		return (status);
kprintf("OHCIUIMControlEDDelete: calling OHCIUIMEndpointDelete if fn=ep fn=%d,ep=%d\n",functionNumber,endpointNumber);
	status = OHCIUIMEndpointDelete(functionNumber, endpointNumber, 3);
	
	return (status);
}

OSStatus OHCIUIMBulkEDCreate(
	UInt8 						functionAddress,
	UInt8						endpointNumber,
	UInt8						direction,
	UInt8						maxPacketSize)
{
	OHCIEndpointDescriptorPtr	pOHCIEndpointDescriptor, pED;
	OSStatus					status = noErr;
	if (direction == kUSBOut)
		direction = kOHCIEDDirectionOut;
	else if (direction == kUSBIn) 
		direction = kOHCIEDDirectionIn;
	else
		direction = kOHCIEDDirectionTD;

	pED = (OHCIEndpointDescriptorPtr) pOHCIUIMData->pBulkHead;
	pOHCIEndpointDescriptor = AddEmptyEndPoint (
					functionAddress, endpointNumber, maxPacketSize, kOHCIEDSpeedFull, direction, pED, kOHCIEDFormatGeneralTD);
	if (pOHCIEndpointDescriptor == nil)
		return(-1);
	return (status);
	
}

/* Naga  This function needs to be edited to replace prepareMemoryforio with kvtophys -- see OHCIUIMInterruptTransfer for cut/paste */
OSStatus OHCIUIMBulkTransfer(
	UInt32						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short 						functionNumber,
	short						endpointNumber,
	UInt32						bufferSize,
	short						direction)
{
	OHCIRegistersPtr			pOHCIRegisters;
	OHCIGeneralTransferDescriptorPtr
								pOHCIGeneralTransferDescriptor,
								newOHCIGeneralTransferDescriptor;
	OSStatus					status = noErr;
	UInt32						myBufferRounding;
	UInt32						endPtDirection;
	UInt32						TDDirection;
	OHCIEndpointDescriptorPtr	pEDQueue, pEDDummy;
	IOPreparationTable			*ioPrep;
	IOPreparationTable			IOPrep;
	UInt32					  	physicalAddresses[kOHCIMaxPages];	
	UInt32						numPages, firstCount, lastCount;
	IOPreparationID				preparationID;
	UInt32						pageSize, pageOffsetMask;
	int							i;
	UInt32						alignment, pageOffset, pageMask, pageCount;

//	DebugStr("OHCIUIM: InSide Bulk TD setup");

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	pageSize = pOHCIUIMData->pageSize;
	ioPrep = &IOPrep;
	
	if (direction == kUSBOut)
		direction = kOHCIEDDirectionOut;
	else if (direction == kUSBIn) 
		direction = kOHCIEDDirectionIn;
	else
		direction = kOHCIEDDirectionTD;

	//search for endpoint descriptor
	pEDQueue = FindBulkEndpoint (functionNumber, endpointNumber, direction, &pEDDummy);

	if (!pEDQueue){
		FailureDebugStr("UIMBulkTransfer: Couldnot find endpoint");
		status = kUSBNotFound;
		return (status);
	}


	pageOffsetMask = pageSize - 1;
	pageMask = ~pageOffsetMask;
	
	myBufferRounding = (UInt32) bufferRounding << kOHCIBufferRoundingOffset;
	endPtDirection = (UInt32) direction << kOHCIEndpointDirectionOffset;
	TDDirection = (UInt32) direction << kOHCIDirectionOffset;

// FERG DEBUG
// uncomment the next line to force the data to be put in TD list, but not be processed
// this is handy for using USBProber/Macsbug to look at TD's to see if they're OK.
//			pEDQueue->dWord0 |= EndianSwap32Bit (kOHCIEDControl_K);
// FERG DEBUG
	alignment = CBP % ((EndianSwap32Bit (pEDQueue->dWord0)
						& kOHCIEDControl_MPS) >> kOHCIEDControl_MPSPhase);
	if(alignment != 0)  
	{
//	DebugStr("OHCIUIM: determining pageOffset");

		pageOffset = CBP & pageOffsetMask;
	}	
	if (bufferSize != 0)
	{
		while ((status == noErr) && (bufferSize > 0)) {
		
			firstCount = pageSize - (CBP & pageOffsetMask);
			if (bufferSize <= firstCount){
				lastCount = 0;
				numPages = 1;
				firstCount = bufferSize;
			} else {
				lastCount = (CBP + bufferSize) & pageOffsetMask;
				numPages = (bufferSize - firstCount - lastCount)/pageSize;
				if (firstCount) numPages++;
				if (lastCount) numPages++;
			}
			
			// Prepare for IO and get physical address
			ioPrep->options = kIOLogicalRanges | kIOIsInput | kIOIsOutput;
			ioPrep->addressSpace = kCurrentAddressSpaceID;			// default
			ioPrep->granularity = 0;								// do it all now
			ioPrep->firstPrepared = 0;
			ioPrep->mappingEntryCount = MIN (numPages, kOHCIMaxPages) ;				// # of pages we will use
			ioPrep->logicalMapping = 0;
			ioPrep->physicalMapping = (PhysicalMappingTablePtr) physicalAddresses;	// return list of phys addrs
			ioPrep->rangeInfo.range.base = (void *) CBP;
			ioPrep->rangeInfo.range.length =  bufferSize;
					
			status = PrepareMemoryForIO (ioPrep);
			if (status == noErr){
				preparationID = ioPrep->preparationID;
				pageCount = ioPrep->mappingEntryCount;
				if ((alignment != 0) && (ioPrep->mappingEntryCount == kOHCIMaxPages))
					pageCount--;
				for (i = 0; i< pageCount; i++) {
					newOHCIGeneralTransferDescriptor = OHCIUIMAllocateTD();
					if (newOHCIGeneralTransferDescriptor == nil) {
						return (-1);
					}
					pOHCIGeneralTransferDescriptor = (OHCIGeneralTransferDescriptorPtr) pEDQueue->pVirtualTailP;
					pOHCIGeneralTransferDescriptor->dWord0 = EndianSwap32Bit (myBufferRounding | TDDirection);
					pOHCIGeneralTransferDescriptor->dWord2 = EndianSwap32Bit ((UInt32)newOHCIGeneralTransferDescriptor->pPhysical);
					pOHCIGeneralTransferDescriptor->pVirtualNext = (UInt32)newOHCIGeneralTransferDescriptor;
					pOHCIGeneralTransferDescriptor->dWord1 = EndianSwap32Bit (physicalAddresses[i]);
					pOHCIGeneralTransferDescriptor->pEndpoint = (UInt32) pEDQueue;
					// zzzzzz - really should check the direction to specify IN or OUT type.
					pOHCIGeneralTransferDescriptor->pType = (UInt32) kOHCIBulkTransferOutType;
					firstCount = pageSize - (CBP & pageOffsetMask);
					// if an IN and not aligned
					if(alignment != 0) {
	//		DebugStr("OHCIUIM: unaligned in buffer");
						pOHCIGeneralTransferDescriptor->dWord1 = EndianSwap32Bit (
													(physicalAddresses[i] & pageMask) + pageOffset);
						if ( bufferSize > pageSize) 
							pOHCIGeneralTransferDescriptor->dWord3 = EndianSwap32Bit (
													physicalAddresses[i+1] + pageOffset -1);
						else if (bufferSize >= (pageSize - pageOffset))
							pOHCIGeneralTransferDescriptor->dWord3 = EndianSwap32Bit (
													physicalAddresses[++i] + bufferSize - (pageSize - pageOffset) -1);
						else
							pOHCIGeneralTransferDescriptor->dWord3 = EndianSwap32Bit ((physicalAddresses[i]& pageMask)
												+ bufferSize + pageOffset - 1);
									
						lastCount = MIN(pageSize, bufferSize);
						firstCount = 0;
					}else if ((firstCount >= bufferSize) 
							|| (i+1 == ioPrep->mappingEntryCount)		// this is really the last (non-full) block to be transferred
							|| pOHCIUIMData->errataBits & kErrataOnlySinglePageTransfers){  // only do a max of one page per transfer descriptor
						lastCount = 0;
						firstCount = MIN(firstCount, bufferSize);
						pOHCIGeneralTransferDescriptor->dWord3 = EndianSwap32Bit (physicalAddresses[i] + firstCount - 1);
					} else {
						i++;
						lastCount = MIN (bufferSize - firstCount, pageSize);
						pOHCIGeneralTransferDescriptor->dWord3 = EndianSwap32Bit (physicalAddresses[i] + lastCount - 1);
					}
					bufferSize -= (firstCount + lastCount);
					// only supply a callback when the entire buffer has been transfered.
					pOHCIGeneralTransferDescriptor->CallBack = (bufferSize <= 0) ? handler : nil;
					pOHCIGeneralTransferDescriptor->refcon = refcon;
					// supply a preparationID at the end of each prepared range
					// pOHCIGeneralTransferDescriptor->preparationID = (i+1 == ioPrep->mappingEntryCount) ? preparationID : nil;
					CBP += (firstCount + lastCount);
					pEDQueue->dWord1 = pOHCIGeneralTransferDescriptor->dWord2;
					pEDQueue->pVirtualTailP = (UInt32) newOHCIGeneralTransferDescriptor;
					pOHCIRegisters->hcCommandStatus = EndianSwap32Bit (kOHCIHcCommandStatus_BLF);
				}
				CheckpointIO(ioPrep->preparationID, nil);  // zzzz - Ferg - keep from overflowing the locked page count

			} else {
				AssertDebugStr("OHCIUIM: PrepareMemoryForIO failed!");
			}
		}

	}
	else
	{
		newOHCIGeneralTransferDescriptor = OHCIUIMAllocateTD();
		/* last in queue is dummy descriptor. Fill it in then add new dummy */
		pOHCIGeneralTransferDescriptor =  (OHCIGeneralTransferDescriptorPtr) pEDQueue->pVirtualTailP;

		pOHCIGeneralTransferDescriptor->dWord0 = EndianSwap32Bit (myBufferRounding | TDDirection);
		pOHCIGeneralTransferDescriptor->dWord2 = EndianSwap32Bit ((UInt32)newOHCIGeneralTransferDescriptor->pPhysical);
		pOHCIGeneralTransferDescriptor->pVirtualNext = (UInt32)newOHCIGeneralTransferDescriptor;

		/*for zero sized buffers */
		pOHCIGeneralTransferDescriptor->dWord1 = 0;
		pOHCIGeneralTransferDescriptor->dWord3 = 0;
		pOHCIGeneralTransferDescriptor->CallBack = handler;
		pOHCIGeneralTransferDescriptor->refcon = refcon;

		/* Make new descriptor the tail */
		pEDQueue->dWord1 = pOHCIGeneralTransferDescriptor->dWord2;
		pEDQueue->pVirtualTailP = (UInt32) newOHCIGeneralTransferDescriptor;
	}
	pOHCIRegisters->hcCommandStatus = EndianSwap32Bit (kOHCIHcCommandStatus_BLF);
	if ( pOHCIUIMData->OptiOn)
		pOHCIRegisters->hcCommandStatus = EndianSwap32Bit (kOHCIHcCommandStatus_CLF);		

	return (status);
}

OSStatus OHCIUIMBulkEDDelete(
	short 						functionNumber,
	short						endpointNumber,
	short						direction)
{
	OSStatus					status = noErr;
	
kprintf("OHCIUIMBulkDelete: calling OHCIUIMEndpointDelete for fn=%d,ep=%d\n",functionNumber,endpointNumber);
	status = OHCIUIMEndpointDelete(functionNumber, endpointNumber, direction);
	
	return (status);
}

Boolean BandwidthAvailable(
	UInt32			pollingRate, 
	UInt32			reserveBandwidth,
	int				*offset)
{
	int num;
	OHCIRegistersPtr			pOHCIRegisters;

#pragma unused reserveBandwidth
	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;

	num = EndianSwap32Bit(pOHCIRegisters->hcFmNumber)&kOHCIFmNumberMask;
	if (pollingRate <  1)
		//error condition
		return(false);
	else if(pollingRate < 2)
		*offset = 62;
	else if(pollingRate < 4)
		*offset = (num%2) + 60;
	else if(pollingRate < 8)
		*offset = (num%4) + 56;
	else if(pollingRate < 16)
		*offset = (num%8) + 48;
	else if(pollingRate < 32)
		*offset = (num%16) + 32;
	else
		*offset = (num%32) + 0;
	return (true);
}


OSStatus OHCIUIMInterruptEDCreate(
	short						functionAddress,
	short						endpointNumber,
	short						speed,
	UInt16 						maxPacketSize,
	short						pollingRate,
	UInt32						reserveBandwidth)
{
	OHCIRegistersPtr			pOHCIRegisters;
	OHCIEndpointDescriptorPtr	pOHCIEndpointDescriptor;
	OHCIEndpointDescriptorPtr	pED;
	UInt32						myFunctionAddress;
	UInt32						myEndpointNumber;
	UInt32						myMaxPacketSize, myEndPointDirection;
	UInt32				 		mySpeed;
	OHCIGeneralTransferDescriptorPtr
								pOHCIGeneralTransferDescriptor;
	OSStatus					status = noErr;
	OHCIIntHeadPtr				pInterruptHead;
	int							offset;

	if (pOHCIUIMData->rootHubFuncAddress == functionAddress) 
		return (status);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	pInterruptHead = pOHCIUIMData->pInterruptHead;


///ZZZZz  opti bug fix!!!!
	if (pOHCIUIMData->OptiOn) 
		if (speed == kOHCIEDSpeedFull) 
			if (pollingRate >= 8)
				pollingRate = 7;


	// Do we have room?? if so return with offset equal to location
	if((Boolean) BandwidthAvailable(pollingRate, reserveBandwidth, &offset) == false) 
		return(bandWidthFullErr);

	//  Allocate Endpoint Descripter
	pOHCIEndpointDescriptor = (OHCIEndpointDescriptorPtr) OHCIUIMAllocateED();

	// Fill in necessary data for descriptor
	myFunctionAddress = ((UInt32) functionAddress) << kOHCIEDControl_FAPhase;
	myEndpointNumber = ((UInt32) endpointNumber) << kOHCIEndpointNumberOffset;
	myMaxPacketSize = (UInt32) maxPacketSize << kOHCIMaxPacketSizeOffset;
	mySpeed = (UInt32) speed << kOHCISpeedOffset;
	myEndPointDirection = ((UInt32) 2) << kOHCIEndpointDirectionOffset;


	pOHCIEndpointDescriptor->dWord0 = EndianSwap32Bit(myFunctionAddress | myEndpointNumber | myMaxPacketSize 
														| mySpeed | myEndPointDirection);

	pOHCIGeneralTransferDescriptor = OHCIUIMAllocateTD();
	if (pOHCIGeneralTransferDescriptor == nil) {
		return (-1);
	}


	/* These were previously nil */
	pOHCIEndpointDescriptor->dWord1 = EndianSwap32Bit( (UInt32) pOHCIGeneralTransferDescriptor->pPhysical);
	pOHCIEndpointDescriptor->dWord2 = EndianSwap32Bit( (UInt32) pOHCIGeneralTransferDescriptor->pPhysical);
	pOHCIEndpointDescriptor->pVirtualTailP = (UInt32) pOHCIGeneralTransferDescriptor;
	pOHCIEndpointDescriptor->pVirtualHeadP = (UInt32) pOHCIGeneralTransferDescriptor;
	
	// Add to Queue
	pED = pInterruptHead[offset].pHead;
	pOHCIEndpointDescriptor->dWord3 = pED->dWord3;
	pED->dWord3 = EndianSwap32Bit ((UInt32) pOHCIEndpointDescriptor->pPhysical);
	pOHCIEndpointDescriptor->pVirtualNext = pED->pVirtualNext;
	pED->pVirtualNext = (UInt32) pOHCIEndpointDescriptor;

/// will never happen???
	if (pInterruptHead[offset].pHead == pInterruptHead[offset].pTail)
		pInterruptHead[offset].pTail = pOHCIEndpointDescriptor;
	
	pInterruptHead[offset].nodeBandwidth += reserveBandwidth;
	
	return (status);
}


OSStatus OHCIUIMInterruptTransfer(
	short						functionNumber,
	short						endpointNumber,
	UInt32 						refcon,
	CallBackFuncPtr 			handler,
	UInt32						CBP,
	Boolean						bufferRounding,
	short						bufferSize)
{
	OHCIRegistersPtr			pOHCIRegisters;
	OHCIGeneralTransferDescriptorPtr
								pOHCIGeneralTransferDescriptor,
								newOHCIGeneralTransferDescriptor;
	OSStatus					status = noErr;
	UInt32						myBufferRounding;
	UInt32						myDirection;
	UInt32						myToggle;
	OHCIEndpointDescriptorPtr	pEDQueue, temp;
	OHCIIntHeadPtr				pInterruptHead;
	IOPreparationTable			*ioPrep;
	IOPreparationTable			IOPrep;
	UInt32						CBPPhysical;
	Boolean done = false;

kprintf("OHCIUIMInterruptTransfer:functionNumber=%d,roothubfn=%d\n",functionNumber,pOHCIUIMData->rootHubFuncAddress);
	if (pOHCIUIMData->rootHubFuncAddress == functionNumber) 
	{
		SimulateRootHubInt(endpointNumber, (void *)CBP, bufferSize, handler, refcon);
		return(noErr);
	}

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	pInterruptHead = pOHCIUIMData->pInterruptHead;
	
	pEDQueue = FindInterruptEndpoint(functionNumber, endpointNumber, &temp);
	if ( pEDQueue != nil) 
	{
		newOHCIGeneralTransferDescriptor = OHCIUIMAllocateTD();
		if (newOHCIGeneralTransferDescriptor == nil) {
			return (-1);
		}
		
		myBufferRounding = (UInt32) bufferRounding << kOHCIBufferRoundingOffset;
		myToggle = 0;	/* Take data toggle from Endpoint Descriptor */


		/* Last in queue is a dummy descriptor. Fill it in then add new dummy */
		pOHCIGeneralTransferDescriptor =  (OHCIGeneralTransferDescriptorPtr) pEDQueue->pVirtualTailP;
	
		myDirection = kOHCIBit20;	// Barry - This was undefined, make it IN. Should be in endpoint anyway
	
		pOHCIGeneralTransferDescriptor->dWord0 = EndianSwap32Bit (myBufferRounding | myDirection | myToggle);
		pOHCIGeneralTransferDescriptor->dWord2 = EndianSwap32Bit ((UInt32)newOHCIGeneralTransferDescriptor->pPhysical);
		pOHCIGeneralTransferDescriptor->pVirtualNext = (UInt32)newOHCIGeneralTransferDescriptor;
	
		if (bufferSize != 0)
		{
                        CBPPhysical = kvtophys((vm_offset_t) CBP);
			pOHCIGeneralTransferDescriptor->dWord1 = EndianSwap32Bit (CBPPhysical);
			pOHCIGeneralTransferDescriptor->dWord3 = EndianSwap32Bit (CBPPhysical + bufferSize-1);

		} else {
			pOHCIGeneralTransferDescriptor->dWord1 = 0;
			pOHCIGeneralTransferDescriptor->dWord3 = 0;
		}
		pOHCIGeneralTransferDescriptor->CallBack = handler;
		pOHCIGeneralTransferDescriptor->refcon = refcon;
		
		/* Make new descriptor the tail */
		pEDQueue->dWord1 = pOHCIGeneralTransferDescriptor->dWord2;
		pEDQueue->pVirtualTailP = (UInt32) newOHCIGeneralTransferDescriptor;
	} else {
		FailureDebugStr("UIMInterruptTransfer: Couldnot find endpoint");
		status = kUSBNotFound;
	}
        kprintf("OHCIUIMInterruptTransfer: returning status=%d\n",status);
	return ( status);
}


OSStatus OHCIUIMEndpointDelete(
	short 						functionNumber,
	short						endpointNumber,
	short						direction)
{
	OHCIRegistersPtr			pOHCIRegisters;
	OSStatus					status = noErr;
	OHCIEndpointDescriptorPtr	pED;
	OHCIEndpointDescriptorPtr	pEDQueueBack;
	UInt32						hcControl;
	UInt32						something, controlMask;
//	UInt32						edDirection;


	if (pOHCIUIMData->rootHubFuncAddress == functionNumber) 
		return (status);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	
	//search for endpoint descriptor
	if (direction == kUSBOut)
		direction = kOHCIEDDirectionOut;
	else if (direction == kUSBIn) 
		direction = kOHCIEDDirectionIn;
	else
		direction = kOHCIEDDirectionTD;
		
kprintf("OHCIUIMEndpointDelete: calling FindEndpoint\n");
	if((pED = FindEndpoint (functionNumber, endpointNumber, direction, &pEDQueueBack, &controlMask)) == nil)
	{
		FailureDebugStr("UIMEndpointDelete: Couldnot find endpoint\n");
		return (kUSBNotFound);
	}

	// Remove Endpoint
	//mark sKipped
	pED->dWord0 |= EndianSwap32Bit (kOHCIEDControl_K);
//	edDirection = EndianSwap32Bit(pED->dWord0) & kOHCIEndpointDirectionMask;
	// remove pointer wraps
	pEDQueueBack->dWord3 = pED->dWord3;
	pEDQueueBack->pVirtualNext = pED->pVirtualNext;


	// clear some bit in hcControl
	hcControl = EndianSwap32Bit (pOHCIRegisters->hcControl);	
	hcControl &= ~controlMask;
	hcControl &= OHCIBitRange(0, 10);

	pOHCIRegisters->hcControl = EndianSwap32Bit (hcControl);

	// poll for interrupt  zzzzz turn into real interrupt
	pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIBit2);
	DelayForHardware(DurationToAbsolute(1*durationMillisecond));
	something = EndianSwap32Bit(pOHCIRegisters->hcInterruptStatus) & kOHCIInterruptSOFMask;
#if 1
			if(!something)
			{	/* This should have been set, just in case wait another ms */
				DelayForHardware(DurationToAbsolute(1*durationMillisecond));
			}
#else
	/* BT - 13Jun98, this was hanging, the grey screen with andromeda hub */
			while (!something) {
				DelayForHardware(DurationToAbsolute(1*durationMillisecond));
				something = EndianSwap32Bit(pOHCIRegisters->hcInterruptStatus) & kOHCIHcInterrupt_SF;
			}
#endif
	//restart hcControl
	hcControl |= controlMask;
	pOHCIRegisters->hcControl = EndianSwap32Bit (hcControl);

	RemoveAllTDs(pED);
	
	pED->dWord3 = nil;
	
	//deallocate ED
	OHCIUIMDeallocateED(pED);
	return (status);
}


OSStatus OHCIUIMAbortEndpoint(
	short 						functionNumber,
	short						endpointNumber,
	short						direction)
{
	OHCIRegistersPtr			pOHCIRegisters;
	OSStatus					status = noErr;
	OHCIEndpointDescriptorPtr	pED;
	OHCIEndpointDescriptorPtr	pEDQueueBack;
	UInt32						hcControl;
	UInt32						something, controlMask;

	if (pOHCIUIMData->rootHubFuncAddress == functionNumber) 
		return (status);

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	
	if (direction == kUSBOut)
		direction = kOHCIEDDirectionOut;
	else if (direction == kUSBIn) 
		direction = kOHCIEDDirectionIn;
	else
		direction = kOHCIEDDirectionTD;

	//search for endpoint descriptor
kprintf("OHCIUIMAbort EP:calling FindEndpoint\n");
	if((pED = FindEndpoint (functionNumber, endpointNumber, direction, &pEDQueueBack, &controlMask)) == nil)
		return(kUSBNotFound);


	pED->dWord0 |= EndianSwap32Bit (kOHCIEDControl_K);
	hcControl = EndianSwap32Bit (pOHCIRegisters->hcControl);	
	hcControl &= ~controlMask;
	hcControl &= OHCIBitRange(0, 10);

	pOHCIRegisters->hcControl = EndianSwap32Bit (hcControl);

	// poll for interrupt  zzzzz turn into real interrupt
	pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIBit2);
	DelayForHardware(DurationToAbsolute(1*durationMillisecond));
	something = EndianSwap32Bit(pOHCIRegisters->hcInterruptStatus) & kOHCIInterruptSOFMask;
#if 1
			if(!something)
			{	/* This should have been set, just in case wait another ms */
				DelayForHardware(DurationToAbsolute(1*durationMillisecond));
			}
#else
	/* BT - 13Jun98, this was hanging, the grey screen with andromeda hub */
			while (!something) {
				DelayForHardware(DurationToAbsolute(1*durationMillisecond));
				something = EndianSwap32Bit(pOHCIRegisters->hcInterruptStatus) & kOHCIHcInterrupt_SF;
			}
#endif
	//restart hcBulk
	hcControl |= controlMask;
	pOHCIRegisters->hcControl = EndianSwap32Bit (hcControl);

	RemoveTDs(pED);

	pED->dWord0 &= ~EndianSwap32Bit (kOHCIEDControl_K);

	return (0);

}

UInt64 OHCIUIMGetCurrentFrameNumber(void) 
{

	UInt64						time;
	OHCIRegistersPtr			pOHCIRegisters;
	
	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	time = usb_U64Add (pOHCIUIMData->frameNumber, U64SetU(EndianSwap32Bit(pOHCIRegisters->hcFmNumber)&kOHCIFmNumberMask));
	return (time);

}



////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Internal routines.
//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



////////////////////////////////////////////////////////////////////////////////
//
// OHCIUIMGetRegisterBaseAddress
//
//   This proc uses the name registry to find the base address of the registers.
//

#if 0  //naga
static OSStatus	OHCIUIMGetRegisterBaseAddress(
	RegEntryIDPtr				pOHCIRegEntryID,
	OHCIRegistersPtr			*ppOHCIRegisters)
{
	PCIAssignedAddressPtr		pPCIAssignedAddresses = nil;
	DeviceLogicalAddressPtr		pDeviceLogicalAddresses = nil;
	RegPropertyValueSize		propSize;
	UInt32						addressSpaceFlags;
	UInt32						registerNumber;
	OHCIRegistersPtr			pOHCIRegisters;
	UInt32						numAddresses,
								addressNum;
	Boolean						done;
	OSStatus					status = noErr;

	// Get assigned addresses property.
	status = RegistryPropertyGetSize
				(pOHCIRegEntryID,
				 (RegPropertyNamePtr) kPCIAssignedAddressProperty,
				 &propSize);

	if (status == noErr)
	{
		if (CurrentExecutionLevel() != kTaskLevel){
			MemoryDebugStr("OHCIUIM: Allocating memory (3) at non-task time, report stack trace please; sc");
		}
		// zzzz do we deallocate this?
		pPCIAssignedAddresses =
			(PCIAssignedAddressPtr) PoolAllocateResident (propSize, false);
		if (pPCIAssignedAddresses == nil)
			status = memFullErr;
	}

	if (status == noErr)
	{
		status = RegistryPropertyGet
					(pOHCIRegEntryID,
					 (RegPropertyNamePtr) kPCIAssignedAddressProperty,
					 pPCIAssignedAddresses,
					 &propSize);
		if (status == noErr)
			numAddresses = propSize / sizeof (PCIAssignedAddress);
	}

	// Get logical addresses property.
	if (status == noErr)
	{
		status = RegistryPropertyGetSize
					(pOHCIRegEntryID,
					 (RegPropertyNamePtr) kAAPLDeviceLogicalAddress,
					 &propSize);
	}

	if (status == noErr)
	{
		// zzzzz do we dealocate this?
		pDeviceLogicalAddresses =
			(DeviceLogicalAddressPtr) PoolAllocateResident (propSize, false);
		if (pDeviceLogicalAddresses == nil)
			status = memFullErr;
	}

	if (status == noErr)
	{
		status = RegistryPropertyGet
					(pOHCIRegEntryID,
					 (RegPropertyNamePtr) kAAPLDeviceLogicalAddress,
					 pDeviceLogicalAddresses,
					 &propSize);
	}

	// Get main memory mapped register file base address.
	if (status == noErr)
	{
		done = false;
		for (addressNum = 0; ((addressNum < numAddresses) && (!done)); addressNum++)
		{
			addressSpaceFlags = pPCIAssignedAddresses[addressNum].addressSpaceFlags;
			registerNumber = pPCIAssignedAddresses[addressNum].registerNumber;
			if ((registerNumber == kOHCIConfigRegBaseAddressRegisterNumber) &&
				(addressSpaceFlags & kPCI32BitMemorySpace))
			{
				pOHCIRegisters = pDeviceLogicalAddresses[addressNum];
				done = true;
			} 
		}

		if (!done)
//zzzz
			status = -4161;
//			status = notFoundErr;
	}

	// Clean up.
	if (pDeviceLogicalAddresses != nil)
		PoolDeallocate ((Ptr) pDeviceLogicalAddresses);
	if (pPCIAssignedAddresses != nil)
		PoolDeallocate ((Ptr) pPCIAssignedAddresses);

	// Return results.
	if (status == noErr)
		*ppOHCIRegisters = pOHCIRegisters;
	else
		*ppOHCIRegisters = nil;

	return (status);
}


OSStatus OHCIUIMInstallInterruptHandler ()

{
	RegEntryIDPtr				pOHCIRegEntryID = &(pOHCIUIMData->ohciRegEntryID);
	ISTProperty					interruptSets;
	InterruptSetID				interruptSetID;
	InterruptMemberNumber		interruptMemberNumber;
	void						*oldInterruptRefCon;
	InterruptHandler			oldInterruptHandler;
	InterruptEnabler			oldInterruptEnabler;
	InterruptDisabler			oldInterruptDisabler;
	RegPropertyValueSize		propSize;
	OSStatus					status = noErr;


	// Get our interrupt set from the name registry.
	propSize = sizeof (ISTProperty);
	status = RegistryPropertyGet (pOHCIRegEntryID, kISTPropertyName,
								  &interruptSets, &propSize);
	interruptSetID = interruptSets[kISTChipInterruptSource].setID;
	interruptMemberNumber = interruptSets[kISTChipInterruptSource].member;

	// Get the interrupt enabler and disabler for our chip interrupt set.
	if (status == noErr)
	{
		status = GetInterruptFunctions
					(interruptSetID, interruptMemberNumber,
					 &oldInterruptRefCon, &oldInterruptHandler,
					 &oldInterruptEnabler, &oldInterruptDisabler);
	}

	// Install the refCon and interrupt handler for our chip interrupt set.
	if (status == noErr)
	{
		status = InstallInterruptFunctions
					(interruptSetID, interruptMemberNumber,
					 pOHCIUIMData, (InterruptHandler) OHCIUIMInterruptHandler,
					 nil, nil);
	}

	// Add info to our UIM Data record.
	if (status == noErr)
	{
		pOHCIUIMData->interruptSetMember.setID = interruptSetID;
		pOHCIUIMData->interruptSetMember.member = interruptMemberNumber;
		pOHCIUIMData->oldInterruptRefCon = oldInterruptRefCon;
		pOHCIUIMData->oldInterruptHandler = oldInterruptHandler;
		pOHCIUIMData->interruptEnabler = oldInterruptEnabler;
		pOHCIUIMData->interruptDisabler = oldInterruptDisabler;
	}
	
	return (status);
}
#endif 0 

OSStatus OHCIUIMInterruptHandler (InterruptSetMember member, void *refCon, UInt32 interruptCount)
{
	register OHCIRegistersPtr			pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	register UInt32						activeInterrupts, interruptEnable;
	UInt32								PhysAddr;
	OHCIGeneralTransferDescriptorPtr 	pHCDoneTD;
	
#pragma unused member
#pragma unused refCon
#pragma unused interruptCount

     usb_task_level = kSecondaryInterruptLevel;
	interruptEnable = EndianSwap32Bit(pOHCIRegisters->hcInterruptEnable);
	
	activeInterrupts = interruptEnable & EndianSwap32Bit(pOHCIRegisters->hcInterruptStatus);
	
	if ((interruptEnable & kOHCIHcInterrupt_MIE) && (activeInterrupts != 0)){

		if (activeInterrupts & kOHCIHcInterrupt_SO){  		// SchedulingOverrun Interrupt
			pOHCIUIMData->errors.scheduleOverrun++;
			pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIHcInterrupt_SO);
		}
		if (activeInterrupts & kOHCIHcInterrupt_WDH){		// WritebackDoneHead Interrupt
			PhysAddr = (UInt32) EndianSwap32Bit(*(UInt32 *)(pOHCIUIMData->pHCCA + 0x84));
			PhysAddr &= kOHCIHeadPMask; // mask off interrupt bits
			pHCDoneTD = (OHCIGeneralTransferDescriptorPtr) OHCIUIMGetLogicalAddress (PhysAddr);
			// write to 0 to the HCCA DoneHead ptr so we won't look at it anymore.
			*(UInt32 *)(pOHCIUIMData->pHCCA + 0x84) = 0L;  
			QueueSecondaryInterruptHandler((SecondaryInterruptHandler2)DoDoneQueueProcessing, 
											nil, (void *)pHCDoneTD, (void *)false);
			pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIHcInterrupt_WDH);
		}
		if (activeInterrupts & kOHCIHcInterrupt_SF){		// StartofFrame Interrupt
			// does USL have SF notification set?
			// if so, tell them now's the time!
			
			// Clear the interrrupt
			pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIHcInterrupt_SF);
			
			// and mask it off so it doesn't happen again.
			// will have to be turned on manually to happen again.
			pOHCIRegisters->hcInterruptDisable = EndianSwap32Bit(kOHCIHcInterrupt_SF);
SysDebugStr("Frame Interrupt");			
			QueueSecondaryInterruptHandler(RootHubFrame, nil, nil, nil);

		}
		if (activeInterrupts & kOHCIHcInterrupt_RD){		// ResumeDetected Interrupt
			// does USL have SF notification set?
			// if so, tell them now's the time to resume!
			pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIHcInterrupt_RD);
		}
		if (activeInterrupts & kOHCIHcInterrupt_UE){		// Unrecoverable Error Interrupt
			pOHCIUIMData->errors.unrecoverableError++;
			// Let's do a SW reset to recover from this condition.  We could make sure all 
			// OCHI registers and in-memory data structures are valid, too.
			pOHCIRegisters->hcCommandStatus = EndianSwap32Bit(kOHCIHcCommandStatus_HCR);  
			DelayForHardware(DurationToAbsolute(10*durationMicrosecond));
			pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIHcInterrupt_UE);
			// zzzz - note I'm leaving the Control/Bulk list processing off for now.
			pOHCIRegisters->hcControl = EndianSwapImm32Bit((kOHCIFunctionalState_Operational << kOHCIHcControl_HCFSPhase) | kOHCIHcControl_PLE);
		}
		if (activeInterrupts & kOHCIHcInterrupt_FNO){		// FrameNumberOverflow Interrupt
			// not really an error, but close enough
			pOHCIUIMData->errors.frameNumberOverflow++;	
			if ((EndianSwap32Bit(pOHCIRegisters->hcFmNumber)&kOHCIFmNumberMask) < kOHCIBit15)
				pOHCIUIMData->frameNumber = usb_U64Add (pOHCIUIMData->frameNumber, U64SetU(kOHCIFrameOverflowBit));

			pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIHcInterrupt_FNO);
		}
		if (activeInterrupts & kOHCIHcInterrupt_RHSC){		// RootHubStatusChange Interrupt
 
			// Clear status change.
			pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIHcInterrupt_RHSC);
SysDebugStr("RHSC Interrupt");			
		//	QueueSecondaryInterruptHandler(RootHubStatusChange, nil, nil, nil);
		}
		if (activeInterrupts & kOHCIHcInterrupt_OC){		// OwnershipChange Interrupt
			// well, we certainly weren't expecting this!
			pOHCIUIMData->errors.ownershipChange++;		
			// Let the USL know about the status change?
			pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIHcInterrupt_OC);
		}
			
	} else {
     usb_task_level = kKernelLevel;
		return(kIsrIsNotComplete);   // wasn't our interrupt
	}
     usb_task_level = kKernelLevel;
	return(kIsrIsComplete);  
}


OSStatus OHCIProcessDoneQueue (void) 
{
	OHCIRegistersPtr					pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	OSStatus							status = noErr;
	UInt32 								interruptStatus;
	UInt32								PhysAddr;
	OHCIGeneralTransferDescriptorPtr 	pHCDoneTD;

//	DebugStr ((ConstStr255Param) "OHCIProcessDoneQueue");

	// check if the OHCI has written the DoneHead yet
	interruptStatus = EndianSwap32Bit(pOHCIRegisters->hcInterruptStatus);
	if( (interruptStatus & kOHCIHcInterrupt_WDH) == 0)
	{	
		return(noErr);
	}
	
	// get the pointer to the list (logical address)
	PhysAddr = (UInt32) EndianSwap32Bit(*(UInt32 *)(pOHCIUIMData->pHCCA + 0x84));
	PhysAddr &= kOHCIHeadPMask; // mask off interrupt bits
	pHCDoneTD = (OHCIGeneralTransferDescriptorPtr) OHCIUIMGetLogicalAddress (PhysAddr);
	// write to 0 to the HCCA DoneHead ptr so we won't look at it anymore.
	*(UInt32 *)(pOHCIUIMData->pHCCA + 0x84) = 0L;  
			
	// Since we have a copy of the queue to process, we can let the host update it again.
	pOHCIRegisters->hcInterruptStatus = EndianSwap32Bit(kOHCIHcInterrupt_WDH);

	DoDoneQueueProcessing(pHCDoneTD, true);
	return(true);
}


OSStatus OHCIUIMIsochEDCreate(
	short						functionAddress,
	short						endpointNumber,
	UInt32						maxPacketSize,
	UInt8						direction)
	
{	
	OHCIEndpointDescriptorPtr	pOHCIEndpointDescriptor, pED;
	OSStatus					status = noErr;
	if (direction == kUSBOut)
		direction = kOHCIEDDirectionOut;
	else if (direction == kUSBIn) 
		direction = kOHCIEDDirectionIn;
	else
		direction = kOHCIEDDirectionTD;

	pED = (OHCIEndpointDescriptorPtr) pOHCIUIMData->pIsochHead;
	pOHCIEndpointDescriptor = AddEmptyEndPoint (
					functionAddress, endpointNumber, maxPacketSize, kOHCIEDSpeedFull, direction, pED, kOHCIEDFormatIsochronousTD);
	if (pOHCIEndpointDescriptor == nil)
		return(-1);
	return (status);
}

OSStatus OHCIUIMIsochTransfer(
	short						functionAddress,						
	short						endpointNumber,	
	UInt32						refcon,
	UInt8						direction,
	IsocCallBackFuncPtr			pIsochHandler,							
	UInt64						frameNumberStart,								
	UInt32						pBufferStart,							
	UInt32						frameCount,									
	USBIsocFrame				*pFrames)								
{

	OHCIIsochTransferDescriptorPtr		pITD;
	int									i;
	UInt32								currentFrame;
	UInt32								numOfPages;
	UInt32								bufferSize;
	OHCIEndpointDescriptorPtr			pED, temp;
	UInt32								pageSize, pageMask;
	OSStatus							status = noErr;
	IOPreparationTable					*ioPrep;
	IOPreparationTable					IOPrep;
	UInt32					  			physicalAddresses[kOHCIMaxPages];	
	UInt32								frameTD, pagesUsed;
	UInt32								alreadyCrossed = kOHCIITDConditionNotCrossPage;
	UInt16								frameNumber;
	UInt32								firstCount, lastCount, pageOffsetMask;
//	DebugStr("ISOC check write to 0");

	
	if (direction == kUSBOut)
		direction = kOHCIEDDirectionOut;
	else if (direction == kUSBIn) 
		direction = kOHCIEDDirectionIn;
	else
		direction = kOHCIEDDirectionTD;

	pED = FindIsochronousEndpoint(functionAddress, endpointNumber, direction, &temp);
	
	if (!pED){
		FailureDebugStr("UIMIsochTransfer: Couldnot find endpoint");
		status = kUSBNotFound;
		return (status);
	}
	frameNumber = (UInt16) U32SetU (frameNumberStart);
	ioPrep = &IOPrep;
	pageSize = pOHCIUIMData->pageSize;
	pageMask = ~(pageSize - 1);
	frameTD = 0;
	pageOffsetMask = pageSize - 1;
//  Get the total size of buffer
	bufferSize = 0;
	for ( i = 0; i< frameCount; i++)
		bufferSize += pFrames[i].frReqCount;
	
	currentFrame = 0;
	while (currentFrame < frameCount -1) {
		
		firstCount = pageSize - (pBufferStart & pageOffsetMask);
		if (bufferSize <= firstCount){
			lastCount = 0;
			numOfPages = 1;
		} else {
			lastCount = (pBufferStart + bufferSize) & pageOffsetMask;
			numOfPages = (bufferSize - firstCount - lastCount)/pageSize;
			if (firstCount) numOfPages++;
			if (lastCount) numOfPages++;
		}
		// Prepare for IO and get physical address
		ioPrep->options = kIOLogicalRanges | kIOIsInput | kIOIsOutput;
		ioPrep->addressSpace = kCurrentAddressSpaceID;			// default
		ioPrep->granularity = 0;								// do it all now
		ioPrep->firstPrepared = 0;
		ioPrep->mappingEntryCount = MIN (numOfPages, kOHCIMaxPages);				// # of pages we will use
		ioPrep->logicalMapping = 0;
		ioPrep->physicalMapping = (PhysicalMappingTablePtr) physicalAddresses;	// return list of phys addrs
		ioPrep->rangeInfo.range.base = (void *) pBufferStart;
		ioPrep->rangeInfo.range.length =  MIN (numOfPages, kOHCIMaxPages) * pageSize;

		status = PrepareMemoryForIO (ioPrep);
		pITD = (OHCIIsochTransferDescriptorPtr) pED->pVirtualTailP;
		pagesUsed = 0;
		while (((pagesUsed < kOHCIMaxPages -1) || (pagesUsed == numOfPages-1)) && (currentFrame < frameCount-1))
		{		 
			if (frameTD == 8){
				// make current ITD = 7 frames
				pITD->dWord0 |= (frameTD-1) << kOHCIITDControl_FCPhase;

				pITD->dWord3 = physicalAddresses[pagesUsed] + ((pBufferStart-1) & ~pageMask);
				// queue another pITD
				pITD = QueueITD(pITD, pED);
				frameTD = 0;
				alreadyCrossed = kOHCIITDConditionNotCrossPage;
			}
			// check if buffer crosses page
			if ((pBufferStart & pageMask) != ((pBufferStart + pFrames[currentFrame].frReqCount) & pageMask)) {
				if (alreadyCrossed == kOHCIITDConditionCrossPage) {
					// fill in value for number of frames
					pITD->dWord0 |= (frameTD-1) << kOHCIITDControl_FCPhase;

					pITD->dWord3 = physicalAddresses[pagesUsed] + ((pBufferStart-1) & ~pageMask);
					// queue another ITD
					pITD = QueueITD(pITD, pED);
					frameTD = 0;
				}
				pagesUsed++;
				alreadyCrossed = kOHCIITDConditionCrossPage;
			}
			if (frameTD == 0) {
				pITD->offset[frameTD] = (pBufferStart & ~pageMask) | 
										kOHCIITDConditionNotAccessed << kOHCIITDOffset_CCPhase  | 
										alreadyCrossed << kOHCIITDOffset_PCPhase;
				if (alreadyCrossed == kOHCIITDConditionCrossPage)
					pITD->dWord1 = physicalAddresses[pagesUsed-1] & pageMask;
				else
					pITD->dWord1 = physicalAddresses[pagesUsed] & pageMask;
				pITD->dWord0 |= frameNumber;
				pITD->pIsocFrame = (UInt32) pFrames;
				pITD->frameNum = currentFrame;
				frameTD++;
			}
			// fill in normally, if alreadycrossed = true, make the 12th bit a 1
			pITD->offset[frameTD] = kOHCIITDConditionNotAccessed << kOHCIITDOffset_CCPhase  | 
									alreadyCrossed << kOHCIITDOffset_PCPhase |
									((pBufferStart+pFrames[currentFrame].frReqCount) & ~pageMask) 
									+ (physicalAddresses[pagesUsed] & ~pageMask);							
			frameTD++;
			frameNumber++;
			bufferSize -= pFrames[currentFrame].frReqCount;
			pBufferStart += pFrames[currentFrame].frReqCount;
			currentFrame++;
		}
		
		CheckpointIO(ioPrep->preparationID, nil); 	
	}
	
	pITD->dWord0 |= (frameTD-1) << kOHCIITDControl_FCPhase;
// barry stuff
//	pITD->dWord3 = physicalAddresses[pagesUsed] + 2*pFrames[currentFrame].frReqCount - 1;
	pBufferStart += pFrames[currentFrame].frReqCount;
	pITD->dWord3 = physicalAddresses[pagesUsed] + ((pBufferStart-1) & ~pageMask);
	pITD->refcon = refcon;
	pITD->handler = pIsochHandler;
	QueueITD(pITD, pED);
//	DebugStr("ISOC check write to 0");
	return(0);

}

OHCIIsochTransferDescriptorPtr QueueITD (
	OHCIIsochTransferDescriptorPtr 	pITD, 
	OHCIEndpointDescriptorPtr 		pED)
{
	OHCIIsochTransferDescriptorPtr		pITDTemp;
	
	pITDTemp = OHCIUIMAllocateITD();
	pITD->dWord2 = pITDTemp->pPhysical;  // assign pitdnext to pitdtemp
	pED->pVirtualTailP = pITD->pVirtualNext = (UInt32) pITDTemp;
	SwapIsoc(pITD);
	pED->dWord1 = pITD->dWord2;			// assign tail pointer to pitd next
	return (pITDTemp);
}


void SwapIsoc(OHCIIsochTransferDescriptorPtr pITD) 
{
	pITD->dWord0= EndianSwap32Bit(pITD->dWord0);
	pITD->dWord1= EndianSwap32Bit(pITD->dWord1);
	pITD->dWord2= EndianSwap32Bit(pITD->dWord2);
	pITD->dWord3= EndianSwap32Bit(pITD->dWord3);
	
	/* barry stuff BT 16bit words in right order, but each is swapped */
	pITD->offset[0] = EndianSwap16Bit(pITD->offset[0]);
	pITD->offset[1] = EndianSwap16Bit(pITD->offset[1]);
	pITD->offset[2] = EndianSwap16Bit(pITD->offset[2]);
	pITD->offset[3] = EndianSwap16Bit(pITD->offset[3]);
	pITD->offset[4] = EndianSwap16Bit(pITD->offset[4]);
	pITD->offset[5] = EndianSwap16Bit(pITD->offset[5]);
	pITD->offset[6] = EndianSwap16Bit(pITD->offset[6]);
	pITD->offset[7] = EndianSwap16Bit(pITD->offset[7]);
}

void trivialSwap( UInt32 *thing) {

	*thing = EndianSwap32Bit (*thing);
}




OSStatus
DoDoneQueueProcessing(OHCIGeneralTransferDescriptorPtr pHCDoneTD, Boolean immediateFlag)
{
	UInt32						control, transferStatus;
	long						bufferSizeRemaining;
	OHCIGeneralTransferDescriptorPtr
								prevTD, nextTD;
	UInt32						PhysAddr;
	UInt32						pageSize;
	UInt32						pageMask;
	OHCIEndpointDescriptorPtr	tempED;
	OHCIRegistersPtr			pOHCIRegisters;
	OHCIIsochTransferDescriptorPtr		pITD;

#pragma unused immediateFlag
	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;

	if(pHCDoneTD == nil)
	{	/* This should not happen */
		return(noErr);
	}
//	DebugStr ((ConstStr255Param) "DoDoneQueueProcessing");

	pageSize = pOHCIUIMData->pageSize;
	pageMask = ~(pageSize - 1);

	/* Reverse the done queue use only the virtual Address fields */
	prevTD = 0;
	while(pHCDoneTD != nil)
	{
		PhysAddr = (UInt32) EndianSwap32Bit(pHCDoneTD->dWord2) & kOHCIHeadPMask;
		nextTD = (OHCIGeneralTransferDescriptorPtr) OHCIUIMGetLogicalAddress (PhysAddr);
		pHCDoneTD->pVirtualNext = (UInt32)prevTD;
		prevTD = pHCDoneTD;
		pHCDoneTD = nextTD;
	}
//	DebugStr ((ConstStr255Param) "DoDoneQueueProcessing queue reversed");
	
	pHCDoneTD = prevTD;	/* New qHead */
	
	while (pHCDoneTD != nil) 
	{
		// find the next one
		nextTD	= (void *)pHCDoneTD->pVirtualNext;

		control = EndianSwap32Bit(pHCDoneTD->dWord0);
		transferStatus = (control & kOHCIGTDControl_CC) >> kOHCIGTDControl_CCPhase;
		if (pOHCIUIMData->OptiOn && (pHCDoneTD->pType == kOHCIOptiLSBug)) {
			// clear any bad errors
			tempED = (OHCIEndpointDescriptorPtr) pHCDoneTD->pEndpoint;
			pHCDoneTD->dWord0 = pHCDoneTD->dWord0 & EndianSwap32Bit (kOHCIGTDClearErrorMask);
			tempED->dWord2 &=  EndianSwap32Bit (kOHCIHeadPMask);
			pHCDoneTD->dWord2 = tempED->dWord1 & EndianSwap32Bit (kOHCIHeadPMask); 
			tempED->dWord1 = EndianSwap32Bit (pHCDoneTD->pPhysical);
			pOHCIRegisters->hcCommandStatus = EndianSwap32Bit (kOHCIHcCommandStatus_CLF);

		// For CMD Buffer Underrun Errata
		} else if ((transferStatus == kOHCIGTDConditionBufferUnderrun) && 
						(pHCDoneTD->pType == kOHCIBulkTransferOutType) && 
						(pOHCIUIMData->errataBits & kErrataRetryBufferUnderruns)) {
		

			tempED = (OHCIEndpointDescriptorPtr) pHCDoneTD->pEndpoint;
			pHCDoneTD->dWord0 = pHCDoneTD->dWord0 & EndianSwap32Bit (kOHCIGTDClearErrorMask);
			pHCDoneTD->dWord2 = tempED->dWord2 & EndianSwap32Bit (kOHCIHeadPMask); 
			pHCDoneTD->pVirtualNext =  OHCIUIMGetLogicalAddress (EndianSwap32Bit (tempED->dWord3) & kOHCIHeadPMask);
			
			tempED->dWord2 = EndianSwap32Bit (pHCDoneTD->pPhysical) | 
										(tempED->dWord2 & EndianSwap32Bit( kOHCIEDToggleBitMask));
			pOHCIRegisters->hcCommandStatus = EndianSwap32Bit (kOHCIHcCommandStatus_BLF);
		
		
		
		} else if (pHCDoneTD->pType == kOHCIIsochronousType) {
			// cast to a isoc type
			pITD = (OHCIIsochTransferDescriptorPtr) pHCDoneTD;
			ProcessCompletedITD(pITD);
			// deallocate td
			OHCIUIMDeallocateITD(pITD);
			
		} else {

			if (pHCDoneTD->preparationID)
				CheckpointIO(pHCDoneTD->preparationID, nil);
			pHCDoneTD->preparationID = nil;
			
			bufferSizeRemaining = findBufferRemaining (pHCDoneTD);
			if (pHCDoneTD->CallBack){
				CallBackFuncPtr pCallBack;
				// zero out callback first than call it
				pCallBack = pHCDoneTD->CallBack;
				pHCDoneTD->CallBack = nil;
				(*pCallBack) (pHCDoneTD->refcon, transferStatus, bufferSizeRemaining);
			} else if (transferStatus != nil) 
				doCallback(pHCDoneTD, transferStatus, bufferSizeRemaining);

			OHCIUIMDeallocateTD(pHCDoneTD);
		}
		pHCDoneTD = nextTD;	/* New qHead */
	}

	return(noErr);
}



void doCallback(
	OHCIGeneralTransferDescriptorPtr 			nextTD,
	UInt32 			transferStatus,
	UInt32 			bufferSizeRemaining) 
{
	OHCIGeneralTransferDescriptorPtr	pCurrentTD;
	OHCIEndpointDescriptorPtr			pED;
	UInt32								PhysAddr;
	
	pED = (OHCIEndpointDescriptorPtr) nextTD->pEndpoint;
	
	PhysAddr = (UInt32) EndianSwap32Bit(pED->dWord2) & kOHCIHeadPMask;
	nextTD = (OHCIGeneralTransferDescriptorPtr) OHCIUIMGetLogicalAddress (PhysAddr);

	pCurrentTD = (OHCIGeneralTransferDescriptorPtr) nextTD;
	while (pCurrentTD->pVirtualNext != nil) 
	{
	
		bufferSizeRemaining += findBufferRemaining (pCurrentTD);
		pCurrentTD->dWord1 = 0;	// make sure this TD won't be added to any future buffer remaining calculations

		if (pCurrentTD->preparationID)
			CheckpointIO(pCurrentTD->preparationID, nil);
		pCurrentTD->preparationID = nil;  // make sure we don't try to do another CheckpointIO later

		if (pCurrentTD->CallBack!=nil) {
			CallBackFuncPtr pCallBack;
			// zero out callback first than call it
			pCallBack = pCurrentTD->CallBack;
			pCurrentTD->CallBack = nil;
			(*pCallBack) (pCurrentTD->refcon, transferStatus, bufferSizeRemaining);
			bufferSizeRemaining = nil;
			return;
		}
		
		pCurrentTD = (OHCIGeneralTransferDescriptorPtr) pCurrentTD->pVirtualNext;
	}
}


UInt32 findBufferRemaining (OHCIGeneralTransferDescriptorPtr pCurrentTD)
{
	UInt32						pageSize;
	UInt32						pageMask;
	UInt32						bufferSizeRemaining;
	pageSize = pOHCIUIMData->pageSize;
	pageMask = ~(pageSize - 1);

		if (pCurrentTD->dWord1 == 0){
			bufferSizeRemaining = 0;
		}else if ((EndianSwap32Bit(pCurrentTD->dWord3) & (pageMask)) == 
						(EndianSwap32Bit(pCurrentTD->dWord1)& (pageMask))) {
			// we're on the same page
			bufferSizeRemaining = (EndianSwap32Bit (pCurrentTD->dWord3) & ~pageMask) - 
									(EndianSwap32Bit (pCurrentTD->dWord1) & ~pageMask) + 1;
		} else {
			bufferSizeRemaining = ((EndianSwap32Bit(pCurrentTD->dWord3) & ~pageMask) + 1)  +  
									(pageSize - (EndianSwap32Bit(pCurrentTD->dWord1) & ~pageMask));
		}

	return (bufferSizeRemaining);
}


OSStatus OHCIUIMControlInitialize () 
{
	OHCIRegistersPtr			pOHCIRegisters;
	OHCIEndpointDescriptorPtr	pED, pED2;


	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	
	// Create ED mark it skipped and assign it to ControlTail
	pED = OHCIUIMAllocateED();
	pED->dWord0 = EndianSwap32Bit (kOHCIEDControl_K);
	pOHCIUIMData->pControlTail = (UInt32) pED;
//kprintf("OHCIUIMControlInitialize:pOHCIUIMData->pControlTail=0x%x\n",pED);	
	// Create ED mark it skipped and assign it to control head
	pED2 = OHCIUIMAllocateED();
	pED2->dWord0 = EndianSwap32Bit (kOHCIEDControl_K);
	pOHCIUIMData->pControlHead = (UInt32) pED2;
//kprintf("OHCIUIMControlInitialize:pOHCIUIMData->pControlHead=0x%x\n",pED2);	
	pOHCIRegisters->hcControlHeadED = EndianSwap32Bit ((UInt32) pED2->pPhysical);
	
	// have bulk head ED point to Control tail ED
	pED2->dWord3 = EndianSwap32Bit ((UInt32) pED->pPhysical);
	pED2->pVirtualNext = (UInt32) pED;
//kprintf("OHCIUIMControlInitialize:pOHCIUIMData->pControlHead->pVirtualNext=0x%x\n",pED);	
	return (0);
}


OSStatus OHCIUIMBulkInitialize (){

	OHCIRegistersPtr			pOHCIRegisters;
	OHCIEndpointDescriptorPtr	pED, pED2;


	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	
	// Create ED mark it skipped and assign it to bulkTail
	pED = OHCIUIMAllocateED();
	pED->dWord0 = EndianSwap32Bit (kOHCIEDControl_K);
	pOHCIUIMData->pBulkTail = (UInt32) pED;
	
	// Create ED mark it skipped and assign it to bulk head
	pED2 = OHCIUIMAllocateED();
	pED2->dWord0 = EndianSwap32Bit (kOHCIEDControl_K);
	pOHCIUIMData->pBulkHead = (UInt32) pED2;
	pOHCIRegisters->hcBulkHeadED = EndianSwap32Bit ((UInt32) pED2->pPhysical);
	
	// have bulk head ED point to Bulk tail ED
	pED2->dWord3 = EndianSwap32Bit ((UInt32) pED->pPhysical);
	pED2->pVirtualNext = (UInt32) pED;
	return (0);

}

OSStatus OHCIUIMIsochronousInitialize()
{

	OHCIRegistersPtr			pOHCIRegisters;
	OHCIEndpointDescriptorPtr	pED, pED2;


	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	
	// Create ED mark it skipped and assign it to bulkTail
	pED = OHCIUIMAllocateED();
	pED->dWord0 = EndianSwap32Bit (kOHCIEDControl_K);
	pOHCIUIMData->pIsochTail = (UInt32) pED;
	
	// Create ED mark it skipped and assign it to bulk head
	pED2 = OHCIUIMAllocateED();
	pED2->dWord0 = EndianSwap32Bit (kOHCIEDControl_K);
	pOHCIUIMData->pIsochHead = (UInt32) pED2;
	

	// have bulk head ED point to Bulk tail ED
	pED2->dWord3 = EndianSwap32Bit ((UInt32) pED->pPhysical);
	pED2->pVirtualNext = (UInt32) pED;
	return (0);

}

//Initializes the HCCA Interrupt list with statically 
//disabled ED's to form the Interrupt polling queues
OSStatus OHCIUIMInterruptInitialize ()
{

	OHCIRegistersPtr			pOHCIRegisters;
	OHCIIntHeadPtr				pInterruptHead;
	OSStatus					status = 0;
	UInt32						dummyControl;
	int							i, p, q, z;
	OHCIEndpointDescriptorPtr	pED, pIsochHead;	
	
	
	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	pInterruptHead = pOHCIUIMData->pInterruptHead;
	
	// create UInt32 with same dword0 for use with searching and tracking, skip should be set, and open area shouldbe marked
	dummyControl = kOHCIEDControl_K;
	dummyControl |= 0;   //should be kOHCIFakeED
	dummyControl = EndianSwap32Bit (dummyControl);
	pIsochHead = (OHCIEndpointDescriptorPtr) pOHCIUIMData->pIsochHead;
	
	//do 31 times
	// change to 65 and make isoch head the last one.?????
	for (i = 0; i < 63; i++)
	{ 
		//allocate Endpoint descriptor
		pED = OHCIUIMAllocateED();
		if (pED == nil){
			status = -1;
			return (status);
		}
		//mark skipped,some how mark as a False endpoint zzzzz
		else {

			pED->dWord0 = dummyControl;
			pInterruptHead[i].pHead = pED;
			pInterruptHead[i].pHeadPhysical = pED->pPhysical;
			pInterruptHead[i].nodeBandwidth = 0;
		}
		if ( i < 32)
	 		((UInt32 *)pOHCIUIMData->pHCCA)[i] = (UInt32) EndianSwap32Bit ((UInt32) pInterruptHead[i].pHeadPhysical);

	}
	
	p = 0;
	q = 32;
 	//
	for (i = 0; i < (32 +16 + 8 + 4 + 2); i++)
	{
		if (i < q/2+p)
			z = i + q;
		else
			z = i + q/2;
		if (i == p+q-1)
		{
			p = p + q;
			q = q/2;
		}
		//point endpoint descriptor to corresponding 8ms descriptor
		pED = pInterruptHead[i].pHead;
		pED->dWord3 =  EndianSwap32Bit (pInterruptHead[z].pHeadPhysical);
		pED->pVirtualNext = (UInt32) pInterruptHead[z].pHead;
		pInterruptHead[i].pTail = (OHCIEndpointDescriptorPtr) pED->pVirtualNext;

	}
	i = 62;
	pED = pInterruptHead[i].pHead;
	pED->dWord3 = EndianSwap32Bit (pIsochHead->pPhysical);
	pED->pVirtualNext = (UInt32) pOHCIUIMData->pIsochHead;
	pInterruptHead[i].pTail = (OHCIEndpointDescriptorPtr) pED->pVirtualNext;

	// point Isochronous head to last endpoint 

		
	return (status);
}

/*
// determines the best fit for the interrupt  ED
OSStatus OHCIUIMInterruptschedule()
{
}


//determines whether or not the ED can fit
OSStatus OHCIUIMInterruptfits()
{
}

// determines the worst and best bandwidth
// for use with scheduling periodic ed's
OSStatus OHCIUIMInterruptBandwidthCheck()

{
}

//Finds an ED in the interrupt queue
OSStatus OHCIUIMInterruptFindED()
{
}
*/
//Allocate 
OSStatus OHCIUIMAllocateMemory (
	int 								num_of_TDs,
	int									num_of_EDs,
	int									num_of_ITDs)
{	
	OSStatus							status;
	Ptr									p;
	UInt32								physical;
	int									tdsPage,pagesTD,edsPage,pagesED, itdsPage, pagesITD;
	UInt32								pageSize;
	OHCIEndpointDescriptorPtr			FreeED, FreeEDCurrent;
	OHCIGeneralTransferDescriptorPtr	FreeTD, FreeTDCurrent;
	OHCIIsochTransferDescriptorPtr		FreeITD, FreeITDCurrent;
	int									i,j;
	
	
	pageSize = pOHCIUIMData->pageSize;
kprintf("pgsize=%d,sizes=%d,%d,%d\n",pageSize,sizeof(OHCIGeneralTransferDescriptor),sizeof(OHCIEndpointDescriptor),sizeof(OHCIIsochTransferDescriptor));


	tdsPage = pageSize/sizeof (OHCIGeneralTransferDescriptor);
	pagesTD = num_of_TDs /tdsPage +1;
	edsPage = pageSize/sizeof (OHCIEndpointDescriptor);
	pagesED = num_of_EDs/edsPage +1;
	itdsPage = pageSize/sizeof (OHCIIsochTransferDescriptor);
	pagesITD = num_of_ITDs /itdsPage +1;

	// use pool allocate to allocate a large sum of memory  zzzzz why plus 3???

kprintf("trying to allocate %d+%d+%d+1 * %d\n",pagesED,pagesTD,pagesITD,pageSize);      
p = Desc_Buffer_Usb;

kprintf("kalloc p=0x%x\n",p);

//naga 	pOHCIUIMData->pDataAllocation = p;
	//page align and 16 byte align(page align automagically makes it 16 byte aligned)
	p = (Ptr) (((UInt32) p + (pageSize - 1)) & ~(pageSize - 1));	// page-align

	// use prepare mem for IO to find physical address
	physical = OHCIUIMGetPhysicalAddress((UInt32) p, pagesED+pagesTD+pagesITD);

	// create a list of unused ED's, filling in Virtual address, physicaladdress and virtual next
	// physical next.
	FreeED = (OHCIEndpointDescriptorPtr) p;
	FreeEDCurrent = FreeED;
	pOHCIUIMData->pFreeED = FreeED;

	
	for (i = 0;i<pagesED;i++)
	{
		for (j = 0; j < edsPage; j++)
		{
			//create EDs
			FreeEDCurrent[j].pPhysical = physical + (j *sizeof (OHCIEndpointDescriptor));
			FreeEDCurrent[j].pVirtualNext = (UInt32) (&FreeEDCurrent[j+1]);
		}
		if (i != (pagesED - 1)){
			FreeEDCurrent[j-1].pVirtualNext =  ((UInt32) FreeEDCurrent + pageSize);
		} else {
			FreeEDCurrent[j-1].pVirtualNext = nil;
			pOHCIUIMData->pLastFreeED = &FreeEDCurrent[j-1];
		}
			
		// goto next page
		FreeEDCurrent = (OHCIEndpointDescriptorPtr) ((UInt32) FreeEDCurrent + pageSize);
		physical += pageSize;
	}
	FreeTD = (OHCIGeneralTransferDescriptorPtr) FreeEDCurrent;
	FreeTDCurrent = FreeTD;
	pOHCIUIMData->pFreeTD = FreeTD;
	for (i = 0;i<pagesTD;i++)
	{
		for (j = 0; j < tdsPage; j++)
		{
			//create TDs
			FreeTDCurrent[j].pPhysical = physical + (j * sizeof (OHCIGeneralTransferDescriptor));
			FreeTDCurrent[j].pVirtualNext = (UInt32) (&FreeTDCurrent[j+1]);

		}
		if (i != (pagesTD - 1)){ 
			FreeTDCurrent[j-1].pVirtualNext = ((UInt32) FreeTDCurrent + pageSize);
		} else {
			FreeTDCurrent[j-1].pVirtualNext = nil;
			pOHCIUIMData->pLastFreeTD = &FreeTDCurrent[j-1];
		}
			
		// goto next page
		FreeTDCurrent = (OHCIGeneralTransferDescriptorPtr) ((UInt32) FreeTDCurrent + pageSize);
		physical += pageSize;

	}
	
	
	// set up freeitd queue
	FreeITD = (OHCIIsochTransferDescriptorPtr) FreeTDCurrent;
	FreeITDCurrent = FreeITD;
	pOHCIUIMData->pFreeITD = FreeITD;
	for (i = 0;i<pagesTD;i++)
	{
		for (j = 0; j < tdsPage; j++)
		{
			//create TDs
			FreeITDCurrent[j].pPhysical = physical + (j * sizeof (OHCIIsochTransferDescriptor));
			FreeITDCurrent[j].pVirtualNext = (UInt32) (&FreeITDCurrent[j+1]);

		}
		if (i != (pagesITD - 1)){ 
			FreeITDCurrent[j-1].pVirtualNext = ((UInt32) FreeITDCurrent + pageSize);
		} else {
			FreeITDCurrent[j-1].pVirtualNext = nil;
			pOHCIUIMData->pLastFreeITD = &FreeITDCurrent[j-1];
		}
			
		// goto next page
		FreeITDCurrent = (OHCIIsochTransferDescriptorPtr) ((UInt32) FreeITDCurrent + pageSize);
		physical += pageSize;

	}
		
	// create a list of unused buffers?????
	
	return (status);
	
}

OHCIIsochTransferDescriptorPtr OHCIUIMAllocateITD()

{
	OHCIIsochTransferDescriptorPtr temp, FreeTD;
	
// pop a TD off of FreeTD list
//if FreeTD == NIL return nil
// should we check if ED is full and if not access that????
	temp = FreeTD = pOHCIUIMData->pFreeITD;

	if (FreeTD != nil)
	{	 
		pOHCIUIMData->pFreeITD = (OHCIIsochTransferDescriptorPtr) FreeTD->pVirtualNext;
	} else {
		AssertDebugStr("OHCIUIM: Out of Transfer Descriptors!  ");
	}
	
	/* BT, I think I need to do this here */
	temp->pType = kOHCIIsochronousType;

	return (temp);

}

OHCIGeneralTransferDescriptorPtr OHCIUIMAllocateTD()

{
	OHCIGeneralTransferDescriptorPtr temp, FreeTD;
	
// pop a TD off of FreeTD list
//if FreeTD == NIL return nil
// should we check if ED is full and if not access that????
	temp = FreeTD = pOHCIUIMData->pFreeTD;

	if (FreeTD != nil)
	{	 
		pOHCIUIMData->pFreeTD = (OHCIGeneralTransferDescriptorPtr) FreeTD->pVirtualNext;
		temp->pVirtualNext = nil;
	} else {
		AssertDebugStr("OHCIUIM: Out of Transfer Descriptors!  ");
	}
	return (temp);

}

OHCIEndpointDescriptorPtr OHCIUIMAllocateED()
{
	OHCIEndpointDescriptorPtr temp, FreeED;
	

	// Pop a ED off the FreeED list
	// If FreeED == nil return Error
	temp = FreeED = (OHCIEndpointDescriptorPtr) pOHCIUIMData->pFreeED;

	if (pOHCIUIMData->pFreeED != nil)
	{	 
		pOHCIUIMData->pFreeED = (OHCIEndpointDescriptorPtr) FreeED->pVirtualNext;
		temp->pVirtualNext = nil;
	} else {
		AssertDebugStr("OHCIUIM: Out of Endpoint Descriptors!  ");
	}
	return (temp);
}

OSStatus OHCIUIMDeallocateITD (
	OHCIIsochTransferDescriptorPtr pTD)
{
	UInt32								physical;

	//zero out all unnecessary fields
	physical = pTD->pPhysical;
	BlockZero(pTD, sizeof(*pTD));
	pTD->pPhysical = physical;
	pTD->pType = kOHCIIsochronousType;
	if (pOHCIUIMData->pFreeITD){
		pOHCIUIMData->pLastFreeITD->pVirtualNext = (UInt32)pTD;
		pOHCIUIMData->pLastFreeITD = pTD;
	} else {
		// list is currently empty
		pOHCIUIMData->pLastFreeITD = pTD;
		pOHCIUIMData->pFreeITD = pTD;
	}
	return (0);
}


OSStatus OHCIUIMDeallocateTD (
	OHCIGeneralTransferDescriptorPtr pTD)
{
	UInt32						physical;

	//zero out all unnecessary fields
	physical = pTD->pPhysical;
	BlockZero(pTD, sizeof(*pTD));
	pTD->pPhysical = physical;
	
	if (pOHCIUIMData->pFreeTD){
		pOHCIUIMData->pLastFreeTD->pVirtualNext = (UInt32)pTD;
		pOHCIUIMData->pLastFreeTD = pTD;
	} else {
		// list is currently empty
		pOHCIUIMData->pLastFreeTD = pTD;
		pOHCIUIMData->pFreeTD = pTD;
	}
	return (0);
}

OSStatus OHCIUIMDeallocateED (
	OHCIEndpointDescriptorPtr pED)
{
	UInt32						physical;

	//zero out all unnecessary fields
	physical = pED->pPhysical;
	BlockZero(pED, sizeof(*pED));
	pED->pPhysical = physical;

	if (pOHCIUIMData->pFreeED){
		pOHCIUIMData->pLastFreeED->pVirtualNext = (UInt32)pED;
		pOHCIUIMData->pLastFreeED = pED;
	} else {
		// list is currently empty
		pOHCIUIMData->pLastFreeED = pED;
		pOHCIUIMData->pFreeED = pED;
	}
	return (0);
}
/*
	
OSStatus OHCIUIMAllocateBuffer()
{}
*/


////////////////////////////////////////////////////////////////////////////////
//
//		UInt32 OHCIUIMGetLogicalAddress
//		Given the physical address, return the virtual address
//

UInt32 OHCIUIMGetLogicalAddress (
	UInt32 				pPhysicalAddress)
{
	OHCIPhysicalLogicalPtr		pPhysicalLogical;
	UInt32						LogicalAddress = nil;
		
	if (pPhysicalAddress == 0)
		return(0);

	pPhysicalLogical = pOHCIUIMData->pPhysicalLogical;
		
	while (pPhysicalLogical != nil) {
		if (pPhysicalAddress <= pPhysicalLogical->PhysicalEnd 
				&& pPhysicalAddress >= pPhysicalLogical->PhysicalStart) 
		{
			LogicalAddress = pPhysicalLogical->LogicalStart + (pPhysicalAddress - pPhysicalLogical->PhysicalStart);
			pPhysicalLogical = nil;
		} else {
			pPhysicalLogical = (OHCIPhysicalLogicalPtr) pPhysicalLogical->pNext;
		}
	}
	
	if ( LogicalAddress == nil) 
		AssertDebugStr("OHCIUIM: LogicalAddress == nil !");

	return (LogicalAddress);

}
		

UInt32 OHCIUIMGetPhysicalAddress(
	UInt32						LogicalAddress,
	UInt32						count)
{
	OHCIPhysicalLogicalPtr		pPhysicalLogical;
	UInt32						PhysicalAddress = nil;
		
	if (LogicalAddress == 0)
		return(0);

	pPhysicalLogical = pOHCIUIMData->pPhysicalLogical;
		
	while (pPhysicalLogical != nil) {
		if (LogicalAddress <= pPhysicalLogical->LogicalEnd 
				&& LogicalAddress >= pPhysicalLogical->LogicalStart) 
		{
			PhysicalAddress = pPhysicalLogical->PhysicalStart + (LogicalAddress - pPhysicalLogical->LogicalStart);
			pPhysicalLogical = nil;
		} else {
			pPhysicalLogical = (OHCIPhysicalLogicalPtr) pPhysicalLogical->pNext;
		}
	}

	if (PhysicalAddress == nil)
		PhysicalAddress = OHCIUIMCreatePhysicalAddress(LogicalAddress, count);
		
	return (PhysicalAddress);
}
		
UInt32 OHCIUIMCreatePhysicalAddress(
	UInt32						pLogicalAddress,
	UInt32						count)

{
	UInt32						pageSize;
	PhysicalMappingTablePtr		physicalAddressTable;
	OHCIPhysicalLogicalPtr		pPhysicalLogical;
	IOPreparationTable			*ioPrep;
	IOPreparationTable			IOPrep;
	OSStatus					status;
	OHCIPhysicalLogicalPtr		p;
	
	pPhysicalLogical = pOHCIUIMData->pPhysicalLogical;
	pageSize = pOHCIUIMData->pageSize;
	
	// zzzzz - revisit this regarding the size of the table  do we deallocate this?
	physicalAddressTable = (PhysicalMappingTablePtr)PoolAllocateResident((count)*sizeof(UInt32), false);

	// zzz do we deallocate this?
	p = (OHCIPhysicalLogicalPtr) PoolAllocateResident (sizeof (OHCIPhysicalLogical), true);

	p->LogicalStart =  pLogicalAddress;
	p->PhysicalStart = kvtophys((vm_offset_t)pLogicalAddress);
	p->LogicalEnd = p->LogicalStart + count*pageSize-1;
	p->PhysicalEnd = p->PhysicalStart + count*pageSize-1;
	p->pNext = nil;

	//now put it all in a table 
	if (pPhysicalLogical == nil) 
	{
		pPhysicalLogical = p;
	} else {
		//traverse Queue
		while (pPhysicalLogical->pNext != nil) 
		{
			pPhysicalLogical = (OHCIPhysicalLogicalPtr) pPhysicalLogical->pNext;
		}
		pPhysicalLogical->pNext = (UInt32) p;
	}
	pOHCIUIMData->pPhysicalLogical = pPhysicalLogical;

	return (p->PhysicalStart);
}



static void returnTransactions(OHCIGeneralTransferDescriptor *transaction, UInt32 tail)
{
	UInt32 							physicalAddress;
	OHCIGeneralTransferDescriptor 	*nextTransaction;
	
	while(transaction->pPhysical != tail)
	{
		if(transaction == nil)
		{
			AssertDebugStr("Return queue broken");
			break;
		}
		else
		{
			if (transaction->preparationID)
				CheckpointIO(transaction->preparationID, nil);
			transaction->preparationID = nil;

			if (transaction->CallBack) {
				CallBackFuncPtr pCallBack;
				// zero out callback first than call it
				pCallBack = transaction->CallBack;
				transaction->CallBack = nil;
				(*pCallBack) (transaction->refcon, returnedErr, 0);
			}
			/* walk the physically-addressed list */
			physicalAddress = (UInt32) EndianSwap32Bit(transaction->dWord2) & kOHCIHeadPMask;
			nextTransaction = (OHCIGeneralTransferDescriptorPtr) OHCIUIMGetLogicalAddress (physicalAddress);
			OHCIUIMDeallocateTD(transaction);
			transaction = nextTransaction;
		}
	}
}


OSStatus OHCIUIMClearEndPointStall(
	short 						functionNumber,
	short						endpointNumber,
	short						direction)
{
	OHCIRegistersPtr			pOHCIRegisters;
	OSStatus					status = noErr;
	OHCIEndpointDescriptorPtr	pEDQueueBack, pED;
	OHCIGeneralTransferDescriptor	 *transaction;
	UInt32 tail, controlMask;


	if (pOHCIUIMData->rootHubFuncAddress == functionNumber) 
		return (status);

	if (direction == kUSBOut)
		direction = kOHCIEDDirectionOut;
	else if (direction == kUSBIn) 
		direction = kOHCIEDDirectionIn;
	else
		direction = kOHCIEDDirectionTD;

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	
	transaction = nil;
	tail = nil;
kprintf("OHCIUIMClear EPS:calling FindEndpoint\n");
	if((pED = FindEndpoint (functionNumber, endpointNumber, direction, &pEDQueueBack, &controlMask)) == nil)
		return(kUSBNotFound);

	if (pED != nil) {
		tail = EndianSwap32Bit(pED->dWord1);
		transaction = (void *)OHCIUIMGetLogicalAddress(EndianSwap32Bit(pED->dWord2) & kOHCIHeadPMask);
		pED->dWord2 = pED->dWord1;	/* unlink all transactions at once (this also clears the halted bit) */
		pED->pVirtualHeadP = pED->pVirtualTailP; 
	}	

	if (transaction != nil){
		returnTransactions(transaction, tail);
	}
	return (status);
}

OHCIEndpointDescriptorPtr FindControlEndpoint (
	short 						functionNumber, 
	short						endpointNumber, 
	OHCIEndpointDescriptorPtr   *pEDBack)
{
	UInt32						unique;
	OHCIEndpointDescriptorPtr	pEDQueue;
	OHCIEndpointDescriptorPtr	pEDQueueBack;

	
	//search for endpoint descriptor
	unique = (UInt32) ((((UInt32) endpointNumber) << kOHCIEndpointNumberOffset) | ((UInt32) functionNumber));
	pEDQueueBack = (OHCIEndpointDescriptorPtr) pOHCIUIMData->pControlHead;
	pEDQueue = (OHCIEndpointDescriptorPtr) pEDQueueBack->pVirtualNext;
//kprintf("FindControlEp:pOHCIUIMData->pControlHead=0x%x,pEDQ=pOHCIUIMData->pControlHead->pVirtualNext=0x%x,pOHCIUIMData->pControlTail=0x%x\n",pEDQueueBack,pEDQueue,pOHCIUIMData->pControlTail);

	while ((UInt32) pEDQueue != pOHCIUIMData->pControlTail) 
	{
//kprintf("FindControlEp:searching for %d(unique),dword0=%d\n",unique, (EndianSwap32Bit(pEDQueue->dWord0) & kUniqueNumNoDirMask));
		if ( (EndianSwap32Bit(pEDQueue->dWord0) & kUniqueNumNoDirMask) == unique) {
			*pEDBack = pEDQueueBack;
			return (pEDQueue);
		} else {
			pEDQueueBack = pEDQueue;
			pEDQueue = (OHCIEndpointDescriptorPtr) pEDQueue->pVirtualNext;
		}
	}
	if (pOHCIUIMData->OptiOn) {
		pEDQueue = FindBulkEndpoint (functionNumber, endpointNumber, kOHCIEDDirectionTD, &pEDQueueBack);
		*pEDBack = pEDQueueBack;
		return (pEDQueue);
	}
	return (nil);
}


OHCIEndpointDescriptorPtr FindBulkEndpoint (
	short 						functionNumber, 
	short						endpointNumber,
	short						direction,
	OHCIEndpointDescriptorPtr   *pEDBack)
{

	UInt32						unique;
	UInt32						myEndPointDirection;
	OHCIEndpointDescriptorPtr	pEDQueue;
	OHCIEndpointDescriptorPtr	pEDQueueBack;

	
	//search for endpoint descriptor
	myEndPointDirection = ((UInt32) direction) << kOHCIEndpointDirectionOffset;
	unique = (UInt32) ((((UInt32) endpointNumber) << kOHCIEndpointNumberOffset) | ((UInt32) functionNumber) 
											| myEndPointDirection);
	pEDQueueBack = (OHCIEndpointDescriptorPtr) pOHCIUIMData->pBulkHead;
	pEDQueue = (OHCIEndpointDescriptorPtr) pEDQueueBack->pVirtualNext;
	while (((UInt32) pEDQueue) != pOHCIUIMData->pBulkTail ) {
		if ( (EndianSwap32Bit(pEDQueue->dWord0) & kUniqueNumMask) == unique) {
			*pEDBack = pEDQueueBack;
			return (pEDQueue);
		} else {
			pEDQueueBack = pEDQueue;
			pEDQueue = (OHCIEndpointDescriptorPtr) pEDQueue->pVirtualNext;
			
		}
	}
	return (nil);

}


OHCIEndpointDescriptorPtr FindEndpoint (
	short 						functionNumber, 
	short 						endpointNumber,
	short 						direction, 
	OHCIEndpointDescriptorPtr 	*pEDQueueBack, 
	UInt32 						*controlMask)
{
	OHCIEndpointDescriptorPtr pED, pEDBack;
	
//	DebugStr("OHCIUIM: FindEndpoint");
kprintf("FindEndPoint:Calling FindControlEndpoint fn=%d,ep=%d\n",functionNumber,endpointNumber);
	if ((pED = FindControlEndpoint (functionNumber, endpointNumber, &pEDBack)) != nil) 
	{
		*pEDQueueBack = pEDBack;
		*controlMask = kOHCIHcControl_CLE;
		return (pED);
	}
	if ((pED = FindBulkEndpoint (functionNumber, endpointNumber, direction, &pEDBack)) != nil)
	{
		*pEDQueueBack = pEDBack;

		*controlMask = kOHCIHcControl_BLE;
		//zzzz Opti Bug
		if(pOHCIUIMData->OptiOn)
			*controlMask = kOHCIHcControl_CLE;
		return (pED);
	}
	
	if ((pED = FindInterruptEndpoint (functionNumber, endpointNumber, &pEDBack)) != nil)
	{
		*pEDQueueBack = pEDBack;
		*controlMask = 0;
		return (pED);	
	}
	
	pED = FindIsochronousEndpoint(functionNumber, endpointNumber, direction, &pEDBack);
	*pEDQueueBack = pEDBack;
	*controlMask = 0;
	return (pED);	
}


OHCIEndpointDescriptorPtr FindIsochronousEndpoint(
	short 						functionNumber,
	short						endpointNumber,
	short 						direction, 
	OHCIEndpointDescriptorPtr	*pEDBack)
{
	UInt32						myEndPointDirection;
	UInt32						unique;
	OHCIEndpointDescriptorPtr	pEDQueue, pEDQueueBack;

	//search for endpoint descriptor
	myEndPointDirection = ((UInt32) direction) << kOHCIEndpointDirectionOffset;
	unique = (UInt32) ((((UInt32) endpointNumber) << kOHCIEndpointNumberOffset) | ((UInt32) functionNumber) 
											| myEndPointDirection);

	pEDQueueBack = (OHCIEndpointDescriptorPtr) pOHCIUIMData->pIsochHead;
	pEDQueue = (OHCIEndpointDescriptorPtr) pEDQueueBack->pVirtualNext;
	while (((UInt32) pEDQueue) != pOHCIUIMData->pIsochTail ) {
		if ( (EndianSwap32Bit(pEDQueue->dWord0) & kUniqueNumMask) == unique) {
			*pEDBack = pEDQueueBack;
			return (pEDQueue);
		} else {
			pEDQueueBack = pEDQueue;
			pEDQueue = (OHCIEndpointDescriptorPtr) pEDQueue->pVirtualNext;
			
		}
	}
	return (nil);

}


OHCIEndpointDescriptorPtr FindInterruptEndpoint(
	short 						functionNumber,
	short						endpointNumber,
	OHCIEndpointDescriptorPtr	*pEDBack)
{
	OHCIRegistersPtr			pOHCIRegisters;
	UInt32						unique;
	OHCIEndpointDescriptorPtr	pEDQueue;
	OHCIIntHeadPtr				pInterruptHead;
	int							i;
	UInt32						temp;
	

	
	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;
	pInterruptHead = pOHCIUIMData->pInterruptHead;

	//search for endpoint descriptor
	unique = (UInt32) ((((UInt32) endpointNumber) << kOHCIEDControl_ENPhase) | (((UInt32) functionNumber) << kOHCIEDControl_FAPhase));
//					sprintf (debugStr, "FindInterruptEndpoint unique %d", unique);
//				DebugStr ((ConstStr255Param) c2pstr (debugStr));

	
	for (i = 0; i < 63; i++) 
	{
		pEDQueue = pInterruptHead[i].pHead;
 		*pEDBack = pEDQueue;
		/* BT do this first, or you find the dummy endpoint all this is hanging off. It matches 0,0 */
		pEDQueue = (OHCIEndpointDescriptorPtr) pEDQueue->pVirtualNext;
		while (pEDQueue != pInterruptHead[i].pTail)
		{
			temp = (EndianSwap32Bit (pEDQueue->dWord0)) & kUniqueNumNoDirMask;

			if ( temp == unique) 
			{
				return(pEDQueue);
			} 
			*pEDBack = pEDQueue;
			pEDQueue = (OHCIEndpointDescriptorPtr) pEDQueue->pVirtualNext;
		}
	}
	return(nil);

}

int GetEDType(
	OHCIEndpointDescriptorPtr pED)
{
	return ((EndianSwap32Bit(pED->dWord0) & kOHCIEDControl_F)  >> kOHCIEDControl_FPhase);
}


static OSStatus RemoveAllTDs (OHCIEndpointDescriptorPtr pED)
{
	RemoveTDs(pED);
	
	if (GetEDType(pED) == kOHCIEDFormatGeneralTD) {
		// remove the last "dummy" TD
		OHCIUIMDeallocateTD((OHCIGeneralTransferDescriptorPtr) pED->pVirtualTailP);
	} else {
		OHCIUIMDeallocateITD((OHCIIsochTransferDescriptorPtr) pED->pVirtualTailP);
	}
	pED->pVirtualHeadP = nil;

	return (0);
}


//removes all but the last of the TDs
static OSStatus RemoveTDs (OHCIEndpointDescriptorPtr pED)
{
	OHCIGeneralTransferDescriptorPtr		pCurrentTD, lastTD;
	UInt32									bufferSizeRemaining = 0;
	OHCIIsochTransferDescriptorPtr			pITD, pITDLast;
	
	if (GetEDType(pED) == kOHCIEDFormatGeneralTD) {
		//process and deallocate GTD's
		pCurrentTD = (OHCIGeneralTransferDescriptorPtr) (EndianSwap32Bit(pED->dWord2) & kOHCIHeadPMask);
		pCurrentTD = (OHCIGeneralTransferDescriptorPtr) OHCIUIMGetLogicalAddress ((UInt32) pCurrentTD);
	
		lastTD = (OHCIGeneralTransferDescriptorPtr) pED->pVirtualTailP;
		pED->pVirtualHeadP = pED->pVirtualTailP;
	
		while (pCurrentTD != lastTD) 
		{
			if (pCurrentTD == nil)
				return (-1);
				
			//take out TD from list
			pED->dWord2 = pCurrentTD->dWord2;
			pED->pVirtualHeadP = pCurrentTD->pVirtualNext;	

			if (pCurrentTD->preparationID)
				CheckpointIO(pCurrentTD->preparationID, nil);
			pCurrentTD->preparationID = nil;
			
			bufferSizeRemaining += findBufferRemaining(pCurrentTD);

			if (pCurrentTD->CallBack){
				CallBackFuncPtr pCallBack;
				// zero out callback first than call it
				pCallBack = pCurrentTD->CallBack;
				pCurrentTD->CallBack = nil;
				(*pCallBack) (pCurrentTD->refcon, EDDeleteErr, bufferSizeRemaining);
				bufferSizeRemaining = 0;
			}
			
			OHCIUIMDeallocateTD(pCurrentTD);
			pCurrentTD = (OHCIGeneralTransferDescriptorPtr) pED->pVirtualHeadP;		
		}		
	} else {
		pITD = (OHCIIsochTransferDescriptorPtr) (EndianSwap32Bit(pED->dWord2) & kOHCIHeadPMask);
		pITD = (OHCIIsochTransferDescriptorPtr) OHCIUIMGetLogicalAddress ((UInt32) pITD);
		pITDLast = (OHCIIsochTransferDescriptorPtr) pED->pVirtualTailP;
		
		while (pITD != pITDLast) {
			if (pITD == nil)
				return (-1);
			ProcessCompletedITD (pITD);
			pITD = (OHCIIsochTransferDescriptorPtr) pITD->pVirtualNext;
		}
	}

	return (0);
}

void ProcessCompletedITD (OHCIIsochTransferDescriptorPtr pITD) {

	USBIsocFrame 			*pFrames;
	int						i;
	
	SwapIsoc (pITD);
	pFrames = (USBIsocFrame *) pITD->pIsocFrame;

	for (i=0; i <= ((pITD->dWord0 & kOHCIITDControl_FC) >> kOHCIITDControl_FCPhase); i++) {
	 	if ( ((pITD->offset[i] & kOHCIITDPSW_CCNA) >> kOHCIITDPSW_CCNAPhase) == kOHCIITDConditionNotAccessed) {
			pFrames[pITD->frameNum + i].frActCount = 0;
			pFrames[pITD->frameNum + i].frStatus = kOHCIITDConditionNotAccessedReturn;
		} else {
			pFrames[pITD->frameNum + i].frStatus = 
									(pITD->offset[i] & kOHCIITDPSW_CC) >> kOHCIITDOffset_CCPhase;
			pFrames[pITD->frameNum + i].frActCount = pITD->offset[i] & kOHCIITDPSW_Size;
		}
	}
	// call callback
	if (pITD->handler){
		IsocCallBackFuncPtr pHandler;
		pHandler = pITD->handler;
		pITD->handler = nil;
		//zero out handler first than call it
		(*pHandler) (pITD->refcon, noErr, pFrames);
	}
}

/*
  This table contains the list of errata that are necessary for known problems with particular silicon
  The format is vendorID, revisionID, lowest revisionID needing errata, highest rev needing errata, errataBits
  The result of all matches is ORed together, so more than one entry may match.  Typically for a given errata a
  list of chips revisions that this applies to is supplied.
*/
static ErrataListEntry	errataList[] = {
	{0x1095, 0x670, 0, 0xffff, kErrataCMDDisableTestMode | kErrataOnlySinglePageTransfers | kErrataRetryBufferUnderruns}, // CMD 670
	{0x1045, 0xc861, 0, 0xffff, kErrataLSHSOpti}, // Opti 1045
};

#define errataListLength (sizeof(errataList)/sizeof(ErrataListEntry))

static UInt32 GetErrataBits (RegEntryIDPtr regEntryIDPtr)
{
	UInt32				vendID, deviceID, revisionID;
	UInt32				vLen = sizeof(vendID), dLen = sizeof(deviceID), rLen = sizeof(revisionID);
	ErrataListEntry		*entryPtr;
	UInt32				i, errata = 0;
	
	extern int  cmd_pci_errata;  //global needed to fix errata for Caps Lock 
								//LED support.  In usbcmd.m.

	if (cmd_pci_errata == 1)
	{
		return 0;  //This allows PowerSurge with CMD card to work with Caps Lock LED
	}

	// get this chips vendID, deviceID, revisionID
	RegistryPropertyGet(regEntryIDPtr, "vendor-id", &vendID, &vLen);
	RegistryPropertyGet(regEntryIDPtr, "device-id", &deviceID, &dLen);
	RegistryPropertyGet(regEntryIDPtr, "revision-id", &revisionID, &rLen);
	for(i=0, entryPtr = errataList; i<errataListLength; i++, entryPtr++){
		if (vendID == entryPtr->vendID && deviceID == entryPtr->deviceID &&
			revisionID >= entryPtr->revisionLo && revisionID <= entryPtr->revisionHi){
				errata |= entryPtr->errata;  // we match, add this errata to our list
		}
	}
	return(errata);
}


void DoOptiFix(OHCIEndpointDescriptorPtr pIsochHead)
{
	OHCIEndpointDescriptorPtr pED;
	OHCIGeneralTransferDescriptorPtr pTD;
	int i = 0;
	int j = 0;
	
//	DebugStr ((ConstStr255Param) "DoOptiFix");
	
	for (i=0; i<20; i++) {
		// allocate ED
		pED = OHCIUIMAllocateED();
		pED->pVirtualNext = nil;
		// make ED and FA = 0
		pTD = OHCIUIMAllocateTD();
		pED->dWord2 = EndianSwap32Bit ((UInt32) pTD->pPhysical);
		pTD->dWord2 = pED->dWord1;
		pTD->pEndpoint = (UInt32) pED;
		pTD->pType = kOHCIOptiLSBug;
		pED->dWord1 = EndianSwap32Bit ((UInt32) pTD->pPhysical);

/*
		for (j=0; j<0; j++) {
			// allocate 4 TDs
			pTD = OHCIUIMAllocateTD();
			pTD->dWord2 = pED->dWord2;
			pED->dWord2 = EndianSwap32Bit ((UInt32) pTD->pPhysical);
			// leave 1 all zeros
			// make other all zeros except next TD
			// mark each to indicate that its this type
			pTD->pType = kOHCIOptiLSBug;
		}
*/
		pED->dWord3 = pIsochHead->dWord3;
		pIsochHead->dWord3 = EndianSwap32Bit((UInt32) pED->pPhysical);
		pIsochHead = pED;
	}
}



OSStatus OptiLSHSFix()
{

//  Do Opti Errata stuff here!!!!!!
	int i;
	OHCIIntHeadPtr				pInterruptHead;
	OHCIEndpointDescriptorPtr   pControlED;
	OHCIRegistersPtr			pOHCIRegisters;
	OSStatus					status = noErr;

	pOHCIRegisters = pOHCIUIMData->pOHCIRegisters;

	pOHCIUIMData->OptiOn = 1;

	//Turn off list processing
	pOHCIRegisters->hcControl =
		EndianSwapImm32Bit
			(kOHCIFunctionalState_Operational << kOHCIHcControl_HCFSPhase);
	
	// wait a millisecond
	DelayForHardware(DurationToAbsolute(1*durationMillisecond));	
		
	pInterruptHead = pOHCIUIMData->pInterruptHead;
		
	// add dummy EDs to 8ms interrupts
	for ( i = 0; i< 8; i++)
		DoOptiFix(pInterruptHead[48 + i].pHead);
//	DoOptiFix((OHCIEndpointDescriptorPtr) pOHCIUIMData->pIsochHead);

	//Assign Tail of Control to point to head of Bulk

	pControlED = (OHCIEndpointDescriptorPtr) pOHCIUIMData->pControlTail;
	pControlED->dWord3 = pOHCIRegisters->hcBulkHeadED;
	
	// add dummy EDs to end of Control
	DoOptiFix( (OHCIEndpointDescriptorPtr) pOHCIUIMData->pControlTail);
		
	// turn on only control and periodic
	pOHCIRegisters->hcControl =
		EndianSwapImm32Bit
			((kOHCIFunctionalState_Operational << kOHCIHcControl_HCFSPhase) |
			 kOHCIHcControl_CLE | kOHCIHcControl_PLE | kOHCIHcControl_IE);

	//End of Opti Fix
	return (status);
}






/*******************************************************************
Function:		AmIThisMachine()
Purpose:		check to see if we're the machine that's name is passed in
Params:			Name of computer to check for
Returns:		Returns back true if this machine's name is that which is 
passed in.
*******************************************************************/
Boolean AmIThisMachine(Str255 inMachineNameStr)
{
	RegEntryID				foundEntry;
	RegPropertyName*		propertyName = "model";
	RegPropertyName*		realPropertyName = "compatible";
	Str31					kiMacMachineName = "iMac,1";
	RegPropertyValueSize	propertySize;
	Ptr						modelNamePropertyStrPtr;
	OSStatus				result;
	long					nameRegistryVersion;
	OSErr					theErr;
	Boolean					isThatMachine = false;
	
	
	
	// Does the name registry exist?
	//naga theErr = Gestalt(gestaltNameRegistryVersion, &nameRegistryVersion);
	if( theErr == noErr )
	{
		// Can we find the "root" node of the device-tree?
		theErr = RegistryCStrEntryLookup(nil, "Devices:device-tree", 
&foundEntry);
	}
	
	if( theErr == noErr )
	{
		if( CompareString(inMachineNameStr, kiMacMachineName, NULL) == 0 )
		{
			// Does the "model" property exist?  And if so, how big is it?
			result = RegistryPropertyGetSize(&foundEntry, propertyName, &propertySize);

			if( result == noErr )
			{
				modelNamePropertyStrPtr = usb_NewPtr(propertySize);
				if( modelNamePropertyStrPtr )
				{
					result = RegistryPropertyGet(&foundEntry, propertyName, 
modelNamePropertyStrPtr, &propertySize);
						
					if( result == noErr )
					{
						c2pstr( modelNamePropertyStrPtr );
						if( CompareString( inMachineNameStr, (unsigned char 
*)modelNamePropertyStrPtr, NULL ) == 0 )
							isThatMachine = true;
					}
					usb_DisposePtr( modelNamePropertyStrPtr );
				}
			}
		}
		else
		{
	
			// Does the "compatible" property exist?  And if so, how big is it?
			result = RegistryPropertyGetSize(&foundEntry, realPropertyName, &propertySize);

			if( result == noErr )
			{
				modelNamePropertyStrPtr = usb_NewPtr(propertySize);
				if( modelNamePropertyStrPtr )
				{
					result = RegistryPropertyGet(&foundEntry, realPropertyName, modelNamePropertyStrPtr, &propertySize);
						
					if( result == noErr )
					{
						c2pstr( modelNamePropertyStrPtr );
						if( CompareString( inMachineNameStr, (unsigned char *)modelNamePropertyStrPtr, NULL ) == 0 )
							isThatMachine = true;
						else
						{
							UInt16 modelNameLength = modelNamePropertyStrPtr[0];
							c2pstr( &(modelNamePropertyStrPtr[++modelNameLength]) );
							if( modelNamePropertyStrPtr[modelNameLength] != 0 )
								if( CompareString( 	inMachineNameStr, (unsigned char *)&(modelNamePropertyStrPtr[ modelNameLength ]), NULL ) == 0 )
									isThatMachine = true;
						}
					}
					usb_DisposePtr( modelNamePropertyStrPtr );
				}
			}
		}
		

		if ((result = RegistryEntryIDDispose(&foundEntry)) != noErr)
			return(false);
	}
	
	return(isThatMachine);
}
