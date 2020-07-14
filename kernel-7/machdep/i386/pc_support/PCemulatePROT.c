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
 * Protected mode emulation.
 *
 * HISTORY
 *
 * 8 Apr 1993 ? at NeXT
 *	Created.
 */

#import <mach/mach_types.h>

#import <architecture/i386/sel.h>
#import <architecture/i386/table.h>

#import <machdep/i386/trap.h>
#import <machdep/i386/desc_inline.h>
#import <machdep/i386/err_inline.h>

#import "PCprivate.h"
#import "PCmiscInline.h"
#import "PCtaskInline.h"

static __inline__
boolean_t
fetch_LDT_entry(
    thread_t		thread,
    unsigned int	index,
    ldt_entry_t		*desc
)
{
    PCshared_t		shared = threadPCShared(thread);
    unsigned int	byte_index = index * sizeof (ldt_entry_t);
    
    if (shared->LDT.length <= byte_index)
    	return (FALSE);
	
    return (fetch8Byte(thread, shared->LDT.address + byte_index, desc));
}

static __inline__
boolean_t
fetch_IDT_entry(
    thread_t		thread,
    unsigned int	index,
    idt_entry_t		*desc
)
{
    PCshared_t		shared = threadPCShared(thread);
    unsigned int	byte_index = index * sizeof (ldt_entry_t);
    
    if (shared->IDT.length <= byte_index)
    	return (FALSE);
	
    return (fetch8Byte(thread, shared->IDT.address + byte_index, desc));
}

static
boolean_t
fetch_desc(
    thread_t		thread,
    sel_t		sel,
    dt_entry_t		*desc
)
{
    if (sel.ti != SEL_LDT)
    	return (FALSE);
	
    return (fetch_LDT_entry(thread, sel.index, (ldt_entry_t *)desc));
}

static __inline__
vm_offset_t
segment_base(
    data_desc_t		*desc
)
{
    conv_base_t		_base = { 0 };
    
    _base.fields.base00 = desc->base00;
    _base.fields.base16 = desc->base16;
    _base.fields.base24 = desc->base24;
    
    return (_base.word);
}

static __inline__
boolean_t
fetch_segment_4byte(
    thread_t		thread,
    data_desc_t		*desc,
    vm_offset_t		offset,
    boolean_t		offsz,
    unsigned int	*data
)
{
    vm_offset_t		base = segment_base(desc);

    if (!offsz)
    	offset &= 0xffff;
	
    return (fetch4Byte(thread, base + offset, data));
}

static
boolean_t __inline__
fetch_segment(
    thread_t		thread,
    data_desc_t		*desc,
    vm_offset_t		offset,
    boolean_t		offsz,
    void		*data,
    unsigned int	length
)
{
#define sdata		((unsigned short *)data)
    vm_offset_t		base = segment_base(desc);
    unsigned int	offmsk;
    
    if (offsz)
    	offmsk = 0xffffffff;
    else
    	offmsk = 0xffff;

    thread->recover = (vm_offset_t)&&fault;
    
    while (length > 0) {
	offset &= offmsk;

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
    data_desc_t		*desc,
    vm_offset_t		offset,
    boolean_t		offsz,
    void		*data,
    unsigned int	length
)
{
#define sdata		((unsigned short *)data)
    vm_offset_t		base = segment_base(desc);
    unsigned int	offmsk;
    
    if (offsz)
    	offmsk = 0xffffffff;
    else
    	offmsk = 0xffff;

    thread->recover = (vm_offset_t)&&fault;
    
    while (length > 0) {
	offset &= offmsk;

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
    code_desc_t		code;
    data_desc_t		stack;
    unsigned int	inst;
    unsigned char	bbyte;
    
    if (!fetch_desc(thread, state->frame.cs, (dt_entry_t *)&code))
    	return (FALSE);
    
    if (!fetch_segment_4byte(
			thread,
			(data_desc_t *)&code, state->frame.eip, code.opsz,
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

	if (!fetch_desc(thread, state->frame.ss, (dt_entry_t *)&stack))
	    return (FALSE);
	
	if (!fetch_segment(
			thread,
			&stack, state->frame.esp, stack.stksz,
			&args, sizeof (args)))
	    return (FALSE);
	    
	return (PCbopFA(thread, state, &args));
    }
    else if (bbyte == 0xFC) {
    	PCbopFC_t	args;

	if (!fetch_desc(thread, state->frame.ss, (dt_entry_t *)&stack))
	    return (FALSE);
	
	if (!fetch_segment(
			thread,
			&stack, state->frame.esp, stack.stksz,
			&args, sizeof (args)))
	    return (FALSE);
	    
	return (PCbopFC(thread, state, &args));
	/* NOTREACHED */
    }
    else if (bbyte == 0xFD) {
    	PCbopFD_t	args;

	if (!fetch_desc(thread, state->frame.ss, (dt_entry_t *)&stack))
	    return (FALSE);
	
	if (!fetch_segment(
			thread,
			&stack, state->frame.esp, stack.stksz,
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

static
boolean_t
emulate_CLI(
    thread_t			thread,
    thread_saved_state_t	*state,
    code_desc_t			*desc
)
{
    PCcontext_t		context = threadPCContext(thread);

    if (desc->opsz)	
	state->frame.eip += 1;
    else
	(unsigned short)state->frame.eip += 1;
	
    context->IF_flag = FALSE;
    
    return (TRUE);
}

static
boolean_t
emulate_STI(
    thread_t			thread,
    thread_saved_state_t	*state,
    code_desc_t			*desc
)
{
    PCcontext_t		context = threadPCContext(thread);

    if (desc->opsz)	
	state->frame.eip += 1;
    else
	(unsigned short)state->frame.eip += 1;
    
    if (!context->IF_flag) {
    	context->IF_flag = TRUE;
	
	context->pendingCallbacks |= (context->runOptions & PC_RUN_IF_SET);
    }
    
    return (TRUE);
}

static
boolean_t
push_frame16_err_code(
    thread_t			thread,
    thread_saved_state_t	*state,
    boolean_t			IF_flag,
    err_code_t			err
)
{
    struct {
    	unsigned short	err;
	unsigned short	ip;
	sel_t		cs;
	unsigned short	flags;
    }				frame;    
    data_desc_t			stack;
    	
    if (!fetch_desc(thread, state->frame.ss, (dt_entry_t *)&stack))
    	return (FALSE);		/* not possible */
	
    frame.err = err_to_error_code(err);

    frame.ip = state->frame.eip;
    frame.cs = state->frame.cs;
    
    frame.flags = state->frame.eflags;
    if (IF_flag)
    	frame.flags |= EFL_IF;
    else
    	frame.flags &= ~EFL_IF;
	
    if (!store_segment(
		    thread,
		    &stack,
		    state->frame.esp - sizeof (frame), stack.stksz,
		    &frame, sizeof (frame)))
	return (FALSE);

    if (stack.stksz)
	state->frame.esp -= sizeof (frame);
    else 
	(unsigned short)state->frame.esp -= sizeof (frame);
    
    return (TRUE);
}

static
boolean_t
push_frame16(
    thread_t			thread,
    thread_saved_state_t	*state,
    boolean_t			IF_flag
)
{
    struct {
	unsigned short	ip;
	sel_t		cs;
	unsigned short	flags;
    }				frame;    
    data_desc_t			stack;
    	
    if (!fetch_desc(thread, state->frame.ss, (dt_entry_t *)&stack))
    	return (FALSE);		/* not possible */

    frame.ip = state->frame.eip;
    frame.cs = state->frame.cs;
    
    frame.flags = state->frame.eflags;
    if (IF_flag)
    	frame.flags |= EFL_IF;
    else
    	frame.flags &= ~EFL_IF;
	
    if (!store_segment(
		    thread,
		    &stack,
		    state->frame.esp - sizeof (frame), stack.stksz,
		    &frame, sizeof (frame)))
	return (FALSE);

    if (stack.stksz)
	state->frame.esp -= sizeof (frame);
    else 
	(unsigned short)state->frame.esp -= sizeof (frame);
    
    return (TRUE);
}

static
boolean_t
push_frame32_err_code(
    thread_t			thread,
    thread_saved_state_t	*state,
    boolean_t			IF_flag,
    err_code_t			err
)
{
    struct {
    	err_code_t	err;
	unsigned int	eip;
	sel_t		cs;
	unsigned short		:16;
	unsigned int	eflags;
    }				frame;    
    data_desc_t			stack;
    	
    if (!fetch_desc(thread, state->frame.ss, (dt_entry_t *)&stack))
    	return (FALSE);		/* not possible */
	
    frame.err = err;

    frame.eip = state->frame.eip;
    frame.cs = state->frame.cs;
    
    frame.eflags = state->frame.eflags;
    if (IF_flag)
    	frame.eflags |= EFL_IF;
    else
    	frame.eflags &= ~EFL_IF;

    if (!store_segment(
		    thread,
		    &stack,
		    state->frame.esp - sizeof (frame), stack.stksz,
		    &frame, sizeof (frame)))
	return (FALSE);

    if (stack.stksz)
	state->frame.esp -= sizeof (frame);
    else 
	(unsigned short)state->frame.esp -= sizeof (frame);
    
    return (TRUE);
}

static
boolean_t
push_frame32(
    thread_t			thread,
    thread_saved_state_t	*state,
    boolean_t			IF_flag
)
{
    struct {
	unsigned int	eip;
	sel_t		cs;
	unsigned short		:16;
	unsigned int	eflags;
    }				frame;    
    data_desc_t			stack;
    	
    if (!fetch_desc(thread, state->frame.ss, (dt_entry_t *)&stack))
    	return (FALSE);		/* not possible */

    frame.eip = state->frame.eip;
    frame.cs = state->frame.cs;
    
    frame.eflags = state->frame.eflags;
    if (IF_flag)
    	frame.eflags |= EFL_IF;
    else
    	frame.eflags &= ~EFL_IF;
	
    if (!store_segment(
		    thread,
		    &stack,
		    state->frame.esp - sizeof (frame), stack.stksz,
		    &frame, sizeof (frame)))
	return (FALSE);
	
    if (stack.stksz)
	state->frame.esp -= sizeof (frame);
    else 
	(unsigned short)state->frame.esp -= sizeof (frame);
    
    return (TRUE);
}

static __inline__
void
enter_gate(
    except_frame_t	*frame,
    trap_gate_t		*gate,
    boolean_t		preserve_trace
)
{
    conv_offset_t		_offset = { 0 };
    
    _offset.fields.offset00 = gate->offset00;
    _offset.fields.offset16 = gate->offset16;
    
    frame->eip = _offset.word;
    frame->cs = gate->seg;

    if (!preserve_trace)	
	frame->eflags &= ~EFL_TF;
}

static
boolean_t
emulate_exception(
    thread_t			thread,
    thread_saved_state_t	*state,
    int				trapno,
    err_code_t			err,
    boolean_t			inhibit_err_code
)
{
    PCcontext_t			context;
    trap_gate_t			gate;
    boolean_t			is_intr, is_32bit;
    code_desc_t			code;
    
    context = threadPCContext(thread);
    
    if (!fetch_IDT_entry(thread, trapno, (idt_entry_t *)&gate))
    	return (FALSE);		/* GP(trapno*8+2+EXT) */

    switch (gate.type) {
	
    case DESC_INTR_GATE:
	is_32bit = TRUE;
	is_intr = TRUE;
	break;
    
    case DESC_TRAP_GATE:
	is_32bit = TRUE;
	is_intr = FALSE;
	break;
	
    case 0x06:
	is_32bit = FALSE;
	is_intr = TRUE;
	break;
	
    case 0x07:
	is_32bit = FALSE;
	is_intr = FALSE;
	break;
	
    default:
    	return (FALSE);		/* GP(trapno*8+2+EXT) */
    }
	
    if (!gate.present)
    	return (FALSE);		/* NP(trapno*8+2+EXT) */
	
    if (!fetch_desc(thread, gate.seg, (dt_entry_t *)&code))
    	return (FALSE);		/* GP(selector+EXT) */
	
    if ((code.type & 0x18) != 0x18) // if (!code)
    	return (FALSE);		/* GP(selector+EXT) */
	
    if (!code.present)
    	return (FALSE);		/* NP(selector+EXT) */
	
    if (inhibit_err_code || (trapno < 8) || trapno == 16 || (trapno > 17)) {
	if (is_32bit) {
	    if (!push_frame32(thread, state, context->IF_flag))
	    	return (FALSE);
	}
	else {
	    if (!push_frame16(thread, state, context->IF_flag))
	    	return (FALSE);
	}
    }
    else
    {
	if (is_32bit) {
	    if (!push_frame32_err_code(thread, state, context->IF_flag, err))
	    	return (FALSE);
	}
	else {
	    if (!push_frame16_err_code(thread, state, context->IF_flag, err))
	    	return (FALSE);
	}
    }

#define preserveTrace ((context->debugOptions & PC_DEBUG_PRESERVE_TRACE) != 0)
    enter_gate(&state->frame, &gate, preserveTrace);
#undef preserveTrace
    
    if (is_intr)
    	context->IF_flag = FALSE;

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
	
    if (!emulate_exception(
    			thread,
			state, state->trapno, state->frame.err,
			FALSE))
	return (FALSE);
    
    return (TRUE);
}

static
boolean_t
emulate_INTn(
    thread_t			thread,
    thread_saved_state_t	*state,
    code_desc_t			*desc,
    unsigned int		inst
)
{
    PCshared_t			shared;
    PCcontext_t			context;
    unsigned char		trapno;
    err_code_t			err = { 0 };

    shared = threadPCShared(thread);
    context = currentContext(shared);
    
    trapno = *((unsigned char *)&inst + 1);
	
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

    if (desc->opsz)	
	state->frame.eip += 2;
    else
    	(unsigned short)state->frame.eip += 2;
    
    if (!emulate_exception(thread, state, trapno, err, TRUE)) {
	if (desc->opsz)	
	    state->frame.eip -= 2;
	else
	    (unsigned short)state->frame.eip -= 2;
	return (FALSE);
    }
    
    return (TRUE);
}

static
boolean_t
emulate_instruction(
    thread_t			thread,
    thread_saved_state_t	*state
)
{
    code_desc_t		code;
    unsigned int	inst;
    unsigned char	ibyte;
    
    if (!fetch_desc(thread, state->frame.cs, (dt_entry_t *)&code))
	return (FALSE);
	
    if (!fetch_segment_4byte(
    			thread,
			(data_desc_t *)&code, state->frame.eip, code.opsz,
			&inst))
    	return (FALSE);
	
    ibyte = *(unsigned char *)&inst;
	
    if (ibyte == 0xCD)
    	return (emulate_INTn(thread, state, &code, inst));
	
    if (ibyte == 0xFA)
    	return (emulate_CLI(thread, state, &code));
	
    if (ibyte == 0xFB)
    	return (emulate_STI(thread, state, &code));
	
    return (FALSE);
}

boolean_t
PCemulatePROT(
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
