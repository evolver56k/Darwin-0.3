
while(<>)
{
  chop;
  s/(\w+)a:.*/$1h /;
  print;
}

# This is in a file, because perl -e requires a tmpfile to run.
# dumb.

