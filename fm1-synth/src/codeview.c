/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "codeview.h"
#include <ctype.h>
#include <string.h>
static int word(unsigned char c){return isalnum(c) || c=='_';}
unsigned fms_code(const char *s,uint32_t len,unsigned scroll,unsigned rows,unsigned columns,fms_glyph emit,void *context){
 uint32_t i=0,start;unsigned row=0,col=0,kind,j,count;if(!columns)return 0;
 while(i<len){
  start=i;kind=FMS_TEXT;
  if(i+1<len && s[i]=='/' && s[i+1]=='/'){kind=FMS_COMMENT;while(i<len && s[i]!='\n')i++;}
  else if(i+1<len && s[i]=='/' && s[i+1]=='*'){
   unsigned depth=1;kind=FMS_COMMENT;i+=2;
   while(i<len && depth){if(i+1<len && s[i]=='/' && s[i+1]=='*'){depth++;i+=2;}else if(i+1<len && s[i]=='*' && s[i+1]=='/'){depth--;i+=2;}else i++;}
  }else if(s[i]=='"' || s[i]=='\''){
   char quote=s[i++];kind=FMS_STRING;while(i<len){char c=s[i++];if(c=='\\' && i<len)i++;else if(c==quote)break;}
  }else if(s[i]=='\\' && i+1<len && word((unsigned char)s[i+1])){kind=FMS_STRING;i++;while(i<len && word((unsigned char)s[i]))i++;}
  else if(isdigit((unsigned char)s[i]) || (s[i]=='.' && i+1<len && isdigit((unsigned char)s[i+1]))){
   kind=FMS_NUMBER;while(i<len && isdigit((unsigned char)s[i]))i++;
   if(i+1<len && s[i]=='.' && isdigit((unsigned char)s[i+1])){i++;while(i<len && isdigit((unsigned char)s[i]))i++;}
   if(i<len && (s[i]=='r' || s[i]=='x')){i++;while(i<len && isalnum((unsigned char)s[i]))i++;}
   else if(i<len && (s[i]=='e' || s[i]=='E')){i++;if(i<len && (s[i]=='-' || s[i]=='+'))i++;while(i<len && isdigit((unsigned char)s[i]))i++;}
  }else if(isalpha((unsigned char)s[i]) || s[i]=='_'){
   kind=isupper((unsigned char)s[i])?FMS_CLASS:FMS_TEXT;while(i<len && word((unsigned char)s[i]))i++;
   if(i-start==3 && (!memcmp(s+start,"arg",3) || !memcmp(s+start,"var",3)))kind=FMS_KEYWORD;
  }else i++;
  for(;start<i;start++){
   unsigned char c=(unsigned char)s[start];if(c=='\r')continue;if(c=='\n'){row++;col=0;continue;}
   if(c>=128 && (c&0xc0)==0x80)continue; /* one replacement glyph per non-ASCII codepoint */
   count=c=='\t'?4-col%4:1;
   for(j=0;j<count;j++){if(col==columns){row++;col=0;}if(row>=scroll && row-scroll<rows && emit)emit(context,row-scroll,col,c=='\t'?' ':c>=128?'?':(char)c,kind);col++;}
  }
 }
 return row+1;
}
