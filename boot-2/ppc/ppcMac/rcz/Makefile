DIR = rcz
include ../MakePaths.dir

UTILDIR = ../util
LIBSADIR = ../libsa
INSTALLDIR = $(DSTROOT)/LocalDeveloper/Headers/standalone

DEBUG = -g
#CFLAGS	= $(PARENT_CFLAGS) $(DEBUG) $(MORECPP) -Wall
CFLAGS	= -O $(PARENT_CFLAGS) $(MORECPP) $(RC_CFLAGS)
DEFINES=
CONFIG = hd
INC = -I. -I$(SYMROOT) -I$(UTILDIR) -I$(LIBSADIR)
MAKE = make
MKDIRS = mkdir -p
AS = as
LD = ld

VPATH = $(OBJROOT):$(SYMROOT)

SFILES =
CFILES = rcz_compress_mem.c rcz_decompress_file.c rcz_decompress_mem.c
HFILES =
OFILES = $(addprefix $(OBJROOT)/, $(CFILES:.c=.o) $(SFILES:.s=.o))
EXPORTED_HFILES =
INSTALLED_HFILES =
OTHERFILES = Makefile
ALLSRC =  $(SFILES) $(CFILES) $(HFILES) $(OTHERFILES)
DIRS_NEEDED = $(OBJROOT) $(SYMROOT)
LIBS = $(SYMROOT)/librcz.a

all:	$(DIRS_NEEDED) $(LIBS)

rcz:	all

$(LIBS):	$(OFILES)
		-rm -f $@
		ar qcT $@ $^
		ranlib $@

clean::
	rm -rf $(LIBS) $(OFILES)

include ../MakeInc.dir

# dependencies
-include $(OBJROOT)/.dependencies
