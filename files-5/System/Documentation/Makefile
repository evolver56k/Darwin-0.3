#
# Top level makefile for the /System/Library directory.
#

# Comment out this line if you want make output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /System/Documentation

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

# Root directory "dot" files
DOTFILES= dir.tiff opendir.tiff
DOTMODE= 644

all:

clean:
	echo "    Cleaning up..."
	/usr/bin/find . -name ".*~" -exec /bin/rm -f {} \;

install: ${DSTDIR}
	echo -n "    Dot files:"
	for i in ${DOTFILES} ; \
	  do \
		echo -n " .$$i" ; \
		install -c -o ${DSTOWNER} -g ${DSTGROUP} -m ${DOTMODE} dot.$$i ${DSTDIR}/.$$i ; \
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
