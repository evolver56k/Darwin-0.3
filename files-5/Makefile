#
# Top level makefile for the FILES project.  This establishes the basic 
# files necessary for a Rhapsody filesystem.
#

# Comment out this line if you want make output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

# Root directory "dot" files
DOTFILES= hidden
DOTMODE= 644

#	Subdirectories with their own makefiles
# WARNING! Make "private" after making "usr"
SUBDIRS = usr private System Local Network

#	Empty directories to be created
#EMPTYDIRS = 

#	Dirs in /private that should be linked to
PRIVATEDIR= private
PRIVATELINKS= cores dev etc tmp var

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
	echo -n "    Dot files:"
	for i in ${DOTFILES} ; \
	  do \
		echo -n " .$$i" ; \
		install -c -o ${DSTOWNER} -g ${DSTGROUP} -m ${DOTMODE} dot.$$i ${DSTDIR}/.$$i ; \
	  done
	echo "."
	echo -n "    Creating special links:"
	for i in ${PRIVATELINKS} ; \
	  do \
		echo -n " $$i" ; \
		ln -fs ${PRIVATEDIR}/$$i ${DSTROOT}/$$i ; \
	  done
	echo "."
	echo -n "    Empty directories:"
#	for i in ${EMPTYDIRS} ; \
#	  do \
#		echo -n " $$i" ; \
#		mkdir -p -m ${DSTMODE} ${DSTROOT}/$$i ; \
#	  done
#	echo "."
	echo -n "    Subdirectories:"
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done
	echo "."
	echo "    Creating /mach"
	ln -fs mach_kernel ${DSTROOT}/mach
#	echo "    Creating /Net"
#	ln -fs Network/Servers ${DSTROOT}/Net

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
	mkdir -p $@

DSTROOT SRCROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
