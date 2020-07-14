#include <string.h>
#include <stdarg.h>

int snprintf (char *str, size_t n, const char *format, ...)
{
  va_list alist;
  int ret;
  
  va_start (format, alist);
  ret = vsnprintf (str, n, format, alist);
  va_end (alist);
  
  return ret;
}
