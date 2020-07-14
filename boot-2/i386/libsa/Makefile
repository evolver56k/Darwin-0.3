
DIR = libsa
include ../MakePaths.dir

UTILDIR = ../util
INSTALL_SA_DIR = $(DSTROOT)/System/Library/Frameworks/System.framework/Versions/B/PrivateHeaders/standalone
INSTALL_MD_DIR = $(DSTROOT)/System/Library/Frameworks/System.framework/Versions/B/PrivateHeaders/machdep/i386

DEBUG = -O
CFLAGS	= $(RC_CFLAGS) $(DEBUG) $(MORECPP) -arch i386 -g -Wmost -Wno-precomp \
	-munaligned-text -static
DEFINES=
CONFIG = hd
INC = -I. -I$(SYMROOT) -I$(UTILDIR)
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

SA_OBJS = prf.o sprintf.o zalloc.o \
	string1.o memcpy.o memset.o strtol.o mach.o error.o \
	bsearch.o setjmp.o getsegbyname.o qsort.o

SFILES = asm.s bios.s setjmp.s
CFILES = console.c vga.c disk.c misc.c prf.c sys.c \
	ufs_byteorder.c zalloc.c malloc.c table.c \
	string1.c string2.c setjmp.c strtol.c mach.c error.c bsearch.c \
	bootstruct.o qsort.c
HFILES = saio.h ufs_byteorder.h zalloc.h memory.h
EXPORTED_HFILES = libsa.h libsaio.h saio.h io_inline.h kernBootStruct.h \
		memory.h console.h fontio.h
INSTALLED_SA_HFILES = libsa.h
INSTALLED_MD_HFILES = kernBootStruct.h
OTHERFILES = Makefile
ALLSRC =  $(SFILES) $(CFILES) \
	$(HFILES) $(OTHERFILES)
LIBS = libsa.a
DIRS_NEEDED = $(OBJROOT) $(SYMROOT)

all: $(DIRS_NEEDED) $(LIBS)

libsa.a: $(SA_OBJS)
	rm -f $(SYMROOT)/libsa.a
	ar q $(SYMROOT)/libsa.a $(SA_OBJS)
	ranlib $(SYMROOT)/libsa.a

clean::
	rm -rf $(SYMROOT)/libsa.a $(SYMROOT)/libsaio.a

$(INSTALL_SA_DIR) $(INSTALL_MD_DIR):
	$(MKDIRS) $@

installhdrs:: $(INSTALL_SA_DIR) $(INSTALL_MD_DIR)
	cp $(INSTALLED_SA_HFILES) $(INSTALL_SA_DIR)
	cp $(INSTALLED_MD_HFILES) $(INSTALL_MD_DIR)
	
include ../MakeInc.dir

# dependencies
-include $(OBJROOT)/Makedep
