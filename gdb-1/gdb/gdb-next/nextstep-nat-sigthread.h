#ifndef _GDB_NEXTSTEP_NAT_SIGTHREAD_H_
#define _GDB_NEXTSTEP_NAT_SIGTHREAD_H_

#include "defs.h"
#include "nextstep-nat-mutils.h"
#include "nextstep-nat-threads.h"

#include <sys/wait.h>

typedef int WAITSTATUS;

#if defined (__MACH30__)
struct next_signal_thread_message
{
  mach_msg_header_t header;
  int pid;
  WAITSTATUS status;
};
#else
struct next_signal_thread_message
{
  msg_header_t header;
  msg_type_t type;
  int pid;
  WAITSTATUS status;
};
#endif /* __MACH30__ */

struct next_signal_thread_status
{
  gdb_thread_t signal_thread;
  mach_port_t signal_port;
  int inferior_pid;
};

typedef struct next_signal_thread_message next_signal_thread_message;
typedef struct next_signal_thread_status next_signal_thread_status;

void next_signal_thread_debug PARAMS ((FILE *f, struct next_signal_thread_status *s));
void next_signal_thread_debug_status PARAMS ((FILE *f, WAITSTATUS status));

void next_signal_thread_init PARAMS ((next_signal_thread_status *s));

void next_signal_thread_create PARAMS ((next_signal_thread_status *s, mach_port_t signal_port, int pid));
void next_signal_thread_destroy PARAMS ((next_signal_thread_status *s));

#endif /* _GDB_NEXTSTEP_NAT_SIGTHREAD_H_*/
