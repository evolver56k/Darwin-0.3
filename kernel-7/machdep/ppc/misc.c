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


#include <debug.h>

#include <mach/ppc/thread_status.h>
#include <mach/machine/vm_types.h>
#include <kern/thread.h>
#include <machdep/ppc/proc_reg.h>
#include <machdep/ppc/pmap.h>
#include <machdep/ppc/exception.h>

#define KGDB_DEBUG(x) kprintf x

/*
 * copyin/out_multiple - the assembler copyin/out functions jump to C for
 * help when the copyin lies over a segment boundary. The C breaks
 * down the copy into two sub-copies and re-calls the assembler with
 * these sub-copies. Very rare occurrance. Warning: These functions are
 * called whilst active_thread->thread_recover is still set.
 */

extern boolean_t copyin_multiple(const char *src,
				 char *dst,
				 vm_size_t count);

boolean_t copyin_multiple(const char *src,
			  char *dst,
			  vm_size_t count)
{
	const char *midpoint;
	vm_size_t first_count;
	boolean_t first_result;

	/* Assert that we've been called because of a segment boundary,
	 * this function is more expensive than the assembler, and should
	 * only be called in this difficult case.
	 */
	assert(((vm_offset_t)src & 0xF0000000) !=
	       ((vm_offset_t)(src + count -1) & 0xF0000000));
#if DEBUG
	KGDB_DEBUG(("copyin across segment boundary,"
		    "src=0x%08x, dst=0x%08x, count=0x%x\n", src, dst, count));
#endif /* DEBUG */
	/* TODO NMGS define sensible constants for segments, and apply
	 * to C and assembler (assembler is much harder)
	 */
	midpoint = (const char*) ((vm_offset_t)(src + count) & 0xF0000000);
	first_count = (midpoint - src);

#if DEBUG
	KGDB_DEBUG(("copyin across segment boundary : copyin("
		    "src=0x%08x, dst=0x%08x ,count=0x%x)\n",
		    src, dst, first_count));
#endif /* DEBUG */
	first_result = copyin(src, dst, first_count);
	
	/* If there was an error, stop now and return error */
	if (first_result != 0)
		return first_result;

	/* otherwise finish the job and return result */
#if DEBUG
	KGDB_DEBUG(("copyin across segment boundary : copyin("
		    "src=0x%08x, dst=0x%08x, count=0x%x)\n",
		    midpoint, dst+first_count, count-first_count));
#endif /* DEBUG */

	return copyin(midpoint, dst + first_count, count-first_count);
}

extern int copyout_multiple(const char *src, char *dst, vm_size_t count);

extern int copyout_multiple(const char *src, char *dst, vm_size_t count)
{
	char *midpoint;
	vm_size_t first_count;
	boolean_t first_result;

	/* Assert that we've been called because of a segment boundary,
	 * this function is more expensive than the assembler, and should
	 * only be called in this difficult case. For copyout, the
	 * segment boundary is on the dst
	 */
	assert(((vm_offset_t)dst & 0xF0000000) !=
	       ((vm_offset_t)(dst + count - 1) & 0xF0000000));

#if DEBUG
	KGDB_DEBUG(("copyout across segment boundary,"
		    "src=0x%08x, dst=0x%08x, count=0x%x\n", src, dst, count));
#endif /* DEBUG */

	/* TODO NMGS define sensible constants for segments, and apply
	 * to C and assembler (assembler is much harder)
	 */
	midpoint = (char *) ((vm_offset_t)(dst + count) & 0xF0000000);
	first_count = (midpoint - dst);

#if DEBUG
	KGDB_DEBUG(("copyout across segment boundary : copyout("
		    "src=0x%08x, dst=0x%08x, count=0x%x)\n", src, dst, count));
#endif /* DEBUG */
	first_result = copyout(src, dst, first_count);
	
	/* If there was an error, stop now and return error */
	if (first_result != 0)
		return first_result;

	/* otherwise finish the job and return result */

#if DEBUG
	KGDB_DEBUG(("copyout across segment boundary : copyout("
		    "src=0x%08x, dst=0x%08x, count=0x%x)\n",
		    src + first_count, midpoint, count-first_count));
#endif /* DEBUG */

	return copyout(src + first_count, midpoint, count-first_count);
}


/* TODO NMGS Mutexes aren't implemented yet. for now define dummies */

#include <kern/lock.h>

#if DEBUG
/*
 * This is a debugging routine, calling the assembler routine-proper
 *
 * Load the context for the first kernel thread, and go.
 */

extern void load_context(thread_t thread);

#define DPRINTF(x)	kprintf x

void load_context(thread_t thread)
{
	struct ppc_kernel_state *kss;

#if 0
	DPRINTF(("thread addr           =0x%08x\n",thread));
	DPRINTF(("thread pcb.ksp=0x%08x\n",thread->pcb->ksp));
	DPRINTF(("thread kstack =0x%08x\n",thread->kernel_stack));
	kss = STACK_IKS(thread->kernel_stack);
	DPRINTF(("thread kss.r1 =0x%08x\n",kss->r1));
	DPRINTF(("thread kss.lr =0x%08x\n",kss->lr));
	DPRINTF(("calling Load_context\n"));
#endif /* 0 */
	Load_context(thread);
}
#endif /* DEBUG */

#if DEBUG
#define kgdb_printf	kprintf
#define NULL	0
void regDump(struct ppc_saved_state *state)
{
	int i;

	for (i=0; i<32; i++) {
		if ((i % 8) == 0)
			kgdb_printf("\n%4d :",i);
			kgdb_printf(" %08x",*(&state->r0+i));
	}

	kgdb_printf("\n");
	kgdb_printf("cr        = 0x%08x\t\t",state->cr);
	kgdb_printf("xer       = 0x%08x\n",state->xer); 
	kgdb_printf("lr        = 0x%08x\t\t",state->lr); 
	kgdb_printf("ctr       = 0x%08x\n",state->ctr); 
	kgdb_printf("srr0(iar) = 0x%08x\t\t",state->srr0); 
	kgdb_printf("srr1(msr) = 0x%08x\n",state->srr1);
	/*kgdb_printf("srr1(msr) = 0x%08B\n",state->srr1,
		    "\x10\x11""EE\x12PR\x13""FP\x14ME\x15""FE0\x16SE\x18"
		    "FE1\x19""AL\x1a""EP\x1bIT\x1c""DT");*/
	kgdb_printf("mq        = 0x%08x\t\t",state->mq);
	kgdb_printf("sr_copyin = 0x%08x\n",state->sr_copyin);
	kgdb_printf("\n");

	/* Be nice - for user tasks, generate some stack trace */
	if (state->srr1 & MASK(MSR_PR)) {
		char *addr = (char*)state->r1;
		unsigned int buf[2];
		for (i = 0; i < 8; i++) {
			if (addr == (char*)NULL)
				break;
			if (!copyin(addr,(char*)buf, 2 * sizeof(int))) {
				printf("0x%08x : %08x\n",buf[0],buf[1]);
				addr = (char*)buf[0];
			} else {
				break;
			}
		}
	}
}

struct ppc_saved_state *enterDebugger(unsigned int trap,
				      struct ppc_saved_state *ssp,
				      unsigned int dsisr)
{
	KGDB_DEBUG(("trap = 0x%x (no=0x%x), dsisr=0x%08x\n",
		    trap, trap / EXC_VECTOR_SIZE,dsisr));
	KGDB_DEBUG(("%c\n%s iar=0x%08x dar=0x%08x dsisr 0x%08b\n",
	       7,
	       trap_type[trap / EXC_VECTOR_SIZE],
	       ssp->srr0,mfdar(),
	       dsisr,"\20\02NO_TRANS\05PROT\06ILL_I/O\07STORE\12DABR\15EAR"));

	//kgdb_trap(trap, trap, ssp);
	//call_kdp();
	enter_debugger(trap, 0, 0, ssp, 1);

	return ssp;
}
#endif /* DEBUG */

