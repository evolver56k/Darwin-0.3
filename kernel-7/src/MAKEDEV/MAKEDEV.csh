#!/bin/sh -

##
# Copyright (c) 1999 Apple Computer, Inc. All rights reserved.
#
# @APPLE_LICENSE_HEADER_START@
# 
# Portions Copyright (c) 1999 Apple Computer, Inc.  All Rights
# Reserved.  This file contains Original Code and/or Modifications of
# Original Code as defined in and that are subject to the Apple Public
# Source License Version 1.1 (the "License").  You may not use this file
# except in compliance with the License.  Please obtain a copy of the
# License at http://www.apple.com/publicsource and read it before using
# this file.
# 
# The Original Code and all software distributed under the License are
# distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, EITHER
# EXPRESS OR IMPLIED, AND APPLE HEREBY DISCLAIMS ALL SUCH WARRANTIES,
# INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE OR NON- INFRINGEMENT.  Please see the
# License for the specific language governing rights and limitations
# under the License.
# 
# @APPLE_LICENSE_HEADER_END@
##

#
#	@(#)MAKEDEV	8.1 (Berkeley) 6/9/93
#
# Device "make" file.  Valid arguments:
#	std	standard devices
#	rhapsody	devices for rhapsody
#	local	configuration specific devices
# Tapes:
#	st*	SCSI mag tape driver
# Disks:
#	rd*	RAM disk
#	od*	Optical disk
#	oc	Optical disk controller
#	hd*	IDE disks
#	sd*	SCSI disks
#	fd*	Floppy disks
# Terminal multiplexors:
#	tty[ab]	scc terminal drivers
# Pseudo terminals:
#	pty*	set of 16 master and slave pseudo terminals
# Printers:
#	np*	NeXT Printer
# Call units:
#	NONE
# Special purpose devices:
#	vid*	frame buffer
#	ev*	event driver
#	sg*	Generic SCSI Driver
#	nvram	Nonvolatile RAM driver
#	hfs_sd*	HFS partitions on SCSI disks
#	hfs_hd*	HFS partitions on IDE disks
#	hfs_fd*	HFS partition on floppy disks
umask 77
PATH=/sbin:/bin/:/usr/bin:/usr/sbin:$PATH
export PATH
for i
do
case $i in

rhapsody)
	$0 std hd0 hd1 hfs_hd0 hfs_hd1 oc od0 od1 sd0 sd1 sd2 sd3 sd4 sd5 sd6 sd7 sd8 sd9 sd10 sd11 sd12 sd13 sd14 sd15 sc sg0 sg1 sg2 sg3 st0 st1 fd0 fd1 hfs_fd0 hfs_fd1 fc vol pty0 pty1 vid0 ev0 evs0 np0 nps0 pp nvram hfs_sd0 hfs_sd1 hfs_sd2 hfs_sd3 hfs_sd4 hfs_sd5 hfs_sd6 hfs_sd7 hfs_sd8 hfs_sd9 hfs_sd10 hfs_sd11 hfs_sd12 hfs_sd13 hfs_sd14 hfs_sd15
	;;

std)
	mknod console	c 0 0	; chmod 622 console
	mknod tty	c 2 0	; chmod 666 tty
	mknod mem	c 3 0	; chmod 640 mem ; chgrp kmem mem
	mknod kmem	c 3 1	; chmod 640 kmem ; chgrp kmem kmem
	mknod null	c 3 2	; chmod 666 null
	mknod dsp	c 3 3	; chmod 666 dsp
	mknod klog	c 6 0	; chmod 600 klog
	mknod drum	c 7 0	; chmod 640 drum ; chgrp kmem drum
	mknod dialup0	c 16 0  ; chmod 600 dialup0
	mknod dialup1	c 16 1	; chmod 600 dialup1
	mknod sound	c 36 0	; chmod 600 sound
	;;

od*|rd*|sd*|hd*)
	umask 2 ; unit=`expr $i : '..\(.*\)'`
	case $i in
	od*) name=od; blk=2; chr=9;;
	rd*) name=rd; blk=5; chr=15;;
	sd*) name=sd; blk=6; chr=14;;
	hd*) name=hd; blk=3; chr=15;;
	esac
	case $unit in
	0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15|16|\
	17|18|19|20|21|22|23|24|25|26|27|28|29|30|31)
		mknod ${name}${unit}a	b $blk `expr $unit '*' 8 + 0`
		mknod ${name}${unit}b	b $blk `expr $unit '*' 8 + 1`
		mknod ${name}${unit}c	b $blk `expr $unit '*' 8 + 2`
		mknod ${name}${unit}g	b $blk `expr $unit '*' 8 + 6`
		mknod r${name}${unit}a	c $chr `expr $unit '*' 8 + 0`
		mknod r${name}${unit}b	c $chr `expr $unit '*' 8 + 1`
		mknod r${name}${unit}c	c $chr `expr $unit '*' 8 + 2`
		mknod r${name}${unit}g	c $chr `expr $unit '*' 8 + 6`
		mknod ${name}${unit}d	b $blk `expr $unit '*' 8 + 3`
		mknod ${name}${unit}e	b $blk `expr $unit '*' 8 + 4`
		mknod ${name}${unit}f	b $blk `expr $unit '*' 8 + 5`
		mknod ${name}${unit}h	b $blk `expr $unit '*' 8 + 7`
		mknod r${name}${unit}d	c $chr `expr $unit '*' 8 + 3`
		mknod r${name}${unit}e	c $chr `expr $unit '*' 8 + 4`
		mknod r${name}${unit}f	c $chr `expr $unit '*' 8 + 5`
		mknod r${name}${unit}h	c $chr `expr $unit '*' 8 + 7`
		chgrp operator ${name}${unit}[a-h] r${name}${unit}[a-h]
		chmod 640 ${name}${unit}[a-h] r${name}${unit}[a-h]
		;;
	*)
		echo bad unit for disk in: $i
		;;
	esac
	umask 77
	;;

# bknight - This case must precede the next one because it is a special case.

hfs_fd*)
	umask 2 ; unit=`expr $i : '......\(.*\)'`
	case $i in
	hfs_fd*) name=fd; blk=1; chr=41;;
	esac
	case $unit in
	0|1|2|3|4|5|6|7)
		mknod ${name}${unit}_hfs_a	b $blk `expr $unit '*' 8 + 128 + 0`
		chgrp operator ${name}${unit}_hfs_a
		chmod 660 ${name}${unit}_hfs_a
		;;
	*)
		echo bad unit for HFS disk in: $i
		;;
	esac
	umask 77
	;;

hfs_*)
	umask 2 ; unit=`expr $i : '......\(.*\)'`
	case $i in
	hfs_sd*) name=sd; blk=6; chr=14;;
	hfs_hd*) name=hd; blk=3; chr=15;;
	esac
	case $unit in
	0|1|2|3|4|5|6|7|8|9|10|11|12|13|14|15)
		mknod ${name}${unit}_hfs_a	b $blk `expr $unit '*' 8 + 128 + 0`
		mknod ${name}${unit}_hfs_b	b $blk `expr $unit '*' 8 + 128 + 1`
		mknod ${name}${unit}_hfs_c	b $blk `expr $unit '*' 8 + 128 + 2`
		mknod ${name}${unit}_hfs_g	b $blk `expr $unit '*' 8 + 128 + 6`
		mknod ${name}${unit}_hfs_d	b $blk `expr $unit '*' 8 + 128 + 3`
		mknod ${name}${unit}_hfs_e	b $blk `expr $unit '*' 8 + 128 + 4`
		mknod ${name}${unit}_hfs_f	b $blk `expr $unit '*' 8 + 128 + 5`
		mknod ${name}${unit}_hfs_h	b $blk `expr $unit '*' 8 + 128 + 7`
		chgrp operator ${name}${unit}_hfs_[a-h]
		chmod 640 ${name}${unit}_hfs_[a-h]
		;;
	*)
		echo bad unit for HFS disk in: $i
		;;
	esac
	umask 77
	;;

fd*)
	umask 2 ; unit=`expr $i : '..\(.*\)'`
	name=fd; blk=1; chr=41;
	case $unit in
	0|1|2|3|4|5|6|7)
		mknod ${name}${unit}a	b $blk `expr $unit '*' 8 + 0`
		mknod r${name}${unit}a	c $chr `expr $unit '*' 8 + 0`
		mknod r${name}${unit}b	c $chr `expr $unit '*' 8 + 1`
		chgrp operator ${name}${unit}[a-b] r${name}${unit}[a-b]
		chmod 660 ${name}${unit}[a-b] r${name}${unit}[a-b]
		;;
	*)
		echo bad unit for disk in: $i
		;;
	esac
	umask 77
	;;
	
oc)
	mknod odc0	c 9 255		; chmod 644 odc0
	;;

fc)
	mknod fdc0	c 41 64		; chmod 644 fdc0
	;;

sc)
	mknod sdc0	c 14 128	; chmod 644 sdc0
	;;

vol)
	mknod vol0	c 42 0		; chmod 644 vol0
	;;

pp)
	mknod pp0	c 40 0		; chmod 666 pp0
	;;

nvram)
	mknod nvram	c 37 0		; chmod 600 nvram
	;;

sg*)
	umask 2 ; unit=`expr $i : '..\(.*\)'`
	case $i in
	sg*) chr=33;;
	esac
	case $unit in
	0|1|2|3)
		mknod $i	c $chr $unit
		chgrp operator $i
		chmod 666 $i 
		;;
	*)
		echo bad unit for device in: $i
		;;
	esac
	umask 77
	;;

st*)
	umask 2 ; unit=`expr $i : '..\(.*\)'`
	case $i in
	st*) chr=34;;
	esac
	case $unit in
	0|1)
		one=`expr $unit '*' 8 + 1`
		two=`expr $unit '*' 8 + 2`
		three=`expr $unit '*' 8 + 3`
		mknod rst$unit	c $chr `expr $unit '*' 8`
		mknod nrst$unit	c $chr $one
		mknod rxt$unit	c $chr $two
		mknod nrxt$unit	c $chr $three
		chgrp operator rst$unit
		chgrp operator nrst$unit
		chgrp operator rxt$unit
		chgrp operator nrxt$unit
		chmod 666 rst$unit 
		chmod 666 nrst$unit 
		chmod 666 rxt$unit 
		chmod 666 nrxt$unit 
		;;
	*)
		echo bad unit for device in: $i
		;;
	esac
	umask 77
	;;

pty*)
	class=`expr $i : 'pty\(.*\)'`
	case $class in
	0) offset=0 name=p;;
	1) offset=16 name=q;;
	2) offset=32 name=r;;
	3) offset=48 name=s;;
	4) offset=64 name=t;;
	5) offset=80 name=u;;
	*) echo bad unit for pty in: $i;;
	esac
	case $class in
	0|1|2|3|4|5)
		umask 0
		eval `echo $offset $name | awk ' { b=$1; n=$2 } END {
			for (i = 0; i < 16; i++)
				printf("mknod tty%s%x c 4 %d; \
					mknod pty%s%x c 5 %d; ", \
					n, i, b+i, n, i, b+i); }'`
		umask 77
		;;
	esac
	;;

vid*)
	name=vid; chr=13;
	unit=`expr $i : 'vid\(.*\)'`
	case $unit in

	0|1|2|3|4|5|6|7)
		umask 0
		mknod ${name}${unit} c ${chr} ${unit}
		chmod 600 vid${unit}
		umask 77
		;;
	*)
		echo bad unit for ${name} in: $i
		;;
	esac
	;;

ev*|evs*|np*|nps*|midi*|ipl*)
	case $i in
	nps*)
		name=nps; chr=39;
		unit=`expr $i : 'nps\(.*\)'`
		;;
	np*)
		name=np; chr=8;
		unit=`expr $i : 'np\(.*\)'`
		;;
	ev?)
		name=ev; chr=10;
		unit=`expr $i : 'ev\(.*\)'`
		;;
	evs?)
		name=evs; chr=40;
		unit=`expr $i : 'evs\(.*\)'`
		;;
	ipl*)
		name=ipl; chr=35;
		unit=`expr $i : 'ipl\(.*\)'`
		;;
	esac
	case $unit in

	0|1|2|3|4|5|6|7)
		umask 0
		mknod ${name}${unit} c ${chr} ${unit}
		umask 77
		;;
	*)
		echo bad unit for ${name} in: $i
		;;
	esac
	;;

tty?)
	case $i in
	tty?)
		name=tty; chr=11;
		unit=`expr $i : 'tty\(.*\)'`
		;;
	esac
	case $unit in
	a|b)
		numunit=`echo $unit | tr ab 01`
		umask 0
		mknod ${name}${unit} c ${chr} ${numunit}
		umask 77
		;;
	*)
		echo bad unit for ${name} in: $i
		;;
	esac
	;;

local)
	sh /private/dev/MAKEDEV.local
	;;
esac
done
