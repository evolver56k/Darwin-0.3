# Awk script to create assym.h

BEGIN {
    printf "#ifndef _ASSYM_H_\n#define _ASSYM_H_\n"
}

/^[ 	]*\.data/ {
    got_data = 1
}

/^[ 	]*_.*:/ {
    if (got_data == 1) {
	printf "#define %s ", substr($1, 3, length($1) - 3);
	got_var = 1;
    }
}
/^[ 	]*\.long/ {
    if (got_var == 1) {
	print $2;
	got_var = 0;
    }
}

# assumption for i386 for KCS_SEL etc.
# XXX change if KCS_SEL etc. ever are greater than 8 bits
/^[ 	]*\.byte/ {
    if (got_var == 1) {
	print $2;
	got_var = 0;
    }
}

/^[ 	]*.text/ {
    if (got_data == 1) exit
}

END {
    printf "#endif /* _ASSYM_H_ */\n"
}
