#
# gendisk Makefile.
#
PROGRAM=	gendisk
CFILES=		gendisk.c
MFILES=
HFILES=	

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

CFLAGS= -g -arch m68k -MD -Wall -Wnoformat -I/Net/iowa/dewitt/dmitch/proj/MKNRW/mknrw

SOURCEFILES= Makefile $(CFILES) $(MFILES) $(HFILES)
OFILES= $(CFILES:.c=.o) $(MFILES:.m=.o)
LIBS=

TARGETS = $(OBJROOT)/gendisk

all debug: $(OBJROOT) $(TARGETS)

#
# Note we don't build this at install time; we have to build on
# an m68k.
#
install installhdrs install_only tags kern:
	
installsrc: $(SRCROOT)
	gnutar cf - $(SOURCEFILES) | (cd $(SRCROOT); gnutar xpf -)

clean:
	-rm -f *.o *.d $(TARGETS) Makedep *~

$(SRCROOT) $(OBJROOT):
	$(MKDIRS) $@

$(OBJROOT)/$(PROGRAM):	$(OFILES)
	${CC} $(CFLAGS) -o $@ $(OFILES) $(LIBS)

.c.o:
	${CC} ${CFLAGS} -c -o $(OBJROOT)/$@ \
		-dependency-file $(OBJROOT)/$(*F).d $(*F).c
	md -u Makedep -d $(OBJROOT)/$(*F).d
		
.m.o:
	${CC} ${CFLAGS} -c -o $(OBJROOT)/$@ \
		-dependency-file $(OBJROOT)/$(*F).d $(*F).m
	md -u Makedep -d $(OBJROOT)/$(*F).d
		
-include Makedep
