/* $Id$ */

#include <stdlib.h>
#include <string.h>
#include "er.h"
#include "m.h"

#ifndef M_STATIC
#define M_STATIC 0
#endif

#if M_STATIC

#ifndef M_FILL 
#define M_FILL 0
#endif

static char memory[M_STATIC];
static char *mp=memory,*pmp=memory;

void m_free(void *p) {
  if(p==pmp) {
    mp=pmp; pmp=(char*)-1;
  }
}

extern void *m_alloc(int length,int size) {
  char *p=mp, *q=mp; int n=length*size;
  pmp=mp; mp+=(n+sizeof(int)-1)/sizeof(int)*sizeof(int);
  while(q!=mp) *(q++)=M_FILL;
  if(mp>=memory+M_STATIC) {
    (*er_printf)("failed to allocate %i bytes of memory\n",length*size);
    exit(1);
  }
  return (char*)p;
}

#else

void m_free(void *p) {
  free(p);
}

extern void *m_alloc(int length,int size) {
  void *p=calloc(length,size);
  if(p==NULL) {
    (*er_printf)("failed to allocate %i bytes of memory\n",length*size);
    exit(1);
  }
  return p;
}

#endif

void *m_stretch(void *p,int newlen,int oldlen,int size) {
  void *newp=m_alloc(newlen,size);
  memcpy(newp,p,oldlen*size);
  m_free(p);
  return newp;
}
