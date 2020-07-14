#define _POSIX_SOURCE

#include <assert.h>
#include <sys/types.h>

pid_t getpgid (pid_t pid)
{
  return getpgrp (pid);
}
