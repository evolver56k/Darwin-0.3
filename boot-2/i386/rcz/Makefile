
DIR = rcz
include ../MakePaths.dir

UTILDIR = ../util
LIBSADIR = ../libsa
INSTALLDIR = $(DSTROOT)/System/Library/Frameworks/System.framework/Versions/B/PrivateHeaders/standalone

DEBUG = -O
CFLAGS	= $(RC_CFLAGS) $(DEBUG) $(MORECPP) -arch i386 -g -Wmost -Wno-precomp \
    -munaligned-text -static
DEFINES=
CONFIG = hd
INC = -I. -I$(SYMROOT) -I$(UTILDIR) -I$(LIBSADIR)
MAKE = /bin/gnumake
ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif
AS = as
LD = ld
# LIBS= -lc_static
LIBS=

VPATH = $(OBJROOT):$(SYMROOT)

RCZ_OBJS =rcz_compress_mem.o \
	rcz_decompress_file.o \
	rcz_decompress_mem.o

SFILES =
CFILES = 
HFILES = 
EXPORTED_HFILES =
INSTALLED_HFILES =
OTHERFILES = Makefile
ALLSRC =  $(SFILES) $(CFILES) \
	$(HFILES) $(OTHERFILES)
LIBS = librcz.a
DIRS_NEEDED = $(OBJROOT) $(SYMROOT)

all: $(DIRS_NEEDED) $(LIBS)

librcz.a: $(RCZ_OBJS)
	rm -f $(SYMROOT)/$(@F)
	libtool -o $(SYMROOT)/$(@F) $(RCZ_OBJS)
	ranlib $(SYMROOT)/$(@F)
	
clean::
	rm -rf $(SYMROOT)/librcz.a

include ../MakeInc.dir

# dependencies
-include $(OBJROOT)/Makedep
