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
 * Copyright (c) 1993 NeXT Computer, Inc.
 *
 * PC support Real mode emulation.
 *
 * HISTORY
 *
 * 23 Mar 1993 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <architecture/i386/sel.h>
#import <machdep/i386/trap.h>
#import <machdep/i386/err_inline.h>

#import "PCprivate.h"
#import "PCmiscInline.h"
#import "PCtaskInline.h"

struct inst_prefix {
    unsigned int	opnd	:1,
			addr	:1;
};

static __inline__
vm_offset_t
segment_base(
    sel_t		sel
)
{
    return ((*(unsigned short *)&sel) << 4);
}

static __inline__
fetch_segment_byte(
    thread_t		thread,
    sel_t		sel,
    unsigned short	offset,
    unsigned char	*data
)
{
    vm_offset_t		base = segment_base(sel);
    
    return (fetchByte(thread, base + offset, data));
}

static __inline__
fetch_segment_2byte(
    thread_t		thread,
    sel_t		sel,
    unsigned short	offset,
    unsigned short	*data
)
{
    vm_offset_t		base = segment_base(sel);
    
    return (fetch2Byte(thread, base + offset, data));
}

static __inline__
store_segment_2byte(
    thread_t		thread,
    sel_t		sel,
    unsigned short	offset,
    unsigned short	data
)
{
    vm_offset_t		base = segment_base(sel);
    
    return (store2Byte(thread, base + offset, data));
}

static __inline__
fetch_segment_4byte(
    thread_t		thread,
    sel_t		sel,
    unsigned short	offset,
    unsigned int	*data
)
{
    vm_offset_t		base = segment_base(sel);
    
    return (fetch4Byte(thread, base + offset, data));
}

static __inline__
store_segment_4byte(
    thread_t		thread,
    sel_t		sel,
    unsigned short	offset,
    unsigned int	data
)
{
    vm_offset_t		base = segment_base(sel);
    
    return (store4Byte(thread, base + offset, data));
}

static __inline__
boolean_t
fetch_segment(
    thread_t		thread,
    sel_t		sel,
    unsigned short	offset,
    void		*data,
    unsigned int	length
)
{
#define sdata		((unsigned short *)data)
    vm_offset_t		base = segment_base(sel);

    thread->recover = (vm_offset_t)&&fault;
    
    while (length > 0) {
	asm volatile(
	    "movw %%fs:%1,%0"
		    : "=q" (*sdata)
		    : "m" (*(unsigned short *)(base + offset)));
	
	sdata++; offset += sizeof (*sdata); length -= sizeof (*sdata);
    }

    thread->recover = 0;
    return (TRUE);
    
fault:
    thread->recover = 0;
    return (FALSE);
#undef sdata
}

static __inline__
boolean_t
store_segment(
    thread_t		thread,
    sel_t		sel,
    unsigned short	offset,
    void		*data,
    unsigned int	length
)
{
#define sdata		((unsigned short *)data)
    vm_offset_t		base = segment_base(sel);

    thread->recover = (vm_offset_t)&&fault;
    
    while (length > 0) {
	asm volatile(
	    "movw %0,%%fs:%1"
		    :
		    : "r" (*sdata), "m" (*(unsigned short *)(base + offset)));
	
	sdata++; offset += sizeof (*sdata); length -= sizeof (*sdata);
    }

    thread->recover = 0;
    return (TRUE);
    
fault:
    thread->recover = 0;
    return (FALSE);
#undef sdata
}

static
boolean_t
handle_bop(
    thread_t			thread,
    thread_saved_state_t	*state
)
{
    PCcontext_t		context = threadPCContext(thread);
    unsigned int	inst;
    unsigned char	bbyte;
    
    if (!fetch_segment_4byte(
    			thread,
			state->frame.cs, state->frame.eip,
			&inst))
	return (FALSE);
    	
    if (*(unsigned short *)&inst != 0xC4C4)
    	return (FALSE);
	
    bbyte = *((unsigned char *)&inst + 2);
	
    if (bbyte == 0xFE) {
	PCcancelTimers(context);

    	context->callHandler = PC_HANDLE_EXIT;
	
	PCcallMonitor(thread, state);
	/* NOTREACHED */
    }
    else if (bbyte == 0xFA) {
    	PCbopFA_t	args;
	
	if (!fetch_segment(
			thread,
			state->frame.ss, state->frame.esp,
			&args, sizeof (args)))
	    return (FALSE);
	
	return (PCbopFA(thread, state, &args));
	/* NOTREACHED */
    }
    else if (bbyte == 0xFC) {
    	PCbopFC_t	args;
	
	if (!fetch_segment(
			thread,
			state->frame.ss, state->frame.esp,
			&args, sizeof (args)))
	    return (FALSE);
	
	return (PCbopFC(thread, state, &args));
	/* NOTREACHED */
    }
    else if (bbyte == 0xFD) {
    	PCbopFD_t	args;
	
	if (!fetch_segment(
			thread,
			state->frame.ss, state->frame.esp,
			&args, sizeof (args)))
	    return (FALSE);
	
	PCbopFD(thread, state, &args);
	/* NOTREACHED */
    }
    else {
	context->trapNum = bbyte;
	context->callHandler = PC_HANDLE_BOP;
	
	PCcallMonitor(thread, state);
	/* NOTREACHED */
    }
}
			
static __inline__
boolean_t
fetch_inst(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				*offset,
    unsigned char		*inst,
    struct inst_prefix		*prefix
)
{
    sel_t		cs;
    unsigned short	ip;
    unsigned char	ibyte;
    int			n;

    cs = state->frame.cs;	
    ip = state->frame.eip;
    
    for (n = 0; n < 15; n++) {	// a reasonable limit
	if (!fetch_segment_byte(thread, cs, ip + n, &ibyte))
	    break;

	switch (ibyte) {

	case 0x66:	// Operand-size prefix
	    prefix->opnd = TRUE;
	    break;
	    
	case 0xf0:	// LOCK prefix
	    break;
	    
	default:
	    *offset = (ip + n) - ip;
	    *inst = ibyte - 0x90;
	    
	    return (TRUE);
	}
    }
    
    return (FALSE);
}

static
boolean_t
emulate_CLI(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				offset,
    struct inst_prefix		prefix
)
{
    PCcontext_t		context = threadPCContext(thread);
    
    (unsigned short)state->frame.eip += (offset + 1);
	
    context->IF_flag = FALSE;
    
    return (TRUE);
}

static
boolean_t
emulate_STI(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				offset,
    struct inst_prefix		prefix
)
{
    PCcontext_t		context = threadPCContext(thread);
	
    (unsigned short)state->frame.eip += (offset + 1);
	
    if (!context->IF_flag) {
	context->IF_flag = TRUE;

	context->pendingCallbacks |= (context->runOptions & PC_RUN_IF_SET);
    }
    
    return (TRUE);
}

static boolean_t	emulate_PUSHFD(
				    thread_t			thread,
				    thread_saved_state_t	*state,
				    int				offset,
				    struct inst_prefix		prefix);

static
boolean_t
emulate_PUSHF(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				offset,
    struct inst_prefix		prefix
)
{
    PCcontext_t			context;
    struct {
    	unsigned short	flags;
    } 				frame;
    
    if (prefix.opnd)
    	return (emulate_PUSHFD(thread, state, offset, prefix));

    context = threadPCContext(thread);
	
    frame.flags = state->frame.eflags;
    if (context->IF_flag)
	frame.flags |= EFL_IF;
    else
    	frame.flags &= ~EFL_IF;
	
    frame.flags |= (context->X_flags & (EFL_NT | EFL_IOPL));
				    
    if (!store_segment_2byte(
			    thread, 
			    state->frame.ss, state->frame.esp - sizeof (frame),
			    frame.flags))
    	return (FALSE);
	
    (unsigned short)state->frame.eip += (offset + 1);
    
    (unsigned short)state->frame.esp -= sizeof (frame);

    return (TRUE);	
}

static
boolean_t
emulate_PUSHFD(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				offset,
    struct inst_prefix		prefix
)
{
    PCcontext_t			context;
    struct {
    	unsigned int	eflags;
    } 				frame;

    context = threadPCContext(thread);
    
    frame.eflags = state->frame.eflags;
    if (context->IF_flag)
	frame.eflags |= EFL_IF;
    else
    	frame.eflags &= ~EFL_IF;
	
    frame.eflags |= (context->X_flags & (EFL_NT | EFL_IOPL));
				    
    if (!store_segment_4byte(
			    thread, 
			    state->frame.ss, state->frame.esp - sizeof (frame),
			    frame.eflags))
    	return (FALSE);
	
    (unsigned short)state->frame.eip += (offset + 1);
    
    (unsigned short)state->frame.esp -= sizeof (frame);

    return (TRUE);	
}

static boolean_t	emulate_POPFD(
				    thread_t			thread,
				    thread_saved_state_t	*state,
				    int				offset,
				    struct inst_prefix		prefix);

static
boolean_t
emulate_POPF(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				offset,
    struct inst_prefix		prefix
)
{
    PCcontext_t			context;
    struct {
	unsigned short	flags;
    }				frame;
    
    if (prefix.opnd)
    	return (emulate_POPFD(thread, state, offset, prefix));

    context = threadPCContext(thread);
				
    if (!fetch_segment_2byte(
			    thread, 
			    state->frame.ss, state->frame.esp,
			    &frame.flags))
    	return (FALSE);
	
    if ((context->debugOptions & PC_DEBUG_PRESERVE_TRACE) != 0)
	frame.flags |= (state->frame.eflags & EFL_TF);
    (unsigned short)state->frame.eflags = frame.flags;
    state->frame.eflags &= ~( EFL_NT | EFL_IOPL | EFL_CLR );
    state->frame.eflags |=  ( EFL_VM | EFL_IF | EFL_SET );
    
    (unsigned short)state->frame.eip += (offset + 1);
    (unsigned short)state->frame.esp += sizeof (frame);
    
    context->X_flags = (frame.flags & (EFL_NT | EFL_IOPL));
	
    if (frame.flags & EFL_IF) {
	if (!context->IF_flag)
	    context->pendingCallbacks |= (context->runOptions & PC_RUN_IF_SET);

	context->IF_flag = TRUE;
    }
    else
    	context->IF_flag = FALSE;
    
    return (TRUE);
}

static
boolean_t
emulate_POPFD(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				offset,
    struct inst_prefix		prefix
)
{
    PCcontext_t			context;
    struct {
	unsigned long	eflags;
    }				frame;

    context = threadPCContext(thread);
				
    if (!fetch_segment_4byte(
			    thread, 
			    state->frame.ss, state->frame.esp,
			    &frame.eflags))
    	return (FALSE);
	
    if ((context->debugOptions & PC_DEBUG_PRESERVE_TRACE) != 0)
	frame.eflags |= (state->frame.eflags & EFL_TF);
    state->frame.eflags = frame.eflags;
    state->frame.eflags &= ~( EFL_NT | EFL_IOPL | EFL_CLR );
    state->frame.eflags |=  ( EFL_VM | EFL_IF | EFL_SET );
    
    (unsigned short)state->frame.eip += (offset + 1);
    (unsigned short)state->frame.esp += sizeof (frame);
    
    context->X_flags = (frame.eflags & (EFL_NT | EFL_IOPL));
	
    if (frame.eflags & EFL_IF) {
	if (!context->IF_flag)
	    context->pendingCallbacks |= (context->runOptions & PC_RUN_IF_SET);

	context->IF_flag = TRUE;
    }
    else
    	context->IF_flag = FALSE;
    
    return (TRUE);
}

static boolean_t	emulate_IRETD(
				    thread_t			thread,
				    thread_saved_state_t	*state,
				    int				offset,
				    struct inst_prefix		prefix);

static
boolean_t
emulate_IRET(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				offset,
    struct inst_prefix		prefix
)
{
    PCcontext_t			context;
    struct {
    	unsigned short	ip;
	sel_t		cs;
	unsigned short	flags;
    }				frame;
    
    if (prefix.opnd)
    	return (emulate_IRETD(thread, state, offset, prefix));

    context = threadPCContext(thread);
				
    if (!fetch_segment(
		    thread,
		    state->frame.ss, state->frame.esp,
		    &frame, sizeof (frame)))
    	return (FALSE);
	
    (unsigned short)state->frame.eip = frame.ip;
    state->frame.cs = frame.cs;

    if ((context->debugOptions & PC_DEBUG_PRESERVE_TRACE) != 0)
	frame.flags |= (state->frame.eflags & EFL_TF);
    (unsigned short)state->frame.eflags = frame.flags;
    state->frame.eflags &= ~( EFL_NT | EFL_IOPL | EFL_CLR );
    state->frame.eflags |=  ( EFL_VM | EFL_IF | EFL_SET );
    
    (unsigned short)state->frame.esp += sizeof (frame);
    
    context->X_flags = (frame.flags & (EFL_NT | EFL_IOPL));
	
    if (frame.flags & EFL_IF) {
	if (!context->IF_flag)
	    context->pendingCallbacks |= (context->runOptions & PC_RUN_IF_SET);

	context->IF_flag = TRUE;
    }
    else
    	context->IF_flag = FALSE;
    
    return (TRUE);
}

static
boolean_t
emulate_IRETD(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				offset,
    struct inst_prefix		prefix
)
{
    PCcontext_t			context;
    struct {
    	unsigned long	eip;
	sel_t		cs;
	unsigned long	eflags;
    }				frame;

    context = threadPCContext(thread);
        
    if (!fetch_segment(
		    thread,
		    state->frame.ss, state->frame.esp,
		    &frame, sizeof (frame)))
    	return (FALSE);
	
    state->frame.eip = frame.eip;
    state->frame.cs = frame.cs;
	
    if ((context->debugOptions & PC_DEBUG_PRESERVE_TRACE) != 0)
	frame.eflags |= (state->frame.eflags & EFL_TF);
    state->frame.eflags = frame.eflags;
    state->frame.eflags &= ~( EFL_NT | EFL_IOPL | EFL_CLR );
    state->frame.eflags |=  ( EFL_VM | EFL_IF | EFL_SET );
    
    (unsigned short)state->frame.esp += sizeof (frame);
    
    context->X_flags = (frame.eflags & (EFL_NT | EFL_IOPL));
	
    if (frame.eflags & EFL_IF) {
	if (!context->IF_flag)
	    context->pendingCallbacks |= (context->runOptions & PC_RUN_IF_SET);

	context->IF_flag = TRUE;
    }
    else
    	context->IF_flag = FALSE;
    
    return (TRUE);
}

static
boolean_t
emulate_exception(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				trapno
)
{
    PCcontext_t			context;
    struct {
    	unsigned short	ip;
	sel_t		cs;
	unsigned short	flags;
    }				frame;
    struct {
	unsigned short	ip;
	sel_t		cs;
    }				vector;

    context = threadPCContext(thread);
    
    frame.ip = state->frame.eip;
    frame.cs = state->frame.cs;
    
    frame.flags = state->frame.eflags;
    if (context->IF_flag)
	frame.flags |= EFL_IF;
    else
    	frame.flags &= ~EFL_IF;
	
    frame.flags |= (context->X_flags & (EFL_NT | EFL_IOPL));
    
    if (!store_segment(
		    thread,
		    state->frame.ss, state->frame.esp - sizeof (frame),
		    &frame, sizeof (frame)))
	return (FALSE);
	
    if (!fetch4Byte(thread, (trapno << 2), (unsigned long *)&vector))
    	return (FALSE);
	
    (unsigned short)state->frame.esp -= sizeof (frame);
	
    state->frame.eip = vector.ip;
    state->frame.cs = vector.cs;
	
    context->IF_flag = FALSE;
    
    if ((context->debugOptions & PC_DEBUG_PRESERVE_TRACE) == 0)
	state->frame.eflags &= ~EFL_TF;
    
    return (TRUE);
}

static
boolean_t
emulate_INTn(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				offset,
    struct inst_prefix		prefix
)
{
    PCshared_t			shared;
    PCcontext_t			context;
    unsigned char		trapno;

    shared = threadPCShared(thread);
    context = currentContext(shared);

    if (!fetch_segment_byte(
			    thread, 
			    state->frame.cs, state->frame.eip + offset + 1,
			    &trapno))
	return (FALSE);
	
    if (maskIsSet(&shared->intNMask, trapno)) {
	context->trapNum = trapno;
	context->errCode = 0;
	if (trapno <= 7)
	    context->callHandler = PC_HANDLE_FAULT;
	else
	    context->callHandler = PC_HANDLE_INTN;
	
	PCcallMonitor(thread, state);
	/* NOTREACHED */
    }
	
    (unsigned short)state->frame.eip += (offset + 2);
	
    if (!emulate_exception(thread, state, trapno)) {
	(unsigned short)state->frame.eip -= (offset + 2);
	return (FALSE);
    }
    
    return (TRUE);
}

static
boolean_t
handle_exception(
    thread_t			thread,
    thread_saved_state_t	*state
)
{
    PCshared_t			shared;
    PCcontext_t			context;
    
    shared = threadPCShared(thread);
    context = currentContext(shared);
    
    if (maskIsSet(&shared->faultMask, state->trapno)) {
	context->trapNum = state->trapno;
	context->errCode = err_to_error_code(state->frame.err);
	context->callHandler = PC_HANDLE_FAULT;
	
	PCcallMonitor(thread, state);
	/* NOTREACHED */
    }
	
    if (!emulate_exception(thread, state, state->trapno))
	return (FALSE);
    
    return (TRUE);
}

#define none	0
#define STI	emulate_STI
#define CLI	emulate_CLI
#define PUSHF	emulate_PUSHF
#define POPF	emulate_POPF
#define IRET	emulate_IRET
#define INTn	emulate_INTn

static
boolean_t (*inst_table[256])() =
{
    /* 90 */ none, /* 91 */ none, /* 92 */ none, /* 93 */ none, 
    /* 94 */ none, /* 95 */ none, /* 96 */ none, /* 97 */ none, 
    /* 98 */ none, /* 99 */ none, /* 9A */ none, /* 9B */ none, 
    /* 9C */PUSHF, /* 9D */ POPF, /* 9E */ none, /* 9F */ none, 
    /* A0 */ none, /* A1 */ none, /* A2 */ none, /* A3 */ none, 
    /* A4 */ none, /* A5 */ none, /* A6 */ none, /* A7 */ none, 
    /* A8 */ none, /* A9 */ none, /* AA */ none, /* AB */ none, 
    /* AC */ none, /* AD */ none, /* AE */ none, /* AF */ none, 
    /* B0 */ none, /* B1 */ none, /* B2 */ none, /* B3 */ none, 
    /* B4 */ none, /* B5 */ none, /* B6 */ none, /* B7 */ none, 
    /* B8 */ none, /* B9 */ none, /* BA */ none, /* BB */ none, 
    /* BC */ none, /* BD */ none, /* BE */ none, /* BF */ none, 
    /* C0 */ none, /* C1 */ none, /* C2 */ none, /* C3 */ none, 
    /* C4 */ none, /* C5 */ none, /* C6 */ none, /* C7 */ none, 
    /* C8 */ none, /* C9 */ none, /* CA */ none, /* CB */ none, 
    /* CC */ none, /* CD */ INTn, /* CE */ none, /* CF */ IRET, 
    /* D0 */ none, /* D1 */ none, /* D2 */ none, /* D3 */ none, 
    /* D4 */ none, /* D5 */ none, /* D6 */ none, /* D7 */ none, 
    /* D8 */ none, /* D9 */ none, /* DA */ none, /* DB */ none, 
    /* DC */ none, /* DD */ none, /* DE */ none, /* DF */ none, 
    /* E0 */ none, /* E1 */ none, /* E2 */ none, /* E3 */ none, 
    /* E4 */ none, /* E5 */ none, /* E6 */ none, /* E7 */ none, 
    /* E8 */ none, /* E9 */ none, /* EA */ none, /* EB */ none, 
    /* EC */ none, /* ED */ none, /* EE */ none, /* EF */ none, 
    /* F0 */ none, /* F1 */ none, /* F2 */ none, /* F3 */ none, 
    /* F4 */ none, /* F5 */ none, /* F6 */ none, /* F7 */ none, 
    /* F8 */ none, /* F9 */ none, /* FA */  CLI, /* FB */  STI, 
    /* FC */ none, /* FD */ none, /* FE */ none, /* FF */ none, 
};

static
boolean_t
emulate_instruction(
    thread_t			thread,
    thread_saved_state_t	*state
)
{
    int				offset;
    unsigned char		inst;
    struct inst_prefix		prefix = { 0 };
    boolean_t			(*emul)();

    return (
	fetch_inst(thread, state, &offset, &inst, &prefix)	&&
	(emul = inst_table[inst]) != 0				&&
	(*emul)(thread, state, offset, prefix));
}

boolean_t
PCemulateREAL(
    thread_t			thread,
    thread_saved_state_t	*state
)
{
    if (state->trapno == T_GENERAL_PROTECTION) {
    	if (!emulate_instruction(thread,state))
	    return handle_exception(thread, state);
	else
	    return TRUE;
    }
    else
    if (state->trapno == T_INVALID_OPCODE) {
    	if (!handle_bop(thread, state))
	    return handle_exception(thread, state);
	else
	    return TRUE;
    }
    else
	return handle_exception(thread, state);
}
