#AUTOMAKE_OPTIONS = ../util/ansi2knr no-dependencies
#
# this is sick - i want the source to be compile with
# two time with different flags resulting different
# objects - still need to find a way to communicate
# that to automake/autoconf
#
# Frank Kardel
#
AUTOMAKE_OPTIONS = ../util/ansi2knr
noinst_LIBRARIES = @MAKE_LIBPARSE@ @MAKE_LIBPARSE_KERNEL@
EXTRA_LIBRARIES = libparse.a libparse_kernel.a
EXTRA_PROGRAMS = parsestreams parsesolaris
noinst_PROGRAMS = @MAKE_PARSEKMODULE@
CLEANFILES = libparse.a libparse_kernel.a

libparse_a_SOURCES = parse.c \
                     parse_conf.c \
                     clk_meinberg.c \
                     clk_schmid.c \
		     clk_rawdcf.c \
		     clk_trimtsip.c \
	             clk_dcf7000.c \
                     clk_trimtaip.c \
                     clk_rcc8000.c \
                     clk_hopf6021.c \
	             clk_computime.c \
                     data_mbg.c

libparse_kernel_a_SOURCES = kparse.c \
                            kparse_conf.c \
                            kclk_meinberg.c \
                            kclk_schmid.c \
		            kclk_rawdcf.c \
		            kclk_trimtsip.c \
	                    kclk_dcf7000.c \
                            kclk_trimtaip.c \
                            kclk_rcc8000.c \
                            kclk_hopf6021.c \
	                    kclk_computime.c

INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/kernel
ETAGS_ARGS = Makefile.am

EXTRA_DIST = parsesolaris.c parsestreams.c

#
# HACK following below...
#
kparse_conf.o: parse_conf.c
kparse.o: parse.c
kclk_rawdcf.o: clk_rawdcf.c
kclk_trimtsip.o: clk_trimtsip.c
kclk_meinberg.o: clk_meinberg.c
kclk_schmid.o: clk_schmid.c
kclk_rawdcf.o: clk_rawdcf.c
kclk_trimtsip.o: clk_trimtsip.c
kclk_dcf7000.o: clk_dcf7000.c
kclk_trimtaip.o: clk_trimtaip.c
kclk_rcc8000.o: clk_rcc8000.c
kclk_hopf6021.o: clk_hopf6021.c
kclk_computime.o: clk_computime.c

parsestreams.loadable_module.o: $(parsestreams_OBJECTS) libparse_kernel.a ../libntp/libntp.a
	$(LD) -r -o $@ $(parsestreams_OBJECTS) libparse_kernel.a ../libntp/libntp.a

parse: $(parsesolaris_OBJECTS) libparse_kernel.a ../libntp/libntp.a
	$(LD) -r -o $@ $(parsesolaris_OBJECTS) libparse_kernel.a ../libntp/libntp.a

../libntp/libntp.a:
	cd ../libntp && $(MAKE)

