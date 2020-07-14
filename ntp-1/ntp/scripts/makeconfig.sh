#!/bin/sh

MACHINE=${1-${OS}}
COMPILER=${2-${CC}}

#
# Figure out which compiler to use. Stolen from Jeff Johnson.
#
if [ -f /usr/bin/uname ]; then
	if [ -x /usr/bin/uname -a `/usr/bin/uname -s` = "BSD/386" ]; then
		COMPILER=cc
	fi
fi

if [ "0$COMPILER" = "0" ]; then
	COMPILER="cc"
	set dummy gcc; word=$2
	IFS="${IFS=   }"; saveifs="$IFS"; IFS="${IFS}:"
	for dir in $PATH; do
	        test -z "$dir" && dir=.
	        if test -f $dir/$word; then
       	         COMPILER="gcc"
       	         break
        	fi
	done
	IFS="$saveifs"
fi

#
# Figure out the byte order and word size.
#
if (cd util && rm -f longsize && $COMPILER -o longsize longsize.c ); then
	if util/longsize >/dev/null 2>&1; then
		LONG=`util/longsize`
	else
		echo "TROUBLE: executables built by your compiler don't work - bug your vendor"
		exit 1
	fi
else
	echo "TROUBLE: could not compile !"
	echo "TROUBLE: either your compiler does not work / is not present"
	echo "TROUBLE: or you have mangled the file tree"
	exit 1
fi
(cd util && rm -f byteorder && $COMPILER -o byteorder byteorder.c $LONG )
BYTE=`util/byteorder `
if [ "0$BYTE" = "0" ]; then
    BYTE="XNTP_BIG_ENDIAN"
fi
(cd util && rm -f byteorder longsize)

INCDEFS=
#
# Figure out what includes to use for kernel mods. The bit of nastiness
# about PPS_SYNC is brought to you by linux.
#
if [ -f /usr/include/sys/timex.h ]; then
	PPSDEFS=`grep PPS_SYNC /usr/include/sys/timex.h`
	if [ "0${PPSDEFS}" != "0" ]; then
		INCDEFS="$INCDEFS -DKERNEL_PLL"
		if [ -f /usr/include/sys/syscall.h ]; then
			GETTIME=`grep ntp_gettime /usr/include/sys/syscall.h`
			ADJTIME=`grep ntp_adjtime /usr/include/sys/syscall.h`
			if [ "0${GETTIME}${ADJTIME}" = "0" ]; then
	 			INCDEFS="$INCDEFS -DNTP_SYSCALLS_LIBC"
				echo "precision time kernel library found"
			else
				echo "precision time kernel syscalls found"
			fi
		fi
	fi
fi
 
#
# Figure out which line discipline/streams module is configured
#
if [ -f /usr/include/sys/clkdefs.h ]; then
	INCDEFS="$INCDEFS -DTTYCLK"
	echo "tty_clk line discipline/streams module found"
fi
if [ -f /usr/include/sys/chudefs.h ]; then
	INCDEFS="$INCDEFS -DCHUCLK"
	echo "chu_clk line discipline/streams module found"
fi
if [ -f /usr/include/sys/ppsclock.h ]; then
	INCDEFS="$INCDEFS -DPPS"
	echo "ppsclock streams module found"
fi

#
# Figure out what includes to use for PPS mods
#
if [ -f /usr/include/sys/ppsclock.h ]; then
	if [ -f /usr/include/sys/termios.h ]; then
		GPPS=`grep TIOCGPPS /usr/include/sys/termios.h`
		SPPS=`grep TIOCSPPS /usr/include/sys/termios.h`
		GPPSEV=`grep TIOCGPPSEV /usr/include/sys/termios.h`
		if [ "0${GPPS}${SPPS}${GPPSEV}" != "0" ]; then
	 		INCDEFS="$INCDEFS -DPPS"
		fi
	fi
fi

#
# Figure out what includes to use for multicast kernel
#
if [ -f /usr/include/netinet/in.h ]; then
	MULTICAST=`grep IP_MULTICAST /usr/include/netinet/in.h`
	if [ "0${MULTICAST}" != "0" ]; then
		INCDEFS="$INCDEFS -DMCAST"
		echo "multicast kernel found"
	fi
fi

#
# Figure out which machine we have.
#
if [ "0$MACHINE" = "0" ]; then
    GUESS=`scripts/Guess.sh`
    if [ "0$GUESS" = "0none" ]; then
        echo ' '
        echo "I don't know your system!"
	echo "I do know about the following systems:"
        (cd machines && ls -C *)
	echo "Choose a system and type \"make OS=<system>\"" 
	exit 1
    else
	if [ -f machines/$GUESS ]; then
	     MACHINE=$GUESS
	else
	     if [ -f machines/$GUESS.posix ]; then
	         MACHINE="$GUESS.posix"
	     else
	         MACHINE="$GUESS.bsd"
	     fi
	fi
    fi
fi

echo "Configuring machines/$MACHINE compilers/$MACHINE.$COMPILER"

if [ -f machines/$MACHINE ]; then 
	cat machines/$MACHINE  >Config ; 
	if [ -f compilers/$MACHINE.$COMPILER ]; then 
	    cat compilers/$MACHINE.$COMPILER >>Config  
	else 
     	    echo "COMPILER= $COMPILER" >>Config 
	fi 
     	echo "LIBDEFS= -D$BYTE" >>Config 
        echo "DEFS_INCLUDE=$INCDEFS" >>Config
	cat Config.local >>Config
else 
	echo "Don't know how to build xntpd for machine $MACHINE " ; 
	exit 1 
fi
