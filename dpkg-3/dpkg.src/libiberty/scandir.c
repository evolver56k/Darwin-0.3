static int (*scandir_comparfn)(const void*, const void*);
static int scandir_compar(const void *a, const void *b) {
  return scandir_comparfn(*(const struct dirent**)a,*(const struct dirent**)b);
}
      
int scandir(const char *dir, struct dirent ***namelist,
            int (*select)(const struct dirent *),
            int (*compar)(const void*, const void*)) {
  DIR *d;
  int used, avail;
  struct dirent *e, *m;
  d= opendir(dir);  if (!d) return -1;
  used=0; avail=20;
  *namelist= malloc(avail*sizeof(struct dirent*));
  if (!*namelist) return -1;
  while ((e= readdir(d)) != 0) {
    if (!select(e)) continue;
    m= malloc(sizeof(struct dirent) + strlen(e->d_name));
    if (!m) return -1;
    *m= *e;
    strcpy(m->d_name,e->d_name);
    if (used >= avail-1) {
      avail+= avail;
      *namelist= realloc(*namelist, avail*sizeof(struct dirent*));
      if (!*namelist) return -1;
    }
    (*namelist)[used]= m;
    used++;
  }
  (*namelist)[used]= 0;
  scandir_comparfn= compar;
  qsort(*namelist, used, sizeof(struct dirent*), scandir_compar);
  return used;
}
