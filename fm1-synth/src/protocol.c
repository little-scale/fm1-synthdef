/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "protocol.h"
#include <string.h>
#ifndef FMS_PROTOCOL_VOICES
#define FMS_PROTOCOL_VOICES 2
#endif
static uint32_t get32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static uint32_t get16(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8;}
static void put32(uint8_t *p,uint32_t x){unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(x>>(8*i));}
static uint32_t dispatch(fms_link *l,unsigned cmd,const uint8_t *p,uint32_t n,uint8_t *out){
 uint32_t len,gen,off,amount,tx;int rc;out[0]=FMS_BAD;
 if(cmd==FMS_INFO && !n){out[0]=FMS_OK;out[1]=2;out[2]=FMS_SLOTS;out[3]=FMS_PROTOCOL_VOICES;put32(out+4,FMS_PAYLOAD);put32(out+8,FMS_CHUNK);put32(out+12,16384u*4);put32(out+16,FMS_FX_PAYLOAD);return 20;}
 if(cmd==FMS_READ && n==11 && p[0]<FMS_SLOTS){
  off=get32(p+1);amount=get16(p+5);gen=get32(p+7);
  if(amount>FMS_CHUNK)return 1;
  if(fms_slot_read(&l->flash,p[0],l->scratch,&len,&tx)){out[0]=FMS_IO;return 1;}
  if(gen!=UINT32_MAX && tx!=gen){out[0]=FMS_CONFLICT;return 1;}
  if(off>len)return 1;if(amount>len-off)amount=len-off;
  out[0]=FMS_OK;put32(out+1,tx);put32(out+5,len);put32(out+9,off);memcpy(out+13,l->scratch+off,amount);return 13+amount;
 }
 if(cmd==FMS_BEGIN && n==17 && p[0]<FMS_SLOTS){
  amount=get32(p+5);if(amount>fms_slot_capacity(p[0]))return 1;
  if(fms_slot_read(&l->flash,p[0],l->scratch,&len,&gen)){out[0]=FMS_IO;return 1;}
  if(gen!=get32(p+1)){out[0]=FMS_CONFLICT;return 1;}
  l->slot=p[0];l->expected=gen;l->length=amount;l->crc=get32(p+9);l->transaction=get32(p+13);l->received=0;l->active=1;out[0]=FMS_OK;return 1;
 }
 if(cmd==FMS_ABORT && n==4){if(l->active && get32(p)!=l->transaction){out[0]=FMS_CONFLICT;return 1;}l->active=0;out[0]=FMS_OK;return 1;}
 if(cmd==FMS_WRITE && n>=8 && n<=8+FMS_CHUNK){
  if(!l->active || get32(p)!=l->transaction){out[0]=FMS_ORDER;return 1;}
  off=get32(p+4);amount=n-8;
  if(off>l->length || amount>l->length-off)return 1;
  if(off<l->received){if(amount>l->received-off || memcmp(l->stage+off,p+8,amount)){out[0]=FMS_CONFLICT;return 1;}}
  else if(off!=l->received){out[0]=FMS_ORDER;return 1;}
  else{memcpy(l->stage+off,p+8,amount);l->received+=amount;}
  out[0]=FMS_OK;put32(out+1,l->received);return 5;
 }
 if(cmd==FMS_COMMIT && n==4){
  if(!l->active || get32(p)!=l->transaction || l->received!=l->length){out[0]=FMS_ORDER;return 1;}
  if(fms_crc(l->stage,l->length)!=l->crc || (l->length && (!l->validate || !l->validate(l->slot,l->stage,l->length)))){out[0]=FMS_INVALID;l->active=0;return 1;}
  rc=fms_slot_write(&l->flash,l->slot,l->stage,l->length,l->expected,l->scratch);
  l->active=0;out[0]=rc==-2?FMS_CONFLICT:rc?FMS_IO:FMS_OK;if(!rc){put32(out+1,l->expected+1);return 5;}return 1;
 }
 return 1;
}
uint32_t fms_request(fms_link *l,const uint8_t *in,uint32_t n,uint8_t *out){
 uint32_t size,total;
 if(n<10 || n>FMS_FRAME || in[0]!=1 || in[1]&0x80 || get16(in+4)!=n-10 || get32(in+n-4)!=fms_crc(in,n-4))return 0;
 if(n==l->last_request_len && !memcmp(in,l->last_request,n)){memcpy(out,l->last_reply,l->last_reply_len);return l->last_reply_len;}
 size=dispatch(l,in[1],in+6,n-10,out+6);out[0]=1;out[1]=in[1]|0x80;out[2]=in[2];out[3]=in[3];out[4]=(uint8_t)size;out[5]=(uint8_t)(size>>8);put32(out+6+size,fms_crc(out,6+size));total=10+size;
 memcpy(l->last_request,in,n);l->last_request_len=n;memcpy(l->last_reply,out,total);l->last_reply_len=total;return total;
}
