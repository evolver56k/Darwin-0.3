#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <sys/time.h>
#include <sys/select.h>

#include "nextstep-nat-sigthread.h"
#include "nextstep-nat-mutils.h"

#include "defs.h"
#include "gdbcmd.h"

static FILE *sigthread_stderr = NULL;
static FILE *sigthread_stderr_re = NULL;
static int sigthread_debugflag = 0;

static int sigthread_debug (const char *fmt, ...)
{
  va_list ap;
  if (sigthread_debugflag) {
    va_start (ap, fmt);
    fprintf (sigthread_stderr, "[%d sigthread]: ", getpid ());
    vfprintf (sigthread_stderr, fmt, ap);
    va_end (ap);
    return 0;
  } else {
    return 0;
  }
}

/* A re-entrant version for use by the signal handling thread */

void sigthread_debug_re (const char *fmt, ...)
{
  va_list ap;
  if (sigthread_debugflag) {
    va_start (ap, fmt);
    fprintf (sigthread_stderr_re, "[%d sigthread]: ", getpid ());
    vfprintf (sigthread_stderr_re, fmt, ap);
    va_end (ap);
    fflush (sigthread_stderr_re);
  }
}

static void next_signal_thread (void *arg);

void next_signal_thread_init (next_signal_thread_status *s)
{
  s->signal_thread = THREAD_NULL;
  s->signal_port = PORT_NULL;
}

void next_signal_thread_create (next_signal_thread_status *s, mach_port_t signal_port, int pid)
{
  s->signal_port = signal_port;
  s->inferior_pid = pid;
  s->signal_thread = gdb_thread_fork ((gdb_thread_fn_t) &next_signal_thread, s);
}

void next_signal_thread_destroy (next_signal_thread_status *s)
{
  assert (s->signal_thread != NULL);
  assert (s->signal_port != PORT_NULL);

  sigthread_debug ("next_signal_thread: killing signal thread\n");
  gdb_thread_kill (s->signal_thread);
  sigthread_debug ("next_signal_thread: signal thread killed\n");
  port_deallocate (task_self (), s->signal_port);

  next_signal_thread_init (s);
}

void next_signal_thread_debug (FILE *f, next_signal_thread_status *s)
{
  fprintf (f, "                signal port: 0x%lx\n", (long) s->signal_port);
}

void next_signal_thread_debug_status (FILE *f, WAITSTATUS status)
{
  if (WIFEXITED (status)) {
    fprintf (f, "process exited with status %d\n", WEXITSTATUS (status));
  } else if (WIFSIGNALED (status)) {
    fprintf (f, "process terminated with signal %d\n", WTERMSIG (status));
  } else if (WIFSTOPPED (status)) {
    fprintf (f, "process stopped with signal %d\n", WSTOPSIG (status));
  } else {
    fprintf (f, "next_debug_status: unknown status value %d\n", status);
  }
}

#if defined (__MACH30__)

static kern_return_t
send_mach_message (next_signal_thread_message* signal, next_signal_thread_status* status)
{
  kern_return_t result;

  signal->header.msgh_bits = MACH_MSGH_BITS (MACH_MSG_TYPE_COPY_SEND, 0);
  signal->header.msgh_size = sizeof (next_signal_thread_message);
  signal->header.msgh_local_port = MACH_PORT_NULL;
  signal->header.msgh_remote_port = status->signal_port;
  signal->header.msgh_reserved = 0;
  signal->header.msgh_id = 0;

  result = mach_msg (&signal->header, (MACH_SEND_MSG | MACH_SEND_INTERRUPT),
		    sizeof (next_signal_thread_message), 0,
		    MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
  return result;
}

#else /* ! __MACH30__ */

static kern_return_t
send_mach_message (next_signal_thread_message* signal, next_signal_thread_status* status)
{
  kern_return_t result;

  signal->header.msg_unused = 0;
  signal->header.msg_simple = TRUE;
  signal->header.msg_size = sizeof (next_signal_thread_message);
  signal->header.msg_type = MSG_TYPE_NORMAL;
  signal->header.msg_local_port = PORT_NULL;
  signal->header.msg_remote_port = status->signal_port;
  signal->header.msg_id = 0;

  signal->type.msg_type_name = MSG_TYPE_INTEGER_32;
  signal->type.msg_type_size = 32;
  signal->type.msg_type_number = 2;
  signal->type.msg_type_inline = TRUE;
  signal->type.msg_type_longform = FALSE;
  signal->type.msg_type_deallocate = FALSE;
  signal->type.msg_type_unused = 0;

  result = msg_send (&signal->header, SEND_INTERRUPT, 0);

  return result;
}

#endif /* __MACH30__ */

static void next_signal_thread (void *arg)
{
  next_signal_thread_status *s = (next_signal_thread_status *) arg;
  assert (s != NULL);

  for (;;) {

    WAITSTATUS status = 0;
    int pid = 0;
    int ret;

    sigthread_debug_re ("next_signal_thread: waiting for signals for pid %d\n", s->inferior_pid);

    pid = 0;
    while (pid == 0) {
      struct timeval tv = { 0, 10000 };
      ret = select (0, NULL, NULL, NULL, &tv);
      if (ret < 0) {
	break;
      }
      assert (ret == 0);
      pid = waitpid (s->inferior_pid, &status, WNOHANG);
    }

    if ((pid < 0) && (errno == ECHILD)) {
      sigthread_debug_re ("next_signal_thread: no children present: waiting for parent\n");
      for (;;) { }
    }

    if ((ret < 0) || (pid < 0)) {
      if (errno != EINTR) {
	fprintf (sigthread_stderr_re, "next_signal_thread: unexpected error: %s\n", strerror (errno));
	abort ();
      }
      sigthread_debug_re ("next_signal_thread: wait interrupted; continuing\n");
      continue;
    }

    if (sigthread_debugflag) {
      sigthread_debug_re ("next_signal_thread: got status %d for pid %d (expected inferior is %d)\n",
			  status, pid, s->inferior_pid);
      sigthread_debug_re ("next_signal_thread: got signal ");
      next_signal_thread_debug_status (sigthread_stderr_re, status);
    }


    if (pid != s->inferior_pid) {
      fprintf (sigthread_stderr_re,
	       "next_signal_thread: got status value %d for unexpected pid %d\n", status, pid);
      abort ();
    }

    {
      next_signal_thread_message msg;
      kern_return_t ret;

      msg.pid = pid;
      msg.status = status;

      ret = send_mach_message (&msg, s);
      if (ret != KERN_SUCCESS) {
	fprintf (sigthread_stderr_re, 
		 "next_signal_thread: error sending signal message: %s (0x%08X)\n", 
		 mach_error_string (ret), ret);
	abort ();
      }
    }
  }
}

void
_initialize_nextstep_nat_sigthread ()
{
  struct cmd_list_element *cmd = NULL;

  sigthread_stderr = fdopen (fileno (stderr), "w+");
  sigthread_stderr_re = fdopen (fileno (stderr), "w+");

  cmd = add_set_cmd ("debug-signals", class_obscure, var_boolean, 
		     (char *) &sigthread_debugflag,
		     "Set if printing signal thread debugging statements.",
		     &setlist);
  add_show_from_set (cmd, &showlist);		
}
