#ifndef _DPKG_DATABASE_H_
#define _DPKG_DATABASE_H_

struct pkginfo *findpackage(const char *name);
void blankpackage(struct pkginfo *pp);
void blankpackageperfile(struct pkginfoperfile *pifp);
void blankversion(struct versionrevision*);
int informative(struct pkginfo *pkg, struct pkginfoperfile *info);
int countpackages(void);
void resetpackages(void);

struct pkgiterator *iterpkgstart(void);
struct pkginfo *iterpkgnext(struct pkgiterator*);
void iterpkgend(struct pkgiterator*);

void hashreport(FILE*);

#endif /* _DPKG_DATABASE_H_ */
