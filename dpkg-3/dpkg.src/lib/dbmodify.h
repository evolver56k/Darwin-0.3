#ifndef _DPKG_DBMODIFY_H_
#define _DPKG_DBMODIFY_H_

enum modstatdb_rw {
  /* Those marked with \*s*\ are possible returns from modstatdb_init. */
  msdbrw_readonly/*s*/, msdbrw_needsuperuserlockonly/*s*/,
  msdbrw_writeifposs,
  msdbrw_write/*s*/, msdbrw_needsuperuser,
  /* Now some optional flags: */
  msdbrw_flagsmask= ~077,
  /* ... of which there are currently none, but they'd start at 0100 */
};

enum modstatdb_rw modstatdb_init(const char *admindir, enum modstatdb_rw reqrwflags);
void modstatdb_note(struct pkginfo *pkg);
void modstatdb_shutdown(void);

extern char *statusfile, *availablefile; /* initialised by modstatdb_init */

#endif /* _DPKG_DBMODIFY_H_ */
