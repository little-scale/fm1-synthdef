/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "performance.h"
#include "slots.h"
#include "protocol.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
static uint8_t nor[0x100000],saved[0x100000],scratch[FMS_PAYLOAD],got[FMS_PAYLOAD],old[FMS_PAYLOAD],fresh[FMS_PAYLOAD];
static int cut=-1,operations,io_fail;
static int read_flash(void *c,uint32_t off,void *dst,uint32_t n){(void)c;if(io_fail)return -1;assert(off>=FMS_FLASH_BASE && off+n<=FMS_FLASH_END);memcpy(dst,nor+off,n);return 0;}
static int erase_flash(void *c,uint32_t off){(void)c;assert(off%4096==0 && off>=FMS_FLASH_BASE && off+4096<=FMS_FLASH_END);if(operations++==cut){memset(nor+off,255,2048);return -1;}memset(nor+off,255,4096);return 0;}
static int program_flash(void *c,uint32_t off,const void *src,uint32_t n){uint32_t i;(void)c;assert(n<=256 && off/256==(off+n-1)/256);assert(off>=FMS_FLASH_BASE && off+n<=FMS_FLASH_END);if(operations++==cut){for(i=0;i<n/2;i++)nor[off+i]&=((const uint8_t *)src)[i];return -1;}for(i=0;i<n;i++){assert((nor[off+i]&((const uint8_t *)src)[i])==((const uint8_t *)src)[i]);nor[off+i]&=((const uint8_t *)src)[i];}return 0;}
static void storage_test(void){
 fms_flash f={0,read_flash,erase_flash,program_flash};uint32_t n,g;int op,total;unsigned i;
 memset(nor,255,sizeof nor);for(i=0;i<FMS_PAYLOAD;i++){old[i]=(uint8_t)i;fresh[i]=(uint8_t)(i*17+3);}
 assert(fms_slot_write(&f,0,old,sizeof old,0,scratch)==0);
 assert(fms_slot_write(&f,7,old,5,0,scratch)==0);
 memcpy(saved,nor,sizeof nor);operations=0;
 assert(fms_slot_write(&f,0,fresh,sizeof fresh,1,scratch)==0);total=operations;
 for(op=0;op<total;op++){
  memcpy(nor,saved,sizeof nor);cut=op;operations=0;
  assert(fms_slot_write(&f,0,fresh,sizeof fresh,1,scratch)<0);cut=-1;
  assert(fms_slot_read(&f,0,got,&n,&g)==0 && n==sizeof old && g==1 && !memcmp(got,old,n));
  assert(fms_slot_read(&f,7,got,&n,&g)==0 && n==5 && g==1 && !memcmp(got,old,n));
 }
 memcpy(nor,saved,sizeof nor);assert(fms_slot_write(&f,0,fresh,100,0,scratch)==-2);
 assert(fms_slot_write(&f,9,fresh,100,0,scratch)==-1);
 assert(fms_slot_write(&f,0,fresh,FMS_PAYLOAD+1,1,scratch)==-1);
 assert(fms_slot_write(&f,0,0,0,1,scratch)==0);
 assert(fms_slot_read(&f,0,got,&n,&g)==0 && n==0 && g==2);
 io_fail=1;assert(fms_slot_write(&f,0,fresh,2,2,scratch)==-1);io_fail=0;
 assert(nor[0xDC000]==255 && nor[0xE0000]==255 && nor[0xFC000]==255);
 printf("Slot journal: %d torn erase/program boundaries, delete, conflict, I/O failure and region guards passed\n",total);
}
static fms_link link;
static void effect_storage_test(void){
 fms_flash f={0,read_flash,erase_flash,program_flash};uint32_t n,g;int total,op;
 memset(nor,255,sizeof nor);nor[0xDB000]=0x35;nor[0xDC000]=0x73;
 assert(!fms_slot_write(&f,7,old,10,0,scratch));
 assert(!fms_slot_write(&f,FMS_FX_SLOT,old,FMS_FX_PAYLOAD,0,scratch));
 memcpy(saved,nor,sizeof nor);operations=0;
 assert(!fms_slot_write(&f,FMS_FX_SLOT,fresh,FMS_FX_PAYLOAD,1,scratch));total=operations;
 for(op=0;op<total;op++){
  memcpy(nor,saved,sizeof nor);cut=op;operations=0;
  assert(fms_slot_write(&f,FMS_FX_SLOT,fresh,FMS_FX_PAYLOAD,1,scratch)<0);cut=-1;
  assert(!fms_slot_read(&f,FMS_FX_SLOT,got,&n,&g) && n==FMS_FX_PAYLOAD && g==1 && !memcmp(got,old,n));
  assert(!fms_slot_read(&f,7,got,&n,&g) && n==10 && !memcmp(got,old,n));
  assert(nor[0xDB000]==0x35 && nor[0xDC000]==0x73);
 }
 assert(fms_slot_write(&f,FMS_FX_SLOT,fresh,FMS_FX_PAYLOAD+1,1,scratch)<0);
 printf("Effect journal: %d interrupted writes preserve slot 0, slot 8 and reserved flash\n",total);
}
static uint32_t request(unsigned cmd,const uint8_t *body,unsigned len,uint8_t *reply){
 static unsigned seq;uint8_t in[FMS_FRAME];uint32_t crc;unsigned i;
 in[0]=1;in[1]=(uint8_t)cmd;in[2]=(uint8_t)++seq;in[3]=(uint8_t)(seq>>8);in[4]=(uint8_t)len;in[5]=(uint8_t)(len>>8);if(len)memcpy(in+6,body,len);crc=fms_crc(in,6+len);for(i=0;i<4;i++)in[6+len+i]=(uint8_t)(crc>>(8*i));return fms_request(&link,in,len+10,reply);
}
static int valid_package(unsigned slot,const void *data,uint32_t n){(void)slot;return n==5 && !memcmp(data,"valid",5);}
static void protocol_test(void){
 uint8_t p[32]={0},reply[FMS_FRAME],cached[FMS_FRAME];uint32_t crc,n,g;unsigned i;
 memset(nor,255,sizeof nor);memset(&link,0,sizeof link);link.flash=(fms_flash){0,read_flash,erase_flash,program_flash};link.validate=valid_package;
 assert(request(FMS_INFO,0,0,reply)==30 && reply[6]==FMS_OK && reply[8]==9);
 p[5]=5;p[13]=42;crc=fms_crc("valid",5);for(i=0;i<4;i++)p[9+i]=(uint8_t)(crc>>(8*i));
 assert(request(FMS_BEGIN,p,17,reply)>0 && reply[6]==FMS_OK);
 memset(p,0,sizeof p);p[0]=42;p[4]=1;memcpy(p+8,"valid",5);
 request(FMS_WRITE,p,13,reply);assert(reply[6]==FMS_BAD);
 p[4]=0;request(FMS_WRITE,p,13,reply);assert(reply[6]==FMS_OK);
 request(FMS_WRITE,p,13,reply);assert(reply[6]==FMS_OK && link.received==5);
 p[8]='X';request(FMS_WRITE,p,13,reply);assert(reply[6]==FMS_CONFLICT);
 n=request(FMS_COMMIT,p,4,reply);assert(reply[6]==FMS_OK);
 assert(fms_request(&link,link.last_request,link.last_request_len,cached)==n && !memcmp(reply,cached,n));
 assert(fms_slot_read(&link.flash,0,got,&n,&g)==0 && g==1 && n==5 && !memcmp(got,"valid",5));
 memset(p,0,sizeof p);p[5]=5;memset(p+7,255,4);request(FMS_READ,p,11,reply);assert(reply[6]==FMS_OK && !memcmp(reply+19,"valid",5));
 memset(p+7,0,4);request(FMS_READ,p,11,reply);assert(reply[6]==FMS_CONFLICT);
 link.last_request[0]^=2;assert(fms_request(&link,link.last_request,link.last_request_len,reply)==0);
 puts("USB transactions: capabilities, chunk bounds/order, retry, conflict, commit and readback passed");
}
static fm_graph graph;static fms_performance player;
static void load(const char *path){FILE *f=fopen(path,"rb");uint8_t data[FM_FILE_BYTES];size_t n;char err[256];assert(f);n=fread(data,1,sizeof data,f);fclose(f);assert(fm_load(&graph,data,n,err,sizeof err));assert(fms_init(&player,&graph));}
static float control(fm_voice *v,const char *name){unsigned i;for(i=0;i<graph.names;i++)if(!strcmp(name,graph.control_name[i].name))return v->control[graph.control_name[i].index];assert(0);return 0;}
static void performance_test(void){
 float out[FM_BLOCK*2],energy=0;unsigned i,j;
 load("../synthdef-engine/fixtures/m3_subtractive_voice.scsyndef");
 fms_key(&player,16,64,1);assert(fabsf(control(&player.voice[0].synth,"freq")-440)<0.001f);assert(fabsf(control(&player.voice[0].synth,"amp")-64/127.0f)<1e-6f);
 fms_octave(&player,1);fms_key(&player,16,0,0);assert(!player.voice[0].held && control(&player.voice[0].synth,"gate")==0);
 fms_key(&player,16,127,1);assert(fabsf(control(&player.voice[1].synth,"freq")-880)<0.001f);
 fms_note_on(&player,2,60,100);assert(player.steals==1 && player.voice[0].id==2);
 fms_note_off(&player,0x10000u+16);assert(player.voice[0].held && !player.voice[1].held);
 for(i=0;i<20;i++){fms_render(&player,out);for(j=0;j<128;j++){assert(isfinite(out[j]) && fabsf(out[j])<=1);energy+=out[j]*out[j];}}
 assert(energy>0 && player.peak[0]>0 && player.peak[1]>0);
 fms_note_on(&player,2,60,0);assert(!player.voice[0].held);
 fms_knob(&player,0,2);assert(player.knobs[0]==1);fms_knob(&player,0,-1);assert(player.knobs[0]==0);
 for(i=0;i<10;i++)fms_octave(&player,1);assert(player.octave==2);
 fms_panic(&player);fms_render(&player,out);for(j=0;j<128;j++)assert(out[j]==0);
 load("../synthdef-engine/fixtures/tone.scsyndef");fms_note_on(&player,42,69,127);for(i=0;i<4;i++)fms_render(&player,out);fms_note_off(&player,42);for(i=0;i<3;i++)fms_render(&player,out);assert(!player.voice[0].synth.active);
 puts("Performance: velocity, frequency, octave-held note-off, stealing, identity, meters, no-gate release and panic passed");
}
static void live_controls_test(void){
 float out[128];unsigned i;
 load("examples/controls.scsyndef");player.master=1;player.gain=1;
 fms_note_on(&player,1,60,64);
 assert(control(&player.voice[0].synth,"midinote")==60 && control(&player.voice[0].synth,"pitch")==60 && control(&player.voice[0].synth,"note")==0);
 assert(fabsf(control(&player.voice[0].synth,"velocity")-64/127.0f)<1e-6f);
 for(i=0;i<3;i++)fms_render(&player,out);assert(fabsf(out[126]-0.25f)<1e-6f && fabsf(out[127]-0.5f)<1e-6f);
 fms_knob(&player,0,0.75f);fms_render(&player,out);fms_render(&player,out);
 assert(fabsf(out[126]-0.75f)<1e-6f && fabsf(out[127]-0.5f)<1e-6f); /* already sounding note */
 fms_note_on(&player,2,69,127);assert(control(&player.voice[1].synth,"knob1")==0.75f);
 for(i=0;i<3;i++)fms_render(&player,out);assert(out[126]==1 && player.clip[0]>0 && player.peak[0]>1);
 fms_knob(&player,0,0.1f);fms_render(&player,out);fms_render(&player,out);assert(fabsf(out[126]-0.2f)<1e-6f);
 puts("Live controls: pitch aliases, normalized velocity, active/future voice knobs, independent stereo output and clipping passed");
}
static void mono_test(void){
 unsigned i,active;float out[128];load("examples/controls.scsyndef");player.voice_limit=1;
 for(i=1;i<=20;i++){
  fms_note_on(&player,i,60+i%12,90);fms_render(&player,out);
  active=player.voice[0].synth.active+player.voice[1].synth.active;assert(active==1);
  if(i>1)fms_note_off(&player,i-1);
  assert(player.voice[0].held+player.voice[1].held==1);
 }
 fms_note_off(&player,20);assert(!player.voice[0].held && !player.voice[1].held);
 puts("Mono limit: repeated stealing and old note-offs preserve the current note");
}
static void preparation_test(void){
 float out[128];unsigned i,j;load("examples/controls.scsyndef");player.prepare_blocks=2;player.master=player.gain=1;
 fms_note_on(&player,1,60,90);fms_note_on(&player,2,67,90);
 for(i=0;i<2;i++){fms_render(&player,out);for(j=0;j<128;j++)assert(out[j]==0);}
 for(i=0;i<2;i++)assert(player.voice[i].synth.prepared && player.voice[i].synth.blocks==0);
 for(i=0;i<3;i++)fms_render(&player,out);
 assert(fabsf(out[126]-0.5f)<1e-6f && fabsf(out[127]-1)<1e-6f && !player.faults);
 fms_note_on(&player,3,72,90);fms_render(&player,out);assert(out[0]>0 && player.steals==1);
 fms_note_off(&player,1);assert(player.voice[0].held && player.voice[1].held);
 puts("Prepared polyphony: simultaneous startup, silent preparation, two audible voices and stolen note-off passed");
}
static fm_graph fxgraph;static fm_voice fxvoice;
static void effect_test(void){
 FILE *f=fopen("examples/master-echo.scsyndef","rb");uint8_t data[FM_FILE_BYTES];size_t n;char err[100];float out[128],energy=0;unsigned i,j;uint32_t blocks;
 assert(f);n=fread(data,1,sizeof data,f);fclose(f);assert(fm_load(&fxgraph,data,n,err,sizeof err) && fms_effect_valid(&fxgraph));
 load("examples/keys-master.scsyndef");assert(fms_attach_effect(&player,&fxgraph,&fxvoice));assert(fabsf(player.knobs[2]-.25f)<1e-6f);assert(fms_reset_effect(&player));
 player.master=player.gain=1;fms_knob(&player,2,1);fms_knob(&player,3,0);fms_note_on(&player,1,69,100);
 for(i=0;i<200;i++)fms_render(&player,out);
 fms_panic(&player);blocks=fxvoice.blocks;
 for(i=0;i<40;i++){fms_render(&player,out);for(j=0;j<128;j++)energy+=out[j]*out[j];}
 assert(energy>1e-5f && fxvoice.blocks==blocks+40 && fxvoice.active && !player.faults);
 /* Choosing another synth preserves the existing effect and its delay tail. */
 blocks=fxvoice.blocks;assert(fms_init(&player,&graph));assert(fms_attach_effect(&player,&fxgraph,&fxvoice));assert(player.knobs[2]==1);
 player.master=player.gain=1;fms_render(&player,out);assert(fxvoice.blocks==blocks+1);
 assert(fms_reset_effect(&player));
 for(i=0;i<100;i++){fms_render(&player,out);for(j=0;j<128;j++)assert(out[j]==0);}
 assert(fms_attach_effect(&player,0,0) && !player.effect_graph);
 puts("Master effect: routing, inherited knob defaults, live controls, note-free tails, preset continuity, reset and bypass passed");
}
int main(void){storage_test();effect_storage_test();protocol_test();performance_test();live_controls_test();mono_test();preparation_test();effect_test();return 0;}
