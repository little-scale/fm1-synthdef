/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "slots.h"
#include <string.h>
static uint32_t get(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
static void put(uint8_t *p,uint32_t x){unsigned i;for(i=0;i<4;i++)p[i]=(uint8_t)(x>>(8*i));}
uint32_t fms_crc(const void *data,uint32_t n){const uint8_t *p=data;uint32_t c=~0u;unsigned j;while(n--){c^=*p++;for(j=0;j<8;j++)c=(c>>1)^(0xedb88320u & (0u-(c&1)));}return ~c;}
static uint32_t base(unsigned slot,unsigned bank){return slot==FMS_FX_SLOT?FMS_FX_BASE+bank*FMS_FX_BANK:FMS_FLASH_BASE+(slot*2+bank)*FMS_BANK;}
/* Returns valid bank generation; CRC checks always include body, even for delete. */
static int read_bank(const fms_flash *f,unsigned slot,unsigned bank,void *dst,uint32_t *len,uint32_t *gen){
 uint8_t h[32];uint32_t off=base(slot,bank),n;
 if(f->read(f->context,off,h,32))return -2;
 if(memcmp(h,"FMS1",4) || get(h+4)!=slot || get(h+24)!=1 || get(h+28)!=fms_crc(h,28))return -1;
 n=get(h+12);if(n>fms_slot_capacity(slot) || !get(h+8) || get(h+8)==UINT32_MAX)return -1;
 if(f->read(f->context,off+256,dst,n))return -2;
 if(fms_crc(dst,n)!=get(h+16))return -1;
 *len=n;*gen=get(h+8);return 0;
}
static int current(const fms_flash *f,unsigned slot,void *dst,uint32_t *len,uint32_t *gen){
 uint32_t a=0,b=0,na=0,nb=0;int va,vb;
 va=read_bank(f,slot,0,dst,&na,&a);vb=read_bank(f,slot,1,dst,&nb,&b);
 if(va==-2 || vb==-2)return -2;
 va=va==0;vb=vb==0;
 if(va && (!vb || a>=b)){if(read_bank(f,slot,0,dst,len,gen))return -1;return 0;}
 if(vb){*len=nb;*gen=b;return 1;}
 *len=0;*gen=0;return -1;
}
int fms_slot_read(const fms_flash *f,unsigned slot,void *dst,uint32_t *len,uint32_t *gen){
 if(slot>=FMS_SLOTS || !f || !dst || !len || !gen)return -1;
 return current(f,slot,dst,len,gen)==-2?-1:0;
}
int fms_slot_write(const fms_flash *f,unsigned slot,const void *src,uint32_t len,uint32_t expected,void *scratch){
 uint8_t h[32];uint32_t n,gen,off,i;int bank;
 if(!f || slot>=FMS_SLOTS || len>fms_slot_capacity(slot) || (len && !src) || !scratch || src==scratch)return -1;
 bank=current(f,slot,scratch,&n,&gen);
 if(bank==-2)return -1;
 if(expected!=gen)return -2;
 if(gen>=UINT32_MAX-1)return -1;
 off=base(slot,bank==0?1:0);
 for(i=0;i<(slot==FMS_FX_SLOT?FMS_FX_BANK:FMS_BANK);i+=4096)if(f->erase(f->context,off+i))return -1;
 for(i=0;i<len;i+=256){n=len-i>256?256:len-i;if(f->program(f->context,off+256+i,(const uint8_t *)src+i,n))return -1;}
 memset(h,0,sizeof h);memcpy(h,"FMS1",4);put(h+4,slot);put(h+8,gen+1);put(h+12,len);put(h+16,fms_crc(src,len));put(h+24,1);put(h+28,fms_crc(h,28));
 if(f->program(f->context,off,h,sizeof h))return -1;
 if(read_bank(f,slot,bank==0?1:0,scratch,&n,&i) || n!=len || i!=gen+1 || (len && memcmp(src,scratch,len)))return -1;
 return 0;
}
