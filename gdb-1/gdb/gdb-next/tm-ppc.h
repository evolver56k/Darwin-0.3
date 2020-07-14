/* Parameters for target execution on an RS6000, for GDB, the GNU debugger.
   Copyright (C) 1986, 1987, 1989, 1991 Free Software Foundation, Inc.
   Contributed by IBM Corporation.

This file is part of GDB.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef __TM_PPC_H__
#define __TM_PPC_H__

#include "ppc-tdep.h"
#include "ppc-frameops.h"
#include "ppc-frameinfo.h"

#define TEXT_SEGMENT_BASE	0x00000000

/* Say how long (ordinary) registers are.  This is a piece of bogosity
   used in push_word and a few other places; REGISTER_RAW_SIZE is the
   real way to know how big a register is.  */

#define REGISTER_SIZE 4

/* Define the byte order of the machine.  */

#define TARGET_BYTE_ORDER	BIG_ENDIAN

/* Define this if the C compiler puts an underscore at the front
   of external names before giving them to the linker.  */

#undef NAMES_HAVE_UNDERSCORE

/* Offset from address of function to start of its code.
   Zero on most machines.  */

#define FUNCTION_START_OFFSET 0

#define SKIP_PROLOGUE(PC) ppc_skip_prologue (PC)

#define INNER_THAN(lhs,rhs) ((lhs) < (rhs)) /* Stack grows downward.  */

#define	PUSH_ARGUMENTS(NARGS, ARGS, SP, STRUCT_RETURN, STRUCT_ADDR) \
  SP = ppc_push_arguments ((NARGS), (ARGS), (SP), (STRUCT_RETURN), (STRUCT_ADDR))

/* Sequence of bytes for breakpoint instruction.  */

#define BIG_BREAKPOINT { 0x7f, 0xe0, 0x00, 0x08 }
#define LITTLE_BREAKPOINT { 0x08, 0x00, 0xe0, 0x7f }

#if TARGET_BYTE_ORDER == BIG_ENDIAN
#define BREAKPOINT BIG_BREAKPOINT
#else
#if TARGET_BYTE_ORDER == LITTLE_ENDIAN
#define BREAKPOINT LITTLE_BREAKPOINT
#endif
#endif

#define DECR_PC_AFTER_BREAK 0

/* Nonzero if instruction at PC is a return instruction.  Allow any of
   the return instructions, including trapv and return from interrupt.  */

#define ABOUT_TO_RETURN(PC) \
   ((read_memory_unsigned_integer ((PC), 4) & 0xfe8007ff) == 0x4e800020)

/* Return 1 if P points to an invalid floating point value.  */

#define INVALID_FLOAT(P, LEN) ppc_invalid_float ((P), (LEN))

/* Say how long (ordinary) registers are.  */

#define REGISTER_TYPE unsigned int
#define FP_REGISTER_TYPE double

#define NUM_REGS 71 /* Number of machine registers */

/* Initializer for an array of names of registers.
   There should be NUM_REGS strings in this initializer.  */

#define REGISTER_NAMES  \
 {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7",  \
  "r8", "r9", "r10","r11","r12","r13","r14","r15", \
  "r16","r17","r18","r19","r20","r21","r22","r23", \
  "r24","r25","r26","r27","r28","r29","r30","r31", \
  "f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",  \
  "f8", "f9", "f10","f11","f12","f13","f14","f15", \
  "f16","f17","f18","f19","f20","f21","f22","f23", \
  "f24","f25","f26","f27","f28","f29","f30","f31", \
  "pc", "ps", "cr", "lr", "ctr", "xer", "mq" }

/* Register numbers of various important registers.
   Note that some of these values are "real" register numbers,
   and correspond to the general registers of the machine,
   and some are "phony" register numbers which are too large
   to be actual register numbers as far as the user is concerned
   but do serve to get the desired values when passed to read_register.  */

#define	GP0_REGNUM 0		/* GPR register 0 */
#define FP_REGNUM 1		/* Contains address of executing stack frame */
#define SP_REGNUM 1		/* Contains address of top of stack */
#define	TOC_REGNUM 2		/* TOC register */

#define FP0_REGNUM 32		/* FPR (Floating point) register 0 */
#define FPLAST_REGNUM 63	/* Last floating point register */  

#define PC_REGNUM 64		/* Program counter (instruction address %iar) */
#define PS_REGNUM 65		/* Processor (or machine) status (%msr) */
#define	CR_REGNUM 66		/* Condition register */
#define	LR_REGNUM 67		/* Link register */
#define	CTR_REGNUM 68		/* Count register */
#define	XER_REGNUM 69		/* Fixed point exception registers */
#define	MQ_REGNUM 70		/* Multiply/quotient register */

#define RV_REGNUM 3		/* Contains simple return values */
#define SRA_REGNUM RV_REGNUM	/* Contains address of struct return values */
#define FP_RV_REGNUM FP0_REGNUM

#define FIRST_GP_REGNUM 0
#define LAST_GP_REGNUM 31
#define NUM_GP_REGS (LAST_GP_REGNUM - FIRST_GP_REGNUM + 1)

#define FIRST_FP_REGNUM 32
#define LAST_FP_REGNUM 63
#define NUM_FP_REGS (LAST_FP_REGNUM - FIRST_FP_REGNUM + 1)

#define	FIRST_SP_REGNUM 64	/* first special register number */
#define LAST_SP_REGNUM  70	/* last special register number */
#define NUM_SP_REGS (LAST_SP_REGNUM - FIRST_SP_REGNUM + 1)

/* Total amount of space needed to store our copies of the machine's
   register state, the array `registers'.
	32 4-byte gpr's
	32 8-byte fpr's
	7  4-byte special purpose registers */

#define REGISTER_BYTES 416

/* Index within `registers' of the first byte of the space for
   register N.  */

#define REGISTER_BYTE(N)  \
  ( \
   ((N) >= FIRST_SP_REGNUM) ? ((((N) - FIRST_SP_REGNUM) * 4) + 384) \
   : ((N) >= FIRST_FP_REGNUM) ? ((((N) - FIRST_FP_REGNUM) * 8) + 128) \
   : ((N) * 4) )

/* Number of bytes of storage in the actual machine representation
   for register N. */

/* Note that the unsigned cast here forces the result of the
   subtractiion to very high positive values if N < FP0_REGNUM */

#define REGISTER_RAW_SIZE(N) (((unsigned) (N) - FP0_REGNUM) < 32 ? 8 : 4)

/* Number of bytes of storage in the program's representation
   for register N.  On the RS6000, all regs are 4 bytes
   except the floating point regs which are 8-byte doubles.  */

#define REGISTER_VIRTUAL_SIZE(N) (((unsigned) (N) - FP0_REGNUM) < 32 ? 8 : 4)

/* Largest value REGISTER_RAW_SIZE can have.  */

#define MAX_REGISTER_RAW_SIZE 8

/* Largest value REGISTER_VIRTUAL_SIZE can have.  */

#define MAX_REGISTER_VIRTUAL_SIZE 8

/* convert a dbx stab register number (from `r' declaration) to a gdb REGNUM */

#define STAB_REG_TO_REGNUM(VALUE) (VALUE)

/* Nonzero if register N requires conversion
   from raw format to virtual format.
   The register format for rs6000 floating point registers is always
   double, we need a conversion if the memory format is float.  */

#define REGISTER_CONVERTIBLE(N) ((N) >= FP0_REGNUM && (N) <= FPLAST_REGNUM)

/* Convert data from raw format for register REGNUM in buffer FROM
   to virtual format with type TYPE in buffer TO.  */

#define REGISTER_CONVERT_TO_VIRTUAL(REGNUM,TYPE,FROM,TO) \
{ \
  if (TYPE_LENGTH (TYPE) != REGISTER_RAW_SIZE (REGNUM)) \
    { \
      double val = extract_floating ((FROM), REGISTER_RAW_SIZE (REGNUM)); \
      store_floating ((TO), TYPE_LENGTH (TYPE), val); \
    } \
  else \
    memcpy ((TO), (FROM), REGISTER_RAW_SIZE (REGNUM)); \
}

/* Convert data from virtual format with type TYPE in buffer FROM
   to raw format for register REGNUM in buffer TO.  */

#define REGISTER_CONVERT_TO_RAW(TYPE,REGNUM,FROM,TO)	\
{ \
  if (TYPE_LENGTH (TYPE) != REGISTER_RAW_SIZE (REGNUM)) \
    { \
      double val = extract_floating ((FROM), TYPE_LENGTH (TYPE)); \
      store_floating ((TO), REGISTER_RAW_SIZE (REGNUM), val); \
    } \
  else \
    memcpy ((TO), (FROM), REGISTER_RAW_SIZE (REGNUM)); \
}

/* Return the GDB type object for the "standard" data type
   of data in register N.  */

#define REGISTER_VIRTUAL_TYPE(N) \
 (((unsigned)(N) - FP0_REGNUM) < 32 ? builtin_type_double : builtin_type_unsigned_int)

/* Store the address of the place in which to copy the structure the
   subroutine will return.  This is called from call_function. */

#define STORE_STRUCT_RETURN(ADDR, SP) \
  { write_register (SRA_REGNUM, (ADDR)); }

/* Extract from an array REGBUF containing the (raw) register state
   a function return value of type TYPE, and copy that, in virtual format,
   into VALBUF.  */

#define EXTRACT_RETURN_VALUE(TYPE, REGBUF, VALBUF) \
  ppc_extract_return_value (TYPE, REGBUF, VALBUF)

#define EXTRACT_STRUCT_VALUE_ADDRESS(REGBUF) \
  ppc_extract_struct_value_address (REGBUF)

#define USE_STRUCT_CONVENTION(GCCP, TYPE) \
  ppc_use_struct_convention (GCCP, TYPE)

/* Write into appropriate registers a function return value
   of type TYPE, given in virtual format.  */

#define STORE_RETURN_VALUE(TYPE,VALBUF) \
  {									\
    if (TYPE_CODE (TYPE) == TYPE_CODE_FLT)				\
									\
     /* Floating point values are returned starting from FPR1 and up.	\
	Say a double_double_double type could be returned in		\
	FPR1/FPR2/FPR3 triple. */					\
									\
      write_register_bytes (REGISTER_BYTE (FP_RV_REGNUM), (VALBUF),	\
						TYPE_LENGTH (TYPE));	\
    else								\
      /* Everything else is returned in GPR3 and up. */			\
      write_register_bytes (REGISTER_BYTE (RV_REGNUM), (VALBUF),	\
						TYPE_LENGTH (TYPE));	\
  }

/* FRAME_CHAIN takes a frame's nominal address
   and produces the frame's chain-pointer. */

#define FRAME_CHAIN(FRAME) ppc_frame_chain (FRAME)

#define FRAME_CHAIN_VALID(CHAIN, FRAME) ppc_frame_chain_valid (CHAIN, FRAME)

#define FRAME_CHAIN_COMBINE(CHAIN, FRAME) (CHAIN)

#define FRAMELESS_FUNCTION_INVOCATION(FRAME) ppc_frameless_function_invocation (FRAME)

/* used by blockframe.c to store frame information */

#define	EXTRA_FRAME_INFO	\
  CORE_ADDR initial_sp;			/* initial stack pointer. */ \
  struct frame_saved_regs *ppc_saved_regs; \
  struct ppc_function_boundaries *bounds; \
  struct ppc_function_properties *props;

#define PRINT_EXTRA_FRAME_INFO(FRAME) \
  ppc_print_extra_frame_info (FRAME)

#define INIT_FRAME_PC_FIRST(FROMLEAF, PREV) \
  ppc_init_frame_pc_first ((FROMLEAF), (PREV))

#define INIT_EXTRA_FRAME_INFO(FROMLEAF, FRAME) \
  ppc_init_extra_frame_info (FROMLEAF, FRAME)

#define INIT_FRAME_PC(FROMLEAF, PREV) \
  ppc_init_frame_pc (FROMLEAF, PREV)

#define FRAME_SAVED_PC(FRAME) \
  ppc_frame_saved_pc (FRAME)

#define FRAME_ARGS_ADDRESS(FRAME) \
  ppc_frame_cache_initial_stack_address (FRAME)

#define FRAME_LOCALS_ADDRESS(FRAME) FRAME_ARGS_ADDRESS(FRAME)

/* no way of knowing how many arguments are passed */

#define FRAME_NUM_ARGS(FRAME) -1

/* number of bytes at start of arglist that are not really args.  */

#define FRAME_ARGS_SKIP 0

#define FRAME_FIND_SAVED_REGS(FRAME, REGS) \
	ppc_frame_cache_saved_regs ((FRAME), &(REGS))

#define STACK_ALLOC(SP, NSP, LEN) ppc_stack_alloc (&(SP), &(NSP), 0, (LEN))

#define PUSH_DUMMY_FRAME ppc_push_dummy_frame ()

#define POP_FRAME \
  ppc_pop_frame ()

#include "tm-ppc-dummy.h"

#define FIX_CALL_DUMMY(DUMMYNAME, PC, FUN, NARGS, ARGS, TYPE, USING_GCC) \
  ppc_fix_call_dummy ((DUMMYNAME), (PC), (FUN), (NARGS), (TYPE))

#define CONVERT_FROM_FUNC_PTR_ADDR(ADDR) \
  ppc_convert_from_func_ptr_addr (ADDR)

#define SAVED_PC_AFTER_CALL(FRAME) \
  ppc_frame_saved_pc_after_call (FRAME)

#define INITIAL_RET_PC() SAVED_PC_AFTER_CALL (0)

#endif /* __TM_PPC_H__ */
