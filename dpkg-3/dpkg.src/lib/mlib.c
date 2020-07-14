/*
 * libdpkg - Debian packaging suite library routines
 * mlib.c - `must' library: routines will succeed or longjmp
 *
 * Copyright (C) 1994, 1995 Ian Jackson <iwj10@cus.cam.ac.uk>
 * Copyright (C) 1997, 1998 Klee Dienes <klee@alum.mit.edu>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with dpkg; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "dpkg.h"
#include "libdpkg-int.h"
#include "config.h"

volatile int onerr_abort = 0;

void *m_malloc (size_t size) 
{
  void *r;
  
  onerr_abort++;
  r = malloc (size);
  if (r == NULL) {
    ohshit (_("unallocate to allocate %lu bytes (memory exhausted)"),
	    (unsigned long) size);
  }
  onerr_abort--;
  
  return r;
}

void *m_realloc (void *r, size_t size) 
{
  onerr_abort++;
  r = realloc (r, size);
  if (r == NULL) {
    ohshit (_("unallocate to reallocate %lu bytes at 0x%lx (memory exhausted)"),
	    (unsigned long) size, (unsigned long) r);
  }
  onerr_abort--;

  return r;
}

static void print_error_forked (const char *emsg, const char *context)
{
  fprintf (stderr, _("%s (subprocess): %s\n"), thisname, emsg);
}

static void cu_m_fork (int argc, void **argv)
{
  /* Don't do the other cleanups; they'll be done by the parent
     process.  */

  exit (EXIT_FAILURE);
}

int m_fork (void)
{
  pid_t r;

  onerr_abort++;
  r = fork ();
  if (r == -1) {
    ohshite (_("fork failed"));
  }
  onerr_abort--;

  if (r > 0) {
    return r;
  }
  
  push_cleanup (cu_m_fork, ~0, 0, 0, 0);
  set_error_display (print_error_forked, 0);
  return r;
}

void m_dup2 (int oldfd, int newfd)
{
  /* submsg: stdFDs */
  const char *const stdstrings[] = { 
    _("standard input"),
    _("standard output"),
    _("standard error")
  };
  
  onerr_abort++;
  if (dup2 (oldfd, newfd) == newfd) {
    return;
  }
  
  onerr_abort++;
  if (newfd < 3) {
    /* supermsg: stdFDs */
    ohshite (_("unable to duplicate file descriptor for %s"), stdstrings[newfd]);
  } else {
    ohshite (_("unable to duplicate file descriptor %d as %d"), oldfd, newfd);
  }
  onerr_abort--;
}

void m_pipe (int *fds)
{
  onerr_abort++;
  if (pipe (fds) == 0) { 
    return;
  }
  ohshite (_("unable to create pipe"));
  onerr_abort--;
}

void checksubprocerr (int status, const char *description, int sigpipeok)
{
  int n;

  onerr_abort++;
  if (WIFEXITED (status)) {
    n = WEXITSTATUS (status);
    if (n != 0) {
      ohshit (_("subprocess %s terminated with error status %d"), description, n);
    }
  } else if (WIFSIGNALED (status)) {
    n = WTERMSIG (status);
    if ((n != 0) && !(sigpipeok && (n == SIGPIPE))) {
      if (WCOREDUMP (status)) {
	ohshit (_("subprocess %s killed by signal %s (core dumped)"),
		description, strsignal (n));
      } else {
	ohshit (_("subprocess %s killed by signal %s (core dumped)"),
		description, strsignal (n));
      }
    }
  } else {
    ohshit (_("subprocess %s failed with unknown status code %d"), description, status);
  }
  onerr_abort--;
}

void waitsubproc (pid_t pid, const char *description, int sigpipeok)
{
  pid_t r;
  int status;

  onerr_abort++;
  for (;;) {
    r = waitpid (pid, &status, 0);
    if ((r == -1) && (errno == EINTR)) { continue; }
    break;
  }

  if (r < 0) {
    ohshite (_("unable to wait for subprocess %s (pid %d)"), description, pid);
  }

  if (r != pid) { 
    /* supermsg: waitsubproc -- arg is usually a command line */
    ohshite (_("noted death of unexpected subprocess (pid %d was not %d)"), r, pid);
  }

  checksubprocerr (status, description, sigpipeok);
  onerr_abort--;
}
