#AUTOMAKE_OPTIONS = ../ansi2knr no-dependencies
AUTOMAKE_OPTIONS = ../util/ansi2knr
EXTRA_PROGRAMS = authcert authspeed md5driver keyparity makeIPFP makePC1 makePC2 \
	makeSP mkrandkeys omakeIPFP unixcert
INCLUDES = -I$(top_srcdir)/include
LDADD = ../libntp/libntp.a
#EXTRA_DIST = README TAGS auth.samplekeys auth.speed certdata md5_sample_output results
EXTRA_DIST = auth.samplekeys auth.speed certdata md5_sample_output results
ETAGS_ARGS = Makefile.am

../libntp/libntp.a:
	cd ../libntp && $(MAKE)

check-local-rsn: authcert authspeed
	./authcert < $(srcdir)/certdata
	./authspeed -d -n 30000 $(srcdir)/auth.samplekeys
	./authspeed -m -n 30000 $(srcdir)/auth.samplekeys
