#ifndef _NEXTSTEP_TDEP_H_
#define _NEXTSTEP_TDEP_H_

struct internal_nlist;
struct external_nlist;

void next_internalize_symbol 
PARAMS ((struct internal_nlist *in, struct external_nlist *ext, bfd *abfd));

#endif /* _NEXTSTEP_TDEP_H_ */
