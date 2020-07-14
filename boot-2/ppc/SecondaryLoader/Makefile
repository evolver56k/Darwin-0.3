NeXTLIBS = ../sym/ppc

DEFINES = -DGENERATINGPOWERPC -DDEBUG \
			-D__ARCHITECTURE__=\"ppc\" -D_NEXT_SOURCE \
			-D__BIG_ENDIAN__ -D__LITTLE_ENDIAN__=0 -DNeXT

INCDIR = /NextLibrary/Frameworks/System.framework/Versions/B/Headers
INCLUDES = -I../include/mac -I. -I../ppcMac/libsaio -I../ppcMac/libsa -I../ppcMac/util \
			-I$(INCDIR)/ansi -I$(INCDIR)/bsd -I$(INCDIR)
DEBUG = -O -g
ARCHLESS_RC_CFLAGS=`echo $(RC_CFLAGS)|sed 's/-arch [a-z0-9]*//g'`
CFLAGS = $(DEFINES) $(INCLUDES) -O -x objective-c -Wno-import -static -fno-builtin -finline -msoft-float -arch ppc $(ARCHLESS_RC_CFLAGS)
SFLAGS = -arch ppc $(ARCHLESS_RC_CFLAGS)

OFFILES = Debugger.of Control2.of
CFILES = SecondaryInC.c HFSSupport.c Display.c DeviceTree.c PEFSupport.c \
		printf.c icon.c
SFILES = setjmp.s
OFILES = $(CFILES:.c=.o) $(SFILES:.s=.o) Debugger.o Control2.o

OBJROOT = ../obj/ppc
SYMROOT = ../sym/ppc

OFILES2 = $(addprefix $(OBJROOT)/, $(OFILES))

LOADADDR = 0x00500000
DATAADDR = 0x00520000
ALIGN    = 0x10

LIBS = $(SYMROOT)/libsaio.a $(SYMROOT)/libsa.a $(SYMROOT)/librcz.a -lcc

.SUFFIXES: .c .o .s .of

$(OBJROOT)/%.c:	%.of
	gawk --assign VAR=$(basename $(notdir $<)) --file ConvertOF.awk $< > $@

$(OBJROOT)/%.o :	%.c
	export DEPENDENCIES_OUTPUT=./.dependencies; \
	$(CC) -c "-DBUILD_DATETIME=\"`date "+%T %y%m%d"`\"" $(CFLAGS) $(CPPFLAGS) $< -o $@

$(OBJROOT)/%.o:	%.s
	$(CC) $(SFLAGS) -c -static $< -o $@

all:	$(SYMROOT)/SecondaryLoader

$(OBJROOT)/Debugger.o:	Debugger.of
	gawk --assign VAR=$(basename $(notdir $<)) --file ConvertOF.awk $< > $(OBJROOT)/Debugger.c
	export DEPENDENCIES_OUTPUT=./.dependencies; \
	$(CC) -c "-DBUILD_DATETIME=\"`date "+%T %y%m%d"`\"" $(CFLAGS) $(CPPFLAGS) $(OBJROOT)/Debugger.c -o $@

$(OBJROOT)/Control2.o:	Control2.of
	gawk --assign VAR=$(basename $(notdir $<)) --file ConvertOF.awk $< > $(OBJROOT)/Control2.c
	export DEPENDENCIES_OUTPUT=./.dependencies; \
	$(CC) -c "-DBUILD_DATETIME=\"`date "+%T %y%m%d"`\"" $(CFLAGS) $(CPPFLAGS) $(OBJROOT)/Control2.c -o $@


clean:
	rm -rf $(OFILES2) $(GENERATED_CFILES) \
		SecondaryLoader SecondaryLoader.mach SecondaryLoader.map

TAGS:
	etags $(CFILES) $(SFILES)

$(SYMROOT)/SecondaryLoader:	$(SYMROOT)/SecondaryLoader.mach
	$(SYMROOT)/macho-to-xcoff $^ $@

x.elf:	$(OFILES) $(LIBS) SecondaryLoader.ld.abs-elf.dbg
	$(CC) -o $@ $(OFILES) $(LIBS) \
			-nostdlib -static -lgcc \
			-Wl,--script=SecondaryLoader.ld.abs-elf.dbg,-e,SecondaryLoaderMainTVector

$(SYMROOT)/SecondaryLoader.mach:	$(OFILES2) $(LIBS)
	$(CC) -o $@ $(OFILES2) $(LIBS) $(ARCHLESS_RC_CFLAGS) \
	-seg1addr $(LOADADDR) -segaddr __DATA $(DATAADDR) -segalign $(ALIGN) \
	-e _SecondaryLoaderMainTVector -arch ppc \
	-nostdlib -static

SecondaryLoader.elf:	$(OFILES) $(LIBS) SecondaryLoader.ld.abs-elf
	$(CC) -o $@ $(OFILES) $(LIBS) \
			-Wl,-Map,SecondaryLoader.map \
			-nostdlib -static -lgcc \
			-Wl,--script=SecondaryLoader.ld.abs-elf,-e,SecondaryLoaderMainTVector -s
#			-Wl,-Ttext,0x700000,-Tdata,0x710000,-e,SecondaryLoaderMainTVector -s
#			-Wl,-e,SecondaryLoaderMain,--script=SecondaryLoader.ld
#			-Wl,-e,SecondaryLoaderMain,-oformat,aixcoff-rs6000

# dependencies
-include ./.dependencies
