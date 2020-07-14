#include "defs.h"
#include "target.h"

/* The following functions are defined for the benefit of inftarg.c;
   they should never be called */

int attach (int pid)
{ 
  fatal_dump_core ("nextstep_nat_inferior: unexpected call to attach ()");
  return 0;
}

void detach (int sig) 
{ 
  fatal_dump_core ("nextstep_nat_inferior: unexpected call to detach ()"); 
}

void kill_inferior ()
{ 
  fatal_dump_core ("nextstep_nat_inferior: unexpected call to kill_inferior ()"); 
}

void child_resume (int pid, int step, enum target_signal sig)
{
  fatal_dump_core ("nextstep_nat_inferior: unexpected call to child_resume ()");
}

int child_wait (int pid, struct target_waitstatus *status)
{
  fatal_dump_core ("nextstep_nat_inferior: unexpected call to child_wait ()");
}

int child_xfer_memory
 (CORE_ADDR memaddr, char *myaddr, int len, int write, struct target_ops *target)
{
  fatal_dump_core ("nextstep_nat_inferior: unexpected call to child_xfer_memory ()");
}
