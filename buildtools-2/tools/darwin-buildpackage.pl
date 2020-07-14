#! /usr/bin/perl

BEGIN {
    unshift @INC, '/usr/lib/perl5';
}

use strict;
use diagnostics;

use Dpkg::Package::Builder;

my $usage = "usage: $0 [ --cvs | --dir ] [ --target {all|headers|objs|local} ] <source> <repository> <dstdir>\n";

my ($type, $target, $srcdir, $seeddir, $dstdir);

if (defined ($ARGV[0]) && ($ARGV[0] =~ /^--(cvs|dir)$/)) {
  $type = $1;
  shift (@ARGV);
} else {
  print $usage;
  exit (1);
}

if (defined ($ARGV[0]) && ($ARGV[0] =~ /^--target$/)) {
  shift (@ARGV);
  $target = $ARGV[0];
  shift (@ARGV);
} else {
  $target = 'all';
}

if ($#ARGV != 2) {
  print $usage;
  exit (1);
}

($srcdir, $seeddir, $dstdir) = @ARGV;

my $repository = [ $dstdir, $seeddir ];

exit (&Dpkg::Package::Builder::build ($type, $srcdir, $repository, $target, $dstdir, 0));
