#!/bin/sh

##
# This script sets up the machine enough to run single-user
##

##
# Set shell to ignore Control-C, etc.
# Prevent lusers from shooting themselves in the foot.
##
stty intr  undef
stty kill  undef
stty quit  undef
stty susp  undef
stty start undef
stty stop  undef
stty dsusp undef

. /etc/rc.common

PATH=/bin:/sbin

##
# Reset pretty boot state
##
fbshow -B -E

##
# Start with some a reasonable hostname
##
hostname localhost

##
# Are we booting from a CD-ROM?  If so, make a note of the fact.
##
if [ -d /System/Installation ] && [ -f /private/etc/rc.cdrom ]; then
    ConsoleMessage "Root device is mounted read-only"
    ConsoleMessage "Filesystem checks skipped"
    iscdrom=1
else
    iscdrom=0
fi

##
# Output the date for reference.
##
date

##
# We must fsck here before we touch anything in the filesystems.
##
fsckerror=0

# Don't fsck if we're single-user, or if we're on a CD-ROM.
if [ ${iscdrom} -ne 1 ]; then
    if [ "$1" = "singleuser" ]; then
	ConsoleMessage "Singleuser boot -- fsck not done"
	ConsoleMessage "Root device is mounted read-only."
	ConsoleMessage "If you want to make modifications to files,"
	ConsoleMessage "run '/sbin/fsck -y' first and then '/sbin/mount -uw /' "
    else
	# We're neither single-user nor on a CD-ROM.
	# Erase the rom's old-style login panel
	ConsoleMessage "Checking disk"

	# Benignly clean up ("preen") any dirty filesystems. 
	# fsck -p will skip disks which were properly unmounted during
	# a normal shutdown.
	fsck -p

	# fsck's success is reflected in its status.
	case $? in
	  0)
	    # No problems
	    ;;
	  2) 
	    # Request was made (via SIGQUIT, ^\) to complete fsck
	    # but prevent going multi-user.
	    ConsoleMessage "Request to remain single-user received"
	    fsckerror=1
	    ;;
	  4)
	    # The root filesystem was checked and fixed.  Let's reboot.
	    # Note that we do NOT sync the disks before rebooting, to
	    # ensure that the filesystem versions of everything fsck fixed
	    # are preserved, rather than being overwritten when in-memory
	    # buffers are flushed.
	    ConsoleMessage "Root filesystem fixed - rebooting"
	    reboot -q -n
	    ;;
	  8)
	    # Serious problem was found.
	    ConsoleMessage "Reboot failed - serious errors"
	    fsckerror=1
	    ;;
	  12)
	    # fsck was interrupted by SIGINT (^C)
	    ConsoleMessage "Reboot interrupted"
	    fsckerror=1
	    ;;
	  *)
	    # Some other error condition ocurred.
	    ConsoleMessage "Unknown error while checking disks"
	    fsckerror=1
	    ;;
	esac
    fi
fi

##
# Syncronize memory with filesystem
##
sync

##
# If booted into single-user mode from a CD-ROM, print out some hints 
# about how to fake up /tmp and get to other disks.
##
if [ "$1" = "singleuser" ] && [ ${iscdrom} -eq 1 ]; then
    echo ""
    echo "You are now in single-user mode while booted from a CD-ROM."
    echo "Since the root disk is read-only, some commands may not work as"
    echo "they normally do.  In particular, commands that try to create"
    echo "files in /tmp will probably fail.  One way to avoid this problem"
    echo "is to mount a separate hard disk or floppy on /tmp using the"
    echo "mount command. For example,'/sbin/mount /dev/fd0a /tmp' puts"
    echo "/tmp on the internal floppy disk."
    echo ""
fi

##
# Exit with error if failed above.
##
if [ ${fsckerror} -ne 0 ]; then
#	'****************************************'
    input=$(fbalert 'yn'				\
	'A serious problem has been detected on'	\
	'your boot disk. You may opt to attempt'	\
	'automatic repairs now, or you may enter'	\
	'single user mode and do manual repairs.'	\
	'Would you like to attempt repairs now?'	\
	'(y to repair, n for single user mode)'		\
	)
#	'****************************************'

    if [ "${input}" = "y" ]; then
	ConsoleMessage "Repairing disk"
	fsck -y

	ConsoleMessage "Rebooting"
	reboot -q -n
    else
	exit 1;
    fi
fi

##
# Exit
##
exit 0
