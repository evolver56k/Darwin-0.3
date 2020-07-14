#
# Makefile for the /private/etc directory
#

# Comment out this next line if you want to see all of the make output
.SILENT:

#	The directory we are building
DIR = /private/etc

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
RESOURCEDIR = ${DSTROOT}/System/Library/CoreServices/Resources/English.lproj
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

#	Ownership and group of the files and directories installed here
OWNER = root
GROUP = wheel
RESOWNER = root
RESGROUP = wheel

#	Files that we don't want people to edit
ROFILES = \
	fstab.hd \
	fstab.rd \
	fstab.sd \
	rc \
	rc.boot \
	rc.common \
	ttys.installer

ROMODE = 444

#	Localization resources that get installed into /System/Library/CoreServices
RESOURCES = Bootstrap.strings

#	Files that are ok to edit
RWFILES	= \
	afpovertcp.cfg \
	changelist \
	crontab \
	crontab.local \
	daily \
	ftpusers \
	gettytab \
	group \
	hostconfig \
	hosts \
	hosts.lpd \
	iftab \
	inetd.conf \
	master.passwd \
	monthly \
	motd \
	netgroup \
	networks \
	passwd \
	phones \
	printcap \
	protocols \
	remote \
	rpc \
	services \
	shells \
	syslog.conf \
	ttys \
	weekly \
	kern_loader.conf \
	bootstrap.conf 

RWMODE = 644

#	Files that are ok to edit that are initially empty
EMPTYFILES = find.codes hosts.equiv rmtab utmp xtab
EMPTYMODE = 644

#	Subdirectories with their own Makefiles
SUBDIRS = netinfo startup

#	It doesn't mean anything to make in this directory
all:
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

#	Clean out editor lint
clean:
	echo "    Removing" *~ .[a-zA-Z]*~ \#* *.BAK
	rm -f *~ .[a-zA-Z]*~ \#* *.BAK
	for i in ${SUBDIRS} ; \
	  do \
		echo ============= make $@ for ${THISDIR}/$$i ============= ; \
		(cd $$i; ${MAKE} $@ THISDIR=${THISDIR}/$$i) || exit $? ; \
	  done

install: ${DSTDIR}
	echo -n "    Read only files:"
	for i in `echo ${ROFILES}` ; \
	  do \
		echo -n " $$i" ; \
		install -c -o ${OWNER} -g ${GROUP} -m ${ROMODE} $$i ${DSTDIR}/$$i ; \
	  done
	echo "."
	echo -n "    Read/Write files:"
	for i in `echo ${RWFILES}` ; \
	  do \
		echo -n " $$i" ; \
		install -c -o ${OWNER} -g ${GROUP} -m ${RWMODE} $$i ${DSTDIR}/$$i ; \
	  done
	echo "."
	chmod 600 ${DSTDIR}/master.passwd
	echo -n "    Empty files:"
	for i in `echo ${EMPTYFILES}` ; \
	  do \
		echo -n " $$i" ; \
		touch ${DSTDIR}/$$i ; \
		chown ${OWNER}.${GROUP} ${DSTDIR}/$$i ; \
		chmod ${EMPTYMODE} ${DSTDIR}/$$i ; \
	  done
	echo "."
	echo -n "    Empty group-writeable files:"
	for i in `echo ${OPERATOREMPTYFILES}` ; \
	  do \
		echo -n " $$i" ; \
		touch ${DSTDIR}/$$i ; \
		chown ${OWNER}.${OPERATORGROUP} ${DSTDIR}/$$i ; \
		chmod ${GROUPEMPTYMODE} ${DSTDIR}/$$i ; \
	  done
	echo "."
	echo -n "    Localization resources:"
	for i in `echo ${RESOURCES}` ; \
	  do \
		echo -n " $$i" ; \
		mkdir -p ${RESOURCEDIR} ; \
		install -c -o ${RESOWNER} -g ${RESGROUP} -m ${ROMODE} $$i ${RESOURCEDIR}/$$i ; \
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
	@echo 'Please make a directory for $@ and try again'; exit 1

DSTROOT:
	@if [ -n "${$@}" ]; then exit 0; else echo Must define $@; exit 1; fi

