#! /usr/bin/perl

my $dpkglibdir, $version;

BEGIN { 
    $dpkglibdir = ".";
    $version = "-1"; # This line modified by Makefile
    push (@INC, $dpkglibdir); 
}

use POSIX;
#use POSIX qw(:errno_h);

require 'controllib.pl';

$format = 'debian';
$changelogfile = 'debian/changelog';

@parserpath = ("/usr/local/lib/dpkg/parsechangelog",
	       "$dpkglibdir/parsechangelog");

sub usageversion {
    print STDERR
"Debian GNU/Linux dpkg-source $version.  Copyright (C) 1996
Ian Jackson.  This is free software; see the GNU General Public Licence
version 2 or later for copying conditions.  There is NO warranty.

Usage: dpkg-parsechangelog [<option> ...]
Options:  -l<changelogfile>      get per-version info from this file
          -v<sinceversion>       include all changes later than version
          -F<changelogformat>    force change log format
          -L<libdir>             look for change log parsers in <libdir>
          -h                     print this message
";
}

@ap=();
while (@ARGV) {
    last unless $ARGV[0] =~ m/^-/;
    $_= shift(@ARGV);
    if (m/^-L/ && length($_)>2) { $libdir=$'; next; }
    if (m/^-F([0-9a-z]+)$/) { $force=1; $format=$1; next; }
    if (m/^-l/ && length($_)>2) { $changelogfile=$'; next; }
    push(@ap,$_);
    m/^--$/ && last;
    m/^-v/ && next;
    &usageerr ("unknown option \"$_\"");
}

@ARGV && &usageerr ("$progname takes no non-option arguments");
$changelogfile= "./$changelogfile" if $changelogfile =~ m/^\s/;

if (!$force) {
    open(STDIN,"< $changelogfile") ||
        &error("cannot open $changelogfile to find format: $!");
    open(P,"tail -40 |") || die "cannot fork: $!\n";
    while(<P>) {
        next unless m/\schangelog-format:\s+([0-9a-z]+)\W/;
        $format=$1;
    }
    close(P); $? && &subprocerr("tail of $changelogfile");
}

for $pd (@parserpath) {
  $pa = "$pd/$format";
  if (! stat ("$pa")) {
    $! == ENOENT || &syserr ("fatal error while checking for format parser \"$pa\"");
  } elsif (! -x _) {
    &warn ("format parser \"$pa\" not executable; ignoring");
  } else {
    $pf = $pa;
  }
}
        
if (! defined ($pf)) {
  &error ("unable to find parser for changelog format \"$format\"");
}

open (STDIN, "< $changelogfile") || die "unable to open change-log file \"$changelogfile\": $!\n";
exec ($pf, @ap);
&die ("unable to execute format parser \"$pf\": $!\n");
