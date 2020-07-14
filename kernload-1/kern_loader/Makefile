PROGRAM=kern_loader
DSTDIR=/usr/sbin

include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

CFILES= async.c kern_loader.c load_cmds.c log.c misc.c obj.c server.c
MIGCFILES= kern_loader_reply_user.c kern_loader_handler.c
OFILES= $(CFILES:.c=.o) $(MIGCFILES:.c=.o)
HFILES= async.h load_cmds.h log.h misc.h obj.h server.h
MIGHFILES= kern_loader_reply.h kern_loader_handler.h
MAKEFILE = Makefile
SOURCEFILES= $(MAKEFILE) $(CFILES) $(HFILES)
SRCDIR=..
SYMROOT=$(SYMDIR)/..
LIBS=$(SYMROOT)/libkernload/libkernload.a
MIGDIR=$(SRCDIR)/../include/kernserv

SYSTEM_FRAMEWORK_DIR = $(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework
CFLAGS= -g -O -MD -Wall -I${INCDIR} -I${SYMDIR} -I${SYMROOT}/include -I$(SYSTEM_FRAMEWORK_DIR)/PrivateHeaders  -I$(SYSTEM_FRAMEWORK_DIR)/Headers \
	-DMACH ${RC_CFLAGS} 

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

migfiles: $(SYMDIR)
	@SRCDIR=`pwd`; $(CD) $${SYMDIR-.}; \
	$(MAKE) -f $$SRCDIR/$(MAKEFILE) VPATH=.:$$SRCDIR SRCDIR=$$SRCDIR MIG

MIG:	$(MIGCFILES) $(MIGHFILES)

profile: PROF

debug: DBG

STD PROF DBG: migfiles $(SYMDIR) ALWAYS
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
	    COPTS=${$@_COPTS} INCDIR=$$SRCDIR/../include \
	    NAME=${$@_NAME} $$SYMDIR/${$@_NAME}

install: $(DSTROOT)$(DSTDIR) all
	$(CD) $${SYMDIR=.}; \
	install -c -s ${STD_NAME} $(DSTROOT)$(DSTDIR)/${PROGRAM}

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

async.o kern_loader.o: kern_loader_handler.h kern_loader_reply.h
kern_loader.o log.o: kern_loader_reply.h

kern_loader_handler.c kern_loader_handler.h: $(MIGDIR)/kern_loader.defs
	$(MIG) $? -handler kern_loader_handler.c -user $(NULL) \
		-header $(NULL) -sheader kern_loader_handler.h

kern_loader_reply_user.c kern_loader_reply.h: $(MIGDIR)/kern_loader_reply.defs
	$(MIG) $? -user kern_loader_reply_user.c -header kern_loader_reply.h \
		-handler $(NULL)

ALWAYS:

tags:	ALWAYS
	$(RM) -f tags
	ctags -o tags $(HFILES) $(CFILES) 

-include Makedep
