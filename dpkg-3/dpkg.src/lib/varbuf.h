#ifndef _DPKG_VARBUF_H_
#define _DPKG_VARBUF_H_

struct varbuf;

extern void varbufaddc (struct varbuf *v, int c);
void varbufprintf (struct varbuf *v, const char *fmt, ...) 
  __attribute__ ((format (printf, 2, 3)));
extern void varbufaddstr (struct varbuf *v, const char *s);
extern void varbufinit (struct varbuf *v);
extern void varbufreset (struct varbuf *v);
extern void varbufextend (struct varbuf *v);
extern void varbuffree (struct varbuf *v);

/* varbufinit must be called exactly once before the use of each
   varbuf (including before any call to varbuffree).
 
   However, varbufs allocated `static' are properly initialised anyway and
   do not need varbufinit; multiple consecutive calls to varbufinit before
   any use are allowed.
   
   varbuffree must be called after a varbuf is finished with, if anything
   other than varbufinit has been done.  After this you are allowed but
   not required to call varbufinit again if you want to start using the
   varbuf again.
   
   Callers using C++ need not worry about any of this.  */

struct varbuf {

  int size;
  int used;
  char *buf;

#ifdef __cplusplus
  void init() { varbufinit(this); }
  void free() { varbuffree(this); }
  varbuf() { varbufinit(this); }
  ~varbuf() { varbuffree(this); }
  inline void operator()(int c); // definition below
  void operator()(const char *s) { varbufaddstr(this,s); }
  inline void terminate(void/*to shut 2.6.3 up*/); // definition below
  void reset() { used=0; }
  const char *string() { terminate(); return buf; }
#endif
};

#if HAVE_INLINE
inline extern void varbufaddc(struct varbuf *v, int c) {
  if (v->used >= v->size) varbufextend(v);
  v->buf[v->used++]= c;
}
#endif

#ifdef __cplusplus
inline void varbuf::operator()(int c) { varbufaddc(this,c); }
inline void varbuf::terminate(void/*to shut 2.6.3 up*/) { varbufaddc(this,0); used--; }
#endif

#endif /* _DPKG_VARBUF_H_ */
