/*	$OpenBSD: libyywrap.c,v 1.4 1996/12/10 22:22:03 millert Exp $	*/

/* libyywrap - flex run-time support library "yywrap" function */

/* $Header: /cvs/Darwin/CoreOS/Commands/NeXT/basic_cmds/sh.tproj/libyywrap.c,v 1.1.1.1 1999/04/15 17:45:16 wsanchez Exp $ */

#include <sys/cdefs.h>

int yywrap __P((void));

int
yywrap()
	{
	return 1;
	}
