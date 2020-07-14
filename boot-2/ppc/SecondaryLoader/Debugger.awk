func trimStatement() {
	gsub (/[ \t]*\\ .*/, "");
	gsub (/[ \t]*\([ \t]+.*[ \t]+\)[ \t]*/, "");
	gsub (/\"/, "\\\"");
	gsub (/^[ \t]+/, "");
}


BEGIN {
  print "const char *DebuggerSource[] = {";
}

/^$/ {next}

{
  trimStatement();
  if (length () == 0) next;
  printf "\t\"%s \"\n", $0;
}

END {
  print ",0};"
}
