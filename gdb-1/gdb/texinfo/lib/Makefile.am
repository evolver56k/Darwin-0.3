## Makefile.am for texinfo/lib.
## $Id: Makefile.am,v 1.1.1.1 1999/08/04 22:28:12 wsanchez Exp $
## Run automake in .. to produce Makefile.in from this.

noinst_LIBRARIES = libtxi.a
INCLUDES = -I../intl

# Don't need to list alloca.c, etc., Automake includes them.
libtxi_a_SOURCES = getopt.c getopt.h getopt1.c system.h xmalloc.c xstrdup.c
libtxi_a_LIBADD = @LIBOBJS@ @ALLOCA@
libtxi_a_DEPENDENCIES = $(libtxi_a_LIBADD)

## xx configure for bzero?, clib, other common stuff

EXTRA_DIST = README
