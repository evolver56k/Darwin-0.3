#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>

int vsnprintf (char *buf, size_t maxsize, const char *fmt, va_list al) 
{
  static FILE *file= 0;

  struct stat stab;
  unsigned long want, nr;
  int retval;

  if (maxsize == 0) { return -1; }
  if (!file) {
    file= tmpfile ();
    if (!file) {
      return -1; 
    }
  } else {
    if (fseek (file, 0, 0)) { return -1; }
    if (ftruncate (fileno (file), 0)) { return -1; }
  }
  if (vfprintf (file, fmt, al) == EOF) { return -1; }
  if (fflush (file)) { return -1; }
  if (fstat (fileno (file), &stab)) { return -1; }
  if (fseek (file, 0, 0)) { return -1; }
  want = stab.st_size;
  if (want >= maxsize) {
    want = maxsize - 1;
    retval = -1;
  } else {
    retval = want;
  }
  nr= fread (buf, 1, want - 1, file);
  if (nr != want-1) { return -1; }
  buf[want]= 0;

  return retval;
}
