/* $Id$ */

#include <stdlib.h> /*calloc,free*/
#include <string.h> /*memcpy*/
#include "rn.h"
#include "ll.h"
#include "rnx.h"

#define LEN_EXP RNX_LEN_EXP
#define LIM_EXP RNX_LIM_EXP

int *rnx_exp=NULL;
int rnx_n_exp;
static int len_exp;

static int initialized=0;
void rnx_init(void) {
  if(!initialized) { initialized=1;
    rnx_exp=(int*)calloc(len_exp=LEN_EXP,sizeof(int));
  }
}

void rnx_clear(void) {}

static void expected(int p) {
  int p1,p2,px=0,i;
  switch(P_TYP(p)) {
  case P_ERROR: break;
  case P_EMPTY: break;
  case P_NOT_ALLOWED: break;
  case P_TEXT: px=p; break;
  case P_CHOICE: Choice(p,p1,p2); expected(p1); expected(p2); break;
  case P_INTERLEAVE: Interleave(p,p1,p2); expected(p1); expected(p2); break;
  case P_GROUP: Group(p,p1,p2); expected(p1); if(nullable(p1)) expected(p2); break;
  case P_ONE_OR_MORE: OneOrMore(p,p1); expected(p1); break;
  case P_LIST: List(p,p1); expected(p1); break;
  case P_DATA: px=p; break;
  case P_DATA_EXCEPT: DataExcept(p,p1,p2); expected(p1); break;
  case P_VALUE: px=p; break;
  case P_ATTRIBUTE: px=p; break;
  case P_ELEMENT: px=p; break;
  case P_AFTER: After(p,p1,p2); expected(p1); if(nullable(p1)) px=p; break;
  case P_REF: break;
  default: assert(0);
  }
  if(px) {
    for(i=0;i!=rnx_n_exp;++i) {
      if(rnx_exp[i]==px) {px=0; break;}
    }
    if(px) {
      if(rnx_n_exp==len_exp) {
	int *newexp=(int*)calloc(len_exp*=2,sizeof(int));
	memcpy(newexp,rnx_exp,rnx_n_exp*sizeof(int)); free(rnx_exp);
	rnx_exp=newexp;
      }
      rnx_exp[rnx_n_exp++]=px;
    }
  }
}
void rnx_expected(int p) {
  if(len_exp>LIM_EXP) {
    free(rnx_exp);
    rnx_exp=(int*)calloc(len_exp=LIM_EXP,sizeof(int));
  }
  rnx_n_exp=0;
  expected(p);
}
