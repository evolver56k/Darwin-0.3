DEFINES = \
	-DGENERATINGPOWERPC \
	-D__ARCHITECTURE__=\"ppc\" -D_NEXT_SOURCE \
	-D__BIG_ENDIAN__ -DNeXT

#	-D__BIG_ENDIAN__ -D__LITTLE_ENDIAN__=0 -DNeXT

INCLUDES = \
	-I/NextLibrary/Frameworks/System.framework/Versions/B/Headers \
	-I/NextLibrary/Frameworks/System.framework/Versions/B/Headers/bsd

DEBUG = -g -L/lib -F/NextLibrary/Frameworks


# CFLAGS = $(DEFINES) $(INCLUDES) $(DEBUG) -x objective-c -Wno-import
CFLAGS = $(DEFINES) $(INCLUDES) $(DEBUG)

decode_macho: main.o load.o load2.o load3.o
	cc $(CFLAGS) main.o load.o load2.o load3.o -o decode_macho
	cp decode_macho ../bin

clean:
	/bin/rm -f main.o load.o load2.o load3.o
