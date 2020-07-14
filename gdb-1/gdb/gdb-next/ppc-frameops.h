#ifndef __PPC_FRAMEOPS_H__
#define __PPC_FRAMEOPS_H__

#include "defs.h"

struct frame_info;
struct frame_saved_regs;
struct value;

void ppc_push_dummy_frame PARAMS (());

void ppc_pop_frame PARAMS (());

void ppc_fix_call_dummy PARAMS 
  ((char *dummyname, CORE_ADDR pc, CORE_ADDR fun, int nargs, int type));

CORE_ADDR ppc_push_arguments PARAMS 
  ((int nargs, struct value **args, CORE_ADDR sp, 
    int struct_return, CORE_ADDR struct_addr));

void ppc_stack_alloc PARAMS ((CORE_ADDR *sp, CORE_ADDR *start, size_t argsize, size_t len));

void ppc_frame_cache_saved_regs PARAMS ((struct frame_info *frame, struct frame_saved_regs *retregs));
void ppc_frame_saved_regs PARAMS ((struct frame_info *frame, struct frame_saved_regs *retregs));

#endif /* __PPC_FRAMEOPS_H__ */
