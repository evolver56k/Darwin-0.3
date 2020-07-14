#
# Makefile for driverkit code examples.

MAKEFILEDIR = $(MAKEFILEPATH)/pb_makefiles
include $(MAKEFILEDIR)/platform.make

CFILES=
MFILES= probeAndInit.m
HFILES=	probeAndInit.h

ifneq "" "$(wildcard /bin/mkdirs)"
  MKDIRS = /bin/mkdirs
else
  MKDIRS = /bin/mkdir -p
endif

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

SOURCEFILES= Makefile $(CFILES) $(MFILES) $(HFILES)
OFILES= $(CFILES:.c=.o) $(MFILES:.m=.o)

TARGETS = $(OBJROOT)/probeAndInit 

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

$(OBJROOT)/probeAndInit:	probeAndInit.o 

.c.o:
	${CC} ${CFLAGS} -c -o $(OBJROOT)/$@ \
		-dependency-file $(OBJROOT)/$(*F).d $(*F).c
	md -u $(OBJROOT)/Makedep -d $(OBJROOT)/$(*F).d
		
.m.o:
	${CC} ${CFLAGS} -c -o $(OBJROOT)/$@ \
		-dependency-file $(OBJROOT)/$(*F).d $(*F).m
	md -u $(OBJROOT)/Makedep -d $(OBJROOT)/$(*F).d
		
-include $(OBJROOT)/Makedep
