#ifndef _DPKG_PARSE_H_
#define _DPKG_PARSE_H_

/*** from parse.c ***/

enum parsedbflags {
  pdb_recordavailable   =001, /* Store in `available' in-core structures, not `status' */
  pdb_rejectstatus      =002, /* Throw up an error if `Status' encountered             */
  pdb_weakclassification=004  /* Ignore priority/section info if we already have any   */
};

const char *illegal_packagename(const char *p, const char **ep);
int parsedb(const char *filename, enum parsedbflags, struct pkginfo **donep,
            FILE *warnto, int *warncount);
void copy_dependency_links(struct pkginfo *pkg,
                           struct dependency **updateme,
                           struct dependency *newdepends,
                           int available);

#endif /* _DPKG_PARSE_H_ */
