;# pwd.pl - keeps track of current working directory in PWD environment var
;#
;# $RCSfile: pwd.pl,v $$Revision: 1.1.1.1 $$Date: 1999/04/23 01:28:09 $
;#
;# $Log: pwd.pl,v $
;# Revision 1.1.1.1  1999/04/23 01:28:09  wsanchez
;# Import of perl 5.004_04
;#
;# Revision 1.1.1.1  1998/08/12 17:33:05  wsanchez
;# Import of perl 5.004_04
;#
;#
;# Usage:
;#	require "pwd.pl";
;#	&initpwd;
;#	...
;#	&chdir($newdir);

package pwd;

sub main'initpwd {
    if ($ENV{'PWD'}) {
	local($dd,$di) = stat('.');
	local($pd,$pi) = stat($ENV{'PWD'});
	if ($di != $pi || $dd != $pd) {
	    chop($ENV{'PWD'} = `pwd`);
	}
    }
    else {
	chop($ENV{'PWD'} = `pwd`);
    }
    if ($ENV{'PWD'} =~ m|(/[^/]+(/[^/]+/[^/]+))(.*)|) {
	local($pd,$pi) = stat($2);
	local($dd,$di) = stat($1);
	if ($di == $pi && $dd == $pd) {
	    $ENV{'PWD'}="$2$3";
	}
    }
}

sub main'chdir {
    local($newdir) = shift;
    $newdir =~ s|/{2,}|/|g;
    if (chdir $newdir) {
	if ($newdir =~ m#^/#) {
	    $ENV{'PWD'} = $newdir;
	}
	else {
	    local(@curdir) = split(m#/#,$ENV{'PWD'});
	    @curdir = '' unless @curdir;
	    foreach $component (split(m#/#, $newdir)) {
		next if $component eq '.';
		pop(@curdir),next if $component eq '..';
		push(@curdir,$component);
	    }
	    $ENV{'PWD'} = join('/',@curdir) || '/';
	}
    }
    else {
	0;
    }
}

1;
