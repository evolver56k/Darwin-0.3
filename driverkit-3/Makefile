#
# 	top-level Makefile for driverkit project
#

SUBDIR=	libDriver driverkit

#
# Directories for the install_only target (just install, don't build).
#
INSTALL_SUBDIR= libDriver driverkit
RELNOTESDEST= /usr/local/RelNotes
RELNOTESSOURCE= DriverKit.rtf
SOURCEFILES= Makefile DriverKitPrivate.rtf $(RELNOTESSOURCE) \
	MakeInc.simple PB.project
ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

#
# OBJROOT and SYMROOT are overridden by RC tools for builds from read-only
# source.
#
OBJROOT= `pwd`
SYMROOT= `pwd`

all tags clean debug kern:
	@for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		( OBJROOT=$(OBJROOT)/$$i; 				  \
		  SYMROOT=$(SYMROOT)/$$i; 				  \
		  cd $$i; ${MAKE} OBJROOT=$$OBJROOT 			  \
			SYMROOT=$$SYMROOT "RC_CFLAGS=$(RC_CFLAGS)" $@ 	  \
		) || exit $$?; 						  \
	done

installhdrs: DSTROOT ${DSTROOT}
	for i in ${SUBDIR}; 						  \
	do 								  \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} DSTROOT=$(DSTROOT) $@) || exit $$?; 	  \
	done

install: DSTROOT ${DSTROOT} installrelnotes
	for i in ${SUBDIR}; 						  \
	do 								  \
		echo ================= make $@ for $$i =================; \
		( OBJROOT=$(OBJROOT)/$$i; 				  \
		  SYMROOT=$(SYMROOT)/$$i; 				  \
		  cd $$i; 						  \
		  ${MAKE} DSTROOT=$(DSTROOT) OBJROOT=$$OBJROOT	 	  \
			SYMROOT=$$SYMROOT "RC_CFLAGS=$(RC_CFLAGS)" $@ 	  \
		) || exit $$?;						  \
	done

install_only: DSTROOT ${DSTROOT} installrelnotes
	@for i in ${INSTALL_SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		( OBJROOT=$(OBJROOT)/$$i; 				  \
		  SYMROOT=$(SYMROOT)/$$i; 				  \
		  cd $$i; 						  \
		  ${MAKE} DSTROOT=$(DSTROOT) OBJROOT=$$OBJROOT	 	  \
			SYMROOT=$$SYMROOT $@	 			  \
		) || exit $$?;						  \
	done

installsrc: SRCROOT ${SRCROOT}
	-rm -rf ${SRCROOT}/*
	gnutar cf - $(SOURCEFILES) | (cd $(SRCROOT); gnutar xpf -) 
	@CWD=`pwd`; cd ${SRCROOT}; SRCROOT=`pwd`; cd $$CWD; 		  \
	for i in ${SUBDIR}; 						  \
	do 								  \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} SRCROOT=$$SRCROOT/$$i $@) || exit $$?;     \
	done

installrelnotes: DSTROOT $(RELNOTESSOURCE)
	$(MKDIRS) $(DSTROOT)$(RELNOTESDEST)
	rm -f $(DSTROOT)$(RELNOTESDEST)/$(RELNOTESSOURCE)
	cp -p $(RELNOTESSOURCE) $(DSTROOT)$(RELNOTESDEST)

SRCROOT DSTROOT:
	@if [ -n "${$@}" ]; 		\
	then 				\
		exit 0; 		\
	else 				\
		echo Must define $@; 	\
		exit 1; 		\
	fi

$(SRCROOT) $(DSTROOT) $(OBJROOT) $(SYMROOT):
	$(MKDIRS) $@


