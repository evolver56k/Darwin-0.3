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
 * km.m - kernel keyboard/monitor module, procedural interface.
 *
 * HISTORY
 * 21 Sep 92	Joe Pasqua
 *	Created i386 version based on m88k version.
 */
 
#import <driverkit/IODevice.h>
#import <driverkit/generalFuncs.h>
#import <bsd/sys/param.h>
#import <bsd/sys/proc.h>
#import <bsd/sys/conf.h>
#import <sys/systm.h>
#import <bsd/sys/tty.h>
#import <bsd/sys/uio.h>
#import <bsd/sys/fcntl.h>		/* for kmopen */
#import <bsd/sys/errno.h>
#import <bsd/sys/msgbuf.h>
#import <bsd/dev/kmreg_com.h>
#import <kern/thread_call.h>
#import <kernserv/ns_timer.h>
#import <bsd/dev/i386/kmDevice.h>
#import <bsd/dev/i386/km.h>
#import <bsd/dev/i386/BasicConsole.h>
#include <machdep/i386/kernBootStruct.h>

#import <kern/assert.h>

/*
 * 'Global' variables, shared only by this file and conf.c.
 */
extern struct tty	cons;
struct tty *km_tty[1] = { &cons };

/*
 * 'Global' variables, shared only by this file and kmDevice.m.
 */
kmDevice *kmId;				// the kmDevice
IOConsoleInfo *basicConsole;
ScreenMode basicConsoleMode;
extern IOConsoleInfo *kmAlertConsole;
static int initialized;			// = 0

/*
 * Static functions.
 */
static int kmoutput(struct tty *tp);
static void kmstart(struct tty *tp);

void kminit();

/*
 * cdevsw interface to km driver.
 */
int 
kmopen(
	dev_t dev, 
	int flag,
	int devtype, 
	struct proc *pp)
{
	int rtn;
	int unit;
	struct tty *tp;
	struct winsize *wp;
	struct ConsoleSize size;
	int ret;
	
#if notdef
	if (flag & (O_POPUP|O_ALERT))
	{
		/* ensure text console visible, in case someone does a late */
		/* open of /dev/console for alert purposes */
		bcTextMode();
	}
#endif

	unit = minor(dev);
	if(unit >= NUM_KM_DEVS)
		return (ENXIO);

	if ( (rtn = [kmId kmOpen:flag]) )
		return rtn;

	/*
	 * This code lifted from 68k km.c.
	 */
	tp = (struct tty *)&cons;
	tp->t_oproc = kmstart;
	tp->t_param = NULL;
	tp->t_dev = dev;
	
	if ((tp->t_state & TS_ISOPEN) == 0) {
		tp->t_iflag = TTYDEF_IFLAG;
		tp->t_oflag = TTYDEF_OFLAG;
		tp->t_cflag = (CREAD | CS8 | CLOCAL);
		tp->t_lflag = TTYDEF_LFLAG;
		tp->t_ispeed = tp->t_ospeed = TTYDEF_SPEED;
		termioschars(&tp->t_termios);
		ttsetwater(tp);
	} else if ((tp->t_state & TS_XCLUDE) && pp->p_ucred->cr_uid != 0)
		return EBUSY;

	tp->t_state |= TS_CARR_ON; /* lie and say carrier exists and is on. */
	ret = ((*linesw[tp->t_line].l_open)(dev, tp));

	if (ret == 0) {
		[kmId getScreenSize:&size];
		wp = &tp->t_winsize;
		wp->ws_row = size.rows;
		wp->ws_col = size.cols;
		wp->ws_xpixel = size.pixel_width;
		wp->ws_ypixel = size.pixel_height;
	}
	
	return ret;
}

int 
kmclose(
	dev_t dev, 
	int flag,
	int mode,
	struct proc *p)
{
	/*
	 * Note we don't close possible alert window here; we have to rely on
	 * user to do a KMIOCRESTORE.
	 */
	 
	struct tty *tp;

	tp = &cons;
	(*linesw[tp->t_line].l_close)(tp,flag);
	ttyclose(tp);
	return (0);
}

int 
kmread(
	dev_t dev, 
	struct uio *uio,
	int ioflag)
{
	register struct tty *tp;
 
	tp = &cons;
	return ((*linesw[tp->t_line].l_read)(tp, uio, ioflag));
}

int 
kmwrite(
	dev_t dev, 
	struct uio *uio,
	int ioflag)
{
	register struct tty *tp;
 
	tp = &cons;
	return ((*linesw[tp->t_line].l_write)(tp, uio, ioflag));
}

int 
kmioctl(
	dev_t dev, 
	int cmd, 
	caddr_t data, 
	int flag,
	struct proc *p)
{
	int error;
	struct tty *tp = &cons;
	struct ConsoleSize size;
	struct winsize *wp;
	
	switch (cmd) {
	    case KMIOCRESTORE:
	    	return [kmId restore];
		
	    case KMIOCDRAWRECT:
	    	return [kmId drawRect:(struct km_drawrect *)data];
		
	    case KMIOCERASERECT:
	    	return [kmId eraseRect:(struct km_drawrect *)data];

	    case KMIOCDISABLCONS:
		return [kmId disableCons];
		
	    case KMIOCDUMPLOG:
	    	return [kmId dumpMsgBuf];
		
	    case KMIOCANIMCTL:
	    	return [kmId animationCtl:*(km_anim_ctl_t *)data];
		
	    case KMIOCSTATUS:
		return [kmId getStatus:(unsigned *)data];

	    case KMIOCSIZE:
		[kmId getScreenSize:&size];
		wp = (struct winsize *)data;
		wp->ws_row = size.rows;
		wp->ws_col = size.cols;
		wp->ws_xpixel = size.pixel_width;
		wp->ws_ypixel = size.pixel_height;
		return 0;
		
	    case TIOCSWINSZ:
		/* Prevent changing of console size --
		 * this ensures that login doesn't revert to the
		 * termcap-defined size
		 */
		return EINVAL;

	    /* Bodge in the CLOCAL flag as the km device is always local */
	    case TIOCSETA:
	    case TIOCSETAW:
	    case TIOCSETAF: {
		register struct termios *t = (struct termios *)data;
		t->c_cflag |= CLOCAL;
		/* No Break */
	    }
	    default:		
		error = (*linesw[tp->t_line].l_ioctl)(tp, cmd, data, flag, p);
		if (error >= 0) {
			return error;
		}
		error = ttioctl (tp, cmd, data, flag, p);
		if (error >= 0) {
			return error;
		}
		else {
			return ENOTTY;
		}
	}
}

/*
 * this works early on, after console_init() but before autoconf (and thus
 * before we have a kmDevice).
 */
int 
kmputc(
	dev_t dev,
	int c)
{
	IOConsoleInfo *console;
	
	ASSERT(fbg != NULL);
	if(kmId) {
		/*
		 * After autoconfig, let kmDevice handle this.
		 */
		if (c == '\n') {
			[kmId kmPutc:'\r'];
		}
		return [kmId kmPutc:c];
	}
	
	console = kmAlertConsole ? kmAlertConsole : basicConsole;
	if(c == '\n') {
		(*console->PutC)(console, '\r');
	}
	(*console->PutC)(console, c);
	return 0;
}

int 
kmgetc(
	dev_t dev)
{
	/* found in bsd/dev/i386/cons.c */
	extern int cnputc(char c);
	int c;
	
	if(kmId == nil) {
		return 0;
	}
	c = [kmId kmGetc];
	if (c == '\r') {
		c = '\n';
	}
	cnputc(c);
	return c;
}

int 
kmgetc_silent(
	dev_t dev)
{
	int c;
	
	if(kmId == nil) {
		return 0;
	}
	c = [kmId kmGetc];
	if (c == '\r') {
		c = '\n';
	}
	return c;
}

/*
 * Callouts from linesw.
 */
 
#define KM_LOWAT_DELAY	((ns_time_t)1000)

static void 
kmstart(struct tty *tp)
{
	if (tp->t_state & (TS_TIMEOUT | TS_BUSY | TS_TTSTOP))
		goto out;
	if (tp->t_outq.c_cc == 0)
		goto out;
	tp->t_state |= TS_BUSY;
	if (tp->t_outq.c_cc > tp->t_lowat) {
		/*
		 * Start immediately.
		 */
		thread_call_func(
			(thread_call_func_t) kmoutput, tp, TRUE);
	}
	else {
		/*
		 * Wait a bit...
		 */
		ns_timeout((func) kmoutput, tp, KM_LOWAT_DELAY,
				CALLOUT_PRI_THREAD);
	}
out:
	ttwwakeup(tp);
}

static int 
kmoutput(
	struct tty *tp)
{
	/*
	 * FIXME - to be grokked...copied from m68k km.c.
	 */
	char 		buf[80];
	char 		*cp;
	int 		cc = -1;

	while (tp->t_outq.c_cc > 0) {
		cc = ndqb(&tp->t_outq, 0);
		if (cc == 0)
			break;
		cc = min(cc, sizeof buf);
		(void) q_to_b(&tp->t_outq, buf, cc);
		for (cp = buf; cp < &buf[cc]; cp++) {
			[kmId kmPutc:(*cp & 0x7f)];
		}
	}
        if (tp->t_outq.c_cc > 0) {
		thread_call_func(
			(thread_call_func_t)kmoutput, tp, TRUE);
	}
	tp->t_state &= ~TS_BUSY;
	ttwwakeup(tp);
	return 0;
}

/*
 * Alert panel interface (which we'd like to do away with if at all 
 * possible).
 */
int 
kmpopup (
	const char *title, 
	int flag, 
	int width, 
	int height, 
	boolean_t allocmem)
{
	if (!initialized) {
	    kminit();
	}
	DoAlert(title, "");
	return 0;
}

int 
kmrestore()
{
	DoRestore();
	return 0;
}


/*
 * Write message to console; create an alert panel if no text-type window
 * currently exists. Caller must call alert_done() when finished.
 * The height and width arguments are not used; they are provided for 
 * compatibility with the 68k version of alert().
 */
int 
alert(
	int width, 
	int height, 
	const char *title, 
	const char *msg, 
	int p1, 
	int p2, 
	int p3, 
	int p4, 
	int p5, 
	int p6, 
	int p7, 
	int p8)
{
	char smsg[200];
	
	sprintf(smsg, msg,  p1, p2, p3, p4, p5, p6, p7, p8);
	DoAlert(title, smsg);
	return 0;
}

int 
alert_done()
{
	DoRestore();
	return 0;
}

/*
 * printf() a message to an an alert panel. Can be used any time after calling
 * alert(). This might go away before release, let's see..
 */
void 
aprint(	
	const char *msg, 
	int p1, 
	int p2, 
	int p3, 
	int p4, 
	int p5, 
	int p6, 
	int p7, 
	int p8)
{
 	char smsg[200];
	char *cp = smsg;
	
	ASSERT(kmId != nil);
	sprintf(smsg, msg,  p1, p2, p3, p4, p5, p6, p7, p8);
	while(cp) {
		kmputc(0, *cp++);
	}
}
 
/*
 * Dump message log.
 */
void kmdumplog()
{
	if (kmId != nil)
	    [kmId dumpMsgBuf];
}

/*
 * Initialize the console enough for printf's to work via kmputc. No 
 * text window will be created if booting in icon mode. Keyboard is 
 * not enabled. 
 */
 
void
kminit()
{
    KERNBOOTSTRUCT *kernbootstruct = KERNSTRUCT_ADDR;

    initialized = TRUE;

    basicConsole = BasicAllocateConsole();
    if (kernbootstruct->graphicsMode) {
	basicConsoleMode = SCM_GRAPHIC;
	(*basicConsole->Init)(
	    basicConsole, SCM_GRAPHIC, FALSE, FALSE, mach_title);
    } else {
	basicConsoleMode = SCM_TEXT;
	(*basicConsole->Init)(
	    basicConsole, SCM_TEXT, TRUE, TRUE, mach_title);
    }
}

void
kmDrawGraphicPanel(GraphicPanelType panelType)
{
    if (kmId != nil)
	[kmId drawGraphicPanel:panelType];
}

void
kmGraphicPanelString(char *str)
{
    if (kmId != nil)
	[kmId graphicPanelString:str];
}

/* end of km.m */

