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
 *	Console is on the Printer Port (chip channel 0)
 *	Debugger is on the Modem Port (chip channel 1)
 */

#define	CONSOLE_PORT	0
#define	KGDB_PORT	1

/*
 * function declarations for performing serial i/o
 * other functions below are declared in kern/misc_protos.h
 *    cnputc, cngetc, cnmaygetc
 */

#if MACH_KGDB
void kgdb_putc(char c), no_spl_scc_putc(int chan, char c);
int kgdb_getc(boolean_t timeout), no_spl_scc_getc(int chan, boolean_t timeout);

/* kgdb_getc() special return values. */
#define KGDB_GETC_BAD_CHAR -1
#define KGDB_GETC_TIMEOUT  -2
#endif

void initialize_serial(void);

#ifdef notdef_next
extern int		scc_probe(
				caddr_t xxx,
				void *bus_device);

extern io_return_t	scc_open(
				dev_t		dev,
				dev_mode_t	flag,
				io_req_t	ior);

extern void		scc_close(
				dev_t		dev);

extern io_return_t	scc_read(
				dev_t		dev,
				io_req_t	ior);

extern io_return_t	scc_write(
				dev_t		dev,
				io_req_t	ior);

extern io_return_t	scc_get_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		data,
				mach_msg_type_number_t	*status_count);

extern io_return_t	scc_set_status(
				dev_t			dev,
				dev_flavor_t		flavor,
				dev_status_t		data,
				mach_msg_type_number_t	status_count);

extern boolean_t	scc_portdeath(
				dev_t		dev,
				ipc_port_t	port);

extern int	 	scc_putc(
				int			unit,
				int			line,
				int			c);

extern int		scc_getc(
				int			unit,
				int			line,
				boolean_t		wait,
				boolean_t		raw);

/* Functions in serial_console.c for switching between serial and video
   consoles.  */
extern int		switch_to_serial_console(
				void);

extern int		switch_to_video_console(
				void);

extern void		switch_to_old_console(
				int			old_console);

#if MACH_KGDB
extern void		no_spl_putc(char c);
extern int		no_spl_getc(void);
#endif
#endif /* notdef_next */
