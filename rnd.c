/* $Id$ */

#include <stdlib.h> /*calloc,free*/
#include <assert.h> /*assert*/
#include <stdarg.h> /*va_list,va_start,va_end*/
#include "er.h"
#include "rn.h"
#include "rnd.h"

#define LEN_F 1024

static int len_f,n_f;
static int *flat;
static int errors;

int rnd_errors() {
  return errors!=0;
}

static void error(int er_no,...) {
  va_list ap; va_start(ap,er_no); (*ver_handler_p)(er_no,ap); va_end(ap);
  ++errors;
}

static void realloc_f() {
  int *newflat;
  newflat=(int*)calloc(len_f*=2,sizeof(int));
  memcpy(newflat,flat,n_f*sizeof(int)); free(flat);
  flat=newflat;
}

static int deref(int p) {
  int p0=p,p1,name;
  P_CHK(p,REF);
  for(;;) {
    Ref(p,p1,name);
    if(!P_IS(p1,REF)||p1==p0) break;
    p=p1;
  }
  return p1;
}

static void flatten(p) { if(!marked(p)) {flat[n_f++]=p; mark(p);}}

void rnd_deref(int start) {
  int p,p1,p2,nc,name,i,changed;

  flat=(int*)calloc(len_f=LEN_F,sizeof(int)); n_f=0;
  errors=0;

  if(P_IS(start,REF)) start=deref(start);
  flat[n_f++]=start; mark(start);
  
  i=0;
  do {
    p=flat[i++];
    switch(P_TYP(p)) {
    case P_EMPTY: case P_NOT_ALLOWED: case P_TEXT: case P_DATA: case P_VALUE: 
      break;

    case P_CHOICE: Choice(p,p1,p2); goto BINARY;
    case P_INTERLEAVE: Interleave(p,p1,p2); goto BINARY;
    case P_GROUP: Group(p,p1,p2); goto BINARY;
    case P_DATA_EXCEPT: DataExcept(p,p1,p2); goto BINARY;
    BINARY:
      changed=0;
      if(P_IS(p1,REF)) {p1=deref(p1); changed=1;}
      if(P_IS(p2,REF)) {p2=deref(p2); changed=1;}
      if(changed) {rn_del_p(p); rn_pattern[p][1]=p1; rn_pattern[p][2]=p2; rn_add_p(p);}
      if(n_f+2>len_f) realloc_f();
      flatten(p1); flatten(p2);
      break;

    case P_ONE_OR_MORE: OneOrMore(p,p1); goto UNARY;
    case P_LIST: List(p,p1); goto UNARY;
    case P_ATTRIBUTE: Attribute(p,nc,p1); goto UNARY;
    case P_ELEMENT: Element(p,nc,p1); goto UNARY;
    UNARY:
      changed=0;
      if(P_IS(p1,REF)) {p1=deref(p1); changed=1;}
      if(changed) {rn_del_p(p); rn_pattern[p][1]=p1; rn_add_p(p);}
      if(n_f+1>len_f) realloc_f();
      flatten(p1);
      break;

    case P_REF:
      Ref(p,p1,name);
      error(ER_LOOPREF,rn_string+name);
      break;

    default:
      assert(0);
    }
  } while(i!=n_f);
  for(i=0;i!=n_f;++i) unmark(flat[i]);
}

static int loop(int p) {
  int p1,p2,ret=1;
  if(marked(p)) return 1;
  mark(p);
  switch(P_TYP(p)) {
  case P_EMPTY: case P_NOT_ALLOWED: case P_TEXT: case P_DATA: case P_VALUE:
  case P_ATTRIBUTE: case P_ELEMENT:
    ret=0; break;

  case P_CHOICE: Choice(p,p1,p2); goto BINARY;
  case P_INTERLEAVE: Interleave(p,p1,p2); goto BINARY;
  case P_GROUP: Group(p,p1,p2); goto BINARY;
  case P_DATA_EXCEPT: DataExcept(p,p1,p2); goto BINARY;
  BINARY:
    ret=loop(p1)||loop(p2); break;

  case P_ONE_OR_MORE: OneOrMore(p,p1); goto UNARY;
  case P_LIST: List(p,p1); goto UNARY;
  UNARY:
    ret=loop(p1); break;

  default:
    assert(0);
  }
  unmark(p);
  return ret;
}

void rnd_loops() {
  int i=0,p=flat[i],nc=-1,p1;
  for(;;) {
    if(loop(p)) {
      if(i==0) error(ER_LOOPST); else {
	char *s=nc2str(nc);
	error(ER_LOOPEL,s);
	free(s);
      }
      return;
    }
    for(;;) {++i;
      if(i==n_f) return;
      p=flat[i];
      if(P_IS(p,ELEMENT)) {
	Element(p,nc,p1); p=p1;
	break;
      }
    }
  }
}
    
void rnd_release() {
  free(flat); flat=NULL;
}

/* 
 * $Log$
 * Revision 1.2  2003/12/07 16:50:55  dvd
 * stage D, dereferencing and checking for loops
 *
 */
