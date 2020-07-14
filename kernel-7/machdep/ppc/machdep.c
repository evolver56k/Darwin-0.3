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
 * Copyright (c) 1997 Apple Computer, Inc.  All rights reserved.
 * Copyright (c) 1992 NeXT Computer, Inc.  All rights reserved.
 *
 * machdep/ppc/machdep.c
 *
 * Machine dependent cruft.
 *
 * 27-Apr-1997  A.Ramesh at Apple 
 *   Added machine dependant table calls
 *
 * March, 1997	Created.	Umesh Vaishampayan [umeshv@NeXT.com]
 *
 */

#import <mach/mach_types.h>
#import <mach/exception.h>
#include <machdep/ppc/exception.h>
#import <mach/machine.h>
#import <sys/reboot.h>
#include <kern/miniMonPrivate.h>
#include <machine/spl.h>
#include <mach_debug/mach_debug_types.h>
#include <ppc/powermac.h>
#include <machine/thread.h>
#include <ppc/cpu_data.h>
#include <ppc/pcb_flags.h>

int reboot_how;
extern struct tty	cons;
extern struct tty	*constty;		/* current console device */

static void prettyPrint(char *);
extern void cuda_restart(int powerOff);
extern int getchar();

void
halt_thread() {
	boot (RB_BOOT, reboot_how, "");
}

void
reboot_mach (how)
{
	if (kernel_task) {
		reboot_how = how;
		thread_call_func((void *)halt_thread, 0, TRUE);
	} else
		boot(RB_BOOT, how|RB_NOSYNC, "");
}


static void
prettyPrint(char *str)
{
	printf(str);
    kmGraphicPanelString(str);
}

void
halt_cpu()
{

	kmDisableAnimation();
	machine_slot[cpu_number()].running = FALSE;

	cuda_restart(1);

	/* Print a friendly message. */
	prettyPrint("It's safe to turn off the computer.\n");
	while (1)
		;
}

void
md_prepare_for_shutdown(paniced, howto, command)
int	paniced, howto;
char	*command;
{
    if (howto & RB_POWERDOWN)
	prettyPrint("Please wait until it's safe\n to turn off the computer.\n");
}

void
md_shutdown_devices(paniced, howto, command)
int	paniced, howto;
char	*command;
{
	printf("md_shutdown_devices() called\n");
}


void
system_power_down(void)
{
	cuda_restart(1);
	prettyPrint("System shutdown.\n It's safe to turn off the computer.\n");
	while (1) {}
}

void
powermac_reboot()
{
    cuda_restart(0);
    printf("It's safe to restart\n");
    while( 1) {}
}

void
md_do_shutdown(paniced, howto, command)
int	paniced, howto;
char	*command;
{
	if (howto & RB_POWERDOWN)
	    system_power_down();

	if (howto & RB_HALT)
	    halt_cpu();

	if (paniced == RB_BOOT)
	    powermac_reboot();

	if (paniced == RB_PANIC)
	    halt_cpu();

	/* reboot the system without sync */
	boot(RB_BOOT, RB_AUTOBOOT|RB_NOSYNC, "");
	/*NOTREACHED*/
}


#define putchar cnputc

void
gets(buf)
	char *buf;
{
	register char *lp;
	register c;

	lp = buf;
	for (;;) {
		c = getchar() & 0177;
		switch(c) {
		case '\n':
		case '\r':
			*lp++ = '\0';
			return;
		case '\b':
			if (lp > buf) {
				lp--;
				putchar(' ');
				putchar('\b');
			}
			continue;
		case '#':
		case '\177':
			lp--;
			if (lp < buf)
				lp = buf;
			continue;
		case '@':
		case 'u'&037:
			lp = buf;
			putchar('\n');		/* XXX calls 'cnputc' on mips */
			continue;
		default:
			*lp++ = c;
		}
	}
}

int
getchar()
{
	int c;

	c = cngetc();

	if (c == 0x1b)		/* ESC ? */
		call_kdp();
	if (c == '\r')
		c = '\n';
        cnputc(c);
	return c;
}

#if DEBUG
int kdp_backtrace;
int kdp_sr_dump;
int kdp_dabr;

#import <kern/kdp_internal.h>

void
enter_debugger(
    unsigned int		exception,
    unsigned int		code,
    unsigned int		subcode,
    void			*saved_state,
    int				noisy
)
{
	unsigned int *fp;
	unsigned int register sp;
	int wasConnect;

        wasConnect = kdp.is_conn;
	if( !wasConnect)
            DoAlert("Kernel Exception", "");

	if (noisy) {
	if (kdp_backtrace) {
		__asm__ volatile("mr %0,r1" : "=r" (sp));

		printf("\nvector=%x, code=%x, subcode=%x\n",
			exception/4, code, subcode);
		regDump(saved_state);

		printf("stack backtrace - sp(%x)  ", sp);
		fp = *((unsigned int *)sp);
		while (fp) {
			printf("0x%08x ", fp[2]);
			fp = (unsigned int *)*fp;
		}
		printf("\n");
	}
	if (kdp_sr_dump) {
		dump_segment_registers();
	}

	printf("vector=%d  ", exception/4);
	}

	kdp_raise_exception(kdp_code(exception), code, subcode, saved_state);

	mtspr(dabr, kdp_dabr);

	if( !wasConnect)
            DoRestore();
}
#endif

#if 0

void
mini_mon(char *prompt, char *title, int ssp)
{
	//call_kdp();
	miniMonGdb();
	for (;;) ;
}

#else

void
mini_mon(prompt, title, ssp)
	char *prompt, *title;
	void *ssp;
{
	char *restartMsg = "\nRestart or shutdown?\n\n"
			   "Type R to restart, or H to shutdown.\n"
			   "Type C to continue.\n";
	register int s, c, panic;

	s = splhigh();
	// Clear any buffered keys
	while( (-1) != kmtrygetc())
	    {}

	if (strcmp(prompt, "restart") == 0) {
	    DoSafeAlert(title, restartMsg, TRUE);
	    while (1) {
		c = kmtrygetc();
                if( (c == 'r') || (c == 'R'))
		    reboot_mach(RB_AUTOBOOT);
                if( (c == 'h') || (c == 'H'))
		    reboot_mach(RB_HALT);
		if (c != -1)
		    break;
	    }
	} else {
	    panic = (strcmp(prompt, "panic") == 0);
	    DoSafeAlert(title, "", FALSE);
	    if (panic)
		kmdumplog();
	    do {
		miniMonLoop(prompt, panic, ssp);
	    } while (panic);
	}
	DoRestore();
	splx(s);
}

#endif

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

	infop->page_size = 4096;
	infop->dcache_block_size = powermac_machine_info.dcache_block_size;
	infop->dcache_size = powermac_machine_info.dcache_size;
	infop->icache_size = powermac_machine_info.icache_size;
	infop->caches_unified = powermac_machine_info.caches_unified;
	infop->processor_version = powermac_machine_info.processor_version;
	infop->cpu_clock_rate_hz = powermac_machine_info.cpu_clock_rate_hz;
	infop->bus_clock_rate_hz = powermac_info.bus_clock_rate_hz;
	infop->dec_clock_rate_hz = powermac_machine_info.dec_clock_rate_hz;

    return KERN_SUCCESS;
}


kern_return_t
enable_bluebox(
      host_t host,
      unsigned flag)
{
  thread_t th = current_thread();

  if (host == HOST_NULL)
    return KERN_INVALID_HOST;

  if (!is_suser())
    return KERN_FAILURE;

  if (flag & ~PCB_BB_MASK)
    return KERN_FAILURE;

  th->pcb->flags |= (flag & PCB_BB_MASK);
  cpu_data[cpu_number()].flags = th->pcb->flags; /* prime pump */

  return KERN_SUCCESS;

}

#endif /* MACH_DEBUG */
