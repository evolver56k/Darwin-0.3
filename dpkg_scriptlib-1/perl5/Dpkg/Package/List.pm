use strict;
use Carp;
require 5.001;

use Dpkg::Package::Package;

package Dpkg::Package::List;

sub new {
    my $this = shift;
    my $class = ref ($this) || $this;
    my %args = @_;

    my $self = {};
    my @packages = ();
    $self->{'packages'} = \@packages;

    if (! defined $args{'filename'}) {
	croak ("requires 'filename' as parameter"); 
    }

    bless ($self, $class);
    $self->parse ('filename' => $args{'filename'});
    return $self;
}

sub parse {
    my $self = shift;
    my %args = @_;

    if (! defined $args{'filename'}) { 
	croak ("requires 'filename' as parameter"); 
    }
    my $filename = $args{'filename'};
    if (! open (PACKAGE_FILE, $filename)) {
	croak ("Unable to load $filename: $!");
    }

    $/ = "";
    while (<PACKAGE_FILE>) {
	if (/^\s*$/) { next; }
	my $package = Dpkg::Package::Package->new ('data' => $_);
	my $packages = $self->{'packages'};
	unshift @$packages, $package;
    }
    $/ = "\n";
    close (PACKAGE_FILE);
}

sub packages {
    my $self = shift;
    return $self->{'packages'};
}
