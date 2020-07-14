OFLAG = -O
MIG = $(NEXT_ROOT)/usr/bin/mig

ifneq "mwcc" "$(notdir $(CC))"
  GCC_FLAGS = -Wall -Wno-precomp
endif
CFLAGS = -g -I$(SRCROOT) -I$(SRCROOT)/../include -I$(OFILE_DIR) $(GCC_FLAGS) \
	-I$(NEXT_ROOT)/System/Library/Frameworks/System.framework/PrivateHeaders

ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

SRCROOT = .
SYMROOT = .
OBJROOT = .
OFILE_DIRS = $(OBJROOT)/ld_dir $(OBJROOT)/shlib_obj $(OBJROOT)/profile_obj \
	     $(OBJROOT)/optimized_obj $(OBJROOT)/dynamic_obj \
	     $(OBJROOT)/ptmp_obj $(OBJROOT)/otmp_obj $(OBJROOT)/dtmp_obj \
	     $(OBJROOT)/sarld_dir

BINDIR = /bin
USRBINDIR = /usr/bin
LOCLIBDIR = /usr/local/lib
SYSTEMDIR = /Local/Developer/System

HFILES = ld.h pass1.h objects.h sections.h cstring_literals.h symbols.h \
	 fvmlibs.h layout.h specs.h pass2.h generic_reloc.h hash_string.h \
	 sets.h 4byte_literals.h 8byte_literals.h literal_pointers.h dylibs.h \
	 indirect_sections.h mod_sections.h i860_reloc.h ppc_reloc.h \
	 m88k_reloc.h hppa_reloc.h sparc_reloc.h
CFILES = ld.c pass1.c objects.c sections.c cstring_literals.c symbols.c \
	 fvmlibs.c layout.c specs.c pass2.c generic_reloc.c rld.c sets.c \
	 4byte_literals.c 8byte_literals.c literal_pointers.c dylibs.c \
	 indirect_sections.c mod_sections.c i860_reloc.c ppc_reloc.c \
	 m88k_reloc.c hppa_reloc.c sparc_reloc.c
OBJS = $(CFILES:.c=.o)
INSTALL_FILES = $(CFILES) $(HFILES) Makefile notes \
		make.defs make_defs.h librld.ofileList

teflon macos:

nextstep: shlib_ofiles

all:	$(OFILE_DIRS) $(SYMROOT) ld_build optimized_ofiles \
	$(RC_OS) dynamic_ofiles sarld_build profile_ofiles

lib_ofiles: $(OFILE_DIRS) $(SYMROOT) optimized_ofiles \
	$(RC_OS) dynamic_ofiles sarld_build

ld_build:	$(OBJROOT)/ld_dir $(SYMROOT)/ld_dir
	@if [ $(SRCROOT) = . ];						\
	then								\
	    cd ld_dir; $(MAKE) -f ../Makefile ld.NEW VPATH=..		\
		OFILE_DIR=. SRCROOT=.. RC_CFLAGS="$(RC_CFLAGS)"		\
		LIBSTUFF="-L../../libstuff -lstuff"			\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		MAKEINCDIR=`pwd` COPTS="-DDEBUG" CC="$(CC)";		\
	else								\
	    cd $(OBJROOT)/ld_dir; $(MAKE) -f $(SRCROOT)/Makefile ld.NEW	\
		VPATH=$(SRCROOT) OFILE_DIR=$(OBJROOT)/ld_dir		\
		SYMROOT=$(SYMROOT)/ld_dir SRCROOT=$(SRCROOT)		\
		MAKEINCDIR=`pwd` COPTS="-DDEBUG"			\
		LIBSTUFF="-L$(SYMROOT)/../libstuff -lstuff"		\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		RC_CFLAGS="$(RC_CFLAGS)" CC="$(CC)";			\
	fi

ld_g_build: ld_g_dir
	cd ld_g_dir; $(MAKE) -f ../Makefile ld_g.NEW VPATH=.. OFILE_DIR=. \
		SRCROOT=.. MAKEINCDIR=`pwd` COPTS="-DDEBUG" OFLAG=""	\
		LIBSTUFF="-L../../libstuff -lstuff"

ld_p_build: ld_p_dir
	cd ld_p_dir; $(MAKE) -f ../Makefile ld.NEW VPATH=.. OFILE_DIR=. \
		SRCROOT=.. MAKEINCDIR=`pwd` COPTS="-pg -DDEBUG"		\
		LIBSTUFF="-L../../libstuff -lstuff_p"

shlib_ofiles: $(OBJROOT)/shlib_obj $(SYMROOT)/ld_dir
	@if [ $(SRCROOT) = . ];						\
	then								\
	    (cd shlib_obj; $(MAKE) -f ../Makefile librld.o VPATH=.. 	\
		MAKEINCDIR=`pwd` OFILE_DIR=. SRCROOT=.. 		\
		COPTS="-static -DRLD -DSHLIB -I/LocalDeveloper/Headers/libsys" \
		LIBSTUFF="-L../../libstuff -lstuff_s"			\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)" CC="$(CC)"	\
		RC_CFLAGS="$(RC_CFLAGS)" RC_ARCHS="$(RC_ARCHS)");	\
	    rm -f shlib_obj/librld.ofileList;				\
	    cp librld.ofileList shlib_obj;				\
	else								\
	    (cd $(OBJROOT)/shlib_obj; $(MAKE) -f $(SRCROOT)/Makefile librld.o \
		VPATH=$(SRCROOT) OFILE_DIR=$(OBJROOT)/shlib_obj		\
		SYMROOT=$(SYMROOT)/ld_dir SRCROOT=$(SRCROOT)		\
		RC_CFLAGS="$(RC_CFLAGS)" CC="$(CC)"			\
		COPTS="-static -DRLD -DSHLIB -I/LocalDeveloper/Headers/libsys" \
		LIBSTUFF="-L$(SYMROOT)/../libstuff -lstuff_s"		\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		MAKEINCDIR=`pwd` RC_ARCHS="$(RC_ARCHS)");		\
	    rm -f $(OBJROOT)/shlib_obj/librld.ofileList;		\
	    cp librld.ofileList $(OBJROOT)/shlib_obj;			\
	fi

profile_ofiles:	$(OBJROOT)/profile_obj $(OBJROOT)/ptmp_obj $(SYMROOT)/ld_dir
	@if [ $(SRCROOT) = . ];						\
	then								\
	    (cd ptmp_obj; $(MAKE) -f ../Makefile librld_pg.a VPATH=.. 	\
		MAKEINCDIR=`pwd` OFILE_DIR=. SRCROOT=.. 		\
		LIBSTUFF="-L../../libstuff -lstuff_p"			\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)" CC="$(CC)"	\
		RC_CFLAGS="$(RC_CFLAGS)" COPTS="-dynamic -DRLD -pg");	\
	    rm -f profile_obj/librld.o;					\
	    rm -f  profile_obj/librld.ofileList;			\
	    cp ptmp_obj/librld.o profile_obj/librld.o;			\
	    cp librld.ofileList profile_obj;				\
	    rm -f dynamic_profile_obj;					\
	    ln -s ./profile_obj dynamic_profile_obj;			\
	else								\
	    (cd $(OBJROOT)/ptmp_obj; $(MAKE) -f $(SRCROOT)/Makefile	\
	        librld_pg.a VPATH=$(SRCROOT) OFILE_DIR=$(OBJROOT)/ptmp_obj \
		SYMROOT=$(SYMROOT)/ld_dir SRCROOT=$(SRCROOT)		\
		RC_CFLAGS="$(RC_CFLAGS)" CC="$(CC)"			\
		LIBSTUFF="-L$(SYMROOT)/../libstuff -lstuff_p"		\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		MAKEINCDIR=`pwd` COPTS="-dynamic -DRLD -pg");		\
	    rm -f $(OBJROOT)/profile_obj/librld.o; 			\
	    rm -f $(OBJROOT)/profile_obj/librld.ofileList;		\
	    cp $(OBJROOT)/ptmp_obj/librld.o $(OBJROOT)/profile_obj/librld.o; \
	    cp librld.ofileList $(OBJROOT)/profile_obj;			\
	    rm -f $(OBJROOT)/dynamic_profile_obj;			\
	    ln -s ./profile_obj $(OBJROOT)/dynamic_profile_obj;\
	fi

optimized_ofiles: $(OBJROOT)/optimized_obj $(OBJROOT)/otmp_obj $(SYMROOT)/ld_dir
	@if [ $(SRCROOT) = . ];						\
	then								\
	    (cd otmp_obj; $(MAKE) -f ../Makefile librld_static.a VPATH=.. \
		MAKEINCDIR=`pwd` OFILE_DIR=. SRCROOT=.. 		\
		LIBSTUFF="-L../../libstuff -lstuff" CC="$(CC)"		\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		RC_CFLAGS="$(RC_CFLAGS)" COPTS="-static -DRLD");	\
	    rm -f optimized_obj/librld.o;				\
	    rm -f optimized_obj/librld.ofileList;			\
	    cp otmp_obj/librld.o optimized_obj/librld.o;		\
	    cp librld.ofileList optimized_obj;				\
	    rm -f static_obj;						\
	    ln -s ./optimized_obj static_obj;				\
	else								\
	    (cd $(OBJROOT)/otmp_obj; $(MAKE) -f $(SRCROOT)/Makefile	\
		librld_static.a VPATH=$(SRCROOT)			\
		OFILE_DIR=$(OBJROOT)/otmp_obj				\
		SYMROOT=$(SYMROOT)/ld_dir SRCROOT=$(SRCROOT)		\
		RC_CFLAGS="$(RC_CFLAGS)" CC="$(CC)"			\
		LIBSTUFF="-L$(SYMROOT)/../libstuff -lstuff"		\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		MAKEINCDIR=`pwd` COPTS="-static -DRLD");		\
	    rm -f $(OBJROOT)/optimized_obj/librld.o; 			\
	    rm -f $(OBJROOT)/optimized_obj/librld.ofileList;		\
	    cp $(OBJROOT)/otmp_obj/librld.o $(OBJROOT)/optimized_obj/librld.o;\
	    cp librld.ofileList $(OBJROOT)/optimized_obj;		\
	    rm -f $(OBJROOT)/static_obj;				\
	    ln -s ./optimized_obj $(OBJROOT)/static_obj;	\
	fi

dynamic_ofiles: $(OBJROOT)/dynamic_obj $(OBJROOT)/dtmp_obj $(SYMROOT)/ld_dir
	@if [ $(SRCROOT) = . ];						\
	then								\
	    (cd dtmp_obj; $(MAKE) -f ../Makefile librld.a VPATH=.. 	\
		MAKEINCDIR=`pwd` OFILE_DIR=. SRCROOT=.. 		\
		LIBSTUFF="-L../../libstuff -lstuff" CC="$(CC)"		\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		RC_CFLAGS="$(RC_CFLAGS)" COPTS="-dynamic -DRLD");	\
	    rm -f dynamic_obj/librld.o;					\
	    rm -f dynamic_obj/librld.ofileList;				\
	    cp dtmp_obj/librld.o dynamic_obj/librld.o;			\
	    cp librld.ofileList dynamic_obj;				\
	else								\
	    (cd $(OBJROOT)/dtmp_obj; $(MAKE) -f $(SRCROOT)/Makefile librld.a \
		VPATH=$(SRCROOT) OFILE_DIR=$(OBJROOT)/dtmp_obj		\
		SYMROOT=$(SYMROOT)/ld_dir SRCROOT=$(SRCROOT)		\
		RC_CFLAGS="$(RC_CFLAGS)" CC="$(CC)"			\
		LIBSTUFF="-L$(SYMROOT)/../libstuff -lstuff"		\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		MAKEINCDIR=`pwd` COPTS="-dynamic -DRLD");		\
	    rm -f $(OBJROOT)/dynamic_obj/librld.o; 			\
	    rm -f $(OBJROOT)/dynamic_obj/librld.ofileList;		\
	    cp $(OBJROOT)/dtmp_obj/librld.o $(OBJROOT)/dynamic_obj/librld.o;\
	    cp librld.ofileList $(OBJROOT)/dynamic_obj;			\
	fi

sarld_build: $(OBJROOT)/sarld_dir $(SYMROOT)/ld_dir
	@if [ $(SRCROOT) = . ];						\
	then								\
	    cd sarld_dir; $(MAKE) -f ../Makefile libsarld.a VPATH=..	\
		MAKEINCDIR=`pwd` OFILE_DIR=. SRCROOT=.. 		\
		COPTS="-static -DRLD -DSA_RLD" CC="$(CC)"		\
		LIBSTUFF="-L../../libstuff -lstuff"			\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		RC_CFLAGS="$(RC_CFLAGS)" RC_ARCHS="$(RC_ARCHS)";	\
	else								\
	    cd $(OBJROOT)/sarld_dir; $(MAKE) -f $(SRCROOT)/Makefile	\
		libsarld.a VPATH=$(SRCROOT) OFILE_DIR=$(OBJROOT)/sarld_dir \
		SYMROOT=$(SYMROOT)/ld_dir SRCROOT=$(SRCROOT) CC="$(CC)"	\
		COPTS="-static -DRLD -DSA_RLD" RC_CFLAGS="$(RC_CFLAGS)"	\
		LIBSTUFF="-L$(SYMROOT)/../libstuff -lstuff"		\
		VERS_STRING_FLAGS="$(VERS_STRING_FLAGS)"		\
		MAKEINCDIR=`pwd` RC_ARCHS="$(RC_ARCHS)";		\
	fi

ld.NEW:	$(OBJS) ld_vers.o makeUser.o
	$(CC) $(COPTS) $(CFLAGS) $(RC_CFLAGS) -nostdlib -r \
		$(OBJS) ld_vers.o makeUser.o $(LIBSTUFF) \
		-o $(SYMROOT)/ld.private.o
	$(CC) $(COPTS) $(CFLAGS) $(RC_CFLAGS) -o $(SYMROOT)/ld.NEW \
		$(SYMROOT)/ld.private.o

ld_g.NEW: $(OBJS) ld_vers.o makeUser.o
	$(CC) $(COPTS) $(CFLAGS) $(RC_CFLAGS) \
		$(OBJS) ld_vers.o makeUser.o $(LIBSTUFF) -o $(SYMROOT)/ld_g.NEW

librld_static.a librld_pg.a librld.a: librld.o
	libtool -static -o $(SYMROOT)/$@ librld.o

librld.o: $(OBJS)
	$(CC) $(COPTS) $(CFLAGS) $(RC_CFLAGS) -nostdlib -r \
		-o $(OFILE_DIR)/librld.o $(OBJS) $(LIBSTUFF)

libsarld.a: $(OBJS)
	$(CC) $(COPTS) $(CFLAGS) $(RC_CFLAGS) -nostdlib -r \
		-o $(SYMROOT)/libsarld.a $(OBJS) $(LIBSTUFF)
ld_vers.c:
	vers_string -l $(VERS_STRING_FLAGS) ld > $(OFILE_DIR)/$@


makeUser.c ld.c: make.h

make.h makeUser.c: make.defs
	$(MIG) $(MIG_FLAGS) $(SRCROOT)/make.defs
	rm -f makeServer.c

ld_vers.o: ld_vers.c
ifeq "mwcc" "$(notdir $(CC))"
	$(CC) $(COPTS) $(CFLAGS) $(OFLAG) $(RC_CFLAGS) -c -MD \
		-Dstatic="__declspec(private_extern)" \
		-dependency-file $(OFILE_DIR)/$*.d -o $(OFILE_DIR)/$*.o $<
	md -d -u $(OFILE_DIR)/Makedep $(OFILE_DIR)/$*.d
else
	$(CC) $(COPTS) $(CFLAGS) $(OFLAG) $(RC_CFLAGS) -c -MD \
		-Dstatic=__private_extern__ \
		-dependency-file $(OFILE_DIR)/$*.d -o $(OFILE_DIR)/$*.o $<
	md -d -u $(OFILE_DIR)/Makedep $(OFILE_DIR)/$*.d
endif

makeUser.o: makeUser.c
ifeq "mwcc" "$(notdir $(CC))"
	$(CC) $(COPTS) $(CFLAGS) $(OFLAG) $(RC_CFLAGS) -c -MD \
		-Dmig_external="__declspec(private_extern)" \
		-dependency-file $(OFILE_DIR)/$*.d -o $(OFILE_DIR)/$*.o $<
else
	$(CC) $(COPTS) $(CFLAGS) $(OFLAG) $(RC_CFLAGS) -c -MD \
		-Dmig_external=__private_extern__ \
		-dependency-file $(OFILE_DIR)/$*.d -o $(OFILE_DIR)/$*.o $<
	md -d -u $(OFILE_DIR)/Makedep $(OFILE_DIR)/$*.d
endif

.c.o:
	$(CC) $(COPTS) $(CFLAGS) $(OFLAG) $(RC_CFLAGS) -c -MD \
		-dependency-file $(OFILE_DIR)/$*.d -o $(OFILE_DIR)/$*.o $<
	md -d -u $(OFILE_DIR)/Makedep $(OFILE_DIR)/$*.d

clean:
	-rm -r -f $(OFILE_DIRS) ld_g_dir ld_p_dir make.out

shlib_clean:
	-rm -r -f $(OBJROOT)/ld_dir $(OBJROOT)/ptmp_obj $(OBJROOT)/otmp_obj \
		$(OBJROOT)/dtmp_obj $(OBJROOT)/sarld_dir ld_g_dir ld_p_dir
	-cd $(OBJROOT)/shlib_obj; rm -f $(OBJS) make.h makeUser.c makeUser.o \
		Makedep ld_vers.c ld_vers.o


install: all $(RC_OS)_install

macos_install: common_install
	$(MKDIRS) $(DSTROOT)$(USRBINDIR)
	install -c -s -m 555 $(SYMROOT)/ld_dir/ld.NEW $(DSTROOT)$(USRBINDIR)/ld
	$(MKDIRS) $(DSTROOT)$(SYSTEMDIR)
	install -c -m 444 $(SYMROOT)/ld_dir/librld.a \
		 $(DSTROOT)$(SYSTEMDIR)/librld.a
	install -c -m 444 $(SYMROOT)/ld_dir/librld_pg.a \
		 $(DSTROOT)$(SYSTEMDIR)/librld_pg.a
	install -c -m 444 $(SYMROOT)/ld_dir/librld_static.a \
		 $(DSTROOT)$(SYSTEMDIR)/librld_static.a

teflon_install: common_install
	$(MKDIRS) $(DSTROOT)$(USRBINDIR)
	install -c -s -m 555 $(SYMROOT)/ld_dir/ld.NEW $(DSTROOT)$(USRBINDIR)/ld

nextstep_install: common_install
	$(MKDIRS) $(DSTROOT)$(BINDIR)
	install -c -s -m 555 $(SYMROOT)/ld_dir/ld.NEW $(DSTROOT)$(BINDIR)/ld

common_install:
	$(MKDIRS) $(DSTROOT)$(LOCLIBDIR)
	-install -c -m 444 $(SYMROOT)/ld_dir/libsarld.a \
		$(DSTROOT)$(LOCLIBDIR)/libsarld.a

installGNUsrc:

installsrc installMWsrc:
	$(MKDIRS) $(SRCROOT)
	chmod 755 $(SRCROOT)
	gnutar cf - $(INSTALL_FILES) | (cd $(SRCROOT); gnutar xf -)
	chmod 444 $(SRCROOT)/*

$(OFILE_DIRS) $(SYMROOT) $(SYMROOT)/ld_dir ld_g_dir ld_p_dir:
	$(MKDIRS) $@

-include $(MAKEINCDIR)/Makedep
