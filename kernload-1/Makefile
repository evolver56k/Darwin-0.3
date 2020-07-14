MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
export MAKEFILEDIR

include $(MAKEFILEDIR)/platform.make
include $(MAKEFILEDIR)/commands-$(OS).make

SILENT=@

RELNOTESDIR=/usr/local/RelNotes/
RELNOTES=LoadableKernelServers.rtf
SUBDIR=	include/kernserv libkernload kern_loader loaded_server cmds #examples
BOMDIR=/usr/local/BOMs
USERBOM=kernload.user_bom
SOURCES= Makefile PB.project ${RELNOTES} ${USERBOM}

all tags clean:
	$(SILENT) for i in ${SUBDIR}; \
	do \
		$(ECHO) ================= make $@ for $$i =================; \
		($(CD) $$i; ${MAKE} \
			$${SYMROOT+SYMROOT=$$SYMROOT} \
			$${SYMROOT+SYMDIR=$$SYMROOT/$$i} \
			$${OBJROOT+OBJDIR=$$OBJROOT/$$i} \
			$@); \
	done

installhdrs: DSTROOT ${DSTROOT}
	$(SILENT) $(ECHO) ================= make $@  =================
	$(SILENT) $(CD) include/kernserv; ${MAKE} DSTROOT=$(DSTROOT) \
		$${SYMROOT+SYMDIR=$$SYMROOT/$$i} install

install: DSTROOT ${DSTROOT} ${DSTROOT}${RELNOTESDIR} ${DSTROOT}${BOMDIR}
	install -c -m 444 ${RELNOTES} ${DSTROOT}${RELNOTESDIR}${RELNOTES}
	install -c -m 444 ${USERBOM} ${DSTROOT}${BOMDIR}
	$(SILENT) for i in ${SUBDIR}; \
	do \
		$(ECHO) ================= make $@ for $$i =================; \
		($(CD) $$i; ${MAKE} DSTROOT=$(DSTROOT) \
			$${SYMROOT+SYMROOT=$$SYMROOT} \
			$${SYMROOT+SYMDIR=$$SYMROOT/$$i} \
			$${OBJROOT+OBJDIR=$$OBJROOT/$$i} \
			$@); \
	done

installsrc: SRCROOT ${SRCROOT}
	-$(RM) -rf ${SRCROOT}/*
	$(CP) -p ${SOURCES} ${SRCROOT}
	$(SILENT) CWD=`pwd`; $(CD) ${SRCROOT}; SRCROOT=`pwd`; $(CD) $$CWD; \
	for i in ${SUBDIR}; \
	do \
		$(ECHO) ================= make $@ for $$i =================; \
		($(CD) $$i; \
		  ${MAKE} SRCDIR=$$SRCROOT/$$i $@;) \
	done

SRCROOT DSTROOT:
	$(SILENT) if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		$(ECHO) Must define $@; \
		exit 1; \
	fi

$(SRCROOT) $(DSTROOT) ${DSTROOT}${RELNOTESDIR} ${DSTROOT}${BOMDIR}:
	$(MKDIRS) $@
