#
# Top level makefile for the /private directory.
#

# Comment out this line if you want make output
#.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /private

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

#	Subdirectories that have their own makefiles
SUBDIRS = dev etc var

#	Empty subdirectories
EMPTYDIRS = 

EMPTY777DIRS = cores

#	Temp directory
TMPDIR = tmp
TMPMODE = 1777

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
	echo "    Making tmp"
	mkdir -p ${DSTDIR}/${TMPDIR}
	chmod ${TMPMODE} ${DSTDIR}/${TMPDIR}
	chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}/${TMPDIR}

#	echo -n "    Empty directories:"
#	for i in ${EMPTYDIRS} ; \
#	  do \
#		echo -n " $$i" ; \
#		mkdir -p -m ${DSTMODE} ${DSTDIR}/$$i ; \
#		chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}/$$i ; \
#	  done
	echo "."
	echo -n "    Empty world-writable directories:"
	for i in ${EMPTY777DIRS} ; \
	  do \
		echo -n " $$i" ; \
		mkdir -p -m 777 ${DSTDIR}/$$i ; \
		chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}/$$i ; \
	  done
	echo "."
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdir -p -m ${DSTMODE} ${DSTDIR}
	chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}

$(DSTROOT):
	echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
