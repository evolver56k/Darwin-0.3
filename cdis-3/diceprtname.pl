
while(<>)
{
  @x = split;
  print "$x[0]\n";
  print "$x[1]\n";
  print "$x[2]\n";
}

# This is in a file, because perl -e requires a tmpfile to run.
# dumb.

