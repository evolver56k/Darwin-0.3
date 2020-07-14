#include "defs.h"
#include "frame.h"
#include "inferior.h"
#include "symtab.h"
#include "target.h"
#include "gdbcore.h"
#include "symfile.h"
#include "objfiles.h"

#include "ppc-next-tdep.h"

void ppc_next_fetch_sp_registers (unsigned char *rdata, gdb_ppc_thread_state_t *gp_regs)
{
  store_unsigned_integer (rdata + (REGISTER_BYTE (PC_REGNUM)), sizeof (REGISTER_TYPE), gp_regs->srr0);
  store_unsigned_integer (rdata + (REGISTER_BYTE (PS_REGNUM)), sizeof (REGISTER_TYPE), gp_regs->srr1);
  store_unsigned_integer (rdata + (REGISTER_BYTE (CR_REGNUM)), sizeof (REGISTER_TYPE), gp_regs->cr);
  store_unsigned_integer (rdata + (REGISTER_BYTE (LR_REGNUM)), sizeof (REGISTER_TYPE), gp_regs->lr);
  store_unsigned_integer (rdata + (REGISTER_BYTE (CTR_REGNUM)), sizeof (REGISTER_TYPE), gp_regs->ctr);
  store_unsigned_integer (rdata + (REGISTER_BYTE (XER_REGNUM)), sizeof (REGISTER_TYPE), gp_regs->xer);
  store_unsigned_integer (rdata + (REGISTER_BYTE (MQ_REGNUM)), sizeof (REGISTER_TYPE), gp_regs->mq);
}

void ppc_next_store_sp_registers (unsigned char *rdata, gdb_ppc_thread_state_t *gp_regs)
{
  gp_regs->srr0 = extract_unsigned_integer (rdata + (REGISTER_BYTE (PC_REGNUM)), sizeof (REGISTER_TYPE));
  gp_regs->srr1 = extract_unsigned_integer (rdata + (REGISTER_BYTE (PS_REGNUM)), sizeof (REGISTER_TYPE));
  gp_regs->cr = extract_unsigned_integer (rdata + (REGISTER_BYTE (CR_REGNUM)), sizeof (REGISTER_TYPE));
  gp_regs->lr = extract_unsigned_integer (rdata + (REGISTER_BYTE (LR_REGNUM)), sizeof (REGISTER_TYPE));
  gp_regs->ctr = extract_unsigned_integer (rdata + (REGISTER_BYTE (CTR_REGNUM)), sizeof (REGISTER_TYPE));
  gp_regs->xer = extract_unsigned_integer (rdata + (REGISTER_BYTE (XER_REGNUM)), sizeof (REGISTER_TYPE));
  gp_regs->mq = extract_unsigned_integer (rdata + (REGISTER_BYTE (MQ_REGNUM)), sizeof (REGISTER_TYPE));
}

void ppc_next_fetch_gp_registers (unsigned char *rdata, gdb_ppc_thread_state_t *gp_regs)
{
  int i;
  for (i = 0; i < NUM_GP_REGS; i++) {
    store_unsigned_integer (rdata + (REGISTER_BYTE (FIRST_GP_REGNUM + i)), 
			    sizeof (REGISTER_TYPE), 
			    gp_regs->gpregs[i]);
  }
}

void ppc_next_store_gp_registers (unsigned char *rdata, gdb_ppc_thread_state_t *gp_regs)
{
  int i;
  for (i = 0; i < NUM_GP_REGS; i++) {
    gp_regs->gpregs[i] = extract_unsigned_integer (rdata + (REGISTER_BYTE (FIRST_GP_REGNUM + i)),
						   sizeof (REGISTER_TYPE));
  }
}

void ppc_next_fetch_fp_registers (unsigned char *rdata, gdb_ppc_thread_fpstate_t *fp_regs)
{
  int i;
  FP_REGISTER_TYPE *fpr = fp_regs->fpregs;
  for (i = 0; i < NUM_FP_REGS; i++) {
    store_floating (rdata + (REGISTER_BYTE (FIRST_FP_REGNUM + i)),
		    sizeof (FP_REGISTER_TYPE), fpr[i]);
  }
}
  
void ppc_next_store_fp_registers (unsigned char *rdata, gdb_ppc_thread_fpstate_t *fp_regs)
{
  int i;
  FP_REGISTER_TYPE *fpr = fp_regs->fpregs;
  for (i = 0; i < NUM_FP_REGS; i++) {
    fpr[i] = extract_floating (rdata + (REGISTER_BYTE (FIRST_FP_REGNUM + i)), 
			       sizeof (FP_REGISTER_TYPE));
  }

  memcpy ((char *) &(fp_regs->fpregs),
	  rdata + (REGISTER_BYTE (FIRST_FP_REGNUM)),
	  NUM_FP_REGS * sizeof (FP_REGISTER_TYPE));
}

/* mread -- read memory (unsigned) and apply a bitmask */

static unsigned long mread (addr, len, mask)
     CORE_ADDR addr;
     unsigned long len, mask;
{
  long ret = read_memory_unsigned_integer (addr, len);
  if (mask) { ret &= mask; }
  return ret;
}

static short ext16 (unsigned long n)
{
  if (n > 32767) {
    return (short) (n - 65536);
  } else {
    return (short) n;
  }
}

static long ext32 (unsigned long n)
{
  return n;
}

CORE_ADDR
ppc_next_skip_trampoline_code (pc)
     CORE_ADDR pc;
{
  CORE_ADDR curpc;
  CORE_ADDR memaddr;

  long hiaddr;
  short loaddr;
  short loaddr2;

  curpc = pc;

#if 0
  /* if not branch, no trampoline */
  if ((curins & 0xfc000003) != 0x48000001) { return pc; } /* bl */

  /* skip to branch target */
  curpc += (curins & 0x03fffffc);
  curins = read_memory_unsigned_integer (curpc, 4);
#endif

  if ((mread (curpc, 4, 0) != 0x7c0802a6)                     /* mflr  r0 */
      || (mread (curpc + 4, 4, 0) != 0x48000005)              /* bl    $pc + 4 */
      || (mread (curpc + 8, 4, 0) != 0x7d6802a6)              /* mflr  r11 */
      || (mread (curpc + 12, 4, 0) != 0x7c0803a6)             /* mtlr  r0 */
      || (mread (curpc + 16, 4, 0xffff0000) != 0x3d6b0000)    /* addis r11, r11, N1 */
      || (mread (curpc + 20, 4, 0xffff0000) != 0x818b0000)    /* lwz   r12, N2(r11) */
      || (mread (curpc + 24, 4, 0) != 0x7d8903a6)             /* mtctr r12 */
      || (mread (curpc + 28, 4, 0xffff0000) != 0x396b0000)    /* addi  r11, r11, N3 */
      || (mread (curpc + 32, 4, 0) != 0x4e800420)) {          /* bctr */
    return pc;
  }

  hiaddr = ext32 (mread (curpc + 16, 4, 0x0000ffff) << 16);
  loaddr = ext16 (mread (curpc + 20, 4, 0x0000ffff));
  loaddr2 = ext16 (mread (curpc + 28, 4, 0x0000ffff));
  
  if (loaddr != loaddr2) {
    fprintf (stderr, "hiaddr = 0x%lx, loaddr = 0x%x, loaddr2 = 0x%x\n", 
	     (unsigned long) hiaddr, (unsigned short) loaddr, (unsigned short) loaddr2);
    return pc;
  }

  /* read from computed address */
  curpc = mread ((curpc + 8) + hiaddr + loaddr, 4, 0);
  
  return curpc;
}

int ppc_next_in_solib_return_trampoline (CORE_ADDR pc, char *name)
{
  return 0;
}

int ppc_next_in_solib_call_trampoline (CORE_ADDR pc, char *name)
{
  if (ppc_next_skip_trampoline_code (pc) != pc) { return 1; }
  return 0;
}
