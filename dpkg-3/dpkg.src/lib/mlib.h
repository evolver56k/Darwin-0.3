#ifndef _DPKG_MLIB_H_
#define _DPKG_MLIB_H_

#include <sys/types.h>

/* Incremented when we do some kind of generally necessary operation,
   so that loops &c know to quit if we take an error exit.
   Decremented again afterwards.  */

extern volatile int onerr_abort;

/* The following functions are similar to their POSIX equivalents, but
   raise an exception in case of error.  m_fork() truncates the error
   handler in the child process to avoid calling the same handlers
   twice. */

void *m_malloc (size_t);
void *m_realloc (void*, size_t);
int m_fork (void);
void m_dup2 (int oldfd, int newfd);
void m_pipe (int fds[2]);

/* Check the program waitstatus <status> for possible errors.
   <description> is used as the name of the process for printing error
   messages.  <sigpipeok> specifies if exiting via SIGPIPE should be
   considered an error. */

void checksubprocerr (int status, const char *description, int sigpipeok);

/* Wait for subprocess specied by <pid> to terminate and check its
   waitstatus for errors.  use <description> as the name of the
   process for printing error messages.  <sigpipe> specifies if
   exiting via SIGPIPE should be considered an error. */

void waitsubproc (pid_t pid, const char *description, int sigpipeok);

#endif /* _DPKG_MLIB_H_ */
