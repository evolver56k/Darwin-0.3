#
# Makefile for the /private/var/log directory.
#

# Comment out this line if you want make output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /private/var/log

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 775

LOGFILES = system.log lpr.log mail.log ftp.log netinfo.log lookupd.log wtmp
LOGMODE = 640

SECUREFILES = secure.log
SECUREMODE = 600

all:

clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK

install: ${DSTDIR}
	echo -n "    Creating logfiles:"
	for i in ${LOGFILES} ; \
	  do \
		echo -n " $$i" ; \
		touch ${DSTDIR}/$$i ; \
		chmod ${LOGMODE} ${DSTDIR}/$$i ; \
	  done
	echo "."
	echo -n "    Creating secure logfiles:"
	for i in ${SECUREFILES} ; \
	  do \
		echo -n " $$i" ; \
		touch ${DSTDIR}/$$i ; \
		chmod ${SECUREMODE} ${DSTDIR}/$$i ; \
	  done
	echo "."

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdir -p -m ${DSTMODE} ${DSTDIR}
	chown ${DSTOWNER}:${DSTGROUP} ${DSTDIR}

$(DSTROOT):
	echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
