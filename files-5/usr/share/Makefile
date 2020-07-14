#
# Makefile for the /usr/share directory.
#

# Comment out this line if you want all of the make commands output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /usr/share

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

TAR = /usr/bin/gnutar

SLAVEDIRS =
#	Subdirectories without their own makefiles
SUBDIRS = misc dict tabset

all:
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	/bin/rm -f *~ .[a-zA-Z]*~ \#* *.BAK
#	for i in ${SLAVEDIRS} ; \
#	  do \
#		echo "    Removing" $$i/*~ $$i/.[a-zA-Z]*~ $$i/\#* $$i/*.BAK ; \
#		/bin/rm -f $$i/*~ $$i/.[a-zA-Z]*~ $$i/\#* $$i/*.BAK ; \
#	  done
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

install: ${DSTDIR}
#	for i in ${SLAVEDIRS} ; \
#	  do \
#		echo " Removing old" ${DSTDIR}/$$i ; \
#		/bin/rm -rf ${DSTDIR}/$$i ; \
#		echo " Copying" $$i to ${DSTDIR}/$$i ; \
#		${TAR} cf - $$i | (cd ${DSTDIR}; ${TAR} xpf -) ; \
#		echo " Setting" ${DSTDIR}/$$i to ug=rwX,o=rX ; \
#		chown -R ${DSTOWNER}.${DSTGROUP} ${DSTDIR}/$$i ; \
#		chmod -R u=rwX,og=rX ${DSTDIR}/$$i ; \
#	  done
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
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
