require 5.001;
use strict;
use diagnostics;

require Carp;
require Net::FTP;
require File::Path;
require File::Basename;
require File::Find;

require Dpkg::Package::List;

package Dpkg::Archive::Directory;

BEGIN {
    use vars qw (@ISA);
    @ISA = qw ();
}

sub new {
    my $this = shift;
    my $class = ref ($this) || $this;
    my %args = @_;

    if (! defined $args{'directory'}) { Carp::croak ("requires 'directory' as parameter");  }

    $args{'type'} = 'directory';

    my $self = \%args;

    bless ($self, $class);
    return $self;
}

sub connect {
    my $self = shift;
    return 0;
}

sub disconnect {
    my $self = shift;
    return 0;
}

sub check_packages {
    my $self = shift;
    my $dir = $self->{'directory'};
    print "Checking Packages file for $dir ...";
    if (-f "$dir/Packages") {
	print " up-to-date\n";
	return 0;
    } else {
	return 1;
    }
}

sub update_packages {
    my $self = shift;
    my ($remote_newer, $remote_name, $remote_modtime, $remote_gzipped) = 
	$self->check_packages ();
    if ($remote_newer) { 
	print " failed (no such file or directory)\n"; 
	return -1;
    } else {
	return 0;
    }
}

sub packages_file {
    my $self = shift;
    my $dir = $self->{'directory'};
    return ("$dir/Packages");
}

sub parse_packages {
    my $self = shift;
    my $dir = $self->{'directory'};
    $self->{'packages'} = Dpkg::Package::List->new ('filename' => "$dir/Packages");
}

sub packages_data {
    my $self = shift;
    return $self->{'packages'};
}
    
sub fetch_package {
    my $self = shift;
    my $package = shift;

    my $location = $package->{'filename'};
    if (! defined $location) { Carp::croak ("no location provided for $package->{'package'}"); } 
    $location =~ s|^(.*/)?binary-[^/]*/(.*)$|$2|o;
    my $dir = $self->{'directory'};

    if (-f "$dir/$location") { return "$dir/$location"; }
    return undef;
}

sub size_remaining {
    my $self = shift;
    my $package = shift;

    my $location = $package->{'filename'};
    if (! defined $location) { Carp::croak ("no location provided for $package->{'package'}"); } 
    $location =~ s|^(.*/)?binary-[^/]*/(.*)$|$2|o;
    my $dir = $self->{'directory'};

    if (-f "$dir/$location") { return 0; }
    return undef;
}    
