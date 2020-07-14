#
# Makefile for the /usr/template directory.
#

# Comment out this line if you want all of the make commands output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /usr/template

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

TAR = /usr/bin/gnutar

#	Subdirectories without their own makefiles
SLAVEDIRS = user

#	Empty subdirectories
EMPTYDIRS = 

all:

clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	/bin/rm -f *~ .[a-zA-Z]*~ \#* *.BAK
	for i in ${SLAVEDIRS} ; \
	  do \
		echo "    Removing" $$i/*~ $$i/.[a-zA-Z]*~ $$i/\#* $$i/*.BAK ; \
		/bin/rm -f $$i/*~ $$i/.[a-zA-Z]*~ $$i/\#* $$i/*.BAK ; \
	  done

install: ${DSTDIR}
	for i in ${SLAVEDIRS} ; \
	  do \
		echo " Removing old" ${DSTDIR}/$$i ; \
		/bin/rm -rf ${DSTDIR}/$$i ; \
		echo " Copying" $$i to ${DSTDIR}/$$i ; \
		${TAR} cf - $$i | (cd ${DSTDIR}; ${TAR} xpf -) ; \
		echo " Setting" ${DSTDIR}/$$i to ug=rwX,o=rX ; \
		chown -R ${DSTOWNER}.${DSTGROUP} ${DSTDIR}/$$i ; \
		chmod -R u=rwX,og=rX ${DSTDIR}/$$i ; \
	  done
	echo -n "    Empty directories:"
#	for i in ${EMPTYDIRS} ; \
#	  do \
#		echo -n " $$i" ; \
#		mkdir -p -m ${DSTMODE} ${DSTDIR}/$$i ; \
#		chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}/$$i ; \
#	  done
#	echo "."

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdir -p -m ${DSTMODE} ${DSTDIR}
	chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
