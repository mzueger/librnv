/* $Id$ */

#include <stdlib.h> /* calloc */
#include <string.h> /* strcmp,memcmp,strlen,strcpy,strdup,memcpy,memset */
#include <stdio.h> /* debugging*/

#include "util.h"
#include "ht.h"
#include "rn.h"

#define LEN_P 1024
#define LEN_NC 1024
#define LEN_S 16384
#define LEN_PS 1024

int (*rn_pattern)[P_SIZE];
int (*rn_nameclass)[NC_SIZE];
char *rn_string,*rn_params;
int rn_empty,rn_text,rn_notAllowed,rn_dt_string,rn_dt_token,rn_xsd_uri;

static struct hashtable ht_p, ht_nc, ht_s;

static int i_p,i_nc,i_s,base_p,i_ref,i_ps;
static int len_p,len_nc,len_s,len_ps;

void rn_new_schema(void) {base_p=i_p; i_ref=0;}

void rn_del_p(int i) {if(ht_get(&ht_p,i)==i) ht_del(&ht_p,i);}
void rn_add_p(int i) {if(ht_get(&ht_p,i)==-1) ht_put(&ht_p,i);}

void setNullable(int i,int x) {if(x) rn_pattern[i][0]|=P_FLG_NUL;}
void setCdata(int i,int x) {if(x) rn_pattern[i][0]|=P_FLG_TXT;}
void setContentType(int i,int t1,int t2) {rn_pattern[i][0]|=(t1>t2?t1:t2);}

int newString(char *s) {
  int len=strlen(s)+1, j;
  if(i_s+len>len_s) {
    char *string=(char*)calloc(len_s=(i_s+len)*2,sizeof(char));
    memcpy(string,rn_string,i_s*sizeof(char)); free(rn_string); rn_string=string;
  }
  strcpy(rn_string+i_s,s);
  if((j=ht_get(&ht_s,i_s))==-1) {
    ht_put(&ht_s,j=i_s);
    i_s+=len;
  }
  return j;
}

#define P_NEW(x) rn_pattern[i_p][0]=P_##x

#define P_LABEL "pattern"
#define NC_LABEL "nameclass"

#define accept(name,n,N)  \
static int accept_##n(void) { \
  int j; \
  if((j=ht_get(&ht_##n,i_##n))==-1) { \
    ht_put(&ht_##n,j=i_##n++); \
    if(i_##n==len_##n) { \
      int (*name)[N##_SIZE]=(int (*)[N##_SIZE])calloc(len_##n*=2,sizeof(int[N##_SIZE])); \
      memcpy(name,rn_##name,i_##n*sizeof(int[N##_SIZE])); \
      free(rn_##name); rn_##name=name; \
    } \
  } \
  memset(rn_##name[i_##n],0,sizeof(int[N##_SIZE])); \
  return j; \
}

accept(pattern,p,P)
accept(nameclass,nc,NC)

int newEmpty(void) { P_NEW(EMPTY);
  setNullable(i_p,1);
  return accept_p();
}

int newNotAllowed(void) { P_NEW(NOT_ALLOWED);
  return accept_p();
}

int newText(void) { P_NEW(TEXT);
  setNullable(i_p,1); 
  setCdata(i_p,1);
  return accept_p();
}

int newChoice(int p1,int p2) { P_NEW(CHOICE);
  rn_pattern[i_p][1]=p1; rn_pattern[i_p][2]=p2;
  setNullable(i_p,nullable(p1)||nullable(p2)); 
  setCdata(i_p,cdata(p1)||cdata(p2)); 
  return accept_p();
}

int newInterleave(int p1,int p2) { P_NEW(INTERLEAVE);
  rn_pattern[i_p][1]=p1; rn_pattern[i_p][2]=p2;
  setNullable(i_p,nullable(p1)&&nullable(p2));
  setCdata(i_p,cdata(p1)||cdata(p2)); 
  return accept_p();
}

int newGroup(int p1,int p2) { P_NEW(GROUP);
  rn_pattern[i_p][1]=p1; rn_pattern[i_p][2]=p2;
  setNullable(i_p,nullable(p1)&&nullable(p2));
  setCdata(i_p,cdata(p1)||cdata(p2)); 
  return accept_p();
}

int newOneOrMore(int p1) { P_NEW(ONE_OR_MORE);
  rn_pattern[i_p][1]=p1;
  setNullable(i_p,nullable(p1));
  setCdata(i_p,cdata(p1));
  return accept_p();
}

int newList(int p1) { P_NEW(LIST);
  rn_pattern[i_p][1]=p1;
  setCdata(i_p,1);
  return accept_p();
}

int newData(int dt,int ps) { P_NEW(DATA);
  rn_pattern[i_p][1]=dt;
  rn_pattern[i_p][2]=ps;
  setCdata(i_p,1);
  return accept_p();
}

int newDataExcept(int p1,int p2) { P_NEW(DATA_EXCEPT);
  rn_pattern[i_p][1]=p1; rn_pattern[i_p][2]=p2;
  setCdata(i_p,1);
  return accept_p();
}

int newValue(int dt,int s) { P_NEW(VALUE);
  rn_pattern[i_p][1]=dt; rn_pattern[i_p][2]=s;
  setCdata(i_p,1);
  return accept_p();
}

int newAttribute(int nc,int p1) { P_NEW(ATTRIBUTE);
  rn_pattern[i_p][2]=nc; rn_pattern[i_p][1]=p1;
  return accept_p();
}

int newElement(int nc,int p1) { P_NEW(ELEMENT);
  rn_pattern[i_p][2]=nc; rn_pattern[i_p][1]=p1; 
  return accept_p();
}

int newAfter(int p1,int p2) { P_NEW(AFTER);
  rn_pattern[i_p][1]=p1; rn_pattern[i_p][2]=p2;
  setCdata(i_p,cdata(p1));
  return accept_p();
}

int newRef(void) { P_NEW(REF);
  rn_pattern[i_p][2]=i_ref++;
  return accept_p();
}

char *p2str(int p) {
  char *s=NULL,*s1;
  int dt,ps,val,nc,p1;
  switch(P_TYP(p)) {
  case P_ERROR: s=strdup("error"); break;
  case P_EMPTY: s=strdup("empty"); break;
  case P_NOT_ALLOWED: s=strdup("notAllowed"); break;
  case P_TEXT: s=strdup("text"); break;
  case P_CHOICE: s=strdup("choice (|)"); break;
  case P_INTERLEAVE: s=strdup("interleave (&)"); break;
  case P_GROUP: s=strdup("group (,)"); break;
  case P_ONE_OR_MORE: s=strdup("one or more (+)"); break;
  case P_LIST: s=strdup("list"); break;
  case P_DATA: Data(p,dt,ps);
    s1=nc2str(dt);
    s=(char*)calloc(strlen("data ")+1+strlen(s1),sizeof(char));
    strcpy(s,"data "); strcat(s,s1);
    free(s1);
    break;
  case P_DATA_EXCEPT: s=strdup("dataExcept (-)");  break;
  case P_VALUE: Value(p,dt,val);
    s=(char*)calloc(strlen("value \"\"")+1+strlen(rn_string+val),sizeof(char));
    strcpy(s,"value \""); strcat(s,rn_string+val); strcat(s,"\"");
    break;
  case P_ATTRIBUTE: Attribute(p,nc,p1);
    s1=nc2str(nc);
    s=(char*)calloc(strlen("attribute ")+1+strlen(s1),sizeof(char));
    strcpy(s,"attribute "); strcat(s,s1);
    free(s1);
    break;
  case P_ELEMENT: Element(p,nc,p1);
    s1=nc2str(nc);
    s=(char*)calloc(strlen("element ")+1+strlen(s1),sizeof(char));
    strcpy(s,"element "); strcat(s,s1);
    free(s1);
    break;
  case P_REF: s=strdup("ref"); break;
  case P_AFTER: s=strdup("after"); break;
  default: assert(0);
  }
  return s;
}

int rn_groupable(int p1,int p2) {
  int ct1=contentType(p1),ct2=contentType(p2);
  return ((ct1&ct2&P_FLG_CTC)||((ct1|ct2)&P_FLG_CTE));
}

int rn_one_or_more(int p) {
  if(P_IS(p,EMPTY)) return p;
  if(P_IS(p,NOT_ALLOWED)) return p;
  if(P_IS(p,TEXT)) return p;
  return newOneOrMore(p);
}

int rn_group(int p1,int p2) {
  if(P_IS(p1,NOT_ALLOWED)) return p1;
  if(P_IS(p2,NOT_ALLOWED)) return p2;
  if(P_IS(p1,EMPTY)) return p2;
  if(P_IS(p2,EMPTY)) return p1;
  return newGroup(p1,p2);
}

static int samechoice(int p1,int p2) {
  if(P_IS(p1,CHOICE)) {
    int p11,p12; Choice(p1,p11,p12);
    return p12==p2||samechoice(p11,p2);
  } else return p1==p2;
}

int rn_choice(int p1,int p2) {
  if(P_IS(p1,NOT_ALLOWED)) return p2;
  if(P_IS(p2,NOT_ALLOWED)) return p1;
  if(P_IS(p2,CHOICE)) {
    int p21,p22; Choice(p2,p21,p22);
    p1=newChoice(p1,p21); return rn_choice(p1,p22);
  }
  if(samechoice(p1,p2)) return p1;
  if(nullable(p1) && (P_IS(p2,EMPTY))) return p1;
  if(nullable(p2) && (P_IS(p1,EMPTY))) return p2;
  return newChoice(p1,p2);
}

int rn_ileave(int p1,int p2) {
  if(P_IS(p1,NOT_ALLOWED)) return p1;
  if(P_IS(p2,NOT_ALLOWED)) return p2;
  if(P_IS(p1,EMPTY)) return p2;
  if(P_IS(p2,EMPTY)) return p1;
  return newInterleave(p1,p2);
}

int rn_after(int p1,int p2) {
  if(P_IS(p1,NOT_ALLOWED)) return p1;
  if(P_IS(p2,NOT_ALLOWED)) return p2;
  return newAfter(p1,p2);
}

#define NC_NEW(x) rn_nameclass[i_nc][0]=NC_##x

int newQName(int uri,int name) { NC_NEW(QNAME);
  rn_nameclass[i_nc][1]=uri; rn_nameclass[i_nc][2]=name;
  return accept_nc();
}

int newNsName(int uri) { NC_NEW(NSNAME);
  rn_nameclass[i_nc][1]=uri;
  return accept_nc();
}

int newAnyName(void) { NC_NEW(ANY_NAME);
  return accept_nc();
}

int newNameClassExcept(int nc1,int nc2) { NC_NEW(EXCEPT);
  rn_nameclass[i_nc][1]=nc1; rn_nameclass[i_nc][2]=nc2;
  return accept_nc();
}

int newNameClassChoice(int nc1,int nc2) { NC_NEW(CHOICE);
  rn_nameclass[i_nc][1]=nc1; rn_nameclass[i_nc][2]=nc2;
  return accept_nc();
}

int newDatatype(int lib,int typ) { NC_NEW(DATATYPE);
  rn_nameclass[i_nc][1]=lib; rn_nameclass[i_nc][2]=typ;
  return accept_nc();
}

char *nc2str(int nc) {
  char *s=NULL,*s1,*s2;
  int nc1,nc2,uri,name;
  switch(NC_TYP(nc)) {
  case NC_ERROR: s=strdup("?"); break;
  case NC_NSNAME:
    NsName(nc,uri);
    s=(char*)calloc(strlen(rn_string+uri)+3,sizeof(char));
    strcpy(s,rn_string+uri); strcat(s,":*");
    break;
  case NC_QNAME:
    QName(nc,uri,name); 
    s=(char*)calloc(strlen(rn_string+uri)+strlen(rn_string+name)+2,sizeof(char));
    strcpy(s,rn_string+uri); strcat(s,"^"); strcat(s,rn_string+name);
    break;
  case NC_ANY_NAME: s=strdup("*"); break;
  case NC_EXCEPT:
    NameClassExcept(nc,nc1,nc2);
    s1=nc2str(nc1); s2=nc2str(nc2);
    s=(char*)calloc(strlen(s1)+strlen(s2)+2,sizeof(char));
    strcpy(s,s1); strcat(s,"-"); strcat(s,s2);
    free(s1); free(s2);
    break;
  case NC_CHOICE:
    NameClassChoice(nc,nc1,nc2);
    s1=nc2str(nc1); s2=nc2str(nc2);
    s=(char*)calloc(strlen(s1)+strlen(s2)+2,sizeof(char));
    strcpy(s,s1); strcat(s,"|"); strcat(s,s2);
    free(s1); free(s2);
    break;
  case NC_DATATYPE:
    Datatype(nc,uri,name); 
    s=(char*)calloc(strlen(rn_string+uri)+strlen(rn_string+name)+2,sizeof(char));
    strcpy(s,rn_string+uri); strcat(s,"^"); strcat(s,rn_string+name);
    break;
  default: assert(0);
  }
  return s;
}

int rn_i_ps(void) {return i_ps;}
static void add_ps(char *s) {
  int len=strlen(s)+1;
  if(i_ps+len>len_ps) {
    char *newparams=(char*)calloc(len_ps*=2,sizeof(char));
    memcpy(newparams,rn_params,i_ps*sizeof(char)); free(rn_params);
    rn_params=newparams;
  }
  memcpy(rn_params+i_ps,s,(len+1)*sizeof(char));
  i_ps+=len;
}
void rn_add_pskey(char *s) {add_ps(s);}
void rn_add_psval(char *s) {add_ps(s);}
void rn_end_ps() {add_ps("");}

static int hash_p(int i);
static int hash_nc(int i);
static int hash_s(int i);

static int equal_p(int p1,int p2);
static int equal_nc(int nc1,int nc2); 
static int equal_s(int s1,int s2);

static void windup(void);

static int initialized=0;
void rn_init() {
  if(!initialized) { initialized=1;
    rn_pattern=(int (*)[P_SIZE])calloc(len_p=LEN_P,sizeof(int[P_SIZE]));
    rn_nameclass=(int (*)[NC_SIZE])calloc(len_nc=LEN_NC,sizeof(int[NC_SIZE]));
    rn_string=(char*)calloc(len_s=LEN_S,sizeof(char));
    rn_params=(char*)calloc(len_ps=LEN_PS,sizeof(char));

    ht_init(&ht_p,len_p,&hash_p,&equal_p);
    ht_init(&ht_nc,len_nc,&hash_nc,&equal_nc);
    ht_init(&ht_s,len_s,&hash_s,&equal_s);

    windup();
  }
}

void rn_clear(void) {
  ht_clear(&ht_p); ht_clear(&ht_nc); ht_clear(&ht_s);
  windup();
}

static void windup(void) {
  i_p=i_nc=i_s=i_ps=0;
  memset(rn_pattern[0],0,sizeof(int[P_SIZE]));
  memset(rn_nameclass[0],0,sizeof(int[NC_SIZE]));
  rn_pattern[0][0]=P_ERROR;  accept_p(); 
  rn_nameclass[0][0]=NC_ERROR; accept_nc();
  newString("");
  rn_empty=newEmpty(); rn_notAllowed=newNotAllowed(); rn_text=newText();
  rn_dt_string=newDatatype(0,newString("string")); rn_dt_token=newDatatype(0,newString("token"));
  rn_xsd_uri=newString("http://www.w3.org/2001/XMLSchema-datatypes");
  rn_end_ps();
}

static int hash_p(int p) {
  int *pp=rn_pattern[p];
  return (pp[0]&0xf)|((pp[1]^pp[2])<<4);
}
static int hash_nc(int nc) {
  int *ncp=rn_nameclass[nc];
  return (ncp[0]&0x7)|((ncp[1]^ncp[2])<<3);
}
static int hash_s(int i) {return strhash(rn_string+i);}

static int equal_p(int p1,int p2) {
  int *pp1=rn_pattern[p1],*pp2=rn_pattern[p2];
  return (pp1[0]&0xff)==(pp2[0]&0xff) && pp1[1] == pp2[1] && pp1[2] == pp2[2];
}
static int equal_nc(int nc1,int nc2) {
  int *ncp1=rn_nameclass[nc1],*ncp2=rn_nameclass[nc2];
  return (ncp1[0]&0xff)==(ncp2[0]&0xff) && ncp1[1] == ncp2[1] && ncp1[2] == ncp2[2];
}
static int equal_s(int s1,int s2) {return strcmp(rn_string+s1,rn_string+s2)==0;}
