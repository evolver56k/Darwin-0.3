#!/usr/local/bin/perl --					# -*-Perl-*-
eval "exec /usr/local/bin/perl -S $0 $*"
    if 0;

# Copy a Texinfo file, replacing @value's, @FIXME's and other gooddies.
# Copyright © 1996 Free Software Foundation, Inc.
# François Pinard <pinard@iro.umontreal.ca>, 1996.

$_ = <>;
while ($_)
{
    if (/^\@c()$/ || /^\@c (.*)/ || /^\@(include .*)/)
    {
	if ($topseen)
	{
	    print "\@format\n";
	    print "\@strong{\@\@c} $1\n";
	    $_ = <>;
	    while (/\@c (.*)/)
	    {
		print "\@strong{\@\@c} $1\n";
		$_ = <>;
	    }
	    print "\@end format\n";
	}
	else
	{
	    $delay .= "\@format\n";
	    $delay .= "\@strong{\@\@c} $1\n";
	    $_ = <>;
	    while (/\@c (.*)/)
	    {
		$delay .= "\@strong{\@\@c} $1\n";
		$_ = <>;
	    }
	    $delay .= "\@end format\n";
	}
    }
    elsif (/^\@chapter /)
    {
	print;
#	print $delay;
	$delay = '';
	$topseen = 1;
	$_ = <>;
    }
    elsif (/^\@macro /)
    {
	$_ = <> while ($_ && ! /^\@end macro/);
	$_ = <>;
    }
    elsif (/^\@set ([^ ]+) (.*)/)
    {
	$set{$1} = $2;
	$_ = <>;
    }
    elsif (/^\@UNREVISED/)
    {
	print "\@quotation\n";
	print "\@emph{(This message will disappear, once this node is revised.)}\n";
	print "\@end quotation\n";
	$_ = <>;
    }
    else
    {
	while (/\@value{([^\}]*)}/)
	{
	    if (defined $set{$1})
	    {
		$_ = "$`$set{$1}$'";
	    }
	    else
	    {
		$_ = "$`\@strong{<UNDEFINED>}$1\@strong{</UNDEFINED>}$'";
	    }
	}
	while (/\@FIXME-?([a-z]*)\{/)
	{
	    $tag = $1 eq '' ? 'fixme' : $1;
	    $tag =~ y/a-z/A-Z/;
	    print "$`\@strong{<$tag>}";
	    $_ = $';
	    $level = 1;
	    while ($level > 0)
	    {
		if (/([{}])/)
		{
		    if ($1 eq '{')
		    {
			$level++;
			print "$`\{";
			$_ = $';
		    }
		    elsif ($level > 1)
		    {
			$level--;
			print "$`\}";
			$_ = $';
		    }
		    else
		    {
			$level = 0;
			print "$`\@strong{</$tag>}";
			$_ = $';
		    }
		}
		else
		{
		    print;
		    $_ = <>;
		}
	    }
	}
	print;
	$_ = <>;
    }
}
exit 0;
