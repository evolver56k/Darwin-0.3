#!./perl

# $RCSfile: cpp.t,v $$Revision: 1.1.1.1 $$Date: 1999/04/23 01:28:39 $

BEGIN {
    chdir 't' if -d 't';
    @INC = '../lib';
}

use Config;
if ( $^O eq 'MSWin32' or
     ($Config{'cppstdin'} =~ /\bcppstdin\b/) and
     ( ! -x $Config{'binexp'} . "/cppstdin") ) {
    print "1..0\n";
    exit; 		# Cannot test till after install, alas.
}

system "./perl -P comp/cpp.aux"
