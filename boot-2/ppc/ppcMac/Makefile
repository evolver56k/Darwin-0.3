#	Makefile for ppc boot program

DEFINES = -D__ARCHITECTURE__=\\\"ppc\\\" -D_NEXT_SOURCE \
			-D__BIG_ENDIAN__ -D__LITTLE_ENDIAN__=0 -DNeXT \
			-DGENERATINGPOWERPC -DDEBUG \
			-DPPC -Dppc -D__PPC__ -D_BIG_ENDIAN
CONFIG = hd
LIBDIR = libsa
INCDIR = /NextLibrary/Frameworks/System.framework/Versions/B/Headers
INC = -I. -I$(LIBDIR) -I$(INCDIR)/ansi -I$(INCDIR)/bsd \
		-I$(INCDIR) -I../../include/mac
ARCHLESS_RC_CFLAGS=`echo $(RC_CFLAGS)|sed 's/-arch [a-z0-9]*//g'`
CFLAGS	= -O -x objective-c $(MORECPP) $(INC) -static $(DEFINES) \
			-Wno-import -Wno-unused -nostdlib \
			-mcpu=604 -traditional-cpp -fno-builtin \
			-finline -msoft-float -arch ppc
MAKE = make
MKDIRS = mkdir -p
AS = as
LD = ld
# LIBS= -lc_static

#
# these paths are only valid in subdirectories of this directory
#
OBJROOT:=`pwd`/../../obj/ppc
SYMROOT:=`pwd`/../../sym/ppc
DSTROOT:=`pwd`/../../dst/ppc
SRCROOT:=/tmp

VPATH = $(OBJROOT):$(SYMROOT)

# SUBDIRS = rcz util libsa libsaio sarld boot2 boot1 boot0 strings
SUBDIRS = rcz libsa libsaio sarld

define DO_SUBDIR
		( \
		  cd $$i ; \
		  export DEPENDENCIES_OUTPUT=$(OBJROOT)/$$i/.dependencies ; \
		  ${MAKE} \
			 "OBJROOT=$(OBJROOT)/$$i" \
			 "SYMROOT=$(SYMROOT)" \
			 "DSTROOT=$(DSTROOT)" \
			 "SRCROOT=$(SRCROOT)" \
			 "RC_ARCHS=$(RC_ARCHS)" \
			 "RC_KANJI=$(RC_KANJI)" \
			 "JAPANESE=$(JAPANESE)" \
			 "PARENT_CFLAGS=$(CFLAGS)" \
			 "RC_CFLAGS=$(ARCHLESS_RC_CFLAGS)" $@ \
		) || exit $?
endef

all tags clean debug install installhdrs:
	@for i in ${SUBDIRS}; do $(DO_SUBDIR); done

$(SUBDIRS)::
	@i=$@; $(DO_SUBDIR)

installsrc:
	tar cf - . | (cd ${SRCROOT}; tar xfBp -)

$(SYMROOT) $(OBJROOT):
	$(MKDIRS) $@


