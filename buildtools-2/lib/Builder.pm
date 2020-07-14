require 5.000;

use strict;
use diagnostics;

use File::Find;

use Dpkg::Package::Package;
use Dpkg::Package::List;

package Dpkg::Package::Builder;

my $debug = 0;

sub copyhash
{
  my ($src) = @_;
  my $ret = { };
  
  for my $key (keys (%$src)) {
    $ret->{$key} = $src->{$key};
  }
  
  return $ret;
}

sub liststring
{
  my $ret = '';
  for my $i (@_) {
    $ret .= " $i";
  }
  return $ret;
}

my @cflags = 
(
 '-Dunix',
 '-D__unix',
 '-D__unix__',
 '-DNX_COMPILER_RELEASE_3_0=300',
 '-DNX_COMPILER_RELEASE_3_1=310',
 '-DNX_COMPILER_RELEASE_3_2=320',
 '-DNX_COMPILER_RELEASE_3_3=330',
 '-DNX_CURRENT_COMPILER_RELEASE=520',
 '-DNS_TARGET=52',
 '-DNS_TARGET_MAJOR=5',
 '-DNS_TARGET_MINOR=2',
 '-DNeXT',
 '-D__NeXT',
 '-D__NeXT__',
 '-D_NEXT_SOURCE'
);

my $baseflags = 
{
 'SRCROOT' => undef,
 'OBJROOT' => undef,
 'SYMROOT' => undef,
 'DSTROOT' => undef,
 'SUBLIBROOTS' => undef,
 'RC_JASPER' => 'YES',
 'RC_ARCHS' => 'ppc',
 'RC_CFLAGS' => '',
 'RC_hppa' => '',
 'RC_i386' => '',
 'RC_m68k' => '',
 'RC_ppc' => '',
 'RC_sparc' => '',
 'RC_KANJI' => '',
 'JAPANESE' => '',
 'RC_OS' => 'teflon',
 'CURRENT_PROJECT_VERSION' => '1',
 'RC_RELEASE' => 'Darwin',
 'NEXT_ROOT' => '',
 'GnuNoInstallSource' => 'YES',
 'Install_Source' => ''
};

sub buildcmd ()
{
    my ($package, $params, $bparams, $target) = @_;

    my $flags = &buildflags ($baseflags, $bparams, $target);
    my @command = ('chroot', $params->{'BUILDROOT'}, 'make', '-w', '-C', $bparams->{'SRCROOT'});

    for my $key (keys (%$flags)) {
	push (@command, "$key=$flags->{$key}");
    }
    push (@command, $target);

    return @command;
}

sub printcmd
{
  for (@_) {
    if (/\s/) {
      print "\"$_\" ";
    } else {
      print "$_ ";
    }
  }
  print "\n";
}

sub buildflags
{
  my ($baseflags, $params, $target) = @_;
  
  my $flags = &copyhash ($baseflags);
  
  $flags->{'SRCROOT'} = $params->{'SRCROOT'};
  $flags->{'OBJROOT'} = $params->{'OBJROOT'};
  $flags->{'SYMROOT'} = $params->{'SYMROOT'};
  $flags->{'SUBLIBROOTS'} = $params->{'SUBLIBROOTS'};
  
  if ($target eq 'installhdrs') {
    $flags->{'DSTROOT'} = $params->{'HDRROOT'};
  } else {
    $flags->{'DSTROOT'} = $params->{'DSTROOT'};
  }

  if ($target eq 'installhdrs') {
    $flags->{'RC_CFLAGS'} = '-arch i386 -arch ppc ' . &liststring (@cflags);
    $flags->{'RC_ARCHS'} = 'i386 ppc';
    $flags->{'RC_i386'} = 'YES';  
    $flags->{'RC_ppc'} = 'YES';
  } else {
    $flags->{'RC_CFLAGS'} = '-arch ppc ' . &liststring (@cflags);
    $flags->{'RC_ARCHS'} = 'ppc';
    $flags->{'RC_i386'} = '';
    $flags->{'RC_ppc'} = 'YES';
  }

  return $flags;
}

my $cwd;

BEGIN {
  $cwd = `pwd`;
  chomp ($cwd);
}

sub canonparams ()
{
  my ($params) = @_;

  my @headers = ('CVSSRCDIR', 'SRCDIR', 'PACKAGEDIR', 'BUILDROOT',
		 'SRCROOT', 'OBJROOT', 'SYMROOT', 'DSTROOT', 'HDRROOT', 'LIBCOBJROOT', 'PACKAGEROOT',
		 'LOGFILE', 'SUBLIBROOTS');

  for my $key (@headers) {
      if (defined ($params->{$key}) && (! ($params->{$key} =~ /^\//))) {
	  $params->{$key} = $cwd . '/' . $params->{$key};
      }
  }

  return $params;
}

sub chrootparams ()
{
    my ($params, $buildroot) = @_;

    $buildroot =~ s,/*$,,;

    my $nparams = { };

    $nparams->{'SRCROOT'} = $buildroot . $params->{'SRCROOT'};
    $nparams->{'OBJROOT'} = $buildroot . $params->{'OBJROOT'};
    $nparams->{'SYMROOT'} = $buildroot . $params->{'SYMROOT'};
    $nparams->{'DSTROOT'} = $buildroot . $params->{'DSTROOT'};
    $nparams->{'HDRROOT'} = $buildroot . $params->{'HDRROOT'};
    $nparams->{'LIBCOBJROOT'} = $buildroot . $params->{'LIBCOBJROOT'};
    $nparams->{'LOGFILE'} = $buildroot . $params->{'LOGFILE'};
    $nparams->{'SUBLIBROOTS'} = $buildroot . $params->{'SUBLIBROOTS'};
    $nparams->{'PACKAGEROOT'} = $buildroot . $params->{'PACKAGEROOT'};
    $nparams->{'BUILDROOT'} = $buildroot;

    return $nparams;
}

sub getparams ()
{
  my ($package, $projectname) = @_;
  my $params = { };
  
  my $buildroot = '/private/tmp/roots';
  if (defined ($ENV{'BUILDIT_DIR'} && $ENV{'BUILDIT_DIR'})) {
    $buildroot = $ENV{'BUILDIT_DIR'};
  }
 
  $params->{'BUILDROOT'} = "$buildroot/$projectname.roots/$projectname.root";
  if (defined ($ENV{'BUILDROOT'} && $ENV{'BUILDROOT'})) {
    $params->{'BUILDROOT'} = $ENV{'BUILDROOT'};
  }

  $params->{'SRCROOT'} = "$buildroot/$projectname.roots/$projectname";
  if (defined ($ENV{'SRCROOT'} && $ENV{'SRCROOT'})) {
    $params->{'SRCROOT'} = $ENV{'SRCROOT'};
  }
  
  $params->{'OBJROOT'} = "$buildroot/$projectname.roots/$projectname.obj";
  if (defined ($ENV{'OBJROOT'} && $ENV{'OBJROOT'})) {
    $params->{'OBJROOT'} = $ENV{'OBJROOT'};
  }
  
  $params->{'SYMROOT'} = "$buildroot/$projectname.roots/$projectname.sym";
  if (defined ($ENV{'SYMROOT'} && $ENV{'SYMROOT'})) {
    $params->{'SYMROOT'} = $ENV{'SYMROOT'};
  }
  
  $params->{'DSTROOT'} = "$buildroot/$projectname.roots/$projectname.dst";
  if (defined ($ENV{'DSTROOT'} && $ENV{'DSTROOT'})) {
    $params->{'DSTROOT'} = $ENV{'DSTROOT'};
  }
  
  $params->{'HDRROOT'} = "$buildroot/$projectname.roots/$projectname.hdr";
  if (defined ($ENV{'HDRROOT'} && $ENV{'HDRROOT'})) {
    $params->{'HDRROOT'} = $ENV{'HDRROOT'};
  }
  
  $params->{'LIBCOBJROOT'} = "$buildroot/$projectname.roots/$projectname.cobj";
  if (defined ($ENV{'LIBCOBJROOT'} && $ENV{'LIBCOBJROOT'})) {
    $params->{'LIBCOBJROOT'} = $ENV{'LIBCOBJROOT'};
  }
  
  $params->{'LOGFILE'} = "$buildroot/$projectname.roots/$projectname.log";
  if (defined ($ENV{'LOGFILE'} && $ENV{'LOGFILE'})) {
    $params->{'LOGFILE'} = $ENV{'LOGFILE'};
  }
  
  $params->{'SUBLIBROOTS'} = "/usr/local/lib/objs";
  if (defined ($ENV{'SUBLIBROOTS'} && $ENV{'SUBLIBROOTS'})) {
    $params->{'SUBLIBROOTS'} = $ENV{'SUBLIBROOTS'};
  }

  $params->{'PACKAGEROOT'} = "$buildroot/$projectname.roots/$projectname.pkg";
  if (defined ($ENV{'PACKAGEROOT'} && $ENV{'PACKAGEROOT'})) {
    $params->{'PACKAGEROOT'} = $ENV{'PACKAGEROOT'};
  }

  return $params;
}

sub pkgname
{
  my ($pname, $revision) = @_;

  $pname =~ s/_/-/g;
  if ($pname eq "appkit") {
    $pname = "appkit-old";
  }
  if ($pname eq "ssh") {
      if ($revision =~ /^1/) {
	  $pname = "ssh1";
      } else {
	  $pname = "ssh2";
      }
  }
  $pname =~ tr/[A-Z]/[a-z]/;

  return $pname;
}

sub dir2name
{
  my ($pbase) = @_;

  $pbase =~ s,/*$,,;
  $pbase =~ s,.*/,,;
  my $revision = $pbase;

  $revision =~ s,.*/,,;
  if ($revision =~ /^[^-]*(-([0-9.]+))?/) {
      $revision = $2;
  }
  
  $pbase =~ s/-[0-9.]+$//;
  my $pname = &pkgname ($pbase, $revision);
  
  return ($pbase, $pname, $revision);
}

sub makecontrol
{
  my ($pname, $revision) = @_;
  
  my $package = Dpkg::Package::Package->new ('data' => '');
  
  $package->{'package'} = $pname;
  $package->{'version'} = "0";
  $package->{'architecture'} = 'powerpc-apple-darwin';
  $package->{'source'} = $package->{'package'};
  $package->{'description'} = "No description available.";
  $package->{'maintainer'} = "Anonymous <darwin-development\@public.lists.apple.com>";
  $package->{'build-depends'} = ['build-base'];
  
  return $package;
}

sub readcontrol
{
  my ($file) = @_;
  my $package = undef;

  $/ = "";
  my $data = <$file>;
  $/ = "\n";

  if (! defined ($data)) {
    return undef;
  }

  $package = Dpkg::Package::Package->new ('data' => $data);

  if (! defined ($package)) {
    return $package;
  }
  
  if (! defined ($package->{'package'})) {
    print "error: package file does not contain 'Package:' entry\n";
    return undef;
  }
  if (! defined ($package->{'version'})) {
    print "error: package file does not contain 'Version:' entry\n";
    return undef;
  }
  if (! defined ($package->{'description'})) {
    $package->{'description'} = "No description available.";
  }
  if (! defined ($package->{'maintainer'})) {
      $package->{'maintainer'} = "Anonymous <darwin-development\@public.lists.apple.com>";
  }
  
  $package->{'architecture'} = 'powerpc-apple-darwin';
  $package->{'source'} = $package->{'package'};
  
  return $package;
}

sub checkret ()
{
  my ($status) = @_;
  my $ret = "";
  
  if ($status == 0) {
    return "exited successfully";
  }
  
  my $lowbyte = ($status & 0xff);
  if ($lowbyte == 0x7f) {
    return "stopped";
  }
  
  my $signal = ($lowbyte & 0177);
  if ($signal != 0) {
    $ret = "terminated by signal $signal";
    if ($lowbyte & 0200) {
      $ret .= " (core dumped)";
    }
    return $ret;
  }
  
  my $exitstatus = (($status >> 8) & 0xff);
  return "failed with status $exitstatus";
}

sub buildpackage ()
{
  my ($spackage, $params, $target) = @_;
  my $dstroot;
  
  my $package = Dpkg::Package::Package->new ('data' => $spackage->unparse ());
  my $pname = $package->{'package'};
  
  if ($target eq 'binary') {
    
    $dstroot = $params->{'DSTROOT'};
    $package->{'package'} = $pname;
    $package->{'provides'} = $pname . '-hdrs';
    $package->{'conflicts'} = $pname . '-hdrs';
    $package->{'replaces'} = $pname . '-hdrs';
    
  } elsif ($target eq 'headers') {
    
    $dstroot = $params->{'HDRROOT'};
    $package->{'package'} = $pname . '-hdrs';
    
  } elsif ($target eq 'objects') {
    
    $dstroot = $params->{'LIBCOBJROOT'};
    $package->{'package'} = $pname . '-obj';
        
  } elsif ($target eq 'local') {

    return;

  } else {
    die ("bad target: \"$target\"");
  }
  
  my $debdir = "$dstroot/DEBIAN";
  
  # ensure that the base package gets created
  if ($target eq 'binary') {
    system ('mkdir', '-p', $debdir);
    if ($? != 0) { die (&checkret ($?)); }
  }
  
  if (! opendir (DESTDIR, $dstroot)) {
    # no files in package
    return;
  }
  my @entries = readdir (DESTDIR);
  closedir (DESTDIR) || die ("unable to close directory");
  if ($#entries == 1) {
    # no files in package
    return;
  }
  
  system ('rm', '-rf', "$dstroot/System/Developer/Source");
  if ($? != 0) { die (&checkret ($?)); }
  
  system ('mkdir', '-p', $debdir);
  if ($? != 0) { die (&checkret ($?)); }
  
  open (CONTROL, ">$debdir/control")
    || die ("unable to open $debdir/control for writing\n");
  $package->write (*CONTROL{IO});
  close (CONTROL);
  
  if ($target eq 'binary') {
    for my $ename ('conffiles', 'preinst', 'postinst', 'prerm', 'postrm') {
      my $extra = "$params->{'SRCDIR'}/dpkg/$ename";
      if (-f $extra) {
	print "copying $ename\n";
	system ('cp', '-p', $extra, "$debdir/$ename");
	if ($? != 0) { die (&checkret ($?)); }
	if ($ename eq 'conffiles') {
	    system ('chmod', '644', $extra, "$debdir/$ename");
	    if ($? != 0) { die (&checkret ($?)); }
	} else {
	    system ('chmod', '755', $extra, "$debdir/$ename");
	    if ($? != 0) { die (&checkret ($?)); }
	}
      }
    }
  }

  system ('dpkg-deb', '--build', $dstroot, $params->{'PACKAGEDIR'});
  if ($? != 0) { die (&checkret ($?)); }
}

sub scandir ()
{
  my ($srcname) = @_;

  my $srcdir = $srcname;
  my ($pbase, $pname, $revision) = &dir2name ($srcname);

  my $package = undef;
  my $params = undef;

  if (open (CONTROL, "<$srcdir/dpkg/control")) {
    $package = &readcontrol (*CONTROL{IO});
    close (CONTROL);
  }
  if (! defined ($package)) {
    if ($debug) { warn ("unable to read control file for \"$srcdir\"; generating default"); }
    $package = &makecontrol ($pname, $revision);
  }
  if (! defined ($package)) {
    die ("unable to determine package version information");
  }

  $package->{'source'} = $pbase;
  if (defined ($revision)) {
      $package->{'version'} = $package->{'version'} . '-' . $revision;
  } else {
      $package->{'version'} = $package->{'version'}
  }

  $params = &getparams ($package, $package->{'package'} . '-' . $package->{'version'});
  return ($package, $params);
}

sub scancvs ()
{
  my ($srcname) = @_;
  
  my ($pbase, $pname, $revision) = &dir2name ($srcname);

  my $package = undef;
  my $params = undef;

  my $cvstag;
  
  if (defined ($revision)) {
      $cvstag = $pbase . '-' . $revision;
      $cvstag =~ s/\./-/og;
  }

  my $cvsarg = "";
  if (defined ($cvstag)) {
      $cvsarg = "-r $cvstag";
  }

  my $cvscommand = "cvs checkout $cvsarg -p $pbase/dpkg/control 2>/dev/null";
  if ($debug) { print "$cvscommand\n"; }
  if (open (CONTROL, "$cvscommand |")) {
    $package = &readcontrol (*CONTROL{IO});
    close (CONTROL);
  }
  
  if (! defined ($package)) {
    die ("unable to read $pbase/dpkg/control via cvs $cvsarg");
  }

  if (! defined ($package)) {
      if (defined ($cvstag)) {
	  if ($debug) { warn ("unable to read control file for \"$cvstag\"; generating default"); }
      } else {
	  if ($debug) { warn ("unable to read control file for \"$pname\"; generating default"); }
      }
    $package = &makecontrol ($pname, $revision);
  }
  if (! defined ($package)) {
    die ("unable to determine package version information");
  }

  $package->{'source'} = $pbase;
  if (defined ($revision)) {
      $package->{'version'} = $package->{'version'} . '-' . $revision;
  } else {
      $package->{'version'} = $package->{'version'}
  }

  if (defined ($cvstag)) {
      $params = &getparams ($package, $package->{'package'} . '-' . $package->{'version'});
  } else {
      my ($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) = gmtime;
      $params = &getparams ($package, $package->{'package'} . '.cvs');
      #$package->{'version'} = 'cvs';
      $package->{'version'} = sprintf ("cvs.%d%02d%02d.%02d%02d", ($year + 1900), ($mon + 1), $mday, $hour, $min);
  }

  return ($package, $params);
}

sub getcvs ()
{
  my ($package, $params, $srcname) = @_;

  my ($pbase, $pname, $revision) = &dir2name ($srcname);
  
  my $cvstag;
  
  if (defined ($revision)) {
      $cvstag = $pbase . '-' . $revision;
      $cvstag =~ s/\./-/og;
  }

  my $srcroot = $params->{'SRCROOT'};

  if (-e $srcroot) {

      print "cvs directory \"$srcroot\" already exists; reusing\n";

      $cwd = `pwd`;
      chomp ($cwd);

      if (! chdir ($srcroot)) {
	  die ("unable to chdir (\"$srcroot\"): $!");
      }

      my @cvscommand = ('cvs', 'update', '.');

      &printcmd (@cvscommand);
      system (@cvscommand);
      if ($? != 0) { die (&checkret ($?)); }

      chdir ($cwd) || die ("unable to chdir (\"$cwd\")");

  } else {

      system ('mkdir', '-p', $srcroot);
      if ($? != 0) { die (&checkret ($?)); }

      system ('rmdir', $srcroot);
      if ($? != 0) { die (&checkret ($?)); }

      my $srcbase;
      my $srcdir;
      if ($srcroot =~ /(.+)\/([^\/]+)/) {
	  $srcbase = $1;
	  $srcdir = $2;
      } else {
	  die "invalid srcroot: $srcroot\n";
      }

      $cwd = `pwd`;
      chomp ($cwd);

      if (! chdir ($srcbase)) {
	  die ("unable to chdir (\"$srcbase\"): $!");
      }

      my @cvscommand;
      if (defined ($cvstag)) {
	  @cvscommand = ('cvs', 'checkout', '-r', $cvstag, '-d', $srcdir, $pbase);
      } else {
	  @cvscommand = ('cvs', 'checkout', '-d', $srcdir, $pbase);
      }

      &printcmd (@cvscommand);
      system (@cvscommand);
      if ($? != 0) { die (&checkret ($?)); }

      chdir ($cwd) || die ("unable to chdir (\"$cwd\")");
  }
}

my @basedeps = 
  (
   # Tools
   'cc', 'cctools', #'objcunique',
   'file-cmds',
   'text-cmds', 'awk', 'grep',
   'shell-cmds',
   'developer-cmds',

   # Makefiles
   'pb-makefiles', 'coreosmakefiles',

   # Libraries
   'libsystem',

   # System
   'files',

   # Junk we should get rid of
   'csu',
   'libc-hdrs', 'gnumake',
   'basic-cmds',
   'architecture-hdrs', 'kernel-hdrs', 'gnutar',
   'project-makefiles', 'bootstrap-cmds', 'objc4-hdrs',
   'system-cmds',
  );

sub makeroot ()
{
    my ($package, $buildroot, $repository) = @_;
    
    my $deps = $package->{'build-depends'};
    if (! defined ($deps)) {
      $deps = \@basedeps;
    }

    my @ndeps = ();
    my %depmap = ();

    print "Building build root:\n";

    for my $dep (@$deps) {
      if ($dep eq 'build-base') {
	for my $tdep (@basedeps) {
	  $depmap{$tdep} = '';
	}
      } else {
	$depmap{$dep} = '';
      }
    }
    
    @ndeps = keys (%depmap);

    my %depfiles = ();

    for my $dep (@ndeps) {
	my $debfile = &resolve_dependency ($dep, $repository);
	if (! defined ($debfile)) {
	  die ("unable to find dependency for \"$dep\"");
	}
	my $debname = $debfile;
	$debname =~ s|^.*/||;
	$debname =~ s|\.deb$||;
	$depfiles{$debname} = $debfile;
    }

    my %curdeps = ();
    my $depfile = "$buildroot/var/adm/package-list";

    if (-f $depfile) {
	open (DEPFILE, "<$depfile") || die ("unable to open $depfile: $!");
	while (<DEPFILE>) {
	    chomp;
	    $curdeps{$_} = "";
	} 
	close (DEPFILE);
    }

    for my $debname (keys (%depfiles)) {

	my $debfile = $depfiles{$debname};

	if (defined ($curdeps{$debname})) {
	    print "\talready have $debfile\n";
	} else {
	    print "\tinstalling $debfile\n";

	    my @command = ('dpkg-deb', '-x', $debfile, $buildroot);
	    system (@command);
	    if ($? != 0) { die (&checkret ($?)); }
	}
    }

    system ('mkdir', '-p', "$buildroot/var/adm");
    if ($? != 0) { die (&checkret ($?)); }

    my $pfilename = "$buildroot/var/adm/package-list";
    open LIST, ">$pfilename" || die ("unable to open $pfilename: $!");
    for my $debname (keys (%depfiles)) {
	print LIST "$debname\n";
    }
    close (LIST) || die ("unable to close $pfilename: $!");
}

sub scan ()
{
    my ($type, $source, $targets) = @_;

    if ($type eq 'cvs') {
	return &scancvs ($source);
    } elsif ($type eq 'dir') {
	return &scandir ($source);
    } else {
	die ("invalid source type \"$source\"");
    }
}

sub setupdirs ()
{
    my ($package, $params, $srcname, $srctype, $repository) = @_;

    system ('mkdir', '-p', $params->{'OBJROOT'});
    if ($? != 0) { die (&checkret ($?)); }
    system ('mkdir', '-p', $params->{'SYMROOT'});
    if ($? != 0) { die (&checkret ($?)); }
    system ('mkdir', '-p', $params->{'DSTROOT'});
    if ($? != 0) { die (&checkret ($?)); }
    system ('mkdir', '-p', $params->{'HDRROOT'});
    if ($? != 0) { die (&checkret ($?)); }
    system ('mkdir', '-p', $params->{'PACKAGEROOT'});
    if ($? != 0) { die (&checkret ($?)); }
    system ('mkdir', '-p', $params->{'BUILDROOT'});
    if ($? != 0) { die (&checkret ($?)); }

    my $buildroot = $params->{'BUILDROOT'};
    &makeroot ($package, $buildroot, $repository);

    if ($srctype eq 'dir') {

	my $srcdir = $params->{'SRCDIR'};
	my $srcroot = $params->{'SRCROOT'};

	my @command = ('mkdir', '-p', $srcroot);
	&printcmd (@command);
	system (@command);
	if ($? != 0) { die (&checkret ($?)); }

	@command = ('sh', '-c', "(cd $srcdir && rsync -avr . $srcroot)");
	&printcmd (@command);
	system (@command);
	if ($? != 0) { die (&checkret ($?)); }

    } elsif ($srctype eq 'cvs') {
	&getcvs ($package, $params, $srcname);
    } else {
	die ("unknown source type $srctype");
    }
}

my @objs = ();

sub findobjs {
  if ($_ eq 'dynamic_obj') {
    unshift (@objs, "$File::Find::dir/$_");
    $File::Find::prune = 1;
  }
}

sub build () 
{
  my ($srctype, $srcname, $repository, $target, $dstdir, $clean) = @_;
  my ($package, $params);

  ($package, $params) = &scan ($srctype, $srcname);

  my $hdrpackage = Dpkg::Package::Package->new ('data' => $package->unparse ());
  $hdrpackage->{'package'} = $hdrpackage->{'package'} . '-hdrs';
  my $hdrfilename = $hdrpackage->canon_name();

  my $filename = $package->canon_name() ;

  if (($target eq 'headers') && (-e "$dstdir/$hdrfilename.deb")) {
      print "package file for \"$hdrfilename\" already exists; not building\n";
      return 0;
  }
  if (-e "$dstdir/$filename.deb") {
      print "package file for \"$filename\" already exists; not building\n";     
      return 0;    
  }    

  my $bparams = $params;
  $params = &chrootparams ($params, $params->{'BUILDROOT'});

  if ($srctype eq 'cvs') {
      $params->{'SRCDIR'} = $params->{'SRCROOT'};
  } elsif ($srctype eq 'dir') {
      $params->{'SRCDIR'} = $srcname;
  } else {
      die ("invalid source type \"$srctype\"");
  }

  $params->{'PACKAGEDIR'} = $dstdir;

  $params = &canonparams ($params);
  $bparams = &canonparams ($bparams);

  print "building $filename from $params->{'SRCDIR'}:\n\n";

  print "building package:\n\n";
  print $package->unparse();
  print "\n";

  print "using parameters:\n\n";
  for my $key (keys (%$params)) {
    print "$key: $params->{$key}\n";
  }
  print "\n";

  print "using build parameters:\n\n";
  for my $key (keys (%$bparams)) {
    print "$key: $bparams->{$key}\n";
  }
  print "\n";

  &setupdirs ($package, $params, $srcname, $srctype, $repository);

  if (($target eq 'all') || ($target eq 'headers')) {
    
    my @installhdrscommand = &buildcmd ($package, $params, $bparams, 'installhdrs');
    print 'UNAME_SYSNAME=Rhapsody PATH=/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin';
    &printcmd (@installhdrscommand);
    $ENV{'UNAME_SYSNAME'} = 'Rhapsody';
    $ENV{'PATH'} = '/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin';
    system (@installhdrscommand);
    if ($? != 0) { die (&checkret ($?)); }
    print "\n";
  }
  
  if (($target eq 'all') || ($target eq 'binary')) {
    
    my @installcommand = &buildcmd ($package, $params, $bparams, 'install');
    print 'UNAME_SYSNAME=Rhapsody /sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin';
    &printcmd (@installcommand);
    $ENV{'UNAME_SYSNAME'} = 'Rhapsody';
    $ENV{'PATH'} = '/sbin:/usr/sbin:/bin:/usr/bin:/usr/local/bin';
    system (@installcommand);
    if ($? != 0) { die (&checkret ($?)); }
    print "\n";
    
    $cwd = `pwd`;
    chomp ($cwd);
    
    @objs = ();
    print "finding files in $params->{'OBJROOT'}\n";
    if (! chdir ($params->{'OBJROOT'})) {
      die ("unable to chdir (\"$params->{'OBJROOT'}\"): $!");
    }
    &File::Find::find (\&findobjs, '.');
    
    for my $file (@objs) {
      print "copying files from $file\n";
      my $objdest = "/usr/local/lib/objs/$package->{'source'}/$file";
      system ('mkdir', '-p', "$params->{'LIBCOBJROOT'}/$objdest");
      if ($? != 0) { die (&checkret ($?)); }
      system ('rmdir', "$params->{'LIBCOBJROOT'}/$objdest");
      if ($? != 0) { die (&checkret ($?)); }
      my @command = ('chroot', $params->{'BUILDROOT'},
		  'cp', '-rp',
		  "$bparams->{'OBJROOT'}/$file",
		  "$bparams->{'LIBCOBJROOT'}/$objdest");
      &printcmd (@command);
      system (@command);
      if ($? != 0) { die (&checkret ($?)); }
    }

    chdir ($cwd) || die ("unable to chdir (\"$cwd\")");

    print "\n";
  }
  
  if (($target eq 'all') || ($target eq 'headers')) {
    &buildpackage ($package, $params, 'headers');
  }

  if (($target eq 'all') || ($target eq 'binary')) {
    &buildpackage ($package, $params, 'binary');
    &buildpackage ($package, $params, 'objects');
    &buildpackage ($package, $params, 'local');
  }

  if ($clean) {
    my @command = ('rm', '-rf', $params->{'BUILDROOT'});
    &printcmd (@command);
    system (@command);
    if ($? != 0) { die (&checkret ($?)); }
  }
  
  return 0;
}

sub resolve_dependency ()
{
  my ($pname, $repository) = @_;
  
  for my $dir (@$repository) {
    
    opendir (DIR, $dir) || die ("unable to open directory \"$dir\": $!");
    my @entries = readdir (DIR);
    for (@entries) {
      my $pattern = "^${pname}_.*\\.deb\$";
      # print "checking $_ for $pattern\n";
      next if (($_ eq '.') || ($_ eq '..'));
      if (/$pattern/) {
	return "$dir/$_";
      }
    }
  }

  return undef;
}

sub exists ()
{
    my ($package, $type, $dir) = @_;

    if ($type eq 'any') {
	my $wildcard = "$dir/$package->{'package'}_*.deb";
	my $filename = `echo -n $wildcard`;
	if (! ($filename =~ /\*/)) {
	    return $filename;
	}
    } elsif ($type eq 'exact') {
	my $filename = "$dir/" . $package->canon_name() . ".deb";
	if (-e $filename) {
	    return $filename;
	}
    } else {
      die ("invalid match type \"$type\"");
    }

    return undef;
}

1;
