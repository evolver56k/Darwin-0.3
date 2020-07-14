#
# Top-level libg++ ("libgpp") Makefile for Apple.
# 
# This Makefile is designed to bridge the gap between
# what Apple/NeXT's Release Control (RC) expects to be
# able to do with Makefiles, and what the FSF/GNU software
# wants to do.
#

# Cannot add "RC_CFLAGS" verbatim here, because it includes all of the
# architectures, and we don't want fat .o files, just fat .a files.

# NT breaks with this OTHER_CFLAGS setting.  Works on Rhapsody.
# Need LOSING_SYS_SIGLIST in order to fix incompatibility of the
# libiberty crap with BSD 4.4 header mistakes.

OTHER_CFLAGS = `echo $(RC_CFLAGS)|sed 's/-arch [a-z0-9]*//g'` -g -fno-common -O2 -DLOSING_SYS_SIGLIST $(CFLAGS)

#
# NT also breaks without the passed-in, null CFLAGS...
#    OTHER_CFLAGS = -g -fno-common -O2
#
# I think it worked only when OTHER_CFLAGS was empty.
#  ON WinNT -->  OTHER_CFLAGS=

OBJROOT = .
SYMROOT = .

###
# DERIVED_DIRT is the top of the hierarchy into which the
# many configure commands output their products, the configured
# Makefiles.  For each target architecture in $(RC_ARCHS), there
# is one first-level subdirectory of DERIVED_DIR, with a name
# like "ppc", "i386", etc.  Below that arch-specific directory is
# where the configured Makefiles get written.
#
DERIVED_DIRT = $(OBJROOT)

#
# In Rhapsody Developer (10/97), there were still a bunch of /Next*
#  and /Local* directories.  These were moved into /System and
#  /Local/* for Rhapsody Premier (11/97).  We use the existence
#  of "/System" (or $NEXT_ROOT/System, on WinNT) to determine that
#  we need to use the new layout.
#

GNUSRC = `if [ -d /System ]; then echo /System/Developer/Source/GNU ; \
          elif [ -d $$NEXT_ROOT/System ]; then echo /System/Developer/Source/GNU ; \
	  else echo /NextDeveloper/Source/GNU ; fi`

#
# Vars for the final copies of the header files, from somewhere
# under SYMROOT, into DSTROOT.  Note that the gnu libg++ build
# process leaves the header files in $(hdrs_from) which ends in a
# "g++-include" directory.   When we create the destination directory,
# we only actually create its parent, $(gxx_include_parent).  That way,
# when we do a "cp -r" from $(hdrs_from), it will not create another
# "g++-include" directory, but instead will copy the contents of that
# directory into a newly-created "g++" directory.
#
# These vars handle all of the necessary headers *except* _G_config.h.
#


# Note:  the following definition no longer works on Rhapsody
# Developer systems, which predated the "/Next*" directory renaming.

gxx_include_parent = `case "${RC_OS}@${RC_RELEASE}" in                  \
 *win*@*) echo Developer ;;                                             \
 *@*4.2mach) echo NextLibrary/Frameworks/System.framework/Versions/A ;; \
 *) echo System/Library/Frameworks/System.framework/Versions/B ;;       \
 esac`/Headers

hdrs_top = $(DSTROOT)/$(gxx_include_parent)
hdrs_to = $(hdrs_top)/g++
hdrs_from = $(SYMROOT)/`case "${RC_OS}" in *win*) echo i386 ;; \
                         *) arch ;; esac`/lib/g++-include
rx_hdr_from = $(SYMROOT)/`case "${RC_OS}" in *win*) echo i386 ;; \
                         *) arch ;; esac`/include

#
# The bindir and libdir which are set here ("bindirt" and "libdirt")
# do not include the target-alias (e.g., ppc-nextstep).  The
# target-alias is appended to these guys in order to form the
# "--bindir" and "--libdir" arguments which are passed to configure,
# and thence on down to any sub-configures.
#

bindirt = $(DSTROOT)/`case "${RC_OS}" in *win*)                         \
                          echo Local/Developer/Executables ;;           \
		       *) echo usr/local/bin ;;                         \
                     esac`
libdirt = $(DSTROOT)/`case "${RC_OS}" in *win*)                         \
			  echo Developer/Libraries ;;                   \
		       *) echo usr/lib ;;                               \
		     esac`
DYLIB_STRIP_OPTS = -S
STRIP = /bin/strip
SYMLINK = ln -s

# Stuff needed in Windows-NT/PDO ports...
EXEEXT = `case "${RC_OS}" in *win*) echo .exe ;; esac`
WINBIN = $$NEXT_ROOT/Developer/Executables
WINFWK_H = $$NEXT_ROOT/Library/Frameworks/System.framework/Headers
WINCXX = "$(WINBIN)/gcc"
WINCC = "$(WINBIN)/gcc -imacros $(WINFWK_H)/winnt-pdo.h"

WINMKDIRX = $(WINBIN)/Utilities/mkdirs.exe

MKDIRS= `if [ -f /bin/mkdirs -o -f $(WINMKDIRX) ] ; then echo mkdirs ; else echo mkdir -p; fi`

all: configure_everything
	@echo @@@ libgpp Makefile '"'all':"'.
	for tarch in $(RC_ARCHS) ; do                                   \
	    inc_dir=`pwd` ;                                             \
	    projVersion="`vers_string -n`" ;                            \
	    DDIR="$(DERIVED_DIRT)/$$tarch" ;                            \
	    cd $$DDIR ;                                                 \
	    case "${RC_OS}" in *win*)                                   \
	       CXX=$(WINCXX); export CXX ;                              \
	       CONFIG_SHELL=sh; export CONFIG_SHELL ;;                  \
	    esac ;                                                      \
	    echo @@@ $$tarch MAKE $@ in `pwd` ;                         \
	    CCXX_FLAGS="-I$$inc_dir -arch $$tarch $(OTHER_CFLAGS)" ;    \
	    $(MAKE) CFLAGS="$$CCXX_FLAGS"                               \
		    CXX="$${CXX-cc}"                                    \
		    CXXFLAGS="$$CCXX_FLAGS"                             \
		    LDFLAGS="-arch $$tarch"                             \
		    OBJROOT="$(OBJROOT)"                                \
		    SYMROOT="$(SYMROOT)"                                \
		    DSTROOT="$(DSTROOT)"                                \
		    CURRENT_PROJECT_VERSION="$$projVersion"             \
		    `case "${RC_OS}" in *win*)                          \
			  echo CONFIG_H=lconfig.h-winnt                 \
			       EXEEXT=.exe                              \
			       NEEDED_LIST=lneeded-list-winnt ;;        \
		      *) echo CONFIG_H=lconfig.h                        \
			       NEEDED_LIST=lneeded-list ;;              \
		     esac`                                              \
		 $@ ;                                                   \
	    cd $$inc_dir ;                                              \
	done
	@echo @@@ libgpp Makefile '"'all':" done'.

#
# Since I can't figure out a way to make either "make all" or
# "make install" depend on the presence of the arch-specific
# subdir of DERIVED_DIRT, I do that test explicitly, within the
# configure_everything rule here, once for each arch in RC_ARCHS.
#

configure_everything:
	@echo @@@ libgpp Makefile '"'configure_everything':"'.
	for tarch in $(RC_ARCHS) ; do                                   \
	    CFG_DIR=`pwd` ;                                             \
	    DDIR="$(DERIVED_DIRT)/$$tarch" ;                            \
	    if [ ! -d $$DDIR ] ; then $(MKDIRS) $$DDIR ; fi ;           \
	    if [ ! -f $$DDIR/Makefile ] ; then                          \
		echo ' @@@' CONFIGURE $$tarch Makefile in $$DDIR ;      \
		cd $$DDIR ;                                             \
		case "${RC_OS}" in                                      \
		   *win*)   CONFIG_SHELL=sh; export CONFIG_SHELL ;      \
			CC=$(WINCC) ; export CC ;                       \
			exeext=.exe ; export exeext ;                   \
			pCFLAGS="\"CFLAGS=-arch $$tarch $(OTHER_CFLAGS)\"" ; \
		        host=i386-nextstep ;                            \
		        TMPDIR=$(OBJROOT)/tmp ; export TMPDIR ;         \
		        if [ ! -d $$TMPDIR ] ; then                     \
		            mkdir $$TMPDIR ;                            \
		        fi ;;                                           \
		   *)   CONFIG_SHELL=/bin/sh; export CONFIG_SHELL ;     \
			CC=cc ; export CC ;                             \
			pCFLAGS="CFLAGS=-arch $$tarch $(OTHER_CFLAGS)" ; \
		        host=`arch`-nextstep ;;                         \
		esac ;                                                  \
		targ=$$tarch-nextstep ;                                 \
		$$CONFIG_SHELL $$CFG_DIR/configure                      \
		   --prefix=$(SYMROOT)/$$tarch                          \
		   --host=$$host                                        \
		   --target=$$targ ;                                    \
		echo '   ' @@@ CONFIGURE $$tarch sub-libraries ;        \
		echo '   ' @@@ DEBUGGING how is make invoked'?' ;       \
		echo '   ' @@@ DEBUGGING make configure-target-libg++ configure-target-libio  \
		 configure-target-librx configure-target-libstdc++      \
			"$$pCFLAGS"                                     \
			`case "${RC_OS}" in *win*)                      \
			      echo CONFIG_H=lconfig.h-winnt             \
				   NEEDED_LIST=lneeded-list-winnt ;;    \
			  *) echo CONFIG_H=lconfig.h                    \
				   NEEDED_LIST=lneeded-list ;;          \
			 esac` ;                                        \
		make configure-target-libg++ configure-target-libio     \
		 configure-target-librx configure-target-libstdc++      \
			"$$pCFLAGS"                                     \
			`case "${RC_OS}" in *win*)                      \
			      echo CONFIG_H=lconfig.h-winnt             \
				   NEEDED_LIST=lneeded-list-winnt ;;    \
			  *) echo CONFIG_H=lconfig.h                    \
				   NEEDED_LIST=lneeded-list ;;          \
			 esac` ;                                        \
		echo cd $$CFG_DIR ;                                     \
		cd $$CFG_DIR ;                                          \
	    fi ;                                                        \
	done
	@echo @@@ libgpp Makefile '"'configure_everything':" done'.

#
# For libg++, for now, the only thing that "installhdrs" does is
#  to configure things.
#
installhdrs: configure_everything
	@echo @@@ libgpp Makefile '"'installhdrs':" done'.

#
# install for each architecture in RC_ARCHS,
#  for each sub-library (libg++, libio, librx, libstdc++),
#    do that library's "make install".
#
# Note:  because it uses the "-C" flag (change to dir before doing
#  anything else including reading Makefiles), this rule depends on
#  the fact that "make" IS gnumake on all NeXT/Apple systems.
#

install_arches: all installhdrs
	@echo @@@ libgpp Makefile '"'install':"'.
	for tarch in $(RC_ARCHS) ; do                                   \
	    case "${RC_OS}" in                                          \
	     *win*) XXXT="EXEEXT=.exe" ;                                \
		    DDIR="$(DERIVED_DIRT)/$$tarch" ;                    \
		    ;;                                                  \
	     *)     XXXT= ;                                             \
		    if [ `arch` = $$tarch ] ; then                      \
			DDIR="$(DERIVED_DIRT)/$$tarch" ;                \
		    else                                                \
			DDIR="$(DERIVED_DIRT)/$$tarch/$$tarch-nextstep" ;\
		    fi ;                                                \
	    esac ;                                                      \
	    echo ' @@@' Make $$tarch  libg++    install        ;        \
	    $(MAKE)         -C $$DDIR/libg++    install $$XXXT ;        \
	    echo ' @@@' Make $$tarch  libio     install        ;        \
	    $(MAKE)         -C $$DDIR/libio     install $$XXXT ;        \
	    echo ' @@@' Make $$tarch  librx     install        ;        \
	    $(MAKE)         -C $$DDIR/librx     install $$XXXT ;        \
	    echo ' @@@' Make $$tarch  libstdc++ install        ;        \
	    $(MAKE)         -C $$DDIR/libstdc++ install $$XXXT ;        \
	done
	@echo @@@ libgpp Makefile: " Copy the cross-built libraries, which"
	@echo @@@ " have been put an extra directory level down, up a level."
	case "${RC_OS}" in                                              \
	 *win*) echo No cross-builds on Windows. ;;                     \
	 *)                                                             \
	    for tarch in $(RC_ARCHS) ; do                               \
		if [ `arch` != $$tarch ] ; then                         \
		    GOOF="$(SYMROOT)/$$tarch/$$tarch-nextstep/lib" ;    \
		    DEST="$(SYMROOT)/$$tarch/lib" ;                     \
		    cp $$GOOF/libg++.a $$DEST ;                         \
		    cp $$GOOF/libstdc++.a $$DEST ;                      \
		fi ;                                                    \
	    done ;                                                      \
	esac
	@echo @@@ 'libgpp Makefile "$@:" per-arch "make install"s done.'

#
# Combine libraries for the different target architectures, to
# make fat libraries.  On OpenStep 4.2, the working architectures
# are sparc, intel ("i386"), and m68k (NeXT's "black hardware").
# On Rhapsody, the relevant architectures are ppc and intel.
#
# Before moving the header files, we remove libc.h (because we have
# a better one in the system framework).
#
combine_arches: install_arches
	@echo @@@ 'libgpp Makefile "$@:"  Move/Copy all of the libraries,'
	@echo @@@ ' binaries, and headers from $(SYMROOT) into $(DSTROOT).'
	@echo @@@ '(------- Create destination directories -------)'
	if [ ! -d $(libdirt) ] ; then $(MKDIRS) $(libdirt) ; fi
	if [ ! -d $(bindirt) ] ; then $(MKDIRS) $(bindirt) ; fi
	if [ ! -d $(hdrs_top) ] ; then $(MKDIRS) $(hdrs_top) ; fi
	rm $(hdrs_from)/libc.h
	@case "${RC_OS}" in                                             \
	 *win*)                                                         \
	    echo @@@ '(No fattening or cross-building on Windows-NT.)' ; \
	    echo @@@ 'DBG Cur dir is ' ; pwd ;                          \
            ntsym="$(SYMROOT)/i386" ;                                   \
            libs_from="$$ntsym/lib" ;                                   \
	    echo @@@ '(------- Copy Libraries -------)' ;               \
	    echo cp "$$libs_from/libg++.a" $(libdirt)  ;                \
	    cp "$$libs_from/libg++.a" $(libdirt)  ;                     \
	    echo cp "$$libs_from/libstdc++.a" $(libdirt)  ;             \
	    cp "$$libs_from/libstdc++.a" $(libdirt)  ;                  \
	    echo cp "$$libs_from/librx.a" $(libdirt)  ;                 \
	    cp "$$libs_from/librx.a" $(libdirt)  ;                      \
	    echo @@@ '(------- Copy Binary -------)' ;                  \
	    echo cp "$$ntsym/bin/gperf.exe" $(bindirt) ;                \
	    cp "$$ntsym/bin/gperf.exe" $(bindirt) ;                     \
	    echo @@@ '(------- Copy Header Files -------)' ;            \
	    echo cp -r $(hdrs_from) $(hdrs_to) ;                        \
	    cp -r $(hdrs_from) $(hdrs_to) ;                             \
	    echo cp -r $(rx_hdr_from)/rx.h $(hdrs_to) ;                 \
	    cp -r $(rx_hdr_from)/rx.h $(hdrs_to) ;                      \
	    echo @@@ '(------- _G_config.h special case -------)' ;     \
	    echo cp $$ntsym/i386-nextstep/_G_config.h $(hdrs_to);       \
	    cp $$ntsym/i386-nextstep/_G_config.h $(hdrs_to);            \
	    ;;                                                          \
	 *)                                                             \
	    echo @@@ 'libgpp Makefile "$@:" per-arch "make install"s done.' ; \
	    echo @@@ 'Debugging:  arch output, target, rc_os, stuff like that,' ; \
	    echo @@@ 'Debugging:  to see if I can find the output libraries.' ; \
	    echo @@@ 'Debugging:  arch sez ' `arch` ;                   \
	    echo @@@ 'Debugging:  RC_ARCHS iz ' $(RC_ARCHS) ;           \
	    echo @@@ 'Debugging:  RC_OS iz ' $(RC_OS) ;                 \
	    echo @@@ @@@ @@@ @@@ @@@ @@@ @@@ @@@ @@@ @@@ @@@ @@@ @@@ ;  \
	    echo @@@ Libraries... ;                                     \
	    echo @@@ '(------ Collect per-arch library names ------)' ; \
	    libgs= ; libios= ; librxs= ; libstds= ;                     \
	    for tarch in $(RC_ARCHS) ; do                               \
		libgs="$$libgs $(SYMROOT)/$$tarch/lib/libg++.a" ;       \
		libstds="$$libstds $(SYMROOT)/$$tarch/lib/libstdc++.a" ;\
		librxs="$$librxs $(SYMROOT)/$$tarch/lib/librx.a" ;      \
	    done ;                                                      \
	    echo @@@ '(------- Fatten and Copy Libraries -------)' ;    \
	    echo libtool -o $(libdirt)/libg++.a $$libgs ;               \
	    libtool -o $(libdirt)/libg++.a $$libgs ;                    \
	    echo libtool -o $(libdirt)/libstdc++.a $$libstds ;          \
	    libtool -o $(libdirt)/libstdc++.a $$libstds ;               \
	    echo libtool -o $(libdirt)/librx.a $$librxs ;               \
	    libtool -o $(libdirt)/librx.a $$librxs ;                    \
	    echo ;                                                      \
	    echo strip -S $(libdirt)/libg++.a ;                         \
	    strip -S $(libdirt)/libg++.a ;                              \
	    echo strip -S $(libdirt)/libstdc++.a ;                      \
	    strip -S $(libdirt)/libstdc++.a ;                           \
	    echo strip -S $(libdirt)/librx.a ;                          \
	    strip -S $(libdirt)/librx.a ;                               \
	    echo ;                                                      \
	    echo @@@ '(------- Copy Binary -------)' ;                  \
	    echo @@@ gperf is the only binary, and it is not cross- ;   \
	    echo @@@ built. \ Therefore, no need to fatten it up. ;     \
	    cp "$(SYMROOT)"/`arch`/bin/gperf $(bindirt) ;               \
	    echo ;                                                      \
	    echo @@@ '(------- Copy Header Files -------)' ;            \
	    echo @@@ Header files, all except _G_config.h, are ;        \
	    echo @@@ architecture-independent, so we install them ;     \
	    echo @@@ outside of the per-arch '"for"' loop. ;            \
	    echo cp -r $(hdrs_from) $(hdrs_to) ;                        \
	    cp -r $(hdrs_from) $(hdrs_to) ;                             \
	    echo cp $(rx_hdr_from)/rx.h $(hdrs_to)/rx.h ;               \
	    cp $(rx_hdr_from)/rx.h $(hdrs_to)/rx.h ;                    \
	    echo @@@ '(------- _G_config.h special case -------)' ;     \
	    echo This header file should be the same on ppc and intel ; \
	    echo but a 1/98 Makefile bug makes the ppc one the better ; \
	    echo one to install, for now. ;                             \
	    echo cp $(SYMROOT)/ppc/ppc-nextstep/_G_config.h $(hdrs_to); \
	    cp $(SYMROOT)/ppc/ppc-nextstep/_G_config.h $(hdrs_to) ;     \
	    ;;                                                          \
	esac
	@echo @@@ libgpp Makefile "$@:" done.

#
# Make sure to keep something in the action section of this rule.
# Otherwise, "make" assumes that the existence of "install.sh"
# means that it should copy install.sh to "install", and then
# chmod "install" so that it's executable.  This won't work if
# your source tree has been copied and made read-only, as RC's
# buildit does here.  Your build will do all its work (over an
# hour), and then indicate failure because it can't create the
# "install" file.
#
install: all installhdrs install_arches combine_arches
	@echo @@@ libgpp Makefile '"'install':" done'.

$(SRCROOT):
	$(MKDIRS) $@

#
# echo tar xfp fails on Rhapsody, the  installsrc command was
#       tar cf - * | (cd $(SRCROOT); tar xfp -)
# and "make" can't even take a "make" action line that consists
# only of a comment!!
#
# 10/97 - using gnutar instead of tar because of Rhapsody kernel bug.
#
installsrc: $(SRCROOT) clean
	gnutar cf - * | (cd $(SRCROOT); gnutar xf -)

installGNUsrc: $(SRCROOT) clean
	gnutar cf - * | (cd $(SRCROOT); gnutar xf -)
	rm -f $(DSTROOT)/$(GNUSRC)/libg++/SGS_ENV

clean:
	-rm -rf $(DERIVED_DIR)
	-rm -f tags .gdb_history
