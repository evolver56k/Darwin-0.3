#
# Makefile for the generic kernel server

NAME= loadserv

include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

DSTDIR= /usr/lib/
LCLDIR= /usr/local/lib/

CFILES= xpr_stub.c
MAKEFILE = Makefile
SOURCEFILES= $(MAKEFILE) $(CFILES) $(HFILES)

SYSTEM_FRAMEWORK_DIR = $(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework
INCDIR=$(SYSTEM_FRAMEWORK_DIR)/Headers
CFLAGS= -static -g -O -MD -Wimplicit -DKERNEL -D_KERNEL -DMACH ${RC_CFLAGS}
IFLAGS= -c -m 644
OFILES= $(CFILES:.c=.o) $(MIGCFILES:.c=.o)
MIGKERNEL=-typed
SYMDIR=SYMBOLS

BUILDS=STD
STD_LIB=lib$(NAME).a
PROF_LIB=lib$(NAME)_p.a
DBG_LIB=lib$(NAME)_g.a
LIB=$(STD_LIB)

STD_COPTS=
PROF_COPTS= -pg
DBG_COPTS= -DDEBUG

all:	 $(BUILDS)

profile: PROF

debug: DBG

STD PROF DBG: migfiles $(SYMDIR) ALWAYS
	@if [ "${OBJDIR}" = "" ]; then \
	    SRCDIR=..; \
	    OBJDIR=$@_OBJS; \
	else \
	    SRCDIR=`pwd`; \
	    OBJDIR=${OBJDIR}/$@_OBJS; \
	fi; \
	$(MKDIRS) ${SYMDIR}; $(CD) ${SYMDIR}; SYMDIR=`pwd`; $(CD) $$SRCDIR; \
	$(MKDIRS) $$OBJDIR; $(CD) $$OBJDIR; \
	$(ECHO) make libmig up to date; \
	$(MAKE) -f $$SRCDIR/$(MAKEFILE) VPATH=.:$$SYMDIR:$$SRCDIR \
	    SRCDIR=$$SRCDIR SYMDIR=$$SYMDIR MIGOFILES; \
	$(ECHO) make $$SYMDIR/${$@_LIB}; \
	$(MAKE) -f $$SRCDIR/$(MAKEFILE) VPATH=.:$$SYMDIR:$$SRCDIR \
	    SRCDIR=$$SRCDIR SYMDIR=$$SYMDIR \
	    COPTS=${$@_COPTS} LIB=${$@_LIB} $$SYMDIR/${$@_LIB}

.c.o:
	$(CC) $(CFLAGS) -c -o $*.o $<
	md -u Makedep -d $*.d

installsrc: $(SRCDIR)
	$(CP) -p $(SOURCEFILES) $(SRCDIR)

${SYMDIR}/${LIB}: $(OFILES) libmig vers.o
	@$(RM) -f $@
	libtool -o $@ -a *.o

install: $(DSTROOT)$(DSTDIR) $(DSTROOT)$(LCLDIR) all
	$(CD) ${SYMDIR}; \
	    install $(IFLAGS) lib$(NAME).a $(DSTROOT)$(DSTDIR); \
	    $(STRIP) -x $(DSTROOT)$(DSTDIR)/lib$(NAME).a; \
	    $(RANLIB) $(DSTROOT)$(DSTDIR)/lib$(NAME).a
	$(CD) ${SYMDIR}; \
	    install $(IFLAGS) lib$(NAME).a $(DSTROOT)$(LCLDIR); \
	    $(RANLIB) $(DSTROOT)$(LCLDIR)/lib$(NAME).a;

GEN_HFILES=mach_interface.h mach_user_internal.h mach_host.h mach_host_user_internal.h exc.h exc_user_internal.h
GEN_MIGFILES=mach_migfiles mach_host_migfiles exc_migfiles

migfiles:	${SYMDIR} ALWAYS
	SRCDIR=`pwd`; $(CD) ${SYMDIR}; \
	${MAKE} -f $$SRCDIR/$(MAKEFILE) VPATH=.:$$SRCDIR \
	    OBJDIR=$${OBJDIR=$$SRCDIR}/SYMBOLS.TMP MIG

MIG:	Makefile.gen ALWAYS

libmig: Makefile.gen ${GEN_MIGFILES}
	${MAKE} -f ${SYMDIR}/Makefile.gen libmig

MIGOFILES:	ALWAYS
	${MAKE} -f ${SYMDIR}/Makefile.gen libmig

Makefile.gen: $(GEN_HFILES) $(GEN_MIGFILES)
	$(RM) -f Makefile.gen
	$(ECHO) "CFILES=`$(ECHO) *.c`"			> Makefile.gen
	$(ECHO)						>> Makefile.gen
	$(ECHO) 'OFILES=$$(CFILES:.c=.o)'			>> Makefile.gen
	$(ECHO) 'CFLAGS=-static -g -O -MD -Wall -DMACH -DKERNEL_FEATURES $${RC_CFLAGS}' \
							>> Makefile.gen
	$(ECHO)						>> Makefile.gen
	$(ECHO) 'libmig: $$(OFILES)'			>> Makefile.gen
	$(ECHO) '	-$(RM) -rf $$@'				>> Makefile.gen
	$(ECHO) '	touch $$@'				>> Makefile.gen
	$(ECHO) '	-md -u Makedep -d *.d'			>> Makefile.gen
	$(ECHO) '-include Makedep'				>> Makefile.gen

mach_interface.h:
	@$(RM) -f mach_interface.h
	$(ECHO) '#include "mach_user_internal.h"'		> mach_interface.h
	$(ECHO) '#include <mach/mach_kernloader.h>'	>> mach_interface.h

mach_migfiles: $(INCDIR)/mach/mach.defs $(FEATURE_FILES)
	$(RM) -fr ${OBJDIR}; \
	$(MKDIRS) ${OBJDIR}; \
	$(CD) ${OBJDIR}; \
	${MIG} ${MIGOPTS} ${MIGKERNEL} -i -server $(NULL) \
		-header mach_interface.h $(INCDIR)/mach/mach.defs
	$(RM) -f ${OBJDIR}/xxx_*
	$(CD) ${OBJDIR}; \
	for i in *port_extract*.c; \
	    do $(MV) $$i `$(ECHO) $$i | $(SED) 's/port_extract/p_extract/'`; done; \
	for i in *port_insert*.c; \
	    do $(MV) $$i `$(ECHO) $$i | $(SED) 's/port_insert/p_insert/'`; done; \
	for i in *port_set*.c; \
	    do $(MV) $$i `$(ECHO) $$i | $(SED) 's/port_set/pset/'`; done
	SYMDIR=`pwd`; $(CD) ${OBJDIR}; $(CP) -p *.c $$SYMDIR
	$(RM) -f mach_migfiles; touch mach_migfiles
	$(RM) -rf ${OBJDIR}

#mach_user_internal.h: $(INCDIR)/mach/mach_interface.h
mach_user_internal.h: $(INCDIR)/mach/mach_kernloader.h
	@$(RM) -f mach_user_internal.h
	-egrep 'Routine ' < $(INCDIR)/mach/mach_kernloader.h | $(SED) -e 's/.*Routine \([a-z_]*\).*/#define \1 \1_EXTERNAL/' > mach_user_internal.h

mach_host.h:
	@$(RM) -f mach_host.h
	$(ECHO) '#include "mach_host_user_internal.h"'	> mach_host.h
	$(ECHO) '#include <mach/mach_host.h>'		>> mach_host.h

mach_host_migfiles: $(INCDIR)/mach/mach_host.defs $(FEATURE_FILES)
	$(RM) -fr ${OBJDIR}; $(MKDIRS) ${OBJDIR}
	$(CD) ${OBJDIR}; ${MIG} ${MIGOPTS} ${MIGKERNEL} -i \
		-server $(NULL) \
		-header mach_host.h $(INCDIR)/mach/mach_host.defs
	$(RM) -f ${OBJDIR}/xxx_*
	$(CD) ${OBJDIR}; \
	for i in *processor_set*.c; \
	    do $(MV) $$i `$(ECHO) $$i | $(SED) 's/processor_set/prset/'`; done; \
	for i in *processor*.c; \
	    do $(MV) $$i `$(ECHO) $$i | $(SED) 's/processor/proc/'`; done
	SYMDIR=`pwd`; $(CD) ${OBJDIR}; $(CP) -p *.c $$SYMDIR
	$(RM) -f mach_host_migfiles; touch mach_host_migfiles
	$(RM) -rf ${OBJDIR}

mach_host_user_internal.h: $(INCDIR)/mach/mach_host.h
	@$(RM) -f mach_host_user_internal.h
	-egrep 'Routine ' < $(INCDIR)/mach/mach_host.h | $(SED) -e 's/.*Routine \([a-z_]*\).*/#define \1 \1_EXTERNAL/' > mach_host_user_internal.h

exc.h:
	@$(RM) -f exc.h
	$(ECHO) '#include "exc_user_internal.h"'		> exc.h
	$(ECHO) '#include <mach/exc.h>'			>> exc.h

exc_migfiles: $(INCDIR)/mach/exc.defs $(FEATURE_FILES)
	$(RM) -fr ${OBJDIR}; $(MKDIRS) ${OBJDIR}
#	$(CD) ${OBJDIR}; ${MIG} ${MIGOPTS} ${MIGKERNEL} -i -server $(NULL) \
#		-header exc.h $(INCDIR)/mach/exc.defs
	$(CD) ${OBJDIR}; ${MIG} ${MIGOPTS}  -i -server $(NULL) \
		-header exc.h $(INCDIR)/mach/exc.defs
	$(RM) -f ${OBJDIR}/xxx_*
	SYMDIR=`pwd`; $(CD) ${OBJDIR}; $(CP) -p *.c $$SYMDIR
	$(RM) -f exc_migfiles; touch exc_migfiles
	$(RM) -rf ${OBJDIR}

exc_user_internal.h: $(INCDIR)/mach/exc.h
	@$(RM) -f exc_user_internal.h
	-egrep 'Routine ' < $(INCDIR)/mach/exc.h | $(SED) -e 's/.*Routine \([a-z_]*\).*/#define \1 \1_EXTERNAL/' > exc_user_internal.h

$(FEATURE_FILES): $(FEATURES)
	@$(RM) -f $@
	@def=`$(ECHO) $* | tr '[a-z]' '[A-Z]'`;				\
	 $(ECHO) -n "#define $$def " > $@;					\
	 (cat $(FEATURES); $(ECHO) $$def) | $(CPP) -DKERNEL_FEATURES | tail -1 >> $@

$(SYMDIR) $(DSTROOT)$(DSTDIR) $(DSTROOT)$(LCLDIR) $(SRCDIR):
	$(MKDIRS) $@

vers.o: $(OFILES)
	@$(RM) -f $*.c
	$(VERS_STRING) -l lib$(NAME).a > $*.c
	$(CC) -c $*.c $(CFLAGS)

clean:
	-$(RM) -rf $(MIGCFILES) $(MIGHFILES) \
		${SYMDIR} ${OBJDIR} *_OBJS \
		lib$(NAME)*.a*

ALWAYS:

-include Makedep
