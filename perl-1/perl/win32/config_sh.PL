my %opt;
while (@ARGV && $ARGV[0] =~ /^([\w_]+)=(.*)$/)
 {
  $opt{$1}=$2;
  shift(@ARGV);
 }

$opt{VERSION} = $];
$opt{INST_VER} =~ s|~VERSION~|$]|g;
if ($] =~ /\.(\d\d\d)?(\d\d)?$/) { # should always be true
  $opt{PATCHLEVEL} = int($1 || 0);
  $opt{SUBVERSION} = $2 || '00';
}

$opt{'cf_by'} = $ENV{USERNAME} unless $opt{'cf_by'};
$opt{'cf_email'} = $opt{'cf_by'} . '@' . (gethostbyname('localhost'))[0]
	unless $opt{'cf_email'};
$opt{'usemymalloc'} = 'y' if $opt{'d_mymalloc'} eq 'define';

while (<>)
 {
  s/~([\w_]+)~/$opt{$1}/g;
  if (/^([\w_]+)=(.*)$/) {
    # this depends on cf_time being empty in the template (or we'll get a loop)
    if ($1 eq 'cf_time') {
      $_ = "$1='" . localtime(time) . "'\n" if $2 =~ /^\s*'\s*'/;
    }
    elsif (exists $opt{$1}) {
      $_ = "$1='$opt{$1}'\n";
    }
  }
  print;
 }

