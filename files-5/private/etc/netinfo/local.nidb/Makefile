#
# Makefile for the /private/etc/netinfo/local.nidb directory.
#

# Comment out this line if you want make output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /private/etc/netinfo/local.nidb

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 750

#	Files to Install
FILES= Collection Transaction

MODE = 600

all:

clean:
	echo "    Cleaning up..."
	/usr/bin/find . -name ".*~" -exec /bin/rm -f {} \;

install: ${DSTDIR}
	echo -n "    Files:"
	for i in ${FILES} ; \
	  do \
		echo -n " $$i" ; \
		install -c -o ${DSTOWNER} -g ${DSTGROUP} -m ${MODE} $$i ${DSTDIR}/$$i ; \
	  done
	echo "."

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
