$parsechangelog= 'dpkg-parsechangelog';

grep($capit{lc $_}=$_, qw(Pre-Depends Standards-Version Installed-Size));

$substvar{'Format'}= 1.5;
$substvar{'Newline'}= "\n";
$substvar{'Space'}= " ";
$substvar{'Tab'}= "\t";
$maxsubsts=50;

$progname= $0; $progname= $& if $progname =~ m,[^/]+$,;

sub getfowner {
    if (defined ($ENV{'LOGNAME'})) {
	if (!defined (getlogin ())) { 
	    warn (sprintf ('no utmp entry available, using value of LOGNAME ("%s")', $ENV{'LOGNAME'})); 
	} else {
	    if (getlogin () ne $ENV{'LOGNAME'}) { 
		warn (sprintf ('utmp entry ("%s") does not match value of LOGNAME ("%s"); using "%s"',
			       getlogin(), $ENV{'LOGNAME'}, $ENV{'LOGNAME'}));
	    }
	}
	@fowner = getpwnam ($ENV{'LOGNAME'});
	if (! @fowner) { die (sprintf ('unable to get login information for username "%s"', $ENV{'LOGNAME'})); }
    } elsif (defined (getlogin ())) {
	@fowner = getpwnam (getlogin ());
	if (! @fowner) { die (sprintf ('unable to get login information for username "%s"', getlogin ())); }
    } else {
	warn (sprintf ('no utmp entry available and LOGNAME not defined; using uid of process (%d)', $<));
	@fowner = getpwuid ($<);
	if (! @fowner) { die (sprintf ('unable to get login information for uid %d', $<)); }
    }
    return @fowner[2, 3];
}

sub findconfigname {
    open (UNAMEOUT, "(uname -m; uname -r; uname -s; uname -v)|");
    $unamem = <UNAMEOUT>; chop ($unamem); $unamem =~ tr/[A-Z]/[a-z]/;
    $unamer = <UNAMEOUT>; chop ($unamer); $unamer =~ tr/[A-Z]/[a-z]/;
    $unames = <UNAMEOUT>; chop ($unames); $unames =~ tr/[A-Z]/[a-z]/;
    $unamev = <UNAMEOUT>; chop ($unamev); $unamev =~ tr/[A-Z]/[a-z]/;
    if ($unames eq "rhapsody") {
	return "powerpc-apple-rhapsody";
    } elif ($unames eq "darwin") {
	return "powerpc-apple-darwin";
    } else {
	return "unknown-unknown-unknown";
    }
}

sub findpatchversion {
    open (PATCHVERSION, "patch --version 2> /dev/null|");
    $patchversion = <PATCHVERSION>; chop ($patchversion);
    close (PATCHVERSION);
    if (! ($patchversion =~ s/^patch (\d+)\.(\d+)$/$1.$2/)) {
	&warn ("unable to determine version of patch program");
	&warn ("assuming uses -b <suffix> and not -b -z <suffix>");
	return (('-b', '.dpkg-orig'));
    }
    if ($patchversion > 2.4) {
	return (('-b', '-z', '.dpkg-orig'));
    } else {
	return (('-b', '.dpkg-orig'));
    }
}

sub findfindversion {
    open (FINDVERSION, "find --version 2> /dev/null|");
    $findversion = <FINDVERSION>; chop ($findversion);
    close (FINDVERSION);
    if (! ($findversion =~ s/^GNU find version (\d+)\.(\d+)$/$1.$2/)) {
	&warn ("unable to determine version of find program");
	&warn ("assuming no support for -print0");
	return ("\n", ());
    }
    if ($findversion > 0) { 
	return ("\0", ('-print0'));
    } else {
	return ("\n", ());
    }
}

sub findcpioversion {

    $configname = findconfigname ();
    if ($configname =~ /^powerpc-apple-rhapsody.*/) {
	return ("\n", ());
    }
    if ($configname =~ /^powerpc-apple-darwin.*/) {
	return ("\n", ());
    }

    open (CPIOVERSION, "cpio --version 2> /dev/null|");
    $cpioversion = <CPIOVERSION>; chop ($cpioversion);
    close (CPIOVERSION);
    if (! ($cpioversion =~ s/^GNU cpio version (\d+)\.(\d+).(\d+)$/$1.$2.$3/)) {
	&warn ("unable to determine version of cpio program");
	&warn ("assuming no support for -0");
	return ("\n", ());
    }
    if ($cpioversion > 0) { 
	return ("\0", ('-0'));
    } else {
	return ("\n", ());
    }
}

sub findtarversion {
    open (TARVERSION, "tar --version 2> /dev/null|");
    $tarversion = <TARVERSION>; chop ($tarversion);
    close (TARVERSION);
    if (! ($tarversion =~ s/^tar \(GNU tar\) (\d+)\.(\d+)$/$1.$2/)) {
	&warn ("unable to determine version of tar program");
	&warn ("assuming BSD-style");
	return (53);
    }
    if ($tarversion > 0) { 
	return (48);
    } else {
	return (53);
    }
}

sub capit {
    return defined($capit{lc $_[0]}) ? $capit{lc $_[0]} :
        (uc substr($_[0],0,1)).(lc substr($_[0],1));
}

sub findarch {
    $arch=`dpkg --print-architecture`;
    $? && &subprocerr("dpkg --print-archictecture");
    $arch =~ s/\n$//;
    $substvar{'Arch'}= $arch;
}

sub substvars {
    my ($v) = @_;
    my ($lhs,$vn,$rhs,$count);
    $count=0;
    while ($v =~ m/\$\{([-:0-9a-z]+)\}/i) {
        $count < $maxsubsts ||
            &error("too many substitutions - recursive ? - in \`$v'");
        $lhs=$`; $vn=$1; $rhs=$';
        if (defined($substvar{$vn})) {
            $v= $lhs.$substvar{$vn}.$rhs;
            $count++;
        } else {
            &warn("unknown substitution variable \${$vn}");
            $v= $lhs.$rhs;
        }
    }
    return $v;
}

sub outputclose {
    my ($dosubstvars) = @_;
    for $f (keys %f) { $substvar{"F:$f"}= $f{$f}; }
    if (length($varlistfile)) {
        $varlistfile="./$varlistfile" if $varlistfile =~ m/\s/;
        if (open(SV,"< $varlistfile")) {
            while (<SV>) {
                next if m/^\#/ || !m/\S/;
                s/\s*\n$//;
                m/^(\w[-:0-9A-Za-z]*)\=/ ||
                    &error("bad line in substvars file $varlistfile at line $.");
                $substvar{$1}= $';
            }
            close(SV);
        } elsif ($! !~ m/no such file or directory/i) {
            &error("unable to open substvars file $varlistfile: $!");
        }
    }
    for $f (sort { $fieldimps{$b} <=> $fieldimps{$a} } keys %f) {
        $v= $f{$f};
        if ($dosubstvars) {
	    $v= &substvars($v);
	}
        $v =~ m/\S/ || next; # delete whitespace-only fields
        $v =~ m/\n\S/ && &internerr("field $f has newline then non whitespace >$v<");
        $v =~ m/\n[ \t]*\n/ && &internerr("field $f has blank lines >$v<");
        $v =~ m/\n$/ && &internerr("field $f has trailing newline >$v<");
        $v =~ s/\$\{\}/\$/g;
        print("$f: $v\n") || &syserr("write error on control data");
    }

    close(STDOUT) || &syserr("write error on close control data");
}

sub parsecontrolfile {
    $controlfile="./$controlfile" if $controlfile =~ m/^\s/;

    open(CDATA,"< $controlfile") || &error("cannot read control file $controlfile: $!");
    $indices= &parsecdata('C',1,"control file $controlfile");
    $indices >= 2 || &error("control file must have at least one binary package part");

    for ($i=1;$i<$indices;$i++) {
        defined($fi{"C$i Package"}) ||
            &error("per-package paragraph $i in control info file is ".
                   "missing Package line");
    }
}

sub parsechangelog {
    defined($c=open(CDATA,"-|")) || &syserr("fork for parse changelog");
    if (!$c) {
        @al=($parsechangelog);
        push(@al,"-F$changelogformat") if length($changelogformat);
        push(@al,"-v$since") if length($since);
        push(@al,"-l$changelogfile");
        exec(@al) || &syserr("exec parsechangelog $parsechangelog");
    }
    &parsecdata('L',0,"parsed version of changelog");
    close(CDATA); $? && &subprocerr("parse changelog");
    $substvar{'Source-Version'}= $fi{"L Version"};
}


sub setsourcepackage {
    if (length($sourcepackage)) {
        $v eq $sourcepackage ||
            &error("source package has two conflicting values - $sourcepackage and $v");
    } else {
        $sourcepackage= $v;
    }
}

sub parsecdata {
    local ($source,$many,$whatmsg) = @_;
    # many=0: ordinary control data like output from dpkg-parsechangelog
    # many=1: many paragraphs like in source control file
    # many=-1: single paragraph of control data optionally signed
    local ($index,$cf);
    $index=''; $cf='';
    while (<CDATA>) {
        s/\s*\n$//;
        if (m/^(\S+)\s*:\s*(.*)$/) {
            $cf=$1; $v=$2;
            $cf= &capit($cf);
            $fi{"$source$index $cf"}= $v;
            if (lc $cf eq 'package') { $p2i{"$source $v"}= $index; }
        } elsif (m/^\s+\S/) {
            length($cf) || &syntax("continued value line not in field");
            $fi{"$source$index $cf"}.= "\n$_";
        } elsif (m/^-----BEGIN PGP/ && $many<0) {
            while (<CDATA>) { last if m/^$/; }
            $many= -2;
        } elsif (m/^$/) {
            if ($many>0) {
                $index++; $cf='';
            } elsif ($many == -2) {
                $_= <CDATA>;
                length($_) ||
                    &syntax("expected PGP signature, found EOF after blank line");
                s/\n$//;
                m/^-----BEGIN PGP/ ||
                    &syntax("expected PGP signature, found something else \`$_'");
                $many= -3; last;
            } else {
                &syntax("found several \`paragraphs' where only one expected");
            }
        } else {
            &syntax("line with unknown format (not field-colon-value)");
        }
    }
    $many == -2 && &syntax("found start of PGP body but no signature");
    if (length($cf)) { $index++; }
    $index || &syntax("empty file");
    return $index;
}

sub unknown {
    &warn("unknown information field $_ in input data in $_[0]");
}

sub syntax {
    &error("syntax error in $whatmsg at line $.: $_[0]");
}

sub failure { die "$progname: failure: $_[0]\n"; }
sub syserr { die "$progname: failure: $_[0]: $!\n"; }
sub error { die "$progname: error: $_[0]\n"; }
sub internerr { die "$progname: internal error: $_[0]\n"; }
sub warn { warn "$progname: warning: $_[0]\n"; }
sub usageerr { print(STDERR "$progname: @_\n\n"); &usageversion; exit(2); }

sub subprocerr {
    local ($p) = @_;
    if (WIFEXITED($?)) {
        die "$progname: failure: $p gave error exit status ".WEXITSTATUS($?)."\n";
    } elsif (WIFSIGNALED($?)) {
        die "$progname: failure: $p died from signal ".WTERMSIG($?)."\n";
    } else {
        die "$progname: failure: $p failed with unknown exit code $?\n";
    }
}

1;
