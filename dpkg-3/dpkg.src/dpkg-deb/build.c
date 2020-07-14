/*
 * dpkg-deb - construction and deconstruction of *.deb archives
 * build.c - building archives
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
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>
#include <ctype.h>
#include <assert.h>

#include <dpkg.h>
#include <dpkg-db.h>
#include <dpkg-var.h>

#include "dpkg-deb.h"
#include "config.h"
#include "version.h"

static void checkversion (const char *vstring, const char *valuename, int *errs) 
{
  const char *p;
  if (!vstring || !*vstring) return;
  for (p = vstring; *p != '\0'; p++) {
    if (isdigit (*p)) return;
  }
  /* supermsg: checkversion 1 */
  fprintf (stderr, _("dpkg-deb - error: %s (`%s') doesn't contain any digits\n"),
	   valuename, vstring);
  (*errs)++;
}

void do_build (const char *const *argv) 
{
  const char *const maintainerscripts[]= {
    PREINSTFILE, POSTINSTFILE, PRERMFILE, POSTRMFILE, 0
  };
  
  char *m;
  const char *debar, *directory, *const *mscriptp, *versionstring, *arch;
  char *controlfile;
  struct pkginfo *checkedinfo;
  struct arbitraryfield *field;
  FILE *ar, *gz, *cf;
  int p1[2],p2[2], warns, errs, n, c, subdir;
  pid_t c1,c2,c3,c4,c5;
  struct stat controlstab, datastab, mscriptstab, debarstab;
  char conffilename[MAXCONFFILENAME+1];
  time_t thetime= 0;
  
  directory= *argv++; if (!directory) badusage(_("--build needs a directory argument"));
  subdir= 0;
  if ((debar= *argv++) !=0) {
    if (*argv) badusage(_("--build takes at most two arguments"));
    if (debar) {
      if (stat(debar,&debarstab)) {
        if (errno != ENOENT)
          ohshite(_("unable to check for existence of archive `%.250s'"),debar);
      } else if (S_ISDIR(debarstab.st_mode)) {
        subdir= 1;
      }
    }
  } else {
    m = m_malloc (strlen (directory) + strlen (DEBEXT) + 1);
    strcpy (m, directory);
    strcat (m, DEBEXT);
    debar = m;
  }
    
  if (nocheckflag) {
    if (subdir)
      ohshit(_("target is directory - cannot skip control file check"));
    printf(_("dpkg-deb: warning, not checking contents of control area.\n"
           "dpkg-deb: building an unknown package in `%s'.\n"), debar);
  } else {
    controlfile = m_malloc (strlen (directory) + strlen (BUILDCONTROLDIR) +
			    strlen (CONTROLFILE) + strlen (CONFFILESFILE) +
			    strlen (POSTINSTFILE) + strlen (PREINSTFILE) +
			    strlen (POSTRMFILE) + strlen (PRERMFILE) +
			    MAXCONFFILENAME + 20);
    sprintf (controlfile, "%s/%s/%s", directory, BUILDCONTROLDIR, CONTROLFILE);
    warns = 0; errs = 0;
    parsedb(controlfile, pdb_recordavailable|pdb_rejectstatus,
            &checkedinfo, stderr, &warns);
    assert(checkedinfo->available.valid);
    if (strspn(checkedinfo->name,
               "abcdefghijklmnopqrstuvwxyz0123456789+-.")
        != strlen(checkedinfo->name))
      ohshit(_("package name has characters that aren't lowercase alphanums or `-+.'"));
    if (checkedinfo->priority == pri_other) {
      fprintf(stderr, _("warning, `%s' contains user-defined Priority value `%s'\n"),
              controlfile, checkedinfo->otherpriority);
      warns++;
    }
    for (field= checkedinfo->available.arbs; field; field= field->next) {
      fprintf(stderr, _("warning, `%s' contains user-defined field `%s'\n"),
              controlfile, field->name);
      warns++;
    }
    /* submsg: checkversion */
    checkversion(checkedinfo->available.version.version,_("(upstream) version"),&errs);
    /* submsg: checkversion */
    checkversion(checkedinfo->available.version.revision,_("Debian revision"),&errs);
    if (errs) ohshit(_("%d errors in control file"),errs);

    if (subdir) {
      versionstring= versiondescribe(&checkedinfo->available.version,vdew_never);
      arch= checkedinfo->available.architecture; if (!arch) arch= "";
      m = m_malloc (strlen (debar) + 1 +
		    strlen (checkedinfo->name) + 1 +
		    strlen (versionstring) + 1 + 
		    strlen (arch) + strlen (DEBEXT) + 1);
      sprintf (m, "%s/%s_%s%s%s%s", debar, checkedinfo->name, versionstring,
	       arch[0] ? "_" : "", arch, DEBEXT);
      debar = m;
    }
    printf (_("dpkg-deb: building package `%s' in `%s'.\n"), checkedinfo->name, debar);

    strcpy(controlfile, directory);
    strcat(controlfile, "/");
    strcat(controlfile, BUILDCONTROLDIR);
    strcat(controlfile, "/");
    if (lstat(controlfile,&mscriptstab)) ohshite(_("unable to stat control directory"));
    if (!S_ISDIR(mscriptstab.st_mode)) ohshit(_("control directory is not a directory"));
    if ((mscriptstab.st_mode & 07757) != 0755)
      ohshit(_("control directory has bad permissions %03lo (must be >=0755 "
             "and <=0775)"), (unsigned long)(mscriptstab.st_mode & 07777));

    for (mscriptp= maintainerscripts; *mscriptp; mscriptp++) {
      strcpy(controlfile, directory);
      strcat(controlfile, "/");
      strcat(controlfile, BUILDCONTROLDIR);
      strcat(controlfile, "/");
      strcat(controlfile, *mscriptp);

      if (!lstat(controlfile,&mscriptstab)) {
        if (S_ISLNK(mscriptstab.st_mode)) continue;
        if (!S_ISREG(mscriptstab.st_mode))
          ohshit(_("maintainer script `%.50s' is not a plain file or symlink"),*mscriptp);
        if ((mscriptstab.st_mode & 07557) != 0555)
          ohshit(_("maintainer script `%.50s' has bad permissions %03lo "
                 "(must be >=0555 and <=0775)"),
                 *mscriptp, (unsigned long)(mscriptstab.st_mode & 07777));
      } else if (errno != ENOENT) {
        ohshite(_("maintainer script `%.50s' is not stattable"),*mscriptp);
      }
    }

    sprintf (controlfile, "%s/%s/%s", directory, BUILDCONTROLDIR, CONFFILESFILE);
    if ((cf= fopen(controlfile,"r"))) {
      while (fgets(conffilename,MAXCONFFILENAME+1,cf)) {
        n= strlen(conffilename);
        if (!n) ohshite(_("empty string from fgets reading conffiles"));
        if (conffilename[n-1] != '\n') {
          fprintf(stderr, _("warning, conffile name `%.50s...' is too long"), conffilename);
          warns++;
          while ((c= getc(cf)) != EOF && c != '\n');
          continue;
        }
        conffilename[n-1]= 0;
        strcpy(controlfile, directory);
        strcat(controlfile, "/");
        strcat(controlfile, conffilename);
        if (lstat(controlfile,&controlstab)) {
          if (errno == ENOENT)
            ohshit(_("conffile `%.250s' does not appear in package"),conffilename);
          else
            ohshite(_("conffile `%.250s' is not stattable"),conffilename);
        } else if (!S_ISREG(controlstab.st_mode)) {
          fprintf(stderr, _("warning, conffile `%s'"
                  " is not a plain file\n"), conffilename);
          warns++;
        }
      }
      if (ferror(cf)) ohshite(_("error reading conffiles file"));
      fclose(cf);
    } else if (errno != ENOENT) {
      ohshite(_("error opening conffiles file"));
    }
    if (warns) {
      if (fprintf(stderr, _("dpkg-deb: ignoring %d warnings about the control"
                  " file(s)\n"), warns) == EOF) werr("stderr");
    }
  }
  if (ferror(stdout)) werr("stdout");
  
  if (!(ar=fopen(debar,"wb"))) ohshite(_("unable to create `%.255s'"),debar);
  if (setvbuf(ar, 0, _IONBF, 0)) ohshite(_("unable to unbuffer `%.255s'"),debar);
  m_pipe(p1);
  if (!(c1= m_fork())) {
    m_dup2(p1[1],1); close(p1[0]); close(p1[1]);
    if (chdir(directory)) ohshite(_("failed to chdir to `%.255s'"),directory);
    if (chdir(BUILDCONTROLDIR)) ohshite(_("failed to chdir to .../DEBIAN"));
    execlp(TAR,"tar","-cf","-",".",(char*)0); ohshite(_("failed to exec tar -cf"));
  }
  close(p1[1]);
  if (!(gz= tmpfile())) ohshite(_("failed to make tmpfile (control)"));
  if (!(c2= m_fork())) {
    m_dup2(p1[0],0); m_dup2(fileno(gz),1); close(p1[0]);
    execlp(GZIP,"gzip","-9c",(char*)0); ohshite(_("failed to exec gzip -9c"));
  }
  close(p1[0]);
  waitsubproc(c2,"gzip -9c",0);
  waitsubproc(c1,"tar -cf",0);
  if (fstat(fileno(gz),&controlstab)) ohshite(_("failed to fstat tmpfile (control)"));
  if (oldformatflag) {
    if (fprintf(ar, "%-8s\n%ld\n", OLDARCHIVEVERSION, (long)controlstab.st_size) == EOF)
      werr(debar);
  } else {
    thetime= time(0);
    if (fprintf(ar,
                "!<arch>\n"
                "debian-binary   %-12lu0     0     100644  %-10ld`\n"
                "%s" "\n"
                "%s"
                "%s" "%-12lu0     0     100644  %-10ld`\n",
                thetime,
                (long) strlen (ARCHIVEVERSION) + 1,
		ARCHIVEVERSION,
                (strlen (ARCHIVEVERSION) & 1) ? "" : "\n",
		ADMINMEMBER,
                (unsigned long) thetime,
                (long) controlstab.st_size) == EOF)
      werr (debar);
  }                
                
  if (lseek(fileno(gz),0,SEEK_SET)) ohshite(_("failed to rewind tmpfile (control)"));
  if (!(c3= m_fork())) {
    m_dup2(fileno(gz),0); m_dup2(fileno(ar),1);
    execlp(CAT,"cat",(char*)0); ohshite(_("failed to exec cat (control)"));
  }
  waitsubproc(c3,_("cat (control)"),0);
  
  if (!oldformatflag) {
    fclose(gz);
    if (!(gz= tmpfile())) ohshite(_("failed to make tmpfile (data)"));
  }
  m_pipe(p2);
  if (!(c4= m_fork())) {
    m_dup2(p2[1],1); close(p2[0]); close(p2[1]);
    if (chdir(directory)) ohshite(_("failed to chdir to `%.255s'"),directory);
    execlp(TAR,"tar","--exclude",BUILDCONTROLDIR,"-cf","-",".",(char*)0);
    ohshite(_("failed to exec tar --exclude"));
  }
  close(p2[1]);
  if (!(c5= m_fork())) {
    m_dup2(p2[0],0); close(p2[0]);
    m_dup2(oldformatflag ? fileno(ar) : fileno(gz),1);
    execlp(GZIP,"gzip","-9c",(char*)0);
    ohshite(_("failed to exec gzip -9c from tar --exclude"));
  }
  close(p2[0]);
  waitsubproc(c5,_("gzip -9c from tar --exclude"),0);
  waitsubproc(c4,"tar --exclude",0);
  if (!oldformatflag) {
    if (fstat(fileno(gz),&datastab)) ohshite("_(failed to fstat tmpfile (data))");
    if (fprintf(ar,
                "%s"
                DATAMEMBER "%-12lu0     0     100644  %-10ld`\n",
                (controlstab.st_size & 1) ? "\n" : "",
                (unsigned long)thetime,
                (long)datastab.st_size) == EOF)
      werr(debar);

    if (lseek(fileno(gz),0,SEEK_SET)) ohshite(_("failed to rewind tmpfile (data)"));
    if (!(c3= m_fork())) {
      m_dup2(fileno(gz),0); m_dup2(fileno(ar),1);
      execlp(CAT,"cat",(char*)0); ohshite(_("failed to exec cat (data)"));
    }
    waitsubproc(c3,_("cat (data)"),0);

    if (datastab.st_size & 1)
      if (putc('\n',ar) == EOF)
        werr(debar);
  }
  if (fclose(ar)) werr(debar);
                             
  _exit(0);
}

