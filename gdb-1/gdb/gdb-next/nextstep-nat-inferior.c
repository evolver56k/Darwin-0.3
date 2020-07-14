#include "nextstep-nat-dyld.h"
#include "nextstep-nat-inferior.h"
#include "nextstep-nat-inferior-debug.h"
#include "nextstep-nat-mutils.h"
#include "nextstep-nat-sigthread.h"
#include "nextstep-nat-threads.h"
#include "nextstep-xdep.h"
#include "nextstep-nat-inferior-util.h"

#include "defs.h"
#include "top.h"
#include "inferior.h"
#include "target.h"
#include "symfile.h"
#include "symtab.h"
#include "objfiles.h"
#include "gdbcmd.h"
#include "gdbcore.h"
#include "gdbthread.h"
#include "environ.h"

#include "bfd.h"

#include <sys/ptrace.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>
#include <string.h>
#include <ctype.h>

extern bfd *exec_bfd;

extern struct target_ops child_ops;
extern struct target_ops next_child_ops;

next_inferior_status *next_status = NULL;

static int inferior_ptrace_flag = 1;
static int inferior_ptrace_on_attach_flag = 1;
int inferior_bind_exception_port_flag = 1;
int inferior_bind_notify_port_flag = 0;
int inferior_handle_exceptions_flag = 1;
static int inferior_handle_all_events_flag = 1;
int inferior_start_dyld_flag = 1;

#if WITH_CFM
int inferior_auto_start_cfm_flag = 0;
#endif /* WITH_CFM */

static void next_mach_process_message
(next_inferior_status *s, msg_header_t *msg, struct target_waitstatus *status);

static void next_process_events (struct next_inferior_status *ns, struct target_waitstatus *status, int timeout);

static void next_child_resume (int tpid, int step, enum target_signal signal);

static void next_handle_signal
(next_signal_thread_message *msg, struct target_waitstatus *status);

static int next_mach_wait (int pid, struct target_waitstatus *status);

static void next_mourn_inferior ();

static int next_lookup_task (char *args, task_t *ptask, int *ppid);

static void next_child_attach (char *args, int from_tty);

static void next_child_detach (char *args, int from_tty);

static void next_kill_inferior ();

static void next_ptrace_me ();

static int next_ptrace_him (int pid);

static void next_child_create_inferior (char *exec_file, char *allargs, char **env);

static void next_child_files_info (struct target_ops *ops);

static char *next_mach_pid_to_str (int tpid);

static int next_child_thread_alive (int tpid);

static void next_mach_process_message
(next_inferior_status *inferior, msg_header_t *message, struct target_waitstatus *status)
{
#if defined (__MACH30__)
  const mach_port_t local_port = message->msgh_local_port;
#else
  const port_t local_port = message->msg_local_port;
#endif
  
  status->kind = TARGET_WAITKIND_SPURIOUS;
  status->value.sig = TARGET_SIGNAL_UNKNOWN;

  if (local_port == inferior->notify_port) {

    assert (inferior_bind_notify_port_flag);
    inferior_debug ("next_mach_process_message: got a notify message");

    next_debug_notification_message (inferior, message);

  } else if (local_port == inferior->signal_port) {

    next_handle_signal ((next_signal_thread_message *) message, status);

  } else if (local_port == inferior->dyld_port) {

    error ("next_mach_process_message: unexpected message received for dyld port");

  } else if (local_port == inferior->inferior_exception_port) {

    assert (inferior_bind_exception_port_flag);
    next_handle_exception (inferior, message, status);
  
  } else {
    error ("next_mach_process_message: message addressed to invalid port 0x%lx\n", local_port);
  }
}

static void next_process_events
(struct next_inferior_status *inferior, struct target_waitstatus *status, int timeout)
{
  kern_return_t kret, kret2;

  unsigned char msg_data[1024];
  msg_header_t *msgin = (msg_header_t *) msg_data;

  int handling_exceptions = 1;

  for (;;) {

    port_t local_port;
    port_set_name_t event_port_set;

    assert (status->kind == TARGET_WAITKIND_SPURIOUS);

    kret = mach_port_allocate (mach_task_self(), MACH_PORT_RIGHT_PORT_SET, &event_port_set);
    MACH_CHECK_ERROR (kret);
    kret = mach_port_move_member (mach_task_self(), inferior->dyld_port, event_port_set);
    MACH_CHECK_ERROR (kret);

    if (inferior_bind_notify_port_flag)
      {
	kret = mach_port_move_member (mach_task_self(), inferior->notify_port, event_port_set);
	MACH_CHECK_ERROR (kret);
      }
    if (inferior_bind_exception_port_flag && handling_exceptions)
      {
	kret = mach_port_move_member (mach_task_self(), inferior->inferior_exception_port, event_port_set);
	MACH_CHECK_ERROR (kret);
      }
    kret = mach_port_move_member (mach_task_self(), inferior->signal_port, event_port_set);
    MACH_CHECK_ERROR (kret);
    
#if WITH_CFM
    if (inferior_auto_start_cfm_flag) {
      if (inferior->cfm_info.cfm_receive_right != PORT_NULL)
	{
	  kret = mach_port_move_member (mach_task_self(), inferior->cfm_info.cfm_receive_right, event_port_set);
	  MACH_CHECK_ERROR (kret);
	}
    }
#endif

    /* wait for event */
    inferior_debug ("next_process_events: waiting for message for inferior\n");
    kret = next_mach_msg_receive (msgin, sizeof (msg_data), timeout, event_port_set);

    kret2 = mach_port_destroy (mach_task_self (), event_port_set);
    MACH_CHECK_ERROR (kret2);

    if (kret == RCV_TIMED_OUT) { return; }
    MACH_CHECK_ERROR (kret);

#if defined (__MACH30__)
    local_port = msgin->msgh_local_port;
#else
    local_port = msgin->msg_local_port;
#endif

    if (local_port == inferior->dyld_port)
      {
	/* process dyld events */
	inferior_debug ("next_process_events: got dyld message\n");
	kret = next_mach_process_dyld_messages (status, msgin);
	MACH_CHECK_ERROR (kret);
	if (status->kind == TARGET_WAITKIND_LOADED)
	  {
	    status->kind = TARGET_WAITKIND_SPURIOUS;
	  }
      }

    else if (local_port == inferior->notify_port)
      {
	/* process notify messages */
	for (;;)
	  {
	    inferior_debug ("next_process_events: got notify message\n");
	    next_mach_process_message (next_status, msgin, status);
	    kret = next_mach_msg_receive (msgin, sizeof (msg_data), 1, inferior->notify_port);
	    if (RCV_TIMED_OUT == kret)
	      {
		break;
	      }
	  }
      }

    else if (local_port == inferior->inferior_exception_port)
      {
	/* process exceptions */
	for (;;)
	  {
	    inferior_debug ("next_process_events: got exception message\n");
	    if (! handling_exceptions)
	      {
		break;
	      }
	    next_mach_process_message (next_status, msgin, status);
	    if (! inferior_handle_all_events_flag)
	      {
		handling_exceptions = 0;
	      }
	    kret = next_mach_msg_receive (msgin, sizeof (msg_data), 1, inferior->inferior_exception_port);
	    if (kret == RCV_TIMED_OUT)
	      {
		break;
	      }
	  }
	if (status->kind != TARGET_WAITKIND_SPURIOUS)
	  {
	    assert (inferior_handle_exceptions_flag);
	    break;
	  }
      }
    else if (local_port == inferior->signal_port)
      {
	/* process signals */
	for (;;)
	  {
	    inferior_debug ("next_process_events: got signal message\n");
	    next_mach_process_message (next_status, msgin, status);
	    assert (status->kind != TARGET_WAITKIND_SPURIOUS);
	    if (! inferior_handle_all_events_flag)
	      {
		break;
	      }
	    kret = next_mach_msg_receive (msgin, sizeof (msg_data), 1, inferior->signal_port);
	    if (RCV_TIMED_OUT == kret)
	      {
		break;
	      }
	  }
	if (status->kind != TARGET_WAITKIND_SPURIOUS)
	  {
	    break;
	  }
      }

#if WITH_CFM      
    else if (local_port == inferior->cfm_info.cfm_receive_right)
      {
	if (inferior_auto_start_cfm_flag) {
	  next_handle_cfm_event(next_status, msgin);
          status->kind = TARGET_WAITKIND_LOADED;
          break;
	}
      }
#endif /* WITH_CFM */
    else
      {
	error ("got message on unknown port: 0x%08x\n", local_port);
	break;
      }
  }

  inferior_debug ("next_process_events: returning with (status->kind == %d)\n", status->kind);
}

void next_mach_check_new_threads ()
{
  thread_array_t thread_list = NULL;
  unsigned int nthreads = 0;

  kern_return_t kret;
  unsigned int i;

  kret = task_threads (next_status->task, &thread_list, &nthreads);
  if (kret != KERN_SUCCESS) { return; }
  MACH_CHECK_ERROR (kret);
  
  for (i = 0; i < nthreads; i++) {
    int tpid = next_thread_list_insert (next_status, next_status->pid, thread_list[i]);
    if (! in_thread_list (tpid)) {
      add_thread (tpid);
    }
  }

  kret = vm_deallocate (task_self(), (vm_address_t) thread_list, (nthreads * sizeof (int)));
  MACH_CHECK_ERROR (kret);
}

static void next_child_resume (int tpid, int step, enum target_signal signal)
{
  int nsignal = target_signal_to_host (signal);
  struct target_waitstatus status;

  int pid;
  thread_t thread;

  if (tpid == -1) { 
    tpid = inferior_pid;
  }
  next_thread_list_lookup_by_id (next_status, tpid, &pid, &thread);

  assert (tm_print_insn != NULL);
  assert (next_status != NULL);

  next_inferior_check_stopped (next_status);

  inferior_debug ("next_child_resume: checking for pending events\n");
  status.kind = TARGET_WAITKIND_SPURIOUS;
  next_process_events (next_status, &status, 1);
  assert (status.kind == TARGET_WAITKIND_SPURIOUS);

  inferior_debug ("next_child_resume: %s process with signal %d\n", 
		  step ? "stepping" : "continuing", nsignal);

  if (inferior_debug_flag) {
    CORE_ADDR pc = read_register (PC_REGNUM);
    fprintf (stdout, "[%d inferior]: next_child_resume: about to execute instruction at ", getpid ());
    print_address (pc, gdb_stdout);
    fprintf (stdout, " (");
    pc += (*tm_print_insn) (pc, &tm_print_insn_info);
    fprintf (stdout, ")\n");
    fprintf (stdout, "[%d inferior]: next_child_resume: subsequent instruction is ", getpid ());
    print_address (pc, gdb_stdout);
    fprintf (stdout, " (");
    pc += (*tm_print_insn) (pc, &tm_print_insn_info);
    fprintf (stdout, ")\n");
  }
  
  if (next_status->stopped_in_ptrace) {
    next_inferior_resume_ptrace (next_status, nsignal, PTRACE_CONT);
  }

  if (step) {
    prepare_threads_before_run (next_status, step, thread, (tpid != -1));
  } else {
    prepare_threads_before_run (next_status, 0, THREAD_NULL, 0);
  }

  next_inferior_resume_mach (next_status);
}

static void next_handle_signal
(next_signal_thread_message *msg, struct target_waitstatus *status)
{
  assert (next_status != NULL);

  assert (next_status->attached_in_ptrace);
  assert (! next_status->stopped_in_ptrace);

  if (inferior_debug_flag) {
    inferior_debug ("next_handle_signal: received signal message: ");
    next_signal_thread_debug_status (stderr, msg->status);
  }

  if (msg->pid != next_status->pid) {
    warning ("next_handle_signal: signal message was for pid %d, not for inferior process (pid %d)\n", 
	     msg->pid, next_status->pid);
    return;
  }
  
  if (WIFEXITED (msg->status)) {
    status->kind = TARGET_WAITKIND_EXITED;
    status->value.integer = WEXITSTATUS (msg->status);
    return;
  }

  if (! WIFSTOPPED (msg->status)) {
    status->kind = TARGET_WAITKIND_SIGNALLED;
    status->value.sig = target_signal_from_host (WTERMSIG (msg->status));
    return;
  }

  next_status->stopped_in_ptrace = 1;
  next_inferior_suspend_mach (next_status);

  status->kind = TARGET_WAITKIND_STOPPED;
  status->value.sig = target_signal_from_host (WSTOPSIG (msg->status));
}

int next_wait (struct next_inferior_status *ns, struct target_waitstatus *status)
{
  int ret;

  assert (ns != NULL);
  
  status->kind = TARGET_WAITKIND_SPURIOUS;
  next_process_events (ns, status, 0);
  assert (status->kind != TARGET_WAITKIND_SPURIOUS);
  
  next_mach_check_new_threads ();

  if (! next_thread_valid (next_status->task, next_status->last_thread)) {
    if (next_task_valid (next_status->task)) {
      warning ("Currently selected thread no longer alive; selecting intial thread");
      next_status->last_thread = next_primary_thread_of_task (next_status->task);
    }
  }

  next_thread_list_lookup_by_info (next_status, next_status->pid, next_status->last_thread, &ret);
  inferior_debug ("next_wait: returning 0x%08x\n", ret);
  return ret;
}

static int next_mach_wait (int pid, struct target_waitstatus *status)
{
  assert (next_status != NULL);
  return next_wait (next_status, status);
}

static void next_mourn_inferior ()
{
  unpush_target (&next_child_ops);
  child_ops.to_mourn_inferior ();
  next_inferior_destroy (next_status);

  inferior_pid = 0;
  attach_flag = 0;

  if (symfile_objfile != NULL) {
    assert (symfile_objfile->obfd != NULL);
    next_init_dyld_symfile (symfile_objfile->obfd);
  } else {
    next_init_dyld_symfile (NULL);
  }
}

static int next_lookup_task (char *args, task_t *ptask, int *ppid)
{
  kern_return_t kret;
  task_t itask;
  char *host_str, *pid_str;
  unsigned long lpid;
  int pid;
  char *tmp1, *tmp2;

  pid = 0;

  assert (ptask != NULL);
  assert (ppid != NULL);

  if (args == NULL) { return TASK_NULL; }

  for (tmp1 = args; *tmp1 && isspace (*tmp1); tmp1++);
  tmp1 = strchr (tmp1, ' ');
  if (tmp1 == NULL) { 
    host_str = NULL;
    pid_str = args;
  } else {
    host_str = args;
    for (tmp2 = tmp1; *tmp2 && isspace (*tmp2); tmp2++);
    *tmp1 = '\0';
    pid_str = tmp2;
  }

  assert (pid_str != NULL);
  lpid = strtoul (pid_str, &tmp1, 10);
  if (isdigit (*pid_str) && (*tmp1 == '\0')) {
    if ((lpid > LONG_MAX) || ((lpid == ULONG_MAX) && (errno == ERANGE))) {
      error ("Unable to locate pid \"%s\" (integer overflow).", pid_str);
    }
    pid_str = NULL;
    pid = lpid;
  }

#if defined (__MACH30__)

  if (host_str != NULL) {
    error ("Unable to attach to remote processes on Mach 3.0 (no netname_look_up()).");
  }
  if (task_for_pid (mach_task_self(), pid, &itask) != KERN_SUCCESS) {
    error ("Unable to locate task for pid %d.", pid);
  }

#else /* ! __MACH30__ */

  if (pid_str == NULL) {
    task_t pid_server = task_self ();
    if (host_str != NULL) {
      kret = netname_look_up (name_server_port, host_str, "pid-server", &pid_server);
      MACH_CHECK_NOERROR (kret);
      if (kret != KERN_SUCCESS) {
	error ("Unable to locate pid server on host \"%s\".", host_str);
      }
    }
    if (task_by_unix_pid (pid_server, pid, &itask) != KERN_SUCCESS) {
      error ("Unable to locate task for pid %d.", pid);
    }
  } else {
    if (host_str == NULL) {
      host_str = "localhost";
    }
    kret = netname_look_up (name_server_port, host_str, pid_str, &itask);
    MACH_CHECK_NOERROR (kret);
    if (kret != KERN_SUCCESS) {
      error ("Unable to find task named \"%s\" on host \"%s\"", pid_str, host_str);
    }
  }

#endif /* __MACH30__ */  

  *ptask = itask;
  *ppid = pid;

  return 0;
}

static void next_child_attach (char *args, int from_tty)
{
  struct target_waitstatus w;
  task_t itask;
  int pid;

  assert (next_status != NULL);
  next_inferior_destroy (next_status);

  next_lookup_task (args, &itask, &pid);
  
  next_create_inferior_for_task (next_status, itask, pid);

  if (inferior_ptrace_on_attach_flag) {
    if (call_ptrace (PTRACE_ATTACH, pid, 0, 0) != 0) {
      error ("next_child_attach: ptrace (%d, %d, %d, %d): %s\n",
	     PTRACE_ATTACH, pid, 0, 0, strerror (errno));
    }
    next_status->attached_in_ptrace = 1;
    next_status->stopped_in_ptrace = 0;
    next_status->suspend_count = 0;
  } else {
    if (inferior_handle_exceptions_flag) {
      next_inferior_suspend_mach (next_status);
    }
  }

  next_clear_start_breakpoint ();
  if (inferior_start_dyld_flag) {
    next_set_start_breakpoint (exec_bfd);
  }

  next_mach_check_new_threads ();

  attach_flag = 1;
  next_thread_list_lookup_by_info (next_status, pid, next_status->last_thread, &inferior_pid);
  
  push_target (&next_child_ops);

  if (next_status->attached_in_ptrace) {
    /* read attach notification */
    next_wait (next_status, &w);
  }
}

static void next_child_detach (char *args, int from_tty)
{
  assert (next_status != NULL);

  next_inferior_check_stopped (next_status);
  
  if (next_status->attached_in_ptrace) {
    next_inferior_suspend_ptrace (next_status);
    assert (next_status->stopped_in_ptrace);
  }

  if (inferior_bind_exception_port_flag) {
#if !defined (__MACH30__)
    kern_return_t kret = task_set_exception_port (next_status->task, next_status->inferior_old_exception_port);
    MACH_CHECK_ERROR (kret);
#else
    int i;
    for (i = 0; i < next_status->exception.count; ++i)
      {
        kern_return_t kret = task_set_exception_ports(next_status->task,
                                                      next_status->exception.masks[i],
                                                      next_status->exception.ports[i],
                                                      next_status->exception.behaviors[i],
                                                      next_status->exception.flavors[i]);
        MACH_CHECK_ERROR(kret);
      }
#endif
  }

  if (next_status->attached_in_ptrace) {
    next_inferior_resume_ptrace (next_status, 0, PTRACE_DETACH);
  }

  prepare_threads_before_run (next_status, 0, THREAD_NULL, 0);
  next_inferior_resume_mach (next_status);

  next_status->attached_in_ptrace = 0;

  next_status->task = TASK_NULL;
  next_status->pid = 0;

  target_mourn_inferior ();
}

static SIGJMP_BUF tmp_buf;

void null_error_hook ()
{
  inferior_debug ("ignoring error while killing threads\n");
  SIGLONGJMP (tmp_buf, 1);
}

static void next_kill_inferior ()
{
  assert (next_status != NULL);
 
  if (inferior_pid == 0) {
    return;
  }

  if (attach_flag) {
    next_child_detach (NULL, 0);
    return;
  }
  
  if (! next_inferior_valid (next_status)) {
    target_mourn_inferior ();
    return;
  }

  next_inferior_check_stopped (next_status);
  
  if (next_status->attached_in_ptrace) {
    if (! next_status->stopped_in_ptrace) {
      next_inferior_suspend_ptrace (next_status);
    }
    assert (next_status->stopped_in_ptrace);
  }
  
  next_inferior_suspend_mach (next_status);
  prepare_threads_before_run (next_status, 0, THREAD_NULL, 0);
  
  if (next_status->attached_in_ptrace) {
    assert (next_status->stopped_in_ptrace);
    if (call_ptrace (PTRACE_KILL, next_status->pid, 0, 0) != 0) {
      error ("next_child_detach: ptrace (%d, %d, %d, %d): %s",
	     PTRACE_KILL, next_status->pid, 0, 0, strerror (errno));
    }
    next_status->stopped_in_ptrace = 0;
  }
  
  {
    NORETURN void (*saved_error_hook) PARAMS ((void)) ATTR_NORETURN;

    saved_error_hook = error_hook;
    error_hook = null_error_hook;

    if (SIGSETJMP (tmp_buf) == 0) {
      prepare_threads_before_run (next_status, 0, THREAD_NULL, 0);
      next_inferior_resume_mach (next_status);
    } else {
      ;
    }

    error_hook = saved_error_hook;
  }

  target_mourn_inferior ();
}

static void next_ptrace_me ()
{
  call_ptrace (PTRACE_TRACEME, 0, 0, 0);
}

static int next_ptrace_him (int pid)
{
  task_t itask;
  kern_return_t kret;
  int traps_expected;
  int pret;

  assert (! next_status->attached_in_ptrace);
  assert (! next_status->stopped_in_ptrace);
  assert (next_status->suspend_count == 0);

  kret = task_by_unix_pid (task_self (), pid, &itask);
  if (kret != KERN_SUCCESS) {
    error ("Unable to find Mach task port for process-id %d: %s (%d).", 
	   pid, mach_error_string (kret), kret);
  }

  inferior_debug ("inferior task: 0x%08x, pid: %d\n", itask, pid);
  
  push_target (&next_child_ops);
  next_create_inferior_for_task (next_status, itask, pid);

  next_status->attached_in_ptrace = 1;
  next_status->stopped_in_ptrace = 0;
  next_status->suspend_count = 0;

  traps_expected = (0 == startup_with_shell_flag ? 1 : 2);
  startup_inferior (traps_expected);
  
  if (! next_task_valid (next_status->task)) {
    target_mourn_inferior ();
    return 0;
  }

  next_inferior_check_stopped (next_status);

  if (inferior_ptrace_flag) {
    assert (next_status->attached_in_ptrace);
    assert (next_status->stopped_in_ptrace);
  } else {
    next_inferior_resume_ptrace (next_status, 0, PTRACE_DETACH);
  }

  next_thread_list_lookup_by_info (next_status, pid, next_status->last_thread, &pret);
  return pret;
}

static void next_child_create_inferior (char *exec_file, char *allargs, char **env)
{
  fork_inferior (exec_file, allargs, env, next_ptrace_me, next_ptrace_him, NULL, NULL);

  next_clear_start_breakpoint ();
  if (inferior_start_dyld_flag) {
    next_set_start_breakpoint (exec_bfd);
  }
  attach_flag = 0;

  proceed ((CORE_ADDR) -1, TARGET_SIGNAL_0, 0);
}

static void next_child_files_info (struct target_ops *ops)
{
  assert (next_status != NULL);
  next_debug_inferior_status (next_status);
}

static char *next_mach_pid_to_str (int tpid)
{
  static char buf[128];
  int pid;
  thread_t thread;

  next_thread_list_lookup_by_id (next_status, tpid, &pid, &thread);
  sprintf (buf, "process %d thread 0x%lx", pid, (unsigned long) thread);
  return buf;
}

static int next_child_thread_alive (int tpid)
{
  int pid;
  thread_t thread;

  next_thread_list_lookup_by_id (next_status, tpid, &pid, &thread);
  assert (pid == next_status->pid);

  return next_thread_valid (next_status->task, thread);
}

struct target_ops next_child_ops;

void 
_initialize_next_inferior ()
{
  struct cmd_list_element *cmd;

  assert (next_status == NULL);
  next_status = (struct next_inferior_status *)
      xmalloc (sizeof (struct next_inferior_status));

  next_inferior_reset (next_status);

  dyld_init_paths (&next_status->dyld_status.path_info);
  dyld_objfile_info_init (&next_status->dyld_status.current_info);

#if WITH_CFM
  cfm_info_init (&next_status->cfm_info);
#endif /* WITH_CFM */

  next_child_ops = child_ops;
  child_ops.to_can_run = NULL;

  next_child_ops.to_shortname = "next-child";
  next_child_ops.to_longname = "NeXTStep/Rhapsody/MacOSX child process";
  next_child_ops.to_doc = "NeXTStep/Rhapsody/MacOSX child process (started by the \"run\" command).";
  next_child_ops.to_attach = next_child_attach;
  next_child_ops.to_detach = next_child_detach;
  next_child_ops.to_create_inferior = next_child_create_inferior;
  next_child_ops.to_files_info = next_child_files_info;
  next_child_ops.to_wait = next_mach_wait;
  next_child_ops.to_mourn_inferior = next_mourn_inferior;
  next_child_ops.to_kill = next_kill_inferior;
  next_child_ops.to_resume = next_child_resume;
  next_child_ops.to_thread_alive = next_child_thread_alive;
  next_child_ops.to_pid_to_str = next_mach_pid_to_str;
  next_child_ops.to_load = NULL;
  next_child_ops.to_xfer_memory = mach_xfer_memory;

  add_target (&next_child_ops);

  inferior_stderr = fdopen (fileno (stderr), "w+");
  inferior_debug ("GDB task: 0x%08x, pid: %d\n", task_self(), getpid());

  cmd = add_set_cmd ("inferior-bind-notify-port", class_obscure, var_boolean, 
		     (char *) &inferior_bind_notify_port_flag,
		     "Set if GDB should bind the task notify port.",
		     &setlist);
  add_show_from_set (cmd, &showlist);		

  cmd = add_set_cmd ("inferior-bind-exception-port", class_obscure, var_boolean, 
		     (char *) &inferior_bind_exception_port_flag,
		     "Set if GDB should bind the task exception port.",
		     &setlist);
  add_show_from_set (cmd, &showlist);		

  cmd = add_set_cmd ("inferior-handle-exceptions", class_obscure, var_boolean, 
		     (char *) &inferior_handle_exceptions_flag,
		     "Set if GDB should handle exceptions or pass them to the UNIX handler.",
		     &setlist);
  add_show_from_set (cmd, &showlist);

  cmd = add_set_cmd ("inferior-handle-all-events", class_obscure, var_boolean, 
		     (char *) &inferior_handle_all_events_flag,
		     "Set if GDB should immediately handle all exceptions upon each stop, "
		     "or only the first received.",
		     &setlist);
  add_show_from_set (cmd, &showlist);		

  cmd = add_set_cmd ("inferior-ptrace", class_obscure, var_boolean, 
		     (char *) &inferior_ptrace_flag,
		     "Set if GDB should attach to the subprocess using ptrace ().",
		     &setlist);
  add_show_from_set (cmd, &showlist);		
  
  cmd = add_set_cmd ("inferior-ptrace-on-attach", class_obscure, var_boolean, 
		     (char *) &inferior_ptrace_on_attach_flag,
		     "Set if GDB should attach to the subprocess using ptrace ().",
		     &setlist);
  add_show_from_set (cmd, &showlist);		
  
  cmd = add_set_cmd ("inferior-auto-start-dyld", class_obscure, var_boolean, 
		     (char *) &inferior_start_dyld_flag,
		     "Set if GDB will try to start dyld automatically.",
		     &setlist);
  add_show_from_set (cmd, &showlist);	

#if WITH_CFM
  cmd = add_set_cmd ("inferior-auto-start-cfm", class_obscure, var_boolean, 
		     (char *) &inferior_auto_start_cfm_flag,
		     "Set if GDB should enable debugging of CFM shared libraries.",
		     &setlist);
  add_show_from_set (cmd, &showlist);
#endif /* WITH_CFM */
}
