# Local Variables:
# tab-width: 4
# End:

NeXTLIBS = ../sym/ppc

DEFINES = \
	-DGENERATINGPOWERPC -DDEBUG \
	-D__ARCHITECTURE__=\"ppc\" -D_NEXT_SOURCE \
	-D__BIG_ENDIAN__ -D__LITTLE_ENDIAN__=0 -DNeXT

INCLUDES = \
	-I/NextLibrary/Frameworks/System.framework/Versions/B/Headers \
	-I/NextLibrary/Frameworks/System.framework/Versions/B/Headers/bsd

DEBUG = -g -L/lib -F/NextLibrary/Frameworks


CFLAGS = $(DEFINES) $(INCLUDES) $(DEBUG) -x objective-c -Wno-import

CFILES = macho-to-xcoff-eng.c decode_xcoff.c
SFILES = 
OFILES = $(CFILES:.c=.o) $(SFILES:.s=.o)

LIBS =
all:	macho-to-xcoff decode_xcoff

macho-to-xcoff:	macho-to-xcoff-eng.c
	$(CC) $(CFLAGS) -arch ppc macho-to-xcoff-eng.c -o macho-to-xcoff.ppc
	$(CC) $(CFLAGS) -arch i386 macho-to-xcoff-eng.c -o macho-to-xcoff.i386
	lipo -create -output macho-to-xcoff macho-to-xcoff.ppc macho-to-xcoff.i386

	cp macho-to-xcoff ../bin

decode_xcoff:	decode_xcoff.c
	$(CC) $(CFLAGS) decode_xcoff.c -o decode_xcoff
	cp decode_xcoff ../bin
clean:
	rm -f macho-to-xcoff-eng decode_xcoff
