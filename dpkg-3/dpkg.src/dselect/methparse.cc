/*
 * dselect - Debian GNU/Linux package maintenance user interface
 * methparse.cc - access method list parsing
 *
 * Copyright (C) 1995 Ian Jackson <iwj10@cus.cam.ac.uk>
 * Copyright (C) 1997 Klee Dienes <klee@mit.edu>
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

#include <curses.h>

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

extern "C" {
#include <dpkg.h>
#include <dpkg-db.h>
#include <dpkg-var.h>
}

#include "dselect.h"
#include "bindings.h"
#include "config.h"

int noptions = 0;
struct option *options = NULL;
struct method *methods = NULL;

struct option *coption = NULL;

static char *methoptfile = NULL;

static void badmethod (const char *pathname, const char *why) NONRETURNING;
static void badmethod (const char *pathname, const char *why)
{
  ohshit (_("syntax error in method options file `%.250s' -- %s"), pathname, why);
}

static void eofmethod (const char *pathname, FILE *f, const char *why) NONRETURNING;
static void eofmethod (const char *pathname, FILE *f, const char *why) 
{
  if (ferror (f)) { ohshite (_("error reading options file `%.250s'"),pathname); }
  badmethod (pathname,why);
}

void domethod (const char *pathbase, option **optionspp, int *nread, 
	       char *pathbuf, char *pathmeth, int baselen, struct dirent *dent) 
{
  static const char *const methodprograms[]= {
    METHODSETUPSCRIPT, METHODUPDATESCRIPT, METHODINSTALLSCRIPT, 0
  };

  const char *const *ccpp;
  int methodlen, c;
  char *p, *pathinmeth;
  FILE *names, *descfile;
  struct varbuf vb;
  method *meth;
  option *opt, **optinsert;
  struct stat stab;

  c= dent->d_name[0];
  if (debug) fprintf(debug,"readmethods(`%s',...) considering `%s' ...\n",
		     pathbase,dent->d_name);
  if (c != '_' && !isalpha(c)) return;
  for (p= dent->d_name+1; (c= *p) != 0 && isalnum(c) && c != '_'; p++);
  if (c) return;
  methodlen= strlen(dent->d_name);
  if (methodlen > IMETHODMAXLEN)
    ohshit(_("method `%.250s' has name that is too long (%d > %d characters)"),
	   dent->d_name, methodlen, IMETHODMAXLEN);
  /* Check if there is a localized version of this method */
    
  strcpy(pathmeth, dent->d_name);
  strcpy(pathmeth+methodlen, "/");
  pathinmeth= pathmeth+methodlen+1;

  for (ccpp= methodprograms; *ccpp; ccpp++) {
    strcpy(pathinmeth,*ccpp);
    if (access(pathbuf,R_OK|X_OK))
      /* ohshite("unable to access method script `%.250s'",pathbuf); */
      return;
  }
  if (debug) fprintf(debug," readmethods(`%s',...) scripts ok\n", pathbase);

  strcpy(pathinmeth,METHODOPTIONSFILE);
  names= fopen(pathbuf,"r");
  if (!names) { return; }
  /* was ohshite(_("unable to read method options file `%.250s'"),pathbuf); */

  meth= new method;
  meth->name= new char[strlen(dent->d_name)+1];
  strcpy(meth->name,dent->d_name);
  meth->path= new char[baselen+1+methodlen+2+50];
  strncpy(meth->path,pathbuf,baselen+1+methodlen);
  strcpy(meth->path+baselen+1+methodlen,"/");
  meth->pathinmeth= meth->path+baselen+1+methodlen+1;
  meth->next= methods;
  meth->back= 0;
  if (methods) methods->back= meth;
  methods= meth;
  if (debug) fprintf(debug," readmethods(`%s',...) new method"
		     " name=`%s' path=`%s' pathinmeth=`%s'\n",
		     pathbase, meth->name, meth->path, meth->pathinmeth);
    
  while ((c= fgetc(names)) != EOF) {
    if (isspace(c)) continue;
    opt= new option;
    opt->meth= meth;
    vb.reset();
    do {
      if (!isdigit(c)) badmethod(pathbuf,"non-digit where digit wanted");
      vb(c);
      c= fgetc(names);
      if (c == EOF) eofmethod(pathbuf,names,"EOF in index string");
    } while (!isspace(c));
    if (strlen(vb.string()) > OPTIONINDEXMAXLEN)
      badmethod(pathbuf,"index string too long");
    strcpy(opt->index,vb.string());
    do {
      if (c == '\n') badmethod(pathbuf,"newline before option name start");
      c= fgetc(names);
      if (c == EOF) eofmethod(pathbuf,names,"EOF before option name start");
    } while (isspace(c));
    vb.reset();
    if (!isalpha(c) && c != '_')
      badmethod(pathbuf,"nonalpha where option name start wanted");
    do {
      if (!isalnum(c) && c != '_') badmethod(pathbuf,"non-alphanum in option name");
      vb(c);
      c= fgetc(names);
      if (c == EOF) eofmethod(pathbuf,names,"EOF in option name");
    } while (!isspace(c));
    opt->name= new char[strlen(vb.string())+1];
    strcpy(opt->name,vb.string());
    do {
      if (c == '\n') badmethod(pathbuf,"newline before summary");
      c= fgetc(names);
      if (c == EOF) eofmethod(pathbuf,names,"EOF before summary");
    } while (isspace(c));
    vb.reset();
    do {
      vb(c);
      c= fgetc(names);
      if (c == EOF) eofmethod(pathbuf,names,"EOF in summary - missing newline");
    } while (c != '\n');
    opt->summary= new char[strlen(vb.string())+1];
    strcpy(opt->summary,vb.string());
      
    strcpy(pathinmeth,OPTIONSDESCPFX);
    strcpy(pathinmeth+strlen(OPTIONSDESCPFX),opt->name);
    descfile= fopen(pathbuf,"r");
    if (!descfile) {
      if (errno != ENOENT)
	ohshite(_("unable to open option description file `%.250s'"),pathbuf);
      opt->description= 0;
    } else { /* descfile != 0 */
      if (fstat(fileno(descfile),&stab))
	ohshite(_("unable to stat option description file `%.250s'"),pathbuf);
      opt->description= new char[stab.st_size+1];  errno=0;
      unsigned long filelen= stab.st_size;
      if (fread(opt->description,1,stab.st_size+1,descfile) != filelen)
	ohshite(_("failed to read option description file `%.250s'"),pathbuf);
      opt->description[stab.st_size]= 0;
      if (ferror(descfile))
	ohshite(_("error during read of option description file `%.250s'"),pathbuf);
      fclose(descfile);
    }
    strcpy(pathinmeth,METHODOPTIONSFILE);
      
    if (debug) fprintf(debug," readmethods(`%s',...) new option"
		       " index=`%s' name=`%s' summary=`%.20s'"
		       " strlen(description=%s)=%ld"
		       " method name=`%s' path=`%s' pathinmeth=`%s'\n",
		       pathbase,
		       opt->index, opt->name, opt->summary,
		       opt->description ? "`...'" : "null",
		       opt->description ? (long) strlen(opt->description) : -1,
		       opt->meth->name, opt->meth->path, opt->meth->pathinmeth);
    for (optinsert = optionspp;
	 *optinsert && strcmp(opt->index,(*optinsert)->index) > 0;
	 optinsert = &(*optinsert)->next);
    opt->next= *optinsert;
    *optinsert= opt;
    (*nread)++;
  }
  if (ferror(names))
    ohshite(_("error during read of method options file `%.250s'"),pathbuf);
  fclose(names);
}

void readmethods (const char *pathbase, option **optionspp, int *nread) 
{
  size_t baselen;
  char *pathbuf, *pathmeth;
  DIR *dir;
  struct dirent *dent;

  baselen = strlen (pathbase);
  pathbuf = new char [baselen + IMETHODMAXLEN + IOPTIONMAXLEN + strlen (OPTIONSDESCPFX) + 10];
  strcpy (pathbuf, pathbase);
  strcpy (pathbuf + baselen, "/");
  pathmeth = pathbuf + baselen + 1;

  dir = opendir (pathbuf);
  if (!dir) {
    if (errno == ENOENT) { return; }
    ohshite (_("unable to read `%.250s' directory for reading methods"), pathbuf);
  }
  if (debug) { fprintf (debug, "readmethods (`%s', ...) directory open\n", pathbase); }
  
  while ((dent = readdir (dir)) != 0) {
    domethod (pathbase, optionspp, nread, pathbuf, pathmeth, baselen, dent);
  }

  closedir (dir);
  delete[] pathbuf;

  if (debug) { fprintf (debug,"readmethods (`%s', ...) done\n", pathbase); }
}

/* read the currently selected option from CMETHOPTFILE into coption.
   sets static 'methoptfile'. */

void getcurrentopt (void)
{
  char methstr[IMETHODMAXLEN];
  char optstr[IOPTIONMAXLEN];
  char fmtstr[64];

  FILE *cmo;
  method *meth;
  option *opt;
  
  if (methoptfile == NULL) {
    size_t admindirlen = strlen (admindir);
    methoptfile = new char [admindirlen + strlen (CMETHOPTFILE) + 2];
    strcpy (methoptfile, admindir);
    strcpy (methoptfile + admindirlen, "/");
    strcpy (methoptfile + admindirlen + 1, CMETHOPTFILE);
  }

  coption = 0;

  cmo = fopen (methoptfile, "r");
  if (cmo == NULL) {
    if (errno == ENOENT) { return; }
    ohshite (_("unable to open current option file `%.250s'"), methoptfile);
  }
  if (debug) { fprintf (debug, "getcurrentopt() cmethopt open\n"); }

  snprintf (fmtstr, 64, "%%%ds %%%ds\\n", IMETHODMAXLEN, IOPTIONMAXLEN);
  if (debug) { fprintf (debug, "getcurrentopt() scanning %s\n", fmtstr); }
  if (fscanf (cmo, fmtstr, methstr, optstr) != 2) {
    ohshit (_("unable to parse current option file `%.250s'"), methoptfile);
    fclose (cmo); 
    return;
  }
  fclose (cmo);
  if (debug) { fprintf (debug, "getcurrentopt() cmethopt meth name `%s' `%s'\n", methstr, optstr); }

  for (meth = methods; meth != NULL; meth = meth->next) {
    if (strcmp (meth->name, methstr) == 0) { break; }
  }
  if (meth == NULL) { return; }
  if (debug) { fprintf (debug, "getcurrentopt() cmethopt meth found; opt `%s'\n", meth->name); }

  for (opt = options; opt != NULL; opt = opt->next) {
    if ((opt->meth == meth) && (strcmp (opt->name, optstr) == 0)) { break; }
  }    
  if (opt == NULL) { return; }
  if (debug) { fprintf (debug, "getcurrentopt() cmethopt opt found\n"); }

  coption = opt;
}

/* write the current value of 'coption' to 'methoptfile'.  uses static
   values of 'methoptfile' and 'coption' */

void writecurrentopt (void) 
{
  FILE *cmo;
  static char *newfile = 0;

  /* this should already have been initialized by getcurrentopt */
  assert (methoptfile != NULL);

  if (newfile == NULL) {
    size_t l = strlen (methoptfile);
    newfile = new char[l + strlen (NEWDBEXT) + 1];
    strcpy (newfile, methoptfile);
    strcpy (newfile + l, NEWDBEXT);
  }

  cmo = fopen (newfile, "w");
  if (cmo == NULL) {
    ohshite (_("unable to open new option file `%.250s'"), newfile);
  }
  if (fprintf (cmo, "%s %s\n", coption->meth->name, coption->name) < 0) {
    fclose (cmo);
    ohshite (_("unable to write new option to `%.250s'"), newfile);
  }
  if (fclose (cmo) != 0) {
    ohshite (_("unable to close new option file `%.250s'"), newfile);
  }
  if (rename (newfile, methoptfile)) {
    ohshite (_("unable to install new option file as `%.250s'"), methoptfile);
  }
}
