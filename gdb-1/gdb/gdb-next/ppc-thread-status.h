#ifndef _GDB_MACH_PPC_THREAD_STATUS_H_
#define _GDB_MACH_PPC_THREAD_STATUS_H_

#define GDB_PPC_THREAD_STATE 1
#define GDB_PPC_THREAD_FPSTATE 2

struct gdb_ppc_thread_state {

  unsigned int srr0;		/* program counter */
  unsigned int srr1;		/* machine state register */

  unsigned int gpregs[32];

  unsigned int cr;		/* condition register */
  unsigned int xer;		/* integer exception register */
  unsigned int lr;		/* link register */
  unsigned int ctr;
  unsigned int mq;

  unsigned int pad;
};

typedef struct gdb_ppc_thread_state gdb_ppc_thread_state_t;

struct gdb_ppc_float_state {
  double fpregs[32];
  unsigned int fpscr_pad;	/* fpscr is 64 bits; first 32 are unused */
  unsigned int fpscr;		/* floating point status register */
};

typedef struct gdb_ppc_float_state gdb_ppc_thread_fpstate_t;

#define GDB_PPC_THREAD_STATE_COUNT \
  (sizeof (gdb_ppc_thread_state_t) / sizeof (unsigned int))

#define GDB_PPC_THREAD_FPSTATE_COUNT \
  (sizeof (gdb_ppc_thread_fpstate_t) / sizeof (unsigned int))

#endif /* _GDB_MACH_PPC_THREAD_STATUS_H_ */
