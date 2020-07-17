#include "arch.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <sys/utsname.h>

#include "varbuf.h"
#include "dpkg.h"
#include "dpkg-db.h"
#include "libdpkg-int.h"
#include "config.h"

#define DPKG_ARCHSTR_MAX 256

static char const *archcompat[] = 
{ "i386", "sparc", "alpha", "m68k", "arm", "powerpc" };

int dpkg_compatible_architecture (char *hostarch, char *pkgarch)
{
  return 0;
}

/* destroys buf */

int dpkg_parse_architecture (char *buf, char **cpu, char **manufacturer, char **kernel, char **os)
{
  char *s = buf;
  *cpu = *manufacturer = *kernel = *os = NULL;

  *cpu = s;
  s = index (s, '-');
  if (s == NULL) { *cpu = NULL; return -1; }
  *s++ = '\0';
  
  *manufacturer = s;
  s = index (s, '-');
  if (s == NULL) { *cpu = *manufacturer = NULL; return -1; }
  *s++ = '\0';
  
  *os = s;
  s = index (s, '-');
  if (s == NULL) { return 0; }
  *s++ = '\0';

  /* if we get here, it's CPU-MANUFACTURER-KERNEL-OS, not CPU-MANUFACTURER-OS */

  *kernel = *os;
  *os = s;
  return 0;
}

int dpkg_canonicalize_architecture (const char *src, unsigned char *dst, size_t n)
{
  const char *cpu, *manufacturer, *kernel, *os;
  char buf[DPKG_ARCHSTR_MAX];
  int ret;
  
  ret = snprintf (buf, DPKG_ARCHSTR_MAX, "%s", src);
  if (ret < 0) { return ret; }
  
  ret = dpkg_parse_architecture
    (buf, (char **) &cpu, (char **) &manufacturer, (char **) &kernel, (char **) &os);
  if (ret != 0) { ohshit (_("unable to parse architecture string `%.250s'"), src); }
  
  if ((cpu == NULL) || (manufacturer == NULL) || (os == NULL)) { return -2; }
  
  if ((kernel != NULL) && (strcmp (kernel, "linux") == 0)) {
    /* remap common processor aliases */
    if ((strcmp (cpu, "i486") == 0)
	|| (strcmp (cpu, "i586") == 0)
	|| (strcmp (cpu, "i686") == 0)
	|| (strcmp (cpu, "pentium") == 0)) { 
      cpu = "i386";
    } else if (strcmp (cpu, "ppc") == 0) {
      cpu = "powerpc";
    }

    if (strcmp (manufacturer, "pc") == 0) {
      manufacturer = "unknown";
    }
  }

  if (strncmp (os, "rhapsody", strlen ("rhapsody")) == 0) {
    os = "rhapsody";
  } else if (strncmp (os, "rhapsody", strlen ("rhapsody")) == 0) {
    os = "darwin";
  } else {
    os = "unknown";
  }

  if (kernel != NULL) {
    return snprintf (dst, n, "%s-%s-%s-%s", cpu, manufacturer, kernel, os);
  } else {
    return snprintf (dst, n, "%s-%s-%s", cpu, manufacturer, os);
  }
}

int dpkg_expand_archcompat (const char *src, char *dst, size_t n)
{
  const char **sptr = NULL;
  for (sptr = archcompat; (*sptr) != NULL; sptr++) {
    if (strcmp (*sptr, src) == 0) {
      return snprintf (dst, n, "%s-unknown-linux-gnu", src);
    }
  }
  return snprintf (dst, n, "%s", src);
}

int dpkg_reduce_archcompat (const char *src, char *dst, size_t n)
{
  const char **sptr = NULL;
  for (sptr = archcompat; (*sptr) != NULL; sptr++) {
    size_t len = strlen (*sptr);
    if ((strncmp (*sptr, src, len) == 0) 
	&& ((strcmp ((src + len), "-unknown-linux-gnu") == 0)
	    || (strcmp ((src + len), "-pc-linux-gnu") == 0))) {
      return snprintf (dst, n, *sptr);
    }
  }
  return snprintf (dst, n, "%s", src);
}

/* return the GNU-format architecture string of the machine on which
   this version of dpkg was compiled */

int dpkg_find_gnu_build_architecture (char *dst, size_t n)
{
  return snprintf (dst, n, "%s", ARCHITECTURE);
}

int dpkg_find_gnu_installation_architecture (unsigned char *str, size_t n)
{
  int ret;
  unsigned char buf[DPKG_ARCHSTR_MAX];

  ret = dpkg_find_gnu_build_architecture (buf, DPKG_ARCHSTR_MAX);
  if (ret <= 0) { return ret; }
  ret = dpkg_canonicalize_architecture (buf, str, n);
  return ret;
}

static void badlgccfn (const char *compiler, const char *output, const char *why) NONRETURNING;
static void badlgccfn (const char *compiler, const char *output, const char *why) {
  fprintf (stderr,
	   _("dpkg: unexpected output from `%s --print-libgcc-file-name':\n"
	     " `%s'\n"),
	   compiler,output);
  ohshit(_("compiler libgcc filename not understood: %.250s"),why);
}

int dpkg_find_gnu_target_architecture (char *dst, size_t n)
{
  struct utsname uts;
  
  if (uname (&uts) < 0) {
    ohshite (_("unable to determine system information using uname()"));
  }

  /* for the moment, we support only native compilation on Rhapsody */
  if (strcmp (uts.sysname, "Rhapsody") == 0) {
    return snprintf (dst, n, "%s-%s", uts.machine, "apple-rhapsody");
  } else if (strcmp (uts.sysname, "Darwin") == 0) {
    return snprintf (dst, n, "%s", "powerpc-apple-darwin");
  }

  /* on linux, use gcc --print-libgcc-file-name to find the compilation architecture */
  if (strcmp (uts.sysname, "Linux") == 0) {

    const char *ccompiler;
    int p1[2], c;
    pid_t c1;
    FILE *ccpipe;
    struct varbuf vb;
    char *p, *q;

    ccompiler = getenv ("CC");
    if (ccompiler == NULL) { ccompiler= "gcc"; }
    varbufinit (&vb);
    m_pipe (p1);
    ccpipe = fdopen (p1[0], "r"); 
    if (! ccpipe) { ohshite (_("failed to fdopen CC pipe")); }
    if (! (c1 = m_fork ())) {
      m_dup2 (p1[1], 1); 
      close (p1[0]); 
      close (p1[1]);
      execlp (ccompiler, ccompiler, "--print-libgcc-file-name", (char*) 0);
      ohshite (_("failed to exec C compiler `%.250s'"), ccompiler);
    }
    close (p1[1]);

    while ((c = getc (ccpipe)) != EOF) { varbufaddc (&vb,c); }
    if (ferror(ccpipe)) ohshite(_("error reading from CC pipe"));
    waitsubproc (c1, "gcc --print-libgcc-file-name", 0);
    if (!vb.used) { badlgccfn (ccompiler, "", _("empty output")); }
    varbufaddc (&vb,0);

    if (vb.buf[vb.used-2] != '\n') { badlgccfn (ccompiler, vb.buf, _("no newline")); }
    vb.used -= 2;
    varbufaddc (&vb,0);
    p = strstr (vb.buf, "/gcc-lib/");
    if (p == NULL) { badlgccfn (ccompiler, vb.buf, _("no gcc-lib component")); }
    p += 9;
    q = strchr (p, '-'); 
    if (q == NULL) { badlgccfn (ccompiler, vb.buf, _("no hyphen after gcc-lib")); }
    q = strchr (q, '/');
    if (q == NULL) { badlgccfn (ccompiler, vb.buf, _("no / after arch-directory")); }
    *q = '\0';
    q = strchr (p, '-');
    if (q != NULL) { *q = '\0'; }

    return snprintf (dst, n, "%s-unknown-linux-gnu", p);
  }

  return 0;
}
