#
# Makefile for kernel-specific libDriver subdirectory. This is just used
# for installsrc.
#
# Exported and Private header files are enumerated separately for installation
# purposes.
#
# .c and .m files are enumerated in the common Makefile for build purposes.
#
PUBLIC_HFILES=
PRIVATE_HFILES=	ctype.h \
		AudioChannel.h AudioStream.h \
		AudioCommand.h InputStream.h OutputStream.h \
		audioLog.h audio_msgs.h audio_server.h \
		audio_types.h audio_mix.h audio_mulaw.h audio_peak.h \
		portFuncs.h snd_reply.h snd_server.h audio_kern_server.h

#
# Define where in DSTDIR this source goes.
#
MACHINE_DIR= Kernel

tags:	always
	rm -f tags
	ctags *.[hm]

always:

# This does the rest.
-include ../../MakefileDir/Makefile.machine_common
