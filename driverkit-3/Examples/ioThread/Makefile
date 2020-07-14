#
# Makefile for driverkit code examples.

MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
include $(MAKEFILEDIR)/platform.make

CFILES=
MFILES= ioThread.m
HFILES=	ioThread.h ioThreadPrivate.h
ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

#
# OBJROOT and SYMROOT are overridden by RC tools for builds from read-only
# source.
#
OBJROOT= .
SYMROOT= .
VPATH= $(OBJROOT)

#
# use these for local build...
#
DRIVER_LIB=	$(SYMROOT)/../../libDriver/syms/libDriver_g.a
CFLAGS= -g -DDEBUG -DXPR_DEBUG $(RC_CFLAGS) -DNRW_BUILD -MD -Wall -I../..

#
# and these for building from installed libs and header files...
#
#DRIVER_LIB=	$(DSTROOT)/usr/local/lib/libDriver_g.a
#CFLAGS= -g -DDEBUG -DXPR_DEBUG -DNRW_BUILD -MD -Wall \
#	 -I//NextDeveloper/Imports 

SOURCEFILES= Makefile $(CFILES) $(MFILES) $(HFILES)
OFILES= $(CFILES:.c=.o) $(MFILES:.m=.o)

TARGETS = $(OBJROOT)/ioThread

USER_CFLAGS= -DIOSERVER

all debug: $(OBJROOT) $(SYMROOT) $(TARGETS)

install: all

installhdrs install_only tags kern:
	
installsrc: $(SRCROOT)
	gnutar cf - $(SOURCEFILES) | (cd $(SRCROOT); gnutar xpf -)

clean:
	-rm -f *.o *.d $(TARGETS) Makedep *~

$(SRCROOT) $(OBJROOT) $(SYMROOT):
	$(MKDIRS) $@

$(OBJROOT)/ioThread:	ioThread.o  $(DRIVER_LIB)
	${CC} ${CFLAGS} $(USER_CFLAGS) -o $@ $@.o $(DRIVER_LIB)

.c.o:
	${CC} ${CFLAGS} -c -o $(OBJROOT)/$@ \
		-dependency-file $(OBJROOT)/$(*F).d $(*F).c
	md -u $(OBJROOT)/Makedep -d $(OBJROOT)/$(*F).d
		
.m.o:
	${CC} ${CFLAGS} -c -o $(OBJROOT)/$@ \
		-dependency-file $(OBJROOT)/$(*F).d $(*F).m
	md -u $(OBJROOT)/Makedep -d $(OBJROOT)/$(*F).d
		
-include $(OBJROOT)/Makedep
