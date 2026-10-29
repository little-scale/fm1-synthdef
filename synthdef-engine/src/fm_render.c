/* SPDX-License-Identifier: GPL-3.0-or-later
 * Envelope, resonator, operator and pitch-lookup equations adapted from SuperCollider 3.14.1,
 * Copyright (c) 2002 James McCartney (GPL-2.0-or-later). See reference/README.md.
 * Bounded single-precision implementation; no allocation or I/O in rendering.
 */
#include "fm_synth.h"
#include <math.h>
#include <string.h>
#define PI 3.14159265358979323846f
#define TAU (2.0f*PI)
#define FM_VALID 0x464d5331u
static float input(const fm_graph *g,const fm_scratch *b,const fm_node *n,int k,int sample) {
 const fm_ref *r=&g->ref[n->input+k];
 if(r->node<0) return g->constant[r->output];
 unsigned o=g->node[r->node].output+r->output;
 return b->signal[o][g->output_rate[o]==2?sample:0];
}
static int input_rate(const fm_graph *g,const fm_node *n,int k) {
 const fm_ref *r=&g->ref[n->input+k];return r->node<0?0:g->node[r->node].rate;
}
static uint32_t phase_word(float angle) {
 return (uint32_t)(int32_t)(fmodf(angle,TAU)*(536870912.0f/TAU));
}
static float unary(int op,float a) {
 switch(op) {
 case 0:return -a; case 5:return fabsf(a); case 8:return ceilf(a); case 9:return floorf(a);
 case 10:return a-floorf(a); case 11:return (a>0)-(a<0); case 12:return a*a; case 13:return a*a*a;
 case 14:return a<0?-sqrtf(-a):sqrtf(a); case 15:return expf(a); case 16:return 1.0f/a;
 case 17:return 440.0f*powf(2.0f,(a-69.0f)/12.0f);
 case 18:return log2f(a/440.0f)*12.0f+69.0f;
 case 19:return powf(2.0f,a/12.0f); case 20:return 12.0f*log2f(a);
 case 21:return powf(10.0f,a*0.05f); case 22:return log10f(a)*20.0f;
 case 23:return 440.0f*exp2f(a-4.75f); case 24:return log2f(a/440.0f)+4.75f;
 case 25:return logf(fabsf(a)); case 26:return log2f(fabsf(a)); case 27:return log10f(fabsf(a));
 case 28:return sinf(a); case 29:return cosf(a); case 30:return tanf(a); case 31:return asinf(a);
 case 32:return acosf(a); case 33:return atanf(a);
 case 34:return sinhf(a); case 35:return coshf(a); case 36:return tanhf(a);
 case 42:return a/(1.0f+fabsf(a)); case 43:return fabsf(a)<=0.5f?a:(fabsf(a)-0.25f)/a;
 default:return 0;
 }
}
static float bounded_wrap(float x,float lo,float hi);
static float bounded_fold(float x,float lo,float hi);
/* Integer operators deliberately stay within float's exact-integer range.
 * Bounded Euclid avoids soft int64 arithmetic on the pi32v2 audio thread. */
static float integer_gcd(float a,float b,int lcm) {
 if(!isfinite(a)||!isfinite(b)||fabsf(a)>16777216.0f||fabsf(b)>16777216.0f)return NAN;
 int32_t ia=(int32_t)a,ib=(int32_t)b,x=ia<0?-ia:ia,y=ib<0?-ib:ib;
 if(!x||!y)return lcm?0.0f:(float)(ia?ia:ib);
 for(unsigned i=0;i<48 && y;i++){int32_t r=x%y;x=y;y=r;}
 if(y)return NAN;
 int32_t d=ia<0 && ib<0?-x:x;
 return lcm?(float)(ia/d)*(float)ib:(float)d;
}
static float binary(int op,float a,float b) {
 switch(op) {
 case 3:return floorf(a/b);
 case 5:return b==0?0:a-b*floorf(a/b);
 case 17:return integer_gcd(a,b,1); case 18:return integer_gcd(a,b,0);
 case 22:return atan2f(a,b); case 23:return hypotf(a,b);
 case 24:return fabsf(a)+fabsf(b)-(1.4142135623730951f-1.0f)*fminf(fabsf(a),fabsf(b));
 case 25:return a>=0?powf(a,b):-powf(-a,b);
 case 30:return a*b+a; case 31:return a*b+a+b; case 32:return a*a*b; case 33:return a*a*b-a*b*b;
 case 34:return a*a-b*b; case 35:return a*a+b*b; case 36:return (a+b)*(a+b); case 37:return (a-b)*(a-b);
 case 38:return fabsf(a-b); case 39:return a<b?0:a; case 40:return a*0.5f*(b+fabsf(b));
 case 41:return a>=0?a:a*b; case 43:return a-fminf(b,fmaxf(-b,a)); case 46:return a;
 case 19:return b==0?a:floorf(a/b+.5f)*b;
 case 20:return b==0?a:ceilf(a/b)*b;
 case 21:return b==0?a:floorf(a/b)*b;
 case 42:return fminf(b,fmaxf(-b,a));
 case 44:return bounded_fold(a,-b,b);
 case 45:return bounded_wrap(a,-b,b);
 case 0:return a+b;case 1:return a-b;case 2:return a*b;case 4:return a/b;
 case 6:return a==b;case 7:return a!=b;case 8:return a<b;case 9:return a>b;
 case 10:return a<=b;case 11:return a>=b;case 12:return fminf(a,b);case 13:return fmaxf(a,b);default:return 0;}
}
#include "fm_noise.inc"

int fm_start(fm_voice *v,const fm_graph *g,float sr) {
 memset(v,0,sizeof *v);
 if(g->valid!=FM_VALID || !isfinite(sr) || sr<8000 || sr>192000) return 0;
 memcpy(v->control,g->defaults,sizeof v->control); v->sample_rate=sr;v->active=1;random_seed(v,0); return 1;
}
int fm_seed(fm_voice *v,uint32_t seed) {if(v->prepared || v->blocks || !v->active)return 0;random_seed(v,seed);return 1;}
int fm_set(fm_voice *v,const fm_graph *g,const char *name,float value) {
 unsigned i,j;
 if(!isfinite(value) || g->valid!=FM_VALID) return 0;
 for(i=0;i<g->names;i++) if(!strcmp(name,g->control_name[i].name)) {
  unsigned index=g->control_name[i].index;
  if(v->prepared || v->blocks)for(j=0;j<g->nodes;j++) { const fm_node *n=&g->node[j]; if(n->type==FM_Control && !n->rate && index>=(unsigned)n->special && index<(unsigned)n->special+n->outputs) return 0; }
  v->control[index]=value;return 1;
 }
 return 0;
}
/* Error term of a float product, without requiring double or hardware FMA. */
static float product_error(float a,float b,float product) {
 union {float f;uint32_t u;} bits;float ah,bh,al,bl;
 bits.f=a;bits.u &= 0xfffff000u;ah=bits.f;
 bits.f=b;bits.u &= 0xfffff000u;bh=bits.f;
 al=a-ah;bl=b-bh;
 return ((ah*bh-product)+ah*bl+al*bh)+al*bl;
}
/* Recover the residual of a rounded quotient, using float pairs. */
static float quotient_error(float a,float b,float q) {
 float product=q*b;return ((a-product)-product_error(q,b,product))/b;
}
static float precise_lerp(float a,float z,float numerator,float denominator) {
 float q=numerator/denominator,qlow=quotient_error(numerator,denominator,q);
 float span=z-a,back=span-z,spanlow=(z-(span-back))+(-a-back);
 float product=span*q,low=product_error(span,q,product)+span*qlow+spanlow*q;
 float sum=a+product;back=sum-a;
 return sum+((a-(sum-back))+(product-back)+low);
}
static void segment(const fm_graph *g,const fm_scratch *b,const fm_node *n,fm_env *e,int sample,float sr,float forced) {
 int k=9+4*e->stage;
 float previous=e->end,dur,steps,low=0;
 if(e->shape==8) e->level=previous;
 e->end=input(g,b,n,k,sample)*input(g,b,n,1,sample)+input(g,b,n,2,sample);
 dur=forced>=0?forced:input(g,b,n,k+1,sample)*input(g,b,n,3,sample);
 /* Bound float-to-integer conversion even for hostile dynamic durations. */
 steps=dur*sr;
 if(isfinite(steps) && steps>=0 && steps<1e9f) {
  float sum=steps+e->residual;
  low=product_error(dur,sr,steps)+(steps-sum+e->residual);
  if(forced<0)low+=product_error(input(g,b,n,k+1,sample),input(g,b,n,3,sample),dur)*sr;
  steps=sum;
 } else steps=fminf(1e9f,fmaxf(0,steps));
 e->remaining=(int32_t)steps; if(steps==(float)e->remaining && low<0)e->remaining--; if(e->remaining<1)e->remaining=1;
 e->residual=steps-(float)e->remaining+low;
 e->duration=fmaxf(1,steps); e->elapsed=0;
 e->shape=(int16_t)input(g,b,n,k+2,sample);e->curve=input(g,b,n,k+3,sample);
 if(e->remaining==1)e->shape=1;
 if(e->shape==5 && fabsf(e->curve)<0.001f)e->shape=1;
 if(e->shape==0)e->level=e->end;
 if(e->shape==8)e->level=previous;
 e->start=e->level;
 if(e->shape==5){e->curve_offset=1.0f/expm1f(e->curve);e->grow=expm1f(e->curve/e->duration);e->curve_compensation=0;e->curve_progress=0;}
}
/* Evaluate from elapsed samples instead of a nearly-unit recursive coefficient.
   This avoids catastrophic precision loss in long sine/Welch segments on an
   FPU without hardware double precision. Segment boundary semantics stay SC's. */
static float curve_value(fm_env *e) {
 float t=0,a=e->start,z=e->end;
 if(e->shape!=5 || fabsf(e->curve)>20)t=(float)e->elapsed/e->duration;
 switch(e->shape) {
 case 1:return a+(z-a)*t;
 case 2:return a*powf(z/a,t);
 case 3:return a+(z-a)*(0.5f-0.5f*cosf(PI*t));
 case 4:return z>=a?a+(z-a)*sinf(PI*0.5f*t):z+(a-z)*cosf(PI*0.5f*t);
 case 5:{
  float delta,next;
  if(fabsf(e->curve)>20)return a+(z-a)*expm1f(e->curve*t)/expm1f(e->curve);
  delta=e->grow*(e->curve_progress+1)-e->curve_compensation;
  next=e->curve_progress+delta;e->curve_compensation=(next-e->curve_progress)-delta;e->curve_progress=next;
  return a+(z-a)*(next*e->curve_offset);
 }
 case 6: {float y=sqrtf(a)+(sqrtf(z)-sqrtf(a))*t;return y*y;}
 case 7: {float y=fmaxf(0,cbrtf(a)+(cbrtf(z)-cbrtf(a))*t);return y*y*y;}
 default:return e->level;
 }
}

static void envelope_init(const fm_graph *g,const fm_scratch *b,const fm_node *n,fm_state *s,int sample) {
 fm_env *e=&s->env;memset(e,0,sizeof *e);
 e->stages=(int16_t)input(g,b,n,6,sample);e->release_node=(int16_t)input(g,b,n,7,sample);e->loop_node=(int16_t)input(g,b,n,8,sample);e->done_action=(int16_t)input(g,b,n,4,sample);
 e->level=e->end=input(g,b,n,5,sample)*input(g,b,n,1,sample)+input(g,b,n,2,sample);
 e->stage=-2;e->remaining=1000000000;e->shape=8;
 if(input(g,b,n,11,sample)==8)e->level=input(g,b,n,9,sample);
}
static float envelope(const fm_graph *g,const fm_scratch *b,const fm_node *n,fm_state *s,int sample,float sr,int *free_voice,float gate) {
 fm_env *e=&s->env;
 float out;int stages,release,loop;
 if(!s->initialized)envelope_init(g,b,n,s,sample);
 stages=e->stages;release=e->release_node;loop=e->loop_node;
 if(e->previous_gate<=0 && gate>0) {e->stage=-1;e->remaining=0;e->released=0;e->done=0;e->residual=0;}
 else if(gate<=-1 && e->previous_gate>-1) {e->stage=stages-1;e->released=1;segment(g,b,n,e,sample,sr,-gate-1);e->residual=0;}
 else if(e->previous_gate>0 && gate<=0 && release>=0 && !e->released) {e->remaining=0;e->stage=release-1;e->released=1;e->residual=0;}
 e->previous_gate=gate;
 if(e->remaining<=0) {
  if(e->stage+1>=stages) {e->remaining=1000000000;e->shape=0;e->done=1;if(e->done_action==2)*free_voice=1;}
  else if(e->stage+1==release && !e->released) {
   if(loop>=0) {e->stage=loop;segment(g,b,n,e,sample,sr,-1);}
   else {e->remaining=1000000000;e->shape=9;e->level=e->end;}
  } else {e->stage++;segment(g,b,n,e,sample,sr,-1);}
 }
 out=e->level;
 if(e->shape>=1 && e->shape<=7) {e->elapsed++;e->level=curve_value(e);}
 if(e->done)e->level=e->end;
 e->remaining--;return out;
}
static void resonator(float freq,float param,float sr,int ring,float *a0,float *b1,float *b2) {
 float w=freq*TAU/sr;
 float r=ring?(param==0?0:expf(-6.907755278982137f/(param*sr))):1-w*param*0.5f;
 float r2=r*r;*b1=4*r2*cosf(w)/(1+r2);*b2=-r2;*a0=ring?0.5f:(1-r2)*0.5f;
}
#include "fm_filters.inc"
#include "fm_bandlimited.inc"
#include "fm_rings_state.inc"
#include "fm_delay.inc"
#include "fm_fast.inc"

/* Keep hot loops separate from the large generic UGen dispatcher. */
static __attribute__((noinline)) void fast_envelope(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int initializing,int *free_voice){
 unsigned j;
 fm_bound_input gate=bind_input(g,b,n,0,0);
 for(j=0;j<count;){
  checked_signal(v,&b->signal[n->output][j],envelope(g,b,n,s,j,sr,free_voice,gate.p[gate.rate==2?j:0]));s->initialized=1;j++;
  /* A scalar gate cannot change inside this block. Run the rest of a
     numeric segment without repeating trigger/release/shape dispatch.
     Stop exactly at the segment boundary; the general path handles it. */
  if(gate.rate!=2 && s->env.shape==5 && fabsf(s->env.curve)<=20 && s->env.remaining>0){
   fm_env *e=&s->env;unsigned end=j+(unsigned)e->remaining;if(end>count)end=count;
   float start=e->start,span=e->end-start,grow=e->grow,offset=e->curve_offset;
   for(;j<end;j++){
    float delta=grow*(e->curve_progress+1)-e->curve_compensation,next=e->curve_progress+delta;
    checked_signal(v,&b->signal[n->output][j],e->level);
    e->curve_compensation=(next-e->curve_progress)-delta;e->curve_progress=next;
    e->level=start+span*(next*offset);e->elapsed++;e->remaining--;
   }
  }else if(gate.rate!=2 && (s->env.shape==0 || s->env.shape==8 || s->env.shape==9) && s->env.remaining>0){
   fm_env *e=&s->env;unsigned end=j+(unsigned)e->remaining;if(end>count)end=count;
   float level=e->level;checked_signal(v,&level,level);
   e->remaining-=(int32_t)(end-j);
   for(;j<end;j++)b->signal[n->output][j]=level;
  }
 }
 if(initializing)envelope_init(g,b,n,s,0);
}

#include "fm_extensions.inc"
#include "fm_utilities.inc"
#include "fm_modulation.inc"
#include "fm_gendy.inc"
#include "fm_formants.inc"
#include "fm_reverb.inc"
#include "fm_rings.inc"
#include "fm_stochastic.inc"
#include "fm_expansion.inc"

static void render_graph(const fm_graph *g,fm_voice *v,fm_scratch *b,float *stereo,int initializing,const float *input_stereo) {
 unsigned ni,j,k;int free_voice=0;float mixed[FM_BLOCK*2];memset(mixed,0,sizeof mixed);
 if(!v->active || g->valid!=FM_VALID)return;
 /* Scratch is shared across voices. Restore cached initial-rate results from
    this voice before consumers run; never rely on the last voice's scratch. */
 /* Stop before consumers can read an output rejected by an earlier node. */
 for(ni=0;ni<g->nodes && !v->fault;ni++) {
  const fm_node *n=&g->node[ni];fm_state *s=&v->state[ni];
  unsigned count=n->rate==2 && !initializing?FM_BLOCK:1;
  float sr=n->rate==2?v->sample_rate:v->sample_rate/FM_BLOCK;
  float target0=0,target1=0,target2=0,old0=0,old1=0,old2=0;
  #define IN(k) input(g,b,n,k,j)
  if(!initializing && !n->rate && n->type!=FM_Control && n->outputs==1 && s->initialized){b->signal[n->output][0]=s->initial_value;continue;}
  if(n->type==FM_Line && !g->used[ni]){
   fm_env *e=&s->env;
   if(!s->initialized){
    float start=input(g,b,n,0,0),end=input(g,b,n,1,0);
    if(fabsf(start)>1e12f || fabsf(end)>1e12f)v->fault=1;
    e->remaining=(int32_t)fminf(1e9f,fmaxf(0,input(g,b,n,2,0)*sr+0.5f));s->initialized=1;
   }
   if(!initializing){if(e->remaining>(int32_t)count)e->remaining-=(int32_t)count;else{e->remaining=0;if(input(g,b,n,3,0)==2)free_voice=1;}}
   continue;
  }
  if(n->type==FM_Control || n->type==FM_LagControl || n->type==FM_TrigControl) {
   for(k=0;k<n->outputs;k++) {
    unsigned index=n->special+k,o=n->output+k;float z=v->control[index];
    if(n->type==FM_LagControl) {
     if(!s->initialized){v->control_smooth[o]=z;v->control_coeff[o]=lag_coeff(input(g,b,n,k,0),sr);}
     z=z+v->control_coeff[o]*(v->control_smooth[o]-z);v->control_smooth[o]=z;
    }
    if(n->type==FM_TrigControl){if(initializing)z=0;else v->control[index]=0;}
    if(!isfinite(z) || fabsf(z)>1e12f){v->fault=1;z=0;}
    b->signal[o][0]=z;
   }
   s->initialized=1;continue;
  }
  if(n->type==FM_In){
   int valid=input(g,b,n,0,0)==0;if(!valid)v->fault=1;
   for(k=0;k<n->outputs;k++)for(j=0;j<count;j++)
    checked_signal(v,&b->signal[n->output+k][j],valid && !initializing && input_stereo?input_stereo[2*j+k]:0);
   s->initialized=1;continue;
  }
  if(n->type==FM_Out) {
   /* The FM-1 exposes one stereo output, not a server-wide bus array. */
   if(input(g,b,n,0,0)!=0){v->fault=1;continue;}
   if(initializing)continue;
   for(k=1;k<n->inputs;k++){
    const fm_ref *r=&g->ref[n->input+k];const float *channel=b->signal[g->node[r->node].output+r->output];
    for(j=0;j<FM_BLOCK;j++)mixed[2*j+k-1]+=channel[j];
   }
   continue;
  }
  if(n->type==FM_BinaryOpUGen || n->type==FM_MulAdd || n->type==FM_Sum3 || n->type==FM_Sum4){fast_arithmetic(g,v,b,n,s,count);s->initial_value=b->signal[n->output][0];continue;}
  if(n->type==FM_EnvGen){fast_envelope(g,v,b,n,s,count,sr,initializing,&free_voice);continue;}
  if(n->type==FM_SinOsc){fast_sine(g,v,b,n,s,count,sr,initializing);continue;}
  if(n->type==FM_Pan2 && input_rate(g,n,1)!=2){fast_pan(g,v,b,n,s,count);continue;}
  if(n->type==FM_Saw || n->type==FM_Pulse){bandlimited(g,v,b,n,s,count,sr,initializing);continue;}
  if(n->type==FM_Decay || n->type==FM_Decay2){fast_decay(g,v,b,n,s,count,sr,initializing);continue;}
  if(n->type==FM_Dust || n->type==FM_Dust2){fast_dust(g,v,b,n,s,count,sr);continue;}
  if(n->type>=FM_OnePole && n->type<=FM_Decay2){extensions(g,v,b,n,s,count,sr,initializing);continue;}
  if(n->type==FM_Formant){formant_render(g,v,b,n,s,count,sr,initializing);continue;}
  if(n->type==FM_Formlet){formlet_render(g,v,b,n,s,count,sr,initializing);continue;}
  if(fm_is_gendy(n->type)){gendy_render(g,v,b,n,s,count,sr,initializing);continue;}
  if(n->type>=FM_VarSaw && n->type<=FM_BHiShelf){modulation(g,v,b,n,s,count,sr,initializing);continue;}
  if(n->type>=FM_Fold && n->type<=FM_MiMu){utilities(g,v,b,n,s,count,sr,initializing);continue;}
  if(n->type>=FM_SinOscFB && n->type<=FM_StandardL){if(n->type>=FM_FBSineN)chaos_render(g,v,b,n,s,count,sr,initializing);else expansion_render(g,v,b,n,s,count,sr,initializing);continue;}
  if(n->type>=FM_Crackle && n->type<=FM_LorenzL){stochastic_render(g,v,b,n,s,count,sr,initializing);continue;}
  if(fm_is_rings(n->type)){rings_render(g,v,b,n,s,initializing);continue;}
  if(fm_is_reverb(n->type)){reverb_render(g,v,b,n,s,count,initializing);continue;}
  if(fm_is_delay(n->type)){delay_render(g,v,b,n,s,count,sr,initializing);continue;}
  if(!s->initialized && (n->type==FM_Klang || n->type==FM_Klank)) {
   j=0;
   for(k=0;k<n->partials;k++) {
    fm_partial *p=&v->partial[n->partial+k];int base=(n->type==FM_Klang?2:4)+3*k;
    if(n->type==FM_Klang) {
     float w=(IN(base)*IN(0)+IN(1))*TAU/sr,amp=IN(base+1),phase=IN(base+2);
     p->y1=amp*sinf(phase-w);p->y2=amp*sinf(phase-2*w);p->b1=2*cosf(w);
    } else {resonator(IN(base)*IN(1)+IN(2),IN(base+2)*IN(3),sr,1,&p->a0,&p->b1,&p->b2);p->a0=IN(base+1)*0.25f;}
   }
  }
  if(is_filter(n->type)) {
   float freq=input(g,b,n,1,0),param=n->inputs>2?input(g,b,n,2,0):0;
   if(!s->initialized || freq!=s->freq || param!=s->width)filter_coeff(n->type,freq,param,sr,&target0,&target1,&target2);
   else {target0=s->a0;target1=s->b1;target2=s->b2;}
   if(!s->initialized) {
    s->a0=target0;s->b1=target1;s->b2=target2;
   }
   s->freq=freq;s->width=param;old0=s->a0;old1=s->b1;old2=s->b2;
  } else if(n->type==FM_Lag || n->type==FM_LagUD || n->type==FM_LeakDC) {
   target1=n->type==FM_LeakDC?input(g,b,n,1,0):lag_coeff(input(g,b,n,1,0),sr);
   target2=n->type==FM_LagUD?lag_coeff(input(g,b,n,2,0),sr):target1;
   if(!s->initialized){s->b1=target1;s->b2=target2;s->y1=n->type==FM_LeakDC?0:input(g,b,n,0,0);s->last=input(g,b,n,0,0);}
   old1=s->b1;old2=s->b2;
  }
  if(!s->initialized && (n->type==FM_BinaryOpUGen || n->type==FM_MulAdd || n->type==FM_Sum3 || n->type==FM_Sum4))
   for(k=0;k<n->inputs;k++)s->previous[k]=input(g,b,n,k,0);
  for(j=0;j<count;j++) {
   float a=n->inputs?IN(0):0,out=0,math_in[4];
   if(n->type==FM_BinaryOpUGen || n->type==FM_MulAdd || n->type==FM_Sum3 || n->type==FM_Sum4) {
    for(k=0;k<n->inputs;k++) {
     float x=IN(k);math_in[k]=(count>1 && input_rate(g,n,k)==1)?s->previous[k]+(x-s->previous[k])*(float)j/count:x;
    }
   }
   switch(n->type) {
   case FM_SampleRate:out=v->sample_rate;break;
   case FM_SampleDur:out=1.0f/v->sample_rate;break;
   case FM_ControlRate:out=v->sample_rate/FM_BLOCK;break;
   case FM_ControlDur:out=FM_BLOCK/v->sample_rate;break;
   case FM_BlockSize:out=FM_BLOCK;break;
   case FM_RadiansPerSample:out=TAU/v->sample_rate;break;
   case FM_Delay1:case FM_Delay2:
    if(!s->initialized){s->y1=IN(1);s->y2=n->type==FM_Delay2?IN(2):0;}
    out=n->type==FM_Delay1?s->y1:s->y2;
    if(!initializing){s->y2=s->y1;s->y1=a;}break;
   case FM_Rand:case FM_ExpRand:
    if(!s->initialized){
     float hi=IN(1),r=random_uniform(v);
     if(n->type==FM_Rand)s->last=r*(hi-a)+a;
     else if(a==0 || hi==0 || (a>0)!=(hi>0)){v->fault=1;s->last=0;}
     else s->last=powf(hi/a,r)*a;
    }
    out=s->last;break;
   case FM_WhiteNoise:case FM_PinkNoise:case FM_LFNoise0:case FM_LFNoise1:case FM_LFNoise2:
    out=noise_tick(v,n,s,n->inputs?input(g,b,n,0,0):0,sr);break;
   case FM_RandSeed:
    if(a>0 && s->last<=0)random_seed(v,(uint32_t)(int32_t)IN(1));s->last=a;out=0;break;
   case FM_SinOsc:
    {
     float phase=IN(1);
     if(!s->initialized)s->last=phase;
     if(input_rate(g,n,1)==1)phase=s->last+(phase-s->last)*(float)j/count;
     if(!isfinite(phase)) {v->fault=1;phase=0;}
     uint32_t word=(s->phase_acc+phase_word(phase))&0x1fffffffu;
     out=sinf((float)word*(TAU/536870912.0f));
     float freq=fmodf(a,sr);
     s->phase_acc+=(uint32_t)(int32_t)(freq*(536870912.0f/sr));
     s->phase_acc &= 0x1fffffffu;
     if(j==count-1)s->last=IN(1);
    }break;
   case FM_DC:out=s->initialized?s->last:a;s->last=out;break;
   case FM_BinaryOpUGen:out=binary(n->special,math_in[0],math_in[1]);break;
   /* Tables live in validated graph constants: no mutable buffer or audio allocation. */
   case FM_MaxLocalBufs: case FM_LocalBuf: case FM_SetBuf:out=0.0f;break;
   case FM_DegreeToKey:{
    float degree=IN(1);
    if(!isfinite(degree)||fabsf(degree)>16777216.0f){v->fault=1;out=0.0f;break;}
    int32_t index=(int32_t)floorf(degree),size=n->partials,oct=index/size,key=index%size;
    /* Euclidean wrap also handles exact negative octaves without an out-of-bounds read. */
    if(key<0){key+=size;oct--;}
    int32_t octave=(int32_t)IN(2);
    out=g->constant[g->ref[n->partial+(unsigned)key].output]+(float)octave*(float)oct;
    break;
   }
   case FM_UnaryOpUGen:out=unary(n->special,a);break;
   case FM_MulAdd:out=math_in[0]*math_in[1]+math_in[2];break;
   case FM_Sum3:out=math_in[0]+math_in[1]+math_in[2];break;
   case FM_Sum4:out=math_in[0]+math_in[1]+math_in[2]+math_in[3];break;
   case FM_K2A:
    if(!s->initialized)s->last=a;
    out=s->last+(a-s->last)*(float)j/FM_BLOCK;
    if(j==FM_BLOCK-1)s->last=a;
    break;
   case FM_A2K:out=a;break;
   case FM_Pan2: {
    float pos=fminf(1,fmaxf(-1,IN(1)));
    float index=floorf(1024*pos+1024.5f);
    float left=sinf((2048-index)*(PI/4096.0f)),right=sinf(index*(PI/4096.0f));
    float level=input(g,b,n,2,0),t=(float)j/count;
    if(input_rate(g,n,1)==2) {
     if(!s->initialized)s->last=level;
     level=s->last+(level-s->last)*t;
     out=a*level*left;b->signal[n->output+1][j]=a*level*right;
     if(j==count-1)s->last=input(g,b,n,2,0);
    } else {
     left*=level;right*=level;
     if(!s->initialized){s->a0=left;s->b1=right;}
     out=a*(s->a0+(left-s->a0)*t);b->signal[n->output+1][j]=a*(s->b1+(right-s->b1)*t);
     if(j==count-1){s->a0=left;s->b1=right;}
    }
   }break;
   case FM_EnvGen:out=envelope(g,b,n,s,j,sr,&free_voice,a);break;
   case FM_Line:case FM_XLine: {
    fm_env *e=&s->env;
    if(!s->initialized) {e->level=a;e->start=a;e->end=IN(1);e->remaining=(int32_t)fminf(1e9f,fmaxf(0,IN(2)*sr+0.5f));
     e->duration=fmaxf(1,(float)e->remaining);if(e->remaining>0)e->grow=n->type==FM_Line?(e->end-a)/e->remaining:powf(e->end/a,1.0f/e->remaining);
     else e->level=e->end;
    }
    out=e->level;
    if(!initializing && e->remaining>0) {e->elapsed++;float t=(float)e->elapsed/e->duration;e->level=n->type==FM_Line?precise_lerp(e->start,e->end,(float)e->elapsed,e->duration):e->start*powf(e->end/e->start,t);e->remaining--;}
    if(!initializing && !e->remaining){e->level=e->end;if(IN(3)==2)free_voice=1;}
   }break;
   case FM_RHPF:case FM_BRF:case FM_LPF:case FM_HPF:case FM_RLPF:case FM_BPF:case FM_Resonz:case FM_Ringz: {
    float t=count==1?(n->type==FM_Resonz || n->type==FM_Ringz?0:1):(float)(j/3)/(count/3);
    out=filter_tick(n->type,s,a,old0+(target0-old0)*t,old1+(target1-old1)*t,old2+(target2-old2)*t);
    if(initializing && n->type==FM_Resonz)out=0;
   }break;
   case FM_Lag:case FM_LagUD:case FM_LeakDC: {
    float t=count==1?1:(float)(j+(n->type==FM_LeakDC?0:1))/count;
    float up=old1+(target1-old1)*t,down=old2+(target2-old2)*t;
    if(n->type==FM_LeakDC){out=a-s->last+up*s->y1;s->last=a;}
    else out=a+(a>s->y1?up:down)*(s->y1-a);
    s->y1=out;
   }break;
   case FM_LFSaw:case FM_LFTri:case FM_LFPulse: {
    if(!s->initialized) {s->phase=IN(1);if(n->type==FM_LFTri)s->phase-=4*floorf(s->phase*0.25f);if(n->type==FM_LFPulse)s->width=IN(2);}
    if(n->type==FM_LFSaw) {
     out=s->phase;lf_advance(s,a*(2.0f/sr));
     if(lf_ge(s,1))s->phase-=2;else if(lf_le(s,-1))s->phase+=2;
    } else if(n->type==FM_LFTri) {
     out=s->phase>1?2-s->phase:s->phase;lf_advance(s,a*(4.0f/sr));if(lf_ge(s,3))s->phase-=4;
    } else {
     if(lf_ge(s,1)){s->phase-=1;s->width=input(g,b,n,2,0);out=s->width<=0.5f?1:0;}
     else out=s->phase<s->width || (s->phase==s->width && s->last<0)?1:0;
     lf_advance(s,a*(1.0f/sr));
    }
   }break;
   case FM_Select: {
    float choice=fminf(n->inputs-2,fmaxf(0,a));out=IN(1+(int)choice);
   }break;
   case FM_Clip: {
    float lo=IN(1),hi=IN(2),t=(float)j/count;
    if(!s->initialized){s->a0=lo;s->b1=hi;}
    if(count>1){if(input_rate(g,n,1)!=2)lo=s->a0+(lo-s->a0)*t;if(input_rate(g,n,2)!=2)hi=s->b1+(hi-s->b1)*t;}
    out=fmaxf(lo,fminf(hi,a));
    if(j==count-1){s->a0=IN(1);s->b1=IN(2);}
   }break;
   case FM_Klang:case FM_Klank: {
    float sum=0;
    for(k=0;k<n->partials;k++) {
     fm_partial *p=&v->partial[n->partial+k];
     float y=n->type==FM_Klang?p->b1*p->y1-p->y2:a+p->b1*p->y1+p->b2*p->y2;
     sum+=n->type==FM_Klang?y:p->a0*y;p->y2=p->y1;p->y1=y;
    }
    if(n->type==FM_Klang)out=sum;
    else {out=sum-s->y2;s->y2=s->y1;s->y1=sum;}
   }break;
   default: v->fault=1;
   }
   if(!isfinite(out) || fabsf(out)>1e12f || (n->type==FM_Pan2 && !isfinite(b->signal[n->output+1][j]))) {
    v->fault=1;out=0;
    if(n->type==FM_Pan2)b->signal[n->output+1][j]=0;
   }
   b->signal[n->output][j]=out;s->initialized=1;
  }
  if(n->type==FM_BinaryOpUGen || n->type==FM_MulAdd || n->type==FM_Sum3 || n->type==FM_Sum4)
   for(k=0;k<n->inputs;k++)s->previous[k]=input(g,b,n,k,0);
  if(initializing) {
   if(n->type==FM_SinOsc)s->phase_acc=0;
   if(n->type==FM_EnvGen)envelope_init(g,b,n,s,0);
   if(n->type==FM_LFSaw || n->type==FM_LFTri || n->type==FM_LFPulse) {
    s->phase=input(g,b,n,1,0);s->last=0;
    if(n->type==FM_LFTri)s->phase-=4*floorf(s->phase*0.25f);
    if(n->type==FM_LFPulse)s->width=input(g,b,n,2,0);
   }
   if(n->type==FM_Klang || n->type==FM_Klank)s->initialized=0;
   if(n->type==FM_Klank){s->y1=0;s->y2=0;for(k=0;k<n->partials;k++){v->partial[n->partial+k].y1=0;v->partial[n->partial+k].y2=0;}}
  }
  if(!n->rate && n->outputs==1)s->initial_value=b->signal[n->output][0];
  if(is_filter(n->type)){s->a0=target0;s->b1=target1;s->b2=target2;}
  else if(n->type==FM_Lag || n->type==FM_LagUD || n->type==FM_LeakDC){s->b1=target1;s->b2=target2;}
  #undef IN
 }
 /* SC doneAction=2 takes effect after the current calculation block. */
 if(!initializing) {
  if(!v->fault)for(j=0;j<FM_BLOCK*2;j++)stereo[j]+=mixed[j];
  v->blocks++;if(free_voice)v->active=0;
 }
 if(v->fault)v->active=0;
}
void fm_prepare(const fm_graph *g,fm_voice *v,fm_scratch *b){
 if(v->active && g->valid==FM_VALID && !v->prepared){render_graph(g,v,b,0,1,0);v->prepared=1;}
}
void fm_render_input(const fm_graph *g,fm_voice *v,fm_scratch *b,float *stereo,const float *input_stereo) {
 if(!v->active || g->valid!=FM_VALID)return;
 fm_prepare(g,v,b);
 render_graph(g,v,b,stereo,0,input_stereo);
}

void fm_render(const fm_graph *g,fm_voice *v,fm_scratch *b,float *stereo){fm_render_input(g,v,b,stereo,0);}
