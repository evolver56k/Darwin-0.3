#include <stdlib.h>

#define DPKG_ARCHSTR_MAX 256

int dpkg_parse_architecture (char *buf, char **cpu, char **manufacturer, char **kernel, char **os);
int dpkg_canonicalize_architecture (const char *src, unsigned char *dst, size_t n);
int dpkg_expand_archcompat (const char *src, char *dst, size_t n);
int dpkg_reduce_archcompat (const char *src, char *dst, size_t n);
int dpkg_find_gnu_build_architecture (char *dst, size_t n);
int dpkg_find_gnu_installation_architecture (unsigned char *str, size_t n);
int dpkg_find_gnu_target_architecture (char *dst, size_t n);
int dpkg_compatible_architecture (char *hostarch, char *pkgarch);
