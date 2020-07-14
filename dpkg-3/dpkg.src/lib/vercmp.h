#ifndef _DPKG_VERCMP_H_
#define _DPKG_VERCMP_H_

int versionsatisfied 
 (struct pkginfoperfile *it, struct deppossi *against);

int versionsatisfied3
 (const struct versionrevision *it,
  const struct versionrevision *ref,
  enum depverrel verrel);

int versioncompare
 (const struct versionrevision *version,
  const struct versionrevision *refversion);

int epochsdiffer
 (const struct versionrevision *a,
  const struct versionrevision *b);

#endif /* _DPKG_VERCMP_H_ */
