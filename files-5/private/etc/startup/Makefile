#
# Top level makefile for the FILES project.  This establishes the basic 
# files necessary for a Rhapsody filesystem.
#

# Comment out this line if you want make output
.SILENT:

# The makefile above hands us THISDIR for informational purposes
THISDIR=.

#	The directory we are building
DIR = /private/etc/startup

#	The directory that must be created
DSTDIR = ${DSTROOT}${DIR}
DSTOWNER = root
DSTGROUP = wheel
DSTMODE = 755

#	Files to Install
FILES= \
	0100_LocalMounts \
	0200_Tune \
	0300_VirtualMemory \
	0400_CleanUp \
	0500_MachMessaging \
	0600_Disks \
	0700_Devices \
	0800_Network \
	0900_SystemLog \
	1000_RPC \
	1100_DirectoryServices \
	1200_NetworkTime \
	1300_AppleTalk \
	1400_AppServices \
	1500_NFS \
	1600_AppleShare \
	1700_IPServices \
	1800_Mail \
	1900_QuickTime \
	2000_WebServer \
	2100_Cron \
	9000_Accounting
MODE= 755

all:

clean:
	echo "    Cleaning up..."
	/usr/bin/find . -name ".*~" -exec /bin/rm -f {} \;

install: ${DSTDIR}
	echo -n "    Files:"
	install -c -o ${DSTOWNER} -g ${DSTGROUP} -m ${MODE} ${FILES} ${DSTDIR}
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
                                                 
