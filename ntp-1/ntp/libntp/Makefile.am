#AUTOMAKE_OPTIONS = ../ansi2knr no-dependencies
AUTOMAKE_OPTIONS = ../util/ansi2knr
noinst_LIBRARIES = libntp.a
libntp_a_SOURCES = a_md5encrypt.c adjtime.c atoint.c atolfp.c atouint.c \
	authencrypt.c authkeys.c authparity.c authreadkeys.c authusekey.c \
	buftvtots.c caljulian.c calleapwhen.c caltontp.c calyearstart.c \
	clocktime.c clocktypes.c decodenetnum.c dofptoa.c dolfptoa.c \
	emalloc.c findconfig.c fptoa.c fptoms.c getopt.c hextoint.c \
	hextolfp.c humandate.c inttoa.c lib_strbuf.c machines.c md5c.c \
	memmove.c mexit.c mfptoa.c mfptoms.c modetoa.c mstolfp.c msutotsf.c \
	msyslog.c netof.c numtoa.c numtohost.c octtoint.c prettydate.c \
	ranny.c refnumtoa.c statestr.c syssignal.c systime.c tsftomsu.c \
	tstotv.c tvtoa.c tvtots.c uglydate.c uinttoa.c utvtoa.c ymd2yd.c \
	mfp_mul.c binio.c ieee754io.c gpstolfp.c
libntp_a_LIBADD = @LIBOBJS@ @DESOBJS@
libntp_a_DEPENDENCIES = @LIBOBJS@ @DESOBJS@
INCLUDES = -I$(top_srcdir)/include
ETAGS_ARGS = Makefile.am

noinst_HEADERS = lib_strbuf.h log.h

../include/des.h:
	touch ../include/des.h

EXTRA_DIST = libntp.mak README $(HEADERS) adjtimex.c log.c strerror.c mktime.c
#EXTRA_DIST = libntp.mak README $(HEADERS) adjtimex.c desc.c.des desc.c.export log.c makemc.bat messages.mc strerror.c mktime.c

mktime_.c: mktime.c $(ANSI2KNR)
	$(ANSI2KNR) $< mktime_.c

strerror_.c: strerror.c $(ANSI2KNR)
	$(ANSI2KNR) $< strerror_.c
