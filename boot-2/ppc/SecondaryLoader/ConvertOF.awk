func trimStatement() {

	gsub (/[[:blank:]]*\\$/, "");					# Remove backslash comments
	gsub (/[[:blank:]]*\\[[:blank:]].*/, "");		# Remove backslash comments

	# Remove parenthesis comments
#	gsub (/[[:blank:]]*\([[:blank:]]+.*[[:blank:]]+\)[[:blank:]]*/, " ");

	gsub (/"/, "\\\"");								# Make sure quotes get thru C
	gsub (/^[[:blank:]]+/, "");						# Remove leading whitespace
	gsub (/[[:blank:]]+$/, "");						# Remove trailing whitespace
}


BEGIN {
  stringSize = 0;
  printf "const char *%sSource[] = {\n", VAR;
}

{
  trimStatement();
  if (length () == 0) next;							# Ignore blank lines after trimming
  stringSize += length ();

#  if (stringSize > 1000) {
#    print ",";
#    stringSize = 0;
#  }

  printf "\t\"%s \"\n", $0;
}

END {
	if (stringSize != 0) print ",";
	print " \" device-end\",0};"
}
