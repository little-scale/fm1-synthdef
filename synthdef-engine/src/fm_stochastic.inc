/* SPDX-License-Identifier: GPL-3.0-or-later
 * SC 3.14.1 Noise/DynNoise/Osc/Chaos adaptations; see reference/README.md.
 * All state reuses fm_state. No extra arena, heap, or software doubles. */
#include "fm_chaos.inc"
/* Exact high 32 bits of a 32x32 product, using bounded 16-bit limbs. */
static uint32_t stochastic_mulhi(uint32_t a,uint32_t b){
 uint32_t a0=a&65535u,a1=a>>16,b0=b&65535u,b1=b>>16;
 uint32_t w0=a0*b0,t=a1*b0+(w0>>16),w1=(t&65535u)+a0*b1;
 return a1*b1+(t>>16)+(w1>>16);
}
static float stochastic_integer(fm_voice *v,float lo,float hi){
 if(!isfinite(lo)||!isfinite(hi)||fabsf(lo)>8388607||fabsf(hi)>8388607){v->fault=1;return 0;}
 int l=(int)lo,h=(int)hi;if(h<l){v->fault=1;return 0;}
 return (float)(l+(int)stochastic_mulhi(random_word(v),(uint32_t)(h-l+1)));
}
static float stochastic_hash(float x){
 uint32_t h;memcpy(&h,&x,4);h+=~(h<<15);h^=h>>10;h+=h<<3;h^=h>>6;h+=~(h<<11);h^=h>>16;h=0x40000000u|(h>>9);memcpy(&x,&h,4);return x-3;
}
static void chaos_render(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int init){
 float args[8]={0};for(unsigned k=0;k<n->inputs;k++)args[k]=input(g,b,n,k,0);
 int henon=n->type>=FM_HenonN&&n->type<=FM_HenonC,cusp=n->type==FM_CuspN||n->type==FM_CuspL,lato=n->type>=FM_LatoocarfianN&&n->type<=FM_LatoocarfianC;
 int valid=args[0]>=0&&args[0]<=4*sr;
 if(henon)valid=valid&&fabsf(args[1])<=2&&fabsf(args[2])<=1&&fabsf(args[3])<=1.5f&&fabsf(args[4])<=1.5f;
 else if(cusp)valid=valid&&fabsf(args[1])<=2&&fabsf(args[2])<=2&&fabsf(args[3])<=4;
 else if(lato)valid=valid&&fabsf(args[1])<=3&&fabsf(args[2])<=3&&fabsf(args[3])<=2&&fabsf(args[4])<=2&&fabsf(args[5])<=4&&fabsf(args[6])<=4;
 else if(n->type>=FM_FBSineN && n->type<=FM_FBSineC){for(unsigned k=1;k<7;k++)valid=valid&&fabsf(args[k])<=16;}
 else if(n->type>=FM_StandardN){valid=valid&&fabsf(args[1])<=16&&fabsf(args[2])<=16&&fabsf(args[3])<=16;}
 else {valid=valid&&args[1]>=0&&args[1]<=30&&args[2]>=0&&args[2]<=60&&args[3]>=0&&args[3]<=8&&args[4]>=0&&args[4]<=.05f;for(unsigned k=5;k<8;k++)valid=valid&&fabsf(args[k])<=100;}
 if(!valid){v->fault=1;return;}
 chaos_state state;memcpy(&state,s,sizeof state);chaos_unit unit={&state,args,b->signal[n->output],sr};
 #define CHAOS_CASE(N) case FM_##N:if(init)chaos_##N##_Ctor(&unit);else chaos_##N##_next(&unit,(int)count);break
 switch(n->type){CHAOS_CASE(HenonN);CHAOS_CASE(HenonL);CHAOS_CASE(HenonC);CHAOS_CASE(CuspN);CHAOS_CASE(CuspL);CHAOS_CASE(LatoocarfianN);CHAOS_CASE(LatoocarfianL);CHAOS_CASE(LatoocarfianC);CHAOS_CASE(LorenzL);CHAOS_CASE(FBSineN);CHAOS_CASE(FBSineL);CHAOS_CASE(FBSineC);CHAOS_CASE(StandardN);CHAOS_CASE(StandardL);default:v->fault=1;break;}
 #undef CHAOS_CASE
 if(!isfinite(state.xn)||!isfinite(state.yn)||!isfinite(state.zn)||fabsf(state.xn)>1e6f||fabsf(state.yn)>1e6f||fabsf(state.zn)>1e6f)v->fault=1;
 memcpy(s,&state,sizeof state);s->initialized=1;for(unsigned j=0;j<count;j++)checked_signal(v,&b->signal[n->output][j],b->signal[n->output][j]);
}
static void stochastic_render(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int init){
 if(n->type>=FM_HenonN){chaos_render(g,v,b,n,s,count,sr,init);return;}
 float p[66]={0};for(unsigned k=0;k<n->inputs;k++)p[k]=input(g,b,n,k,0);
 if(n->type==FM_Crackle&&(p[0]<0||p[0]>2)){v->fault=1;return;}
 if((n->type==FM_LFClipNoise||n->type==FM_LFDClipNoise)&&(p[0]<0||p[0]>4*sr)){v->fault=1;return;}
 if(n->type==FM_CoinGate&&(p[0]<0||p[0]>1)){v->fault=1;return;}
 if(n->type==FM_Logistic&&(p[0]<0||p[0]>4||p[1]<0||p[1]>4*sr||p[2]<0||p[2]>1)){v->fault=1;return;}
 float sum=0;
 if(n->type==FM_TWindex){
  for(unsigned k=2;k<n->inputs;k++){if(p[k]<0){v->fault=1;return;}sum+=p[k];}
  if(!(sum>0)||!isfinite(sum)||(p[1]!=0&&p[1]!=1)||(p[1]==0&&fabsf(sum-1)>1e-5f)){v->fault=1;return;}if(p[1]==0)sum=1;
 }
 if(!s->initialized){
  if(n->type==FM_Crackle)s->y1=(float)random_word(v)*(1.0f/4294967296.0f);
  if(n->type==FM_Logistic)s->y1=p[2];
  if(n->type==FM_CoinGate)s->last=p[1];
  if(n->type==FM_TIRand){s->y1=stochastic_integer(v,p[0],p[1]);s->last=p[2];}
  if(n->type==FM_TWindex)s->last=1;
 }
 for(unsigned j=0;j<count;j++){
  float a=n->inputs?input(g,b,n,0,j):0,z=0;
  switch(n->type){
  case FM_Crackle:z=fabsf(s->y1*p[0]-s->y2-.05f);s->y2=s->y1;s->y1=z;break;
  case FM_GrayNoise:{s->phase_acc^=1u<<(random_word(v)&31u);int32_t word;memcpy(&word,&s->phase_acc,4);z=(float)word*4.65661287308e-10f;break;}
  case FM_Hasher:z=stochastic_hash(a);break;
  case FM_LFClipNoise:
   if(s->env.remaining<=0){s->env.remaining=(int32_t)fmaxf(1,fminf(1e9f,sr/fmaxf(p[0],.001f)));s->y1=(random_word(v)&0x80000000u)?-1:1;}z=s->y1;s->env.remaining--;break;
  case FM_LFDClipNoise:
   if(a<0||a>4*sr){v->fault=1;break;}s->phase-=a*(1/sr);if(s->phase<0){s->phase=bounded_wrap(s->phase,0,1);s->y1=(random_word(v)&0x80000000u)?-1:1;}z=s->y1;break;
  case FM_CoinGate:{float t=input(g,b,n,1,j);if(!init&&t>0&&s->last<=0&&random_uniform(v)<p[0])z=t;s->last=t;break;}
  case FM_TIRand:{float t=input(g,b,n,2,j);if(!init&&t>0&&s->last<=0)s->y1=stochastic_integer(v,p[0],p[1]);s->last=t;z=s->y1;break;}
  case FM_TWindex:
   if(init||(a>0&&s->last<=0)){float draw=sum*random_uniform(v),acc=0;s->y1=(float)(n->inputs-3);for(unsigned k=2;k<n->inputs;k++){acc+=p[k];if(acc>=draw){s->y1=(float)(k-2);break;}}}if(!init)s->last=a;z=s->y1;break;
  case FM_Logistic:
   if(init||s->env.remaining<=0){s->y1=p[0]*s->y1*(1-s->y1);s->env.remaining=(int32_t)fmaxf(1,fminf(1e9f,sr/fmaxf(p[1],.001f)));}z=s->y1;if(!init)s->env.remaining--;break;
  default:v->fault=1;break;
  }
  checked_signal(v,&b->signal[n->output][j],z);
 }
 if(init&&n->type==FM_Logistic)s->env.remaining=0;
 s->initialized=1;
}
