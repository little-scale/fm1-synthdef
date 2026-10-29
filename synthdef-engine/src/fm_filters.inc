/* SPDX-License-Identifier: GPL-3.0-or-later
 * Adapted from SC 3.14.1 FilterUGens.cpp (James McCartney, GPL-2.0-or-later).
 * Included by fm_render.c; private helpers, single-precision arithmetic.
 */
static int is_filter(int type) {
 return type==FM_RHPF || type==FM_BRF || type==FM_LPF || type==FM_HPF || type==FM_RLPF || type==FM_BPF || type==FM_Resonz || type==FM_Ringz;
}
static void filter_coeff(int type,float freq,float param,float sr,float *a0,float *b1,float *b2) {
 float w=freq*(TAU/sr),t,d,c;
 if(type==FM_Resonz || type==FM_Ringz) {resonator(freq,param,sr,type==FM_Ringz,a0,b1,b2);return;}
 if(type==FM_LPF || type==FM_HPF) {
  t=tanf(w*0.5f);d=1.0f+1.4142135623730951f*t+t*t;
  *a0=(type==FM_LPF?t*t:1.0f)/d;*b1=2*(1-t*t)/d;*b2=-(1-1.4142135623730951f*t+t*t)/d;
 } else if(type==FM_BRF){t=tanf(w*param*0.5f);*a0=1/(1+t);*b2=(1-t)* *a0;t=sinf(w*.5f);*b1=4* *a0*t*t;
 } else if(type==FM_BPF) {
  t=tanf(w*param*0.5f);*a0=t/(1+t);*b1=2*cosf(w)/(1+t);*b2=(t-1)/(t+1);
 } else {
  t=tanf(w*fmaxf(0.001f,param)*0.5f);c=(1-t)/(1+t);
  *b1=(1+c)*cosf(w);*b2=-c;
  t=type==FM_RHPF?cosf(w*0.5f):sinf(w*0.5f);*a0=(1+c)*0.5f*t*t;
  if(type==FM_RHPF){t=sinf(w*.5f);*b1=2*(1+c)*t*t;*b2=c;}
 }
}
static void lf_advance(fm_state *s,float inc);
static float filter_tick(int type,fm_state *s,float x,float a0,float b1,float b2) {
 /* Difference-state realization avoids cancellation of large internal
    delays at low cutoff. Coefficients remain linear transforms of SC's
    interpolated coefficients, so modulation keeps the same semantics. */
 if(type==FM_RHPF || type==FM_BRF){
  float d=(type==FM_RHPF?a0*x:x)-b1*s->phase+b2*s->y2;
  float out=type==FM_RHPF?d-s->y2:a0*(d-s->y2)+b1*s->phase;
  lf_advance(s,d);s->y2=d;return out;
 }
 float y=(type==FM_RLPF?a0*x:x)+b1*s->y1+b2*s->y2,out;
 if(type==FM_RHPF)out=y-2*s->y1+s->y2;
 else if(type==FM_BRF)out=a0*(y+s->y2)-b1*s->y1;
 else if(type==FM_RLPF)out=y+2*s->y1+s->y2;
 else if(type==FM_LPF)out=a0*(y+2*s->y1+s->y2);
 else if(type==FM_HPF)out=a0*(y-2*s->y1+s->y2);
 else out=a0*(y-s->y2);
 s->y2=s->y1;s->y1=y;return out;
}
static float lag_coeff(float seconds,float sr) {return seconds==0?0:expf(-6.907755278982137f/(seconds*sr));}
/* Compensated phase addition keeps discontinuities at the same sample as SC's
   double accumulator without software double arithmetic on the device. */
static void lf_advance(fm_state *s,float inc) {
 float sum=s->phase+inc,back=sum-s->phase;
 float low=((s->phase-(sum-back))+(inc-back))+s->last;
 s->phase=sum+low;s->last=low-(s->phase-sum);
}
static int lf_ge(fm_state *s,float bound) {return s->phase>bound || (s->phase==bound && s->last>=0);}
static int lf_le(fm_state *s,float bound) {return s->phase<bound || (s->phase==bound && s->last<=0);}
