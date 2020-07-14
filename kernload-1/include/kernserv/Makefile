include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

SYSTEM_FRAMEWORK_DIR=$(SYSTEM_LIBRARY_DIR)/Frameworks/System.framework
VERSION_DIR=Versions/B
DSTDIR=$(SYSTEM_FRAMEWORK_DIR)/$(VERSION_DIR)/Headers/kernserv

MAKEFILE = Makefile
INSTFLAGS=-c -m 444
MIGFILES=kern_loader.defs kern_loader_reply.defs \
	kern_server.defs kern_server_reply.defs
HFILES= kern_loader_types.h kern_loader_error.h
MIG_GENERATED= kern_loader.h kern_loader_reply_handler.h \
	kern_server.h kern_server_reply.h kern_server_reply_handler.h
MIG_GENERATED_INSTALL= kern_loader.h kern_loader_reply_handler.h \
	kern_server_reply_handler.h
SOURCEFILES= $(MAKEFILE) $(MIGFILES) $(HFILES) 

MIGFLAGS= -handler $(NULL) -user $(NULL)

.SUFFIXES: .defs .h

all migfiles:	$(SYMDIR) ALWAYS
	@SRCDIR=`pwd`; $(CD) $${SYMDIR-.}; \
	$(MAKE) -f $$SRCDIR/$(MAKEFILE) VPATH=.:$$SRCDIR MIG

MIG: $(MIG_GENERATED)

.defs.h:
	$(MIG) $(MIGFLAGS) -header $@ $?

kern_loader_reply_handler.h: kern_loader_reply.defs
	$(MIG) $(MIGFLAGS) -sheader $@ -header $(NULL) $?

kern_server_reply.h kern_server_reply_handler.h: kern_server_reply.defs
	$(MIG) $(MIGFLAGS) -sheader kern_server_reply_handler.h \
		-header kern_server_reply.h \
		-user $(NULL) \
		-server $(NULL) $?

clean:
	-$(RM) -f $(MIG_GENERATED)

install: $(DSTROOT)$(DSTDIR) all
	@for i in $(HFILES); \
	do \
		install ${INSTFLAGS} $$i ${DSTROOT}/${DSTDIR}; \
	done
	@$(CD) $${SYMDIR=.}; for i in $(MIG_GENERATED_INSTALL); \
	do \
		install ${INSTFLAGS} $$i ${DSTROOT}/${DSTDIR}; \
	done

install:

installsrc: $(SRCDIR)
	$(CP) -p $(SOURCEFILES) $(SRCDIR)

$(SYMDIR) ${SRCDIR} $(DSTROOT)$(DSTDIR):
	$(MKDIRS) $@

ALWAYS:
