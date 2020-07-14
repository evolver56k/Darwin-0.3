#ifndef NT_H
#define NT_H

#include <winnt-pdo.h>
#include <stdio.h>
#include <stdlib.h>

/* maximum available file handles determined via experiment */
#define NOFILE 32

#define access _access

#define chdir _chdir
#define getcwd _getcwd

#define environ _environ

#define mktemp _mktemp

#define lseek _lseek

#define pipe _pipe
#define dup2 _dup2
#define dup _dup

#endif

