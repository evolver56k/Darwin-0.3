DIR = libsaio
include ../MakePaths.dir

UTILDIR = ../util
LIBSADIR = ../libsa
INSTALLDIR = $(DSTROOT)/LocalDeveloper/Headers/standalone

DEBUG = -g
INC = -I. -I$(SYMROOT) -I$(UTILDIR) -I$(LIBSADIR)
DEFINES=
CFLAGS	= -O $(INC) $(PARENT_CFLAGS) $(MORECPP) $(RC_CFLAGS) \
	-DSAIO_INTERNAL_USER $(DEFINES)
CONFIG = hd
MAKE = make
MKDIRS = mkdir -p
AS = as
LD = ld

VPATH = $(OBJROOT):$(SYMROOT)

CFILES = 	reallyPrint.c misc.c gets.c font.c \
		sys.c cache.c \
		ufs_byteorder.c unpackbits.c\
		stringTable.c load.c disk.c \
		localize.c \
		choose.c bitmap.c \
		localPrintf.c

DOS_CFILES = console.c bootstruct.c drivers.c printf.c localVPrintf.c errorV.c

SFILES = # asm.s
HFILES =
OFILES = $(addprefix $(OBJROOT)/, $(CFILES:.c=.o) $(SFILES:.s=.o))
EXPORTED_HFILES =
INSTALLED_HFILES =
OTHERFILES = Makefile

LIBS = $(SYMROOT)/libsaio.a
DIRS_NEEDED = $(OBJROOT) $(SYMROOT)

all: $(DIRS_NEEDED) libsaio.h $(LIBS)

$(SYMROOT)/libsaio.a: $(OFILES)
		-rm -f $@
		ar qcT $@ $^
		ranlib $@

clean::
		rm -rf $(LIBS) $(OFILES)

include ../MakeInc.dir

# dependencies
-include $(OBJROOT)/.dependencies
