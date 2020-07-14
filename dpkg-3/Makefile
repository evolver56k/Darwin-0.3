##
# Makefile for dpkg
##

# Project info
Project               = dpkg.src
ProjectName           = dpkg
Extra_Configure_Flags = --localstatedir=/var --sharedstatedir=/etc --libdir=/usr/share
Extra_Install_Flags   = localstatedir=/private/var sharedstatedir=/private/etc libdir=/usr/share
UserType              = 
ToolType              = 

# It's a GNU Source project
include /System/Developer/Makefiles/CoreOS/ReleaseControl/GNUSource.make

install::
	mkdir -p $(DSTROOT)/System/Documentation/Administration/Commands
	mv $(DSTROOT)/usr/doc/dpkg $(DSTROOT)/System/Documentation/Administration/Commands/
	mv $(DSTROOT)/System/Documentation/Administration/Commands/dpkg/packaging.html \
	   $(DSTROOT)/System/Documentation/Administration/Commands/dpkg/packaging
	mv $(DSTROOT)/System/Documentation/Administration/Commands/dpkg/internals.html \
	   $(DSTROOT)/System/Documentation/Administration/Commands/dpkg/internals
	rm $(DSTROOT)/System/Documentation/Administration/Commands/dpkg/developer-keys.pgp
	rm $(DSTROOT)/usr/sbin/install-info
	rm $(DSTROOT)/usr/sbin/update-rc.d
	rm $(DSTROOT)/usr/sbin/start-stop-daemon
	rm $(DSTROOT)/usr/share/man/man8/install-info.8
	rm $(DSTROOT)/usr/share/man/man8/update-rc.d.8
	rm $(DSTROOT)/usr/share/man/man8/start-stop-daemon.8
	rmdir $(DSTROOT)/usr/doc
	rm -rf $(DSTROOT)/usr/etc
	mkdir -p $(DSTROOT)/System/Library/Frameworks/dpkg.framework/Headers
	mv $(DSTROOT)/usr/include/dpkg \
	   $(DSTROOT)/System/Library/Frameworks/dpkg.framework/Headers/
	rmdir $(DSTROOT)/usr/include
