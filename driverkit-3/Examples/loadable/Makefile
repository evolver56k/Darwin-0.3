ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

SUBDIR=	Kernel User
SOURCEFILES= Makefile README.rtf 
OBJROOT= `pwd`
SYMROOT= `pwd`

all tags clean debug:
	@for i in ${SUBDIR}; \
	do \
		echo ================= make $@ for $$i =================; \
		( OBJROOT=$(OBJROOT)/$$i; 				  \
		  SYMROOT=$(SYMROOT)/$$i; 				  \
		  cd $$i; ${MAKE} OBJROOT=$$OBJROOT 			  \
			SYMROOT=$$SYMROOT "RC_CFLAGS=$(RC_CFLAGS)" $@ 	  \
		) || exit $?; 						  \
	done

install installhdrs install_only: DSTROOT ${DSTROOT}
	@for i in ${SUBDIR}; 						  \
	do 								  \
		echo ================= make $@ for $$i =================; \
		( OBJROOT=$(OBJROOT)/$$i; 				  \
		  SYMROOT=$(SYMROOT)/$$i; 				  \
		  cd $$i; 						  \
		  ${MAKE} DSTROOT=$(DSTROOT) OBJROOT=$$OBJROOT	 	  \
			SYMROOT=$$SYMROOT "RC_CFLAGS=$(RC_CFLAGS)" $@ 	  \
		) || exit $?;						  \
	done

installsrc: SRCROOT ${SRCROOT}
	-rm -rf ${SRCROOT}/*
	gnutar cf - $(SOURCEFILES) | (cd $(SRCROOT); gnutar xpf -) 
	@CWD=`pwd`; cd ${SRCROOT}; SRCROOT=`pwd`; cd $$CWD; 		  \
	for i in ${SUBDIR}; 						  \
	do 								  \
		echo ================= make $@ for $$i =================; \
		(cd $$i; ${MAKE} SRCROOT=$$SRCROOT/$$i $@) || exit $?;     \
	done

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


