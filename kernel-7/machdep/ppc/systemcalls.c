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
 * Copyright (c) 1997 Apple Computer, Inc.
 *
 * PowerPC Family:	System Call handlers.
 *
 * HISTORY
 * 18-Aug-97   Umesh Vaishampayan  (umeshv@apple.com)
 *	Added syscalltrace filtering. 
 * 27-July-97  Umesh Vaishampayan  (umeshv@apple.com)
 *	Debug printf() changed to kprintf(). Cleanup unused system timing.
 */
 
#include <mach/mach_types.h>
#include <mach/exception.h>
#include <mach/error.h>

#include <kern/syscall_sw.h>
#include <kern/kdp.h>
#include <kern/kdebug.h>

#include <machdep/ppc/frame.h>
#include <machdep/ppc/thread.h>
#include <machdep/ppc/asm.h>
#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/trap.h>
#include <machdep/ppc/exception.h>
#include <machdep/ppc/pcb_flags.h>

#include <sys/param.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/systm.h>

#if DIAGNOSTIC
/* syscalltrace defines */
#define _SCT_NUM_PIDS	8

#define _SCT_OFF	0x00
#define	_SCT_MACHCALLS	0x01
#define	_SCT_UNIXCALLS	0x02
#define	_SCT_ALL	(_SCT_MACHCALLS | _SCT_UNIXCALLS)

int syscalltrace = 0;
int usepids = 0;		/* 1 enables pid filtering */
int pidstraced[_SCT_NUM_PIDS];	/* can specify upto _SCT_NUM_PIDS to filter */

static int
matchpid( int pid )
{
	int i;
	for(i=0; i<_SCT_NUM_PIDS; i++)
		if( pidstraced[i] == pid )
			return 1; /* match! */
	return 0; /* not found */
}

#endif /* DIAGNOSTIC */

/*
** Function:	mach_syscall
**
** Inputs:	pcb	- pointer to Process Control Block
**		arg1	- arguments to mach system calls
**		arg2
**		arg3
**		arg4
**		arg5
**		arg6
**		arg7
**
** Outputs:	none
**
** Notes:	There may be additional arguments to the mach calls
**		that will be read in from user space if necessary
*/
void
mach_syscall(
    struct pcb * pcb,
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7
    )
{
    struct ppc_saved_state	*regs;
    int				error;	/* error code */
    struct uthread		*uthread;
    struct proc			*p;
    
    /* the mach trap number supplied by libmach */
    int				mach_trap;
    mach_trap_t			*trapp;
    thread_t			thread;
    int				arg0;
    struct stack_frame		*usfp;
    int				sbargs[4], uasfp, *argp;
    extern char			*mach_callnames[];  


    if (!USERMODE(pcb->ss.srr1))
	panic("mach_syscall");

    regs = &pcb->ss;

    thread = current_thread();
    uthread = thread->_uthread;
    uthread->uu_ar0 = (int *)pcb;
    p = current_proc();

    /* get the mach trap number */
    mach_trap = -(regs->r0);

    /* get the first argument to the mach trap call */
    arg0 = regs->r3;

    error = 0;

    /* find mach trap table entry */
    if (mach_trap >= mach_trap_count  ||  mach_trap < 0)
    {
	error = KERN_FAILURE;
    }
    else
    {
    	trapp = &mach_trap_table[mach_trap];

	if (trapp->mach_trap_arg_count > MAXREGARGS)
	{
	    int i, word;

	    /*
	    **  This code gets executed when there are more than MAXREGARGS
	    ** worth of parameters expected for the call.  The additional
	    ** parameters are passed on the users stack and must be copies
	    ** in from user space.
	    */
	    if (trapp->mach_trap_arg_count > NARGS)
		panic("mach_syscall: max arg count exceeded");

	    /* get the user stack frame pointer from saved state r1 */
	    usfp = (struct stack_frame *)regs->r1;

	    /* set up a pointer to the user arguments stack frame pointer */
	    uasfp = (int)&usfp->pa.spa.saved_p8;

	    /* setup the pointer for the stack based args */
	    argp = sbargs;

	    for (i = 0; i < (trapp->mach_trap_arg_count - MAXREGARGS); i++ )
	    {
		if (copyin ((void *)uasfp++, (void *)&word, sizeof(int)))
		{
 		    regs->r3 = -1;
		    thread_exception_return();
		}
		*argp++ = word;
	    }
	}

#if DIAGNOSTIC
	if ((syscalltrace == _SCT_MACHCALLS) || (syscalltrace == _SCT_ALL)) {
		int trpid = ((p) ? p->p_pid : -1);
		if((!usepids) || (usepids && matchpid(trpid)))
			kprintf("%s (%d) mach_syscall[%d = %s] --",
			((p) ? p->p_comm : "no-proc"),
			trpid, mach_trap, mach_callnames[mach_trap]);
	}
#endif /* DIAGNOSTIC */
	/*
	** Notice that there are always NARGS passed into the mach_trap
	** regardless whether they are needed or not.
	**
	** If NARGS changes this function call must be changed as well!!
	*/
	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_EXCP_SC, 0) | DBG_FUNC_START,
		                             mach_trap, arg0, arg1, arg2, arg3);

    	error = (*(trapp->mach_trap_function))(	arg0, arg1,
						arg2, arg3,
						arg4, arg5,
						arg6, arg7,
						sbargs[0], sbargs[1],
						sbargs[2], sbargs[3] );

	KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_EXCP_SC, 0) | DBG_FUNC_END,
		                             mach_trap, error, 0, 0, 0);
#if DIAGNOSTIC
	if ((syscalltrace == _SCT_MACHCALLS) || (syscalltrace == _SCT_ALL)) {
		int trpid = ((p) ? p->p_pid : -1);
		if((!usepids) || (usepids && matchpid(trpid))) {
			if (error)
	    			kprintf("> sys=0x%X, sub=0x%X, code=0x%X\n",
				err_get_system(error), err_get_sub(error),
				err_get_code(error));
			else
    				kprintf("> error == 0x%X\n", error);
		}
	}
#endif /* DIAGNOSTIC */
    }

    regs->r3 = error;

    thread_exception_return();
    /* NOTREACHED */
}


/*
** Function:	unix_syscall
**
** Inputs:	pcb	- pointer to Process Control Block
**		arg1	- arguments to mach system calls
**		arg2
**		arg3
**		arg4
**		arg5
**		arg6
**		arg7
**
** Outputs:	none
*/
void
unix_syscall(
    struct pcb * pcb,
    int arg1,
    int arg2,
    int arg3,
    int arg4,
    int arg5,
    int arg6,
    int arg7 
    )
{
    struct ppc_saved_state	*regs;
    thread_t			thread;
    struct uthread		*uthread;
    struct sysent		*callp;
    struct proc			*p;
    int				nargs, error;
    unsigned short		code;
    int				rval[2];
    extern char *syscallnames[];  

    if (!USERMODE(pcb->ss.srr1))
	panic("unix_syscall");

    regs = &pcb->ss;
    thread = current_thread();

    uthread = thread->_uthread;
    uthread->uu_ar0 = (int *)pcb;
    p = current_proc();

    /*
    ** Get index into sysent table
    */   
    code = regs->r0;

    if (code > nsysent)
    {
	error = 0;
	switch (code) {
	case 0x7FFA:
	  if ((thread->pcb->flags & PCB_BB_MASK) == 0) {
	     error = EPERM;
	     break;
	  }
	  NotifyInterruption(regs->r3,arg1,arg2,arg3,arg4,arg5,arg6,arg7);
	  break;
	case 0x7FFF:
	    mapBlueBoxShmem(regs->r3,arg1,arg2,arg3,arg4,arg5,arg6,arg7);
	    break;
	case 0x7FFD:
	    unmapBlueBoxShmem(regs->r3,arg1,arg2,arg3,arg4,arg5,arg6,arg7);
	    break;
	}
    }
    else {
	/*
	** Set up call pointer
	*/
	callp = (code >= nsysent) ? &sysent[63] : &sysent[code];

	/*
	** SPECIAL CASE function 0 to be indirect system call
	*/
	if (callp == sysent)
	{
	code = regs->r3;
	callp = (code >= nsysent) ? &sysent[63] : &sysent[code];
	uthread->uu_arg[0] = arg1;
	uthread->uu_arg[1] = arg2;
	uthread->uu_arg[2] = arg3;
	uthread->uu_arg[3] = arg4;
	uthread->uu_arg[4] = arg5;
	uthread->uu_arg[5] = arg6;
	uthread->uu_arg[6] = arg7;
	}
	else
	{
	/*
	** Set up the args for the system call
	*/ 
	uthread->uu_arg[0] = regs->r3; 
	uthread->uu_arg[1] = arg1;
	uthread->uu_arg[2] = arg2;
	uthread->uu_arg[3] = arg3;
	uthread->uu_arg[4] = arg4;
	uthread->uu_arg[5] = arg5;
	uthread->uu_arg[6] = arg6;
	uthread->uu_arg[7] = arg7;
	}

	if (callp->sy_narg > 8)
	panic("unix_syscall: max arg count exceeded");

	rval[0] = 0;

	/* r4 is volatile, if we set it to regs->r4 here the child
	 * will have parents r4 after execve */
	rval[1] = 0;

	error = 0; /* Start with a good value */

	/*
	** the PPC runtime calls cerror after every unix system call, so
	** assume no error and adjust the "pc" to skip this call.
	** It will be set back to the cerror call if an error is detected.
	*/
	regs->srr0 += 4;
#if DIAGNOSTIC
	if ((syscalltrace == _SCT_UNIXCALLS) || (syscalltrace == _SCT_ALL)) {
		int trpid = ((p) ? p->p_pid : -1);
		if((!usepids) || (usepids && matchpid(trpid))) {
	    		kprintf("%s(%d) unix_syscall[%d = %s] --",
			((p) ? p->p_comm : "no-proc"),
			trpid, code, syscallnames[code]);
	    		if ( (code == 5) || (code == 59) )
				kprintf(" arg0 = %x, arg1 = %d, arg2 = %d, "
				"arg3 = %d --",
				uthread->uu_arg[0], uthread->uu_arg[1],
				uthread->uu_arg[2], uthread->uu_arg[3]);
		}
	}
#endif /* DIAGNOSTIC */
#if KDEBUG
	if (code != 180) {
	        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_EXCP_SC, 1) | DBG_FUNC_START,
		                             code, uthread->uu_arg[0], uthread->uu_arg[1],
		                                   uthread->uu_arg[2], uthread->uu_arg[3]);
	}
#endif /* KDEBUG */

	error = (*(callp->sy_call))(p, (caddr_t)uthread->uu_arg, rval);

#if KDEBUG
	if (code != 180) {
	        KERNEL_DEBUG(MACHDBG_CODE(DBG_MACH_EXCP_SC, 1) | DBG_FUNC_END,
		                             code, error, rval[0], rval[1], 0);
	}
#endif /* KDEBUG */
#if DIAGNOSTIC
	if ((syscalltrace == _SCT_UNIXCALLS) || (syscalltrace == _SCT_ALL)) {
		int trpid = ((p) ? p->p_pid : -1);
		if((!usepids) || (usepids && matchpid(trpid)))
			if (error)
    				kprintf("> error == 0x%X\n", error);
			else
				kprintf("> rval=0x%X %X\n", rval[0],rval[1]);
	}
#endif /* DIAGNOSTIC */
    }
    if (error == ERESTART) {
	regs->srr0 -= 8;
    }
    else if (error != EJUSTRETURN) {
	if (error)
	{
	    regs->r3 = error;
	    /* set the "pc" to execute cerror routine */
	    regs->srr0 -= 4;
	} else { /* (not error) */
	    regs->r3 = rval[0];
	    regs->r4 = rval[1];
	} 
    }
    /* else  (error == EJUSTRETURN) { nothing } */

    thread_exception_return();
    /* NOTREACHED */
	
}

