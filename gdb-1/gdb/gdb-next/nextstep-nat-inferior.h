#ifndef _NEXTSTEP_NAT_INFERIOR_H_
#define _NEXTSTEP_NAT_INFERIOR_H_

#include "defs.h"
#include "target.h"

#include "nextstep-nat-sigthread.h"
#include "nextstep-nat-dyld.h"
#include "nextstep-nat-threads.h"

#if defined (__MACH30__)
typedef struct mach_exception_info
{
  exception_mask_t masks[EXC_TYPES_COUNT];
  mach_port_t ports[EXC_TYPES_COUNT];
  exception_behavior_t behaviors[EXC_TYPES_COUNT];
  thread_state_flavor_t flavors[EXC_TYPES_COUNT];
  mach_msg_type_number_t count;
} mach_exception_info;
#endif

typedef struct next_cfm_info
{
  void *info_api_cookie;
  mach_port_t cfm_receive_right;
  mach_port_t cfm_send_right;
} next_cfm_info;

typedef struct next_thread_entry
{
  struct next_thread_entry *next;
  int pid;
  thread_t thread;
  int id;
} thread_process_id;

typedef struct next_inferior_status
{

  int pid;
  task_t task;

  int attached_in_ptrace;
  int stopped_in_ptrace;

  unsigned int suspend_count;

  thread_t last_thread;

  struct next_thread_entry *thread_list;

  next_signal_thread_status signal_status;
  mach_port_t signal_port;

  next_dyld_thread_status dyld_status;
  mach_port_t dyld_port;

  next_cfm_info	cfm_info;

  mach_port_t notify_port;
  mach_port_t inferior_exception_port;

#if defined (__MACH30__)
  mach_exception_info exception;
#endif

#if !defined (__MACH30__)
  mach_port_t inferior_old_exception_port;
#endif

  mach_port_t old_exception_reply_port;

} next_inferior_status;

#if !defined (__MACH30__)
typedef struct next_exception_data
{
  msg_header_t header;
  msg_type_t thread_type;
  thread_t thread;
  msg_type_t task_type;
  task_t task;
  msg_type_t exception_type;
  int exception;
  msg_type_t code_type;
  int code;
  msg_type_t sub_code_type;
  int subcode;
} next_exception_data;

typedef struct next_exception_reply
{
  msg_header_t header;
  msg_type_t retcode_type;
  kern_return_t retcode;
} next_exception_reply;
#endif

void next_mach_check_new_threads ();
int next_wait (struct next_inferior_status *inferior, struct target_waitstatus *status);

extern int inferior_bind_exception_port_flag;
extern int inferior_bind_notify_port_flag;
extern int inferior_handle_exceptions_flag;

/* from rhapsody-nat.c and macosx-nat.c */

void next_handle_exception
  PARAMS ((struct next_inferior_status *inferior, msg_header_t *msg, struct target_waitstatus *status));

void next_create_inferior_for_task
  PARAMS ((struct next_inferior_status *inferior, task_t task, int pid));

#endif /* _NEXTSTEP_NAT_INFERIOR_H_ */
