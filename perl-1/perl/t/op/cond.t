#!./perl

# $RCSfile: cond.t,v $$Revision: 1.1.1.1 $$Date: 1999/04/23 01:28:43 $

print "1..4\n";

print 1 ? "ok 1\n" : "not ok 1\n";	# compile time
print 0 ? "not ok 2\n" : "ok 2\n";

$x = 1;
print $x ? "ok 3\n" : "not ok 3\n";	# run time
print !$x ? "not ok 4\n" : "ok 4\n";
