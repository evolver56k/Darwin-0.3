#ifndef _DPKG_NFMALLOC_H_
#define _DPKG_NFMALLOC_H_

void *nfmalloc(size_t);
char *nfstrsave(const char*);
void nffreeall(void);

#endif /* _DPKG_NFMALLOC_H_ */
