
DIR = libsa
include ../MakePaths.dir

UTILDIR = ../util
INSTALL_SA_DIR = $(DSTROOT)/LocalDeveloper/Headers/standalone
INSTALL_MD_DIR = $(DSTROOT)/LocalDeveloper/Headers/machdep/ppc

DEBUG = -g
INC = -I. -I$(SYMROOT) -I$(UTILDIR)
DEFINES=
CFLAGS	= -O $(PARENT_CFLAGS) $(MORECPP) $(INC) $(DEFINES) $(RC_CFLAGS)
CONFIG = hd
MAKE = make
MKDIRS = mkdir -p
AS = as
LD = ld

VPATH = $(OBJROOT):$(SYMROOT)

SFILES = # asm.s
CFILES = strtol.c mach.c error.c bsearch.c string1.c string2.c zalloc.c sprintf.c prf.c getsegbyname.c qsort.c

SAIO_CFILES = console.c disk.c misc.c sys.c \
				ufs_byteorder.c table.c
UNUSED_x86_CFILES = setjmp.c bootstruct.c prf.c sprintf.c

OFILES = $(addprefix $(OBJROOT)/, $(CFILES:.c=.o) $(SFILES:.s=.o))

EXPORTED_HFILES = libsa.h libsaio.h saio.h io_inline.h kernBootStruct.h \
				memory.h console.h fontio.h
INSTALLED_SA_HFILES = libsa.h
INSTALLED_MD_HFILES = kernBootStruct.h
OTHERFILES = Makefile
ALLSRC =  $(SFILES) $(CFILES) \
		$(HFILES) $(OTHERFILES)
LIBS = $(SYMROOT)/libsa.a
DIRS_NEEDED = $(OBJROOT) $(SYMROOT)

all: $(DIRS_NEEDED) $(LIBS)

$(SYMROOT)/libsa.a: $(OFILES)
		-rm -f $@
		ar qcT $@ $(foreach F, $^, $(F))
		ranlib $@

clean::
		-rm -rf $(LIBS)

$(INSTALL_SA_DIR) $(INSTALL_MD_DIR):
		$(MKDIRS) $@

installhdrs:: $(INSTALL_SA_DIR) $(INSTALL_MD_DIR)
		cp $(INSTALLED_SA_HFILES) $(INSTALL_SA_DIR)
		cp $(INSTALLED_MD_HFILES) $(INSTALL_MD_DIR)

include ../MakeInc.dir

# dependencies
-include $(OBJROOT)/.dependencies
