/* SPDX-License-Identifier: GPL-3.0-or-later
 * SC 3.14.1 OscUGens.cpp / FilterUGens.cpp, James McCartney
 * (GPL-2.0-or-later), adapted to bounded single-precision rendering.
 */
static uint32_t formant_reset(uint32_t phase,uint32_t frequency,uint32_t fundamental) {
 /* Preserve upstream's wrapped 32-bit multiply, followed by SIGNED division.
  * Unsigned multiplication makes the wrap defined; no software double or
  * 64-bit multiply/load hazard on pi32v2. Caller ensures fundamental != 0. */
 uint32_t product=phase*frequency;int32_t signed_product;
 memcpy(&signed_product,&product,4);return (uint32_t)(signed_product/(int32_t)fundamental);
}
static void formant_render(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int init) {
 uint32_t inc[3],phase[3],width;float scale=536870912.0f/sr;unsigned j,k;
 for(k=0;k<3;k++){
  float x=input(g,b,n,k,0);if(!isfinite(x) || x<0 || x>sr*.5f){v->fault=1;return;}
  inc[k]=(uint32_t)(scale*x);phase[k]=s->dice[k];
 }
 width=inc[0]>inc[2]?inc[0]:inc[2];
 for(j=0;j<count;j++){
  float out=0;
  if(phase[2]<0x20000000u || (phase[2]&0x80000000u)){
   out=(fm_sine[((phase[2]+0x18000000u)>>16)&8191u]+1)*fm_sine[(phase[1]>>16)&8191u];phase[2]+=width;
  }
  checked_signal(v,&b->signal[n->output][j],out);
  phase[0]+=inc[0];phase[1]+=inc[1];
  if(phase[0]>0x20000000u){
   phase[0]-=0x20000000u;
   if(!inc[0]){v->fault=1;return;}
   phase[1]=formant_reset(phase[0],inc[1],inc[0]);phase[2]=formant_reset(phase[0],inc[2],inc[0]);
  }
 }
 for(k=0;k<3;k++)s->dice[k]=init?0:phase[k];
}
static void formlet_coeff(float freq,float time,float sr,float *k,float *r2) {
 float loss=time==0?1:-expm1f(-13.815510557964274f/(time*sr));
 float r=1-loss,sh=sinf(freq*(TAU*.5f/sr));
 *k=(loss*loss+8*r*sh*sh)/(1+r);*r2=r;
}
static void formlet_accumulate(float *hi,float *lo,float x) {
 float sum=*hi+x,back=sum-*hi,low=((*hi-(sum-back))+(x-back))+*lo;
 *hi=sum+low;*lo=low-(*hi-sum);
}
static void formlet_render(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int init) {
 float freq=input(g,b,n,1,0),attack=input(g,b,n,2,0),decay=input(g,b,n,3,0),target[4];unsigned j,k;int high=freq>sr*.25f;
 if(!isfinite(freq) || freq<0 || freq>sr*.5f || !isfinite(attack) || !isfinite(decay) || attack<0 || decay<0 || attack>1000000 || decay>1000000){v->fault=1;return;}
 /* Modulate high resonances down by Nyquist so the difference state also
  * avoids cancellation near the upper band edge. Convert state and the old
  * coefficients if a live frequency change crosses the representation. */
 if(!init && high!=(int)s->dice[1]){
  float sign=(s->dice[0]&1)?1:-1;
  s->a0=sign*((2*s->phase-s->a0)+2*s->last);s->b1=sign*((2*s->y1-s->b1)+2*s->y2);
  s->phase*=sign;s->last*=sign;s->y1*=sign;s->y2*=sign;
  s->previous[0]=2*(1+s->previous[1])-s->previous[0];s->previous[2]=2*(1+s->previous[3])-s->previous[2];
 }
 s->dice[1]=(uint32_t)high;
 if(init || freq!=s->freq || attack!=s->width || decay!=s->initial_value){
  formlet_coeff(high?sr*.5f-freq:freq,decay,sr,&target[0],&target[1]);formlet_coeff(high?sr*.5f-freq:freq,attack,sr,&target[2],&target[3]);
 }else for(k=0;k<4;k++)target[k]=s->previous[k];
 if(init || count==1)for(k=0;k<4;k++)s->previous[k]=target[k];
 for(j=0;j<count;j++){
  float sign=high && (s->dice[0]&1)?-1:1;
  float c[4],t=count==1?1:(float)(j/3)/(count/3),x=input(g,b,n,0,j)*sign,d0,d1,out;
  for(k=0;k<4;k++)c[k]=s->previous[k]+(target[k]-s->previous[k])*t;
  /* Difference states avoid subtracting large resonator delays at low
   * frequencies. This is a linear transform of SC's two recurrences,
   * including its three-sample coefficient interpolation and 0.25 gain. */
  d0=x-c[0]*s->phase+c[1]*s->a0;
  d1=x-c[2]*s->y1+c[3]*s->b1;
  out=sign*.25f*((d0+s->a0)-(d1+s->b1));s->dice[0]^=1;
  formlet_accumulate(&s->phase,&s->last,d0);formlet_accumulate(&s->y1,&s->y2,d1);s->a0=d0;s->b1=d1;
  checked_signal(v,&b->signal[n->output][j],out);
 }
 for(k=0;k<4;k++)s->previous[k]=target[k];
 s->freq=freq;s->width=attack;s->initial_value=decay;
}
