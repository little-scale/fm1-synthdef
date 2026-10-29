/* SPDX-License-Identifier: GPL-3.0-or-later
 * SC 3.14.1 LFUGens, DynNoiseUGens and FilterUGens; see reference/README.md. */
static void equalizer_coeff(unsigned type,float freq,float rq,float db,float sr,float c[5]){
 float a=powf(10,db*.025f),w=TAU*freq/sr,co=cosf(w),si=sinf(w),sh=sinf(w*.5f),alpha,b;
 if(type==FM_BPeakEQ){alpha=si*.5f*rq;b=1/(1+alpha/a);c[3]=2*b*co;c[0]=(1+alpha*a)*b;c[1]=-c[3];c[2]=(1-alpha*a)*b;c[4]=(1-alpha/a)*b;c[1]=4*b*sh*sh;c[3]=c[1];}
 else {float i=(a+1)*co,j=(a-1)*co,k;alpha=si*.5f*sqrtf((a+1/a)*(rq-1)+2);k=2*sqrtf(a)*alpha;
  if(type==FM_BLowShelf){b=1/((a+1)+j+k);c[0]=a*((a+1)-j+k)*b;c[1]=2*a*((a-1)-i)*b;c[2]=a*((a+1)-j-k)*b;c[3]=2*((a-1)+i)*b;c[4]=((a+1)+j-k)*b;c[1]=8*a*a*b*sh*sh;c[3]=8*b*sh*sh;}
  else {b=1/((a+1)-j+k);c[0]=a*((a+1)+j+k)*b;c[1]=-2*a*((a-1)+i)*b;c[2]=a*((a+1)+j-k)*b;c[3]=-2*((a-1)-i)*b;c[4]=((a+1)-j-k)*b;c[1]=8*a*b*sh*sh;c[3]=8*a*b*sh*sh;}
 }
}
static __attribute__((noinline)) void modulation(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int initializing){
 unsigned j,k,steps=count;float p[4]={0},old[5]={0},target[5]={0};
 for(k=0;k<n->inputs;k++)p[k]=input(g,b,n,k,0);
 if(!s->initialized){
  if(n->type==FM_VarSaw){s->phase=bounded_wrap(p[1],0,1);s->width=fminf(.999f,fmaxf(.001f,p[2]));}
  if(n->type==FM_LFPar)s->phase=p[1];
  if(n->type==FM_LFCub)s->phase=p[1]+.5f;
  if(n->type==FM_LFDNoise1)s->y2=random_bipolar(v);
  if(n->type==FM_LFDNoise3)for(k=0;k<4;k++)s->previous[k]=random_bipolar(v)*.8f;
  if(n->type==FM_Amplitude)s->y1=fabsf(p[0]);
 }
 if(n->type==FM_Amplitude){
  if(p[1]<0 || p[2]<0){v->fault=1;return;}
  target[0]=p[1]==0?0:expf(-2.302585092994046f/(p[1]*sr));target[1]=p[2]==0?0:expf(-2.302585092994046f/(p[2]*sr));
  if(n->rate==1 && input_rate(g,n,0)==2 && !initializing)steps=FM_BLOCK;
 }
 if(n->type>=FM_BPeakEQ){
  if(p[1]<=0 || p[1]>=sr*.5f || p[2]<=0 || fabsf(p[3])>120){v->fault=1;return;}
  if(!s->initialized || p[1]!=s->freq || p[2]!=s->width || p[3]!=s->a0){equalizer_coeff(n->type,p[1],p[2],p[3],sr,target);s->freq=p[1];s->width=p[2];s->a0=p[3];}
  else {for(k=0;k<4;k++)target[k]=s->previous[k];target[4]=s->b2;}
  if(!s->initialized){for(k=0;k<4;k++)s->previous[k]=target[k];s->b2=target[4];}
  for(k=0;k<4;k++)old[k]=s->previous[k];old[4]=s->b2;
 }
 for(j=0;j<steps;j++){
  float a=input(g,b,n,0,j),z=0;
  if(n->type>=FM_VarSaw && n->type<=FM_LFPar && (a<0 || a>sr*.5f)){v->fault=1;break;}
  switch(n->type){
  case FM_VarSaw:{float phase;
   if(lf_ge(s,1)){lf_advance(s,-1);s->width=fminf(.999f,fmaxf(.001f,p[2]));}
   phase=s->phase;z=(phase<s->width?phase*(2/s->width):(1-phase)*(2/(1-s->width)))-1;
   if(!initializing)lf_advance(s,a*(1/sr));break;
  }
  case FM_LFCub:{float q;
   if(lf_ge(s,2))lf_advance(s,-2);
   q=s->phase<1?s->phase:2-s->phase;z=q*q*(6-4*q)-1;
   if(!initializing)lf_advance(s,a*(2/sr));break;
  }
  case FM_LFPar:{float q;
   if(lf_ge(s,3))lf_advance(s,-4);
   if(s->phase<1){q=s->phase;z=1-q*q;}else {q=s->phase-2;z=q*q-1;}
   if(!initializing)lf_advance(s,a*(4/sr));break;
  }
  case FM_SyncSaw:{
   float slave=input(g,b,n,1,j)*(2/sr),master=a*(2/sr);fm_state phase2={0};
   if(slave<0 || slave>1){v->fault=1;break;}
   phase2.phase=s->y1;phase2.last=s->y2;z=phase2.phase;lf_advance(&phase2,slave);if(lf_ge(&phase2,1))lf_advance(&phase2,-2);
   lf_advance(s,master);if(lf_ge(s,1)){lf_advance(s,-2);phase2.phase=(s->phase+1)*slave/master-1;phase2.last=0;}
   s->y1=phase2.phase;s->y2=phase2.last;break;
  }
  case FM_LFDNoise0:case FM_LFDNoise1:case FM_LFDNoise3:
   if(a<0 || a>sr*4){v->fault=1;break;}
   s->phase-=(n->type!=FM_LFDNoise3 || initializing || input_rate(g,n,0)==2)?a*(1/sr):a/sr;if(s->phase<0){s->phase=bounded_wrap(s->phase,0,1);
    if(n->type==FM_LFDNoise0)s->y1=random_bipolar(v);
    else if(n->type==FM_LFDNoise1){s->y1=s->y2;s->y2=random_bipolar(v);}
    else {for(k=0;k<3;k++)s->previous[k]=s->previous[k+1];s->previous[3]=random_bipolar(v)*.8f;}
   }
   if(n->type==FM_LFDNoise0)z=s->y1;
   else if(n->type==FM_LFDNoise1)z=s->y2+s->phase*(s->y1-s->y2);
   else z=delay_cubic(1-s->phase,s->previous[0],s->previous[1],s->previous[2],s->previous[3]);break;
  case FM_Amplitude:{float x=fabsf(a),coef=x<s->y1?target[1]:target[0];s->y1=x+(s->y1-x)*coef;z=s->y1;break;}
  case FM_BPeakEQ:case FM_BLowShelf:case FM_BHiShelf:{
   float c[5],t=count==1?1:(float)(j/3)/(count/3),y;
   for(k=0;k<5;k++)c[k]=old[k]+(target[k]-old[k])*t;
   y=a-c[3]*s->phase+c[4]*s->y2;z=c[0]*y-c[2]*s->y2+c[1]*s->phase;lf_advance(s,y);s->y2=y;break;
  }
  default:v->fault=1;break;
  }
  checked_signal(v,&b->signal[n->output][n->rate==1?0:j],z);
 }
 if(n->type>=FM_BPeakEQ){for(k=0;k<4;k++)s->previous[k]=target[k];s->b2=target[4];}
 s->initialized=1;
}
