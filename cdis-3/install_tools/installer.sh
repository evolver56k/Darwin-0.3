#! /bin/sh

# pmpkginstall: a poor man's installer script.
#
# This script does everything Installer.app does, except for fancy graphics
# and getting the correct permissions to write a receipt (since we cannot
# make it setuid just for that; an obvious fix is to rewrite it in Perl).
#   Well, this may have changed: use --limitations to know for sure how 
# this tool performs at a given time.
#
#
# Yves Arrouye, 1997

: ${TMPDIR:=/tmp}

me=`basename $0`

version="1.3"

interactive=yes
action=install
receipt=1
verbose=0
dwiw=0

# Urk.

if [ -x /usr/bin/lsbom ]
then
    lsbom=/usr/bin/lsbom
else
    lsbom=/usr/etc/lsbom
fi

#

which() {
    for d in `echo $PATH | tr ':' ' '`
    do
	if [ -f ${d}/"$1" -a -x ${d}/"$1" ]
	then
	    echo ${d}/"$1"
	    return
	fi
    done
}

usage() {
    bye=$1
    : ${bye:=1}

    if [ $bye -eq 0 ]
    then
	echo -n U
    else
	echo -n u
	exec >&2
    fi

    echo "sage: $me [ --help ] [ --version ] [ --limitations ]"
    echo "       $me [ --help ] [ --interactive | --automatic ] [ --verbose ] [ --install ] [ --dwiw ] [ --noreceipt ] package [ installdir ]"
    echo "       $me [ --help ] [ --interactive | --automatic ] --delete [ --dwiw ] package"
    echo "       $me [ --help ] --info package"
    echo "       $me [ --help ] --list package"
   
    if [ $bye -eq 0 ]
    then
	cat <<EOF

Options: --help		print this help message
	 --version      print the version
	 --limitations  list limitations and differences with Installer.app
	 --interactive	ask if scripts are to be run (default)
	 --automatic	run scripts without asking
	 --verbose      do additional reporting
	 --install	install a package (default)
	 --dwiw         do what I want (even if it is forbidden; note that
			using this option is asking for trouble...)
	 --noreceipt    do not create an installation receipt
	 --delete	delete a package
	 --info		print the information of the package
	 --list		list the contents of a package
EOF
    fi

    exit $bye
}

limitations() {
    cat <<EOF
Known limitations:
  1. One must install packages as root in order to be able to write
     receipts in /Local/Library/Receipts. (More exactly, one must have
     the appropriate write permissions on this directory.) Use the
     --noreceipt option if you do not want to create a receipt. On
     the other hand, if you're installing a package in user space,
     the receipt will go to ~/Library/Receipts and this is just what
     you want...
  2. Multiple-volumes packages on floppy disks are not handled.
  3. If one installs packages without being root, the installation may
     fail because some files and directories do not have the correct
     permissions. It is recommend to first "remove" the package as
     root (using the --delete --dwiw flags) and reinstalling again as
     the user, or simply to install as root.


Known differences with Installer.app:
  1. The script does not check for files that will be overwritten.
     This is a design choice!
  2. It is possible to avoid answering questions about whether to run
     scripts or not; $me can be invoked by other tools.
  3. There are no fancy graphics.
EOF
}

bye() {
    if [ "`2>/dev/null getinfokey DisableStop`" != YES ]
    then
        exit
    fi
}

trap "bye"

getinfokey() {
    if [ ! -z "$package_info" ]
    then
        grep -i "^[ 	]*$1[ 	]*" $package_info | sed -n "s/^[ 	]*[^ 	]*[ 	]*\\(.*\\)/\\1/p"
    fi
}

while [ $# -ne 0 ]
do
    case "$1" in
	--help)
	    usage 0
	    ;;
	--version)
	    echo "$me version $version, by Yves Arrouye"
	    exit 0
	    ;;
	--limitations)
	    limitations
	    exit 0
	    ;;
	--interactive)
	    interactive=yes
	    ;;
	--automatic)
	    interactive=
	    ;;
	--verbose)
	    verbose=1
	    ;;
	--install)
	    action=install
	    ;;
	--dwiw)
	    dwiw=1
	    ;;
	--noreceipt)
	    receipt=0
	    ;;
	--delete)
	    action=delete
	    ;;
	--info)
	    action=info
	    ;;
	--list)
	    action=list
	    ;;
	-*)
	    usage;;
	*)
	    if [ -z "$package" ]
	    then
		package="$1"
	    else
		if [ -z "$installdir" ]
		then
		    installdir="$1"
		else
		    usage
		fi
	    fi
	    ;;
    esac
    shift
done

if [ -z "$package" ]
then
    usage
fi

if [ $action = "delete" -a ! -z "$installdir" ]
then
    usage
fi

if [ ! -d /Local/Library/Receipts ]
then
    2>/dev/null mkdir -p /Local/Library/Receipts
fi
if [ ! -w /Local/Library/Receipts ]
then
    receiptsdir=$HOME/Library/Receipts
else
    receiptsdir=/Local/Library/Receipts
fi

languages=`2>/dev/null defaults read NSGlobalDomain NSLanguages`
: ${languages="(English, French, German, Spanish, Italian, Swedish)"}
languages=`echo "$languages" | sed 's/[^a-zA-Z]/ /g'`

findfile() {
    for language in $languages
    do
	if [ -r "$2"/$language.lproj/"$3"."$1" ]
	then
	    echo "$2"/$language.lproj/"$3"."$1"
	    return
	fi
    done

    if [ -r "$2"/"$3"."$1" ]
    then
	echo "$2"/"$3"."$1"
    else
	for lproj in `find "$2" -name \*.lproj -print`
	do
	    if [ -r "$lproj"/"$3"."$1" ]
	    then
		echo "$lproj"/"$3"."$1"
		return
	    fi
	done
    fi
}

findinfo() {
    findfile info "$1" "$2"
}

findarchive() {
    for _e in .gz .Z
    do
	if [ -f "$1"/"$2".tar${_e} ]
	then
	    echo "$1"/"$2".tar${_e}
	fi
    done
}

check_uid() {
    if [ "`whoami`" = root ]
    then
	UID=0
    else
	if [ "$UID" = "" ]
	then
	    UID=-1
	fi
    fi

    export UID
}

check_auth() {
    needsauth=`getinfokey NeedsAuthorization | tr '[a-z]' '[A-Z]'`

    if [ "$needsauth" = YES ]
    then
 	: ${action:="mess with"}

        check_uid

	if [ $UID != 0 ]
	then
	    >&2 echo $me: you must be root to $action this package
	    exit 7
	fi
    fi
}

run_script() {
    _scriptoutput=${TMPDIR}/.`basename "$1"`.$$
    _scriptexit=${TMPDIR}/.`basename "$1"`.exit.$$
    (sh -c "$1 $2 $3" 2>&1; echo $? >$_scriptexit) | tee $_scriptoutput
    if [ ! -f $_scriptexit ]
    then
	_exitcode=1
    else
        _exitcode=`cat $_scriptexit`
    fi
    if [ -z "`2>/dev/null cat $_scriptoutput`" ]
    then
	if [ $_exitcode -eq 0 ]
	then
	    echo "OK."
	else
	    echo "ERR (script exited with code $_exitcode)."
	fi
    fi
    /bin/rm -f $_scriptoutput $_scriptexit
    if [ $_exitcode -ne 0 ]
    then
	exit $_exitcode
    fi
}

install_package() {
    if [ ! -d $package ]
    then
	if [ ! -f $package ]
	then
	    >&2 echo $me: $package does not exist
	else
            >&2 echo $me: $package does not look like an Installer package
	fi
        exit 2
    else
        case "$package" in
	    /*)
	        ;;
	    *)
	        package=`/bin/pwd`/$package
	        ;;
        esac
        package_name=`basename $package .pkg`

        package_info=`findinfo $package $package_name`

	src_location=`getinfokey SourceLocation`
	if [ -z "$src_location" ]
	then
    	    package_archive=`findarchive $package $package_name`
    	    if [ ! -f "$package_info" -o ! -f "$package_archive" ]
    	    then
		>&2 echo $me: $package lacks components required for its installation
		exit 2
    	    fi
	else
    	    case "$src_location" in
		/*)
	            ;;
		*)
		    # .. is needed because a relative src_location is relative
		    # to package loc, not package contents
	            src_location=$package/../$src_location
		    ;;
    	    esac
    	    if [ ! -f "$package_info" -o ! -d "$src_location" ]
    	    then
		>&2 echo $me: $package lacks components required for its installation
		exit 2
    	    fi
	fi

        package_sizes=$package/$package_name.sizes
        package_bom=$package/$package_name.bom

        if [ ! -f $package_sizes -o ! -f $package_bom ]
        then
    	    >&2 echo $me: $package lacks components required by Installer.app
	    >&2 echo $me: $package will be installed anyway, but should be corrected
        fi
    fi

    relocatable=`getinfokey Relocatable | tr '[a-z]' '[A-Z]'`

    if [ "$relocatable" != YES -a ! -z "$installdir" ]
    then
	if [ $dwiw -eq 0 ]
	then
            >&2 echo $me: package $package is not relocatable
            exit 3
	fi
    fi

    default_location=`getinfokey DefaultLocation`
    : ${installdir:=$default_location}

    if [ -z "$src_location" ]
    then
        # this used to look for installer.app to use it's tar versions
        uses_gnutar=1

        if [ $uses_gnutar -ne 1 ]
        then
            long_file_names=`getinfokey LongFileNames | tr '[a-z]' '[A-Z]'`
            case "$long_file_names" in
                YES*)
                    installer_tar=$installer_bin/installer_bigtar
                    ;;
                *)
                    installer_tar=$installer_bin/installer_tar
                    ;;
            esac
        else
            gnutar=`which gnutar`
            : ${gnutar:=/usr/bin/gnutar}
            if [ ! -x "$gnutar" ]
            then
                >&2 echo $me: "couldn't execute \`$gnutar' for gnutar"
                exit 7
            fi
            installer_tar=$gnutar
            installer_tar_opts=p
        fi

        case "$installer_tar" in
            gnutar|*/gnutar)
                uses_gnutar=1
                ;;
            *)
                uses_gnutar=0
                ;;
        esac

        case "$package_archive" in
            *.Z)
                package_tar=$TMPDIR/`basename $package_archive .Z`
                ;;
            *.gz)
                package_tar=$TMPDIR/`basename $package_archive .gz`
                ;;
        esac
    fi

    needs_copy=0

    pre_install=`findfile pre_install $package $package_name`
    post_install=`findfile post_install $package $package_name`

    if [ -f "$pre_install" -o -f "$post_install" ]
    then
        if [ ! -z "$interactive" ]
        then
	    echo "This package contains script that will be run during the installation..."
	    echo -n "Do you want to run the scripts? [y] "
	    read ans

	    case "$ans" in
	        y*|Y*|"")
		    ;;
	        *)
		    if [ $dwiw -ne 0 ]
		    then
			echo "Scripts not run. Installation may be incomplete."
			scripts=no
		    else
		        echo "Aborting installation."
		        exit 0
		    fi
		    ;;
	    esac
        fi
    fi

    check_auth

    echo Installing package $package
    if [ ! -d $installdir ]
    then
	if [ -x /bin/mkdirs ]
	then
	    /bin/mkdirs $installdir
	else
	    /bin/mkdir -p $installdir
	fi
	code=$?
	if [ $code -ne 0 ]
	then
	    exit $code
	fi
    fi

    if cd $installdir
    then
	:
    else
	>&2 echo $me: cannot cd to $installdir
	exit 4
    fi

    if [ -f "$pre_install" ]
    then
	if [ "$scripts" != no ]
	then
            echo -n Running pre_install script...\ 
	    run_script $pre_install $package $installdir
	else
	    echo Not running pre_install script.
	fi
    fi

    if [ "$UID" = 0 ]
    then
	# we'll use ditto
    fi

    if [ -z "$src_location" ]
    then
	echo -n Extracting package contents...\ 
	tarerrs=${TMPDIR}/.$me.tar.$$
	trap "/bin/rm -f $tarerrs; bye" 1 2 3 15
	if [ $needs_copy -eq 1 ]
	then
	    echo -n Copying... \ 
	    cp $package_archive $TMPDIR
	    /bin/rm -f $package_tar
	    gzip -d $package_archive
	    $installer_tar x${installer_tar_opts}f $package_tar 2>$tarerrs
	else
	    if [ $uses_gnutar -eq 1 ]
	    then
		$installer_tar xz${installer_tar_opts}f $package_archive 2>$tarerrs
	    else
		gzip -dc $package_archive | $installer_tar x${installer_tar_opts}f - 2>$tarerrs
	    fi
	fi
	code=$?

	/bin/rm -f $package_tar
	if [ $code -ne 0 ]
	then
	    echo "ERR (tar failed)."
	    if [ $verbose -ne 0 ]
	    then
		echo Tar log follows:
		cat $tarerrs
	    fi
	    /bin/rm -f $tarerrs
	    exit $code
	else
	    echo OK.
	fi
	/bin/rm -f $tarerrs
    else	# we have a src path, so use ditto
	echo -n Extracting package contents...\ 
	dittoerrs=${TMPDIR}/.$me.ditto.$$
	trap "/bin/rm -f $dittoerrs; bye" 1 2 3 15
	if [ $verbose -ne 0 ]
	then
	    dittoFlags=-V
	    echo
	    echo Source: $src_location
	    echo Destintaion: `pwd`
	fi
	if [ -r $package_bom ]
	then
	    dittoBOMFlags="-bom $package_bom"
	fi
	ditto $dittoFlags $dittoBOMFlags $src_location . 2>$dittoerrs

	code=$?
	if [ $code -ne 0 ]
	then
	    echo "ERR (ditto failed)."
	    if [ $verbose -ne 0 ]
	    then
		echo Ditto log follows:
		cat $dittoerrs
	    fi
	    /bin/rm -f $dittoerrs
	    exit $code
	else
	    echo OK.
	fi
    fi

    if [ -f "$post_install" ]
    then
	if [ "$scripts" != no ]
	then
            echo -n Running post_install script...\ 
            run_script $post_install $package $installdir
	else
	    echo Not running post_install script.
	fi
    fi
   
    if [ $receipt -ne 0 ]
    then
	if [ ! -d $receiptsdir ]
	then
	    2>/dev/null mkdir -p $receiptsdir
	fi
        if [ ! -w $receiptsdir ]
        then
            receipt_failed=yes
        else
            if [ -d $receiptsdir/$package_name.pkg ]
            then
	        if [ ! -w $receiptsdir/$package_name.pkg ]
	        then
		    receipt_failed=yes
	        fi
            fi
	fi

	if [ -z "$receipt_failed" ]
	then
            echo -n "Creating receipt for package in $receiptsdir... "
            /bin/rm -rf $receiptsdir/$package_name.pkg
            if [ -x /bin/mkdirs ]
            then
                /bin/mkdirs $receiptsdir/$package_name.pkg
            else
                /bin/mkdir -p $receiptsdir/$package_name.pkg
            fi
            for f in info bom sizes tiff
            do
                if [ -f $package/$package_name.$f ]
                then
    	            cp $package/$package_name.$f $receiptsdir/$package_name.pkg
                fi
            done
	    for f in software_version
	    do
                if [ -f $package/$f ]
		then
    	            cp $package/$f $receiptsdir/$package_name.pkg
		else
		    if [ -f /System/Library/CoreServices/$f ]
		    then
			cp /System/Library/CoreServices/$f $receiptsdir/$package_name.pkg
		    fi
		fi
	    done
	    if false
	    then
	        if [ ! -f $receiptsdir/$package_name.pkg/$package_name.info ]
	        then
		    firstinfo=`find $package -name \*.info -print | head -1`
		    if [ ! -z "$firstinfo" ]
		    then
		        cp $firstinfo $receiptsdir/$package_name.pkg
		    fi
	        fi
	    else
	        for d in `find $package -name \*.lproj -print`
	        do
	    	    (cd $package; gnutar cf - `basename $d`) | (cd $receiptsdir/$package_name.pkg; gnutar xpf -)
	        done
	    fi
        fi

        for f in pre_install post_install pre_delete post_delete
        do
            if [ -f $package/$package_name.$f ]
            then
	        cp $package/$package_name.$f $receiptsdir/$package_name.pkg
            fi
        done
        echo "$installdir" >$receiptsdir/$package_name.pkg/$package_name.location
        echo "installed" >$receiptsdir/$package_name.pkg/$package_name.status

	echo OK.
    else
	>&2 echo $me: cannot write a receipt for the package in $receiptsdir
	exit 4
    fi
}

delete_package() {
    if [ ! -d $package ]
    then
	if [ ! -f $package ]
	then
	    >&2 echo $me: $package does not exist
	else
            >&2 echo $me: $package does not look like an Installer package
	fi
        exit 2
    else
        case "$package" in
	    /*)
	        ;;
	    *)
	        package=`/bin/pwd`/$package
	        ;;
        esac
        package_name=`basename $package .pkg`

        package_info=`findinfo $package $package_name`

        package_location=$package/$package_name.location
        package_status=$package/$package_name.status

        if [ ! -r $package_location -o ! -r $package_status ]
	then
	    if [ $dwiw -ne 1 ]
	    then
	        >&2 echo $me: $package does not look like an installed package
	        exit 3
	    fi
	fi

	installdir="`cat $package/$package_name.location`"

	if [ -z "$installdir" -o ! -d "$installdir" ]
	then
	    if [ $dwiw -ne 1 ]
	    then
	        >&2 echo $me: $package has an incorrect installation directory
	        exit 3
	    else
		installdir=`getinfokey Location`
		if [ -z "$installdir" -o ! -d "$installdir" ]
		then
		    >&2 echo $me: cannot figure out the installation directory for $package
		else
		    >&2 echo $me: warning: assuming package $package was installed in its $installdir default location
		fi
	    fi
	fi

	if [ "`cat $package_status`" != installed ]
	then
	    if [ $dwiw -ne 1 ]
	    then
	        >&2 echo $me: $package was not properly installed
	        exit 3
	    fi
	fi

	package_bom=$package/$package_name.bom

	if [ ! -r $package_bom ]
	then
	    >&2 echo $me: $package lacks a bill of materials and cannot be delete
	    exit 3
	fi

	check_auth

	installonly=`getinfokey InstallOnly | tr '[a-z]' '[A-Z]'`
	if [ "$installonly" = YES ]
	then
	    if [ $dwiw -ne 1 ]
	    then
	        >&2 echo $me: $package can only be installed, not deleted
	        exit 4
	    fi
	fi

	deletewarning=`getinfokey DeleteWarning`
	if [ ! -z "$deletewarning" ]
	then
	    if [ ! -z "$interactive" ]
	    then
		echo $deletewarning
		echo -n "Do you really want to delete this package? [y] "
		read ans

	        case "$ans" in
	            y*|Y*|"")
		        ;;
	            *)
		        if [ $dwiw -ne 0 ]
		        then
			    echo "Scripts not run. Deletion may be incomplete."
			    scripts=no
		        else
		            echo "Aborting deletion."
		            exit 0
			fi
		        ;;
	        esac
	    fi
	fi

        pre_delete=`findfile pre_delete $package $package_name`
        post_delete=`findfile post_delete $package $package_name`

        if [ -f "$pre_delete" -o -f "$post_delete" ]
        then
            if [ ! -z "$interactive" ]
            then
	        echo "This package contains script that will be run during the deletion..."
	        echo -n "Do you want to run the scripts? [y] "
	        read ans
    
	        case "$ans" in
	            y*|Y*|"")
		        ;;
	            *)
		        echo "Aborting deletion"
		        exit 0
		        ;;
	        esac
            fi
        fi

	echo Deleting $package

        if [ -f "$pre_delete" ]
        then
	    if [ "$scripts" != no ]
	    then
            	echo -n Running pre_delete script...\ 
	    	run_script $pre_delete $package $installdir
	    else
	        echo Not running pre_delete script.
	    fi
        fi

	dirsfile=${TMPDIR}/.$me.$$

	/bin/rm -f $dirsfile
	trap "/bin/rm -f $dirsfile; bye" 1 2 3 15

	echo -n Deleting package contents...\ 

	for i in `$lsbom $package_bom | sed 's/[ 	][ 	]*[0-9].*//'`
	do
	    what="$installdir"/"$i"

	    if [ -d "$what" ]
	    then
		(cd "$what"; /bin/pwd) >>$dirsfile
	    else
		2>/dev/null /bin/rm -f "$what"
		code=$?
		if [ $code -ne 0 ]
		then
		    echo "ERR (removing $what)"
		    exit $code
		fi
	    fi
	done

	for i in `sort -r $dirsfile`
	do
	    if [ -d "$i" ]
	    then
		2>/dev/null /bin/rmdir "$i"
	    fi
	done

        /bin/rm -f $dirsfile

        if [ -f "$post_delete" ]
        then
	    if [ "$scripts" != no ]
	    then
            	echo -n Running post_delete script...\ 
	    	run_script $post_delete $package $installdir
	    else
	        echo Not running post_delete script.
	    fi
        fi

	echo Deleting $package

	/bin/rm -rf $package
    fi
}

info_package() {
    if [ ! -d $package ]
    then
	if [ ! -f $package ]
	then
	    >&2 echo $me: $package does not exist
	else
            >&2 echo $me: $package does not look like an Installer package
	fi
        exit 2
    else
        case "$package" in
	    /*)
	        ;;
	    *)
	        package=`/bin/pwd`/$package
	        ;;
        esac
        package_name=`basename $package .pkg`

        package_info=`findinfo $package $package_name`

        if [ ! -f "$package_info" ]
	then
	    >&2 echo $me: $package does not have a readable info file
	    exit 3
	fi

        title=`getinfokey Title`
	if [ ! -z "$title" ]
	then
	    echo $title
	fi
        version=`getinfokey Version`
	if [ ! -z "$version" ]
	then
	    echo Version: $version
	fi
        descr=`getinfokey Description`
	if [ ! -z "$descr" ]
	then
	    echo Description: $descr
	fi
	if [ -r $package/software_version ]
	then
	    echo Software Version: `cat $package/software_version`
	fi
        package_archive=`findarchive $package $package_name`
	case "$package_archive" in
	    *.Z)
		if [ $verbose -ne 0 ]
		then
		    echo Archive: Apparently made with BSD compress
		fi
		;;
	    *.gz)
		if [ $verbose -ne 0 ]
		then
		    echo "Archive: Apparently made with GNU gzip"
		fi
		;;
	esac

	src_location =`getinfokey SourceLocation`
	if [ ! -z "$src_location" ]
	then
	    if [ $verbose -ne 0 ]
	    then
		echo Non-archive: source located at $src_location
	    fi
	fi

	status="not installed"
	if [ -r $package/$package_name.status ]
	then
	    status=`cat $package/$package_name.status`
	    if [ -r $package/$package_name.location ]
	    then
		status="$status (in `cat $package/$package_name.location`)"
	    fi
	else
    	    default_location=`getinfokey DefaultLocation`
    	    relocatable=`getinfokey Relocatable`
	    if [ ! -z default_location ]
	    then
		status="$status (will install in $default_location"
		if [ "$relocatable" = YES ]
		then
		    status="$status by default"
		fi
		status="$status)"
	    fi
	fi
	echo Status: `echo $status | sed 's/^\(.\).*/\1/' | tr '[a-z]' '[A-Z]'``echo $status | sed 's/.\(.*\)$/\1/'`
    fi
}

list_package() {
    if [ ! -d $package ]
    then
	if [ ! -f $package ]
	then
	    >&2 echo $me: $package does not exist
	else
            >&2 echo $me: $package does not look like an Installer package
	fi
        exit 2
    else
        case "$package" in
	    /*)
	        ;;
	    *)
	        package=`/bin/pwd`/$package
	        ;;
        esac
        package_name=`basename $package .pkg`

        package_bom=$package/$package_name.bom

        if [ ! -r $package_bom ]
	then
	    >&2 echo $me: $package does not have a bill of materials
	    exit 3
	fi

        package_location=$package/$package_name.location

	if [ -r "$package_location" ]
	then
	    installdir="`cat $package_location`"
	fi

	$lsbom $package_bom | \
	    sed -e 's/[ 	][ 	]*[0-9].*//' \
		-e 's,^\./,,' -e "s,^,$installdir,"
    fi
}

${action}_package

