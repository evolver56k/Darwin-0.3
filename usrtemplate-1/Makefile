
UTC  = $(DSTROOT)/usr/template/client
UTCV = $(UTC)/var
ME   = $(DSTROOT)/Local/Users/me
UTU  = /usr/template/user
PRIV = /private

COPYDIRS = 		\
    Drivers		\
    etc			\
    tftpboot		\
    var/at		\
    var/db		\
    var/log		\
    var/run		\
    var/rwhos		\
    var/spools		\


install: $(DSTROOT) installhdrs
	@ echo ""
	@ echo Copying /private to /usr/template/client ...
	@ echo ""

	# create /usr/template/client directory
	rm -rf $(UTC)
	mkdir -p $(UTC)
	chmod 755 $(UTC)
	chown root.wheel $(UTC)
	@ echo ""
	
	# create /usr/template/client/var directory
	mkdir -p $(UTCV)
	chmod 755 $(UTCV)
	chown root.wheel $(UTCV)
	@ echo ""
	
	# copy some directories
	@ for dir in $(COPYDIRS); do					\
	    if [ -d $(PRIV)/$$dir ]; then set -e;			\
		install -d $(UTC);					\
		echo "(cd $(PRIV) && pax -rw $$dir $(UTC)/)";		\
		(cd $(PRIV) && pax -rw $$dir $(UTC)/);			\
	    fi;								\
	done
	@ echo ""
	
	# make /usr/template/client/cores
	@ echo Creating $(UTC)/cores ...
	mkdir -p $(UTC)/cores
	chown root.wheel $(UTC)/cores
	chmod 777 $(UTC)/cores
	@ echo ""
	
	# make /usr/template/client/tmp
	@ echo Creating $(UTC)/tmp ...
	mkdir -p $(UTC)/tmp
	chown root.wheel $(UTC)/tmp
	chmod 1777 $(UTC)/tmp
	@ echo ""
	
	# make /usr/template/client/var/tmp
	@ echo Creating $(UTC)/var/tmp ...
	mkdir -p $(UTC)/var/tmp
	chown root.wheel $(UTC)/var/tmp
	chmod 1777 $(UTC)/var/tmp
	@ echo ""
	
	# make /usr/template/client/var/vm
	@ echo Creating $(UTC)/var/vm ...
	mkdir -p $(UTC)/var/vm
	chown root.wheel $(UTC)/var/vm
	chmod 755 $(UTC)/var/vm
	@ echo ""

installhdrs:

installsrc: $(SRCROOT)
	cp -p Makefile PB.project $(SRCROOT)

clean:
