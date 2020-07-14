#
# Top level makefile for the /private/root/.OpenStep directory.
#

# Comment out this line if you want make output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /Local/Users/Administrator/.OpenStep

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 700

#	Subdirectories that have their own makefiles
SUBDIRS = UserDefaults2

all:
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

clean:
	echo "    Cleaning up..."
	/usr/bin/find . -name ".*~" -exec /bin/rm -f {} \;
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

install: ${DSTDIR}
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

#	Make a snapshot of this source directory.
installsrc: SRCROOT ${SRCROOT}
	gnutar cf - . | (cd ${SRCROOT}; gnutar xfBp -)

#	Install any header files
installhdrs:
	@echo installhdrs is currently a no-op.

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdir -p -m ${DSTMODE} ${DSTDIR}
	chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}

$(DSTROOT) $(SRCROOT):
	echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
