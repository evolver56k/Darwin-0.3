#include "nextstep-nat-dyld.h"
#include "nextstep-nat-dyld-path.h"
#include "nextstep-nat-inferior.h"
#include "nextstep-nat-mutils.h"

#include "defs.h"
#include "inferior.h"
#include "target.h"
#include "gdbcmd.h"
#include "annotate.h"

#include <assert.h>
#include <unistd.h>

bfd *sym_bfd;

#include "mach-o.h"

#include "nextstep-nat-dyld.h"
#include "nextstep-nat-dyld-info.h"
#include "nextstep-nat-dyld-path.h"
#include "nextstep-nat-dyld-process.h"

#include "gdbcore.h"		/* for core_ops */

extern struct target_ops exec_ops;
extern int inferior_start_dyld_flag;

extern next_inferior_status *next_status;

#ifndef USE_DYLD_EVENT_SERVER
#if defined (__MACH30__)
#define USE_DYLD_EVENT_SERVER 1
#else
#define USE_DYLD_EVENT_SERVER 0
#endif
#endif /* USE_DYLD_EVENT_SERVER */

#define DYLD_THREAD_FATAL(ret) \
{ \
  dyld_debug_re ("error on line %u of \"%s\" in function \"%s\": %s\n", \
		 __LINE__, __FILE__, __MACH_CHECK_FUNCTION, mach_error_string (ret)); \
  abort (); \
}

static FILE *dyld_stderr = NULL;
static FILE *dyld_stderr_re = NULL;
static int dyld_debug_flag = 0;

static int dyld_drain_events_flag = 0;
static int dyld_preload_libraries_flag = 1;

#if WITH_CFM
extern int inferior_auto_start_cfm_flag;
#endif /* WITH_CFM */

void dyld_debug (const char *fmt, ...)
{
  va_list ap;
  if (dyld_debug_flag) {
    va_start (ap, fmt);
    fprintf (dyld_stderr, "[%d dyld]: ", getpid ());
    vfprintf (dyld_stderr, fmt, ap);
    va_end (ap);
    fflush (dyld_stderr);
  }
}

/* A re-entrant version for use by the dyld handling thread */

static void dyld_debug_re (const char *fmt, ...)
{
  va_list ap;
  if (dyld_debug_flag) {
    va_start (ap, fmt);
    fprintf (dyld_stderr_re, "[%d dyld]: ", getpid ());
    vfprintf (dyld_stderr_re, fmt, ap);
    va_end (ap);
    fflush (dyld_stderr_re);
  }
}

const char *dyld_debug_error_string (enum dyld_debug_return ret)
{
  switch (ret) {
  case DYLD_SUCCESS: return "success";
  case DYLD_INCONSISTENT_DATA: return "inconsistent data";
  case DYLD_INVALID_ARGUMENTS: return "invalid arguments";
  case DYLD_FAILURE: return "general failure";
  default: return "[UNKNOWN]";
  }  
}

static void debug_dyld_event_request(struct _dyld_event_message_request* request)
{
    next_debug_message(&request->head);
    dyld_debug_re ("               type: 0x%lx\n", (long) request->event.type);
    dyld_debug_re ("arg[0].vmaddr_slide: 0x%lx\n", (long) request->event.arg[0].vmaddr_slide);
    dyld_debug_re ("arg[0].module_index: 0x%lx\n", (long) request->event.arg[0].module_index);
    dyld_debug_re ("arg[1].vmaddr_slide: 0x%lx\n", (long) request->event.arg[1].vmaddr_slide);
    dyld_debug_re ("arg[1].module_index: 0x%lx\n", (long) request->event.arg[1].module_index);
}

void dyld_print_status_info
(struct next_dyld_thread_status *s) 
{
  switch (s->state) {
  case dyld_clear:
    printf_filtered ("The DYLD shared library state has not yet been initialized.\n"); 
    break;
  case dyld_initialized:
    printf_filtered 
      ("The DYLD shared library state has been initialized from the "
       "executable's shared library information.  All symbols should be "
       "present, but the addresses of some symbols may move when the program "
       "is executed, as DYLD may relocate library load addresses if "
       "necessary.\n");
    break;
  case dyld_started:
    printf_filtered ("DYLD shared library information has been read from the DYLD debugging thread.\n");
    break;
  default:
    fatal_dump_core ("invalid value for s->dyld_state");
    break;
  }

  if (s->start_error) {
    printf_filtered ("\nAn error occurred trying to start the DYLD debug thread.\n");
  }

  dyld_print_shlib_info (&s->current_info);
}

void next_dyld_thread_destroy (struct next_inferior_status *ns, struct next_dyld_thread_status *s)
{
#if 0
  struct dyld_objfile_info saved_info, new_info;

  dyld_objfile_info_init (&saved_info);
  dyld_objfile_info_init (&new_info);

  dyld_objfile_info_copy (&saved_info, &s->current_info);
  dyld_objfile_info_free (&s->current_info);

  dyld_update_shlibs (ns, &ns->dyld_status.path_info, &saved_info, &new_info, &s->current_info);

  dyld_objfile_info_free (&saved_info);
  dyld_objfile_info_free (&new_info);
#endif

  /* The dyld port must be removed on detaching, or the inferior 
     will hang trying to talk to this port. */

  if (s->port != PORT_NULL) {
    port_deallocate (task_self (), s->port);
    s->state = dyld_clear;
    s->port = PORT_NULL;
  }

  if (s->thread != THREAD_NULL) {
    gdb_thread_kill (s->thread);
    s->thread = NULL;
  }
}

void next_dyld_thread_init (next_dyld_thread_status *s)
{
  s->thread = PORT_NULL;
  s->port = NULL;
  s->start_error = 0;
  s->state = dyld_clear;

  s->requests = xmalloc (1000 * sizeof (struct _dyld_event_message_request));
  s->max_requests = 1000;
  s->start_request = 0;
  s->end_request = 0;
}

#if (! USE_DYLD_EVENT_SERVER)

static void dyld_process_message 
(struct _dyld_event_message_request *request, 
 struct _dyld_event_message_reply *reply)
{
  reply->head.msg_simple = 1;
  reply->head.msg_size = sizeof (struct _dyld_event_message_reply);
  reply->head.msg_type = request->head.msg_type;
  reply->head.msg_local_port = PORT_NULL;
  reply->head.msg_remote_port = request->head.msg_remote_port;
  reply->head.msg_id = request->head.msg_id + 100;

  reply->RetCodeType.msg_type_name = MSG_TYPE_INTEGER_32;
  reply->RetCodeType.msg_type_size = sizeof (int) * 8;
  reply->RetCodeType.msg_type_number = 1;
  reply->RetCodeType.msg_type_inline = 1;
  reply->RetCodeType.msg_type_longform = 0;
  reply->RetCodeType.msg_type_deallocate = 0;

  if (request->head.msg_id != 200) {
     reply->RetCode = MIG_BAD_ID; 
     return; 
  }

  if ((request->head.msg_size != 56) ||
      (request->head.msg_simple != TRUE)) {
    reply->RetCode = MIG_BAD_ARGUMENTS;
    return;
  }

  if ((request->eventType.msg_type_inline != TRUE) ||
      (request->eventType.msg_type_longform != FALSE) ||
      (request->eventType.msg_type_name != MSG_TYPE_INTEGER_32) ||
      (request->eventType.msg_type_number != 7) ||
      (request->eventType.msg_type_size != 32)) {
    reply->RetCode = MIG_BAD_ARGUMENTS;
    return;
  }

  reply->RetCode = KERN_SUCCESS;
}

#else /* USE_DYLD_EVENT_SERVER */

#if defined (__MACH30__)

static boolean_t dyld_process_message
(struct _dyld_event_message_request *request,
 struct _dyld_event_message_reply *reply)
{
    return _dyld_event_server (&request->head, &reply->head);
}

kern_return_t
_dyld_event_server_callback (port_t subscriber, struct dyld_event event)
{
#if 0
  fprintf(stderr, "GDB received %d: %#x %#x\n",
          event.arg[1].header,
          event.arg[1].vmaddr_slide,
          event.arg[1].module_index);
#endif    
  return KERN_SUCCESS;
}

#else

static void dyld_process_message
(struct _dyld_event_message_request *request,
 struct _dyld_event_message_reply *reply)
{
   _dyld_event_server (request, reply);
}

void
_dyld_event_server_callback (port_t subscriber, struct dyld_event event)
{
  return;
}

#endif


#if defined (__DYNAMIC__)
/* The following symbol is referenced by libsys symbolically (instead of
   through undefined reference). To get strip(1) to know this symbol is not
   to be stripped it needs to have the REFERENCED_DYNAMICALLY bit
   (0x10) set. This would have been done automaticly by ld(1) if this symbol
   were referenced through an undefined symbol. */
asm (".desc __dyld_event_server_callback, 0x10");
#endif

#endif /* USE_DYLD_EVENT_SERVER */

static void next_mach_update_dyld (struct next_inferior_status *s)
{
  struct target_waitstatus ws;
  next_mach_process_dyld_messages (&ws, NULL);
}

static void dyld_drain_events (const struct next_inferior_status *ns, struct dyld_objfile_info *info)
{
    for (;;) {
        struct {
	  struct _dyld_event_message_request request;
#if defined (__MACH30__)            
	  u_char trailer[MAX_TRAILER_SIZE];
#endif
        } event_message;

        kern_return_t kret;

#if defined (__MACH30__)
        kret = mach_msg(&event_message.request.head,
                        (MACH_RCV_MSG | MACH_RCV_INTERRUPT),
                        0,
                        sizeof(event_message),
                        ns->dyld_port,
                        MACH_MSG_TIMEOUT_NONE,
                        MACH_PORT_NULL);
        if (MACH_RCV_INTERRUPT == kret)
        {
            warning("dyld request interrupted; aborting");
            break;
        }
#else
        event_message.request.head.msg_local_port = ns->dyld_port;
        event_message.request.head.msg_size = sizeof(event_message);

        kret = msg_receive (&event_message.request.head, RCV_LARGE | RCV_INTERRUPT, 0);
        if (kret == RCV_INTERRUPTED) {
            warning ("dyld request timed out; aborting");
            break;
        }
#endif

        MACH_CHECK_ERROR (kret);
        dyld_process_event (info, &event_message.request.event);

        if (event_message.request.event.type == DYLD_PAST_EVENTS_END) {
            break;
        }
    }
}

static int dyld_drain_events_helper (char *s)
{
  struct next_inferior_status *ns = (struct next_inferior_status *) s;

  dyld_drain_events (ns, &ns->dyld_status.current_info);
  return 1;
}

static int next_dyld_thread_empty (next_dyld_thread_status *s)
{
  return (s->start_request == s->end_request);
}

static struct _dyld_event_message_request *next_dyld_thread_current
  (next_dyld_thread_status *s)
{
  return &s->requests[s->start_request];
}

static void next_dyld_thread_enqueue 
  (next_dyld_thread_status *s, struct _dyld_event_message_request *r)
{
  unsigned int new_end_request = (s->end_request + 1) % s->max_requests;
  if (new_end_request == s->start_request) {
    /* overflow */
    abort ();
  } 
  s->requests[s->end_request] = *r;
  s->end_request = new_end_request;
}

static void next_dyld_thread_dequeue (next_dyld_thread_status *s)
{
  assert (s->start_request != s->end_request);
  s->start_request = (s->start_request + 1) % s->max_requests;
}

static void next_dyld_thread_wait (next_dyld_thread_status *s)
{
    kern_return_t kret;
    struct {
        struct _dyld_event_message_request request;
#if defined (__MACH30__)        
        u_char trailer[MAX_TRAILER_SIZE];
#endif        
    } event_message;
    struct _dyld_event_message_reply reply;

    while (1)
    {
#if defined (__MACH30__)
        if (s->start_request == s->end_request)
        {
            kret = mach_msg(&event_message.request.head,
                            (MACH_RCV_MSG),
                            0,
                            sizeof(event_message),
                            s->port,
                            MACH_MSG_TIMEOUT_NONE,
                            MACH_PORT_NULL);
        }
        else
        {
            kret = mach_msg(&event_message.request.head,
                            (MACH_RCV_MSG | MACH_RCV_TIMEOUT),
                            0,
                            sizeof(event_message),
                            s->port,
                            100,
                            MACH_PORT_NULL);
        }
#else
        event_message.request.head.msg_local_port = s->port;
        event_message.request.head.msg_size = sizeof(event_message);

        if (s->start_request == s->end_request) {
            kret = msg_receive (&event_message.request.head, MSG_OPTION_NONE, 0);
        } else {
            kret = msg_receive (&event_message.request.head, RCV_TIMEOUT, 100);
        }
#endif

        if (kret == RCV_TIMED_OUT) { return; }

        MACH_CHECK_ERROR(kret);

        dyld_debug_re ("got message in thread\n");

        registers_changed ();
        dyld_process_message (&event_message.request, &reply);
        registers_changed ();
        next_dyld_thread_enqueue (s, &event_message.request);

#if defined (__MACH30__)
    /* event message is one-way - no reply needed */
#else
        dyld_debug_re ("sending message from thread to dyld\n");
        kret = msg_send (&reply.head, SEND_TIMEOUT, 0);
        MACH_CHECK_ERROR(kret);
#endif
    }
}

static void next_dyld_thread_transmit (next_dyld_thread_status *s)
{
    assert (next_status != NULL);

    for (;;) {

        struct _dyld_event_message_request request;
        kern_return_t ret;

        if (next_dyld_thread_empty (s)) {
            return;
        }

        dyld_debug_re ("sending message from thread to gdb\n");
        request = *(next_dyld_thread_current (s));
#if defined (__MACH30__)
        request.head.msgh_bits = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0);
        request.head.msgh_size = sizeof (struct _dyld_event_message_request);
        request.head.msgh_remote_port = next_status->dyld_port;
        request.head.msgh_local_port = MACH_PORT_NULL;
        request.head.msgh_reserved = 0;
        ret = mach_msg(&request.head,
                       (MACH_SEND_MSG | MACH_SEND_TIMEOUT),
                       sizeof (struct _dyld_event_message_request),
                       0,
                       MACH_PORT_NULL,
                       MACH_MSG_TIMEOUT_NONE,
                       MACH_PORT_NULL);
#else
        request.head.msg_remote_port = next_status->dyld_port;
        ret = msg_send (&request.head, SEND_TIMEOUT, 0);
#endif
        if (ret == SEND_TIMED_OUT) {
            return;
        }
        if (ret != KERN_SUCCESS) {
            DYLD_THREAD_FATAL (ret);
        }

        next_dyld_thread_dequeue (s);
    }
}

static void next_dyld_thread (void *arg)
{
    next_dyld_thread_status *s = (next_dyld_thread_status *) arg;
    assert (s != NULL);

    for (;;) {
        next_dyld_thread_wait (s);
        next_dyld_thread_transmit (s);
    }
}

static void next_dyld_restore_runnable (PTR arg)
{
  struct _dyld_debug_task_state *state = (struct _dyld_debug_task_state *) arg;
  assert (next_status != NULL);
  _dyld_debug_restore_runnable (next_status->task, state);
}

int next_mach_start_dyld (struct next_inferior_status *s)
{
  struct _dyld_debug_task_state state;
  struct cleanup *state_cleanup;
  enum dyld_debug_return dret;
  kern_return_t ret;

  dyld_debug ("next_mach_start_dyld: starting inferior dyld_debug thread\n");

  assert (s->dyld_status.state != dyld_started);
  s->dyld_status.start_error = 1;

#if defined (__MACH30__)
  ret = mach_port_allocate(mach_task_self(),
                           MACH_PORT_RIGHT_RECEIVE,
                           &s->dyld_status.port);
  MACH_CHECK_ERROR(ret);
#else
  ret = port_allocate (task_self (), &s->dyld_status.port);
  MACH_CHECK_ERROR (ret);
  ret = port_set_backlog (task_self (), s->dyld_status.port, PORT_BACKLOG_MAX);
  MACH_CHECK_ERROR (ret);
#endif
  
  s->dyld_status.thread = gdb_thread_fork ((gdb_thread_fn_t) &next_dyld_thread, &s->dyld_status);
  s->dyld_status.start_error = 0;
  s->dyld_status.state = dyld_started;

  registers_changed ();
  dret = _dyld_debug_add_event_subscriber
    (s->task, 0, 120000, 0, s->dyld_status.port);
  registers_changed ();

  /* Check for new threads created by dyld.  this probably shouldn't
     be done here, but I'm in a hurry. */

  next_mach_check_new_threads ();

  if (dret != DYLD_SUCCESS) {
    warning ("next_mach_start_dyld: failed to start inferior dyld_debug thread: %s (%d)\n",
             dyld_debug_error_string (dret), dret);
    return -1;
  } else {
    dyld_debug ("next_mach_start_dyld: started inferior dyld_debug thread\n");
  }

  registers_changed ();
  dyld_debug ("next_mach_start_dyld: making runnable\n");

  _dyld_debug_make_runnable (next_status->task, &state);
  state_cleanup = make_cleanup (next_dyld_restore_runnable, (PTR) &state);

  if (dyld_drain_events_flag) {

    struct dyld_objfile_info previous_info, new_info;
    int ret;
    
    dyld_objfile_info_init (&previous_info);
    dyld_objfile_info_init (&new_info);
    
    dyld_objfile_info_copy (&previous_info, &s->dyld_status.current_info);
    dyld_objfile_info_free (&s->dyld_status.current_info);
    
    dyld_debug ("next_mach_start_dyld: processing previous dyld events\n");
    ret = catch_errors 
      (dyld_drain_events_helper, s, "error processing dyld events: ", RETURN_MASK_ALL);
    dyld_debug ("next_mach_start_dyld: done processing previous dyld events\n");
    
    dyld_update_shlibs (s, &s->dyld_status.path_info, &previous_info, &s->dyld_status.current_info, &new_info);
    dyld_objfile_info_free (&previous_info);

    dyld_objfile_info_copy (&s->dyld_status.current_info, &new_info);
    dyld_objfile_info_free (&new_info);
    
  } else {
    next_mach_update_dyld (s);
  }

  _dyld_debug_restore_runnable (next_status->task, &state);
  discard_cleanups (state_cleanup);

  dyld_debug ("next_mach_start_dyld: restored runnable\n");
  registers_changed ();
  
#if WITH_CFM
  if (inferior_auto_start_cfm_flag) {
    next_init_cfm_info_api (next_status);
  }
#endif /* WITH_CFM */

  return 0;
}

void next_clear_start_breakpoint ()
{
  remove_solib_event_breakpoints ();
}

void next_set_start_breakpoint (bfd *exec_bfd)
{
  struct breakpoint *b;

  struct symtab_and_line sal;

  if ((exec_bfd == NULL) || (bfd_get_start_address (exec_bfd) == (CORE_ADDR) -1)) {
    warning ("next_set_start_breakpoint: unable to determine entry point of executable\n");
    return;
  }
    
  INIT_SAL (&sal);
  sal.pc = bfd_get_start_address (exec_bfd);

  b = set_momentary_breakpoint (sal, NULL, bp_shlib_event);
  
  b->disposition = donttouch;
  b->thread = -1;

  b->addr_string = strsave ("start");

  breakpoints_changed ();
}

static void info_dyld_command (args, from_tty)
     char *args;
     int from_tty;
{
  assert (next_status != NULL);
  dyld_print_status_info (&next_status->dyld_status);
}

void next_mach_try_start_dyld ()
{
  assert (next_status != NULL);

  if (! inferior_start_dyld_flag) {
    return; 
  }

  /* Don't to start DYLD debug thread in the kernel */
  if (strcmp (current_target.to_shortname, "remote-kdp") == 0) {
    return;
  }

  /* DYLD debug thread has already been started */
  if (next_status->dyld_status.state == dyld_started) {
    return;
  }

  if (next_status->dyld_status.start_error) {
    return;
  }

  next_mach_start_dyld (next_status);
}

void next_mach_add_shared_symbol_files ()
{
  struct dyld_objfile_info *result = NULL;
  
  assert (next_status != NULL);
  result = &next_status->dyld_status.current_info;

  dyld_load_libraries (&next_status->dyld_status.path_info, result);

  dyld_merge_section_tables (result);
  dyld_update_section_tables (result, &current_target);
  dyld_update_section_tables (result, &exec_ops);

  reread_symbols ();
  breakpoint_re_set ();
}

void next_init_dyld (struct next_inferior_status *s, bfd *sym_bfd)
{
  struct dyld_objfile_info previous_info, new_info;

  dyld_init_paths (&s->dyld_status.path_info);

  dyld_objfile_info_init (&previous_info);
  dyld_objfile_info_init (&new_info);

  dyld_objfile_info_copy (&previous_info, &s->dyld_status.current_info);
  dyld_objfile_info_free (&s->dyld_status.current_info);

  if (dyld_preload_libraries_flag) {
    dyld_add_inserted_libraries (&s->dyld_status.current_info, &s->dyld_status.path_info);
    if (sym_bfd != NULL) {
      dyld_add_image_libraries (&s->dyld_status.current_info, sym_bfd);
    }
  }

  dyld_update_shlibs (s, &s->dyld_status.path_info,
		      &previous_info, &s->dyld_status.current_info, &new_info);
  dyld_objfile_info_free (&previous_info);
  
  dyld_objfile_info_copy (&s->dyld_status.current_info, &new_info);
  dyld_objfile_info_free (&new_info);

  s->dyld_status.state = dyld_initialized;
}

void next_init_dyld_symfile (bfd *sym_bfd)
{
  assert (next_status != NULL);
  next_init_dyld (next_status, sym_bfd);
}

static void next_dyld_init_command (args, from_tty)
     char *args;
     int from_tty;
{
  assert (next_status != NULL);
  next_init_dyld (next_status, sym_bfd);
}

static void next_dyld_start_command (char *args, int from_tty)
{
  assert (next_status != NULL);
  next_mach_start_dyld (next_status);
}

static void next_dyld_update_command (char *args, int from_tty)
{
  assert (next_status != NULL);
  next_mach_update_dyld (next_status);
}

kern_return_t next_mach_process_dyld_messages
(struct target_waitstatus *status, struct _dyld_event_message_request* initial_message)
{
  struct dyld_objfile_info previous_info, new_info;
  struct _dyld_debug_task_state state;
  struct cleanup *state_cleanup;
  kern_return_t kret;

  assert (next_status != NULL);
  assert (status != NULL);

  dyld_objfile_info_init (&previous_info);
  dyld_objfile_info_init (&new_info);

  dyld_objfile_info_copy (&previous_info, &next_status->dyld_status.current_info);

  if (NULL != initial_message)
    {
      dyld_process_event(&next_status->dyld_status.current_info, &initial_message->event);
    }
       
  for (;;) {
    char message_buffer[512];
    struct _dyld_event_message_request* request = (struct _dyld_event_message_request*) message_buffer;

#if defined (__MACH30__)
    kret = mach_msg(&request->head,
                    (MACH_RCV_MSG | MACH_RCV_TIMEOUT),
                    0,
                    sizeof (message_buffer),
                    next_status->dyld_port,
                    MACH_MSG_TIMEOUT_NONE,
                    MACH_PORT_NULL);
#else
    request->head.msg_local_port = next_status->dyld_port;
    request->head.msg_size = sizeof (struct _dyld_event_message_request);
    
    kret = msg_receive (&request->head, RCV_LARGE | RCV_TIMEOUT, 0);
#endif
    if (kret == RCV_TIMED_OUT) { break; }
    MACH_CHECK_ERROR (kret);

#if defined (__MACH30__)
    if (dyld_debug_flag &&
        (request->head.msgh_size != sizeof(struct _dyld_event_message_request)))
      {
        dyld_debug_re("dyld_drain_events: request->head.msgh_size != sizeof(struct _dyld_event_message_request\n");
        debug_dyld_event_request(request);
      }
#endif

    dyld_debug ("processing event\n");
    dyld_process_event (&next_status->dyld_status.current_info, &request->event);
  }

  dyld_debug ("all pending dyld messages cleared\n");

  if (next_status->dyld_status.current_info.nents == previous_info.nents) {
    dyld_objfile_info_free (&previous_info);
    return KERN_SUCCESS;
  }

  dyld_debug ("processing new library\n");

  registers_changed ();
  dyld_debug ("making runnable\n");

  _dyld_debug_make_runnable (next_status->task, &state);
  state_cleanup = make_cleanup (next_dyld_restore_runnable, (PTR) &state);

  dyld_update_shlibs (next_status, &next_status->dyld_status.path_info,
		      &previous_info, &next_status->dyld_status.current_info, &new_info);
  dyld_objfile_info_free (&previous_info);
  dyld_objfile_info_copy (&next_status->dyld_status.current_info, &new_info);
  dyld_objfile_info_free (&new_info);
    
  _dyld_debug_restore_runnable (next_status->task, &state);
  discard_cleanups (state_cleanup);

  dyld_debug ("restored runnable\n");
  registers_changed ();

  status->kind = TARGET_WAITKIND_LOADED;
  return KERN_SUCCESS;
}

static void dyld_cache_purge_comand (char *exp, int from_tty)
{
  assert (next_status != NULL);
  dyld_purge_cached_libraries (&next_status->dyld_status.current_info);
}

void
_initialize_nextstep_nat_dyld ()
{
  struct cmd_list_element *cmd;
  
  dyld_stderr = fdopen (fileno (stderr), "w+");
  dyld_stderr_re = fdopen (fileno (stderr), "w+");
  
  add_com ("dyld-start", class_run, next_dyld_start_command,
	   "Start the DYLD debugging thread.");

  add_com ("dyld-init", class_run, next_dyld_init_command,
	   "Init DYLD libraries to initial guesses.");

  add_com ("dyld-update", class_run, next_dyld_update_command,
	   "Process all pending DYLD events.");

  cmd = add_set_cmd ("dyld-preload-libraries", class_obscure, var_boolean, 
		     (char *) &dyld_preload_libraries_flag,
		     "Set if GDB should pre-load symbols for DYLD libraries.",
		     &setlist);
  add_show_from_set (cmd, &showlist);		

  cmd = add_set_cmd ("debug-dyld", class_obscure, var_boolean, 
		     (char *) &dyld_debug_flag,
		     "Set if printing dyld debugging statements.",
		     &setlist);
  add_show_from_set (cmd, &showlist);

  cmd = add_set_cmd ("dyld-drain-events", class_obscure, var_boolean, 
		     (char *) &dyld_drain_events_flag,
		     "Set if GDB should wait for DYLD_EVENTS_END after starting DYLD thread.",
		     &setlist);
  add_show_from_set (cmd, &showlist);

  add_info ("dyld", info_dyld_command,
	    "Show current DYLD state.");

  add_info ("sharedlibrary", info_dyld_command,
	    "Show current DYLD state.");

  add_com ("dyld-cache-purge", class_obscure, dyld_cache_purge_comand,
	   "Purge all symbols for DYLD images cached by GDB.");
}
