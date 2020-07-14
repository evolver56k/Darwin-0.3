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
 * kmDevice.m - kernel keyboard/monitor module, ObjC implementation.
 *
 * HISTORY
 * 21 Sep 92	Joe Pasqua
 *	Created i386 version based on m88k version.
 */
 
#import <driverkit/generalFuncs.h>
#import <machkit/NXLock.h>
#import <driverkit/ppc/driverTypesPrivate.h>
#import <driverkit/kernelDriver.h>
#import <bsd/sys/types.h>
#import <bsd/sys/tty.h>
#import <bsd/dev/ppc/cons.h>
#import <bsd/sys/conf.h>
#import <bsd/sys/fcntl.h>
#import <bsd/sys/errno.h>
#import <bsd/sys/msgbuf.h>
#import <bsd/dev/ppc/FBConsole.h>
#import <bsd/dev/ppc/SerialConsole.h>
#import <bsd/dev/ppc/kmDevice.h>
#import <bsd/dev/kmreg_com.h>
#import <bsd/sys/reboot.h>
#import <kernserv/prototypes.h>
#import <driverkit/IODisplay.h>
#import <driverkit/IOFrameBufferDisplay.h>
#import <driverkit/IODisplayPrivate.h>
#import <driverkit/IOConfigTable.h>
#import <bsd/dev/ppc/kmWaitCursor.h>
#import <bsd/sys/proc.h>
#import <bsd/sys/systm.h>

#import <kern/assert.h>
#import <machdep/ppc/boot.h>

#import "PCKeyboardDefs.h"
#import "PPCKeyboardPriv.h"
#import "ConsoleSupport.h"
#import "km.h"

IODisplayInfo	bootDisplayInfo;

static IOConsoleInfo *serialConsole;
static int bootGraphicsMode = NO;
static short prettyShutdown = 0;

// Title strings for the various windows
const char *mach_title = "Darwin Operating System"; // Used by km.m also
static const char *alert_title  = "Alert";


#if	KERNOBJC
// Module Globals
static int kmUnit;	// Incremented to keep track of what unit we're on
static id display;
	// display == nil means that we do not have a display object around
	// yet. In this case we use the basicConsole. When display != nil
	// we can use it's console support mechanism to do our displaying.

// The following function pointers are used by DoAlert and DoRestore. They
// get called at interrupt level and can't execute ObjC code. To avoid doing
// so we bind the lock and unlock methods that are used and call them thru
// these pointers.
static id (*lockProc)(), (*unlockProc)();

#define	AllocConsole()		\
		AllocDisplayConsole(display) 

LANG_TYPE glLanguage;

#else

#define	AllocConsole()		basicConsole

#endif KERNOBJC


static IOConsoleInfo * 
AllocDisplayConsole( id display )
{
IOConsoleInfo * consoleInfo = NULL;

    if( display)
	consoleInfo = [display allocateConsoleInfo];
    if( NULL == consoleInfo)
	consoleInfo = basicConsole;
    return( consoleInfo);
}


// Used for putting up alert panels very early, before kmId is valid.
IOConsoleInfo *kmAlertConsole;

/*
 * Calculate incremented index for inBuf.
 */
static inline int
inbuf_incr(int x)
{
	if(++x == INBUF_SIZE) {
		return 0;
	}
	else {
		return x;
	}
}

/*
 * Initialize the console enough for printf's to work via kmputc. No 
 * text window will be created if booting in icon mode. Keyboard is 
 * not enabled. 
 */

void
initialize_screen(void * args)
{
    extern int initialized;
    Boot_Video * framebufferArgs = (Boot_Video *) args;

    cons.t_dev = makedev(12, 0); /* point the cons device to cdevsw's km dev */

    serialConsole = SerialAllocateConsole();

    if( framebufferArgs->v_baseAddr) {
       /*
	* Initialize the OpenFirmware based framebuffer console.
	*/
	static char * ofPixelEncoding = "PPPPPPPP";

	bzero(&bootDisplayInfo, sizeof(bootDisplayInfo));
	bootDisplayInfo.width			= framebufferArgs->v_width;
	bootDisplayInfo.height			= framebufferArgs->v_height;
	bootDisplayInfo.totalWidth		= framebufferArgs->v_rowBytes;
	bootDisplayInfo.rowBytes		= framebufferArgs->v_rowBytes;
	bootDisplayInfo.refreshRate		= 75;		// very approx
	bootDisplayInfo.frameBuffer		= (void *)framebufferArgs->v_baseAddr;
	bootDisplayInfo.bitsPerPixel		= IO_8BitsPerPixel;
	bootDisplayInfo.colorSpace		= IO_RGBColorSpace;
	strcpy( bootDisplayInfo.pixelEncoding, ofPixelEncoding);
	basicConsole = BasicAllocateConsole(&bootDisplayInfo);
    }
    if( basicConsole == NULL)
	basicConsole = serialConsole;

    if( framebufferArgs->v_display & 1)
	bootGraphicsMode = YES;

    basicConsoleMode = bootGraphicsMode ? SCM_GRAPHIC : SCM_TEXT;
    (*basicConsole->Init)(
	basicConsole, basicConsoleMode, (bootGraphicsMode == NO), (bootGraphicsMode == NO), mach_title);

    initialized = TRUE;
}

#if	KERNOBJC

@implementation kmDevice

- (void)registerDisplay:newDisplay
{
    IOConsoleInfo *	newConsole;

    if (display == nil) {
	display = newDisplay;

	if( NO == probed) {
	    newConsole = AllocDisplayConsole( newDisplay);
	    if( newConsole) {
    		(*newConsole->Init)(
		    newConsole, fbMode, YES, fbMode == SCM_TEXT, mach_title);
                fbp[FB_DEV_NORMAL] = newConsole;
		if( fbMode == SCM_TEXT)
		    kmdumplog();
		else {
                    kmDrawString( newConsole, "Starting Darwin OS");
                    [self animationCtl: KM_ANIM_RESUME];
		}
	    }
            newConsole = AllocDisplayConsole( newDisplay);
	    if( newConsole)
                fbp[FB_DEV_ALERT] = newConsole;

	} else {
	    // somebody like the window server has changed modes,
	    // but we still own the display!
	    fbMode = SCM_OTHER;
            [self animationCtl: KM_ANIM_STOP];

            fbp[FB_DEV_NORMAL] = AllocDisplayConsole( newDisplay);
            fbp[FB_DEV_ALERT] = AllocDisplayConsole( newDisplay);
	}
    }
}

- (void)unregisterDisplay:oldDisplay
{
    if (display == oldDisplay) {
        [self animationCtl: KM_ANIM_SUSPEND];
	basicConsole = serialConsole;
        display = nil;

        if( fbp[FB_DEV_NORMAL])
            (*fbp[FB_DEV_NORMAL]->Free) (fbp[FB_DEV_NORMAL]);
        if( fbp[FB_DEV_ALERT])
            (*fbp[FB_DEV_ALERT]->Free) (fbp[FB_DEV_ALERT]);

        fbp[FB_DEV_NORMAL] = serialConsole;
        fbp[FB_DEV_ALERT] = serialConsole;
    }
}


/* Probe routine for a pseudo-device driver. */
+ (BOOL)probe : deviceDescription
{
    // Proceed with keyboard initialization.

    if ([kmId init:YES fb_mode: 
	    bootGraphicsMode ? SCM_GRAPHIC : SCM_TEXT] == nil)
    {
	IOLog("kmDevice: continuing with bad kmDevice instance\n");
	[kmId free];
	kmId = nil;
	return NO;
    } else
	kmId->probed = YES;

    return YES;
}

+ (IODeviceStyle)deviceStyle
{
    return IO_PseudoDevice;
}

/* 
 * Can't put this in the kmDevice object because we might need it
 * before kmId is initialized.
 */
static int kmDeviceAnimationState = -1;
static simple_lock_data_t kmDeviceAnimationLock;

/*
 * Create a kmDevice. This should only be called once in the lifetime of 
 * the system. It can be called from kmDevice's probe:, or from km.m's
 * alert() (if alert is called before IOTask autoconf).
 */
+ new
{
	char 		name[20];
	kmDevice	*km_id;
	int i;
	
        simple_lock_init(&kmDeviceAnimationLock);

	km_id = [self alloc];
	km_id->kmOpenLock = [NXLock new];
	(IMP)lockProc = [km_id->kmOpenLock methodFor:@selector(lock)];
	(IMP)unlockProc = [km_id->kmOpenLock methodFor:@selector(unlock)];
	km_id->kbId = nil;
	
	for(i=0; i<NUM_FB_DEVS; i++) {
		km_id->fbp[i] = NULL;
	}
	
	/*
	 * Global kmId is id of first kmDevice created.
	 */
	if(kmUnit == 0) {
		ASSERT(kmId == nil);
		kmId = km_id;
	}
	
	/*
	 * Special case for this one, it's hard coded - there is always 
	 * at least one of these, and it's created before autoconf.
	 */
        km_id->fbp[FB_DEV_NORMAL] = basicConsole;
        km_id->fbp[FB_DEV_ALERT] = basicConsole;
	[kmId setUnit:kmUnit];
	sprintf(name, "kmDevice%d", kmUnit++);	
	[kmId setName:name];
	[kmId setDeviceKind:"kmDevice"];
	[kmId setLocation:NULL];
	return km_id;
}

/*
 * Reusable initialization method. kmOpenLock must already have been 
 * initialized. initKb enables initialiation of keyboard.
 */
- init : (BOOL)initKb
	 fb_mode : (ScreenMode)fb_mode
{
	id		rtn = self;
	
	[kmOpenLock lock];
	
	/*
	 * Init local instance variables.
	 */
	inDex = outDex 	= 0;
	blockIn 	= 0;
	alertRefCount 	= 0;
	fbMode          = fb_mode;
	simple_lock_init(&inBufLock);
	
	if(initKb) {
		/*
		 * connect to Keyboard.
		 */
		if([self initKb] == nil) {
			IOLog("kmDevice: No Keyboard Found\n");
			/* but continue anyway... */
		}
	}
	[super init];
	if( (!hasRegistered) && initKb) {
		[self registerDevice];
		hasRegistered = YES;
	}

	[kmOpenLock unlock];
	return rtn;
}

- initKb
// Description:	Initialize connection with PCKeyboard object
{
	IOReturn drtn;
	
	drtn = IOGetObjectForDeviceName("PPCKeyboard0", &kbId);
	if(drtn) {
		IOLog("km init: Can't find PPCKeyboard0 (%s)\n",
			[self stringFromReturn:drtn]);
		return nil;
	}
	
	drtn = [kbId becomeOwner:self];
	if(drtn) {
		IOLog("km init: becomeOwner failed (%s)\n",
			[self stringFromReturn:drtn]);
		return nil;
	}
	
	/*
	 * Also register to be notified if anyone else relinquishes
	 * ownership after we give it up. This allows us to take over
	 * the frame buffer and keyboard when the window server exits.
	 */
	drtn = [kbId desireOwnership:self];
	if(drtn) {
		IOLog("km init: desireOwnership failed (%s)\n",
			[self stringFromReturn:drtn]);
		return nil;
	}
	return self;
}

/*
 * Interface to bsd code.
 */
- (int)kmOpen : (int)flag
{
	int rtn = 0;

	[kmOpenLock lock];
	if(flag & ( O_POPUP | O_ALERT)) {
		switch(fbMode) {
		    case SCM_TEXT:
		    	/*
			 * Easy case, we already have a place to do putc()'s. 
			 * Note for SCM_TEXT case, alert text just goes to 
			 * the normal console.
			 */
			break;
		    case SCM_ALERT:
			alertRefCount++;
			break;

		    default:
			/*
			 * Create an alert window.
			 *
			 * We only want to allow this for
			 * programs during system startup (eg.
			 * they must be run as root, and PostScript
			 * must not be running.
			 */
			if(suser(current_proc()->p_ucred,
						&current_proc()->p_acflag)) {
				rtn = EACCES;
				goto out;
			}
			if(fbMode == SCM_OTHER) {
				rtn = EBUSY;
				goto out;
			}

			/*
			 * Dropping to single user mode => O_POPUP,
			 * so make the console text mode.
			 */
			if( flag & O_POPUP) {
			    fbMode = SCM_TEXT;
			    (*fbp[FB_DEV_NORMAL]->Init)(
				    fbp[FB_DEV_NORMAL], SCM_TEXT, TRUE, TRUE, mach_title );

			} else {
			   /*
			    * Jump into "alert mode". Note that alertRefCount
			    * doesn't get decremented in close; the user has
			    * to do an explicit KMIOCRESTORE.
			    */
			    alertRefCount++;
			    savedFbMode = fbMode;
			    fbMode = SCM_ALERT;
			    (*fbp[FB_DEV_ALERT]->Init)(
				    fbp[FB_DEV_ALERT], fbMode, TRUE, TRUE, alert_title );
			}
		}
	}
	
	/*
	 * km.m takes care of Unix-y tty stuff.
	 */
out:
	[kmOpenLock unlock];
	return rtn;
}

- (int)kmPutc : (int)c
{
	switch(fbMode) {
	    case SCM_TEXT:
		(*fbp[FB_DEV_NORMAL]->PutC)(fbp[FB_DEV_NORMAL], c);
		return 0;
		
	    case SCM_ALERT:
	    	ASSERT(fbp[FB_DEV_ALERT] != NULL);
		(*fbp[FB_DEV_ALERT]->PutC)(fbp[FB_DEV_ALERT], c);
		return 0;

	    default:
	    	/*
		 * No place to put this text. This is not an error.
		 */
		return 0;
	}
}

/*
 * Blocking read routine. This assumes that interrupts and threads are 
 * working, since keyboard driver will need that functionality...OK?
 */

- (int)kmGetc
{
	int c;
	
	simple_lock(&inBufLock);
	blockIn = 1;
	while(inDex == outDex) {
		/*
		 * Just wait for something to show up...
		 */
		thread_sleep((void *)inBuf, &inBufLock, TRUE);
		simple_lock(&inBufLock);
	} 
	c = inBuf[outDex];
	outDex = inbuf_incr(outDex);
	blockIn = 0;
	simple_unlock(&inBufLock);
	return c;
}

#endif	//KERNOBJC

/*
 * ioctl equivalents. All return an errno.
 */
 
/*
 * Client finished with alert panel. Called from ioctl and from inside the 
 * kernel via alert_done().
 */
int DoRestore(void)
{
	int rtn = 0;

#if	KERNOBJC
	
	if (kmAlertConsole) {	// No kmDevice yet!
	    (*kmAlertConsole->Restore)(kmAlertConsole);
	    (*kmAlertConsole->Free)(kmAlertConsole);
	    kmAlertConsole = NULL;
	    return 0;
	}
	
	if (kmId == nil)
	    return 0;


	/*
	 * Have to protect with kmOpenLock since we may alter frame buffer
	 * context.
	 */
	(*lockProc)(kmId->kmOpenLock, @selector(lock));
	if(kmId->fbMode == SCM_ALERT) {
		if(kmId->alertRefCount == 0) {
			/*
			 * No can do. I'm not sure when this could 
			 * ever happen...
			 */
			rtn = EBUSY;
			goto out;
		}
		if(--kmId->alertRefCount != 0) {
		
			/*
			 * Not out of the woods yet, other user(s) of alert
			 * panel still active. 
	    		 */
			goto out;
		}
		
		/*
		 * Last user of alert/popup window.
		 */
		kmId->fbMode = kmId->savedFbMode;
		if(kmId->fbMode != SCM_ALERT) {
			rtn = (*kmId->fbp[FB_DEV_ALERT]->Restore)(
			    kmId->fbp[FB_DEV_ALERT]);
		}
		else {
			IOLog("kmDevice: Recursive SCM_ALERT in restore!\n");
		}
		goto out;
	} else {
		/*
		 * Not in alert mode, this is a nop (and it's also somewhat
		 * unexpected...).
		 */
		rtn = EINVAL;
	}
out:
	(*unlockProc)(kmId->kmOpenLock, @selector(unlock));

#endif	// KERNOBJC

	return rtn;
}

#if	KERNOBJC

- (int)restore
{
    return DoRestore();
}

- (int)drawRect : (const struct km_drawrect *)kmRect
{
	struct km_drawrect rect = *kmRect;
	int size, ret;
	
	if(fbMode != SCM_GRAPHIC) {
	    	/*
		 * No can do.
		 */
		return EBUSY;
	}
	/*
	 * Copy in user data so the console DrawRect routine
	 * doesn't have to.
	 */

        if( (rect.x & 3) == 3) {
            size = rect.width * rect.height;		// size in bytes, 8bit src

	} else {
	rect.width = (rect.width + 3) & ~3;	// Round up to 4 pixel boundary
	size = (rect.width >> 2) * rect.height;	// size in bytes
	}
	rect.data.bits = (unsigned char *)kalloc( size );
	if (copyin(kmRect->data.bits, rect.data.bits, size)) {
	    ret = -1;
	    goto out;
	}

	ret = (*fbp[FB_DEV_NORMAL]->DrawRect)(fbp[FB_DEV_NORMAL], &rect);

out:
	kfree( rect.data.bits, size );
	return ret;
}

- (int)eraseRect : (const struct km_drawrect *)kmRect
{
        if(fbMode != SCM_GRAPHIC) {
	    	/*
		 * No can do.
		 */
		return EBUSY;
	}
	return (*fbp[FB_DEV_NORMAL]->EraseRect)(fbp[FB_DEV_NORMAL], kmRect);
}

- (int)disableCons
{
	/*
	 * This means that tty code has detected a "console 
	 * now invalid" situation.
	 */
#if 0
	if(fbMode == SCM_TEXT) {
		fbMode = SCM_OTHER;
	}
#endif
	return 0;
}

#endif	// KERNOBJC

static void
kmDeviceDrawCursor()
{
    IOConsoleInfo *device;
    
    simple_lock(&kmDeviceAnimationLock);

    if (kmDeviceAnimationState > 0) {
	device = kmId->fbp[FB_DEV_NORMAL];
	if (device && (kmId->fbMode == SCM_GRAPHIC)) {
	    (void)(*device->DrawRect)(device,
		&kmDeviceWaitCursor[kmDeviceAnimationState - 1]);
	}
	if (++kmDeviceAnimationState > KM_DEVICE_WAITCURSOR_NSTATES)
	    kmDeviceAnimationState = 1;
	ns_timeout((func)kmDeviceDrawCursor, 0, KM_DEVICE_WAITCURSOR_DELAY, 
			CALLOUT_PRI_THREAD);
    }
    simple_unlock(&kmDeviceAnimationLock);
}

/* Called once and only once from main(),
 */
void
kmEnableAnimation(void)
{
    if( kmId == nil) {
	kmId = [kmDevice new];
	[kmId init:NO fb_mode: 
		bootGraphicsMode ? SCM_GRAPHIC : SCM_TEXT];
    }
    [kmId animationCtl:KM_ANIM_RESUME];
}

void
kmDisableAnimation(void)
{
    if (kmId != nil)
	[kmId animationCtl:KM_ANIM_STOP];
}

#if	KERNOBJC

- (int)animationCtl : (km_anim_ctl_t)ctl
{
	int doAnimation = 0;
	int ret = 0;
	
	if (fbMode != SCM_GRAPHIC)
	    return 0;
	    
	simple_lock(&kmDeviceAnimationLock);
	switch(ctl) {
	    case KM_ANIM_STOP:
	    case KM_ANIM_SUSPEND:
		if (kmDeviceAnimationState > 0) {
		    kmDeviceAnimationState = -kmDeviceAnimationState;
		    (void)(*fbp[FB_DEV_NORMAL]->DrawRect)
			(fbp[FB_DEV_NORMAL], &kmDeviceBlankCursor);
		}
                ns_untimeout((func)kmDeviceDrawCursor, 0);
                if( ctl == KM_ANIM_STOP)
                    kmDeviceAnimationState = 0;
		break;
	    case KM_ANIM_RESUME:
		if (kmDeviceAnimationState < 0) {
		    kmDeviceAnimationState = -kmDeviceAnimationState;
		    doAnimation = 1;
		}
		break;
	    default:
		ret = EINVAL;
		break;
	}
	simple_unlock(&kmDeviceAnimationLock);
	if (doAnimation)
	    kmDeviceDrawCursor();
	return ret;
}

- (int)getStatus : (unsigned *)statusp
{
	unsigned status = KMS_SEE_MSGS;
	
	if(fbMode == SCM_GRAPHIC) {
	    status = 0;
	}
	*statusp = status;
	return 0;
}

- (int)getScreenSize : (struct ConsoleSize *)size
{
	if (fbp[FB_DEV_NORMAL] == 0)
	    return EINVAL;
	
	(*fbp[FB_DEV_NORMAL]->GetSize)(fbp[FB_DEV_NORMAL], size);
	return 0;
}

#endif	KERNOBJC

#import <bsd/dev/ppc/keycodes.h>

int ProcessKbdEvent(PCKeyboardEvent *ke)
{
    // State of the modifier keys:
    static boolean_t	control_left, control_right,
			shift_left, shift_right, caps_lock,
			alt_left, alt_right,
			cmd_left, cmd_right;
    boolean_t valid, shift, control, cmd, c;

    valid = FALSE;
    switch (ke->keyCode) {
	case ADBK_CONTROL:
	    control_left = ke->goingDown;
	    break;

	case ADBK_CONTROL_R:
	    control_right = ke->goingDown;
	    break;
	    
	case ADBK_SHIFT:
	    shift_left = ke->goingDown;
	    break;

	case ADBK_SHIFT_R:
	    shift_right = ke->goingDown;
	    break;

	case ADBK_CAPSLOCK:
	    caps_lock = ke->goingDown;
	    break;
	
	case ADBK_OPTION:
	    alt_left = ke->goingDown;
	    break;
	    
	case ADBK_OPTION_R:
	    alt_right = ke->goingDown;
	    break;

	case ADBK_FLOWER:
	    cmd_left = ke->goingDown;
	    break;

	default:
	    valid = TRUE;
    }

    if (!valid)
        return inv;

    // caps lock is just a pain at this point
    shift = shift_right || shift_left;  // || caps_lock;

    control = (control_left || control_right) ? (_N_KEYCODES * 2) : 0;
    c = keycodeToAscii[((ke->keyCode << 1) | shift) + control];

   cmd = (cmd_right || cmd_left) ? 0x80 : 0;

    switch (c) {
	case dim:
	case bright:
	case loud:
	case quiet:
	case up:
	case down:
	    break;
    
	default:
	    c |= cmd;
	    break;
    }

//printf("{raw:%lx, ascii:%lx}",ke->keyCode,c);

    if (!ke->goingDown)	// Ignore keys coming up
	return(inv);
    return(c);
}

#if	KERNOBJC

- (void)dispatchKeyboardEvent:(PCKeyboardEvent *)event;
// Description:	Called when the keyboard object receives a new keyboard
//		Event. Don't free the event object.
{
    register int c;

    if ((c = ProcessKbdEvent(event)) == inv)
	return;

    // Normal ASCII data. pass it up.
    if(blockIn) {
	    //
	    // We're in 'blocking input' mode; add this to 
	    // the input buffer if there is room.
	    //
	    simple_lock(&inBufLock);
	    if(inbuf_incr(inDex) == outDex) {
		    //
		    // Overflow. Oh well.
		    //
		    simple_unlock(&inBufLock);
		    return;
	    }
	    inBuf[inDex] = c;
	    inDex = inbuf_incr(inDex);
	    thread_wakeup((void *)inBuf);
	    simple_unlock(&inBufLock);
    }
    else {
	    //
	    // Normal console input.
	    //
	    struct tty *tp = &cons;

	    (*linesw[tp->t_line].l_rint) (c, tp);
    }
    return;
}

#else

void
cDispatchKeyboardEvent(PCKeyboardEvent *event)
{
    register int c;
    struct tty *tp = &cons;

    if ((c = ProcessKbdEvent(event)) == inv)
	return;
#if 0
    cnputc(c);
#else
    (*linesw[tp->t_line].l_rint) (c, tp);
#endif

    return;
}

#endif	KERNOBJC

#if	KERNOBJC

/*
 * ev driver (or DPS or ...) is taking over the screen and keyboard. 
 * This should always succeed.
 */
- (IOReturn)relinquishOwnershipRequest : device
{
        [self animationCtl: KM_ANIM_STOP];

	fbMode = SCM_OTHER;
	/*
	 * FIXME - notify parent of SCM_UNINIT.
	 */
	return IO_R_SUCCESS;
}

/*
 * Called when ev/DPS, which has been the owner of Keyboard, relinquishes 
 * ownership. I think this means that we unconditionally go to SCM_TEXT 
 * mode...
 */

static char *k_languages[L_NUM_LANGUAGE] = {
	"English",
	"French",
	"German",
	"Spanish",
	"Italian",
	"Swedish",
	"Japanese", 
	};


- (IOReturn)canBecomeOwner : device
{
	IOReturn drtn;
	static short checkedSystemConfig = 0;
	static short preventPrettyShutdown = 0;
	
	/*
	 * Regain control of keyboard.
	 */
	if((drtn = [kbId becomeOwner:self])) {
	    IOLog("km canBecomeOwner: becomeOwner failed (%s)\n",
		    [self stringFromReturn:drtn]);
	    /*
	     * But continue, although we have no keyboard...
	     */
	}

	if (!checkedSystemConfig) {
	    IOConfigTable	*configTable;
	    const char		*val;
	    
	    checkedSystemConfig = 1;
	    configTable = [IOConfigTable newFromSystemConfig];
	    val = [configTable valueForStringKey:"Shutdown Graphics"];
	    if (val) {
		if (!strcmp(val,"No"))
		    preventPrettyShutdown = 1;
		[IOConfigTable freeString:val];
	    }

	    val = [configTable valueForStringKey:"Language"];
	    if (val) {
		int 		i;
		
		for (i=0; i<L_NUM_LANGUAGE; i++) {
		    if (!strcmp(val, k_languages[i])) {
			glLanguage = i;
			break;
		    }
		}
		[IOConfigTable freeString:val];
	    }

	    [configTable free];
	}

	if (preventPrettyShutdown)
	    prettyShutdown = 0;
	
	if (prettyShutdown)
	    fbMode = SCM_GRAPHIC;
	else
	    fbMode = SCM_TEXT;
	    
	(*fbp[FB_DEV_NORMAL]->Init)(
	    fbp[FB_DEV_NORMAL], fbMode, TRUE, TRUE, mach_title);

        if (prettyShutdown) {
            [self drawGraphicPanel: KM_PANEL_NS];

            switch(prettyShutdown) {
            case 1:
                [self graphicPanelString: "Restarting the computer...\n"];
                break;
            case 2:
                [self graphicPanelString: "Shutting down the computer...\n"];
                break;
            default:
                [self graphicPanelString: "Please wait..."];
	    }

            kmDeviceAnimationState = -1;
            [self animationCtl: KM_ANIM_RESUME];
	}

	return IO_R_SUCCESS;
}

#endif	// KERNOBJC

/*
 * Kernel internal alert panel support.
 */

void DoSafeAlert(const char *windowTitle, const char *msg, boolean_t saveUnder )
{
#if	KERNOBJC
    if (kmId == nil)
#endif
    {
	// We haven't created a kmDevice yet! We can't put up a panel or
	// call ObjC stuff.  Create an alert console.
#if	KERNOBJC
	if (kmAlertConsole)
	    return;
#endif
	// ...unless we already have basicConsole in a text mode.
	if (basicConsoleMode == SCM_ALERT ||
	    basicConsoleMode == SCM_TEXT) {
		if (basicConsole)
		    while (*msg)
			(basicConsole->PutC)(basicConsole, *msg++);
		return;
	}
	kmAlertConsole = basicConsole;
	if (kmAlertConsole) {
	    (*kmAlertConsole->Init)(
		kmAlertConsole, SCM_ALERT, FALSE, TRUE, windowTitle);
	    while(*msg)
		(kmAlertConsole->PutC)(kmAlertConsole, *msg++);
	}
	return;
    }

#if	KERNOBJC
    /* 
     * Create alert panel if necessary
     */
    (*lockProc)(kmId->kmOpenLock, @selector(lock));
    switch(kmId->fbMode) {
	case SCM_TEXT:
	    /*
	     * Easy case, we already have a place to do putc()'s. 
	     * Note for SCM_TEXT case, alert text just goes to 
	     * the normal console.
	     */
	    break;

	/*
	 * FIXME - maybe do a case for SCM_UNINIT, do allow printfs
	 * via alert() very early in boot.
	 */		
	default:
	    /*
	     * Jump into "alert mode". 
	     */

            kmId->savedFbMode = kmId->fbMode;
            kmId->fbMode = SCM_ALERT;

            /*
            * Create an alert window.
            */
            (*kmId->fbp[FB_DEV_ALERT]->Init)(
                kmId->fbp[FB_DEV_ALERT], SCM_ALERT, saveUnder, TRUE, windowTitle);

	    /* fall through */

	case SCM_ALERT:
	    kmId->alertRefCount++;
	    break;
    }
    (*unlockProc)(kmId->kmOpenLock, @selector(unlock));
    
    /*
     * Print alert message.
     */
    {
    IOConsoleInfo *cons;
    
	if (kmId->fbMode == SCM_ALERT)
	    cons = kmId->fbp[FB_DEV_ALERT];
	else
	    cons = kmId->fbp[FB_DEV_NORMAL];
	while(*msg)
	    (*cons->PutC)(cons, *msg++);
    }
#endif	// KERNOBJC

    return;
}

extern int scc_getc(int unit, int line, boolean_t wait, boolean_t raw);
extern PCKeyboardEvent * PPCStealKeyEvent();

// This gets called from interrupt level by kdp

int kmtrygetc()
{
    int c = -1;
    PCKeyboardEvent * event;

    event = PPCStealKeyEvent();
    if(event)
	c = ProcessKbdEvent(event);

    if( (c != -1) && ( c != inv)) {
	return(c);
    } else {
/* modem port is 1, printer port is 0 */
#define LINE	1
	return( scc_getc(0 /* ignored */, LINE, 0, 0));
    }
}

void DoAlert(const char *windowTitle, const char *msg)
{
    DoSafeAlert(windowTitle, msg, TRUE);
}

#if	KERNOBJC

- (void)doAlert	: (const char *)windowTitle
		   msg : (const char *)msg
{
	DoSafeAlert(windowTitle, msg, TRUE);
}


- (IOReturn)setIntValues		: (unsigned *)parameterArray
			   forParameter : (IOParameterName)parameterName
			          count : (unsigned)count
{
	if(!strcmp(parameterName, "prettyShutdown"))
	{
		prettyShutdown = (short)(*parameterArray);
		return IO_R_SUCCESS;
	}
	else
	{ 
	/* Pass parameters we don't recognize to our superclass. */
        return [super setIntValues:parameterArray
		forParameter:parameterName count:count];
	}
}

@end

#endif	// KERNOBJC

/* end of kmDevice.m */


