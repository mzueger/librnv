/* $Id$ */

#include "util.h"

#define whitespace(v) ((v)==' '||(v)=='\t')

int tokncmp(char *s1,char *s2,int n2) {
  char *end2=s2+n2;
  while(whitespace(*s1)) ++s1;
  while(s2!=end2&&whitespace(*s2)) ++s2;
  for(;;) {
    if(s2==end2) {
      while(whitespace(*s1)) ++s1;
      return *s1;
    }
    if(*s1=='\0') {
      while(s2!=end2&&whitespace(*s2)) ++s2;
      return s2==end2?0:-*s2;
    }
    if(whitespace(*s1)&&whitespace(*s2)) {
      do ++s1; while(whitespace(*s1));
      do ++s2; while(s2!=end2&&whitespace(*s2));
    } else {
      if(*s1!=*s2) return *s1-*s2;
      ++s1; ++s2;
    }
  }
}

int strhash(char *s) {
  int h=0; 
  while(*s) h=h*31+*(s++); 
  return h;
}

extern char *abspath(char *r,char *b) {
  if(*r!='/') {
    char *c=b,*sep=(char*)0;
    for(;;) {if(!(*c)) break; if(*c++=='/') sep=c;}
    if(sep) {
      char *p,*q;
      p=r; while(*p++); q=p+(sep-b);
      do *(--q)=*(--p); while(p!=r);
      while(b!=sep) *r++=*b++;
    }
  }
  return r;
}

/*
 * $Log$
 * Revision 1.2  2003/12/11 23:37:58  dvd
 * derivative in progress
 *
 * Revision 1.1  2003/11/27 21:00:23  dvd
 * abspath,strhash
 *
 */
