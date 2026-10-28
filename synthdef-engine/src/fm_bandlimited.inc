/* SPDX-License-Identifier: GPL-3.0-or-later
 * SC 3.14.1 Saw/Pulse band-limited impulse recurrence, including harmonic-count
 * crossfades and width interpolation. Source: OscUGens.cpp, James McCartney.
 */
#include "fm_tables.inc"
static float table_sine(uint32_t phase) {
 unsigned index=(phase>>16)&8191u;float f=(phase&65535u)*(1.0f/65536.0f);
 return fm_sine[index]+f*(fm_sine[index+1]-fm_sine[index]);
}
static int bad_cosecant(unsigned i) {return i<=8 || i>=8184 || (i>=4088 && i<=4104);}
static float blip_ratio(uint32_t phase,int harmonics,int *singular) {
 unsigned index=(phase>>16)&8191u;float f=(phase&65535u)*(1.0f/65536.0f),denom;
 float numerator=table_sine(phase*(uint32_t)(2*harmonics+1));*singular=0;
 if(bad_cosecant(index) || bad_cosecant(index+1)) {
  denom=table_sine(phase);if(fabsf(denom)<0.0005f){*singular=1;return 0;}
  return numerator/denom;
 }
 denom=fm_cosecant[index]+f*(fm_cosecant[index+1]-fm_cosecant[index]);return numerator*denom;
}
static float saw_blip(uint32_t phase,int n,float scale) {
 int singular;float ratio=blip_ratio(phase,n,&singular);return singular?1:(ratio-1)*scale;
}
static void bandlimited(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int initializing) {
 float freq=input(g,b,n,0,0),duty=n->type==FM_Pulse?input(g,b,n,1,0):0;
 float partials=(sr*0.5f)/freq;unsigned j;int next_n,old_n,crossfade;
 uint32_t step,next_offset=0,offset=s->phase_offset;int32_t offset_step=0;
 float scale,old_scale;
 /* These ranges prevent undefined float conversions and pathological inputs.
    The importer does not silently replace an invalid oscillator with a sine. */
 if(!(freq>0) || !isfinite(partials) || partials<1 || partials>16777216 || duty<0 || duty>1) {
  v->fault=1;for(j=0;j<count;j++)b->signal[n->output][j]=0;return;
 }
 next_n=(int)partials+(int)floorf((partials-(int)partials)+quotient_error(sr*0.5f,freq,partials));scale=0.5f/next_n;
 if(!s->initialized){s->freq=freq;s->env.remaining=next_n;s->a0=scale;s->y1=n->type==FM_Saw?-0.46f:0;}
 old_n=s->env.remaining;old_scale=s->a0;crossfade=next_n!=old_n;
 {
  float rate=crossfade?fmaxf(freq,s->freq):freq;
  float factor=268435456.0f/sr,product=factor*rate;
  float residual=product_error(factor,rate,product)+quotient_error(268435456.0f,sr,factor)*rate;
  step=(uint32_t)((int32_t)product+(int32_t)floorf((product-(int32_t)product)+residual));
 }
 if(n->type==FM_Pulse){next_offset=(uint32_t)(duty*268435456.0f);offset_step=(int32_t)(((float)next_offset-(float)offset)/(float)count);}
 for(j=0;j<count;j++) {
  float out,blip,t=(float)j/count;
  uint32_t phase=s->phase_acc;
  if(initializing && n->type==FM_Pulse){b->signal[n->output][j]=0;continue;}
  if(n->type==FM_Saw) {
   blip=saw_blip(phase,next_n,scale);
   if(crossfade){float old=saw_blip(phase,old_n,old_scale);blip=old+t*(blip-old);}
  } else if(crossfade) {
   float old=saw_blip(phase,old_n,old_scale),next=saw_blip(phase,next_n,scale);
   float old2=saw_blip(phase+offset,old_n,old_scale),next2=saw_blip(phase+offset,next_n,scale);
   blip=(old+t*(next-old))-(old2+t*(next2-old2));
  } else {
   int singular;float p1=blip_ratio(phase,next_n,&singular);if(singular)p1=1.0f/scale+1;
   float p2=blip_ratio(phase+offset,next_n,&singular);if(singular)p2=1.0f/scale+1;
   blip=(p1-p2)*scale;
  }
  out=blip+0.999f*s->y1;s->y1=out;s->phase_acc+=step;offset+=(uint32_t)offset_step;
  if(!isfinite(out) || fabsf(out)>1e12f){v->fault=1;out=0;}
  b->signal[n->output][j]=out;
 }
 s->a0=scale;s->env.remaining=next_n;s->freq=freq;s->initialized=1;
 if(initializing){s->phase_acc=0;s->phase_offset=0;s->y1=n->type==FM_Saw?-0.46f:0;}
 else s->phase_offset=next_offset;
}
