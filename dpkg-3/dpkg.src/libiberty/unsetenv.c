void unsetenv (const char *p) 
{
  char *q;
  q= malloc(strlen(p)+3); if (!q) return;
  strcpy(q,p); strcat(q,"="); putenv(q);
}
