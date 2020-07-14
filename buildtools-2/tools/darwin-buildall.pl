#! /usr/bin/perl

BEGIN {
    unshift @INC, '/usr/lib/perl5';
}

use strict;
use diagnostics;

use Dpkg::Package::Builder;
use Dpkg::Package::Manifest;

if ($#ARGV != 2) {
  print "usage: $0 <srclist> <repository> <dstdir>\n";
  exit (1);
}
  
my ($srcdir, $seeddir, $dstdir) = @ARGV;

my $repository = [ $dstdir, $seeddir ];
	
my @list = &Dpkg::Package::Manifest::readdir ($srcdir);

my $cwd;

BEGIN {
  $cwd = `pwd`;
  chomp ($cwd);
}

foreach my $e (@list) {

    my $type = $e->{'type'};
    my $source = $e->{'source'};
    my $targets = $e->{'targets'};

    chdir ($cwd) || die ("unable to chdir (\"$cwd\")");

    my ($package, $params) = &Dpkg::Package::Builder::scan ($type, $source);
    my $filename = &Dpkg::Package::Builder::exists ($package, 'any', $dstdir);
    if (! defined ($filename)) {
	my $name = $package->canon_name();
	print "must build $name.deb using $type $source\n";
	eval {
	    &Dpkg::Package::Builder::build ($type, $source, $repository, $targets, $dstdir, 1);
	};
	warn $@ if $@;
    } else {
	print "already have $filename\n";
    }
}
