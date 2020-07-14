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

/*	@(#)ldd.h	2.0	03/20/90	(c) 1990 NeXT	
 *
 * kernserv/prototypes.h - kernel prototypes used by loadable device drivers
 *
 * HISTORY
 *  04-Aug-97  Umesh Vaishampayan (umeshv@apple.com)
 *	Added current_proc_EXTERNAL() function for the use of kernel
 * 	lodable modules.
 *
 * 22-May-91	Gregg Kellogg (gk) at NeXT
 *	Split out public interface.  KERNEL_FEATURES is used to not conflict
 *	with within-KERNEL definitions.
 *
 * 16-Aug-90  Gregg Kellogg (gk) at NeXT
 *	Removed a lot of stuff that's defined in other header files. 
 *	Eventually this file should either go away or contain only imports of
 *	other files.
 *
 * 20-Mar-90	Doug Mitchell at NeXT
 *	Created.
 *
 */

#ifndef	_KERN_INTERNAL_PROTOTYPES_
#define _KERN_INTERNAL_PROTOTYPES_

#import <sys/types.h>
#import <sys/kernel.h>
#import <sys/buf.h>
#import <sys/uio.h>
#import <kernserv/machine/us_timer.h>
#import <kern/lock.h>

#ifdef	KERNEL
#ifdef	KERNEL_PRIVATE
#else
#import <kernserv/kalloc.h>
#import <mach/message.h>

/* Cause a thread to sleep or wakeup: */
void	assert_wait(void *event, boolean_t interruptible);
void	biodone(struct buf *bp);
int	biowait __P((struct buf *));
void	clear_wait(thread_t thread, int result, boolean_t interrupt_only);
void	thread_block(void);
void	thread_set_timeout(int ticks);
void	thread_sleep(void *event, simple_lock_t lock, boolean_t interruptible);
#endif

/* Get information about this thread or task: */

#ifdef	MACH_USER_API
extern task_t 	current_task_EXTERNAL();
extern thread_t	current_thread_EXTERNAL();
#define current_task()	 current_task_EXTERNAL()
#define current_thread() current_thread_EXTERNAL()

extern struct proc * current_proc_EXTERNAL();
#define current_proc() current_proc_EXTERNAL()
#endif	/* MACH_USER_API */

#ifdef	KERNEL_PRIVATE
#else
task_t	(current_task)(void);
int	thread_wait_result(void);
extern  task_t task_self();
extern 	thread_t thread_self();

/* Create or kill a thread: */
thread_t	kernel_thread(task_t task, void (*start)());
void		thread_halt_self(void);

/* Send a message: */
extern msg_return_t msg_send_from_kernel(
    msg_header_t *msgptr,
    msg_option_t option,
    msg_timeout_t tout
);
#endif
#endif	/* KERNEL */

/* Get or test a virtual address that corresponds to a hardware address: */
caddr_t	map_addr(caddr_t address, int size);
int	probe_rb(void *address);

/* Kill the loadable kernel server: */
#if	defined(KERNEL) && !defined(KERNEL_BUILD) && !defined(ASSERT)
#if	DEBUG
#define	ASSERT(e) \
	if ((e) == 0) { \
		printf ("ASSERTION " #e " failed at line %d in %s\n", \
		    __LINE__, __FILE__); \
		panic ("assertion failed"); \
	}
#else	/* DEBUG */
#define ASSERT(e)
#endif	/* DEBUG */
#endif	/* defined(KERNEL) && defined(KERNEL_BUILD) && !defined(ASSERT) */

/* Modify or inspect a string: */
char *	strcat(char *string1, const char *string2);
int 	strcmp(const char *string1, const char *string2);
int 	strncmp(const char *string1, const char *string2, unsigned long len);
char *	strcpy(char *to, const char *from);
char *  strncpy(char *to, const char *from, size_t len);
size_t 	strlen(const char *string);

/* In a UNIX-style server, determine whether the user has root privileges: */
int		suser (struct ucred *cred, u_short *acflag);


#endif	/* _KERN_INTERNAL_PROTOTYPES_ */
