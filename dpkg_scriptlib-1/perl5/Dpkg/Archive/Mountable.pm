require 5.001;
use strict;
use diagnostics;

require Carp;
require Net::FTP;
require File::Path;
require File::Basename;
require File::Find;

require Dpkg::Package::List;

package Dpkg::Archive::Mountable;

BEGIN {
    use vars qw (@ISA);
    @ISA = qw (Dpkg::Archive::Directory);
}

my $mountnum = 0;
my $mounttime = time;

sub new {
    my $this = shift;
    my $class = ref ($this) || $this;
    my %args = @_;

    if (! defined $args{'mount-device'}) { Carp::croak ("requires 'mount-device' as parameter");  }
    if (! defined $args{'mount-type'}) { Carp::croak ("requires 'mount-type' as parameter");  }
    if (! defined $args{'directory'}) { Carp::croak ("requires 'directory' as parameter");  }

    $args{'type'} = 'mountable';

    my $self = \%args;

    bless ($self, $class);
    return $self;
}

sub connect {
    my $self = shift;

    $self->{'mountpoint'} = "$mountnum.$mounttime";

    my @args = ("mount", "-t", $self->{'mount-type'}, $self->{'mount-device'}, $self->{'mountpoint'});
    system (@args);
}

sub disconnect {
    my $self = shift;

    my @args = ("mount", "-t", $self->{'mount-type'}, $self->{'mount-device'}, $self->{'mountpoint'});
    while (system (@args) != 0) {
	print "Unable to unmount $self->{'mountpoint'}.  Press [RETURN] to try again.\n";
	<STDIN>;
    }
}
