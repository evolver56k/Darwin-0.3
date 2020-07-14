#
# Makefile for the /private/var directory.
#

# Comment out this line if you want all of the make commands output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /private/var

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

#	Subdirectories with their own makefiles
SUBDIRS = at log run spool vm

#	Empty subdirectories
EMPTYDIRS = backups db msgs rwho

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
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done
	echo -n "    Empty directories:"
	for i in ${EMPTYDIRS} ; \
	  do \
		echo -n " $$i" ; \
		mkdir -p -m ${DSTMODE} ${DSTDIR}/$$i ; \
		chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}/$$i ; \
	  done
	echo "."
	echo -n "    Temporary directory: tmp"
	mkdir -p ${DSTDIR}/tmp
	chown root:wheel ${DSTDIR}/tmp
	chmod 1777 ${DSTDIR}/tmp
	echo -n "    Mail directories: mail"
	mkdir -p -m 775 ${DSTDIR}/mail
	chown root:mail ${DSTDIR}/mail
	echo "."

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdir -p -m ${DSTMODE} ${DSTDIR}
	chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
