/*
 * dselect - Debian GNU/Linux package maintenance user interface
 * main.cc - main program
 *
 * Copyright (C) 1994,1995 Ian Jackson <iwj10@cus.cam.ac.uk>
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2,
 * or (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with dpkg; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

#include <curses.h>

extern "C" {
#include <dpkg.h>
#include <dpkg-db.h>
#include <dpkg-var.h>
#include <myopt.h>
}

#include "dselect.h"
#include "bindings.h"
#include "pkglist.h"
#include "config.h"
#include "version.h"

const char *thisname = DSELECT;
const char *const printforhelp = N_("Type dselect --help for help.");

modstatdb_rw readwrite;
const char *admindir = NULL;
FILE *debug;

static keybindings packagelistbindings 
  (packagelist_kinterps, packagelist_korgbindings);

struct menuentry {
  const char *option;
  const char *menuent;
  urqfunction *fn;
};

static const menuentry menuentries[]= {
  { N_("access"), N_("Choose the access method to use."), &urq_setup },
  { N_("update"), N_("Update list of available packages, if possible."), &urq_update },
  { N_("select"), N_("Request which packages you want on your system."), &urq_list },
  { N_("install"), N_("Install and upgrade wanted packages."), &urq_install },
  { N_("config"), N_("Configure any packages that are unconfigured."), &urq_config  },
  { N_("remove"), N_("Remove unwanted software."), &urq_remove  },
  { N_("quit"), N_("Quit dselect."), &urq_quit },
  { N_("menu"), 0, &urq_menu },
  { 0 }
};

static void printversion (void) 
{
  if (fprintf
      (stdout,
       _("Debian GNU/Linux `%s' package handling frontend version %s.\n"
	 "Copyright (C) 1994-1996 Ian Jackson.   This is free software; see the\n"
	 "GNU General Public Licence version 2 or later for copying conditions.\n"
	 "There is NO warranty.  See `%s --licence' for details.\n"),
       DSELECT, DPKG_VERSION_ARCH, DSELECT) < 0) {
    werr ("stdout");
  }
}

static void usage (void) {
  if (fprintf
      (stdout,
       _("Usage: %s [options]\n"
	 "       %s [options] action ...\n"
	 "Options:  --admindir <directory> (default is %s)\n"
	 "          --help  --version  --licence --debug <file> | -D<file> | -D\n"
	 "Actions:  access update select install config remove quit menu\n"),
       DSELECT, DSELECT, ADMINDIR) < 0) {
    werr ("stdout");
  }
}

/* These are called by C code, so need to have C calling convention */

extern "C" {

  static void helponly (const struct cmdinfo*, const char*) {
    usage(); 
    exit (0);
  }

  static void versiononly (const struct cmdinfo*, const char*) {
    printversion(); 
    exit (0);
  }

  static void setdebug (const struct cmdinfo*, const char *v) {
    debug = fopen (v, "a");
    if (!debug) {
      ohshite (_("couldn't open debug file `%.255s'\n"), v);
    }
    setvbuf (debug, 0, _IONBF, 0);
  }

} /* End of extern "C" */

static const struct cmdinfo cmdinfos[]= {
  { "admindir",   0,   1,  0,  &admindir,  0                      },
  { "debug",     'D',  1,  0,  0,          setdebug               },
  { "help",      'h',  0,  0,  0,          helponly               },
  { "version",    0,   0,  0,  0,          versiononly            },
  { "licence",    0,   0,  0,  0,          showcopyright          }, /* UK spelling */
  { "license",    0,   0,  0,  0,          showcopyright          }, /* US spelling */
  {  0,           0                                               }
};

static int cursesareon= 0;
void curseson ()
{
  if (!cursesareon) {
    const char *cup, *smso;
    initscr();
    cup = tigetstr ("cup");
    smso = tigetstr ("smso");
    if ((cup == NULL) || (smso == NULL)) {
      endwin();
      if (cup == NULL) {
        fputs (_("Terminal does not appear to support cursor addressing.\n"), stderr);
      }
      if (smso == NULL) {
        fputs (_("Terminal does not appear to support highlighting.\n"), stderr);
      }
      fprintf (stderr, _("Set your TERM variable correctly, use a better terminal,\n"
			 "or make do with the per-package management tool %s.\n"), DPKG);
      ohshit (_("terminal lacks necessary features, giving up"));
    }
  }
  cursesareon = 1;
}

void cursesoff ()
{
  if (!cursesareon) { 
    return; 
  }
  clear();
  refresh();
  endwin();
  cursesareon = 0;
}

extern void *operator new (size_t size) 
{
  void *p;
  p = m_malloc (size);
  assert (p);
  return p;
}

extern void operator delete (void *p)
{
  free (p);
}

urqresult urq_list (void) 
{
  readwrite = modstatdb_init (admindir, msdbrw_writeifposs);
  curseson ();
  packagelist *l = new packagelist (&packagelistbindings);
  l->resolvesuggest();
  l->display();
  delete l;

  modstatdb_shutdown();
  resetpackages();
  return urqr_normal;
}

void dme (int i, int so) 
{
  char buf[120];
  int y, x;
  const menuentry *me = &menuentries[i];

  sprintf (buf," %c %d. [%c]%-10.10s %-80.80s ",
	   so ? '*' : ' ', i,
	   toupper (me->option[0]), me->option + 1,
	   gettext (me->menuent));
  
  getmaxyx (stdscr, y, x);

  attrset (so ? A_REVERSE : A_NORMAL);
  mvaddnstr (i + 2, 0, buf, x - 1);
  attrset (A_NORMAL);
}

int refreshmenu (void) 
{
  curseson();
  cbreak();
  noecho();
  nonl();
  keypad (stdscr, TRUE);

  int y, x;
  getmaxyx (stdscr, y, x);

  clear();
  attrset (A_BOLD);
  mvaddnstr (0, 0, _("Debian GNU/Linux `%s' package handling frontend version %s.\n"), x - 1);

  attrset(A_NORMAL);
  const struct menuentry *mep; 
  int i;
  for (mep = menuentries, i = 0; mep->option && mep->menuent; mep++, i++)
    dme(i,0);

  attrset(A_BOLD);
  addstr(_("\n\n"
         "Use ^P and ^N, cursor keys, initial letters, or digits to select;\n"
         "Press ENTER to confirm selection.   ^L to redraw screen.\n\n"));

  attrset(A_NORMAL);
  addstr(_("Copyright (C) 1994-1996 Ian Jackson.   This is free software; see the\n"
	   "GNU General Public Licence version 2 or later for copying conditions.\n"
	   "There is NO warranty.  See `dselect --licence' for details.\n"));

  return i;
}

urqresult urq_menu(void) {
#define C(x) ((x)-'a'+1)
  int entries, c, i;
  entries= refreshmenu();
  int cursor=0;
  dme(0,1);
  for (;;) {
    refresh();
    c= getch();  if (c==ERR) ohshite(_("failed to getch in main menu"));
    if (c==C('n') || c==KEY_DOWN || c==' ') {
      dme(cursor,0); cursor++; cursor %= entries; dme(cursor,1);
    } else if (c==C('p') || c==KEY_UP || c==C('h') ||
               c==KEY_BACKSPACE || c==KEY_DC) {
      dme(cursor,0); cursor+= entries-1; cursor %= entries; dme(cursor,1);
    } else if (c=='\n' || c=='\r' || c==KEY_ENTER) {
      clear(); refresh();
      switch (menuentries[cursor].fn()) { /* fixme: trap errors in urq_... */
      case urqr_quitmenu:
        return urqr_quitmenu;
      case urqr_normal:
        cursor++; cursor %= entries;
      case urqr_fail:
        break;
      default:
        internerr("unknown menufn");
      }
      refreshmenu();
      dme (cursor, 1);
    } else if (c == C ('l')) {
      clearok (stdscr, TRUE);
      clear();
      refreshmenu();
      dme (cursor, 1);
    } else if (isdigit (c)) {
      char buf[2];
      buf[0] = c;
      buf[1] = 0;
      c = atoi (buf);
      if (c < entries) {
        dme (cursor, 0);
	cursor = c;
	dme (cursor, 1);
      } else {
        beep();
      }
    } else if (isalpha(c)) {
      c= tolower(c);
      for (i=0; i<entries && menuentries[i].option[0] != c; i++);
      if (i < entries) {
        dme(cursor,0); cursor=i; dme(cursor,1);
      } else {
        beep();
      }
    } else {
      beep();
    }
  }
}

urqresult urq_quit (void) {
  return urqr_quitmenu;
  /* fixme: check packages OK */
}

static void print_error_fatal (const char *emsg, const char *contextstring) 
{
  fprintf (stderr, "%s: %s\n", thisname, emsg);
}

int main (int, const char *const *argv)
{
  jmp_buf ejbuf;

  setlocale (LC_ALL, "");
  bindtextdomain ("dselect", LOCALEDIR);
  textdomain ("dselect");

  admindir = ADMINDIR;

  if (setjmp (ejbuf)) {
    /* expect warning about possible clobbering of argv */
    cursesoff ();
    error_unwind (ehflag_bombout); 
    exit (2);
  }
  push_error_handler (&ejbuf, print_error_fatal, 0);

  myopt (&argv, cmdinfos);

  if (*argv) {
    const char *a;
    while ((a = *argv++) != 0) {
      const menuentry *me;
      for (me = menuentries; me->option && strcmp (me->option, a); me++);
      if (!me->option) {
	badusage (_("unknown action string `%.50s'"), a);
      }
      me->fn();
    }
  } else {
    urq_menu();
  }

  cursesoff();
  set_error_display (0, 0);
  error_unwind (ehflag_normaltidy);
  return (0);
}
