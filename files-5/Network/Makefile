#
# Top level makefile for the /Local directory.
#

# Comment out this line if you want make output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /Network

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

EMPTYDIRS =
EMPTYSTICKYDIRS = 

#	Subdirectories that have their own makefiles
SUBDIRS = Administration Applications Developer Documentation Library Public Users

# Root directory "dot" files
DOTFILES= dir.tiff opendir.tiff
DOTMODE= 644

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
	echo -n "    Empty directories:"
#	for i in ${EMPTYDIRS} ; \
#	  do \
#		echo -n " $$i" ; \
#		mkdir -p -m ${DSTMODE} ${DSTDIR}/$$i ; \
#		chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}/$$i ; \
#	  done
#	echo "."
#	echo -n "    Empty sticky directories:"
#	for i in ${EMPTYSTICKYDIRS} ; \
#	  do \
#		echo -n " $$i" ; \
#		mkdir -p -m ${DSTMODE} ${DSTDIR}/$$i ; \
#		chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}/$$i ; \
#		chmod 1777 ${DSTDIR}/$$i ; \
#	  done
#	echo "."
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done
	echo -n "    Dot files:"
	for i in ${DOTFILES} ; \
	  do \
		echo -n " .$$i" ; \
		install -c -o ${DSTOWNER} -g ${DSTGROUP} -m ${DOTMODE} dot.$$i ${DSTDIR}/.$$i ; \
	  done

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdir -p -m ${DSTMODE} ${DSTDIR}
	chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}

$(DSTROOT):
	echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
