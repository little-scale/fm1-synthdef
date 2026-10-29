/* SPDX-License-Identifier: GPL-3.0-or-later
 * Bounded float ports of SC 3.14.1 Osc/Trigger/Delay/FilterUGens.cpp.
 * James McCartney and upstream contributors; MoogFF Federico Fontana /
 * Dan Stowell. Original notices are retained in reference/. */
static float expansion_sine(uint32_t phase){
 unsigned k=(phase>>16)&8191u;float a=fm_sine[k],z=fm_sine[(k+1)&8191u];
 return (2*a-z)+(z-a)*(1+(phase&65535u)*(1.0f/65536.0f));
}
static float expansion_radians(float x){float p=x*85445656.0f;return p+(product_error(x,85445656.0f,p)+x*3.44705395f);}
static void phasor_add(fm_state *s,float x){float z=s->y1+x,b=z-s->y1,e=(s->y1-(z-b))+(x-b)+s->y2;s->y1=z+e;s->y2=e-(s->y1-z);}
static void phasor_wrap(fm_state *s,float lo,float hi){
 float x=s->y1,span=hi-lo;
 if(x>hi || (x==hi&&s->y2>=0) || x<lo || (x==lo&&s->y2<0)){
  float turns=floorf((x-lo)/span);if(x==hi&&s->y2<0)turns=0;if(x==lo&&s->y2<0)turns=-1;
  phasor_add(s,-turns*span);
 }
}
typedef struct {float coef[12],y[12],phasein;uint32_t phase;} shift_state;
typedef char shift_fits_node[(sizeof(shift_state)<=offsetof(fm_state,initialized))?1:-1];
static void expansion_shift(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int init){
 shift_state st;memcpy(&st,s,sizeof st);float freq=input(g,b,n,1,0),phase=input(g,b,n,2,0);
 if(fabsf(phase)>16||fabsf(freq)>sr*.5f){v->fault=1;return;}
 int af=!init&&input_rate(g,n,1)==2,ap=!init&&input_rate(g,n,2)==2;
 if(init){static const float gamma[]={.3609f,2.7412f,11.1573f,44.7581f,179.6242f,798.4578f,1.2524f,5.5671f,22.3423f,89.6271f,364.7914f,2770.1114f};
  for(unsigned k=0;k<12;k++){float c=(15*PI/sr)*gamma[k];st.coef[k]=(c-1)/(c+1);}st.phasein=phase;st.phase=(input_rate(g,n,1)!=2&&input_rate(g,n,2)!=2)?(uint32_t)(int32_t)expansion_radians(phase):0;
 }
 float pm=st.phasein,delta=(phase-pm)/(n->rate==2?FM_BLOCK:1);
 for(unsigned j=0;j<count;j++){
  float f=af?input(g,b,n,1,j):freq,p=ap?input(g,b,n,2,j):pm;
  if(fabsf(f)>sr*.5f||fabsf(p)>16){v->fault=1;return;}
  float x=input(g,b,n,0,j),a=x,z=x;
  for(unsigned k=0;k<12;k++){float in=k<6?a:z,y=in-st.coef[k]*st.y[k],out=st.coef[k]*y+st.y[k];st.y[k]=y;if(k<6)a=out;else z=out;}
  uint32_t off=st.phase,step=af?(uint32_t)(int32_t)((536870912.0f/sr)*f):sine_control_step(f,sr);
  if(af||ap)off+=(uint32_t)(int32_t)expansion_radians(p);
  else step+=(uint32_t)(int32_t)expansion_radians(delta);
  checked_signal(v,&b->signal[n->output][j],a*expansion_sine(off+134217728u)+z*expansion_sine(off));st.phase+=step;pm+=delta;
 }
 st.phasein=phase;memcpy(s,&st,sizeof st);s->initialized=1;
}
static void expansion_render(const fm_graph *g,fm_voice *v,fm_scratch *b,const fm_node *n,fm_state *s,unsigned count,float sr,int init){
 float p[6]={0};for(unsigned k=0;k<n->inputs;k++)p[k]=input(g,b,n,k,0);
 float *out=b->signal[n->output];unsigned j;
 if(n->type==FM_FreqShift){expansion_shift(g,v,b,n,s,count,sr,init);return;}
 if(n->type==FM_SinOscFB){
  if(fabsf(p[0])>sr*.5f||fabsf(p[1])>8){v->fault=1;return;}
  float target=expansion_radians(p[1]);if(!s->initialized)s->last=target;
  float fb=s->last,delta=(target-fb)/(n->rate==2?FM_BLOCK:1);uint32_t step=sine_control_step(p[0],sr);
  for(j=0;j<count;j++){s->y1=expansion_sine(s->phase_acc+(uint32_t)(int32_t)(fb*s->y1));out[j]=s->y1;s->phase_acc+=step;fb+=delta;}
  s->last=fb;if(init)s->phase_acc=0;
 }else if(n->type==FM_Blip){
  float partials=sr*.5f/p[0];if(!(p[0]>0)||p[0]>sr*.5f||partials>16777216||fabsf(p[1])>16777216){v->fault=1;return;}
  int requested=(int)p[1],maximum=(int)partials,nh=requested>maximum?maximum:requested;if(nh<1)nh=1;
  if(!s->initialized){s->env.remaining=nh;s->freq=p[0];s->width=p[1];}
  int old=s->env.remaining;float f=(old!=nh&&requested>maximum)?fmaxf(s->freq,p[0]):p[0];uint32_t step=sine_control_step(f,sr)/2;
  for(j=0;j<count;j++){float x=saw_blip(s->phase_acc,nh,.5f/nh);if(old!=nh){float a=saw_blip(s->phase_acc,old,.5f/old);x=a+(float)j/(n->rate==2?FM_BLOCK:1)*(x-a);}out[j]=x;s->phase_acc+=step;}
  s->env.remaining=nh;s->freq=p[0];s->width=p[1];if(init)s->phase_acc=0;
 }else if(n->type==FM_Phasor){
  if(!(p[3]-p[2]>=1e-6f)||fabsf(p[2])>1e6f||fabsf(p[3])>1e6f||fabsf(p[4])>1e6f){v->fault=1;return;}
  if(!s->initialized)s->y1=p[2];int at=n->rate==1||input_rate(g,n,0)==2;
  for(j=0;j<count;j++){
   float t=input(g,b,n,0,j),rate=input(g,b,n,1,j);if(fabsf(rate)>1024*(p[3]-p[2])){v->fault=1;return;}
   if(s->last<=0&&t>0&&(at||j==0)){s->y1=p[4];s->y2=0;if(at)phasor_add(s,(1-s->last/(t-s->last))*rate);}
   if(!at)phasor_wrap(s,p[2],p[3]);checked_signal(v,out+j,s->y1+s->y2);phasor_add(s,rate);if(at)phasor_wrap(s,p[2],p[3]);s->last=t;
  }
  if(init){s->y1=p[2];s->y2=0;s->last=0;}
 }else if(n->type==FM_MoogFF){
  if(p[1]<0||p[1]>=sr*.5f){v->fault=1;return;}
  float gain=fminf(4,fmaxf(0,p[2]));if(!s->initialized)s->last=p[2];
  if(!s->initialized||p[1]!=s->freq){float t=tanf(PI*p[1]/sr);s->a0=t/(t+1);s->b1=(t-1)/(t+1);s->freq=p[1];}
  if(p[3]>0)memset(s->previous,0,sizeof s->previous);
  float k=s->last,dk=(gain-k)/(n->rate==2?FM_BLOCK:1),b0=s->a0,a1=s->b1,b4=b0*b0*b0*b0;
  for(j=0;j<count;j++){
   float *q=s->previous,x=input(g,b,n,0,j),o=q[3]+b0*(q[2]+b0*(q[1]+b0*q[0])),y=(b4*x+o)/(1+b4*k),u=x-k*y;
   for(unsigned h=0;h<3;h++){float next=b0*u+q[h];q[h]=b0*u-a1*next;u=next;}q[3]=b0*u-a1*y;checked_signal(v,out+j,y);k+=dk;
  }s->last=gain;
 }else if(n->type==FM_Pluck||n->type==FM_Limiter){
  if(!s->total||!v->delay_memory||s->phase_offset>v->delay_capacity||s->total>v->delay_capacity-s->phase_offset){v->fault=1;return;}float *mem=v->delay_memory+s->phase_offset;
  if(n->type==FM_Limiter){
   if(p[1]<0||p[1]>1e6f){v->fault=1;return;}unsigned size=s->total/3;
   if(!s->initialized){s->a0=1;s->dice[0]=0;s->dice[1]=1;s->dice[2]=2;}
   for(j=0;j<count;j++){
    float x=input(g,b,n,0,j);mem[s->dice[0]*size+s->phase_acc]=x;out[j]=s->env.elapsed>=2?s->a0*mem[s->dice[2]*size+s->phase_acc]:0;s->a0+=s->b1;s->y1=fmaxf(s->y1,fabsf(x));
    if(++s->phase_acc==size){s->phase_acc=0;float peak=fmaxf(s->y1,s->y2),next=peak>p[1]?p[1]/peak:1;s->y2=s->y1;s->y1=0;s->b1=(next-s->a0)/size;unsigned t=s->dice[2];s->dice[2]=s->dice[1];s->dice[1]=s->dice[0];s->dice[0]=t;if(s->env.elapsed<2)s->env.elapsed++;}
    checked_signal(v,out+j,out[j]);
   }
  }else{
   if(p[3]<0||p[3]>p[2]||fabsf(p[4])>1e6f||fabsf(p[5])>1){v->fault=1;return;}
   float ds=fminf(s->total,fmaxf(2,p[3]*sr)),fb=delay_feedback(p[3],p[4]);int ac=input_rate(g,n,5)==2,at=input_rate(g,n,1)==2;
   if(init){s->freq=p[3];s->width=p[4];s->last=ds;s->a0=fb;s->b1=p[5];s->initialized=1;out[0]=0;return;}
   float d=s->last,f=s->a0,c=s->b1,dd=(ds-d)/FM_BLOCK,df=(fb-f)/FM_BLOCK,dc=(p[5]-c)/FM_BLOCK;int changed=p[3]!=s->freq||p[4]!=s->width||p[5]!=s->b1;
   for(j=0;j<count;j++){
    float t=input(g,b,n,1,j);if((at||j==0)&&s->previous[0]<=0&&t>0){float a=p[3]*sr,z=a+.5f,lo=product_error(p[3],sr,a)+(a-z+.5f);s->env.remaining=(int32_t)z;if(z==s->env.remaining&&lo<0)s->env.remaining--;}s->previous[0]=t;
    if(changed)d+=dd;float coef=ac?input(g,b,n,5,j):c;if(fabsf(coef)>1){v->fault=1;return;}
    unsigned id=(unsigned)d,read=s->phase_acc-id,mask=s->total-1;float frac=d-id,y=delay_cubic(frac,mem[(read+1)&mask],mem[read&mask],mem[(read-1)&mask],mem[(read-2)&mask]);
    float z=(1-fabsf(coef))*y+coef*s->y1,x=s->env.remaining>0?input(g,b,n,0,j):0;
    if(s->env.remaining>0)s->env.remaining--;if((unsigned)s->env.elapsed<s->total&&s->phase_acc+1<id)z=0;
    mem[s->phase_acc&mask]=x+f*z;checked_signal(v,&mem[s->phase_acc&mask],mem[s->phase_acc&mask]);checked_signal(v,out+j,z);s->y1=z;s->phase_acc++;if((unsigned)s->env.elapsed<s->total)s->env.elapsed++;if(changed){f+=df;c+=dc;}
   }s->last=d;s->a0=f;s->b1=p[5];s->freq=p[3];s->width=p[4];
  }
 }
 s->initialized=1;
}
