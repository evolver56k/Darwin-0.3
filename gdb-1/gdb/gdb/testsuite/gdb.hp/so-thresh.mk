# Make file for so-thresh test

OBJDIR=.
SRCDIR=.
CFLAGS = +DA1.1 -g

# This is how to build this generator.
gen-so-thresh.o: ${SRCDIR}/gen-so-thresh.c
	$(CC) $(CFLAGS) -o gen-so-thresh.o -c ${SRCDIR}/gen-so-thresh.c
gen-so-thresh: gen-so-thresh.o
	$(CC) $(CFLAGS) -o gen-so-thresh gen-so-thresh.o

# This is how to run this generator.
# This target should be made before the 'all' target,
# to ensure that the shlib sources are all available.
require_shlibs: gen-so-thresh
	if ! [ -a lib00-so-thresh.c ] ; then \
	  gen-so-thresh ; \
	fi
	if ! [ -a lib01-so-thresh.c ] ; then \
	  gen-so-thresh ; \
	fi
	if ! [ -a lib02-so-thresh.c ] ; then \
	  gen-so-thresh ; \
	fi

# This is how to build all the shlibs.
# Be sure to first make the require_shlibs target!
lib00-so-thresh.o: lib00-so-thresh.c
	$(CC) $(CFLAGS) +Z -o lib00-so-thresh.o -c lib00-so-thresh.c
lib00-so-thresh.sl: lib00-so-thresh.o
	$(LD) $(LDFLAGS) -b -o lib00-so-thresh.sl lib00-so-thresh.o
lib01-so-thresh.o: lib01-so-thresh.c
	$(CC) $(CFLAGS) +Z -o lib01-so-thresh.o -c lib01-so-thresh.c
lib01-so-thresh.sl: lib01-so-thresh.o
	$(LD) $(LDFLAGS) -b -o lib01-so-thresh.sl lib01-so-thresh.o
lib02-so-thresh.o: lib02-so-thresh.c
	$(CC) $(CFLAGS) +Z -o lib02-so-thresh.o -c lib02-so-thresh.c
lib02-so-thresh.sl: lib02-so-thresh.o
	$(LD) $(LDFLAGS) -b -o lib02-so-thresh.sl lib02-so-thresh.o




# For convenience, here's names for all pieces of all shlibs.
SHLIB_SOURCES = \
	lib00-so-thresh.c \
	lib01-so-thresh.c \
	lib02-so-thresh.c 

SHLIB_OBJECTS = $(SHLIB_SOURCES:.c=.o)
SHLIBS = $(SHLIB_SOURCES:.c=.sl)
SHLIB_NAMES = $(SHLIB_SOURCES:.c=)
EXECUTABLES = $(SHLIBS) gen-so-thresh so-thresh
OBJECT_FILES = $(SHLIB_OBJECTS) gen-so-thresh.o so-thresh.o

shlib_objects: $(SHLIB_OBJECTS)
shlibs: $(SHLIBS)

# This is how to build the debuggable testcase that uses the shlibs.
so-thresh.o: so-thresh.c
	$(CC) $(CFLAGS) -o so-thresh.o -c so-thresh.c
so-thresh: shlibs so-thresh.o
	$(LD) $(LDFLAGS) -o so-thresh -lc -L${OBJDIR} -c so-thresh.linkopts /opt/langtools/lib/end.o /lib/crt0.o so-thresh.o

# Yeah, but you should first make the require_shlibs target!
all: so-thresh gen-so-thresh

# To remove everything built via this makefile...
clean:
	rm -f lib0*-so-thresh.*
	rm -f *.o gen-so-thresh so-thresh.linkopts so-thresh.c
	rm -f so-thresh
