/* SPDX-License-Identifier: GPL-3.0-or-later
 * Adapted from SC 3.14.1 LF/Trigger/Noise/Pan/Filter UGens and MiMu.
 * MiMu: Volker Boehm / Emilie Gillet; original MIT notice in reference/mi-ugens.
 * No allocation; reuse the fixed per-node state. */
static float bounded_wrap(float x,float lo,float hi){
 float range=hi-lo;if(!(range>0))return lo;
 x-=lo;return x-range*floorf(x/range)+lo;
}
static float bounded_fold(float x,float lo,float hi){
 float range=hi-lo;if(!(range>0))return lo;
 x=bounded_wrap(x-lo,0,range*2);return lo+(x>range?range*2-x:x);
}
static float mimu(float x){
 int pcm,mask,seg,t;unsigned u;
 x=x<-3?-1:x>3?1:x*(27+x*x)/(27+9*x*x);
 /* 32768*x is exact. Recover the subtraction rounding residual before
    truncation, matching upstream's double product without device doubles. */
 {float hi=x*32768,product=hi-x,low=(hi-product)-x;pcm=(int)product;
  if(product==(float)pcm){if(product>0 && low<0)pcm--;else if(product<0 && low>0)pcm++;}}
 pcm=pcm>=0?pcm/4:-((-pcm+3)/4);
 if(pcm<0){pcm=-pcm;mask=0x7f;}else mask=0xff;
 if(pcm>8159)pcm=8159;
 pcm+=33;for(seg=0;seg<8 && pcm>(0x3f<<seg)+( (1<<seg)-1 );seg++);
 u=seg>=8?(unsigned)(0x7f^mask):(unsigned)(((seg<<4)|((pcm>>(seg+1))&15))^mask);
 u=(~u)&255;t=((int)(u&15)<<3)+132;t<<=(u&0x70)>>4;
 return ((u&128)?132-t:t-132)*(1.0f/32767.0f);
}
static int bounded_integer(fm_voice *v,float x){if(!isfinite(x)||fabsf(x)>8388607){v->fault=1;return 0;}return (int)x;}
static __attribute__((noinline)) void utilities(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int initializing){
 unsigned j,k;float old[6]={0},target[6]={0},left=0,right=0,lstep=0,rstep=0;
 #define U(k) input(g,b,n,k,j)
 for(k=0;k<n->inputs;k++)target[k]=input(g,b,n,k,0);
 if(!s->initialized){
  s->previous[0]=target[1];s->previous[1]=target[2];
  if(n->type==FM_BrownNoise)s->y1=random_bipolar(v);
  if(n->type==FM_PulseDivider)s->env.elapsed=bounded_integer(v,floorf(target[2]+.5f));
  if(n->type==FM_Stepper)s->y1=(float)bounded_integer(v,target[5]);
  if(n->type==FM_Slew || n->type==FM_Lag2 || n->type==FM_Lag3)s->y1=s->y2=s->a0=target[0];
 }
 if(n->type==FM_PulseDivider){(void)bounded_integer(v,target[1]);if(v->fault)return;}
 old[0]=s->previous[0];old[1]=s->previous[1];
 if(n->type==FM_XFade2 || n->type==FM_Balance2 || n->type==FM_LinXFade2){
  float pos=fminf(1,fmaxf(-1,target[2]));
  if(n->type==FM_LinXFade2){right=pos*.5f+.5f;left=1-right;}
  else {unsigned ix=(unsigned)(1024*pos+1024.5f);left=target[3]*sinf((2048-ix)*(PI/4096.0f));right=target[3]*sinf(ix*(PI/4096.0f));}
  if(!s->initialized){s->b1=left;s->b2=right;s->width=target[3];}
  lstep=(left-s->b1)/count;rstep=(right-s->b2)/count;old[2]=left;old[3]=right;left=s->b1;right=s->b2;
 }
 if(n->type==FM_Lag2 || n->type==FM_Lag3){
  if(target[1]<0){v->fault=1;return;}
  target[2]=lag_coeff(target[1],sr);if(!s->initialized)s->b1=target[2];
 }
 if(n->type==FM_LinExp && (target[1]==target[2] || target[3]==0 || target[4]==0 || (target[3]>0)!=(target[4]>0))){v->fault=1;return;}
 for(j=0;j<count;j++){
  float a=n->inputs?U(0):0,z=0;
  switch(n->type){
  case FM_Fold:case FM_Wrap:{
   float lo=(count==1 || input_rate(g,n,1)==2)?U(1):old[0]+(target[1]-old[0])*(float)j/count;
   float hi=(count==1 || input_rate(g,n,2)==2)?U(2):old[1]+(target[2]-old[1])*(float)j/count;
   if(hi<lo){v->fault=1;break;}z=n->type==FM_Fold?bounded_fold(a,lo,hi):bounded_wrap(a,lo,hi);break;
  }
  case FM_BrownNoise:if(!initializing){s->y1+=random_bipolar(v)*.125f;if(s->y1>1)s->y1=2-s->y1;else if(s->y1< -1)s->y1=-2-s->y1;}z=s->y1;break;
  case FM_ClipNoise:z=(random_word(v)&0x80000000u)?-1:1;break;
  case FM_Trig:case FM_Trig1:
   if(s->total)z=--s->total?s->y1:0;
   else if(a>0 && s->last<=0){float duration=target[1]*sr+.5f;if(duration<0 || duration>2147483520.0f){v->fault=1;break;}s->total=(uint32_t)fmaxf(1,duration);s->y1=n->type==FM_Trig?a:1;z=s->y1;}
   s->last=a;break;
  case FM_PulseDivider:
   if(a>0 && s->last<=0){s->env.elapsed++;if(s->env.elapsed>=bounded_integer(v,target[1])){s->env.elapsed=0;z=1;}}s->last=a;break;
  case FM_ToggleFF:if(a>0 && s->last<=0)s->y1=1-s->y1;z=s->y1;s->last=a;break;
  case FM_PulseCount:case FM_SetResetFF:case FM_Stepper:{
   float reset=U(1);int edge=a>0 && s->last<=0,rise=reset>0 && s->y2<=0;
   if((n->type==FM_PulseCount || (n->type==FM_Stepper && n->rate==2)) && input_rate(g,n,1)==0 && !initializing)rise=0;
   if(n->type==FM_Stepper){
    int lo=bounded_integer(v,target[2]),hi=bounded_integer(v,target[3]),step=bounded_integer(v,target[4]),rv=bounded_integer(v,target[5]);
    if(hi<lo){v->fault=1;break;}
    if(rise || edge)s->y1=bounded_wrap(rise?(float)rv:s->y1+step,(float)lo,(float)hi+1);
   }else if(rise)s->y1=0;else if(edge)s->y1=n->type==FM_SetResetFF?1:s->y1+1;
   s->last=a;s->y2=reset;z=s->y1;break;
  }
  case FM_XFade2:case FM_LinXFade2:case FM_Balance2:{
   float l=left,r=right;
   if(input_rate(g,n,2)==2){float pos=fminf(1,fmaxf(-1,U(2)));
    if(n->type==FM_LinXFade2){r=pos*.5f+.5f;l=1-r;}
    else {unsigned ix=(unsigned)(1024*pos+1024.5f);float lev=s->width+(target[3]-s->width)*(float)j/count;l=lev*sinf((2048-ix)*(PI/4096.0f));r=lev*sinf(ix*(PI/4096.0f));}
   }
   if(n->type==FM_Balance2){checked_signal(v,&b->signal[n->output+1][j],U(1)*r);z=a*l;}
   else if(n->type==FM_LinXFade2)z=a+r*(U(1)-a);else z=a*l+U(1)*r;
   left+=lstep;right+=rstep;break;
  }
  case FM_LinExp:{float inv=1/(target[2]-target[1]);z=target[3]*powf(target[4]/target[3],a*inv-target[1]*inv);break;}
  case FM_Slew:{if(target[1]<0 || target[2]<0){v->fault=1;break;}float delta=a-s->y1;delta=fmaxf(-target[2]/sr,fminf(target[1]/sr,delta));s->y1+=delta;z=s->y1;break;}
  case FM_Lag2:case FM_Lag3:{float coef=s->b1+(target[2]-s->b1)*(float)(j+1)/count;s->y1=a+coef*(s->y1-a);s->y2=s->y1+coef*(s->y2-s->y1);if(n->type==FM_Lag3)s->a0=s->y2+coef*(s->a0-s->y2);z=n->type==FM_Lag3?s->a0:s->y2;break;}
  case FM_MiMu:z=target[2]!=0?a:mimu(a*U(1));break;
  default:v->fault=1;break;
  }
  checked_signal(v,&b->signal[n->output][j],z);
 }
 if(n->type==FM_Fold || n->type==FM_Wrap){s->previous[0]=target[1];s->previous[1]=target[2];}
 if(n->type==FM_XFade2 || n->type==FM_LinXFade2 || n->type==FM_Balance2){s->b1=old[2];s->b2=old[3];s->width=target[3];}
 if(n->type==FM_Lag2 || n->type==FM_Lag3)s->b1=target[2];
 s->initialized=1;
 if(initializing){
  if(n->type==FM_Trig || n->type==FM_Trig1 || n->type==FM_ToggleFF || n->type==FM_SetResetFF || n->type==FM_PulseCount || n->type==FM_PulseDivider || n->type==FM_Stepper){s->last=s->y1=s->y2=0;s->total=0;if(n->type==FM_PulseDivider)s->env.elapsed=bounded_integer(v,floorf(target[2]+.5f));if(n->type==FM_Stepper)s->y1=(float)bounded_integer(v,target[5]);}
 }
 #undef U
}
