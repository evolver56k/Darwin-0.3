#
# Makefile for the /private/spool directory
#

# Comment out this next line if you want to see all of the make output
.SILENT:

#	The directory we are building
DIR = /private/var/spool

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}

#	It doesn't mean anything to make in this directory
all:

#	Clean out editor lint
clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK

install: ${DSTDIR}
	echo -n "    Printer directories: lpd"
	mkdir -p -m 770 ${DSTDIR}/lpd
	chown daemon:daemon ${DSTDIR}/lpd
	echo -n " Faxes"
	mkdir -p -m 775 ${DSTDIR}/Faxes
	chown daemon:daemon ${DSTDIR}/Faxes
	echo "."
	echo -n "    Mail directories:"
	echo -n " mqueue"
	mkdir -p -m 750 ${DSTDIR}/mqueue
	chown root:wheel ${DSTDIR}/mqueue
	echo -n " syslog"
	touch ${DSTDIR}/mqueue/syslog
	chown root.staff ${DSTDIR}/mqueue/syslog
	chmod 640 ${DSTDIR}/mqueue/syslog
	echo "."

${DSTDIR}: DSTROOT ${DSTROOT}
	echo "    Making ${DSTDIR}"
	mkdir -p -m 755 ${DSTDIR}
	chown root:wheel ${DSTDIR}

$(DSTROOT):
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi
