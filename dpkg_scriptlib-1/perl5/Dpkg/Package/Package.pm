use strict;
use Carp;
require 5.001;

package Dpkg::Package::Package;

sub new {
    my $this = shift;
    my $class = ref ($this) || $this;
    my %args = @_;

    my $self = {};

    if (! defined $args{'data'}) { 
	die ("requires 'data' as parameter"); 
    }

    bless ($self, $class);

    $self->parse ('data' => $args{'data'});
    return $self;
}

sub parse {
  my $self = shift;
  my %args = @_;
  
  if (! defined $args{'data'}) { 
    die ("requires 'data' as parameter"); 
  }
  my $data = $args{'data'};
  
  $data =~ s/\n\s+/\376\377/g;	# fix continuation lines
  $data =~ s/\376\377\s*\376\377/\376\377/og;
  
  while ($data =~ m/^(\S+):\s*(.*)\s*$/mg) {
    my ($key, $value) = ($1, $2);
    $key =~ tr/[A-Z]/[a-z]/;
    $value =~ s/\376\377/\n /g;
    $self->{$key} = $value;
  }
  
  if (defined ($self->{'build-depends'})) {
    my @s = split (/[ ,]+/, $self->{'build-depends'});
    $self->{'build-depends'} = \@s;
  }
}

sub unparse {
    my $self = shift;
    my $ret = "";
    
    $ret .= "Package: " . $self->{'package'} . "\n";
    if (defined ($self->{'provides'})) {
	$ret .= "Provides: " . $self->{'provides'} . "\n";
    }	
    if (defined ($self->{'conflicts'})) {
	$ret .= "Conflicts: " . $self->{'conflicts'} . "\n";
    }	
    if (defined ($self->{'replaces'})) {
	$ret .= "Replaces: " . $self->{'replaces'} . "\n";
    }	
    $ret .= "Maintainer: " . $self->{'maintainer'} . "\n";
    $ret .= "Version: " . $self->{'version'} . "\n";
    $ret .= "Source: " . $self->{'source'} . "\n";
    if (defined ($self->{'build-depends'})) {
	my $r = $self->{'build-depends'};
	my $s = '';
	for my $dep (@$r) {
	    if ($s eq '') {
		$s = "$dep";
	    } else {
		$s .= ", $dep";
	    }
	}
	$ret .= "Build-Depends: $s\n";
    }	
    $ret .= "Architecture: " . $self->{'architecture'} . "\n";
    $ret .= "Description: " . $self->{'description'} . "\n";

    return $ret;
}
    
sub write {
    my $self = shift;
    my $file = shift;

    print $file $self->unparse();
}

sub canon_version {
    my $self = shift;

    my $r = $self->{'version'};
    if (defined ($self->{'revision'}) && defined ($self->{'package_revision'})) {
	die ("package has both revision and package_revision entries");
    }
    if (defined ($self->{'package_revision'})) {
	$r = $r . $self->{'package_revision'};
    }
    if (defined ($self->{'revision'})) {
	$r = $r . $self->{'revision'};
    }
    return $r;
}

sub canon_name {
    my $self = shift;
    return $self->{'package'} . "_" . $self->canon_version () . "_" . $self->{'architecture'};
}
