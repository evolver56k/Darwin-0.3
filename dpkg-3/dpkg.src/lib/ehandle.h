#ifndef _DPKG_EHANDLE_H_
#define _DPKG_EHANDLE_H_

#include <stdarg.h>
#include <setjmp.h>

enum {
  ehflag_normaltidy = 1,
  ehflag_bombout = 2,
  ehflag_recursiveerror = 4
};

struct varbuf;

typedef void (printerror_func) (const char *emsg, const char *contextstring);

void push_error_handler
  (jmp_buf *jbufp, printerror_func *printerror, const char *contextstring);

void set_error_display
(printerror_func *printerror, const char *contextstring);

void print_error_fatal (const char *emsg, const char *contextstring);

/* Unwind the first errorcontext from the stack */

void error_unwind (int flagset);

/* Push a pair of cleanup functions onto the stack of the current
   error context.  These functions will be popped and called in
   reverse. */

void push_cleanup
(void (*f1) (int argc, void **argv), int flagmask1,
 void (*f2) (int argc, void **argv), int flagmask2,
 int nargs, ...);

/* This will arrange that when error_unwind() is called, all
   previous cleanups will be executed with flagset =
   (original_flagset & mask) | value where original_flagset is the
   argument to error_unwind (as modified by any checkpoint which was
   pushed later).  */

void push_checkpoint (int mask, int value);

void pop_cleanup (int flagset);

void do_internerr (const char *string, int line, const char *file)
  __attribute__ ((noreturn));

void ohshit (const char *fmt, ...) __attribute__ ((format (printf, 1, 2), noreturn));
void ohshits (const char *fmt);
void ohshitv (const char *fmt, va_list al) __attribute__ ((noreturn));
void ohshite (const char *fmt, ...) __attribute__ ((format (printf, 1, 2), noreturn));
void ohshitvb (struct varbuf *) __attribute__ ((noreturn ));
void badusage (const char *fmt, ...) __attribute__ ((format (printf, 1, 2), noreturn));
void werr (const char *what) __attribute__ ((noreturn));

#endif /* _DPKG_EHANDLE_H_ */
