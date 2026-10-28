/* SPDX-License-Identifier: GPL-3.0-or-later */
#ifndef PLAT_HOST
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
/* Word stores avoid the BSP fallback's byte-at-a-time clearing of large DSP buffers.
 * may_alias permits aligned access to float/state storage under strict aliasing. */
typedef uint32_t alias_word __attribute__((may_alias));
void *fs_memset(void *dst,int c,unsigned n){
 uint8_t *d=dst;uint32_t word=(uint8_t)c;word|=word<<8;word|=word<<16;
 while(n && ((uintptr_t)d&3)){*d++=(uint8_t)c;n--;}
 while(n>=16){alias_word *w=(alias_word *)d;w[0]=word;w[1]=word;w[2]=word;w[3]=word;d+=16;n-=16;}
 while(n>=4){*(alias_word *)d=word;d+=4;n-=4;}
 while(n--)*d++=(uint8_t)c;return dst;
}
void *fs_memcpy(void *dst,const void *src,unsigned n){
 uint8_t *d=dst;const uint8_t *s=src;
 if(!(((uintptr_t)d|(uintptr_t)s)&3)){
  while(n>=16){alias_word *w=(alias_word *)d;const alias_word *v=(const alias_word *)s;w[0]=v[0];w[1]=v[1];w[2]=v[2];w[3]=v[3];d+=16;s+=16;n-=16;}
  while(n>=4){*(alias_word *)d=*(const alias_word *)s;d+=4;s+=4;n-=4;}
 }
 while(n--)*d++=*s++;return dst;
}
size_t strlen(const char *s){size_t n=0;while(s[n])n++;return n;}
int strcmp(const char *a,const char *b){while(*a && *a==*b){a++;b++;}return (unsigned char)*a-(unsigned char)*b;}
char *strcpy(char *d,const char *s){char *p=d;while((*p++=*s++)){}return d;}
/* Only integer/string conversions: the UI never needs varargs software doubles. */
int snprintf(char *d,size_t cap,const char *fmt,...){
 va_list args;size_t used=0;va_start(args,fmt);
 #define EMIT(c) do{if(used+1<cap)d[used]=(c);used++;}while(0)
 while(*fmt){
  unsigned width=0,precision=0xffffffffu,zero=0,plus=0;char buf[40],*s;unsigned n,i;uint32_t v;int negative=0;
  if(*fmt!='%'){EMIT(*fmt++);continue;}fmt++;
  if(*fmt=='+'){plus=1;fmt++;}if(*fmt=='0'){zero=1;fmt++;}
  while(*fmt>='0'&&*fmt<='9')width=width*10+(unsigned)(*fmt++-'0');
  if(*fmt=='.'){fmt++;precision=0;while(*fmt>='0'&&*fmt<='9')precision=precision*10+(unsigned)(*fmt++-'0');}
  if(*fmt=='z'||*fmt=='l')fmt++;
  if(*fmt=='s'){s=va_arg(args,char *);if(!s)s="";for(i=0;s[i]&&i<precision;i++)EMIT(s[i]);fmt++;continue;}
  if(*fmt=='%'){EMIT('%');fmt++;continue;}
  if(*fmt!='d'&&*fmt!='u'&&*fmt!='x'){if(*fmt)EMIT(*fmt++);continue;}
  v=va_arg(args,unsigned);if(*fmt=='d' && (int32_t)v<0){negative=1;v=0u-v;}
  n=0;do{unsigned base=*fmt=='x'?16:10;buf[n++]="0123456789abcdef"[v%base];v/=base;}while(v);fmt++;
  if(negative||plus){EMIT(negative?'-':'+');if(width)width--;}
  while(width>n){EMIT(zero?'0':' ');width--;}
  while(n)EMIT(buf[--n]);
 }
 if(cap)d[used<cap?used:cap-1]=0;va_end(args);return (int)used;
 #undef EMIT
}
#endif
