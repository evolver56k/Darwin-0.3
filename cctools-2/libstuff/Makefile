OFLAG = -O
ifeq "mwcc" "$(notdir $(CC))"
  CFLAGS = $(OFLAG) -g -I$(SRCROOT)/../include
else
  CFLAGS = $(OFLAG) -g -I$(SRCROOT)/../include -Wall -Wno-precomp
endif

ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

SRCROOT = .
SYMROOT = .
OBJROOT = .
OFILE_DIRS = $(OBJROOT)/static_obj $(OBJROOT)/dynamic_obj\
	     $(OBJROOT)/profile_obj $(OBJROOT)/shlib_obj
OFILE_DIR = $(OBJROOT)
VPATH = $(OFILE_DIR)

CFILES1 = allocate.c errors.c arch.c execute.c bytesex.c round.c hppa.c \
	  swap_headers.c reloc.c version_number.c get_toc_byte_sex.c \
	  best_arch.c fatals.c arch_usage.c ofile.c print.c set_arch_flag_name.c
CFILES2 = breakout.c writeout.c checkout.c fatal_arch.c ofile_get_word.c \
	  vm_flush_cache.c hash_string.c
OBJS1 = $(CFILES1:.c=.o)
OBJS2 = $(CFILES2:.c=.o)
INSTALL_FILES = $(CFILES1) $(CFILES2) Makefile notes

teflon macos:

nextstep: shlib

all: $(OFILE_DIRS) $(SYMROOT) static dynamic profile $(RC_OS)

fromGNUsrc fromMWsrc:	$(OFILE_DIRS) $(SYMROOT) dynamic

static: $(OBJROOT)/static_obj
	@if [ $(SRCROOT) = . ];						\
	then								\
	    cd static_obj;						\
		$(MAKE) -f ../Makefile libstuff.a			\
		PRODUCT=libstuff.a					\
		VPATH=.. 						\
		OFILE_DIR=.						\
		SRCROOT=..						\
		SYMROOT=..						\
		COPTS="-static"						\
		OFLAG="$(OFLAG)"					\
		CFILES1="$(CFILES1)"					\
		CFILES2="$(CFILES2)"					\
		RC_CFLAGS="$(RC_CFLAGS)"				\
		RC_ARCHS="$(RC_ARCHS)";					\
	else								\
	    cd $(OBJROOT)/static_obj;					\
		$(MAKE) -f $(SRCROOT)/Makefile libstuff.a		\
		PRODUCT=libstuff.a					\
		VPATH=$(SRCROOT)					\
		OFILE_DIR=$(OBJROOT)/static_obj				\
		SRCROOT=$(SRCROOT)					\
		SYMROOT=$(SYMROOT)					\
		COPTS="-static"						\
		OFLAG="$(OFLAG)"					\
		CFILES1="$(CFILES1)"					\
		CFILES2="$(CFILES2)"					\
		RC_CFLAGS="$(RC_CFLAGS)"				\
		RC_ARCHS="$(RC_ARCHS)";					\
	fi

dynamic: $(OBJROOT)/dynamic_obj
	@if [ $(SRCROOT) = . ];						\
	then								\
	    cd dynamic_obj;						\
		$(MAKE) -f ../Makefile libstuff.dylib			\
		PRODUCT=libstuff.dylib					\
		VPATH=.. 						\
		OFILE_DIR=.						\
		SRCROOT=..						\
		SYMROOT=..						\
		COPTS="-dynamic"					\
		OFLAG="$(OFLAG)"					\
		CFILES1="$(CFILES1)"					\
		CFILES2="$(CFILES2)"					\
		RC_CFLAGS="$(RC_CFLAGS)"				\
		RC_ARCHS="$(RC_ARCHS)";					\
	else								\
	    cd $(OBJROOT)/dynamic_obj;					\
		$(MAKE) -f $(SRCROOT)/Makefile libstuff.dylib		\
		PRODUCT=libstuff.dylib					\
		VPATH=$(SRCROOT)					\
		OFILE_DIR=$(OBJROOT)/dynamic_obj			\
		SRCROOT=$(SRCROOT)					\
		SYMROOT=$(SYMROOT)					\
		COPTS="-dynamic"					\
		OFLAG="$(OFLAG)"					\
		CFILES1="$(CFILES1)"					\
		CFILES2="$(CFILES2)"					\
		RC_CFLAGS="$(RC_CFLAGS)"				\
		RC_ARCHS="$(RC_ARCHS)";					\
	fi

profile: $(OBJROOT)/profile_obj
	@if [ $(SRCROOT) = . ];						\
	then								\
	    cd profile_obj;						\
		$(MAKE) -f ../Makefile libstuff_p.a			\
		PRODUCT=libstuff_p.a					\
		VPATH=.. 						\
		OFILE_DIR=.						\
		SRCROOT=..						\
		SYMROOT=..						\
		COPTS="-dynamic -pg"					\
		OFLAG="$(OFLAG)"					\
		RC_CFLAGS="$(RC_CFLAGS)"				\
		RC_ARCHS="$(RC_ARCHS)";					\
	else								\
	    cd $(OBJROOT)/profile_obj;					\
		$(MAKE) -f $(SRCROOT)/Makefile libstuff_p.a		\
		PRODUCT=libstuff_p.a					\
		VPATH=$(SRCROOT)					\
		OFILE_DIR=$(OBJROOT)/profile_obj			\
		SRCROOT=$(SRCROOT)					\
		SYMROOT=$(SYMROOT)					\
		COPTS="-dynamic -pg"					\
		OFLAG="$(OFLAG)"					\
		RC_CFLAGS="$(RC_CFLAGS)"				\
		RC_ARCHS="$(RC_ARCHS)";					\
	fi

shlib: $(OBJROOT)/shlib_obj
	@if [ $(SRCROOT) = . ];						\
	then								\
	    cd shlib_obj;						\
		$(MAKE) -f ../Makefile libstuff_s.a			\
		PRODUCT=libstuff_s.a					\
		VPATH=.. 						\
		OFILE_DIR=.						\
		SRCROOT=..						\
		SYMROOT=..						\
		COPTS="-static -DSHLIB -I/LocalDeveloper/Headers/libsys" \
		OFLAG="$(OFLAG)"					\
		RC_CFLAGS="$(RC_CFLAGS)"				\
		RC_ARCHS="$(RC_ARCHS)";					\
	else								\
	    cd $(OBJROOT)/shlib_obj;					\
		$(MAKE) -f $(SRCROOT)/Makefile libstuff_s.a		\
		PRODUCT=libstuff_s.a					\
		VPATH=$(SRCROOT)					\
		OFILE_DIR=$(OBJROOT)/shlib_obj				\
		SRCROOT=$(SRCROOT)					\
		SYMROOT=$(SYMROOT)					\
		COPTS="-static -DSHLIB -I/LocalDeveloper/Headers/libsys" \
		OFLAG="$(OFLAG)"					\
		RC_CFLAGS="$(RC_CFLAGS)"				\
		RC_ARCHS="$(RC_ARCHS)";					\
	fi

$(PRODUCT):	$(OFILE_DIR) $(SYMROOT) $(OBJS1) $(OBJS2)
	libtool -static -o $(SYMROOT)/$(PRODUCT) $(OBJS1) $(OBJS2)

ifeq "mwcc" "$(notdir $(CC))"
.c.o:
	$(CC) $(CFLAGS) $(RC_CFLAGS) $(COPTS) -c -o $(OFILE_DIR)/$*.o $<
else
.c.o:
	$(CC) $(CFLAGS) $(RC_CFLAGS) $(COPTS) -c \
		-MD -dependency-file $(OFILE_DIR)/$*.d \
		-o $(OFILE_DIR)/$*.o $<
	md -d -u $(OFILE_DIR)/Makedep $(OFILE_DIR)/$*.d
endif

clean shlib_clean:
	-rm -r -f $(OFILE_DIRS)
	-rm -f libstuff.a libstuff.dylib libstuff_p.a libstuff_s.a

install: all

installGNUsrc installMWsrc:
	$(MKDIRS) $(SRCROOT)
	chmod 755 $(SRCROOT)
	gnutar cf - $(CFILES1) Makefile | (cd $(SRCROOT); gnutar xf -)
	chmod 444 $(SRCROOT)/*

installsrc:
	$(MKDIRS) $(SRCROOT)
	chmod 755 $(SRCROOT)
	gnutar cf - $(INSTALL_FILES) | (cd $(SRCROOT); gnutar xf -)
	chmod 444 $(SRCROOT)/*

$(OFILE_DIRS) $(SYMROOT):
	$(MKDIRS) $@

-include $(OFILE_DIR)/Makedep
