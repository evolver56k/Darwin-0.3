#define IS_GP_REGNUM(regno) ((regno >= FIRST_GP_REGNUM) && (regno <= LAST_GP_REGNUM))
#define IS_FP_REGNUM(regno) ((regno >= FIRST_FP_REGNUM) && (regno <= LAST_FP_REGNUM))
#define IS_SP_REGNUM(regno) ((regno >= FIRST_SP_REGNUM) && (regno <= LAST_SP_REGNUM))

#define FIRST_GP_REGNUM 0
#define LAST_GP_REGNUM 31
#define NUM_GP_REGS (LAST_GP_REGNUM - FIRST_GP_REGNUM + 1)

#define FIRST_FP_REGNUM 32
#define LAST_FP_REGNUM 63
#define NUM_FP_REGS (LAST_FP_REGNUM - FIRST_FP_REGNUM + 1)

#define	FIRST_SP_REGNUM 64	/* first special register number */
#define LAST_SP_REGNUM  70	/* last special register number */
#define NUM_SP_REGS (LAST_SP_REGNUM - FIRST_SP_REGNUM + 1)

#include "ppc-thread-status.h"

void ppc_next_fetch_sp_registers PARAMS ((unsigned char *rdata, gdb_ppc_thread_state_t *gp_regs));
void ppc_next_store_sp_registers PARAMS ((unsigned char *rdata, gdb_ppc_thread_state_t *gp_regs));
void ppc_next_fetch_gp_registers PARAMS ((unsigned char *rdata, gdb_ppc_thread_state_t *gp_regs));
void ppc_next_store_gp_registers PARAMS ((unsigned char *rdata, gdb_ppc_thread_state_t *gp_regs));
void ppc_next_fetch_fp_registers PARAMS ((unsigned char *rdata, gdb_ppc_thread_fpstate_t *fp_regs));
void ppc_next_store_fp_registers PARAMS ((unsigned char *rdata, gdb_ppc_thread_fpstate_t *fp_regs));
CORE_ADDR ppc_next_skip_trampoline_code PARAMS ((CORE_ADDR pc));
int ppc_next_in_solib_return_trampoline PARAMS ((CORE_ADDR pc, char *name));
int ppc_next_in_solib_call_trampoline PARAMS ((CORE_ADDR pc, char *name));
