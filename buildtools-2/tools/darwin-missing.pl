#! /usr/bin/perl

BEGIN {
    unshift @INC, '/usr/lib/perl5';
}

use strict;
use diagnostics;

use Dpkg::Package::Builder;
use Dpkg::Package::Manifest;

if ($#ARGV != 1) {
  print "usage: $0 <srclist> <dstdir>\n";
  exit (1);
}
  
my ($srclist, $dstdir) = @ARGV;
	
my @list = &Dpkg::Package::Manifest::readdir ($srclist);

for my $e (@list) {

    my $type = $e->{'type'};
    my $source = $e->{'source'};

    my ($package, $params) = &Dpkg::Package::Builder::scan ($type, $source);

    my $filename = &Dpkg::Package::Builder::exists ($package, 'any', $dstdir);
    if (! defined ($filename)) {
	my $name = $package->canon_name();
	print "must build $name.deb using $type $source\n";
    } else {
	# print "already have $filename.deb\n";
    }
}
