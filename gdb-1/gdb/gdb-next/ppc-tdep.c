#include "ppc-tdep.h"

#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "target.h"
#include "gdbcore.h"
#include "symfile.h"
#include "objfiles.h"
#include "gdbcmd.h"

#include "ppc-frameinfo.h"
#include "ppc-frameops.h"

#include <assert.h>

#ifndef CPU_TYPE_POWERPC
#define CPU_TYPE_POWERPC (18)
#endif

/* external functions and globals */

extern int print_insn_big_powerpc PARAMS ((bfd_vma, struct disassemble_info *));
extern struct obstack frame_cache_obstack;
extern int stop_stack_dummy;

/* local definitions */

static int ppc_debugflag = 0;

void ppc_debug (const char *fmt, ...)
{
  va_list ap;
  if (ppc_debugflag) {
    va_start (ap, fmt);
    vfprintf (stderr, fmt, ap);
    va_end (ap);
  }
}

/* Default offset from SP where the LR is stored */

#define	DEFAULT_LR_SAVE 8

/* function implementations */

void
ppc_init_extra_frame_info (fromleaf, frame)
     int fromleaf;
     struct frame_info *frame;
{
  assert (frame != NULL);
  frame->initial_sp = 0;
  frame->ppc_saved_regs = NULL;
  frame->bounds = NULL;
  frame->props = 0;
  frame->signal_handler_caller = 0;
}

void
ppc_print_extra_frame_info (frame)
     struct frame_info *frame;
{
  if (frame->signal_handler_caller) {
    printf_filtered (" This function was called from a signal handler.\n");
  } else {
    printf_filtered (" This function was not called from a signal handler.\n");
  }

  ppc_frame_cache_initial_stack_address (frame);
  if (frame->initial_sp) {
    printf_filtered (" The initial stack pointer for this frame was 0x%lx.\n", 
		     (unsigned long) frame->initial_sp);
  } else {
    printf_filtered (" Unable to determine initial stack pointer for this frame.\n");
  }    

  ppc_frame_cache_boundaries (frame, NULL);
  if (frame->bounds != NULL) {
    ppc_print_boundaries (frame->bounds);
  } else {
    printf_filtered (" Unable to determine function boundary information.\n");
  }

  ppc_frame_cache_properties (frame, NULL);
  if (frame->props != NULL) {
    ppc_print_properties (frame->props);
  } else {
    printf_filtered (" Unable to determine function property information.\n");
  }
}

void
ppc_init_frame_pc_first (fromleaf, frame)
     int fromleaf;
     struct frame_info *frame;
{
  struct frame_info *next;

  assert (frame != NULL);
  next = get_next_frame (frame);
  assert (next != NULL);
  frame->pc = ppc_frame_saved_pc (next);
}

void
ppc_init_frame_pc (fromleaf, frame)
     int fromleaf;
     struct frame_info *frame;
{
  assert (frame != NULL);
}

CORE_ADDR
ppc_frame_saved_pc (frame)
     struct frame_info *frame;
{
  CORE_ADDR prev;

  assert (frame != NULL);

  if (frame->signal_handler_caller) {
    CORE_ADDR psp = read_memory_unsigned_integer (frame->frame, 4);
    /* psp is the top of the signal handler data pushed by the kernel */
    /* 0x220 is offset to SIGCONTEXT; 0x10 is offset to $pc */
    ppc_debug ("ppc_frame_saved_pc: determing previous pc from signal context\n");
    return read_memory_unsigned_integer (psp + 0x220 + 0x10, 4);
  }

  prev = ppc_frame_chain (frame);
  if ((prev == 0) || (! ppc_frame_chain_valid (prev, frame))) { 
    ppc_debug ("ppc_frame_saved_pc: previous stack frame not valid: returning 0\n");
    return 0; 
  }
  ppc_debug ("ppc_frame_saved_pc: value of prev is 0x%lx\n", (unsigned long) prev);

  if (ppc_frame_cache_properties (frame, NULL) != 0) {
    ppc_debug ("ppc_frame_saved_pc: unable to find properties of function containing 0x%lx\n",
	       (unsigned long) frame->pc);
    ppc_debug ("ppc_frame_saved_pc: assuming link register saved in normal location\n");
    if (ppc_frameless_function_invocation (frame)) {
      return read_register (LR_REGNUM);
    } else {
      return read_memory_unsigned_integer (prev + DEFAULT_LR_SAVE, 4);
    }
  }
  assert (frame->props != NULL);
  
  if (frame->props->lr_saved) {
    return read_memory_unsigned_integer (prev + frame->props->lr_offset, 4);
  } else {
    ppc_debug ("ppc_frame_saved_pc: function did not save link register\n");
  }

  if (frame->next == NULL) {
    ppc_debug ("ppc_frame_saved_pc: function is leaf; link register should be current\n");
    return read_register (LR_REGNUM);
  }

  if ((frame->next != NULL) 
      && frame->next->signal_handler_caller) {
    ppc_debug ("ppc_frame_saved_pc: using link area from signal handler\n");
    return read_memory_unsigned_integer (frame->frame - 0x320 + DEFAULT_LR_SAVE, 4);
  }

  if ((frame->next != NULL) && (ppc_is_dummy_frame (frame->next))) {
    ppc_debug ("ppc_frame_saved_pc: using link area from call dummy\n");
    return read_memory_unsigned_integer (frame->frame - 0x1c, 4);
  }

  assert (frame->next != NULL);

  ppc_debug ("ppc_frame_saved_pc: function is not a leaf\n");
  ppc_debug ("ppc_frame_saved_pc: assuming link register saved in normal location\n");
  return read_memory_unsigned_integer (prev + DEFAULT_LR_SAVE, 4);
}

CORE_ADDR
ppc_frame_saved_pc_after_call (frame)
     struct frame_info *frame;
{
  assert (frame != NULL);
  return ppc_frame_saved_pc (frame);
}

CORE_ADDR
ppc_frame_chain (frame)
     struct frame_info *frame;
{
  CORE_ADDR psp = read_memory_unsigned_integer (frame->frame, 4);

  if (frame->signal_handler_caller) {
    /* psp is the top of the signal handler data pushed by the kernel */
    /* 0x70 is offset to PPC_SAVED_STATE; 0xc is offset to $r1 */
    return read_memory_unsigned_integer (psp + 0x70 + 0xc, 4);
  }

  /* If a frameless function is interrupted by a signal, no change to
     the stack pointer */
  if (frame->next != NULL
      && frame->next->signal_handler_caller
      && ppc_frameless_function_invocation (frame)) {
    return frame->frame;
  }

  return psp;
}

int 
ppc_frame_chain_valid (chain, frame)
     CORE_ADDR chain;
     struct frame_info *frame;
{
  if (chain == 0) { return 0; }

  /* reached end of stack? */
  if (read_memory_unsigned_integer (chain, 4) == 0) { return 0; }

#if 0
  if (inside_entry_func (frame->pc)) { return 0; }
  if (inside_main_func (frame->pc)) { return 0; }
#endif

  /* check for bogus stack frames */
  if (! (chain >= frame->frame)) {
    warning ("ppc_frame_chain_valid: stack pointer from 0x%lx to 0x%lx "
	     "grows upward; assuming invalid\n",
	     (unsigned long) frame->frame, (unsigned long) chain);
    return 0;
  }
  if ((chain - frame->frame) > 65536) {
    warning ("ppc_frame_chain_valid: stack frame from 0x%lx to 0x%lx "
	     "larger than 65536 bytes; assuming invalid",
	     (unsigned long) frame->frame, (unsigned long) chain);
    return 0;
  }

  return 1;
}

int
ppc_is_dummy_frame (frame)
     struct frame_info *frame;
{
  /* using get_prev_frame or ppc_frame_chain 
     would cause infinite recursion in some cases */

  CORE_ADDR chain = read_memory_unsigned_integer (frame->frame, 4);

  if (frame->signal_handler_caller) {
    /* psp is the top of the signal handler data pushed by the kernel */
    /* 0x70 is offset to PPC_SAVED_STATE; 0xc is offset to $r1 */
    chain = read_memory_unsigned_integer (chain + 0x70 + 0xc, 4);
  }
    
  if (chain == 0) { return 0; }
  return (PC_IN_CALL_DUMMY (frame->pc, frame->frame, chain));
}

/* Return the address of a frame. This is the inital %sp value when the frame
   was first allocated. For functions calling alloca(), it might be saved in
   an alloca register. */

CORE_ADDR
ppc_frame_cache_initial_stack_address (frame)
     struct frame_info *frame;
{
  assert (frame != NULL);
  if (frame->initial_sp == 0) { 
    frame->initial_sp = ppc_frame_initial_stack_address (frame);
  }
  return frame->initial_sp;
}

CORE_ADDR
ppc_frame_initial_stack_address (frame)
     struct frame_info *frame;
{
  CORE_ADDR tmpaddr;
  struct frame_info *callee;

  /* Find out if this function is using an alloca register. */

  if (ppc_frame_cache_properties (frame, NULL) != 0) {
    ppc_debug ("ppc_frame_initial_stack_address: unable to find properties of " 
		 "function containing 0x%lx\n", frame->pc);
    return 0;
  }

  /* Read and cache saved registers if necessary. */

  ppc_frame_cache_saved_regs (frame, NULL);

  /* If no alloca register is used, then frame->frame is the value of
     %sp for this frame, and it is valid. */

  if (frame->props->alloca_reg < 0) {
    frame->initial_sp = frame->frame;
    return frame->initial_sp;
  }

  /* This function has an alloca register. If this is the top-most frame
     (with the lowest address), the value in alloca register is valid. */

  if (! get_next_frame (frame)) {
    frame->initial_sp = read_register (frame->props->alloca_reg);     
    return frame->initial_sp;
  }

  /* Otherwise, this is a caller frame. Callee has usually already saved
     registers, but there are exceptions (such as when the callee
     has no parameters). Find the address in which caller's alloca
     register is saved. */

  for (callee = get_next_frame (frame); callee != NULL; callee = get_next_frame (callee)) {

    ppc_frame_cache_saved_regs (callee, NULL);

    /* this is the address in which alloca register is saved. */

    tmpaddr = callee->ppc_saved_regs->regs [frame->props->alloca_reg];
    if (tmpaddr) {
      frame->initial_sp = read_memory_unsigned_integer (tmpaddr, 4); 
      return frame->initial_sp;
    }

    /* Go look into deeper levels of the frame chain to see if any one of
       the callees has saved alloca register. */
  }

  /* If alloca register was not saved, by the callee (or any of its callees)
     then the value in the register is still good. */

  frame->initial_sp = read_register (frame->props->alloca_reg);     
  return frame->initial_sp;
}

int
ppc_is_magic_function_pointer (addr)
     CORE_ADDR addr;
{
  return 0;
}

/* Usually a function pointer's representation is simply the address of
   the function. On the RS/6000 however, a function pointer is represented
   by a pointer to a TOC entry. This TOC entry contains three words,
   the first word is the address of the function, the second word is the
   TOC pointer (r2), and the third word is the static chain value.
   Throughout GDB it is currently assumed that a function pointer contains
   the address of the function, which is not easy to fix.
   In addition, the conversion of a function address to a function
   pointer would require allocation of a TOC entry in the inferior's
   memory space, with all its drawbacks.
   To be able to call C++ virtual methods in the inferior (which are called
   via function pointers), find_function_addr uses this macro to
   get the function address from a function pointer.  */

CORE_ADDR
ppc_convert_from_func_ptr_addr (addr)
     CORE_ADDR addr;
{
  return (ppc_is_magic_function_pointer (addr) ? read_memory_unsigned_integer (addr, 4) : (addr));
}

CORE_ADDR
ppc_find_toc_address (pc)
     CORE_ADDR pc;
{
  return 0;
}

int ppc_use_struct_convention (gccp, valtype)
     int gccp;
     struct type *valtype;
{
  return 1;
}

CORE_ADDR
ppc_extract_struct_value_address (regbuf)
  char regbuf[REGISTER_BYTES];
{
  return extract_unsigned_integer (&regbuf[REGISTER_BYTE (GP0_REGNUM + 3)],
				   sizeof (CORE_ADDR));
}

void
ppc_extract_return_value (valtype, regbuf, valbuf)
     struct type *valtype;
     char regbuf[REGISTER_BYTES];
     char *valbuf;
{
  int offset = 0;

  if (TYPE_CODE (valtype) == TYPE_CODE_FLT) {

    /* floats and doubles are returned in fpr1. fpr's have a size of 8 bytes.
       We need to truncate the return value into float size (4 byte) if
       necessary. */

    double dd;
    float ff;

    switch (TYPE_LENGTH (valtype)) {
    case 8: /* double */
      memcpy (valbuf, &regbuf[REGISTER_BYTE (FP0_REGNUM + 1)], 8);
      break;
    case 4:
      memcpy (&dd, &regbuf[REGISTER_BYTE (FP0_REGNUM + 1)], 8);
      ff = (float) dd;
      memcpy (valbuf, &ff, sizeof (float));
      break;
    default:
      error ("unknown TYPE_LENGTH for return type %d", TYPE_LENGTH (valtype));
    }

  } else {

    unsigned int gpretreg = GP0_REGNUM + 3;
    /* return value is copied starting from r3. */
    if (TARGET_BYTE_ORDER == BIG_ENDIAN
	&& TYPE_LENGTH (valtype) < REGISTER_RAW_SIZE (gpretreg))
      offset = REGISTER_RAW_SIZE (gpretreg) - TYPE_LENGTH (valtype);
    
    memcpy (valbuf, regbuf + REGISTER_BYTE (gpretreg) + offset,
	    TYPE_LENGTH (valtype));
  }
}

CORE_ADDR 
ppc_skip_prologue (pc)
     CORE_ADDR pc;
{
  ppc_function_boundaries_request request;
  ppc_function_boundaries bounds;
  int ret;
  
  ppc_clear_function_boundaries_request (&request);
  ppc_clear_function_boundaries (&bounds);

  request.prologue_start = pc;
  ret = ppc_find_function_boundaries (&request, &bounds);
  if (ret != 0) { return 0; }

  return bounds.body_start;
}

/* Determines whether the current frame has allocated a frame on the stack or not.  */

int
ppc_frameless_function_invocation (frame)
     struct frame_info *frame;
{
  if (ppc_frame_cache_properties (frame, NULL) != 0) {
    ppc_debug ("frameless_function_invocation: unable to find properties of " 
		 "function containing 0x%lx; assuming not frameless\n", frame->pc);
    return 0;
  }

  /* if not a leaf, it's not frameless --- ignore result of ppc_frame_cache_properties */
  /* (unless it was interrupted by a signal or a call_dummy) */

  if (frame->next != NULL 
      && !frame->next->signal_handler_caller
      && !ppc_is_dummy_frame (frame->next))
    { return 0; }

  return frame->props->frameless;
}

int ppc_invalid_float (f, len)
     char *f;
     size_t len;
{
  return 0;
}

void
_initialize_ppc_tdep ()
{
  struct cmd_list_element *cmd = NULL;

  tm_print_insn = print_insn_big_powerpc;

  cmd = add_set_cmd ("debug-ppc", class_obscure, var_boolean, 
		     (char *) &ppc_debugflag,
		     "Set if printing PPC stack analysis debugging statements.",
		     &setlist),
  add_show_from_set (cmd, &showlist);		
}
