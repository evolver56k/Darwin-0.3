require 5.001;

use strict;
use diagnostics;

require Carp;
require Net::FTP;
require File::Path;
require File::Basename;
require File::Find;

require Dpkg::Package::List;

package Net::FTP;

sub nmdtm {
    my $ftp  = shift;
    my $file = shift;

    return undef
 	unless $ftp->_MDTM($file);

    my @gt = reverse ($ftp->message =~ /(\d{4})(\d\d)(\d\d)(\d\d)(\d\d)(\d\d)/);
    $gt[4] -= 1;
    timegm(@gt);
}

package Dpkg::Archive::FTP;

BEGIN {
    use vars qw (@ISA);
    @ISA = qw ();
}

sub readpass {
    print "Enter password for ftp: ";
    system ("stty", "-echo");
    my $pass = <STDIN>;
    chomp $pass;
    print "\n";
    system ("stty", "echo");
    return $pass;
}

sub new {
    my $this = shift;
    my $class = ref ($this) || $this;
    my %args = @_;

    if (! defined $args{'ftpsite'}) { Carp::croak ("requires 'ftpsite' as parameter");  }
    if (! defined $args{'passive'}) { Carp::croak ("requires 'passive' as parameter");  }
    if (! defined $args{'username'}) { Carp::croak ("requires 'username' as parameter");  }
    if (! defined $args{'password'}) { Carp::croak ("requires'password' as parameter");  }
    if (! defined $args{'ftpdir'}) { Carp::croak ("requires 'ftpdir' as parameter");  }
    if (! defined $args{'localdir'}) { Carp::croak ("requires 'localdir' as parameter");  }

    $args{'type'} = 'ftp';

    my $self = \%args;

    bless ($self, $class);
    return $self;
}

sub connect {

    my $self = shift;

    my ($ftpsite, $passive, $username, $password, $ftpdir) = 
	($self->{'ftpsite'}, $self->{'passive'}, $self->{'username'},
	 $self->{'password'}, $self->{'ftpdir'});

    print "Connecting to $ftpsite ...";
    my $ftp = Net::FTP->new ($ftpsite, Passive => $passive);
    if ($ftp && $ftp->ok) { 
	print " done\n";
    } else {
	print " failed\n";
	print "Unable to connect to $ftpsite; skipping site\n";
	return -1;
    }
    $ftp->debug (0);
    print "Logging in as $username ...";
    my $pass = $password;
    if ($pass eq "?") {
	$pass = &readpass ();
    }
    if ($ftp->login ($username, $pass)) { 
	print " done\n";
    } else {
	print " failed\n";
	print $ftp->message() . "\n";
	print "Unable to log in as $username; skipping site\n"; 
	return -1;
    }
    print "Setting transfer mode to binary ...";
    if ($ftp->binary ()) { 
	print " done\n"; 
    } else { 
	print " failed\n"; 
	print $ftp->message() . "\n";
	print "Unable to set transfer mode to binary; skipping site\n"; 
	return -1;
    }
    print "Changing directory to '$ftpdir' ...";
    if ($ftp->cwd ($ftpdir)) { 
	print " done\n"; 
    } else { 
	print " failed\n"; 
	print $ftp->message() . "\n";
	print "Unable to change directory to '$ftpdir'; skipping site\n"; 
	return -1;
    }

    $self->{'ftp'} = $ftp;
    return 0;
}    

sub check_packages {

    my $self = shift;
    if (! defined $self->{'ftp'}) { Carp::croak ("mirror is not connected");  }
    my ($ftp, $ftpdir, $localdir) = ($self->{'ftp'}, $self->{'ftpdir'}, $self->{'localdir'});

    my $local_modtime;
    my $remote_modtime;
    my $remote_name;
    my $remote_gzipped;

    # Install pacakge mirroring directory if necessary.

    if (system ("install", "-d", "$localdir")) {
	print "Unable to create package directory '$localdir'\n";
	return -1;
    }

    # First check modification time on remote packages file.
    # A Packages.gz file will be used by preference to a Packages file
    # regardless of any modification times.

    print "Checking Packages file for $ftpdir ...";
    $remote_modtime = $ftp->nmdtm ("$ftpdir/Packages.gz");
    if (! defined $remote_modtime) {
	$remote_modtime = $ftp->nmdtm ("$ftpdir/Packages");
	if (! defined $remote_modtime) {
	    print " failed\n";
	    print "Unable to find Packages[.gz] file in $ftpdir; not updating\n";
	    return -1;
	} else {
	    $remote_name = "Packages";
	    $remote_gzipped = 0;
	}
    } else {
	$remote_name = "Packages.gz";
	$remote_gzipped = 1;
    }

    # Check modification time on local Packages file.

    my @local_stat = stat ("$localdir/Packages");
    if ($#local_stat < 0) {
	$local_modtime = 0;
    } else {
	$local_modtime = $local_stat[9];
    }

    # Don't update if already up-to-date.

    if (($local_modtime == 0) || ($local_modtime < $remote_modtime)) { 
	return (1, $remote_name, $remote_modtime, $remote_gzipped);
    } elsif ($local_modtime > $remote_modtime) {
	print " more current than remote archive (not updating)\n";
	return (0, $remote_name, $remote_modtime, $remote_gzipped);
    } else {
	print " up-to-date\n";
	return (0, $remote_name, $remote_modtime, $remote_gzipped);
    }
}

sub update_packages {

    my $self = shift;
    if (! defined $self->{'ftp'}) { Carp::croak ("mirror is not connected");  }
    my ($ftp, $ftpdir, $localdir) = ($self->{'ftp'}, $self->{'ftpdir'}, $self->{'localdir'});

    my ($remote_newer, $remote_name, $remote_modtime, $remote_gzipped) = 
	$self->check_packages ();
    
    if ($remote_newer == 0) { return 0; }

    # Fetch Packages.gz if available; otherwise fetch Packages.
    # Use Packages~ or Packages.gz~ as the temporary filename.

    my $temp_path = "$localdir/$remote_name~";
    my $remote_path = "$ftpdir/$remote_name";

    unlink ("$localdir/$remote_name~");
    
    print " updating ...";
    if (! $ftp->get ("$remote_path", "$temp_path")) {
	print " failed\n";
	print "Unable to retrieve $remote_path; not updating";
	next;
    }
    print " done\n";

    # Compress or uncompress file as appropriate and move into
    # appropriate place in archive.
    
    unlink ("$localdir/Packages");
    unlink ("$localdir/Packages.gz");

    if ($remote_gzipped) {
	print "Uncompressing Packages file ...";
	if (system ("gzip -cd < $temp_path > $localdir/Packages")) {
	    print " failed\n";
	    print "Unable to uncompress Packages file; not updating";
	    return -1;
	}
	print " done\n";
	if (! rename ("$temp_path", "$localdir/Packages.gz")) {
	    print "Unable to rename $temp_path to $localdir/Packages.gz; not updating";
	    return -1;
	}
    } else {
	print "Uncompressing Packages file ...";
	if (system ("gzip -c9fq < $temp_path > $localdir/Packages.gz")) {
	    print " failed\n";
	    print "Unable to compress Packages file; not updating";
	    return -1;
	}
	print " done\n";
	if (! rename ("$temp_path", "$localdir/Packages")) {
	    print "Unable to rename $temp_path to $localdir/Packages; not updating";
	    return -1;
	}
    }

    utime $remote_modtime, $remote_modtime, "$localdir/Packages.gz";
    utime $remote_modtime, $remote_modtime, "$localdir/Packages";

    return 0;
}

sub packages_file {
    my $self = shift;
    return $self->{'localdir'} . "/Packages";
}

sub parse_packages {
    my $self = shift;
    $self->{'packages'} = Dpkg::Package::List->new ('filename' => $self->{'localdir'} . "/Packages");
}

sub packages_data {
    my $self = shift;
    return $self->{'packages'};
}
    
sub disconnect {
    my $self = shift;
    $self->{'ftp'}->close ();
}

sub size_remaining {
    my $self = shift;
    my $package = shift;

    my $location = $package->{'filename'};
    if (! defined $location) { Carp::croak ("no location provided for $package->{'package'}"); } 
    $location =~ s|^(.*/)?binary-[^/]*/(.*)$|$2|o;
    my $localdir = $self->{'localdir'};

    if (-f "$localdir/$location") { return 0; }
    elsif (-f "$localdir/$location~") { return ($package->{'size'} - (-s "$localdir/$location~")); }
    else { return ($package->{'size'}); }
}    

sub remove_installed {
    my $status = shift;
    my $statusindex = shift;
}

sub remove_installed_wanted {
    if ($_ =~ /\.deb$/) {
	print "Removing $_\n"; 
	unlink $_;
    }
}

sub remove_packages {
    my $self = shift;
    File::Find::find (\&remove_installed_wanted, $self->{'localdir'})
}

sub fetch_package {
    my $self = shift;
    my $package = shift;

    my $location = $package->{'filename'};
    if (! defined $location) { Carp::croak ("no location provided for $package->{'package'}"); } 
    $location =~ s|^(.*/)?binary-[^/]*/(.*)$|$2|o;
    my $localdir = $self->{'localdir'};
    my $pname = $package->{'package'};
    my $pversion = $package->canon_version ();
    my $ftp = $self->{'ftp'};

    if (! defined $ftp) { print " failed (not connected)\n"; }
    File::Path::mkpath ([File::Basename::dirname ("$localdir/$location")], 0, 0755);

    my $dest = "$localdir/$location";
    my $tmpdest = "$localdir/$location~";

    if (-f "$dest") {
	#print "Fetching $pname" . "_" . "$pversion from $self->{'ftpsite'} ... done (already present)\n";
	return $dest;
    }

    my $rval;
    if (-f "$tmpdest") {
	my $size = (-s "$tmpdest");
	print "Fetching $pname" . "_" . "$pversion from $self->{'ftpsite'} (continuing from $size) ...";
	$rval = $ftp->get ("$location", "$tmpdest", $size);
    } else {
	print "Fetching $pname" . "_" . "$pversion from $self->{'ftpsite'} ...";
	$rval = $ftp->get ("$location", "$tmpdest");
    }	
    
    if (! $rval) {
	if (($ftp->code() == 550) || ($ftp->code() == 550)) {
	    print " failed (file not found)\n";
	} else {
	    my $message = $ftp->message();
	    chomp ($message);
	    print "Error fetching $pname" . "_" . "$pversion: $message\n\n";
	}
	return undef;
    }

    if (! rename ("$tmpdest", "$dest")) {
	print " failed\n";
	print "Unable to rename $tmpdest to $dest; not updating";
	return undef;
    }
    print " done\n";
    return $dest;
}
