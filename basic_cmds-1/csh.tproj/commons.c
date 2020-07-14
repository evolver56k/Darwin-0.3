#import "csh.h"
 
FILE *cshin, *cshout, *csherr;
 
bool    chkstop;		/* Warned of stopped jobs... allow exit */
bool    didfds;			/* Have setup i/o fd's for child */
bool    doneinp;		/* EOF indicator after reset from readc */
bool    exiterr;		/* Exit if error or non-zero exit status */
bool    child;			/* Child shell ... errors cause exit */
bool    haderr;			/* Reset was because of an error */
bool    intty;			/* Input is a tty */
bool    intact;			/* We are interactive... therefore prompt */
bool    justpr;			/* Just print because of :p hist mod */
bool    loginsh;		/* We are a loginsh -> .login/.logout */
bool    neednote;		/* Need to pnotify() */
bool    noexec;			/* Don't execute, just syntax check */
bool    pjobs;			/* want to print jobs if interrupted */
bool    setintr;		/* Set interrupts on/off -> Wait intr... */
bool    timflg;			/* Time the next waited for command */
bool    havhash;		/* path hashing is available */
 
Char   *arginp;			/* Argument input for sh -c and internal `xx` */
int     onelflg;		/* 2 -> need line for -t, 1 -> exit on read */
Char   *ffile;			/* Name of shell file for $0 */
 
Char   *shtemp;			/* Temp name for << shell files in /tmp */
 
struct timeval time0;		/* Time at which the shell started */
struct rusage ru0;
 
Char   *doldol;			/* Character pid for $$ */
int	backpid;		/* Pid of the last background process */
int     uid, euid;		/* Invokers uid */
int     gid, egid;		/* Invokers gid */
time_t  chktim;			/* Time mail last checked */
int     shpgrp;			/* Pgrp of shell */
int     tpgrp;			/* Terminal process group */
 
int     opgrp;			/* Initial pgrp and tty pgrp */
 
int   SHIN;			/* Current shell input (script) */
int   SHOUT;			/* Shell output */
int   SHERR;			/* Diagnostic output... shell errs go here */
int   OLDSTD;			/* Old standard input (def for cmds) */
 
jmp_buf reslab;
 
Char   *gointr;			/* Label for an onintr transfer */
 
sig_t parintr;		/* Parents interrupt catch */
sig_t parterm;		/* Parents terminate catch */
 

int     AsciiOnly;	/* If set only 7 bits is expected in characters */
struct Bin B;
 
struct Ain lineloc;
bool    cantell;		/* Is current source tellable ? */
 
Char   *lap;
struct whyle *whyles;
struct varent shvhed, aliases;
 
struct wordent *alhistp;	/* Argument list (first) */
struct wordent *alhistt;	/* Node after last in arg list */
Char  **alvec;	/* The (remnants of) alias vector */
 
int   gflag;		/* After tglob -> is globbing needed? */
 
Char   *pargs;		/* Pointer to start current word */
long    pnleft;		/* Number of chars left in pargs */
Char   *pargcp;		/* Current index into pargs */
 
struct Hist Histlist;
 
struct wordent paraml;	/* Current lexical word list */
int     eventno;		/* Next events number */
int     lastev;		/* Last event reference (default) */
 
Char    HIST;		/* history invocation character */
Char    HISTSUB;		/* auto-substitute character */
 
 
char   *bname;
 
Char   *Vsav;
Char   *Vdp;
Char   *Vexpath;
char  **Vt;
 
Char  **evalvec;
Char   *evalp;
 
Char   *word_chars;
 
Char   *STR_SHELLPATH;
 
#ifdef _PATH_BSHELL
Char   *STR_BSHELL; 
#endif
Char   *STR_WORD_CHARS;
Char  **STR_environ;
