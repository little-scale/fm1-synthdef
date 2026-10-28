/* SPDX-License-Identifier: GPL-3.0-or-later */
#include "performance.h"
#include <math.h>
#include <string.h>
static const char *const knobs[]={"knob1","knob2","knob3","knob4"};
static float bound(float x,float a,float b){return fminf(b,fmaxf(a,x));}
void fms_panic(fms_performance *p){unsigned i;for(i=0;i<FMS_VOICES;i++){p->voice[i].synth.active=0;p->voice[i].held=0;p->voice[i].fade=0;}memset(p->keys,-1,sizeof p->keys);p->peak[0]=p->peak[1]=0;p->clip[0]=p->clip[1]=0;}
int fms_init(fms_performance *p,const fm_graph *g){
 unsigned i,j,k;
 /* Delay storage outlives instrument selection: the effect owns its high end. */
 memset(p,0,offsetof(fms_performance,delays));
 memset((uint8_t *)p+offsetof(fms_performance,knobs),0,sizeof *p - offsetof(fms_performance,knobs));
 memset(p->keys,-1,sizeof p->keys);
 for(i=0;i<g->nodes;i++)if(g->node[i].type==FM_In)return 0;
 if(fm_delay_required(g,44100)>FMS_DELAY_SAMPLES)return 0;
 p->voice_delay_samples=fm_delay_required(g,44100);p->graph=g;p->master=0.7f;p->voice_limit=FMS_VOICES;
 for(i=0;i<4;i++){p->knobs[i]=0.5f;for(j=0;j<g->names;j++)if(!strcmp(knobs[i],g->control_name[j].name)){
  unsigned index=g->control_name[j].index;int live=0;p->knobs[i]=bound(g->defaults[index],0,1);
  for(k=0;k<g->nodes;k++){const fm_node *n=&g->node[k];if((n->type==FM_Control || n->type==FM_LagControl) && n->rate==1 && index>=(unsigned)n->special && index<(unsigned)n->special+n->outputs)live=1;}
  if(!live){p->graph=0;return 0;}
 }}
 return 1;
}
int fms_effect_valid(const fm_graph *g){
 unsigned i,j,k;
 if(fm_delay_required(g,44100)>FMS_DELAY_TOTAL)return 0;
 for(i=0;i<g->nodes;i++){
  const fm_node *n=&g->node[i];int done=n->type==FM_EnvGen?4:(n->type==FM_Line || n->type==FM_XLine)?3:-1;
  if(done>=0){const fm_ref *r=&g->ref[n->input+done];if(r->node>=0 || g->constant[r->output]!=0)return 0;}
 }
 for(i=0;i<4;i++)for(j=0;j<g->names;j++)if(!strcmp(knobs[i],g->control_name[j].name)){
  unsigned index=g->control_name[j].index;int live=0;
  for(k=0;k<g->nodes;k++){const fm_node *n=&g->node[k];if((n->type==FM_Control || n->type==FM_LagControl) && n->rate==1 && index>=(unsigned)n->special && index<(unsigned)n->special+n->outputs)live=1;}
  if(!live)return 0;
 }
 return 1;
}
int fms_attach_effect(fms_performance *p,const fm_graph *g,fm_voice *v){
 unsigned i,j;uint32_t need;
 if(!g){p->effect_graph=0;p->effect_voice=0;return 1;}
 need=fm_delay_required(g,44100);
 if(!v || !fms_effect_valid(g) || p->voice_delay_samples*FMS_VOICES>FMS_DELAY_TOTAL-need)return 0;
 p->effect_graph=g;p->effect_voice=v;
 for(i=0;i<4;i++){
  int synth_uses=0;
  if(p->graph)for(j=0;j<p->graph->names;j++)if(!strcmp(knobs[i],p->graph->control_name[j].name))synth_uses=1;
  if(!synth_uses)for(j=0;j<g->names;j++)if(!strcmp(knobs[i],g->control_name[j].name)){
   unsigned index=g->control_name[j].index;
   p->knobs[i]=bound(v->active && v->prepared?v->control[index]:g->defaults[index],0,1);
  }
  fm_set(v,g,knobs[i],p->knobs[i]);
 }
 return 1;
}
int fms_reset_effect(fms_performance *p){
 uint32_t need;unsigned i;
 if(!p->effect_graph || !p->effect_voice)return 1;
 need=fm_delay_required(p->effect_graph,44100);
 if(!fm_start(p->effect_voice,p->effect_graph,44100) || !fm_bind_delay(p->effect_voice,p->effect_graph,p->delays+FMS_DELAY_TOTAL-need,need))return 0;
 for(i=0;i<4;i++)fm_set(p->effect_voice,p->effect_graph,knobs[i],p->knobs[i]);
 return 1;
}
void fms_note_off(fms_performance *p,uint32_t id){unsigned i;for(i=0;i<FMS_VOICES;i++){fms_note *n=&p->voice[i];if(n->held && n->id==id){n->held=0;n->released_frames=0;if(!fm_set(&n->synth,p->graph,"gate",0))n->forced=1;}}}
void fms_note_on(fms_performance *p,uint32_t id,unsigned midi,unsigned velocity){
 unsigned i,choice=FMS_VOICES,active=0;fms_note *n;float tail[2]={0};
 if(!p->graph || midi>127 || velocity>127)return;
 if(!velocity){fms_note_off(p,id);return;}
 fms_note_off(p,id);
 for(i=0;i<FMS_VOICES;i++)active+=p->voice[i].synth.active!=0;
 if(active<p->voice_limit)for(i=0;i<FMS_VOICES;i++)if(!p->voice[i].synth.active){choice=i;break;}
 if(choice==FMS_VOICES){
  for(i=0;i<FMS_VOICES;i++)if(p->voice[i].synth.active && (choice==FMS_VOICES || (!p->voice[i].held && p->voice[choice].held) || (p->voice[i].held==p->voice[choice].held && (uint32_t)(p->serial-p->voice[i].age)>(uint32_t)(p->serial-p->voice[choice].age))))choice=i;
  if(choice==FMS_VOICES)return;
  tail[0]=p->voice[choice].last[0];tail[1]=p->voice[choice].last[1];p->steals++;
 }
 n=&p->voice[choice];memset((uint8_t *)n+offsetof(fms_note,id),0,sizeof *n-offsetof(fms_note,id));n->tail[0]=tail[0];n->tail[1]=tail[1];n->fade=128;
 if(!fm_start(&n->synth,p->graph,44100) || !fm_bind_delay(&n->synth,p->graph,p->delays+choice*p->voice_delay_samples,p->voice_delay_samples)){n->synth.active=0;p->faults++;return;}
 n->id=id;n->age=++p->serial;n->held=1;n->startup_blocks=(uint8_t)p->prepare_blocks;fm_seed(&n->synth,p->serial);
 fm_set(&n->synth,p->graph,"freq",440*exp2f(((float)midi-69)/12));
 fm_set(&n->synth,p->graph,"midinote",(float)midi);fm_set(&n->synth,p->graph,"pitch",(float)midi);
 fm_set(&n->synth,p->graph,"note",(float)midi-60);fm_set(&n->synth,p->graph,"amp",velocity/127.0f);
 fm_set(&n->synth,p->graph,"velocity",velocity/127.0f);fm_set(&n->synth,p->graph,"gate",1);
 for(i=0;i<4;i++)fm_set(&n->synth,p->graph,knobs[i],p->knobs[i]);
}
void fms_key(fms_performance *p,unsigned key,unsigned velocity,int down){
 if(key>=27)return;
 if(down && p->keys[key]<0){int note=53+(int)key+12*p->octave;p->keys[key]=(int8_t)note;fms_note_on(p,0x10000u+key,(unsigned)note,velocity);}
 else if(!down && p->keys[key]>=0){fms_note_off(p,0x10000u+key);p->keys[key]=-1;}
}
void fms_octave(fms_performance *p,int delta){if(delta>0 && p->octave<2)p->octave++;else if(delta<0 && p->octave>-2)p->octave--;}
void fms_knob(fms_performance *p,unsigned knob,float value){unsigned i;if(knob>=4 || !isfinite(value) || (!p->graph && !p->effect_graph))return;p->knobs[knob]=bound(value,0,1);for(i=0;i<FMS_VOICES;i++)if(p->voice[i].synth.active)fm_set(&p->voice[i].synth,p->graph,knobs[knob],p->knobs[knob]);if(p->effect_graph && p->effect_voice)fm_set(p->effect_voice,p->effect_graph,knobs[knob],p->knobs[knob]);}
void fms_render(fms_performance *p,float out[FM_BLOCK*2]){
 unsigned i,j,c;float master=bound(p->master,0,1);memset(out,0,FM_BLOCK*2*sizeof(float));
 for(i=0;i<FMS_VOICES;i++){
  fms_note *n=&p->voice[i];float tmp[FM_BLOCK*2];
  if(!p->graph || !n->synth.active)continue;
  if(n->startup_blocks){
   fm_prepare(p->graph,&n->synth,&p->scratch);
   /* Fade a stolen voice's last sample while preparing its replacement. */
   if(n->tail[0]!=0 || n->tail[1]!=0)for(j=0;j<FM_BLOCK;j++){
    float fade=((n->startup_blocks-1)*FM_BLOCK+FM_BLOCK-j)/(float)(p->prepare_blocks*FM_BLOCK);
    for(c=0;c<2;c++)out[2*j+c]+=n->tail[c]*fade;
   }
   if(!--n->startup_blocks)n->tail[0]=n->tail[1]=0;
   if(n->synth.fault){p->faults++;n->held=0;}
   continue;
  }
  memset(tmp,0,sizeof tmp);
  fm_render(p->graph,&n->synth,&p->scratch,tmp);
  if(n->synth.fault){p->faults++;n->held=0;}
  if(!n->forced && !n->fade){
   for(j=0;j<FM_BLOCK*2;j++)out[j]+=tmp[j];
   n->last[0]=tmp[FM_BLOCK*2-2];n->last[1]=tmp[FM_BLOCK*2-1];
   if(!n->held)n->released_frames+=FM_BLOCK;
  }else for(j=0;j<FM_BLOCK;j++){
   float fade=n->forced?fmaxf(0,1-(float)n->released_frames/128):1;
   for(c=0;c<2;c++){float x=tmp[2*j+c]*fade;if(n->fade)x=x*(1-n->fade/128.0f)+n->tail[c]*(n->fade/128.0f);out[2*j+c]+=x;n->last[c]=x;}
   if(n->fade)n->fade--;if(!n->held)n->released_frames++;
  }
  if((n->forced && n->released_frames>=128) || (!n->held && n->released_frames>=441000))n->synth.active=0;
 }
 if(p->effect_graph && p->effect_voice && p->effect_voice->active){
  float wet[FM_BLOCK*2];memset(wet,0,sizeof wet);
  fm_render_input(p->effect_graph,p->effect_voice,&p->scratch,wet,out);
  if(p->effect_voice->fault)p->faults++;else memcpy(out,wet,sizeof wet);
 }
 for(j=0;j<FM_BLOCK;j++){
  p->gain+=(master-p->gain)*0.004f;
  for(c=0;c<2;c++){float x=out[2*j+c]*p->gain;p->peak[c]=fmaxf(fabsf(x),p->peak[c]*0.999826f);if(fabsf(x)>=1)p->clip[c]=44100;else if(p->clip[c])p->clip[c]--;out[2*j+c]=bound(x,-1,1);}
 }
}
