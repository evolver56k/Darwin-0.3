#ifndef _NEXTSTEP_NAT_DYLD_H_
#define _NEXTSTEP_NAT_DYLD_H_

#include "defs.h"
#include "nextstep-nat-mutils.h"
#include "nextstep-nat-threads.h"

#include <mach-o/dyld_debug.h>

struct objfile;
struct target_waitstatus;

struct next_inferior_status;

struct dyld_objfile_entry;

#include "nextstep-nat-dyld-info.h"
#include "nextstep-nat-dyld-path.h"

typedef struct next_dyld_thread_status {

  gdb_thread_t thread;

  mach_port_t port;
  int start_error;
  enum { dyld_clear, dyld_initialized, dyld_started } state;

  struct _dyld_event_message_request *requests;
  unsigned int max_requests;
  unsigned int start_request;
  unsigned int end_request;

  struct dyld_objfile_info current_info;
  struct dyld_path_info path_info;

} next_dyld_thread_status;

void dyld_debug PARAMS ((const char *fmt, ...));

const char *dyld_debug_error_string
PARAMS ((enum dyld_debug_return ret));

void dyld_print_status_info
PARAMS ((struct next_dyld_thread_status *s));

void next_dyld_thread_destroy
PARAMS ((struct next_inferior_status *ns,
	 struct next_dyld_thread_status *s));

void next_dyld_thread_init
PARAMS ((next_dyld_thread_status *s));

int next_mach_start_dyld
PARAMS ((struct next_inferior_status *s));

void next_clear_start_breakpoint
PARAMS (());

void next_set_start_breakpoint
PARAMS ((bfd *exec_bfd));

void next_mach_try_start_dyld
PARAMS (());

void next_mach_add_shared_symbol_files
PARAMS (());

void next_init_dyld
PARAMS ((struct next_inferior_status *s, bfd *sym_bfd));

void next_init_dyld_symfile
PARAMS ((bfd *sym_bfd));

kern_return_t next_mach_process_dyld_messages
PARAMS ((struct target_waitstatus *status, struct _dyld_event_message_request* initial_message));

#endif /* _NEXTSTEP_NAT_DYLD_H_ */
