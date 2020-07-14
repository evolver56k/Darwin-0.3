# Building for three target OS's are currently supported:
#
# MacOS X (the default)
#	RC_OS is set to macos (the top level makefile does this)
#	RC_CFLAGS needs no additional flags
#	The code is #ifdef'ed with __Mach30__ is picked up from <mach/mach.h>
# Rhapsody
#	RC_OS is set to teflon
#	RC_CFLAGS needs no additional flags
# Openstep
#	RC_OS is set to nextstep
#	RC_CFLAGS needs the additional flag -D__OPENSTEP__
#
RC_OS = RC_OS
RC_CFLAGS =

APPLE_SUBDIRS = ar file
COMMON_SUBDIRS = libstuff as gprof include misc ld dyld libdyld \
		 libmacho mkshlib otool profileServer RelNotes man

ifeq "nextstep" "$(RC_OS)"
  SUBDIRS = $(COMMON_SUBDIRS)
else
  SUBDIRS = $(APPLE_SUBDIRS) $(COMMON_SUBDIRS)
endif

GNUSRC = /System/Developer/Source/GNU

ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

all clean:
	@if [ $(SRCROOT) ];						\
	then								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    for i in `echo $(SUBDIRS)`;					\
	      do							\
		    echo =========== $(MAKE) $@ for $$i =============;	\
		    (cd $$i; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
			RC_ARCHS="$(RC_ARCHS)" RC_OS="$(RC_OS)"		\
			VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"	\
			DSTROOT=$$DSTROOT				\
			SRCROOT=$(SRCROOT)/$$i				\
			OBJROOT=$(OBJROOT)/$$i				\
			SYMROOT=$(SYMROOT)/$$i $@);			\
	      done							\
	else								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    for i in `echo $(SUBDIRS)`;					\
	      do							\
		    echo =========== $(MAKE) $@ for $$i =============;	\
		    (cd $$i; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
			RC_ARCHS="$(RC_ARCHS)" RC_OS="$(RC_OS)"		\
			DSTROOT=$$DSTROOT $@);				\
	      done							\
	fi

install:
	@if [ $(SRCROOT) ];						\
	then								\
	    projName=`basename $(SRCROOT) | 				\
		sed 's/-[-0-9.]*//' | sed 's/\.cvs//'`;			\
	    if [ "$$projName" = cctools ];				\
	    then							\
	    	target=install_tools;					\
	    elif [ "$$projName" = cctoolslib ];				\
	    then							\
	    	target=lib_ofiles;					\
	    else							\
	        echo "Unknown project name $$projName";			\
		exit 1;							\
	    fi;								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    echo =========== $(MAKE) $$target =============;		\
	    $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"				\
		RC_ARCHS="$(RC_ARCHS)"					\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		DSTROOT=$$DSTROOT					\
		SRCROOT=$(SRCROOT)					\
		OBJROOT=$(OBJROOT)					\
		SYMROOT=$(SYMROOT) $$target;				\
	else								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    echo =========== $(MAKE) install_tools =============;	\
	    $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"				\
		RC_ARCHS="$(RC_ARCHS)"					\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		DSTROOT=$$DSTROOT install_tools;			\
	fi

install_tools:
	@if [ $(SRCROOT) ];						\
	then								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    for i in `echo $(SUBDIRS)`;					\
	      do							\
		    echo =========== $(MAKE) install for $$i =============;	\
		    (cd $$i; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
			RC_ARCHS="$(RC_ARCHS)"				\
			VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"	\
			DSTROOT=$$DSTROOT				\
			SRCROOT=$(SRCROOT)/$$i				\
			OBJROOT=$(OBJROOT)/$$i				\
			SYMROOT=$(SYMROOT)/$$i install);		\
	      done;							\
	    if [ $(RC_RELEASE) ];					\
	    then							\
	      CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;	\
	      for i in `echo $(SUBDIRS)`;				\
	        do							\
		    echo ===== $(MAKE) shlib_clean for $$i ==========;	\
		    (cd $$i; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
			RC_ARCHS="$(RC_ARCHS)"				\
			VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"	\
			DSTROOT=$$DSTROOT				\
			SRCROOT=$(SRCROOT)/$$i				\
			OBJROOT=$(OBJROOT)/$$i				\
			SYMROOT=$(SYMROOT)/$$i shlib_clean);		\
	      done							\
	    fi								\
	else								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    for i in `echo $(SUBDIRS)`;					\
	      do							\
		    echo =========== $(MAKE) install for $$i =============;	\
		    (cd $$i; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
			RC_ARCHS="$(RC_ARCHS)"				\
			VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"	\
			DSTROOT=$$DSTROOT install);			\
	      done							\
	fi
	$(MAKE) installGNUsrc SRCROOT=$(DSTROOT)/$(GNUSRC)/cctools
	chown -fR root.wheel $(DSTROOT)/$(GNUSRC)

lib_ofiles:
	@if [ $(SRCROOT) ];						\
	then								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    echo =========== $(MAKE) all for libstuff =============;	\
	    (cd libstuff; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT					\
		SRCROOT=$(SRCROOT)/libstuff				\
		OBJROOT=$(OBJROOT)/libstuff				\
		SYMROOT=$(SYMROOT)/libstuff all);			\
	    echo =========== $(MAKE) lib_ofiles for ld =============;	\
	    (cd ld; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"			\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT					\
		SRCROOT=$(SRCROOT)/ld					\
		OBJROOT=$(OBJROOT)/ld					\
		SYMROOT=$(SYMROOT)/ld lib_ofiles);			\
	    echo =========== $(MAKE) lib_ofiles for libdyld =============;	\
	    (cd libdyld; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT					\
		SRCROOT=$(SRCROOT)/libdyld				\
		OBJROOT=$(OBJROOT)/libdyld				\
		SYMROOT=$(SYMROOT)/libdyld lib_ofiles);			\
	    echo =========== $(MAKE) lib_ofiles for libmacho =============;	\
	    (cd libmacho; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT					\
		SRCROOT=$(SRCROOT)/libmacho				\
		OBJROOT=$(OBJROOT)/libmacho				\
		SYMROOT=$(SYMROOT)/libmacho lib_ofiles);		\
	else								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    echo =========== $(MAKE) all for libstuff =============;	\
	    (cd libstuff; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT all);					\
	    echo =========== $(MAKE) lib_ofiles for ld =============;	\
	    (cd ld; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"			\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT lib_ofiles);				\
	    echo =========== $(MAKE) lib_ofiles for libdyld =============;	\
	    (cd libdyld; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT lib_ofiles);				\
	    echo =========== $(MAKE) lib_ofiles for libmacho =============;	\
	    (cd libmacho; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT lib_ofiles);				\
	fi

fromGNUsrc:
	@if [ $(SRCROOT) ];						\
	then								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    echo =========== $(MAKE) fromGNUsrc for libstuff =============;\
	    (cd libstuff; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT					\
		SRCROOT=$(SRCROOT)/libstuff				\
		OBJROOT=$(OBJROOT)/libstuff				\
		SYMROOT=$(SYMROOT)/libstuff				\
		CFILES2="" fromGNUsrc);					\
	    echo =========== $(MAKE) all for as =============;		\
	    (cd as; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"			\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT					\
		SRCROOT=$(SRCROOT)/as					\
		OBJROOT=$(OBJROOT)/as					\
		SYMROOT=$(SYMROOT)/as all);				\
	else								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    echo =========== $(MAKE) fromGNUsrc for libstuff =============;\
	    (cd libstuff; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
		RC_ARCHS="$(RC_ARCHS)" CFILES2=""			\
		DSTROOT=$$DSTROOT fromGNUsrc);				\
	    echo =========== $(MAKE) all for as =============;		\
	    (cd as; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"			\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT all);					\
	fi

fromMWsrc:
	@if [ $(SRCROOT) ];						\
	then								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    echo =========== $(MAKE) fromMWsrc for libstuff =============;\
	    (cd libstuff; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT					\
		SRCROOT=$(SRCROOT)/libstuff				\
		OBJROOT=$(OBJROOT)/libstuff				\
		SYMROOT=$(SYMROOT)/libstuff				\
		CFILES2="" fromMWsrc);					\
	    echo =========== $(MAKE) all for ld =============;		\
	    (cd ld; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"			\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT					\
		SRCROOT=$(SRCROOT)/ld					\
		OBJROOT=$(OBJROOT)/ld					\
		SYMROOT=$(SYMROOT)/ld ld_build);			\
	    echo =========== $(MAKE) all for ar =============;	\
	    (cd ar; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"			\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT					\
		SRCROOT=$(SRCROOT)/ar					\
		OBJROOT=$(OBJROOT)/ar					\
		SYMROOT=$(SYMROOT)/ar all);				\
	    echo =========== $(MAKE) libtool.NEW for misc =============; \
	    (cd misc; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"			\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT					\
		SRCROOT=$(SRCROOT)/misc					\
		OBJROOT=$(OBJROOT)/misc					\
		SYMROOT=$(SYMROOT)/misc libtool.NEW);			\
	else								\
	    CWD=`pwd`; cd $(DSTROOT); DSTROOT=`pwd`; cd $$CWD;		\
	    echo =========== $(MAKE) fromMWsrc for libstuff =============;\
	    (cd libstuff; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"		\
		RC_ARCHS="$(RC_ARCHS)" CFILES2=""			\
		DSTROOT=$$DSTROOT fromMWsrc);				\
	    echo =========== $(MAKE) all for ld =============;		\
	    (cd ld; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"			\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT ld_build);				\
	    echo =========== $(MAKE) all for ar =============;		\
	    (cd ar; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"			\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT all);					\
	    echo =========== $(MAKE) libtool.NEW for misc =============; \
	    (cd misc; $(MAKE) RC_CFLAGS="$(RC_CFLAGS)"			\
		RC_ARCHS="$(RC_ARCHS)"					\
		DSTROOT=$$DSTROOT libtool.NEW);				\
	fi

installsrc installGNUsrc installMWsrc: SRCROOT
	$(MKDIRS) $(SRCROOT)
	cp Makefile APPLE_LICENSE $(SRCROOT)
	for i in `echo $(SUBDIRS)`; \
	  do \
		echo =========== $(MAKE) $@ for $$i =============;	\
		(cd $$i; $(MAKE) SRCROOT=$$SRCROOT/$$i $@); \
	  done

installhdrs: $(DSTROOT)
	@if [ $(SRCROOT) ];						\
	then								\
	    projName=`basename $(SRCROOT) | sed 's/-[0-9.]*//'`;	\
	    if [ "$$projName" = cctoolslib ];				\
	    then							\
		cd include; $(MAKE) DSTROOT=$(DSTROOT) install;		\
	    fi;								\
	else								\
	    cd include; $(MAKE) DSTROOT=$(DSTROOT) install;		\
	fi

$(DSTROOT):
	$(MKDIRS) $@

SRCROOT:
	@if [ -n "${$@}" ]; \
	then \
		exit 0; \
	else \
		echo Must define $@; \
		exit 1; \
	fi
