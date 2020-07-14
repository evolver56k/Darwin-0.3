#ifndef _DPKG_DUMP_H_
#define _DPKG_DUMP_H_

void writerecord
 (FILE*, const char*,
  const struct pkginfo*, const struct pkginfoperfile*);

void writedb (const char *filename, int available, int mustsync);

void varbufrecord (struct varbuf*, const struct pkginfo*, const struct pkginfoperfile*);
void varbufdependency (struct varbuf *vb, struct dependency *dep);

/* NB THE VARBUF MUST HAVE BEEN INITIALISED AND WILL NOT BE NULL-TERMINATED */

#endif /* _DPKG_DUMP_H_ */
