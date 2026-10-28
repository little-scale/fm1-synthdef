/* SPDX-License-Identifier: GPL-3.0-or-later
 * Adapted from SuperCollider 3.14.1 FilterUGens, TriggerUGens, NoiseUGens and
 * LFUGens (James McCartney and contributors, GPL-2.0-or-later).
 * Existing per-node state is reused: no larger voice or delay allocation.
 */
static float impulse_wrap(fm_state *s,float inc) {
 if(inc<0){
  if(!lf_le(s,0))return 0;
  lf_advance(s,1);if(lf_le(s,0))lf_advance(s,-ceilf(s->phase));return 1;
 }
 if(!lf_ge(s,1))return 0;
 lf_advance(s,-1);if(lf_ge(s,1))lf_advance(s,-floorf(s->phase));return 1;
}
/* Store exp(coefficient)-1 accurately and retain a low state word. A rounded
 * near-one float pole otherwise accumulates audible error on long decays. */
static float decay_grow(float seconds,float sr){return seconds==0?-1:expm1f(-6.907755278982137f/(seconds*sr));}
static float decay_tick(float *hi,float *lo,float x,float grow){
 float p=grow*(*hi),pl=product_error(grow,*hi,p)+grow*(*lo);
 float d=x+p,back=d-x,dl=(x-(d-back))+(p-back)+pl;
 float sum=*hi+d;back=sum-*hi;
 float low=(*hi-(sum-back))+(d-back)+*lo+dl;
 *hi=sum+low;*lo=low-(*hi-sum);return *hi;
}
static void extensions(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int initializing) {
 unsigned j;float *out=b->signal[n->output],x=input(g,b,n,0,0);
 float param=n->inputs>1?input(g,b,n,1,0):0,third=n->inputs>2?input(g,b,n,2,0):0;
 float old=s->b1,target=old,old2=s->b2,target2=old2,previous_inc,block_trigger=s->last;
 if(n->type==FM_OnePole || n->type==FM_OneZero){
  if(fabsf(param)>1){v->fault=1;return;}
  target=param;if(!s->initialized){old=param;s->y1=n->type==FM_OneZero?x:0;}
 }
 if(n->type==FM_Decay || n->type==FM_Decay2){
  if(param<0 || (n->type==FM_Decay2 && third<0)){v->fault=1;return;}
  target=decay_grow(n->type==FM_Decay?param:third,sr);
  target2=n->type==FM_Decay2?decay_grow(param,sr):-1;
  if(!s->initialized){old=n->type==FM_Decay?-1:target;old2=target2;}
 }
 if(!s->initialized){
  if(n->type==FM_LPZ1 || n->type==FM_HPZ1)s->y1=x;
  if(n->type==FM_Impulse){
   s->phase=param-floorf(param);s->last=0;s->freq=x*(1.0f/sr);
   if(s->phase==0 && s->freq>=0)s->phase=1;
  }
 }
 previous_inc=s->freq;
 for(j=0;j<count;j++){
  float a=input(g,b,n,0,j),z=0,t=(float)j/count;
  switch(n->type){
  case FM_OnePole:case FM_OneZero:{
   float coef=old+(target-old)*t;
   z=initializing && n->type==FM_OnePole?a+coef*(s->y1-a):(1-fabsf(coef))*a+coef*s->y1;
   s->y1=n->type==FM_OnePole?z:a;break;
  }
  case FM_LPZ1:case FM_HPZ1:z=0.5f*(a+(n->type==FM_LPZ1?s->y1:-s->y1));s->y1=a;break;
  case FM_Latch:case FM_Gate:{
   float trig=input(g,b,n,1,j);
   if(trig>0 && (n->type==FM_Gate || s->last<=0))s->y1=a;
   s->last=trig;z=s->y1;break;
  }
  case FM_TRand:case FM_TExpRand:{
   float trig=input(g,b,n,2,j);
   if(!s->initialized || (trig>0 && (n->type==FM_TExpRand && n->rate==2?block_trigger:s->last)<=0)){
    float r=random_uniform(v);
    if(n->type==FM_TRand)s->y1=r*(param-x)+x;
    else if(x==0 || param==0 || (x>0)!=(param>0)){v->fault=1;s->y1=0;}
    else s->y1=powf(param/x,r)*x;
   }
   s->last=trig;z=s->y1;break;
  }
  case FM_Dust:case FM_Dust2:{
   float threshold=x*(1.0f/sr),r=random_uniform(v);
   if(r<threshold)z=r*((n->type==FM_Dust?1.0f:2.0f)/threshold)-(n->type==FM_Dust?0.0f:1.0f);
   break;
  }
  case FM_Impulse:{
   float inc;
   z=impulse_wrap(s,previous_inc);
   if(input_rate(g,n,0)==2)inc=a*(1.0f/sr);
   else if(input_rate(g,n,0)==1)inc=s->freq+(x*(1.0f/sr)-s->freq)*((float)(j+1)/count);
   else inc=s->freq;
   lf_advance(s,inc);previous_inc=inc;break;
  }
  case FM_Decay:case FM_Decay2:
   decay_tick(&s->y1,&s->previous[0],a,old+(target-old)*t);
   decay_tick(&s->y2,&s->previous[1],a,old2+(target2-old2)*t);
   z=n->type==FM_Decay?s->y1:s->y1-s->y2;break;
  default:break;
  }
  checked_signal(v,&out[j],z);s->initialized=1;
 }
 s->b1=target;s->b2=target2;
 if(n->type==FM_Impulse){
  s->freq=input(g,b,n,0,count-1)*(1.0f/sr);
  if(initializing){s->phase=param-floorf(param);s->last=0;if(s->phase==0 && s->freq>=0)s->phase=1;}
 }
 if(initializing && (n->type==FM_Latch || n->type==FM_Gate || n->type==FM_Decay || n->type==FM_Decay2)){s->y1=s->y2=s->last=s->previous[0]=s->previous[1]=0;}
}
