use strict;
use Carp;
require 5.001;

use Dpkg::Package::Package;

package Dpkg::Package::Index;

sub new {
    my $this = shift;
    my $class = ref ($this) || $this;
    my %args = @_;

    my $self = {};
    my @packages = ();
    $self->{'packages'} = \@packages;

    if (! defined $args{'packages'}) {
	croak ("requires 'packages' as parameter"); 
    }

    bless ($self, $class);
    $self->index ('packages' => $args{'packages'});
    return $self;
}

sub index {
    my $self = shift;
    my %args = @_;

    if (! defined $args{'packages'}) { 
	croak ("requires 'packages' as parameter"); 
    }

    my $package;
    my $packages = $args{'packages'}->packages();
    for $package (@$packages) {
	$self->{$package->{'package'}} = $package;
    }
}

sub lookup {
    my $self = shift;
    my $package = shift;
    return $self->{$package};
}
