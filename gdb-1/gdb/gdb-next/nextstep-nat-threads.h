#ifndef __NEXTSTEP_NAT_THREADS_H__
#define __NEXTSTEP_NAT_THREADS_H__

struct next_inferior_status;

#if defined (USE_PTHREADS)

#include <pthread.h>

typedef void* (*pthread_fn_t) (void *arg);

void gdb_pthread_kill (pthread_t pthread);
pthread_t gdb_pthread_fork (pthread_fn_t function, void *arg);

#define gdb_thread_exit pthread_exit
#define gdb_thread_fork gdb_pthread_fork
#define gdb_thread_kill	gdb_pthread_kill

#define gdb_thread_t pthread_t
#define gdb_thread_fn_t pthread_fn_t

#else /* ! USE_PTHREADS */

#include <mach/cthreads.h>

void gdb_cthread_kill (cthread_t cthread);

#define gdb_thread_exit cthread_exit
#define gdb_thread_fork cthread_fork
#define gdb_thread_kill gdb_cthread_kill

#define gdb_thread_t cthread_t
#define gdb_thread_fn_t cthread_fn_t

#endif /* USE_PTHREADS */

void set_trace_bit PARAMS ((thread_t thread));
void clear_trace_bit PARAMS ((thread_t thread));
void clear_suspend_count PARAMS ((thread_t thread));

void prepare_threads_before_run 
  PARAMS ((struct next_inferior_status *inferior, int step, thread_t current, int stop_others));

void prepare_threads_after_stop PARAMS ((struct next_inferior_status *inferior));

char *unparse_run_state PARAMS ((int run_state));

void print_thread_info PARAMS ((thread_t tid));

void info_task_command PARAMS ((char *args, int from_tty));
void info_thread_command PARAMS ((char *tidstr, int from_tty));

#endif /* __NEXTSTEP_NAT_THREADS_H__ */

