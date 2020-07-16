/*
 * This file was generated by the mkbuiltins program.
 */

#include "shell.h"
#include "builtins.h"

int bltincmd();
int bgcmd();
int breakcmd();
int cdcmd();
int dotcmd();
int echocmd();
int evalcmd();
int execcmd();
int exitcmd();
int expcmd();
int exportcmd();
int falsecmd();
int histcmd();
int fgcmd();
int getoptscmd();
int hashcmd();
int jobidcmd();
int jobscmd();
int localcmd();
int pwdcmd();
int readcmd();
int returncmd();
int setcmd();
int setvarcmd();
int shiftcmd();
int trapcmd();
int truecmd();
int umaskcmd();
int unaliascmd();
int unsetcmd();
int waitcmd();
int aliascmd();
int ulimitcmd();

int (*const builtinfunc[])() = {
	bltincmd,
	bgcmd,
	breakcmd,
	cdcmd,
	dotcmd,
	echocmd,
	evalcmd,
	execcmd,
	exitcmd,
	expcmd,
	exportcmd,
	falsecmd,
	histcmd,
	fgcmd,
	getoptscmd,
	hashcmd,
	jobidcmd,
	jobscmd,
	localcmd,
	pwdcmd,
	readcmd,
	returncmd,
	setcmd,
	setvarcmd,
	shiftcmd,
	trapcmd,
	truecmd,
	umaskcmd,
	unaliascmd,
	unsetcmd,
	waitcmd,
	aliascmd,
	ulimitcmd,
};

const struct builtincmd builtincmd[] = {
	{ "command", 0 },
	{ "bg", 1 },
	{ "break", 2 },
	{ "continue", 2 },
	{ "cd", 3 },
	{ "chdir", 3 },
	{ ".", 4 },
	{ "echo", 5 },
	{ "eval", 6 },
	{ "exec", 7 },
	{ "exit", 8 },
	{ "exp", 9 },
	{ "let", 9 },
	{ "export", 10 },
	{ "readonly", 10 },
	{ "false", 11 },
	{ "-h", 12 },
	{ "fc", 12 },
	{ "fg", 13 },
	{ "getopts", 14 },
	{ "hash", 15 },
	{ "jobid", 16 },
	{ "jobs", 17 },
	{ "local", 18 },
	{ "pwd", 19 },
	{ "read", 20 },
	{ "return", 21 },
	{ "set", 22 },
	{ "setvar", 23 },
	{ "shift", 24 },
	{ "trap", 25 },
	{ ":", 26 },
	{ "true", 26 },
	{ "umask", 27 },
	{ "unalias", 28 },
	{ "unset", 29 },
	{ "wait", 30 },
	{ "alias", 31 },
	{ "ulimit", 32 },
	{ NULL, 0 }
};