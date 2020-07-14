#! /usr/bin/perl

use strict;
use diagnostics;

package Dpkg::Package::Manifest;

sub readdir {
  
  my ($dir) = @_;
  my @ret;
  
  if (-d $dir) {
    
    opendir (DIR, "$dir");
    my @list = readdir (DIR);
    close (DIR);
    foreach my $i (@list) {
      next if $dir =~ /\.|\.\./;
      push (@ret, { 'type' => 'dir', 'source' => ($dir . '/' . $i) });
    }
    return @ret;
    
  } else {
    
    open (DIR, "<$dir") || die ("unable to open \"$dir\": $!");
    while (<DIR>) {
      chomp;
      s/\#.*//og;
      next if /^ *$/;
      my @l = split;
      my $r = { 'type' => $l[0], 'source' => $l[1] };
      $r->{'targets'} = $l[2];
      push (@ret, $r);
    }
    return @ret;
  }
}

1;
