#
# Makefile for the usr directory.
#

# Comment out this line if you want all of the make commands output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /usr

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

#	Subdirectories with their own makefiles
SUBDIRS = share template

all:
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

install: ${DSTDIR}
	mkdir -p ${DSTROOT}/System/Library
	echo "    Linking /System/Library/Devices to ../../private/Devices"
	ln -fs ../../private/Devices ${DSTROOT}/System/Library/Devices
	echo "    Linking /usr/Devices to ../private/Devices"
	ln -fs ../private/Devices ${DSTROOT}/usr/Devices
	echo "    Linking ${DSTDIR}/include to ../System/Library/Frameworks/System.framework/Headers/bsd"
	ln -fs ../System/Library/Frameworks/System.framework/Headers/bsd ${DSTDIR}/include
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

${DSTDIR}: DSTROOT ${DSTROOT}
	mkdir -p -m ${DSTMODE} ${DSTDIR}
	chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
