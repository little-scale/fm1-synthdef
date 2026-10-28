/* SPDX-License-Identifier: GPL-3.0-or-later
 * Panel events cross a bounded queue; graph/storage work suspends audio first.
 */
#include "performance.h"
#include "slots.h"
#include "codeview.h"
#include "protocol.h"
#include <stdio.h>
#ifdef PLAT_HOST
#include <stdlib.h>
#endif
#include <string.h>
#include <math.h>
static fms_performance instrument PLAT_POOL;
static fm_graph synth_graph PLAT_POOL;
/* The effect fits ordinary RAM; both voices and its delay lines share the pool. */
static fm_graph effect_graph;
static fm_voice effect_voice;
static int effect_ready,effect_reset_pending;
static uint8_t package[FMS_PAYLOAD+1] PLAT_POOL;
static uint32_t held_keys,held_buttons,overlay_until,last_frame,source_len;
static unsigned slot,view_slot,scroll_line,total_lines=1;
static int overlay=-1,loaded;
static const char *source="// Empty slot";
static char title[64]="Empty",notice[64];
static volatile unsigned paused,panic_pending,qread,qwrite;
static volatile uint32_t overloads,render_peak_us,render_last_us,start_peak_us,steady_peak_us;
static uint32_t microseconds(void){
#ifdef PLAT_HOST
 return plat_ms()*1000u;
#else
 return fm1_ticks()/FM1_TICKS_PER_US;
#endif
}
typedef struct {unsigned kind,key,value;} panel_event;
static panel_event queue[64];
static float ui_knobs[4];
static fms_link link PLAT_POOL;
static int flash_read(void *context,uint32_t off,void *dst,uint32_t n){
 (void)context;if(off<0x97000u || off>FMS_FLASH_END || n>FMS_FLASH_END-off)return -1;
#ifdef PLAT_HOST
 const char *path=getenv("FMS_BANK");FILE *f;if(!path)return -1;
 f=fopen(path,"rb");if(!f)return -1;if(fseek(f,(long)off,SEEK_SET) || fread(dst,1,n,f)!=n){fclose(f);return -1;}fclose(f);return 0;
#else
 const uint8_t *p=plat_data(off);if(!p)return -1;memcpy(dst,p,n);return 0;
#endif
}
static int flash_erase(void *ctx,uint32_t off){(void)ctx;if(off<0x97000u || off>=FMS_FLASH_END)return -1;return plat_data_erase(off);}
static int flash_write(void *ctx,uint32_t off,const void *src,uint32_t n){(void)ctx;if(off<0x97000u || off>FMS_FLASH_END || n>FMS_FLASH_END-off)return -1;return plat_data_write(off,src,n);}
static void suspend_audio(void){uint32_t f=plat_lock();paused=1;qread=qwrite=0;panic_pending=0;plat_unlock(f);fms_panic(&instrument);}
static void resume_audio(void){uint32_t f=plat_lock();paused=0;plat_unlock(f);}
static void event(unsigned kind,unsigned key,unsigned value){
 uint32_t f=plat_lock();unsigned next=(qwrite+1)%64;
 if(next==qread)panic_pending=1;else{queue[qwrite]=(panel_event){kind,key,value};qwrite=next;}
 plat_unlock(f);
}
static uint32_t little32(const uint8_t *p){return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24;}
void app_panic(void){panic_pending=1;held_keys=0;}
void app_overload(void){fms_panic(&instrument);effect_voice.active=0;overloads++;}
static int package_graph(fm_graph *graph,const void *ptr,uint32_t n){
 const uint8_t *p=ptr;uint32_t m,s,b;char err[100];
 if(n<20 || memcmp(p,"FMB1",4))return 0;m=little32(p+4);s=little32(p+8);b=little32(p+12);
 if(m>n-20 || s>n-20-m || b!=n-20-m-s || little32(p+16)!=fms_crc(p+20,n-20))return 0;
 return fm_load(graph,p+20+m+s,b,err,sizeof err);
}
static void load_effect(void){
 fms_flash flash={0,flash_read,flash_erase,flash_write};uint32_t n,g;
 effect_ready=effect_reset_pending=0;effect_voice.active=0;
 if(!fms_slot_read(&flash,FMS_FX_SLOT,package,&n,&g) && n && package_graph(&effect_graph,package,n) && fms_effect_valid(&effect_graph))effect_ready=effect_reset_pending=1;
}
static void choose(unsigned index){
 fms_flash flash={0,flash_read,flash_erase,flash_write};uint32_t n,g,m,s,b;char err[256];int octave=instrument.octave;
 suspend_audio();instrument.graph=0;instrument.voice_delay_samples=0;slot=index%8;view_slot=slot;loaded=0;scroll_line=0;overlay=-1;source="// Empty slot";source_len=(uint32_t)strlen(source);strcpy(title,"Empty");notice[0]=0;
 if(fms_slot_read(&flash,slot,package,&n,&g)){strcpy(title,"Bank unavailable");goto done;}
 if(!n)goto done;
 if(n<20 || memcmp(package,"FMB1",4))goto invalid;
 m=little32(package+4);s=little32(package+8);b=little32(package+12);
 if(m>n-20 || s>n-20-m || b!=n-20-m-s || little32(package+16)!=fms_crc(package+20,n-20))goto invalid;
 if(!fm_load(&synth_graph,package+20+m+s,b,err,sizeof err) || !fms_init(&instrument,&synth_graph))goto invalid;
#ifndef PLAT_HOST
 instrument.prepare_blocks=2;
#endif
 loaded=1;instrument.octave=octave;strcpy(title,synth_graph.name);
 if(s){source=(char *)package+20+m;source_len=s;}else{source="// Source unavailable\n// Binary SynthDef loaded";source_len=(uint32_t)strlen(source);}
 goto done;
 invalid:strcpy(title,"Invalid patch");
 done:
 if(!loaded){instrument.graph=0;instrument.voice_delay_samples=0;}
 if(effect_ready){
  if(!fms_attach_effect(&instrument,&effect_graph,&effect_voice)){
   loaded=0;instrument.graph=0;instrument.voice_delay_samples=0;strcpy(title,"FX memory conflict");
   fms_attach_effect(&instrument,&effect_graph,&effect_voice);
  }
  if(effect_reset_pending){fms_reset_effect(&instrument);effect_reset_pending=0;}
 }else fms_attach_effect(&instrument,0,0);
 total_lines=fms_code(source,source_len,0,0,38,0,0);memcpy(ui_knobs,instrument.knobs,sizeof ui_knobs);resume_audio();
}
/* Browsing code is separate from selecting a playable synth. Audio uses only
 * the parsed graphs, so replacing this UI package does not touch either voice. */
static void view_code(unsigned index){
 fms_flash flash={0,flash_read,flash_erase,flash_write};uint32_t n,g,m,s,b;
 view_slot=index;scroll_line=0;overlay=-1;
 source=index==FMS_FX_SLOT?"// Slot 0 bypassed\n// No effect loaded":"// Empty slot";
 source_len=(uint32_t)strlen(source);
 strcpy(title,index==FMS_FX_SLOT?(effect_ready?effect_graph.name:"Bypassed"):(loaded?synth_graph.name:"Empty"));
 if(fms_slot_read(&flash,index,package,&n,&g) || n<20 || memcmp(package,"FMB1",4))goto done;
 m=little32(package+4);s=little32(package+8);b=little32(package+12);
 if(m>n-20 || s>n-20-m || b!=n-20-m-s || little32(package+16)!=fms_crc(package+20,n-20))goto done;
 if(s){source=(char *)package+20+m;source_len=s;}
 else{source="// Source unavailable\n// Binary SynthDef loaded";source_len=(uint32_t)strlen(source);}
 done:total_lines=fms_code(source,source_len,0,0,38,0,0);
}

void app_init(void){held_keys=held_buttons=0;overlay_until=last_frame=0;memset(&instrument,0,sizeof instrument);memset(&link,0,sizeof link);link.flash=(fms_flash){0,flash_read,flash_erase,flash_write};load_effect();choose(0);}
static uint32_t midi_id(unsigned port,unsigned channel,unsigned key){return 0x20000u+(port<<12)+(channel<<8)+key;}
void app_render(int32_t *lr,uint32_t n,uint32_t master_q12){
 uint32_t packet,j,count=0,base,t0=microseconds(),serial=instrument.serial,elapsed;int port;float out[FM_BLOCK*2];
 if(paused){memset(lr,0,n*2*sizeof *lr);while(count++<32 && plat_midi_in(&packet,&port)){}return;}
 if(panic_pending){fms_panic(&instrument);fms_reset_effect(&instrument);qread=qwrite;panic_pending=0;}
 while(qread!=qwrite){panel_event e=queue[qread];qread=(qread+1)%64;
  if(e.kind==0 && loaded)fms_key(&instrument,e.key,e.value&127,e.value>>7);
  else if(e.kind==1)fms_octave(&instrument,e.value?1:-1);
  else if(e.kind==2)fms_knob(&instrument,e.key,(float)e.value/100);
 }
 while(count++<32 && plat_midi_in(&packet,&port)){
  unsigned status=packet>>8&255,key=packet>>16&127,velocity=packet>>24&127;uint32_t id=midi_id((unsigned)port,status&15,key);
  if((status&240)==144){if(loaded)fms_note_on(&instrument,id,key,velocity);}
  else if((status&240)==128)fms_note_off(&instrument,id);
  else if((status&240)==176 && (key==120 || key==123))fms_panic(&instrument);
 }
 instrument.master=(float)master_q12/4096;
 if(n%FM_BLOCK || (!loaded && !effect_ready)){memset(lr,0,n*2*sizeof *lr);return;}
 for(base=0;base<n;base+=FM_BLOCK){fms_render(&instrument,out);for(j=0;j<FM_BLOCK*2;j++)lr[base*2+j]=(int32_t)(out[j]*8388607.0f);}
 elapsed=microseconds()-t0;render_last_us=elapsed;if(elapsed>render_peak_us)render_peak_us=elapsed;
 if(instrument.serial!=serial){if(elapsed>start_peak_us)start_peak_us=elapsed;}else if(elapsed>steady_peak_us)steady_peak_us=elapsed;
}
static void inputs(void){
 unsigned k;uint32_t keys,buttons=plat_buttons(),pressed=buttons&~held_buttons;int e;
 touch_poll();keys=touch_keys();
 if(pressed&(1u<<B_OCTUP))event(1,0,1);if(pressed&(1u<<B_OCTDN))event(1,0,0);
 if(pressed&(1u<<B_HOME)){app_panic();held_keys=keys;}
 if((e=plat_enc(EN_PRESET))){int pick=((int)view_slot+e)%9;if(pick<0)pick+=9;if((unsigned)pick==FMS_FX_SLOT || (unsigned)pick==slot)view_code((unsigned)pick);else{choose((unsigned)pick);held_keys=keys;}}
 for(k=0;k<NKEYS;k++){uint32_t bit=1u<<k;if((keys&bit) && !(held_keys&bit)){if(loaded)event(0,k,128u+touch_vel(k));}else if(!(keys&bit) && held_keys&bit)event(0,k,0);}
 if((e=plat_enc(EN_SELECT))){int next=(int)scroll_line+e,max=total_lines>15?(int)total_lines-15:0;scroll_line=(unsigned)clamp(next,0,max);}
 for(k=0;k<4;k++)if((e=plat_enc(EN_K1+(int)k))){unsigned v=(unsigned)clamp((int)(ui_knobs[k]*100+0.5f)+2*e,0,100);ui_knobs[k]=(float)v/100;event(2,k,v);overlay=(int)k;overlay_until=plat_ms()+1200;}
 held_keys=keys;held_buttons=buttons;plat_glow(1);plat_leds(buttons,keys);
}
void app_input_only(void){inputs();}
static unsigned draw_row;
static void code_glyph(void *ctx,unsigned row,unsigned column,char character,unsigned color){
 static const uint16_t colors[]={0x0000,RGB(191,0,0),RGB(0,0,191),RGB(128,0,128),RGB(0,128,0),RGB(115,115,115)};
 char text[2]={character,0};(void)ctx;if(row==draw_row)cv_text(6+(int)column*6,0,&FONT_XS,text,colors[color]);
}
static void draw(void){
 char text[64];unsigned row,k;
 cv_begin(240,24,C_WHITE);
 if(view_slot==FMS_FX_SLOT){
  snprintf(text,sizeof text,"00 FX %.26s",title);cv_text(6,0,&FONT_XS,text,C_BLACK);
  snprintf(text,sizeof text,"PLAY %02u %.24s",slot+1,loaded?synth_graph.name:"Empty");cv_text(6,12,&FONT_XS,text,C_BLACK);
 }else{snprintf(text,sizeof text,"%02u/08 %.18s%s",slot+1,title,effect_ready?(effect_voice.active?" FX":" FX!"):"");cv_text(6,5,&FONT_XS,text,C_BLACK);}
 cv_blit(0,0);
 for(row=0;row<15;row++){cv_begin(240,12,C_WHITE);draw_row=row;fms_code(source,source_len,scroll_line,15,38,code_glyph,0);cv_blit(0,26+(int)row*12);}
 cv_begin(240,24,C_WHITE);snprintf(text,sizeof text,"%u/%u OCT%+d",scroll_line+1,total_lines,instrument.octave);cv_text(6,7,&FONT_XS,text,C_BLACK);
 for(k=0;k<2;k++){float peak=instrument.peak[k],db=peak>0.001f?20*log10f(peak):-60;int width=(int)(90*fminf(1,fmaxf(0,(db+60)/60)));cv_text(130,1+(int)k*11,&FONT_XS,k?"R":"L",C_BLACK);cv_rect(140,4+(int)k*11,90,5,RGB(235,235,235));cv_rect(140,4+(int)k*11,width,5,instrument.clip[k]?RGB(191,0,0):C_BLACK);}
 cv_blit(0,216);
 if(overlay>=0 && plat_ms()<overlay_until){unsigned value=(unsigned)(ui_knobs[overlay]*100+0.5f);cv_begin(228,24,C_WHITE);snprintf(text,sizeof text,"knob%d   %u.%02u",overlay+1,value/100,value%100);cv_text(6,1,&FONT_XS,text,C_BLACK);cv_rect(6,18,(int)(210*ui_knobs[overlay]),3,C_BLACK);cv_blit(6,180);}
}
void app_ui_init(void){touch_on(1);lcd_fill(0,0,240,240,C_WHITE);draw();}
void app_frame(void){inputs();if(plat_ms()-last_frame>=40){draw();last_frame=plat_ms();}}
void app_say(const char *a,const char *b){snprintf(notice,sizeof notice,"%.30s %.30s",a?a:"",b?b:"");}
int app_expect(const char *what,int *got){
 unsigned i;if(!strcmp(what,"slot"))*got=(int)slot;else if(!strcmp(what,"view"))*got=(int)view_slot;else if(!strcmp(what,"loaded"))*got=loaded;else if(!strcmp(what,"octave"))*got=instrument.octave;else if(!strcmp(what,"scroll"))*got=(int)scroll_line;
 else if(!strcmp(what,"held")){*got=0;for(i=0;i<FMS_VOICES;i++)*got+=instrument.voice[i].held;}
 else if(!strcmp(what,"knob1"))*got=(int)(instrument.knobs[0]*100+0.5f);else return 0;return 1;
}
/* The wire namespace is separate from the platform's updater and remote panel. */
static int validate_package(unsigned target,const void *ptr,uint32_t n){
 uint32_t effect_need=effect_ready?fm_delay_required(&effect_graph,44100):0;
 if(target==FMS_FX_SLOT){
  unsigned i;uint32_t bytes,generation;fms_flash flash={0,flash_read,flash_erase,flash_write};
  if(!package_graph(&effect_graph,ptr,n) || !fms_effect_valid(&effect_graph))return 0;
  effect_need=fm_delay_required(&effect_graph,44100);
  for(i=0;i<FMS_SYNTH_SLOTS;i++){
   if(fms_slot_read(&flash,i,package,&bytes,&generation))return 0;
   if(bytes && (!package_graph(&synth_graph,package,bytes) || !fms_init(&instrument,&synth_graph) || FMS_VOICES*fm_delay_required(&synth_graph,44100)>FMS_DELAY_TOTAL-effect_need))return 0;
  }
  return 1;
 }
 return package_graph(&synth_graph,ptr,n) && fms_init(&instrument,&synth_graph) && FMS_VOICES*fm_delay_required(&synth_graph,44100)<=FMS_DELAY_TOTAL-effect_need;
}

int app_sysex(const uint8_t *p,uint32_t n){
 static const uint8_t prefix[]={0x7d,0x46,0x4d,0x53,1};
 uint8_t raw[FMS_FRAME],reply[FMS_FRAME],wire[310];unsigned i=5,j=0,k,count,h,r,w;int commit;
 if(n<5 || memcmp(p,prefix,5))return 0;
 /* Explicit diagnostic: benchmark while the audio ISR emits silence, then reload. */
 if(n==6 && p[5]==0x7e){
  uint32_t values[8]={0},t,elapsed;float out[FM_BLOCK*2];
  suspend_audio();
  if(loaded){
   instrument.voice_limit=FMS_VOICES; /* isolated benchmark only */
   t=microseconds();fms_note_on(&instrument,0x777,60,70);values[0]=microseconds()-t;
   t=microseconds();fms_render(&instrument,out);values[1]=microseconds()-t;
   for(k=0;k<16;k++){t=microseconds();fms_render(&instrument,out);elapsed=microseconds()-t;if(elapsed>values[2])values[2]=elapsed;}
   t=microseconds();fms_note_on(&instrument,0x778,67,70);values[3]=microseconds()-t;
   t=microseconds();fms_render(&instrument,out);values[4]=microseconds()-t;
   for(k=0;k<16;k++){t=microseconds();fms_render(&instrument,out);elapsed=microseconds()-t;if(elapsed>values[5])values[5]=elapsed;}
   values[6]=instrument.faults;values[7]=1;
  }
  choose(slot);held_keys=touch_keys();wire[0]=0xf0;memcpy(wire+1,prefix,5);wire[6]=0x7e;w=7;
  for(k=0;k<8;k++)for(j=0;j<5;j++)wire[w++]=(uint8_t)(values[k]>>(7*j))&127;
  wire[w++]=0xf7;plat_sysex_send(wire,w);return 1;
 }
 /* Read-only device telemetry for acceptance and troubleshooting. */
 if(n==6 && p[5]==0x7f){
  uint32_t values[14]={slot,loaded,plat_cpu_pct(),plat_xruns(),instrument.faults,overloads,0,0,render_peak_us,render_last_us,start_peak_us,steady_peak_us,view_slot,effect_ready && effect_voice.active};
  uint32_t f=plat_lock();for(k=0;k<FMS_VOICES;k++)values[6]+=instrument.voice[k].synth.active;values[7]=instrument.serial;plat_unlock(f);
  wire[0]=0xf0;memcpy(wire+1,prefix,5);wire[6]=0x7f;w=7;
  for(k=0;k<14;k++)for(j=0;j<5;j++)wire[w++]=(uint8_t)(values[k]>>(7*j))&127;
  wire[w++]=0xf7;plat_sysex_send(wire,w);return 1;
 }
 while(i<n){h=p[i++];count=n-i<7?n-i:7;if(!count || h>=128 || h>>count || j+count>FMS_FRAME)return 1;
  for(k=0;k<count;k++){if(p[i]>=128)return 1;raw[j++]=p[i++]|((h>>k&1)<<7);}}
 commit=j>=2 && raw[1]==FMS_COMMIT;
 if(commit)suspend_audio();link.validate=validate_package;r=fms_request(&link,raw,j,reply);
 if(commit){unsigned old_view=view_slot;load_effect();choose(slot);if(old_view==FMS_FX_SLOT)view_code(old_view);held_keys=touch_keys();}
 if(!r)return 1;
 wire[0]=0xf0;memcpy(wire+1,prefix,5);w=6;
 for(i=0;i<r;i+=count){count=r-i<7?r-i:7;h=0;for(k=0;k<count;k++)h|=(reply[i+k]>>7)<<k;wire[w++]=(uint8_t)h;for(k=0;k<count;k++)wire[w++]=reply[i+k]&127;}
 wire[w++]=0xf7;plat_sysex_send(wire,w);return 1;
}
