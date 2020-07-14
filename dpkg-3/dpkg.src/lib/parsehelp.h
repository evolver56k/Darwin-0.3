#ifndef _DPKG_PARSEHELP_H_
#define _DPKG_PARSEHELP_H_

struct namevalue {
  const char *name;
  int value;
};

extern const struct namevalue booleaninfos[];
extern const struct namevalue priorityinfos[];
extern const struct namevalue statusinfos[];
extern const struct namevalue eflaginfos[];
extern const struct namevalue wantinfos[];

const char *skip_slash_dotslash(const char *p);

int informativeversion(const struct versionrevision *version);

enum versiondisplayepochwhen { vdew_never, vdew_nonambig, vdew_always };
void varbufversion(struct varbuf*, const struct versionrevision*,
                   enum versiondisplayepochwhen);
const char *parseversion(struct versionrevision *rversion, const char*);
const char *versiondescribe(const struct versionrevision*,
                            enum versiondisplayepochwhen);

#endif /* _DPKG_PARSEHELP_H_ */
