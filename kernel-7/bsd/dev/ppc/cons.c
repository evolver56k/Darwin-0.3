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
 * Copyright (c) 1987, 1988 NeXT, Inc.
 *
 * HISTORY
 *  7-Jan-93  Mac Gillon (mgillon) at NeXT
 *	Integrated POSIX support
 *
 * 12-Aug-87  John Seamons (jks) at NeXT
 *	Ported to NeXT.
 */ 

/*
 * Indirect driver for console.
 */
#import <sys/param.h>
#import <sys/systm.h>
#import <sys/conf.h>
#import <sys/ioctl.h>
#import <sys/tty.h>
#import <sys/proc.h>
#import <sys/uio.h>
#import <bsd/dev/ppc/cons.h>

struct tty	cons;
struct tty	*constty;		/* current console device */

/*ARGSUSED*/
int
cnopen(dev, flag, devtype, pp)
	dev_t dev;
	int flag, devtype;
	struct proc *pp;
{
	dev_t device;

	if (constty)
	    device = constty->t_dev;
	else
	    device = cons.t_dev;
	return ((*cdevsw[major(device)].d_open)(device, flag, devtype, pp));
}

/*ARGSUSED*/
int
cnclose(dev, flag, mode, pp)
	dev_t dev;
	int flag, mode;
	struct proc *pp;
{
	dev_t device;

	if (constty)
	    device = constty->t_dev;
	else
	    device = cons.t_dev;
	return ((*cdevsw[major(device)].d_close)(device, flag, mode, pp));
}

/*ARGSUSED*/
int
cnread(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
	dev_t device;

	if (constty)
	    device = constty->t_dev;
	else
	    device = cons.t_dev;
	return ((*cdevsw[major(device)].d_read)(device, uio, ioflag));
}

/*ARGSUSED*/
int
cnwrite(dev, uio, ioflag)
	dev_t dev;
	struct uio *uio;
	int ioflag;
{
    dev_t device;

	if (constty)
	    device = constty->t_dev;
	else
	    device = cons.t_dev;
    return ((*cdevsw[major(device)].d_write)(device, uio, ioflag));
}

/*ARGSUSED*/
int
cnioctl(dev, cmd, addr, flag, p)
	dev_t dev;
	int cmd;
	caddr_t addr;
	int flag;
	struct proc *p;
{
	dev_t device;

	if (constty)
	    device = constty->t_dev;
	else
	    device = cons.t_dev;
	/*
	 * Superuser can always use this to wrest control of console
	 * output from the "virtual" console.
	 */
	if (cmd == TIOCCONS && constty) {
		int error = suser(p->p_ucred, (u_short *) NULL);
		if (error)
			return (error);
		constty = NULL;
		return (0);
	}
	return ((*cdevsw[major(device)].d_ioctl)(device, cmd, addr, flag, p));
}

/*ARGSUSED*/
int
cnselect(dev, flag, p)
	dev_t dev;
	int flag;
	struct proc *p;
{
	dev_t device;

	if (constty)
	    device = constty->t_dev;
	else
	    device = cons.t_dev;
	return ((*cdevsw[major(device)].d_select)(device, flag, p));
}

int
cngetc()
{
	dev_t device;

	if (constty)
	    device = constty->t_dev;
	else
	    device = cons.t_dev;
	return ((*cdevsw[major(device)].d_getc)(device));
}

/*ARGSUSED*/
int
cnputc(c)
	char c;
{
	dev_t device;

	if (constty)
	    device = constty->t_dev;
	else
	    device = cons.t_dev;
	return ((*cdevsw[major(device)].d_putc)(device, c));
}

#if	NCPUS > 1
slave_cnenable()
{
	/* FIXME: what to do here? */
}
#endif	NCPUS > 1

/* Send a debug char to modem port */
#define LINE	1
extern int scc_putc(int unit, int line, int c);
extern int scc_getc(int unit, int line, boolean_t wait, boolean_t raw);

void
czputc(char c)
{
    if (c == '\n')
		(void) scc_putc(0 /* ignored */, LINE, '\r');

    (void) scc_putc(0 /* ignored */, LINE, c);
}

#include <stdarg.h>
#include <sys/subr_prf.h>

void
kprintf( const char *format, ...)
{
    extern int kdp_flag;

    if( kdp_flag & 2) {

	char buffer[256];
	char *sptr = buffer;
	char *cptr = buffer;
	va_list ap;
	
	va_start(ap, format);
	prf(format, ap, TOSTR, (struct tty *)&sptr);
	va_end(ap);
    
	while( cptr != sptr)
	    czputc( *cptr++);
    }
}

