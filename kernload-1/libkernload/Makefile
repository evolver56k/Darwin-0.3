#
# Makefile for the kern_loader interface library
#

NAME= kernload

include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

DSTDIR= /usr/lib/
LCLDIR= /usr/local/lib/
MIGDIR=$(SRCDIR)/../include/kernserv

SILENT=@

MAKEFILE=Makefile

CFILES= kern_loader_error.c kern_loader_look_up.c
MIGCFILES= kern_loader_user.c kern_loader_reply_handler.c \
	kern_server_user.c kern_server_reply_handler.c
MIGHFILES= kern_loader.h kern_loader_reply_handler.h kern_server.h \
	kern_server_reply_handler.h

SOURCEFILES= $(MAKEFILE) $(CFILES)

CFLAGS= -g -O -MD -Wall -DMACH -I${INCDIR} ${RC_CFLAGS} 
IFLAGS= -c -m 644
OFILES= $(CFILES:.c=.o) $(MIGCFILES:.c=.o)
STRIPFLAGS= -x

BUILDS=STD STATIC
STD_LIB=lib$(NAME).a
STATIC_LIB=lib$(NAME)_static.a
PROF_LIB=lib$(NAME)_p.a
DBG_LIB=lib$(NAME)_g.a
LIB=$(STD_LIB)

STD_COPTS=
STATIC_COPTS= -static
PROF_COPTS= -pg
DBG_COPTS= -DDEBUG

all:	 $(BUILDS)

migfiles: $(SYMDIR)
	$(SILENT) SRCDIR=`pwd`; $(CD) $${SYMDIR-.}; \
	$(MAKE) -f $$SRCDIR/$(MAKEFILE) VPATH=.:$$SRCDIR SRCDIR=$$SRCDIR MIG

MIG:	$(MIGCFILES) $(MIGHFILES)

profile: PROF

debug: DBG

STD STATIC PROF DBG: migfiles $(SYMDIR) ALWAYS
	$(SILENT) if [ "${OBJDIR}" = "" ]; then \
	    SRCDIR=..; \
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
	    LIB=${$@_LIB} $$SYMDIR/${$@_LIB}

${SYMDIR}/${LIB}: $(OFILES) vers.o
	$(SILENT) $(RM) -f $@
	libtool -o $@ $(OFILES) vers.o
	$(RANLIB) $@

install: $(DSTROOT)$(DSTDIR) $(DSTROOT)$(LCLDIR) all
	$(CD) $${SYMDIR=.}; \
	    install $(IFLAGS) lib$(NAME).a $(DSTROOT)$(DSTDIR); \
	    $(STRIP) $(STRIPFLAGS) $(DSTROOT)$(DSTDIR)/lib$(NAME).a; \
	    $(RANLIB) $(DSTROOT)$(DSTDIR)/lib$(NAME).a;
	$(CD) $${SYMDIR=.}; \
	    install $(IFLAGS) lib$(NAME)_static.a $(DSTROOT)$(LCLDIR); \
	    $(STRIP) $(STRIPFLAGS) $(DSTROOT)$(LCLDIR)/lib$(NAME)_static.a; \
	    $(RANLIB) $(DSTROOT)$(LCLDIR)/lib$(NAME)_static.a

.c.o:
	$(CC) $(COPTS) $(CFLAGS) -c -o ./$*.o $<
	md -u Makedep -d $*.d

kern_loader_user.c kern_loader.h: $(MIGDIR)/kern_loader.defs
	$(MIG) $? -user kern_loader_user.c -header kern_loader.h -server $(NULL)

kern_loader_reply_handler.c kern_loader_reply_handler.h: \
		$(MIGDIR)/kern_loader_reply.defs
	$(MIG) $? -handler kern_loader_reply_handler.c -user $(NULL) \
		-header $(NULL) -sheader kern_loader_reply_handler.h
		
kern_server_user.c kern_server.h: $(MIGDIR)/kern_server.defs
	$(MIG) $? -user kern_server_user.c -header kern_server.h -server $(NULL)

kern_server_reply_handler.c kern_server_reply_handler.h: \
		$(MIGDIR)/kern_server_reply.defs
	$(MIG) $? -handler kern_server_reply_handler.c -user $(NULL) \
		-header $(NULL) -sheader kern_server_reply_handler.h

installsrc: $(SRCDIR)
	$(CP) -p $(SOURCEFILES) $(SRCDIR)

$(SYMDIR) $(DSTROOT)$(DSTDIR) $(DSTROOT)$(LCLDIR) $(SRCDIR):
	$(MKDIRS) $@

vers.o: $(OFILES)
	$(SILENT) $(RM) -f $*.c
	$(VERS_STRING) -l lib$(NAME).a > $*.c
	$(CC) -c ${CFLAGS} $*.c

clean:
	-$(RM) -rf $(MIGCFILES) $(MIGHFILES) \
		${SYMDIR} ${OBJDIR} *_OBJS \
		lib$(NAME)*.a*

ALWAYS:

-include Makedep
