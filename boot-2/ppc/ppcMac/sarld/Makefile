
DEFINES = -D__ARCHITECTURE__=\\\"ppc\\\" -D_NEXT_SOURCE \
                        -D__BIG_ENDIAN__ -D__LITTLE_ENDIAN__=0 -DNeXT \
                        -DGENERATINGPOWERPC -DDEBUG \
                        -DPPC -Dppc -D__PPC__ -D_BIG_ENDIAN -arch ppc

CFLAGS  = $(RC_CFLAGS) \
                        -mcpu=604 -traditional-cpp -fno-builtin \
                        -finline -msoft-float $(DEFINES) -O \
			-I/NextLibrary/Frameworks/System.framework/Versions/B/Headers \
			-I/NextLibrary/Frameworks/System.framework/Versions/B/Headers/bsd

INCLUDE= \
    -I/NextLibrary/Frameworks/System.framework/Versions/B/Headers/bsd \
    -I/NextLibrary/Frameworks/System.framework/Versions/B/Headers

DIR = sarld
MKDIRS = mkdir

include ../MakePaths.dir

DIRS_NEEDED = $(OBJROOT) $(SYMROOT)
VPATH = $(OBJROOT):$(SYMROOT)

INSTALLDIR = $(DSTROOT)/usr/standalone/i386

LIBSA=$(SYMROOT)/libsa.a
LIBSADIR = ../libsa
LIBSARLD = -lsarld
SARLD = $(SYMROOT)/sarld

ARCHFLAGS = -arch ppc -L/usr/local/lib -L$(SYMROOT)

#RLD_ADDR = `gawk -e '/RLD_ADDR/ { print $$3 }' < $(LIBSADIR)/memory.h`
RLD_ADDR = 700000
GLUE= $(OBJROOT)/glue.o $(OBJROOT)/setjmp.o
DIRS_NEEDED = $(OBJROOT) $(SYMROOT)

all: $(DIRS_NEEDED) $(SARLD)

$(SARLD): $(SARLD).sys
		strip -o $(SARLD) $(SARLD).sys
		
$(SARLD).sys: $(LIBSA) glue.o setjmp.o
		$(CC) $(COPTS) $(CFLAGS) $(RC_CFLAGS) $(ARCHFLAGS) -o $(SARLD).sys \
			-e _sa_rld_with_symtab -u _sa_rld_with_symtab $(GLUE) \
			-seg1addr $(RLD_ADDR) -nostdlib \
			$(LIBSARLD) -lsa -static

glue.o:	glue.c
	cc -c -static -arch ppc $(RC_CFLAGS) glue.c -o $(OBJROOT)/glue.o

setjmp.o:	setjmp.s
	cc -c -static -arch ppc $(RC_CFLAGS) setjmp.s -I../../SecondaryLoader -o $(OBJROOT)/setjmp.o

rldtest.o:	rldtest.c
	cc -c rldtest.c $(INCLUDE)

rldtest: rldtest.o
	$(CC) $(COPTS) $(CFLAGS) $(ARCHFLAGS) -o rldtest rldtest.o \
	-L/lib -F/NextLibrary/Frameworks

# -L. -lsarld -static


clean::
		rm -rf $(SARLD) $(SARLD).sys $(GLUE)
		
install_i386:: $(SARLD) $(INSTALLDIR)
		cp $(SARLD) $(INSTALLDIR)

include ../MakeInc.dir

#dependencies
