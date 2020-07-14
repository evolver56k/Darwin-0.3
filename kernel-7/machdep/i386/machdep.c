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
 * Copyright (c) 1992 NeXT Computer, Inc.
 *
 * Machine dependent cruft.
 *
 * HISTORY
 *
 * 27-Apr-1997  A.Ramesh at Apple 
 *   Added machine dependant table calls
 *
 * Tue Dec  7 17:37:14 PST 1993 Matt Watson (mwatson) at NeXT
 *	Moved initrootnet() into swapgeneric.m
 *
 * 17 June 1992 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>
#import <mach/exception.h>
#import <mach/machine.h>

#import <kern/miniMon.h>
#import <kern/power.h>

#import <kern/sched_prim.h>

#import <vm/vm_kern.h>

#import <sys/param.h>
#import <sys/buf.h>
#import <sys/reboot.h>
#import <sys/callout.h>
#import <sys/proc.h>
#import <sys/user.h>
#import <sys/tty.h>
#import <sys/syslog.h>

#import <dev/kmreg_com.h>

#import <i386/reg.h>
#import <bsd/dev/i386/km.h>
#import <bsd/dev/i386/BasicConsole.h>	// XXX get rid of this

#import <machdep/i386/io_inline.h>
#import <machdep/i386/intr_exported.h>
#include <mach_debug/mach_debug_types.h>

extern struct tty	cons;
extern struct tty	*constty;		/* current console device */

int reboot_how;

short prettyShutdown;

static void prettyPrint(char *);

void
halt_thread() {
	boot (RB_BOOT, reboot_how);
}

void
reboot_mach (how)
	int how;
{
	if (kernel_task) {
		reboot_how = how;
		thread_call_func((void *)halt_thread, 0, TRUE);
	} else
		boot(RB_BOOT, how|RB_NOSYNC);
}

void
led_msg(msg)
char	*msg;
{
	register	i;
	
	for (i = 0; i < 4; i++)
	    outb(0xcaf - i, msg[i]);
}

#import <gdb.h>

/* nmi mini-monitor */
int	nmi_cont, nmi_gdb, nmi_mon, nmi_help, nmi_halt, nmi_msg, nmi_stay,
	nmi_reboot, nmi_big;
#if	XPR_DEBUG
int	nmi_xpr, nmi_xprclear;
#endif	XPR_DEBUG

void
mini_mon(prompt, title, locr0)
	char *prompt, *title;
	void *locr0;
{
	char *restartMsg = "Restart or halt?  Type r to restart,\n"
			   "or type h to halt.  Type n to cancel.\n";
	register int saved = 0, s, restart, c, panic;

	s = splhigh();
	restart = strcmp(prompt, "restart") == 0;
	panic = strcmp(prompt, "panic") == 0;
	if (panic) {
	    /* Grab the frame pointer, since it wasn't supplied by caller */
	    asm("movl %%ebp, %0" : "=m" (locr0) : /* no inputs */);
	}
	if (constty != &cons || /*(km.flags & KMF_SEE_MSGS) == 0*/TRUE) {
		saved = 1;
		if (restart) {
			DoSafeAlert(title, restartMsg, TRUE);
			restart = 1;
		} else
                        DoSafeAlert(title, "", FALSE);
		if (panic)
			kmdumplog();
	} else {
		DoAlert(title, "");
	}
	if (restart) {
		while (1) {
			c = kmtrygetc();
			if (c == 'r')
				reboot_mach(RB_AUTOBOOT);
			if (c == 'h')
				reboot_mach(RB_HALT);
			if (c != -1)
				break;
		}
		goto out;
	}
	do {
#if	GDB
	    miniMonLoop(prompt, panic, locr0);
#else	GDB
	    asm volatile("hlt");
#endif	GDB
	} while (panic);
out:
	if (saved && !nmi_stay)
		DoRestore();
	nmi_stay = 0;
	splx(s);
}

/* keep nmi messages from getting written to message log */
int
nmi_prf(fmt, x1)
	char *fmt;
	unsigned x1;
{
	return prf(fmt, &x1, 1, 0);
}

void halt_cpu(int howto)
{
	/* Print a friendly message. */
	// XXX use kmLocalizeString here.
//	if (constty != &cons || /*(km.flags & KMF_SEE_MSGS) == 0*/TRUE)
	{
		switch(glLanguage)
		{
		case L_ENGLISH:
		default:
			prettyPrint("It's safe to turn off the computer.\n");
			break;
		case L_FRENCH:
			prettyPrint("Vous pouvez maintenant eteindre\n"
				"votre ordinateur en toute securite.\n");
			break;
		case L_GERMAN:
			prettyPrint("Jetzt koennen Sie Ihren Computer\n"
				"sicher ausschalten.\n");
			break;
		case L_SPANISH:
			prettyPrint("Ahora es seguro apagar el ordenador.\n");
			break;
		case L_ITALIAN:
			prettyPrint("Ora puoi spegnere il computer.\n");
			break;
		case L_SWEDISH:
			prettyPrint("Nu ar det sakert att stanga av datorn.\n");
			break;
#if notdef
		case L_JAPANESE:
			prettyPrint("Japanese: It's safe to turn off the computer.\n");
			break;
#endif
		}
	}
	kmDisableAnimation();
	led_msg("Hltd");
	(void)intr_disbl();
	machine_slot[cpu_number()].running = FALSE;

	if (howto & RB_POWERDOWN)
	    PMSetPowerState(PM_SYSTEM_DEVICE, PM_OFF);

	for (;;)
	    asm volatile("hlt");
}

static void prettyPrint(char *str)
{
	printf(str);
	if (prettyShutdown)
	    kmGraphicPanelString(str);
}

void
md_prepare_for_shutdown(paniced, howto, command)
int	paniced, howto;
char	*command;
{
	// This string could be output when control returns
	// to the basic console in BasicConsole.c
	if (howto & RB_HALT)
	{
	    prettyPrint(
		kmLocalizeString("Please wait until it's safe\n"
			         "to turn off the computer.\n"));
	}
}

void
md_shutdown_devices(paniced, howto, command)
int	paniced, howto;
char	*command;
{
	/* For now, always turn devices off, even if we're rebooting */
	_io_setDriverPowerState(PM_OFF);
}

// saves a val to the day of week field in CMOS ram for the booter
static void valToCMOS(int val)
{
	int oldval;

	outb(0x70, 6);
	oldval = inb(0x71);
	val |= oldval;

	outb(0x70, 6);
	outb(0x71, val);
}

int	rebootflag;

void
md_do_shutdown(paniced, howto, command)
int	paniced, howto;
char	*command;
{
	rebootflag = 1;		/* if rebootflag is on, keyboard driver will
				 * send a CPU_RESET command to the keyboard
				 * controller to reset the system */
	if (howto&RB_HALT)
	    halt_cpu(howto);

	if (paniced == RB_PANIC)
	    halt_cpu(howto);


	// indicate whether rebooting into NeXTstep or DOS specified
	if (howto&RB_BOOTNEXT) valToCMOS(0x10);
	else if (howto&RB_BOOTDOS) valToCMOS(0x20);

	(void) intr_disbl();
	keyboard_reboot();

	for (;;)
	    asm volatile("hlt");
	/*NOTREACHED*/
}

#import <sys/table.h>

int machine_table_setokay(int id)
{
    return (TBL_MACHDEP_BAD);
}

int machine_table(int id, int index, caddr_t addr, int nel, u_int lel, int set)
{
    return (TBL_MACHDEP_NONE);
}


#if MACH_DEBUG
kern_return_t
host_machine_info(
	host_t	host,
	host_machine_info_t	*infop)
{
	if (host == HOST_NULL)
        return KERN_INVALID_HOST;

	/* Not implemented */
    return KERN_FAILURE;
}

kern_return_t
enable_bluebox(
	       host_t host,
	       unsigned flag)
{
  return KERN_FAILURE;
}
#endif /* MACH_DEBUG */
