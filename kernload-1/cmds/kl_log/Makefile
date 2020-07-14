PROGRAM=kl_log

include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

CFILES=kl_log.c
OFILES=$(CFILES:.c=.o)
DSTDIR=/usr/sbin
MAKEFILE = Makefile
SOURCEFILES= $(MAKEFILE) $(CFILES) $(HFILES)
SRCDIR=..
SYMROOT=$(SYMDIR)/../..
LIBS=$(SYMROOT)/libkernload/libkernload.a

CFLAGS= -g -O $(DEBUG) -MD -Wimplicit -I${INCDIR} \
	-I${SYMROOT}/include ${RC_CFLAGS} 

#
# Options for release builds
#
STD_COPTS=
STD_NAME=$(PROGRAM)

#
# Options for profiling builds
#
PROF_COPTS= -pg
PROF_NAME=$(PROGRAM)_PROF

#
# Options for debugging builds
#
DBG_COPTS= -DDEBUG
DBG_NAME=$(PROGRAM)_DBG

NAME=$(STD_NAME)

BUILDS=STD

all:	$(BUILDS)

profile: PROF

debug: DBG

STD PROF DBG: $(SYMDIR) ALWAYS
	@if [ "${OBJDIR}" = "" ]; then \
	    SRCDIR=${SRCDIR}; \
	    OBJDIR=$@_OBJS; \
	else \
	    SRCDIR=`pwd`; \
	    OBJDIR=${OBJDIR}/$@_OBJS; \
	fi; \
	: $${SYMDIR=$$SRCDIR}; \
	$(MKDIRS) $$OBJDIR; $(CD) $$OBJDIR; \
	SYMDIR=`(cd $$SYMDIR; pwd)`; \
	$(MAKE) -f $$SRCDIR/$(MAKEFILE) VPATH=.:$$SYMDIR:$$SRCDIR \
	    SRCDIR=$$SRCDIR SYMDIR=$$SYMDIR \
	    COPTS=${$@_COPTS} INCDIR=$$SRCDIR/../../include \
	    NAME=${$@_NAME} \
	    $$SYMDIR/${$@_NAME}

install: $(DSTROOT)$(DSTDIR) all
	$(CD) $${SYMDIR=.}; install -c -s ${STD_NAME} $(DSTROOT)$(DSTDIR)/${PROGRAM}

installsrc: $(SRCDIR)
	$(CP) -p $(SOURCEFILES) $(SRCDIR)

clean:
	-$(RM) -rf $(MIGCFILES) $(MIGHFILES) ${SYMDIR} ${OBJDIR} *_OBJS \
		${STD_NAME} ${PROF_NAME} ${DBG_NAME}

$(SYMDIR) $(DSTROOT)$(DSTDIR) $(SRCDIR):
	$(MKDIRS) $@

${SYMDIR}/${NAME}: $(OFILES) vers.o
	${CC} ${CFLAGS} -o $@ $(OFILES) vers.o ${LIBS}

.c.o:
	$(CC) $(COPTS) $(CFLAGS) -c -o $*.o $<
	md -u Makedep -d $*.d

vers.o: $(OFILES)
	@$(RM) -f $*.c
	$(VERS_STRING) -c ${NAME} > $*.c
	$(CC) -c $*.c $(CFLAGS)

ALWAYS:

-include Makedep
